# FPGA NPU Performance Benchmark Suite

A comprehensive performance benchmarking suite for FPGA-based Neural Processing Unit (NPU) evaluation. This suite provides detailed analysis of throughput, latency, scalability, and power efficiency characteristics.

## Overview

The FPGA NPU Benchmark Suite is designed to provide comprehensive performance evaluation of NPU implementations across multiple dimensions:

- **Throughput Benchmarks**: Measure peak computational performance for various operations
- **Latency Benchmarks**: Analyze response times and operation latencies
- **Scalability Benchmarks**: Evaluate multi-threaded performance and data size scaling
- **Power Efficiency Benchmarks**: Assess energy consumption and thermal behavior

## Features

### Benchmark Categories

#### 🚀 Throughput Benchmarks
- **Matrix Multiplication**: Dense linear algebra operations (GEMM)
- **2D Convolution**: Convolutional neural network operations
- **Element-wise Operations**: Vector arithmetic and activation functions
- **Memory Bandwidth**: Data transfer performance

#### ⚡ Latency Benchmarks
- **Single Operation Latency**: Individual operation response times
- **Batch Operation Latency**: Batch processing efficiency
- **Memory Access Latency**: Memory subsystem performance
- **Context Switch Latency**: Task switching overhead

#### 📈 Scalability Benchmarks
- **Multi-threaded Throughput**: Parallel processing scaling
- **Data Size Scaling**: Performance vs. problem size analysis
- **Concurrent Mixed Workload**: Real-world usage patterns
- **Load Balancing**: Optimal thread configuration

#### 🔋 Power Efficiency Benchmarks
- **Power Efficiency Analysis**: GOPS/Watt measurements
- **Thermal Behavior**: Temperature and throttling analysis
- **DVFS Efficiency**: Dynamic voltage/frequency scaling
- **Idle Power Consumption**: Baseline power measurements

### Key Features

- **Configurable Test Sizes**: Small, Medium, Large, XLarge problem sizes
- **Flexible Execution**: Run specific benchmarks or complete suites
- **Power Monitoring**: Real-time power and thermal tracking
- **Multi-threading Support**: Scalability analysis across thread counts
- **Comprehensive Reporting**: CSV and JSON output formats
- **Statistical Analysis**: Min/max/average/percentile metrics
- **Cross-platform Support**: Linux and other POSIX systems

## Quick Start

### Prerequisites

- GCC compiler with C11 support
- FPGA NPU hardware and drivers
- NPU user library installed
- pthread library
- Optional: Power monitoring hardware interface

### Building

```bash
# Clone the repository
git clone <repository-url>
cd fpga_npu/tests/benchmarks

# Build the benchmark suite
make

# Or build with specific configuration
make BUILD_TYPE=release ENABLE_POWER_MONITORING=1
```

### Basic Usage

```bash
# Run quick functionality test
make run-quick

# Run complete benchmark suite
make run-full

# Run specific benchmark category
make run-throughput
make run-latency
make run-scalability
make run-power

# Run specific benchmark
./bin/npu_benchmark --benchmark matmul_throughput --size large --iterations 100
```

## Detailed Usage

### Command Line Options

```bash
./bin/npu_benchmark [OPTIONS]

Benchmark Selection:
  -a, --all                  Run all benchmarks (default)
  -t, --throughput           Run throughput benchmarks
  -l, --latency              Run latency benchmarks
  -s, --scalability          Run scalability benchmarks
  -p, --power                Run power efficiency benchmarks
  -b, --benchmark NAME       Run specific benchmark

Benchmark Configuration:
  --size SIZE                Benchmark size (small, medium, large, xlarge)
  --iterations N             Number of iterations
  --warmup N                 Warmup iterations
  --threads N                Thread count for scalability tests

Monitoring Options:
  --enable-power             Enable power monitoring
  --enable-thermal           Enable thermal monitoring

Output Options:
  -v, --verbose              Enable verbose output
  -o, --output DIR           Output directory
  --csv                      Generate CSV report
  --json                     Generate JSON report
```

### Available Benchmarks

| Benchmark Name | Type | Description |
|---|---|---|
| `matmul_throughput` | Throughput | Matrix multiplication performance |
| `conv2d_throughput` | Throughput | 2D convolution performance |
| `elementwise_throughput` | Throughput | Element-wise operations |
| `memory_bandwidth` | Memory | Memory transfer bandwidth |
| `single_op_latency` | Latency | Single operation latency |
| `batch_op_latency` | Latency | Batch operation latency |
| `memory_access_latency` | Latency | Memory access latency |
| `context_switch_latency` | Latency | Context switching latency |
| `multithreaded_throughput` | Scalability | Multi-threaded scaling |
| `data_size_scaling` | Scalability | Data size scaling analysis |
| `concurrent_mixed_workload` | Scalability | Mixed workload performance |
| `load_balancing` | Scalability | Load balancing optimization |
| `power_efficiency_matmul` | Power | Power-efficient matrix multiplication |
| `thermal_behavior` | Power | Thermal analysis under load |
| `dvfs_efficiency` | Power | DVFS optimization |
| `idle_power` | Power | Idle power consumption |

## Example Usage Scenarios

### Performance Characterization

```bash
# Comprehensive performance analysis
./bin/npu_benchmark --all --size large --verbose --csv --json \
    --output ./results/characterization

# Matrix multiplication scaling analysis
./bin/npu_benchmark --benchmark data_size_scaling --verbose \
    --output ./results/scaling
```

