# Sequence Design

## 1. False Sharing Prevention

**False sharing** occurs when threads access different variables in the same cache line (64 bytes). Writes invalidate the entire line, forcing other cores to reload.

### Solution: alignas(128)

```cpp
class alignas(128) Sequence {
    std::atomic<int64_t> value_;  // 8 bytes + padding to 128
};
```

**Why 128 bytes?** Intel L2 prefetcher operates on 128-byte blocks. Per Intel Rule 18: "separate synchronization variables by 128 bytes."

**Java uses manual padding** (no alignas support):

```java
class LhsPadding { protected byte p10...p77; }
class Value extends LhsPadding { protected long value; }
class RhsPadding extends Value { protected byte p90...p157; }
```

See `docs/CACHE_LINE_PADDING.md` for details.

## 2. Memory Ordering

Java's VarHandle provides fine-grained memory ordering. C++ equivalent:

| Method | Java Pattern | C++ Implementation |
|--------|--------------|-------------------|
| `get()` | plain read + acquireFence | relaxed load + acquire fence |
| `set()` | releaseFence + plain write | release fence + relaxed store |
| `setVolatile()` | releaseFence + write + fullFence | release fence + relaxed store + seq_cst fence |

```cpp
int64_t get() const {
    int64_t v = value_.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    return v;
}

void set(int64_t v) {
    std::atomic_thread_fence(std::memory_order_release);
    value_.store(v, std::memory_order_relaxed);
}

void setVolatile(int64_t v) {
    std::atomic_thread_fence(std::memory_order_release);
    value_.store(v, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
}
```

**Why relaxed+fence instead of acquire/release store?**  
Matches Java VarHandle semantics. May allow reading "slightly stale" values with reduced barrier overhead.

## 3. Summary

| Technique | Purpose |
|-----------|---------|
| `alignas(128)` | Prevent false sharing (Intel 128-byte prefetch blocks) |
| relaxed + fence | Fine-grained memory ordering matching Java VarHandle |
