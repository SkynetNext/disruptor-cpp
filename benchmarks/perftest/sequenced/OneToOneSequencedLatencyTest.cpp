// SPSC Ping-Pong (Round-Trip) Latency Test
//
// Measures RTT/2 as one-way latency using the standard ping-pong method:
//
//   Producer (Thread A)           Consumer (Thread B)
//       |                              |
//       |--- rb1.publish(seq) -------->|  (forward)
//       |                              |
//       |<----- rb2.publish(seq) ------|  (echo back)
//       |   wait rb2                   |
//       |   record RTT                 |
//
// Two Disruptor RingBuffers form the round-trip channel.
// Each iteration = one full round trip. RTT/2 = one-way latency.
//
// Also includes an atomic ping-pong baseline (no Disruptor) to isolate
// hardware cross-core cache-line transfer cost.
//
// Methodology (same as rigtorp/SPSCQueue, Aeron IPC, LMAX Java):
//   - rdtsc before send, rdtscp after receiving echo
//   - CPU affinity: adjacent physical cores, same socket
//   - 500K warmup, 10M measurement iterations

#include <benchmark/benchmark.h>

#include "disruptor/BatchEventProcessor.h"
#include "disruptor/BatchEventProcessorBuilder.h"
#include "disruptor/BusySpinWaitStrategy.h"
#include "disruptor/EventFactory.h"
#include "disruptor/EventHandler.h"
#include "disruptor/RingBuffer.h"

#include <array>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

#ifdef __linux__
#  include <sched.h>
#endif

#ifdef __x86_64__
#  include <x86intrin.h>
#endif

