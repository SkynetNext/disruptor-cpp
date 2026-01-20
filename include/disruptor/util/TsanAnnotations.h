#pragma once
// TSAN annotations for fence-based synchronization patterns
// TSAN cannot automatically detect happens-before relationships established
// through standalone memory fences. These annotations inform TSAN about
// the synchronization semantics.

// Reference: https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual

#if defined(__SANITIZE_THREAD__) ||                                            \
    defined(__has_feature) && __has_feature(thread_sanitizer)
#define DISRUPTOR_TSAN_ENABLED 1
#else
#define DISRUPTOR_TSAN_ENABLED 0
#endif

#if DISRUPTOR_TSAN_ENABLED

extern "C" {
void __tsan_acquire(void *addr);
void __tsan_release(void *addr);
}

// Call after acquiring data (consumer side, after acquire fence)
#define DISRUPTOR_TSAN_ACQUIRE(addr) __tsan_acquire(addr)

// Call before releasing data (producer side, before release fence)
#define DISRUPTOR_TSAN_RELEASE(addr) __tsan_release(addr)

#else

#define DISRUPTOR_TSAN_ACQUIRE(addr) ((void)0)
#define DISRUPTOR_TSAN_RELEASE(addr) ((void)0)

#endif
