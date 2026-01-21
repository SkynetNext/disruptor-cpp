#include <benchmark/benchmark.h>

#include "jmh_config.h"
#include "jmh_util.h"

#include "disruptor/BusySpinWaitStrategy.h"
#include "disruptor/dsl/Disruptor.h"
#include "disruptor/util/DaemonThreadFactory.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

namespace {
// Java: reference/disruptor/src/jmh/java/com/lmax/disruptor/util/Constants.java
constexpr int kRingBufferSize = 1 << 20;

// Static Disruptor instance (matches Java @State(Scope.Thread) - but we use Benchmark scope for shared consumer)
using DisruptorType = disruptor::dsl::Disruptor<disruptor::bench::jmh::SimpleEvent,
                                                  disruptor::dsl::ProducerType::SINGLE,
                                                  disruptor::BusySpinWaitStrategy>;
using RingBufferType = disruptor::RingBuffer<disruptor::bench::jmh::SimpleEvent,
                                              disruptor::SingleProducerSequencer<disruptor::BusySpinWaitStrategy>>;

// Static instance shared across all iterations (Java: @State(Scope.Thread) but Disruptor is shared)
static std::mutex g_init_mutex;
static DisruptorType* g_disruptor = nullptr;
static std::shared_ptr<RingBufferType> g_ringBuffer = nullptr;
static std::atomic<bool> g_initialized{false};
static disruptor::bench::jmh::ConsumeHandler* g_handler = nullptr; // Keep handler alive
} // namespace

// Setup function (1:1 with Java @Setup)
// Java: @Setup - creates Disruptor with consumer (called once per benchmark)
static void Setup_SPSC_producing(const benchmark::State& state) {
  if (state.thread_index() != 0) return; // Only thread 0 does setup
  
  std::lock_guard<std::mutex> lock(g_init_mutex);
  if (!g_initialized.load(std::memory_order_relaxed)) {
    try {
      disruptor::BusySpinWaitStrategy ws;
      auto factory = std::make_shared<disruptor::bench::jmh::SimpleEventFactory>();
      auto& threadFactory = disruptor::util::DaemonThreadFactory::INSTANCE();

      // Create Disruptor (matches Java @Setup)
      // Java: disruptor = new Disruptor<>(SimpleEvent::new, Constants.RINGBUFFER_SIZE, DaemonThreadFactory.INSTANCE, ProducerType.SINGLE, new BusySpinWaitStrategy());
      g_disruptor = new DisruptorType(factory, kRingBufferSize, threadFactory, ws);

      // Add consumer handler (1:1 with Java: disruptor.handleEventsWith(new SimpleEventHandler(bh)))
      // Handler must live for the entire Disruptor lifetime, so store it statically
      g_handler = new disruptor::bench::jmh::ConsumeHandler();
      g_disruptor->handleEventsWith(*g_handler);

      // Start consumer thread (1:1 with Java: ringBuffer = disruptor.start())
      g_ringBuffer = g_disruptor->start();
      
      g_initialized.store(true, std::memory_order_release);
    } catch (const std::exception& e) {
      // Setup errors are hard to report, but will be caught in benchmark body
      g_initialized.store(false, std::memory_order_release);
    }
  }
}

// Teardown function (1:1 with Java @TearDown)
// Java: @TearDown - disruptor.shutdown() (called once per benchmark)
static void Teardown_SPSC_producing(const benchmark::State& state) {
  if (state.thread_index() != 0) return; // Only thread 0 does teardown
  
  // Use try_lock to avoid blocking if another thread is in Setup
  std::unique_lock<std::mutex> lock(g_init_mutex, std::try_to_lock);
  if (!lock.owns_lock()) {
    // Another thread is in Setup, skip teardown (will be cleaned up later)
    return;
  }
  
  if (g_initialized.load(std::memory_order_relaxed) && g_disruptor != nullptr) {
    try {
      // Java: disruptor.shutdown() - waits for backlog to drain, then halts and joins threads
      g_disruptor->halt(); // halt() stops processors and joins threads
      delete g_disruptor;
      g_disruptor = nullptr;
      delete g_handler;
      g_handler = nullptr;
      g_ringBuffer = nullptr;
      g_initialized.store(false, std::memory_order_release);
    } catch (...) {
      // Best-effort cleanup
      g_disruptor = nullptr;
      g_handler = nullptr;
      g_ringBuffer = nullptr;
      g_initialized.store(false, std::memory_order_release);
    }
  }
}

// 1:1 with Java JMH class:
// reference/disruptor/src/jmh/java/com/lmax/disruptor/SingleProducerSingleConsumer.java
static void
JMH_SingleProducerSingleConsumer_producing(benchmark::State &state) {
  const auto wait_entries_before =
      disruptor::sp_wrap_wait_entries().load(std::memory_order_relaxed);
  const auto wait_loops_before =
      disruptor::sp_wrap_wait_loops().load(std::memory_order_relaxed);

  // Ensure all threads see the ringBuffer (wait for initialization)
  while (!g_initialized.load(std::memory_order_acquire) || g_ringBuffer == nullptr) {
    std::this_thread::yield();
  }

  // Check if initialization failed
  if (g_ringBuffer == nullptr) {
    state.SkipWithError("RingBuffer is null after initialization");
    return;
  }

  // 1:1 with Java benchmark body:
  //   long sequence = ringBuffer.next();
  //   SimpleEvent e = ringBuffer.get(sequence);
  //   e.setValue(0);
  //   ringBuffer.publish(sequence);
  for (auto _ : state) {
    const int64_t sequence = g_ringBuffer->next();
    auto &e = g_ringBuffer->get(sequence);
    e.value = 0;
    g_ringBuffer->publish(sequence);
  }

  const auto wait_entries_after =
      disruptor::sp_wrap_wait_entries().load(std::memory_order_relaxed);
  const auto wait_loops_after =
      disruptor::sp_wrap_wait_loops().load(std::memory_order_relaxed);

  const double entries =
      static_cast<double>(wait_entries_after - wait_entries_before);
  const double loops =
      static_cast<double>(wait_loops_after - wait_loops_before);
  const double ops = static_cast<double>(state.iterations());

  // Raw totals (per repetition) for debugging.
  state.counters["sp_wrap_wait_entries"] =
      benchmark::Counter(entries, benchmark::Counter::kAvgThreads);
  state.counters["sp_wrap_wait_loops"] =
      benchmark::Counter(loops, benchmark::Counter::kAvgThreads);

  // Per-operation rates (more interpretable than raw counts).
  if (ops > 0) {
    state.counters["sp_wrap_wait_entries_per_op"] =
        benchmark::Counter(entries / ops, benchmark::Counter::kAvgThreads);
    state.counters["sp_wrap_wait_loops_per_op"] =
        benchmark::Counter(loops / ops, benchmark::Counter::kAvgThreads);
  }
}

static auto *bm_JMH_SingleProducerSingleConsumer_producing = [] {
  auto *b =
      benchmark::RegisterBenchmark("JMH_SingleProducerSingleConsumer_producing",
                                   &JMH_SingleProducerSingleConsumer_producing);
  b->Setup(Setup_SPSC_producing);  // 1:1 with Java @Setup
  b->Teardown(Teardown_SPSC_producing);  // 1:1 with Java @TearDown
  return disruptor::bench::jmh::applyJmhDefaults(b);
}();
