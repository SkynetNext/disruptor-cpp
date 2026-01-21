#pragma once
// 1:1 port of com.lmax.disruptor.SingleProducerSequencer
// Source:
// reference/disruptor/src/main/java/com/lmax/disruptor/SingleProducerSequencer.java

#include "AbstractSequencer.h"
#include "Error.h"
#include "ProcessingSequenceBarrier.h"
#include "Sequence.h"
#include "WaitStrategy.h"
#include "util/ThreadHints.h"
#include "util/Util.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <stdexcept>
#ifndef NDEBUG
#  include <mutex>
#  include <thread>
#  include <unordered_map>
#endif

namespace disruptor {

// Diagnostic counters used by benchmarks to detect producer backpressure (wrap
// wait) in SPSC. These are intentionally relaxed atomics; they are not part of
// the Disruptor API contract.
inline std::atomic<uint64_t>& sp_wrap_wait_entries() {
  static std::atomic<uint64_t> v{0};
  return v;
}

inline std::atomic<uint64_t>& sp_wrap_wait_loops() {
  static std::atomic<uint64_t> v{0};
  return v;
}

// Java reference (padding intent):
//   reference/disruptor/src/main/java/com/lmax/disruptor/SingleProducerSequencer.java
// Java uses padding superclasses to reduce false sharing around sequencer hot
// fields.
namespace detail {

// 112 bytes of padding (same shape as Java p10..p77).
template <typename WaitStrategyT>
struct SpSequencerPad : public AbstractSequencer<WaitStrategyT> {
  std::array<std::byte, 112> p1{};

  SpSequencerPad(int bufferSize, WaitStrategyT& waitStrategy)
    : AbstractSequencer<WaitStrategyT>(bufferSize, waitStrategy) {}
};

template <typename WaitStrategyT>
struct SpSequencerFields : public SpSequencerPad<WaitStrategyT> {
  int64_t nextValue_;
  int64_t cachedValue_;

  SpSequencerFields(int bufferSize, WaitStrategyT& waitStrategy)
    : SpSequencerPad<WaitStrategyT>(bufferSize, waitStrategy)
    , nextValue_(Sequence::INITIAL_VALUE)
    , cachedValue_(Sequence::INITIAL_VALUE) {}
};

}  // namespace detail

template <typename WaitStrategyT>
class SingleProducerSequencer final : public detail::SpSequencerFields<WaitStrategyT> {
public:
  SingleProducerSequencer(int bufferSize, WaitStrategyT& waitStrategy)
    : detail::SpSequencerFields<WaitStrategyT>(bufferSize, waitStrategy) {}

  bool hasAvailableCapacity(int requiredCapacity) {
    return hasAvailableCapacity(requiredCapacity, false);
  }

  int64_t next() {
    return next(1);
  }

  int64_t next(int n) {
    // Java: assert sameThread() when assertions enabled. We keep a debug check.
    // IMPORTANT: This must be debug-only. A per-call mutex/map check destroys
    // SPSC performance and Java only performs this when assertions are enabled.
#ifndef NDEBUG
    if (!sameThread()) {
      throw std::runtime_error("Accessed by two threads - use ProducerType.MULTI!");
    }
#endif
    if (n < 1 || n > this->bufferSize_) {
      throw std::invalid_argument("n must be > 0 and < bufferSize");
    }

    int64_t nextValue = this->nextValue_;
    int64_t nextSequence = nextValue + n;
    int64_t wrapPoint = nextSequence - this->bufferSize_;
    int64_t cachedGatingSequence = this->cachedValue_;

    if (wrapPoint > cachedGatingSequence || cachedGatingSequence > nextValue) {
      sp_wrap_wait_entries().fetch_add(1, std::memory_order_relaxed);
      this->cursor_.setVolatile(nextValue);  // StoreLoad fence

      int64_t minSequence;
      while (wrapPoint > (minSequence = minimumSequence(nextValue))) {
        sp_wrap_wait_loops().fetch_add(1, std::memory_order_relaxed);
        // Java: LockSupport.parkNanos(1L)
        // Use CPU pause hint instead of yield() to match Java's parkNanos(1L)
        // which doesn't actually yield the CPU on most systems
        disruptor::util::ThreadHints::onSpinWait();
      }

      this->cachedValue_ = minSequence;
    }

    this->nextValue_ = nextSequence;
    return nextSequence;
  }

  std::expected<int64_t, Error> tryNext() {
    return tryNext(1);
  }

  std::expected<int64_t, Error> tryNext(int n) {
    if (n < 1) [[unlikely]] {
      return std::unexpected(Error::invalid_argument("n must be > 0"));
    }

    if (!hasAvailableCapacity(n, true)) [[unlikely]] {
      return std::unexpected(Error::insufficient_capacity());
    }

    this->nextValue_ += n;
    return this->nextValue_;  // [[likely]] path - compiler optimizes this
  }

