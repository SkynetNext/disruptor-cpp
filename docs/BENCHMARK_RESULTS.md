# Benchmark Results (JMH-aligned Disruptor comparison)

## Test environment 

| Property | Value |
|----------|-------|
| Host | GitHub Actions Runner |
| Date (UTC) | 2026-01-21T05:56:48+00:00 |
| CPU Cores | 4 |
| CPU Frequency | 3303.86 MHz |
| CPU Scaling | Enabled (system default) |
| ASLR | Enabled |

### Cache information

| Cache | Details |
|------|---------|
| L1 Data | 32 KiB (x2) |
| L1 Instruction | 32 KiB (x2) |
| L2 Unified | 512 KiB (x2) |
| L3 Unified | 32768 KiB (x1) |

---

## 🎯 Performance Comparison (C++ vs Java)

### Throughput Comparison

| Test | C++ | Java | **C++/Java Ratio** |
|------|-----|------|-------------------|
| **SPSC** | 267.4 Mops/sec | 130.8 Mops/sec | **2.04x** ⬆️ |
| **MPSC (单事件)** | 40.9 Mops/sec | 36.3 Mops/sec | **1.13x** ⬆️ |
| **MPSC (批量)** | 292.3 Mops/sec | 203.9 Mops/sec | **1.43x** ⬆️ |

### Latency Comparison (SPSC Only)

| Test | C++ | Java | **C++/Java Ratio** |
|------|-----|------|-------------------|
| **SPSC** | 3.74 ns/op | 7.646 ns/op | **0.49x** ⬇️ (faster) |

**Note**: 
- **SPSC**: Both C++ and Java measure latency directly
  - Java: `@BenchmarkMode(Mode.AverageTime)` → `7.646 ns/op`
  - C++: Google Benchmark Time mode → `3.74 ns/op`
- **MPSC**: Both use throughput mode only (aligned with Java `@BenchmarkMode(Mode.Throughput)`)
  - Java: Only outputs throughput (`ops/ms`), no latency/time data
  - C++: Uses `SetItemsProcessed()` to calculate throughput directly (not derived from time)
  - **Important**: Google Benchmark may still display time columns (e.g., `97.7 ns`), but these are **not latency measurements** and should **not be used for comparison**. Only throughput data (`Mops/sec` or `items_per_second`) is reliable and comparable.

### End-to-End Test Comparison

| Test | C++ | Java | **C++/Java Ratio** |
|------|-----|------|-------------------|
| **SPSC End-to-End** | 261 Mops/sec | 150 Mops/sec | **1.74x** ⬆️ |

---

## Detailed Results

### C++ Disruptor Benchmarks (2026-01-21T05:56:48)

| Benchmark | Throughput | Iterations | Notes |
|-----------|------------|------------|-------|
| **SPSC** | 267.4 Mops/sec | 3 | Latency: 3.74 ns/op (for reference only) |
| **MPSC (单事件)** | 40.9 Mops/sec | 3 | 4 threads, throughput only (aligned with Java) |
| **MPSC (批量)** | 292.3 Mops/sec | 3 | 4 threads, items_per_second: 292.331M/s |

**SPSC Details:**
- Latency: 3.74 ns/op (mean), 3.75 ns/op (median)
- stddev: 0.022 ns, cv: 0.58%
- sp_wrap_wait_entries: 1.78033k
- sp_wrap_wait_entries_per_op: 954.18n

**MPSC (单事件) Details:**
- Throughput: 40.9 Mops/sec (4 threads total)
- Threads: 4
- Note: Throughput measurement only (aligned with Java `Mode.Throughput`)

**MPSC (批量) Details:**
- Throughput: 292.3 Mops/sec (4 threads total)
- items_per_second: 292.331M/s
- Threads: 4
- Note: Throughput measurement only (aligned with Java `Mode.Throughput`)

### Java Disruptor Benchmarks (2026-01-21)

| Benchmark | Mode | Score | Error | Units | Iterations |
|-----------|------|-------|-------|-------|------------|
| **SPSC** | avgt | 7.646 | ±0.468 | ns/op | 3 |
| **MPSC (单事件)** | thrpt | 36276.427 | ±1229.441 | ops/ms | 3 |
| **MPSC (批量)** | thrpt | 203878.950 | ±9179.161 | ops/ms | 3 |

**SPSC Details:**
- min: 7.488 ns/op
- max: 7.737 ns/op
- stdev: 0.137
- CI (99.9%): [5.147, 10.144]

**MPSC (单事件) Details:**
- min: 36219.647 ops/ms
- max: 36350.897 ops/ms
- stdev: 67.390
- CI (99.9%): [35046.985, 37505.868]
- Threads: 4

**MPSC (批量) Details:**
- min: 203387.948 ops/ms
- max: 204393.409 ops/ms
- stdev: 503.141
- CI (99.9%): [194699.789, 213058.110]
- Threads: 4

