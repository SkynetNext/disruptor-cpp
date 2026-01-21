# Benchmark Results (JMH-aligned Disruptor comparison)

## Test environment 

| Property | Value |
|----------|-------|
| Host | GitHub Actions Runner |
| Date (UTC) | 2026-01-21T06:24:38+00:00 |
| CPU Cores | 4 |
| CPU Frequency | 3244.47 MHz |
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
| **SPSC** | 310.6 Mops/sec | 138.9 Mops/sec | **2.24x** ⬆️ |
| **MPSC (单事件)** | 51.4 Mops/sec | 36.1 Mops/sec | **1.42x** ⬆️ |
| **MPSC (批量)** | 291.6 Mops/sec | 208.1 Mops/sec | **1.40x** ⬆️ |

### Latency Comparison (SPSC Only)

| Test | C++ | Java | **C++/Java Ratio** |
|------|-----|------|-------------------|
| **SPSC** | 3.22 ns/op | 7.197 ns/op | **0.45x** ⬇️ (faster) |

**Note**: 
- **SPSC**: Both C++ and Java measure latency directly
  - Java: `@BenchmarkMode(Mode.AverageTime)` → `7.197 ns/op`
  - C++: Google Benchmark Time mode → `3.22 ns/op`
- **MPSC**: Both use throughput mode only (aligned with Java `@BenchmarkMode(Mode.Throughput)`)
  - Java: Only outputs throughput (`ops/ms`), no latency/time data
  - C++: Uses `SetItemsProcessed()` to calculate throughput directly (not derived from time)
  - **Important**: Google Benchmark may still display time columns (e.g., `97.7 ns`), but these are **not latency measurements** and should **not be used for comparison**. Only throughput data (`Mops/sec` or `items_per_second`) is reliable and comparable.

### End-to-End Test Comparison

| Test | C++ | Java | **C++/Java Ratio** |
|------|-----|------|-------------------|
| **SPSC End-to-End** | 260 Mops/sec | 155 Mops/sec | **1.68x** ⬆️ |

---

## Detailed Results

### C++ Disruptor Benchmarks (2026-01-21T06:24:38)

| Benchmark | Throughput | Iterations | Notes |
|-----------|------------|------------|-------|
| **SPSC** | 310.6 Mops/sec | 3 | Latency: 3.22 ns/op (for reference only) |
| **MPSC (单事件)** | 51.4 Mops/sec | 3 | 4 threads, items_per_second: 51.4493M/s (mean) |
| **MPSC (批量)** | 291.6 Mops/sec | 3 | 4 threads, items_per_second: 291.591M/s (mean) |

**SPSC Details:**
- Latency: 3.22 ns/op (mean), 3.22 ns/op (median)
- stddev: 0.003 ns, cv: 0.10%
- sp_wrap_wait_entries: 2.073k
- sp_wrap_wait_entries_per_op: 953.613n

**MPSC (单事件) Details:**
- Throughput: 51.4 Mops/sec (4 threads total)
- items_per_second: 51.4493M/s (mean), 51.5944M/s (median)
- Threads: 4
- Note: Throughput measurement using `SetItemsProcessed()` (aligned with Java `Mode.Throughput`)

**MPSC (批量) Details:**
- Throughput: 291.6 Mops/sec (4 threads total)
- items_per_second: 291.591M/s (mean), 296.214M/s (median)
- Threads: 4
- Note: Throughput measurement only (aligned with Java `Mode.Throughput`)

### Java Disruptor Benchmarks (2026-01-21)

| Benchmark | Mode | Score | Error | Units | Iterations |
|-----------|------|-------|-------|-------|------------|
| **SPSC** | avgt | 7.197 | ±0.456 | ns/op | 3 |
| **MPSC (单事件)** | thrpt | 36075.522 | ±2336.050 | ops/ms | 3 |
| **MPSC (批量)** | thrpt | 208051.272 | ±7114.622 | ops/ms | 3 |

**SPSC Details:**
- min: 7.177 ns/op
- max: 7.225 ns/op
- stdev: 0.025
- CI (99.9%): [6.742, 7.653]

**MPSC (单事件) Details:**
- min: 35961.517 ops/ms
- max: 36214.060 ops/ms
- stdev: 128.047
- CI (99.9%): [33739.473, 38411.572]
- Threads: 4

**MPSC (批量) Details:**
- min: 207601.079 ops/ms
- max: 208285.118 ops/ms
- stdev: 389.976
- CI (99.9%): [200936.650, 215165.894]
- Threads: 4

### End-to-End Tests

#### C++ OneToOneSequencedThroughputTest (2026-01-21T06:27:40)

| Run | Throughput (ops/sec) | BatchPercent | AverageBatchSize |
|-----|---------------------|--------------|------------------|
| 0 | 263,852,242 | 99.99% | 13,789 |
| 1 | 263,157,894 | 99.99% | 14,409 |
| 2 | 262,467,191 | 100.00% | 32,701 |
| 3 | 263,852,242 | 100.00% | 32,573 |
| 4 | 255,754,475 | 100.00% | 32,701 |
| 5 | 256,410,256 | 100.00% | 32,584 |
| 6 | 253,807,106 | 100.00% | 32,289 |

**Summary:**
- Average: 260 Mops/sec
- Range: 253-264 Mops/sec
- avg_batch_size: 32.2893k
- batch_percent: 99.9969%

#### Java OneToOneSequencedThroughputTest

| Run | Throughput (ops/sec) | BatchPercent | AverageBatchSize |
|-----|---------------------|--------------|------------------|
| 0 | 194,174,757 | 96.14% | 25 |
| 1 | 153,374,233 | 94.34% | 17 |
| 2 | 172,117,039 | 95.09% | 20 |
| 3 | 155,520,995 | 93.40% | 15 |
| 4 | 150,375,939 | 92.78% | 13 |
| 5 | 164,744,645 | 94.27% | 17 |
| 6 | 151,057,401 | 92.59% | 13 |

**Summary:**
- Average: 155 Mops/sec
- Range: 150-194 Mops/sec

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
