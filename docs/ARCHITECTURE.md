# Disruptor-CPP Architecture

High-performance, low-latency C++ communication library featuring a **1:1 C++ port of LMAX Disruptor**. Design principles are inspired by both LMAX Disruptor and Aeron.

## Design Philosophy

Disruptor-CPP is a **1:1 C++ port of LMAX Disruptor**, implementing the same lock-free ring buffer patterns for inter-thread communication. Design principles are inspired by:
- **LMAX Disruptor**: Lock-free ring buffer patterns (directly ported)
- **Aeron**: Zero-copy, high-performance messaging design principles (design inspiration)

### Core Principles

- **Zero Allocation**: Pre-allocated buffers eliminate runtime memory allocation
- **Cache-Line Optimized**: Proper memory alignment prevents false sharing
- **Lock-Free Algorithms**: Atomic operations for maximum performance
- **Low Latency**: Optimized for time-critical applications (3.22ns SPSC micro-benchmark)

## System Architecture

### Component Overview

```
┌─────────────────────────────────────────────────────────┐
│                    Disruptor-CPP                        │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────────────────────────────┐              │
│  │         Disruptor (C++ Port)         │              │
│  │                                      │              │
│  │  RingBuffer  │  Sequence            │              │
│  │  Consumer     │  WaitStrategy        │              │
│  │  EventHandler │  EventTranslator     │              │
│  └──────────────────────────────────────┘              │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

## Disruptor Architecture

### Ring Buffer

The ring buffer is the core data structure, providing:
- **Pre-allocated events**: Fixed-size buffer eliminates allocation overhead
- **Power-of-2 sizing**: Enables fast modulo operations using bitwise AND
- **Cache-line alignment**: Prevents false sharing between producer and consumers

```cpp
template<typename T>
class RingBuffer {
    Sequence cursor_;              // Producer position
    std::vector<T> entries_;        // Pre-allocated events
    std::vector<Sequence> gating_;  // Consumer positions
};
```

### Sequence

128-byte aligned atomic sequence number for progress tracking:

```cpp
class alignas(128) Sequence {
    std::atomic<int64_t> value_;
    // Compiler inserts padding to 128 bytes
};
```

**Key Features:**
- 128-byte alignment prevents false sharing (Intel L2 prefetcher operates on 128-byte blocks)
- Memory ordering optimizations (relaxed + fence pattern matching Java VarHandle semantics)
- Lock-free atomic operations

See `docs/CACHE_LINE_PADDING.md` and `docs/SEQUENCE_DESIGN.md` for details.

### Producer Types

#### Single Producer
- Direct assignment (no atomic operations)
- **End-to-end**: 260 Mops/sec - representative of real-world usage
- **Micro-benchmark**: 310.6 Mops/sec (3.22 ns/op) - theoretical maximum

#### Multi Producer
- Atomic CAS operations
- **MPSC (Single Event, 4 threads)**: 51.4 Mops/sec (C++), 36.1 Mops/sec (Java)
- **MPSC (Batch, 4 threads)**: 291.6 Mops/sec (C++), 208.1 Mops/sec (Java)
- Thread-safe for concurrent producers

### Wait Strategies

Different strategies for handling backpressure:

- **BusySpinWaitStrategy**: Lowest latency, highest CPU usage
- **YieldingWaitStrategy**: Balanced (default)
- **BlockingWaitStrategy**: Lowest CPU usage, highest latency
- **SleepingWaitStrategy**: Exponential backoff

### Event Processing

```cpp
// Producer
int64_t sequence = ring_buffer.next();
Event& event = ring_buffer.get(sequence);
event.data = ...;
ring_buffer.publish(sequence);

