# FPGA NPU Integration Test Suite

Comprehensive integration testing framework for end-to-end validation of the FPGA NPU system, including hardware-software integration, stress testing, and reliability validation.

## Overview

The integration test suite provides thorough validation of the complete FPGA NPU system through:

- **End-to-End Testing**: Complete data flow validation from application to hardware
- **Stress Testing**: High-intensity workloads and resource exhaustion scenarios  
- **Reliability Testing**: Long-term stability and error recovery validation
- **Performance Monitoring**: Throughput, latency, and efficiency measurements
- **Concurrent Testing**: Multi-threaded operation validation

## Quick Start

```bash
# Build the test suite
make

# Run all tests
make test

# Run specific test categories
make test-e2e      # End-to-end tests only
make test-stress   # Stress tests only
make test-quick    # Quick validation tests
```

## Test Categories

### End-to-End Integration Tests

**Purpose**: Validate complete system functionality and data flow

**Test Cases**:
- System initialization and device discovery
- Memory management and buffer operations  
- Matrix multiplication validation
- Tensor operations (add, multiply, convolution)
- Performance monitoring and profiling
- Error handling and recovery

**Duration**: ~5-10 minutes  
**Hardware Requirements**: FPGA NPU device accessible via PCIe

### Stress and Reliability Tests

**Purpose**: Validate system stability under extreme conditions

**Test Cases**:
- Memory allocation stress and fragmentation testing
- Large buffer operations (16MB+ transfers)
- Concurrent multi-threaded operations
- Computational intensity testing (1000+ iterations)
- Resource exhaustion and recovery
- Long-term stability validation

**Duration**: ~30-60 minutes  
**Hardware Requirements**: FPGA NPU device with sufficient memory

## Test Framework Architecture

### Core Components

```
integration_test_framework.h/c    # Core testing framework
├── Test execution engine
├── Performance monitoring
├── Assertion macros
├── Resource management
└── Report generation

e2e_tests.c                       # End-to-end test implementations
stress_tests.c                    # Stress test implementations  
integration_test_main.c           # Main test runner application
```

### Key Features

- **Object-oriented test design** with base test context
- **Comprehensive assertion macros** for validation
- **Performance metrics collection** (GOPS, latency, efficiency)
- **Multi-threaded test execution** support
- **HTML and JSON report generation**
- **Signal handling** for graceful shutdown
- **Memory leak detection** integration

## Usage

### Command Line Options

```bash
./bin/integration_test [OPTIONS]

Test Selection:
  -e, --e2e              Run end-to-end integration tests
  -s, --stress           Run stress and reliability tests  
  -a, --all              Run all test suites (default)

Execution Control:
  -v, --verbose          Enable verbose output
  -f, --stop-on-failure  Stop execution on first test failure
  -t, --timeout SECONDS  Set test timeout (default: 30)

Output Control:
  -o, --output DIR       Output directory for reports
  -l, --log FILE         Log file path (default: stdout)
  --html                 Generate HTML report (default: enabled)
  --json                 Generate JSON report  
  --no-html              Disable HTML report generation

Other:
  -h, --help             Show help message
```

### Usage Examples

```bash
# Run all tests with verbose output
./bin/integration_test -a -v

# Run stress tests with custom timeout
./bin/integration_test -s -t 120

# Run E2E tests with JSON output
./bin/integration_test -e --json -o ./results

# Quick validation (E2E only, short timeout)
./bin/integration_test -e -t 10
```

### Make Targets

```bash
# Build targets
make                 # Build test suite
make debug          # Debug build with symbols
make release        # Optimized release build

# Test execution
make test           # Run all tests
make test-e2e       # End-to-end tests only
make test-stress    # Stress tests only
make test-quick     # Quick validation
make test-custom    # Custom configuration

# Development
make install        # Install to /usr/local/bin
make lint           # Static analysis
make memcheck       # Memory leak check
make profile        # Performance profiling

# CI/CD
make ci             # Complete CI pipeline
make pre-commit     # Pre-commit validation
```

## Test Configuration

### Environment Setup

**Prerequisites**:
- FPGA NPU hardware installed and accessible
- NPU driver loaded (`sudo modprobe fpga_npu`)
- NPU userspace library built (`make -C ../../software/userspace`)

**System Requirements**:
- Linux operating system
- GCC compiler with C99 support
- pthread library
- At least 1GB available RAM for stress tests
- PCIe FPGA board with NPU implementation

### Test Data Configuration

The test suite uses various data patterns and sizes:

- **Small matrices**: 16x16 for quick validation
- **Medium matrices**: 32x32 for standard testing  
- **Large matrices**: 64x64 for performance testing
- **Stress buffers**: Up to 16MB for memory testing
- **Random data**: Cryptographically secure random patterns
- **Deterministic patterns**: Sequence and checkerboard patterns

### Performance Baselines

Expected performance ranges (hardware-dependent):

