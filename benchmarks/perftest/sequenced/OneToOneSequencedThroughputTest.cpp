// 1:1 port of com.lmax.disruptor.sequenced.OneToOneSequencedThroughputTest
// Source: reference/disruptor/src/perftest/java/com/lmax/disruptor/sequenced/OneToOneSequencedThroughputTest.java
//
// UniCast a series of items between 1 publisher and 1 event processor.
//
// +----+    +-----+
// | P1 |--->| EP1 |
// +----+    +-----+
//
// Disruptor:
// ==========
//              track to prevent wrap
//              +------------------+
//              |                  |
//              |                  v
// +----+    +====+    +====+   +-----+
// | P1 |--->| RB |<---| SB |   | EP1 |
// +----+    +====+    +====+   +-----+
//      claim      get    ^        |
//                        |        |
//                        +--------+
//                          waitFor
//
// P1  - Publisher 1
// RB  - RingBuffer
// SB  - SequenceBarrier
// EP1 - EventProcessor 1

#include <benchmark/benchmark.h>

#include "disruptor/BatchEventProcessor.h"
#include "disruptor/BatchEventProcessorBuilder.h"
#include "disruptor/RingBuffer.h"
#include "disruptor/YieldingWaitStrategy.h"

#include "perftest/support/PerfTestContext.h"
#include "perftest/support/PerfTestUtil.h"
#include "perftest/support/ValueAdditionEventHandler.h"
#include "perftest/support/ValueEvent.h"

#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <print>
#include <thread>
#include <exception>

namespace {

constexpr int kBufferSize = 1024 * 64;
constexpr int64_t kIterations = 1000L * 1000L * 100L; // 100 million
constexpr int kRuns = 7; // Match Java's AbstractPerfTestDisruptor.RUNS = 7

// 1:1 port of OneToOneSequencedThroughputTest class structure
class OneToOneSequencedThroughputTest {
public:
  // Type aliases for convenience
  using PerfTestContext = disruptor::bench::perftest::PerfTestContext;
  using ValueEvent = disruptor::bench::perftest::support::ValueEvent;
  using ValueAdditionEventHandler = disruptor::bench::perftest::support::ValueAdditionEventHandler;
  using RingBufferType = disruptor::RingBuffer<ValueEvent, disruptor::SingleProducerSequencer<disruptor::YieldingWaitStrategy>>;
  using BarrierType = disruptor::ProcessingSequenceBarrier<disruptor::SingleProducerSequencer<disruptor::YieldingWaitStrategy>, disruptor::YieldingWaitStrategy>;
  using BatchProcessorType = disruptor::BatchEventProcessor<ValueEvent, BarrierType>;
  
  OneToOneSequencedThroughputTest()
      : expectedResult_(disruptor::bench::perftest::support::accumulatedAddition(kIterations)),
        ws_(),
        ringBuffer_(disruptor::SingleProducerRingBuffer<
                    ValueEvent, disruptor::YieldingWaitStrategy>::createSingleProducer(
            ValueEvent::EVENT_FACTORY, kBufferSize, ws_)),
        sequenceBarrier_(ringBuffer_->newBarrier()),
        handler_(std::make_shared<ValueAdditionEventHandler>()),
        batchEventProcessor_([this]() {
          disruptor::BatchEventProcessorBuilder builder;
          return builder.build(*ringBuffer_, *sequenceBarrier_, *handler_);
        }()) {
    ringBuffer_->addGatingSequences(batchEventProcessor_->getSequence());
  }

  int getRequiredProcessorCount() const { return 2; }

