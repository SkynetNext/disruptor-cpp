# Cache Line Padding & False Sharing Optimization

## Overview

This document explains the cache line padding strategy used in `Sequence` and other hot-path structures, based on Intel's optimization guidelines.

## Hardware Background

### L2 Prefetch Behavior

Intel processors use two L2 prefetchers that affect data layout decisions:

| Prefetcher | Behavior |
|------------|----------|
| **L2 Streamer** | Fetches **128-byte aligned blocks**; accessing one cache line triggers prefetch of its pair line |
| **L2 Spatial Prefetcher** | Prefetches the adjacent line within the same 128-byte block |

> **Key insight**: Although cache lines are 64 bytes, the L2 prefetcher operates on **128-byte blocks**. Two adjacent 64-byte lines form a "pair" and are often fetched together.

### False Sharing

When multiple threads write to different variables that share the same cache line (or 128-byte prefetch block), the CPU must invalidate and reload the entire block—even though the variables are logically independent.

```
Thread A writes var_a ─┐
                       ├── Same 128-byte block → Cache invalidation ping-pong
Thread B writes var_b ─┘
```

## Intel Coding Rules

From *Intel® 64 and IA-32 Architectures Optimization Reference Manual* §11.4.6:

> ### 11.4.6 PLACEMENT OF SHARED SYNCHRONIZATION VARIABLE
>
> On processors based on Intel NetBurst microarchitecture, bus reads typically fetch 128 bytes into a cache, the optimal spacing to minimize eviction of cached data is 128 bytes. To prevent false-sharing, synchronization variables and system objects (such as a critical section) should be allocated to reside alone in a 128-byte region and aligned to a 128-byte boundary.
>
> On processors based on Intel Core microarchitecture; a synchronization variable should be placed alone and in separate cache line to avoid false-sharing. Software must not allow a synchronization variable to span across page boundary.
>
> **Example 11-6. Placement of Synchronization and Regular Variables**
> ```c
> int regVar;
> int padding[32];
> int SynVar[32*NUM_SYNC_VARS];
> int AnotherVar;
> ```
>
> **User/Source Coding Rule 18.** *(M impact, ML generality)* Place each synchronization variable alone, separated by 128 bytes or in a separate cache line.
>
> **User/Source Coding Rule 19.** *(H impact, L generality)* Do not place any spin lock variable to span a cache line boundary.
>
> At the code level, false sharing is a special concern in the following cases:
> - Global data variables and static data variables that are placed in the same cache line and are written by different threads.
> - Objects allocated dynamically by different threads may share cache lines. Make sure that the variables used locally by one thread are allocated in a manner to prevent sharing the cache line with other threads.
>
> Another technique to enforce alignment of synchronization variables and to avoid a cacheline being shared is to use compiler directives when declaring data structures.
>
> *— Document #: 248966-050US, Page 11-16*

## Sequence Padding Design

### Java Reference Implementation

```java
// reference/disruptor/src/main/java/com/lmax/disruptor/Sequence.java
class LhsPadding {
    protected byte p10, p11, ..., p77;  // 56 bytes (7 × 8)
}
class Value extends LhsPadding {
    protected long value;               // 8 bytes
}
class RhsPadding extends Value {
    protected byte p90, p91, ..., p157; // 56 bytes (7 × 8)
}
public class Sequence extends RhsPadding { }
```

**Java object size**: 12 (header) + 56 + 8 + 56 + 4 (alignment) = **136 bytes**

### C++ Implementation

```cpp
// include/disruptor/Sequence.h
struct LhsPadding {
  std::byte p1[64];                    // 64 bytes (1 cache line)
};
struct Value : LhsPadding {
  std::atomic<int64_t> value_;         // 8 bytes
};
struct RhsPadding : Value {
  std::byte p2[60];                    // 60 bytes
};
class Sequence : public RhsPadding { }
```

**C++ object size**: 64 + 8 + 60 = **132 bytes**

### Memory Layout

```
Offset    Content              Purpose
──────────────────────────────────────────────────────
0x00      ┌─────────────────┐
          │  LhsPadding     │  Isolate from preceding objects
          │  (64 bytes)     │  in same 128-byte prefetch block
0x40      ├─────────────────┤
          │  value_         │  ← Hot field (atomic<int64_t>)
          │  (8 bytes)      │
0x48      ├─────────────────┤
          │  RhsPadding     │  Isolate from following objects
          │  (60 bytes)     │  in same 128-byte prefetch block
0x84      └─────────────────┘
          Total: 132 bytes (> 128 bytes)
```

**Why > 128 bytes?**  
Ensures `value_` cannot share a 128-byte prefetch block with any adjacent `Sequence` or other object.

## Practical Impact

### Benchmark Results: MPSC (Multi-Producer Single-Consumer)

| Platform | 64B | 132B | Improvement |
|----------|-----|------|-------------|
| **Intel i7-10700KF** | 113ns | **91.6ns** | **19% faster** ✓ |

**Key Finding**: Intel's 128-byte rule shows **significant impact** (~20% improvement in MPSC).

### Interpretation

| Scenario | Without Padding (64B) | With Padding (≥128B) |
|----------|----------------------|----------------------|
| Multiple `Sequence` objects in array | Adjacent values share 128B prefetch block → false sharing | Each value isolated |
| Multi-producer writes | **Intel: ~20% slower** due to pair-line invalidation | Optimal |
| Single-producer (SPSC) | No significant difference | No significant difference |

## Application to Other Structures

The same principle applies to:

- `SingleProducerSequencer` cursor and cached values
- `MultiProducerSequencer` cursor
- Any frequently-updated atomic variable accessed by multiple threads

## References

1. **Intel® 64 and IA-32 Architectures Optimization Reference Manual**  
   - §3.7.3 Hardware Prefetching for Second-Level Cache  
   - §11.4.6 Placement of Shared Synchronization Variable  
   - 文档编号: 248966 (搜索此编号可获取最新版本)  
   - 官方索引页: https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html

2. **LMAX Disruptor Technical Paper**  
   - "Disruptor: High performance alternative to bounded queues"  
   - https://lmax-exchange.github.io/disruptor/disruptor.html

3. **Java VarHandle Memory Ordering**  
   - https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/invoke/VarHandle.html

## Summary

| Aspect | Value |
|--------|-------|
| Cache line size | 64 bytes |
| L2 prefetch granularity | 128 bytes |
| Minimum padding per side | 56-64 bytes |
| Total `Sequence` size | ≥128 bytes |
| Key Intel rule | Rule 18: separate by 128 bytes |
| **Intel MPSC impact** | **~20% improvement** with ≥128B padding |