# FPGA NPU Library Unit Tests

This directory contains comprehensive unit tests for the FPGA NPU user-space library, covering core functionality, memory management, tensor operations, and performance monitoring.

## Test Coverage

### Core Functions (`test_core.c`)
- Library initialization and cleanup
- Tensor creation and validation
- Performance counter operations
- Error handling and logging
- Device status monitoring
- Instruction execution

### Memory Management (`test_memory.c`)
- Legacy memory allocation
- Managed buffer allocation/deallocation
- Buffer mapping/unmapping
- Buffer read/write operations
- Buffer synchronization
- Memory statistics tracking
- Tensor creation from buffers

### Tensor Operations (`test_tensor_ops.c`)
- Matrix multiplication
- 2D convolution
- Element-wise operations (add, multiply)
- Activation functions (ReLU, Sigmoid, Tanh, Softmax)
- Pooling operations (max, average, global)
- Normalization (batch norm, layer norm)
- Utility operations (dropout, transpose, reshape, concat)

## Test Framework

### Features
- **Assertion Macros**: Comprehensive set of assertion macros for different data types
- **Colored Output**: Visual test result indication with colors
- **Mock System**: Mock implementations for system calls (open, ioctl, mmap, etc.)
- **Performance Testing**: Built-in performance measurement capabilities
- **Memory Leak Detection**: Integration with valgrind for memory analysis
- **Code Coverage**: Support for gcov code coverage analysis

### Assertion Macros
```c
ASSERT_TRUE(condition)              // Assert condition is true
ASSERT_FALSE(condition)             // Assert condition is false
ASSERT_EQ(expected, actual)         // Assert equality
ASSERT_NEQ(not_expected, actual)    // Assert inequality
ASSERT_NULL(ptr)                    // Assert pointer is NULL
ASSERT_NOT_NULL(ptr)                // Assert pointer is not NULL
ASSERT_STR_EQ(expected, actual)     // Assert string equality
ASSERT_FLOAT_EQ(exp, act, tol)      // Assert float equality with tolerance
```

## Building and Running Tests

### Prerequisites
- GCC compiler with C99 support
- GNU Make
- Linux development environment

### Optional Tools (for enhanced testing)
- **valgrind**: Memory leak detection and analysis
- **cppcheck**: Static code analysis
- **clang-format**: Code formatting
- **gcov**: Code coverage analysis

### Build Commands

```bash
# Build unit tests
make

# Build with debug information and sanitizers
make debug

# Build optimized release version
make release

# Clean build artifacts
make clean
```

### Running Tests

```bash
# Run basic unit tests
make test

# Run tests with memory checking (requires valgrind)
make test-memory

# Generate code coverage report (requires gcov)
make coverage

# Run static analysis (requires cppcheck)
make analyze

# Run performance benchmarks
make benchmark
```

### Using the Test Script

The `run_tests.sh` script provides a comprehensive test execution environment:

```bash
# Make script executable
chmod +x run_tests.sh

# Run basic tests
./run_tests.sh

# Run all test modes
./run_tests.sh --all

# Run specific test modes
./run_tests.sh --memory --coverage

# Clean build and run with verbose output
./run_tests.sh --clean --verbose

# Run benchmarks only
./run_tests.sh --benchmark

# Show help
./run_tests.sh --help
```

## Test Structure

### File Organization
```
tests/unit/
├── test_framework.h          # Test framework declarations
├── test_framework.c          # Test framework implementation
├── test_core.c               # Core functionality tests
├── test_memory.c             # Memory management tests
├── test_tensor_ops.c         # Tensor operation tests
├── test_main.c               # Main test runner
├── Makefile                  # Build configuration
├── run_tests.sh              # Comprehensive test script
└── README.md                 # This file
```

### Mock System

The test framework includes mock implementations for system calls to enable testing without actual hardware:

