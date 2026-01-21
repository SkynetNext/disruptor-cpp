#pragma once
// 1:1 port of com.lmax.disruptor.util.ThreadHints
// Source: reference/disruptor/src/main/java/com/lmax/disruptor/util/ThreadHints.java

#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
#  include <immintrin.h>  // _mm_pause
#elif !defined(__aarch64__)
// Only include <thread> for platforms that need std::this_thread::yield()
#  include <thread>
#endif

namespace disruptor::util {

class ThreadHints final {
public:
  ThreadHints() = delete;

  // Java: public static void onSpinWait() { Thread.onSpinWait(); }
  // Match Java's Thread.onSpinWait() intent as closely as possible.
  // Uses CPU pause instructions to reduce power consumption and cache contention
  // without yielding the CPU (no context switch overhead).
  static void onSpinWait() {
#if defined(__x86_64__) || defined(_M_X64)
    // x86/x64: PAUSE instruction reduces power consumption and cache contention
    _mm_pause();
#elif defined(__aarch64__)
    // ARM64: YIELD instruction hints the CPU that this thread is spinning
    __asm__ __volatile__("yield" ::: "memory");
#else
    // Other platforms: fallback to yield() (may involve context switch)
    std::this_thread::yield();
#endif
  }
};

}  // namespace disruptor::util