### End-to-End Tests

#### C++ OneToOneSequencedThroughputTest (2026-01-21T05:59:50)

| Run | Throughput (ops/sec) | BatchPercent | AverageBatchSize |
|-----|---------------------|--------------|------------------|
| 0 | 260,416,666 | 99.92% | 1,220 |
| 1 | 247,524,752 | 99.88% | 810 |
| 2 | 260,416,666 | 99.98% | 4,157 |
| 3 | 263,852,242 | 100.00% | 32,733 |
| 4 | 263,157,894 | 100.00% | 32,723 |
| 5 | 261,096,605 | 100.00% | 32,733 |
| 6 | 263,157,894 | 100.00% | 32,510 |

**Summary:**
- Average: 261 Mops/sec
- Range: 247-264 Mops/sec
- avg_batch_size: 32.5098k
- batch_percent: 99.9969%

#### Java OneToOneSequencedThroughputTest

| Run | Throughput (ops/sec) | BatchPercent | AverageBatchSize |
|-----|---------------------|--------------|------------------|
| 0 | 189,035,916 | 96.37% | 27 |
| 1 | 145,137,880 | 92.49% | 13 |
| 2 | 146,198,830 | 92.39% | 13 |
| 3 | 143,884,892 | 91.99% | 12 |
| 4 | 143,678,160 | 91.75% | 12 |
| 5 | 146,842,878 | 92.22% | 12 |
| 6 | 143,678,160 | 91.87% | 12 |

**Summary:**
- Average: 150 Mops/sec
- Range: 143-189 Mops/sec

---

## How to run (same parameters as CI)

### C++ Benchmarks (JMH-aligned)

```bash
cd build
./benchmarks/disruptor_cpp_benchmarks \
  --benchmark_min_warmup_time=10 \
  --benchmark_min_time=5s \
  --benchmark_repetitions=3 \
  --benchmark_filter="JMH_SingleProducerSingleConsumer_producing|JMH_MultiProducerSingleConsumer_producing"
```

**Parameters:**
- `--benchmark_min_warmup_time=10`: 10 seconds total warmup (matches Java: 2 iterations × 5s)
- `--benchmark_min_time=5s`: 5 seconds per measurement iteration
- `--benchmark_repetitions=3`: 3 measurement iterations
- Filter: Only SPSC and MPSC end-to-end tests

### Java Benchmarks (JMH)

```bash
cd reference/disruptor
./gradlew jmhJar --no-daemon
java -jar build/libs/*-jmh.jar \
  -foe true -v NORMAL \
  -f 1 -wi 2 -w 5s -i 3 -r 5s \
  ".*(SingleProducerSingleConsumer|MultiProducerSingleConsumer).*"
```

**Parameters:**
- `-f 1`: 1 fork
- `-wi 2 -w 5s`: 2 warmup iterations, 5s each (10s total warmup)
- `-i 3 -r 5s`: 3 measurement iterations, 5s each
- Filter: Only SPSC and MPSC end-to-end tests

### End-to-End Tests

**C++ (OneToOneSequencedThroughputTest):**
```bash
cd build
./benchmarks/disruptor_cpp_benchmarks \
  --benchmark_filter="PerfTest_OneToOneSequencedThroughputTest" \
  --benchmark_min_time=1s \
  --benchmark_repetitions=1
```

**Java (OneToOneSequencedThroughputTest):**
```bash
cd reference/disruptor
./gradlew perftestClasses --no-daemon
bash ../../scripts/run_java_perftest.sh
```

---

## Test Parameters

### C++ Benchmarks
- Warmup: 10 seconds
- Measurement: 5 seconds per iteration
- Repetitions: 3
- Filter: `JMH_SingleProducerSingleConsumer_producing|JMH_MultiProducerSingleConsumer_producing`

### Java Benchmarks
- Warmup: 2 iterations, 5s each
- Measurement: 3 iterations, 5s each
- Fork: 1
- Filter: `.*(SingleProducerSingleConsumer|MultiProducerSingleConsumer).*`

---

## Notes

- **SPSC**: Single Producer Single Consumer
- **MPSC**: Multi Producer Single Consumer (4 threads)
- **SPSC vs OneToOneSequencedThroughputTest**:
  - **SPSC (JMH)**: Micro-benchmark measuring single operation performance (`next()` + `get()` + `setValue()` + `publish()` loop). Consumer runs continuously in background. Measures operation-level latency/throughput with minimal overhead.
  - **OneToOneSequencedThroughputTest**: End-to-end test measuring complete producer-consumer flow. Producer generates fixed number of events, then waits for all events to be consumed. Includes thread startup, synchronization, and waiting overhead. More representative of real-world usage scenarios.
- **Optimizations Applied**: 
  - `atomic_thread_fence` pattern matching Java `VarHandle` semantics
  - `ThreadHints::onSpinWait()` using `_mm_pause()` instead of `std::this_thread::yield()`