  PerfTestContext runDisruptorPass() {
    using namespace disruptor::bench::perftest;
    using namespace disruptor::bench::perftest::support;

    PerfTestContext perfTestContext;
    
    // Ensure processor is stopped before starting a new run
    // (Java's executor.submit will start a new run even if previous one finished)
    if (batchEventProcessor_->isRunning()) {
      batchEventProcessor_->halt();
      // Wait for processor to actually stop (C++ requires explicit thread management)
      // Java's executor handles this automatically, but in C++ we must wait
      while (batchEventProcessor_->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
    
    // Use atomic bool as latch (CountDownLatch equivalent)
    auto latch = std::make_shared<std::atomic<bool>>(false);
    
    // Java: expectedCount = batchEventProcessor.getSequence().get() + ITERATIONS
    int64_t currentSequence = batchEventProcessor_->getSequence().get();
    int64_t expectedCount = currentSequence + kIterations;
    handler_->reset(latch, expectedCount);

    std::atomic<bool> threadError{false};
    std::string threadErrorMsg;
    
    // RAII wrapper to ensure thread is always joined (C++ exception safety)
    // In Java, executor manages thread lifecycle automatically
    struct ThreadGuard {
      std::thread t;
      ~ThreadGuard() {
        if (t.joinable()) {
          t.join();
        }
      }
    };
    
    // Start processor in separate thread (Java: executor.submit(batchEventProcessor))
    ThreadGuard processorThreadGuard;
    processorThreadGuard.t = std::thread([this, &threadError, &threadErrorMsg]() {
      try {
        batchEventProcessor_->run();
      } catch (const std::exception& e) {
        threadError = true;
        threadErrorMsg = e.what();
      } catch (...) {
        threadError = true;
        threadErrorMsg = "Unknown exception";
      }
    });

    // Java: long start = System.currentTimeMillis();
    auto start = std::chrono::steady_clock::now();

    // Producer loop
    // Java: for (long i = 0; i < ITERATIONS; i++) { long next = rb.next(); rb.get(next).setValue(i); rb.publish(next); }
    for (int64_t i = 0; i < kIterations; i++) {
      int64_t next = ringBuffer_->next();
      ringBuffer_->get(next).setValue(i);
      ringBuffer_->publish(next);
    }

    // Wait for completion (Java: latch.await())
    while (!latch->load(std::memory_order_acquire)) {
      if (threadError.load()) {
        // Halt processor before throwing (ensure cleanup)
        batchEventProcessor_->halt();
        throw std::runtime_error("Processor thread error: " + threadErrorMsg);
      }
      std::this_thread::yield();
    }

    // Java: perfTestContext.setDisruptorOps((ITERATIONS * 1000L) / (System.currentTimeMillis() - start));
    // System.currentTimeMillis() returns milliseconds, so we need to match that
    auto end = std::chrono::steady_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                         end - start)
                         .count();
    if (elapsedMs == 0) elapsedMs = 1; // Avoid division by zero

    perfTestContext.setDisruptorOps((kIterations * 1000L) / elapsedMs);
    
    // Java: perfTestContext.setBatchData(handler.getBatchesProcessed(), ITERATIONS);
    // Read batchesProcessed after latch.await() (all events are processed)
    int64_t batchesProcessed = handler_->getBatchesProcessed();
    perfTestContext.setBatchData(batchesProcessed, kIterations);
    
    // Java: waitForEventProcessorSequence(expectedCount)
    while (batchEventProcessor_->getSequence().get() != expectedCount) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Java: batchEventProcessor.halt()
    batchEventProcessor_->halt();
    
    // Wait for processor thread to actually stop (C++ requires explicit wait)
    // ThreadGuard destructor will ensure join() even if exception is thrown
    while (batchEventProcessor_->isRunning()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Verify result (Java: failIfNot(expectedResult, handler.getValue()))
    failIfNot(expectedResult_, handler_->getValue());

    return perfTestContext;
  }

  void testImplementations() {
    using namespace disruptor::bench::perftest;
    using namespace disruptor::bench::perftest::support;

    const int availableProcessors = std::thread::hardware_concurrency();
    if (getRequiredProcessorCount() > availableProcessors) {
      std::print("*** Warning ***: your system has insufficient processors to execute the test efficiently. ");
      std::print("Processors required = {} available = {}\n", getRequiredProcessorCount(), availableProcessors);
    }

    std::array<PerfTestContext, kRuns> contexts;

    std::print("Starting Disruptor tests\n");
    for (int i = 0; i < kRuns; i++) {
      // Java: System.gc(); (C++ doesn't have explicit GC, but we can hint)
      // Note: In C++, we rely on RAII and the fact that runDisruptorPass() cleans up threads
      
      PerfTestContext context = runDisruptorPass();
      contexts[i] = context;
      
      // Java: System.out.format("Run %d, Disruptor=%,d ops/sec BatchPercent=%.2f%% AverageBatchSize=%,d\n", ...)
      int64_t ops = context.getDisruptorOps();
      double batchPercent = context.getBatchPercent() * 100.0;
      double avgBatchSize = context.getAverageBatchSize();
      std::print("Run {}, Disruptor={} ops/sec BatchPercent={:.2f}% AverageBatchSize={:.0f}\n",
                 i, ops, batchPercent, avgBatchSize);
    }
  }

private:
  const int64_t expectedResult_;
  disruptor::YieldingWaitStrategy ws_;
  std::shared_ptr<RingBufferType> ringBuffer_;
  std::shared_ptr<BarrierType> sequenceBarrier_;
  std::shared_ptr<ValueAdditionEventHandler> handler_;
  std::shared_ptr<BatchProcessorType> batchEventProcessor_;
};

} // namespace

// Google Benchmark wrapper that mimics Java's main() -> testImplementations() flow
// 1:1 port: Java's main() creates test instance and calls testImplementations()
// testImplementations() loops 7 times, calling runDisruptorPass() each time
// 
// Implementation: Each Google Benchmark iteration = one Java run (7 iterations total)
// We use a static test instance to match Java's structure (one instance, reused across runs)
void PerfTest_OneToOneSequencedThroughputTest(benchmark::State& state) {
  using namespace disruptor::bench::perftest;
  using namespace disruptor::bench::perftest::support;
  
  // Create test instance once (like Java's main() creates test instance)
  // The instance is reused across all 7 runs, matching Java's structure
  // Use unique_ptr for RAII (C++ best practice: avoid raw pointers)
  static std::unique_ptr<OneToOneSequencedThroughputTest> test;
  static bool headerPrinted = false;
  static int runCounter = 0;
  
  if (!test) {
    test = std::make_unique<OneToOneSequencedThroughputTest>();
    
    // Print header and processor check (like Java's testImplementations())
    const int availableProcessors = std::thread::hardware_concurrency();
    if (test->getRequiredProcessorCount() > availableProcessors) {
      std::print("*** Warning ***: your system has insufficient processors to execute the test efficiently. ");
      std::print("Processors required = {} available = {}\n", test->getRequiredProcessorCount(), availableProcessors);
    }
    if (!headerPrinted) {
      std::print("Starting Disruptor tests\n");
      headerPrinted = true;
    }
  }

  // Each iteration is one run (Java's runDisruptorPass())
  // Java does System.gc() before each run, but C++ doesn't have explicit GC
  for (auto _ : state) {
    PerfTestContext context = test->runDisruptorPass();
    
    // Print result in Java's format: "Run %d, Disruptor=%,d ops/sec BatchPercent=%.2f%% AverageBatchSize=%,d\n"
    int64_t ops = context.getDisruptorOps();
    double batchPercent = context.getBatchPercent() * 100.0;
    double avgBatchSize = context.getAverageBatchSize();
    std::print("Run {}, Disruptor={} ops/sec BatchPercent={:.2f}% AverageBatchSize={:.0f}\n",
               runCounter++,
               ops, batchPercent, avgBatchSize);
    
    // Report to Google Benchmark for statistics
    state.counters["ops_per_sec"] =
        benchmark::Counter(context.getDisruptorOps(),
                           benchmark::Counter::kIsRate);
    state.counters["batch_percent"] =
        benchmark::Counter(context.getBatchPercent() * 100.0);
    state.counters["avg_batch_size"] =
        benchmark::Counter(context.getAverageBatchSize());
  }
  
  // Cleanup after all runs (when benchmark is done)
  // unique_ptr automatically handles deletion (RAII)
  if (state.iterations() >= kRuns) {
    test.reset();
    headerPrinted = false;
  }
}

static auto* bm_PerfTest_OneToOneSequencedThroughputTest = [] {
  auto* b = benchmark::RegisterBenchmark("PerfTest_OneToOneSequencedThroughputTest",
                                         &PerfTest_OneToOneSequencedThroughputTest);
  return b->Unit(benchmark::kMillisecond)->Iterations(kRuns); // Match Java's RUNS = 7
}();