// Consumer
if (ring_buffer.is_available(sequence)) {
    const Event& event = ring_buffer.get(sequence);
    // Process event...
}
```


## Memory Layout

### Cache-Line Optimization

```
┌─────────────────────────────────────────┐
│  Prefetch Block 1 (128 bytes)           │
│  Sequence::value_ + padding             │
│  (alignas(128) ensures isolation)       │
├─────────────────────────────────────────┤
│  Prefetch Block 2 (128 bytes)            │
│  RingBuffer fields + padding            │
├─────────────────────────────────────────┤
│  Prefetch Block 3+                      │
│  Pre-allocated Event Array              │
└─────────────────────────────────────────┘
```

### False Sharing Prevention

Each `Sequence` instance is aligned to 128-byte boundaries:
- Prevents false sharing between producer and consumers (Intel L2 prefetcher operates on 128-byte blocks)
- Reduces cache coherency traffic
- Improves multi-core performance (~20% improvement in MPSC scenarios)

See `docs/CACHE_LINE_PADDING.md` for Intel optimization guidelines.

## Hardware Latency Context

*Reference: Martin Thompson (Disruptor/Aeron core architect) frequently cited hardware latency benchmarks in QCon presentations and LMAX technical blogs (2018-2022), based on DDR4, Linux x86_64, 3GHz CPU.*

Understanding hardware latency helps explain why Disruptor's design choices matter:

| Operation | Typical Latency | 3GHz Clock Cycles | Notes |
|-----------|----------------|-------------------|-------|
| L1 cache hit | 0.5 ns | 1-2 | Core-private, baseline |
| L2 cache hit | 3-5 ns | 9-15 | On-chip, 6-10x slower than L1 |
| L3 cache hit | 10-20 ns | 30-60 | Shared across socket cores |
| DDR4 main memory (random read) | 60-80 ns | 180-240 | Cross-cache to main memory |
| StoreLoad barrier | 15-25 ns | 45-75 | x86 memory visibility sync |
| Lock-free CAS (success) | 15-30 ns | 45-90 | Foundation of lock-free programming |
| Mutex lock/unlock (no contention) | 25-50 ns | 75-150 | With contention: escalates to μs |
| Thread context switch | 1-5 μs | 3000-15000 | Kernel scheduling + cache invalidation |
| Disruptor (shared memory, lock-free) | 50-200 ns | 150-600 | Zero-copy + lock-free, direct thread-to-thread |
| Aeron IPC (shared memory) | 300 ns-1 μs | 900-3000 | Low-overhead protocol + lock-free |
| localhost UDP (CPU-bound + zero-copy) | 500 ns-2 μs | 1500-6000 | p50 latency, p99: < 5 μs |
| localhost TCP (NODELAY) | 5-10 μs | 15000-30000 | p50 latency, p99: 10-20 μs |

**Key Insight**: Disruptor/Aeron design philosophy is to minimize software overhead to "hardware baseline + minimal protocol/scheduling overhead" by using cache-line padding, lock-free CAS, and zero-copy techniques to keep operations within the minimum cycle count.

## Performance Characteristics

*Note: End-to-end tests (OneToOneSequencedThroughputTest) are more representative of real-world performance than micro-benchmarks, as they include full system overhead (thread scheduling, wait strategies, batch processing, synchronization).*

### End-to-End Performance (Production-like)

- **SPSC End-to-End**: 
  - C++: 260 Mops/sec
  - Java: 155 Mops/sec
  - **C++ is 1.68x faster**

### Micro-Benchmark Performance

- **SPSC micro-benchmark**: 3.22 ns/op (C++), 7.197 ns/op (Java) → 310.6 Mops/sec (C++), 138.9 Mops/sec (Java)
- **MPSC (Single Event, 4 threads)**: 51.4 Mops/sec (C++), 36.1 Mops/sec (Java)
- **MPSC (Batch, 4 threads)**: 291.6 Mops/sec (C++), 208.1 Mops/sec (Java)

*See `docs/BENCHMARK_RESULTS.md` for detailed benchmark data.*

### Scalability

- Linear scaling with batch size
- Minimal overhead in multi-threaded scenarios
- Zero allocations during operation

## Design Patterns

### Single Writer Principle

Only one thread writes to each sequence, eliminating write contention.

### Event Factory Pattern

```cpp
RingBuffer<Event> buffer(size, []() { return Event(); });
```

Pre-allocates all events at construction time.

### Zero-Copy Design

Events are reused, only their content is updated. No copying during operation.

## Use Cases

### High-Frequency Trading
- Low latency requirements (3.22ns/op micro-benchmark, 260 Mops/sec end-to-end, C++)
- Predictable performance
- Zero GC pauses

### Real-Time Systems
- Consistent latency
- High throughput
- Memory-efficient

### Game Engines
- Frame-critical event processing
- Low-latency input handling
- Efficient event distribution

## Comparison with Alternatives

*Note: Performance metrics are based on end-to-end tests (OneToOneSequencedThroughputTest), which better reflect real-world usage than micro-benchmarks.*

| Feature | Disruptor-CPP | std::queue + mutex | LMAX Disruptor |
|---------|-------------|-------------------|----------------|
| Language | C++ | C++ | Java |
| Latency (SPSC micro-benchmark) | 3.22 ns/op | ~88ns | 7.197 ns/op |
| Throughput (SPSC End-to-End) | 260 Mops/sec | ~11 Mops/sec | 155 Mops/sec |
| Allocation | Zero | Per-op | Zero |
| Lock-free | ✅ | ❌ | ✅ |
| Cache Optimized (128-byte) | ✅ | ❌ | ✅ |

## Future Enhancements

- **NUMA Awareness**: Optimize for multi-socket systems
- **Huge Pages**: Reduce TLB misses
- **SIMD Optimizations**: Vectorized operations
- **Additional Wait Strategies**: More backpressure handling options

