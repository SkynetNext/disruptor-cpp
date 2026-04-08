// Aeron IPC Ping-Pong RTT benchmark with CPU pinning.
//
// Same methodology as Disruptor latency test:
//   - Two IPC channels for ping/pong
//   - rdtsc timestamps for sub-10ns precision
//   - CPU affinity on producer (ping) and consumer (pong) threads
//   - Warmup + measurement phases
//
// Requires: aeronmd (media driver) running.
// Usage:    ./aeron_ipc_pingpong <producer_cpu> <consumer_cpu>
// Example:  ./aeron_ipc_pingpong 20 68

#include "Aeron.h"
#include "FragmentAssembler.h"
#include "concurrent/BusySpinIdleStrategy.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sched.h>
#include <thread>
#include <vector>

#ifdef __x86_64__
#  include <x86intrin.h>
#endif

using namespace aeron;
using namespace std::chrono;

static constexpr int64_t kWarmupIterations = 500'000;
static constexpr int64_t kMeasureIterations = 10'000'000;
static constexpr int kRuns = 5;
static constexpr int kMessageLength = 100;  // Payload size in bytes
static constexpr int kFragmentLimit = 10;

static const std::string kPingChannel = "aeron:ipc?term-length=8388608";
static const std::string kPongChannel = "aeron:ipc?term-length=8388608";
static constexpr int32_t kPingStreamId = 1002;
static constexpr int32_t kPongStreamId = 1003;

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
  return static_cast<uint64_t>(steady_clock::now().time_since_epoch().count());
}
inline uint64_t rdtscp_fenced() { return rdtsc_fenced(); }
#endif

