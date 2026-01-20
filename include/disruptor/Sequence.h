#pragma once
// 1:1 port skeleton of com.lmax.disruptor.Sequence

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace disruptor {

// Java reference (for padding intent):
//   reference/disruptor/src/main/java/com/lmax/disruptor/Sequence.java
// Java uses padding superclasses to reduce false sharing around the hot `value` field.
// We mirror the structure in C++.
//
// Java 64-bit JVM layout (compressed oops enabled):
//   Object Header: 12 bytes
//   LhsPadding:    56 bytes (7 rows × 8 bytes)
//   value:          8 bytes
//   RhsPadding:    56 bytes (7 rows × 8 bytes)
//   Alignment:      4 bytes
//   Total:        136 bytes
//
// C++ has no Object Header, so we use:
//   LhsPadding: 64 bytes (1 cache line, covers Java's header + most of padding)
//   RhsPadding: 60 bytes (Java RhsPadding 56 + Alignment 4)
//   Total: 64 + 8 + 60 = 132 bytes (> 128 bytes for L2 Spatial Prefetcher)
//
// This ensures value doesn't share cache lines with adjacent objects,
// accounting for L2 Spatial Prefetcher's 128-byte prefetch granularity.
namespace detail {

// Java: Sequence.INITIAL_VALUE = -1L
inline constexpr int64_t kInitialValue = -1;

// 64 bytes left padding (1 cache line)
struct LhsPadding {
  std::byte p1[64]{};
};

struct Value : LhsPadding {
  std::atomic<int64_t> value_;
  Value() noexcept : value_(kInitialValue) {}
  explicit Value(int64_t initial) noexcept : value_(initial) {}
};

// 60 bytes right padding (Java RhsPadding 56 + Alignment 4)
// Total: 64 + 8 + 60 = 132 bytes (> 128 bytes, satisfies L2 prefetch requirement)
struct RhsPadding : Value {
  std::byte p2[60]{};
  RhsPadding() noexcept : Value() {}
  explicit RhsPadding(int64_t initial) noexcept : Value(initial) {}
};

} // namespace detail

class Sequence : public detail::RhsPadding {
public:
  static constexpr int64_t INITIAL_VALUE = -1;

  Sequence() noexcept : detail::RhsPadding(INITIAL_VALUE) {}
  explicit Sequence(int64_t initial) noexcept : detail::RhsPadding(initial) {}
  virtual ~Sequence() = default;

  // Java: long value = this.value; VarHandle.acquireFence(); return value;
  // Java reads a plain field (not volatile), then executes acquireFence.
  // This allows reading a "slightly stale" value, which may reduce memory
  // barrier overhead. C++: Match Java semantics - relaxed load + acquire fence.
  virtual int64_t get() const {
    int64_t value = value_.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    return value;
  }
  // Java: VarHandle.releaseFence(); this.value = value;
  // Java executes release fence first, then writes to plain field (not
  // volatile). C++: Match Java semantics - release fence + relaxed store.
  virtual void set(int64_t v) {
    std::atomic_thread_fence(std::memory_order_release);
    value_.store(v, std::memory_order_relaxed);
  }

  // Java: setVolatile - used as StoreLoad fence.
  // Java: VarHandle.releaseFence(); this.value = value; VarHandle.fullFence();
  // C++: Matches Java semantics - release fence + relaxed store + full fence.
  // This pattern may provide better performance than a single seq_cst store.
  virtual void setVolatile(int64_t v) {
    std::atomic_thread_fence(std::memory_order_release);
    value_.store(v, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
  }

  virtual bool compareAndSet(int64_t expected, int64_t desired) {
    return value_.compare_exchange_weak(expected, desired,
                                        std::memory_order_acq_rel,
                                        std::memory_order_acquire);
  }

  virtual int64_t incrementAndGet() {
    return value_.fetch_add(1, std::memory_order_acq_rel) + 1;
  }

  virtual int64_t addAndGet(int64_t increment) {
    return value_.fetch_add(increment, std::memory_order_acq_rel) + increment;
  }

  virtual int64_t getAndAdd(int64_t increment) {
    return value_.fetch_add(increment, std::memory_order_acq_rel);
  }
};

} // namespace disruptor