namespace {

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
constexpr int kBufferSize = 1 << 23;  // 8M, matches SPSCQueue's 10M queue size
constexpr int64_t kWarmupIterations = 500'000;
constexpr int64_t kMeasureIterations = 10'000'000;
constexpr int kRuns = 5;
constexpr int kMessageLength = 100;   // Payload size in bytes

// Default CPU cores; overridden by benchmark Args(cpu1, cpu2).
constexpr int kDefaultProducerCpu = 20;
constexpr int kDefaultConsumerCpu = 68;

// ---------------------------------------------------------------------------
// rdtsc
// ---------------------------------------------------------------------------
#ifdef __x86_64__
inline uint64_t rdtsc_fenced() {
  _mm_lfence();
  return __rdtsc();
}
inline uint64_t rdtscp_fenced() {
  unsigned int aux;
  uint64_t tsc = __rdtscp(&aux);
  _mm_lfence();
  return tsc;
}
#else
inline uint64_t rdtsc_fenced() {
  return static_cast<uint64_t>(
    std::chrono::steady_clock::now().time_since_epoch().count());
}
inline uint64_t rdtscp_fenced() { return rdtsc_fenced(); }
#endif

double calibrateTscGHz() {
#ifdef __x86_64__
  auto t0 = std::chrono::steady_clock::now();
  uint64_t tsc0 = __rdtsc();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  uint64_t tsc1 = __rdtsc();
  auto t1 = std::chrono::steady_clock::now();
  double ns = static_cast<double>(
    std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
  return static_cast<double>(tsc1 - tsc0) / ns;
#else
  return 1.0;
#endif
}

// ---------------------------------------------------------------------------
// CPU affinity
// ---------------------------------------------------------------------------
bool pinToCore([[maybe_unused]] int cpu) {
#ifdef __linux__
  if (cpu < 0) return false;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  return sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == 0;
#else
  return false;
#endif
}

// ---------------------------------------------------------------------------
// Event / Factory (shared by both directions)
// ---------------------------------------------------------------------------
struct PingEvent {
  std::array<uint8_t, kMessageLength> data{};
};

struct PingEventFactory : public disruptor::EventFactory<PingEvent> {
  PingEvent newInstance() override { return PingEvent{}; }
};

static auto PING_EVENT_FACTORY = std::make_shared<PingEventFactory>();

// ---------------------------------------------------------------------------
// Latency result
// ---------------------------------------------------------------------------
struct LatencyResult {
  double min_ns;
  double max_ns;
  double mean_ns;
  double p50_ns;
  double p90_ns;
  double p99_ns;
  double p999_ns;
};

LatencyResult computePercentiles(std::vector<uint64_t>& latencies, double nsPerCycle) {
  std::sort(latencies.begin(), latencies.end());
  auto n = static_cast<int64_t>(latencies.size());
  if (n == 0) return {0, 0, 0, 0, 0, 0, 0};

  auto toNs = [nsPerCycle](uint64_t cycles) -> double {
    return static_cast<double>(cycles) * nsPerCycle;
  };

  uint64_t sum = 0;
  for (auto v : latencies) sum += v;

  return {
    toNs(latencies.front()),
    toNs(latencies.back()),
    toNs(sum / static_cast<uint64_t>(n)),
    toNs(latencies[static_cast<size_t>(n * 50 / 100)]),
    toNs(latencies[static_cast<size_t>(n * 90 / 100)]),
    toNs(latencies[static_cast<size_t>(n * 99 / 100)]),
    toNs(latencies[static_cast<size_t>(n * 999 / 1000)]),
  };
}

void reportResult(benchmark::State& state, const LatencyResult& r, int& run,
                  const char* label) {
  // Report RTT/2 (one-way)
  std::fprintf(stderr,
    "Run %d [%s RTT/2]: min=%.0fns  p50=%.0fns  p90=%.0fns  p99=%.0fns  "
    "p99.9=%.0fns  max=%.0fns  mean=%.0fns\n",
    run++, label,
    r.min_ns / 2, r.p50_ns / 2, r.p90_ns / 2, r.p99_ns / 2,
    r.p999_ns / 2, r.max_ns / 2, r.mean_ns / 2);

  state.counters["rtt_min_ns"] = benchmark::Counter(r.min_ns);
  state.counters["rtt_p50_ns"] = benchmark::Counter(r.p50_ns);
  state.counters["rtt_p90_ns"] = benchmark::Counter(r.p90_ns);
  state.counters["rtt_p99_ns"] = benchmark::Counter(r.p99_ns);
  state.counters["rtt_p999_ns"] = benchmark::Counter(r.p999_ns);
  state.counters["rtt_max_ns"] = benchmark::Counter(r.max_ns);
  state.counters["rtt_mean_ns"] = benchmark::Counter(r.mean_ns);
  // One-way = RTT/2
  state.counters["latency_p50_ns"] = benchmark::Counter(r.p50_ns / 2);
  state.counters["latency_mean_ns"] = benchmark::Counter(r.mean_ns / 2);
}

// ===================================================================
// Test 1: Disruptor Ping-Pong (two RingBuffers)
//
//   rb1: Producer → Consumer (forward path)
//   rb2: Consumer → Producer (echo path)
//
// Consumer uses BatchEventProcessor on rb1.
// On receiving an event, consumer echoes via rb2.publish().
// Producer measures RTT = time from rb1.publish to rb2 response.
// ===================================================================

// Consumer handler: on receiving event from rb1, echo back via rb2.
template <typename RingBufferType>
class EchoHandler : public disruptor::EventHandler<PingEvent> {
public:
  explicit EchoHandler(std::shared_ptr<RingBufferType> replyBuffer)
    : replyBuffer_(std::move(replyBuffer)) {}

  void onEvent(PingEvent& event, int64_t sequence, bool /*endOfBatch*/) override {
    // Echo: publish the same payload back on rb2
    int64_t replySeq = replyBuffer_->next();
    std::memcpy(replyBuffer_->get(replySeq).data.data(), event.data.data(), kMessageLength);
    replyBuffer_->publish(replySeq);
  }

private:
  std::shared_ptr<RingBufferType> replyBuffer_;
};

class DisruptorPingPongTest {
public:
  using WS = disruptor::BusySpinWaitStrategy;
  using SeqT = disruptor::SingleProducerSequencer<WS>;
  using RB = disruptor::RingBuffer<PingEvent, SeqT>;
  using BarrierT = disruptor::ProcessingSequenceBarrier<SeqT, WS>;
  using ProcessorT = disruptor::BatchEventProcessor<PingEvent, BarrierT>;

  DisruptorPingPongTest()
    : cyclesPerNs_(calibrateTscGHz())
    // rb1: producer → consumer
    , ws1_()
    , rb1_(disruptor::SingleProducerRingBuffer<PingEvent, WS>::createSingleProducer(
        PING_EVENT_FACTORY, kBufferSize, ws1_))
    , barrier1_(rb1_->newBarrier())
    // rb2: consumer → producer (echo)
    , ws2_()
    , rb2_(disruptor::SingleProducerRingBuffer<PingEvent, WS>::createSingleProducer(
        PING_EVENT_FACTORY, kBufferSize, ws2_))
    , echoHandler_(std::make_shared<EchoHandler<RB>>(rb2_))
    , processor1_([this]() {
      disruptor::BatchEventProcessorBuilder builder;
      return builder.build(*rb1_, *barrier1_, *echoHandler_);
    }()) {
    rb1_->addGatingSequences(processor1_->getSequence());
    std::fprintf(stderr, "TSC calibration: %.3f GHz (%.2f ns/cycle)\n",
                 cyclesPerNs_, 1.0 / cyclesPerNs_);
  }

  LatencyResult runLatencyPass(int producerCpu, int consumerCpu) {
    if (processor1_->isRunning()) {
      processor1_->halt();
      while (processor1_->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }

    int64_t total = kWarmupIterations + kMeasureIterations;
    latencies_.clear();
    latencies_.reserve(static_cast<size_t>(kMeasureIterations));

    // Start consumer thread with BatchEventProcessor on rb1
    std::atomic<bool> consumerReady{false};
    std::atomic<bool> threadError{false};
    std::thread consumerThread([this, consumerCpu, &consumerReady, &threadError]() {
      pinToCore(consumerCpu);
      consumerReady.store(true, std::memory_order_release);
      try { processor1_->run(); }
      catch (...) { threadError = true; }
    });

    pinToCore(producerCpu);
    while (!consumerReady.load(std::memory_order_acquire)) { _mm_pause(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // Producer: send on rb1, wait for echo on rb2.
    // Embed TSC timestamp in payload so RTT = tsc_end - tsc_embedded.
    // This eliminates the overlap artifact where consumer processes
    // the echo during the producer's publish overhead.
    int64_t replySeq = rb2_->getCursor() + 1;  // continue from previous run
    for (int64_t i = 0; i < total; i++) {
      uint64_t tsc_start = rdtsc_fenced();

      // Send — embed timestamp in first 8 bytes of payload
      int64_t seq = rb1_->next();
      std::memcpy(rb1_->get(seq).data.data(), &tsc_start, sizeof(tsc_start));
      rb1_->publish(seq);

      // Wait for echo on rb2
      while (rb2_->getCursor() < replySeq) {
        _mm_pause();
      }

      uint64_t tsc_end = rdtscp_fenced();

      // Extract embedded timestamp from echoed payload
      uint64_t tsc_sent;
      std::memcpy(&tsc_sent, rb2_->get(replySeq).data.data(), sizeof(tsc_sent));
      replySeq++;

      // Record RTT (skip warmup)
      if (i >= kWarmupIterations) {
        latencies_.push_back(tsc_end - tsc_sent);
      }
    }

    processor1_->halt();
    consumerThread.join();

    return computePercentiles(latencies_, 1.0 / cyclesPerNs_);
  }

private:
  double cyclesPerNs_;
  WS ws1_;
  std::shared_ptr<RB> rb1_;
  std::shared_ptr<BarrierT> barrier1_;
  WS ws2_;
  std::shared_ptr<RB> rb2_;
  std::shared_ptr<EchoHandler<RB>> echoHandler_;
  std::shared_ptr<ProcessorT> processor1_;
  std::vector<uint64_t> latencies_;
};

// ===================================================================
// Test 2: Atomic Ping-Pong Baseline (no Disruptor)
//
// Pure atomic store/load round-trip. Establishes hardware floor.
// ===================================================================
class AtomicPingPongTest {
public:
  AtomicPingPongTest() : cyclesPerNs_(calibrateTscGHz()) {
    std::fprintf(stderr, "TSC calibration: %.3f GHz (%.2f ns/cycle)\n",
                 cyclesPerNs_, 1.0 / cyclesPerNs_);
  }

  LatencyResult runLatencyPass(int producerCpu, int consumerCpu) {
    int64_t total = kWarmupIterations + kMeasureIterations;
    latencies_.clear();
    latencies_.reserve(static_cast<size_t>(kMeasureIterations));

    struct alignas(128) Channel {
      std::atomic<int64_t> sequence{-1};
      std::array<uint8_t, kMessageLength> data{};
    };

    Channel forward;   // producer → consumer
    Channel backward;  // consumer → producer

    std::atomic<bool> consumerReady{false};

    std::thread consumer([&, consumerCpu]() {
      pinToCore(consumerCpu);
      consumerReady.store(true, std::memory_order_release);

      for (int64_t i = 0; i < total; i++) {
        while (forward.sequence.load(std::memory_order_acquire) < i) {
          _mm_pause();
        }
        // Echo back — copy payload (preserves embedded timestamp)
        std::memcpy(backward.data.data(), forward.data.data(), kMessageLength);
        backward.sequence.store(i, std::memory_order_release);
      }
    });

    pinToCore(producerCpu);
    while (!consumerReady.load(std::memory_order_acquire)) { _mm_pause(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    for (int64_t i = 0; i < total; i++) {
      uint64_t tsc_start = rdtsc_fenced();

      // Send — embed timestamp in payload
      std::memcpy(forward.data.data(), &tsc_start, sizeof(tsc_start));
      forward.sequence.store(i, std::memory_order_release);

      while (backward.sequence.load(std::memory_order_acquire) < i) {
        _mm_pause();
      }

      uint64_t tsc_end = rdtscp_fenced();

      // Extract embedded timestamp from echoed payload
      uint64_t tsc_sent;
      std::memcpy(&tsc_sent, backward.data.data(), sizeof(tsc_sent));

      if (i >= kWarmupIterations) {
        latencies_.push_back(tsc_end - tsc_sent);
      }
    }

    consumer.join();
    return computePercentiles(latencies_, 1.0 / cyclesPerNs_);
  }

private:
  double cyclesPerNs_;
  std::vector<uint64_t> latencies_;
};

}  // namespace

// ===================================================================
// Benchmark registrations
// ===================================================================

void PerfTest_OneToOneSequencedLatencyTest(benchmark::State& state) {
  int producerCpu = static_cast<int>(state.range(0));
  int consumerCpu = static_cast<int>(state.range(1));

  static std::unique_ptr<DisruptorPingPongTest> test;
  if (!test) test = std::make_unique<DisruptorPingPongTest>();

  int run = 0;
  for (auto _ : state) {
    auto r = test->runLatencyPass(producerCpu, consumerCpu);
    reportResult(state, r, run, "Disruptor");
  }
  if (state.iterations() >= kRuns) test.reset();
}

void PerfTest_AtomicBaselineLatencyTest(benchmark::State& state) {
  int producerCpu = static_cast<int>(state.range(0));
  int consumerCpu = static_cast<int>(state.range(1));

  static std::unique_ptr<AtomicPingPongTest> test;
  if (!test) test = std::make_unique<AtomicPingPongTest>();

  int run = 0;
  for (auto _ : state) {
    auto r = test->runLatencyPass(producerCpu, consumerCpu);
    reportResult(state, r, run, "Atomic");
  }
  if (state.iterations() >= kRuns) test.reset();
}

static auto* bm_disruptor = [] {
  auto* b = benchmark::RegisterBenchmark("PerfTest_OneToOneSequencedLatencyTest",
                                         &PerfTest_OneToOneSequencedLatencyTest);
  return b->Unit(benchmark::kMillisecond)
          ->Iterations(kRuns)
          ->Args({kDefaultProducerCpu, kDefaultConsumerCpu})
          ->Args({20, 21});
}();

static auto* bm_baseline = [] {
  auto* b = benchmark::RegisterBenchmark("PerfTest_AtomicBaselineLatencyTest",
                                         &PerfTest_AtomicBaselineLatencyTest);
  return b->Unit(benchmark::kMillisecond)
          ->Iterations(kRuns)
          ->Args({kDefaultProducerCpu, kDefaultConsumerCpu})
          ->Args({20, 21});
}();