- **Mock Device I/O**: Simulates device file operations
- **Mock Memory Management**: Simulates mmap/munmap operations
- **Mock IOCTL**: Simulates driver communication
- **Configurable Failures**: Can simulate various error conditions

### Test Categories

1. **Unit Tests**: Test individual functions in isolation
2. **Integration Tests**: Test component interactions
3. **Performance Tests**: Measure execution speed and efficiency
4. **Stress Tests**: Test with extreme conditions and repeated operations
5. **Edge Case Tests**: Test boundary conditions and error scenarios

## Continuous Integration

### Test Execution Pipeline
1. **Static Analysis**: Code quality and style checking
2. **Unit Tests**: Functional correctness verification
3. **Memory Testing**: Leak detection and memory safety
4. **Coverage Analysis**: Test coverage measurement
5. **Performance Benchmarks**: Performance regression detection

### Report Generation

The test framework generates comprehensive reports:

- **Build Logs**: Compilation output and warnings
- **Test Results**: Detailed test execution results
- **Memory Reports**: Valgrind memory analysis
- **Coverage Reports**: Line and function coverage data
- **Performance Metrics**: Benchmark results and timing
- **HTML Summary**: Consolidated test report

## Mock Configuration

### Setting Up Mock Behavior

```c
// Reset mock to default state
mock_reset();

// Configure initialization failure
mock_set_init_fail(true);

// Configure IOCTL failure
mock_set_ioctl_fail(true);

// Set mock device status
mock_set_status(STATUS_READY | STATUS_DONE);

// Set mock performance counters
mock_set_performance_counters(1000000, 50000);
```

## Adding New Tests

### Creating a New Test Function

```c
bool test_my_new_feature(void)
{
    TEST_CASE("my new feature description");
    
    // Setup
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Test logic
    int result = my_function_under_test(handle, parameters);
    ASSERT_EQ(NPU_SUCCESS, result);
    
    // Cleanup
    npu_cleanup(handle);
    TEST_PASS();
}
```

### Adding to Test Suite

```c
void run_my_test_suite(void)
{
    TEST_SUITE("My Feature Tests");
    
    RUN_TEST(test_my_new_feature);
    RUN_TEST(test_another_feature);
}
```

### Integration with Main

```c
int main(int argc, char* argv[])
{
    TEST_INIT();
    
    run_core_tests();
    run_memory_tests();
    run_tensor_tests();
    run_my_test_suite();  // Add your suite
    
    TEST_EXIT();
}
```

## Performance Benchmarking

The test framework includes performance measurement capabilities:

- **Timing Measurements**: High-resolution timing for operations
- **Throughput Calculation**: GOPS (Giga Operations Per Second) metrics
- **Memory Usage**: Buffer allocation and usage statistics
- **Regression Detection**: Compare against baseline performance

## Troubleshooting

### Common Issues

1. **Build Failures**
   - Check GCC version and C99 support
   - Verify include paths and library dependencies
   - Review build logs in `build/reports/build.log`

2. **Test Failures**
   - Check mock configuration for expected behavior
   - Verify test data and expected results
   - Review detailed test logs

3. **Memory Issues**
   - Run with valgrind for detailed memory analysis
   - Check buffer bounds and allocation patterns
   - Verify proper cleanup in test teardown

4. **Performance Issues**
   - Use benchmark mode for performance analysis
   - Check for debug vs release build configuration
   - Monitor system resources during test execution

### Debug Tips

- Use `make debug` for detailed debugging information
- Enable verbose output with `--verbose` flag
- Check individual test logs in `build/reports/`
- Use GDB for interactive debugging of test failures

## Contributing

When adding new tests:

1. Follow the existing naming conventions
2. Include both positive and negative test cases
3. Test error conditions and edge cases
4. Add appropriate documentation
5. Update this README if adding new test categories
6. Ensure tests pass in all modes (basic, memory, coverage)

## License

These unit tests are part of the FPGA NPU project and follow the same licensing terms as the main project.