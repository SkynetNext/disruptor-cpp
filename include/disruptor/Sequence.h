#pragma once
// 1:1 port skeleton of com.lmax.disruptor.Sequence

#include <atomic>
#include <cstdint>

#include "util/TsanAnnotations.h"

namespace disruptor {

// Java reference:
// reference/disruptor/src/main/java/com/lmax/disruptor/Sequence.java
//
// Java has no alignas, so it uses manual padding via inheritance hierarchy
// (LhsPadding -> Value -> RhsPadding) to prevent false sharing.
//
// C++ can use alignas(128) directly:
//   - Aligns object start to 128-byte boundary
//   - Pads sizeof to 128 bytes (compiler inserts trailing padding)
//   - Ensures value_ doesn't share 128-byte prefetch block with adjacent
//   objects
//
// This satisfies Intel's Rule 18: separate synchronization variables by 128
// bytes. See docs/CACHE_LINE_PADDING.md for details.

class alignas(128) Sequence {
public:
  static constexpr int64_t INITIAL_VALUE = -1;

  Sequence() noexcept : value_(INITIAL_VALUE) {}
  explicit Sequence(int64_t initial) noexcept : value_(initial) {}
  virtual ~Sequence() = default;

  // Java: long value = this.value; VarHandle.acquireFence(); return value;
  // Java reads a plain field (not volatile), then executes acquireFence.
  // This allows reading a "slightly stale" value, which may reduce memory
  // barrier overhead. C++: Match Java semantics - relaxed load + acquire fence.
  // TSAN annotation: inform TSAN about acquire semantics on sequence address.
  virtual int64_t get() const {
    int64_t value = value_.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    DISRUPTOR_TSAN_ACQUIRE(const_cast<std::atomic<int64_t> *>(&value_));
    return value;
  }

  // Java: VarHandle.releaseFence(); this.value = value;
  // Java executes release fence first, then writes to plain field (not
  // volatile). C++: Match Java semantics - release fence + relaxed store.
  // TSAN annotation: inform TSAN about release semantics on sequence address.
  virtual void set(int64_t v) {
    DISRUPTOR_TSAN_RELEASE(&value_);
    std::atomic_thread_fence(std::memory_order_release);
    value_.store(v, std::memory_order_relaxed);
  }

  // Java: setVolatile - used as StoreLoad fence.
  // Java: VarHandle.releaseFence(); this.value = value; VarHandle.fullFence();
  // C++: Matches Java semantics - release fence + relaxed store + full fence.
  // This pattern may provide better performance than a single seq_cst store.
  // TSAN annotation: inform TSAN about release semantics on sequence address.
  virtual void setVolatile(int64_t v) {
    DISRUPTOR_TSAN_RELEASE(&value_);
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

private:
  std::atomic<int64_t> value_;
};

} // namespace disruptor
