#pragma once
// TSAN annotations for fence-based synchronization patterns
// TSAN cannot automatically detect happens-before relationships established
// through standalone memory fences. These annotations inform TSAN about
// the synchronization semantics.

// Reference: https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual

// GCC uses __SANITIZE_THREAD__, Clang uses __has_feature(thread_sanitizer)
#if defined(__SANITIZE_THREAD__)
#define DISRUPTOR_TSAN_ENABLED 1
#elif defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define DISRUPTOR_TSAN_ENABLED 1
#endif
#endif

#ifndef DISRUPTOR_TSAN_ENABLED
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
