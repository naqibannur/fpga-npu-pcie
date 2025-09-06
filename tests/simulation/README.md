# FPGA NPU Simulation Test Suite

Comprehensive simulation test suite for FPGA NPU hardware validation.

## Quick Start

```bash
# Run basic tests
./run_tests.sh

# Run with specific simulator
./run_tests.sh -s vivado -m full

# Run regression tests
./run_tests.sh -m regression
```

## Test Framework Structure

```
tests/simulation/
├── sim_test_framework.sv    # Base test framework
├── npu_core_tests.sv        # NPU core functionality tests
├── memory_dma_tests.sv      # Memory and DMA tests
├── pcie_tests.sv            # PCIe controller tests
├── sim_test_main.sv         # Main test runner
├── Makefile                 # Build system
├── run_tests.sh             # Test execution script
└── README.md                # This file
```

## Test Categories

### NPU Core Tests
- Basic operations (add, multiply, matrix operations)
- Pipeline functionality
- Error handling and recovery
- Performance stress testing

### Memory & DMA Tests
- DMA transfer validation
- Memory coherency testing
- Buffer management
- Cache behavior verification

### PCIe Tests
- Configuration space access
- Transaction layer protocol (TLP) testing
- Link management and training
- Error detection and recovery

## Usage

### Command Line Options

```bash
./run_tests.sh [OPTIONS]

Options:
  -s, --simulator SIMULATOR   Simulator (vivado, modelsim, vcs, verilator)
  -m, --mode MODE             Test mode (quick, full, regression, stress, all)
  -c, --coverage BOOL         Enable coverage (0|1)
  -a, --assertions BOOL       Enable assertions (0|1)
  -p, --performance BOOL      Enable performance monitoring (0|1)
  --clean                     Clean before running
  -v, --verbose               Verbose output
  -h, --help                  Show help
```

### Make Targets

```bash
# Basic test execution
make test                    # Run full test suite
make test-quick             # Quick tests without coverage
make test-full              # Full tests with all features
make test-regression        # Multi-simulator regression

# Coverage and analysis
make coverage-report        # Generate coverage report
make performance           # Performance analysis
make lint                  # Static analysis

# CI/CD targets
make ci                    # Continuous integration pipeline
make nightly              # Nightly regression tests
```

## Test Configuration

### Environment Variables

- `SIM`: Simulator selection (vivado, modelsim, vcs, verilator)
- `ENABLE_COVERAGE`: Coverage collection (0/1)
- `ENABLE_ASSERTIONS`: Assertion checking (0/1)
- `ENABLE_PERFORMANCE_MONITORING`: Performance monitoring (0/1)
- `TEST_TIMEOUT`: Test timeout in cycles

### Simulator Requirements

#### Vivado Simulator
```bash
# Ensure Vivado is in PATH
source /opt/Xilinx/Vivado/2023.2/settings64.sh
```

#### ModelSim/Questa
```bash
# Ensure ModelSim is in PATH
export PATH=$PATH:/opt/modelsim/bin
```

#### VCS
```bash
# Ensure VCS is in PATH
source /opt/synopsys/vcs/bin/setup.sh
```

#### Verilator
```bash
# Install Verilator
sudo apt-get install verilator
```

## Test Framework Features

### Object-Oriented Test Classes
- Base test class with common functionality
- Assertion macros for validation
- Test lifecycle management (setup, run, cleanup)
- Automatic timeout handling

### Coverage Collection
- Functional coverage groups
- Code coverage metrics
- Assertion coverage tracking
- Coverage merging across test runs

### Performance Monitoring
- Cycle count tracking
- Transaction monitoring
- Throughput calculations
- Performance regression detection

### Reporting
- HTML test reports
- Coverage summaries
- Performance analysis
- CI/CD integration

## Adding New Tests

### Create Test Class
```systemverilog
`TEST_CLASS(my_new_test)
    function new();
        super.new("my_new_test", SEV_HIGH, 5000);
    endfunction
    
    virtual task run();
        super.run();
        // Test implementation
        assert_eq(actual, expected, "Test description");
    endtask
`END_TEST_CLASS
```

### Register Test
```systemverilog
function void register_my_tests();
    `RUN_TEST(my_new_test);
endfunction
```

## Continuous Integration

### GitHub Actions Example
```yaml
name: Simulation Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Run simulation tests
        run: |
          cd tests/simulation
          ./run_tests.sh -m regression
```

### Jenkins Pipeline
```groovy
pipeline {
    agent any
    stages {
        stage('Test') {
            steps {
                sh './tests/simulation/run_tests.sh -m ci'
            }
        }
    }
    post {
        always {
            publishHTML([
                allowMissing: false,
                alwaysLinkToLastBuild: true,
                keepAll: true,
                reportDir: 'tests/simulation/latest/reports',
                reportFiles: 'comprehensive_report.html',
                reportName: 'Test Report'
            ])
        }
    }
}
```

## Troubleshooting

### Common Issues

1. **Simulator not found**
   - Ensure simulator is in PATH
   - Check license availability

2. **Compilation errors**
   - Verify SystemVerilog version support
   - Check file paths and dependencies

3. **Test timeouts**
   - Increase timeout with `-t` option
   - Check for infinite loops in tests

4. **Coverage collection failures**
   - Verify simulator supports coverage
   - Check coverage database permissions

### Debug Mode

```bash
# Run with maximum verbosity
./run_tests.sh -v -m quick

# Interactive debugging
make debug SIM=vivado

# Generate waveforms
make waves SIM=vivado
```

## Performance Optimization

### Parallel Execution
```bash
# Run tests in parallel
make test-parallel

# Parallel regression testing
./run_tests.sh --parallel -m regression
```

### Resource Management
- Use quick tests for development
- Reserve full regression for CI/CD
- Monitor memory usage with large test suites

## License

This simulation test suite is part of the FPGA NPU project and follows the same licensing terms as the main project.