- **Matrix multiplication**: 100+ GOPS for 32x32 FP32
- **Memory bandwidth**: 1+ GB/s for large transfers
- **Latency**: <10ms for typical operations
- **Error rate**: <0.1% under normal conditions
- **Power efficiency**: 10+ GOPS/Watt

## Test Results and Reporting

### Output Formats

**Console Output**:
- Real-time test progress with color coding
- Pass/fail status for each test
- Performance metrics summary
- Final statistics and recommendations

**HTML Report** (`test_results/test_report.html`):
- Executive summary with pass/fail overview
- Detailed test results with timestamps
- Performance charts and metrics
- System configuration information
- Error details and debugging information

**JSON Report** (`test_results/test_report.json`):
- Machine-readable test results
- Complete performance data
- CI/CD integration friendly format
- Historical trend analysis support

### Performance Metrics

For each test, the framework collects:

- **Throughput**: Operations per second (GOPS)
- **Latency**: Average operation completion time
- **Memory usage**: Peak and average consumption
- **Power consumption**: Watts (if available)
- **Efficiency**: GOPS per Watt
- **Error rates**: Success/failure ratios
- **Resource utilization**: CPU, memory, PCIe bandwidth

## Adding New Tests

### Creating a Test Function

```c
test_result_t test_my_feature(test_context_t *ctx)
{
    // Test implementation
    start_performance_monitoring(ctx);
    
    // Perform test operations
    int result = npu_my_operation(ctx->npu_handle, params);
    ASSERT_SUCCESS(ctx, result, "My operation failed");
    
    // Update performance metrics
    update_performance_metrics(ctx, operation_count);
    
    return TEST_RESULT_PASS;
}
```

### Test Configuration

```c
test_config_t my_test_config = {
    .name = "My Feature Test",
    .category = TEST_CATEGORY_FUNCTIONAL,
    .severity = TEST_SEVERITY_MEDIUM,
    .timeout_seconds = 30,
    .enable_performance_monitoring = true,
    .enable_stress_testing = false,
    .enable_concurrent_execution = false,
    .iterations = 1,
    .data_size_bytes = 4096
};
```

### Registering Tests

Add to appropriate test suite:

```c
static test_function_t my_test_functions[] = {
    // ... existing tests
    test_my_feature
};

static test_config_t my_test_configs[] = {
    // ... existing configs  
    my_test_config
};
```

## Troubleshooting

### Common Issues

**1. Device Not Found**
```
Error: NPU initialization failed
```
- Verify FPGA board is properly connected
- Check driver is loaded: `lsmod | grep fpga_npu`  
- Verify device permissions: `ls -l /dev/fpga_npu`

**2. Memory Allocation Failures**
```
Error: Failed to allocate buffer
```
- Check available system memory: `free -h`
- Verify NPU memory pool size
- Reduce test data sizes in stress tests

**3. Test Timeouts**
```
Test timed out after 30 seconds
```
- Increase timeout with `-t` option
- Check for infinite loops in test code
- Verify hardware is not thermal throttling

**4. Performance Issues**
```
Warning: Low performance detected
```
- Check thermal throttling status
- Verify PCIe link speed and width
- Review system load and competing processes

### Debug Mode

Enable debug builds for detailed information:

```bash
make debug
./bin/integration_test -v -e  # Verbose E2E tests
```

### Memory Debugging

Check for memory leaks:

```bash
make memcheck  # Runs valgrind automatically
```

### Performance Profiling

Generate performance profiles:

```bash
make profile   # Creates profile.txt
```

## Continuous Integration

### GitHub Actions Example

```yaml
name: Integration Tests
on: [push, pull_request]

jobs:
  integration-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Setup environment
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc make
          
      - name: Build userspace library
        run: make -C software/userspace
        
      - name: Build integration tests
        run: make -C tests/integration
        
      - name: Run quick tests
        run: make -C tests/integration test-quick
        
      - name: Upload test results
        uses: actions/upload-artifact@v2
        with:
          name: test-results
          path: tests/integration/test_results/
```

### Jenkins Pipeline

```groovy
pipeline {
    agent any
    
    stages {
        stage('Build') {
            steps {
                sh 'make -C software/userspace'
                sh 'make -C tests/integration'
            }
        }
        
        stage('Test') {
            steps {
                sh 'make -C tests/integration test'
            }
            post {
                always {
                    publishHTML([
                        allowMissing: false,
                        alwaysLinkToLastBuild: true,
                        keepAll: true,
                        reportDir: 'tests/integration/test_results',
                        reportFiles: 'test_report.html',
                        reportName: 'Integration Test Report'
                    ])
                }
            }
        }
    }
}
```

## Contributing

When adding new tests or modifying the framework:

1. **Follow coding standards**: Use consistent style and naming
2. **Add comprehensive validation**: Include positive and negative test cases  
3. **Document performance expectations**: Define expected ranges and baselines
4. **Test error conditions**: Validate graceful failure handling
5. **Update documentation**: Keep README and comments current
6. **Verify CI compatibility**: Ensure tests work in automated environments

## License

This integration test suite is part of the FPGA NPU project and follows the same licensing terms as the main project.