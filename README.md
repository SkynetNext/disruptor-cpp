# Disruptor-CPP

A high-performance, low-latency C++ library for inter-thread communication, featuring a **1:1 C++ port of LMAX Disruptor** (API + tests), plus aligned C++ benchmarks for fair comparison against Java JMH.

## Features

- **Disruptor (C++ port)**: `include/disruptor/**` with namespaces `disruptor`, `disruptor::dsl`, `disruptor::util`
- **Tests**: Java tests ported to GoogleTest under `tests/disruptor/**`
- **Benchmarks**: JMH-aligned C++ benchmarks under `benchmarks/jmh/**` (Google Benchmark), comparable to Java JMH results

## Quick Start (build)

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## Performance Results

### Throughput (C++ vs Java)

| Test | C++ | Java | **C++/Java** |
|------|-----|------|--------------|
| **SPSC** | 310.6 Mops/sec | 138.9 Mops/sec | **2.24x** ⬆️ |
| **MPSC (Single Event)** | 51.4 Mops/sec | 36.1 Mops/sec | **1.42x** ⬆️ |
| **MPSC (Batch)** | 291.6 Mops/sec | 208.1 Mops/sec | **1.40x** ⬆️ |
| **SPSC End-to-End** | 260 Mops/sec | 155 Mops/sec | **1.68x** ⬆️ |

### Latency (SPSC)

| Test | C++ | Java | **C++/Java** |
|------|-----|------|--------------|
| **SPSC** | 3.22 ns/op | 7.197 ns/op | **0.45x** ⬇️ (faster) |

*Results from GitHub Actions CI (4 cores, 3244 MHz). See [Benchmark Results](docs/BENCHMARK_RESULTS.md) for detailed data.*

## Documentation

- **Benchmark Results**: `docs/BENCHMARK_RESULTS.md`
- **Build Guide**: `docs/BUILD_GUIDE.md`

## Testing

```bash
./build/tests/disruptor_cpp_tests      # Unit tests
./build/benchmarks/disruptor_cpp_benchmarks  # Performance benchmarks
./build/examples/basic_example       # Usage example
```

## 📄 License

Licensed under the Apache License 2.0. See [LICENSE](LICENSE) for details.

## Inspiration

- [LMAX Disruptor](https://github.com/LMAX-Exchange/disruptor) - Ultra-fast inter-thread messaging
- [Aeron](https://github.com/real-logic/aeron) - High-performance messaging transport