double calibrateTscGHz() {
#ifdef __x86_64__
  auto t0 = steady_clock::now();
  uint64_t tsc0 = __rdtsc();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  uint64_t tsc1 = __rdtsc();
  auto t1 = steady_clock::now();
  double ns = static_cast<double>(
    duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
  return static_cast<double>(tsc1 - tsc0) / ns;
#else
  return 1.0;
#endif
}

void pinToCore(int cpu) {
  if (cpu < 0) return;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
    std::perror("sched_setaffinity");
  }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
std::shared_ptr<Publication> findPublication(
    std::shared_ptr<Aeron> aeron, int64_t id) {
  std::shared_ptr<Publication> pub;
  while (!(pub = aeron->findPublication(id))) {
    std::this_thread::yield();
  }
  return pub;
}

std::shared_ptr<Subscription> findSubscription(
    std::shared_ptr<Aeron> aeron, int64_t id) {
  std::shared_ptr<Subscription> sub;
  while (!(sub = aeron->findSubscription(id))) {
    std::this_thread::yield();
  }
  return sub;
}

struct LatencyResult {
  double min_ns, max_ns, mean_ns, p50_ns, p90_ns, p99_ns, p999_ns;
};

LatencyResult computePercentiles(std::vector<uint64_t>& lat, double nspc) {
  std::sort(lat.begin(), lat.end());
  auto n = static_cast<int64_t>(lat.size());
  if (n == 0) return {0,0,0,0,0,0,0};

  auto toNs = [nspc](uint64_t c) { return static_cast<double>(c) * nspc; };
  uint64_t sum = 0;
  for (auto v : lat) sum += v;

  return {
    toNs(lat.front()), toNs(lat.back()),
    toNs(sum / static_cast<uint64_t>(n)),
    toNs(lat[static_cast<size_t>(n * 50 / 100)]),
    toNs(lat[static_cast<size_t>(n * 90 / 100)]),
    toNs(lat[static_cast<size_t>(n * 99 / 100)]),
    toNs(lat[static_cast<size_t>(n * 999 / 1000)]),
  };
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  int producerCpu = -1, consumerCpu = -1;
  if (argc >= 3) {
    producerCpu = std::atoi(argv[1]);
    consumerCpu = std::atoi(argv[2]);
    std::fprintf(stderr, "Pinning: producer=cpu%d  consumer=cpu%d\n",
                 producerCpu, consumerCpu);
  } else {
    std::fprintf(stderr, "Usage: %s <producer_cpu> <consumer_cpu>\n", argv[0]);
    std::fprintf(stderr, "Running without CPU pinning.\n");
  }

  double cyclesPerNs = calibrateTscGHz();
  double nsPerCycle = 1.0 / cyclesPerNs;
  std::fprintf(stderr, "TSC calibration: %.3f GHz (%.2f ns/cycle)\n",
               cyclesPerNs, nsPerCycle);

  // --- Connect to Aeron ---
  aeron::Context context;
  context.preTouchMappedMemory(true);
  auto aeron = Aeron::connect(context);

  auto pingPubId = aeron->addPublication(kPingChannel, kPingStreamId);
  auto pongSubId = aeron->addSubscription(kPongChannel, kPongStreamId);
  auto pingSubId = aeron->addSubscription(kPingChannel, kPingStreamId);
  auto pongPubId = aeron->addPublication(kPongChannel, kPongStreamId);

  auto pingPub = findPublication(aeron, pingPubId);
  auto pongSub = findSubscription(aeron, pongSubId);
  auto pingSub = findSubscription(aeron, pingSubId);
  auto pongPub = findPublication(aeron, pongPubId);

  // Wait for images
  while (!pongSub->isConnected() || !pingSub->isConnected()) {
    std::this_thread::yield();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::fprintf(stderr, "Connected. Starting benchmark...\n");

  // --- Pong thread (consumer/echo) ---
  std::atomic<bool> running{true};
  Publication& pongPubRef = *pongPub;
  BusySpinIdleStrategy pongIdleStrategy;

  FragmentAssembler pongAssembler(
    [&](concurrent::AtomicBuffer& buffer, util::index_t offset,
        util::index_t length, const Header&) {
      // Echo back
      while (pongPubRef.offer(buffer, offset, length) < 0L) {
        pongIdleStrategy.idle();
      }
    });

  std::thread pongThread([&]() {
    pinToCore(consumerCpu);

    auto image = pingSub->imageByIndex(0);
    BusySpinIdleStrategy idleStrategy;

    while (running) {
      idleStrategy.idle(image->poll(pongAssembler.handler(), kFragmentLimit));
    }
  });

  // --- Ping thread (producer/measurer) ---
  pinToCore(producerCpu);
  std::this_thread::sleep_for(std::chrono::milliseconds(5));

  auto pongImage = pongSub->imageByIndex(0);
  std::unique_ptr<uint8_t[]> msgBuf(new uint8_t[kMessageLength]);
  concurrent::AtomicBuffer srcBuffer(msgBuf.get(),
                                      static_cast<size_t>(kMessageLength));
  BusySpinIdleStrategy pingIdleStrategy;

  auto doPingPong = [&](int64_t count, std::vector<uint64_t>* latencies) {
    uint64_t tsc_sent = 0;
    FragmentAssembler asm_(
      [&tsc_sent](concurrent::AtomicBuffer& buffer, util::index_t offset,
         util::index_t /*length*/, const Header&) {
        // Extract embedded timestamp from echoed pong payload
        std::memcpy(&tsc_sent, buffer.buffer() + offset, sizeof(tsc_sent));
      });

    for (int64_t i = 0; i < count; i++) {
      // Embed timestamp in first 8 bytes of payload
      uint64_t tsc_start = rdtsc_fenced();
      std::memcpy(msgBuf.get(), &tsc_start, sizeof(tsc_start));

      // Publish ping
      int64_t position;
      do {
        position = pingPub->offer(srcBuffer, 0, kMessageLength);
      } while (position < 0L);

      // Wait for pong
      while (pongImage->position() < position) {
        pongImage->poll(asm_.handler(), kFragmentLimit);
        pingIdleStrategy.idle(0);
      }

      uint64_t tsc_end = rdtscp_fenced();

      if (latencies) {
        latencies->push_back(tsc_end - tsc_sent);
      }
    }
  };

  // Warmup
  std::fprintf(stderr, "Warming up with %lld messages...\n",
               (long long)kWarmupIterations);
  doPingPong(kWarmupIterations, nullptr);
  std::fprintf(stderr, "Warmup done.\n");

  // Measurement runs
  std::vector<uint64_t> latencies;
  latencies.reserve(static_cast<size_t>(kMeasureIterations));

  for (int run = 0; run < kRuns; run++) {
    latencies.clear();
    doPingPong(kMeasureIterations, &latencies);

    auto r = computePercentiles(latencies, nsPerCycle);
    std::fprintf(stderr,
      "Run %d [Aeron IPC RTT/2]: min=%.0fns  p50=%.0fns  p90=%.0fns  "
      "p99=%.0fns  p99.9=%.0fns  max=%.0fns  mean=%.0fns\n",
      run, r.min_ns / 2, r.p50_ns / 2, r.p90_ns / 2,
      r.p99_ns / 2, r.p999_ns / 2, r.max_ns / 2, r.mean_ns / 2);
  }

  running = false;
  pongThread.join();

  return 0;
}
