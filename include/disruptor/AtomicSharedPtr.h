#pragma once
// Spinlock-based atomic shared_ptr wrapper for C++20 compilers whose standard
// library does not yet support std::atomic<std::shared_ptr<T>> (requires
// libstdc++ from GCC 12+ or libc++ 16+).
//
// This is only used for gating-sequence management (cold path), so the
// spinlock overhead is negligible.

#include <atomic>
#include <memory>

namespace disruptor {

template <typename T>
class AtomicSharedPtr {
public:
  AtomicSharedPtr() = default;

  explicit AtomicSharedPtr(std::shared_ptr<T> p) : ptr_(std::move(p)) {}

  std::shared_ptr<T> load(std::memory_order /*mo*/ = std::memory_order_seq_cst) const {
    lock();
    auto copy = ptr_;
    unlock();
    return copy;
  }

  void store(std::shared_ptr<T> desired, std::memory_order /*mo*/ = std::memory_order_seq_cst) {
    lock();
    ptr_ = std::move(desired);
    unlock();
  }

  bool compare_exchange_weak(std::shared_ptr<T>& expected,
                             std::shared_ptr<T> desired,
                             std::memory_order /*success*/ = std::memory_order_seq_cst,
                             std::memory_order /*failure*/ = std::memory_order_seq_cst) {
    lock();
    if (ptr_ == expected) {
      ptr_ = std::move(desired);
      unlock();
      return true;
    }
    expected = ptr_;
    unlock();
    return false;
  }

private:
  mutable std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
  std::shared_ptr<T> ptr_;

  void lock() const {
    while (flag_.test_and_set(std::memory_order_acquire)) {
      // spin
    }
  }

  void unlock() const {
    flag_.clear(std::memory_order_release);
  }
};

}  // namespace disruptor