  int64_t remainingCapacity() {
    int64_t nextValue = this->nextValue_;
    int64_t consumed = minimumSequence(nextValue);
    int64_t produced = nextValue;
    return this->getBufferSize() - (produced - consumed);
  }

  void claim(int64_t sequence) {
    this->nextValue_ = sequence;
  }

  void publish(int64_t sequence) {
    this->cursor_.set(sequence);
    if constexpr (WaitStrategyT::kIsBlockingStrategy) {
      this->waitStrategy_->signalAllWhenBlocking();
    }
  }

  void publish(int64_t lo, int64_t hi) {
    (void)lo;
    publish(hi);
  }

  bool isAvailable(int64_t sequence) {
    const int64_t currentSequence = this->cursor_.get();
    return sequence <= currentSequence && sequence > currentSequence - this->bufferSize_;
  }

  int64_t getHighestPublishedSequence(int64_t /*lowerBound*/, int64_t availableSequence) {
    return availableSequence;
  }

  std::shared_ptr<ProcessingSequenceBarrier<SingleProducerSequencer<WaitStrategyT>, WaitStrategyT>>
  newBarrier(Sequence* const* sequencesToTrack, int count) {
    return std::make_shared<
      ProcessingSequenceBarrier<SingleProducerSequencer<WaitStrategyT>, WaitStrategyT>>(
      *this, *this->waitStrategy_, this->cursor_, sequencesToTrack, count);
  }

  // Override to invalidate cache when gating sequences change
  void addGatingSequences(Sequence* const* gatingSequences, int count) {
    AbstractSequencer<WaitStrategyT>::addGatingSequences(gatingSequences, count);
    gatingSequencesCache_ = nullptr;  // Invalidate cache
  }

  bool removeGatingSequence(Sequence& sequence) {
    bool result = AbstractSequencer<WaitStrategyT>::removeGatingSequence(sequence);
    gatingSequencesCache_ = nullptr;  // Invalidate cache
    return result;
  }

private:
  // Optimization: Cache raw pointer to gatingSequences vector to avoid atomic
  // shared_ptr operations. This is safe because gatingSequences_ is only
  // updated during add/remove (not on hot path), and we refresh the cache when
  // it's null.
  mutable const std::vector<Sequence*>* gatingSequencesCache_ = nullptr;

  // Trailing padding to mirror Java's extra padding in the concrete class.
  std::array<std::byte, 112 - sizeof(const std::vector<Sequence*>*)> p2_{};

  bool hasAvailableCapacity(int requiredCapacity, bool doStore) {
    int64_t nextValue = this->nextValue_;
    int64_t wrapPoint = (nextValue + requiredCapacity) - this->bufferSize_;
    int64_t cachedGatingSequence = this->cachedValue_;

    if (wrapPoint > cachedGatingSequence || cachedGatingSequence > nextValue) {
      if (doStore) {
        this->cursor_.setVolatile(nextValue);  // StoreLoad fence
      }

      int64_t minSequence = minimumSequence(nextValue);
      this->cachedValue_ = minSequence;

      if (wrapPoint > minSequence) {
        return false;
      }
    }

    return true;
  }

  int64_t minimumSequence(int64_t defaultMin) {
    // Optimization: Cache raw pointer to avoid shared_ptr atomic operations on
    // hot path. gatingSequences_ is updated rarely (only when consumers are
    // added/removed), but accessed frequently in next(). Use cached pointer for
    // fast path.
    auto* cached = gatingSequencesCache_;
    if (!cached) {
      // Fallback: reload if cache is null (initialization or after
      // modifications)
      auto snap = this->gatingSequences_.load(std::memory_order_acquire);
      if (!snap) {
        return defaultMin;
      }
      gatingSequencesCache_ = snap.get();
      return disruptor::util::Util::getMinimumSequence(*snap, defaultMin);
    }
    return disruptor::util::Util::getMinimumSequence(*cached, defaultMin);
  }

  // Java-only debug assertion; best-effort in C++.
  bool sameThread() {
#ifdef NDEBUG
    return true;
#else
    static std::mutex m;
    static std::unordered_map<const SingleProducerSequencer*, std::thread::id> producers;
    std::lock_guard<std::mutex> lock(m);
    const auto tid = std::this_thread::get_id();
    auto it = producers.find(this);
    if (it == producers.end()) {
      producers.emplace(this, tid);
      return true;
    }
    return it->second == tid;
#endif
  }
};

}  // namespace disruptor