### Power Analysis

```bash
# Complete power efficiency analysis
./bin/npu_benchmark --power --enable-power --enable-thermal \
    --size medium --output ./results/power_analysis

# Thermal stress testing
./bin/npu_benchmark --benchmark thermal_behavior --enable-power \
    --enable-thermal --output ./results/thermal
```

### Latency Analysis

```bash
# Detailed latency characterization
./bin/npu_benchmark --latency --size small --iterations 1000 \
    --verbose --output ./results/latency

# Single operation latency with high precision
./bin/npu_benchmark --benchmark single_op_latency --iterations 10000 \
    --warmup 1000 --output ./results/precision_latency
```

### Scalability Testing

```bash
# Multi-threaded performance scaling
./bin/npu_benchmark --benchmark multithreaded_throughput \
    --threads 16 --size large --output ./results/scaling

# Load balancing optimization
./bin/npu_benchmark --benchmark load_balancing \
    --size medium --output ./results/load_balance
```

## Output and Reporting

### CSV Output Format

The benchmark suite generates detailed CSV reports with the following metrics:

- **Benchmark Name**: Test identifier
- **Configuration**: Test parameters (size, iterations, etc.)
- **Throughput (GOPS)**: Operations per second
- **Latency (ms)**: Average response time
- **Bandwidth (GB/s)**: Memory bandwidth (where applicable)
- **Power (W)**: Average power consumption
- **Efficiency (GOPS/W)**: Energy efficiency
- **Temperature (°C)**: Peak operating temperature
- **Errors**: Error count during execution

### JSON Output Format

Structured JSON output provides detailed metrics and metadata:

```json
{
  "benchmark_name": "matmul_throughput",
  "configuration": {
    "size": "large",
    "iterations": 100,
    "warmup_iterations": 10
  },
  "metrics": {
    "throughput_gops": 245.67,
    "latency_ms": 4.08,
    "power_watts": 15.2,
    "efficiency_gops_watt": 16.16,
    "temperature_c": 65.4
  },
  "statistics": {
    "min_latency_ms": 3.95,
    "max_latency_ms": 4.21,
    "p95_latency_ms": 4.15,
    "p99_latency_ms": 4.19
  }
}
```

## Build Configuration

### Build Types

- **Debug**: Full debugging symbols, no optimization
- **Release**: Full optimization, no debug symbols
- **Profile**: Profiling enabled with moderate optimization

```bash
make BUILD_TYPE=debug     # Debug build
make BUILD_TYPE=release   # Release build (default)
make BUILD_TYPE=profile   # Profiling build
```

### Feature Flags

```bash
# Enable power monitoring support
make ENABLE_POWER_MONITORING=1

# Enable thermal monitoring support
make ENABLE_THERMAL_MONITORING=1

# Enable profiling support
make ENABLE_PROFILING=1
```

## Development and Testing

### Code Quality

```bash
# Format source code
make format

# Run static analysis
make analyze

# Memory leak detection
make memcheck
```

### Continuous Integration

```bash
# Run CI pipeline
make ci

# Basic functionality tests
make test

# Performance regression check
make regression
```

## Performance Optimization Tips

### For Maximum Throughput

1. Use `--size xlarge` for large problem sizes
2. Enable all CPU cores with appropriate `--threads` setting
3. Ensure adequate cooling for sustained performance
4. Use release build for maximum optimization

### For Latency Analysis

1. Use `--size small` to minimize measurement noise
2. Increase `--iterations` for statistical significance
3. Use adequate `--warmup` iterations to stabilize performance
4. Run with minimal system load

### For Power Analysis

1. Enable both `--enable-power` and `--enable-thermal`
2. Allow adequate warmup time for thermal stability
3. Run extended tests for thermal characterization
4. Monitor ambient temperature conditions

## Troubleshooting

### Common Issues

**NPU Initialization Failed**
- Verify NPU hardware is properly installed
- Check driver installation and loading
- Ensure proper permissions for device access

**Power Monitoring Not Available**
- Verify power monitoring hardware interface
- Check if power monitoring is supported on your platform
- Ensure proper permissions for power monitoring access

**Performance Lower Than Expected**
- Check for thermal throttling
- Verify NPU is running at expected frequency
- Ensure adequate system resources (memory, CPU)
- Check for competing system processes

### Debug Options

```bash
# Enable verbose output for detailed information
./bin/npu_benchmark --verbose

# Run with debug build for additional diagnostics
make debug
./bin/npu_benchmark --benchmark <name> --verbose

# Use system monitoring tools
htop          # CPU and memory usage
nvidia-smi    # GPU monitoring (if applicable)
sensors       # Temperature monitoring
```

## Contributing

### Adding New Benchmarks

1. Implement benchmark function in appropriate category file
2. Add benchmark definition to `benchmark_main.c`
3. Update documentation and help text
4. Add test cases and validation

### Coding Standards

- Follow existing code style and naming conventions
- Add comprehensive error handling
- Include detailed comments for complex algorithms
- Ensure thread safety for multi-threaded benchmarks

## License

This benchmark suite is part of the FPGA NPU project and follows the same licensing terms.

## Support

For issues, questions, or contributions, please refer to the main project documentation and issue tracking system.

---

**Note**: This benchmark suite is designed for development and evaluation purposes. Always ensure proper cooling and monitoring when running extended performance tests to prevent hardware damage.