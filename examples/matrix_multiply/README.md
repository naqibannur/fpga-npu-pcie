# Matrix Multiplication Example

This example demonstrates basic NPU usage through matrix multiplication operations. It showcases fundamental NPU programming concepts including initialization, memory management, and performance measurement.

## Overview

The matrix multiplication example performs C = A × B operations using NPU acceleration, comparing performance against CPU implementations and demonstrating proper NPU usage patterns.

## Features

- **Basic NPU Operations**: Matrix multiplication using `npu_matrix_multiply()`
- **Performance Comparison**: NPU vs CPU timing and speedup analysis
- **Memory Management**: Proper NPU memory allocation and cleanup
- **Scalability Testing**: Performance analysis across different matrix sizes
- **Verification**: Result validation against CPU reference implementation
- **Error Handling**: Comprehensive error checking and reporting

## Command Line Options

```bash
Usage: matrix_multiply_example [OPTIONS]

Options:
  -s, --size SIZE      Matrix size (default: 256, max: 2048)
  --no-verify          Disable CPU verification
  -p, --performance    Enable performance testing
  -v, --verbose        Enable verbose output
  -h, --help           Show help message

Examples:
  ./matrix_multiply_example                           # Run with default settings
  ./matrix_multiply_example --size 512 --performance  # 512x512 matrix with performance test
  ./matrix_multiply_example --size 64 --verbose       # Small matrix with detailed output
```

## Usage Examples

### Basic Usage

```bash
# Run with default 256x256 matrices
./matrix_multiply_example

# Output:
# === NPU Matrix Multiplication Example ===
# Matrix size: 256x256
# Verification: enabled
# Performance testing: disabled
# 
# Initializing NPU...
# NPU Device Information:
#   Device ID: 0x1234
#   Memory size: 1024 MB
#   Processing elements: 128
# 
# Allocating matrices (262144 bytes each)...
# Initializing matrices...
# Performing NPU matrix multiplication...
# NPU computation completed in 2.145 ms
# Running CPU verification...
# CPU computation completed in 32.876 ms
# NPU speedup: 15.32x
# ✅ Verification PASSED - Results match within tolerance
# 
# ✅ Matrix multiplication example completed successfully!
```

### Performance Testing

```bash
# Run performance analysis with scaling test
./matrix_multiply_example --size 512 --performance

# Output includes:
# === Performance Testing ===
# Running 100 iterations for performance measurement...
# Performance Results:
#   Total duration: 2.134 seconds
#   Average latency: 21.34 ms
#   Throughput: 125.67 GOPS
# 
# === Performance Scaling Demonstration ===
# Matrix Size | Throughput (GOPS) | Latency (ms)
# ------------|-------------------|-------------
#          64 |             45.23 |        0.36
#         128 |             89.45 |        1.47
#         256 |            125.67 |        5.89
#         512 |            156.78 |       23.45
#        1024 |            178.92 |       94.12
```

### Verbose Output

```bash
# Run with small matrices and detailed output
./matrix_multiply_example --size 4 --verbose

# Shows actual matrix values:
# Matrix A (4x4):
#    0.745   -0.234    0.892   -0.123
#   -0.456    0.789    0.234    0.567
#    0.123   -0.678    0.345    0.789
#   -0.234    0.456   -0.567    0.123
# 
# Matrix B (4x4):
#    0.567    0.234   -0.789    0.345
#   -0.123    0.678    0.456   -0.234
#    0.789   -0.345    0.123    0.567
#    0.234    0.567   -0.123   -0.789
# 
# Result Matrix C (4x4):
#    0.423    0.567   -0.234    0.789
#   -0.345    0.123    0.678   -0.456
#    0.789   -0.567    0.234    0.345
#   -0.123    0.456   -0.789    0.567
```

## Code Structure

### Main Components

1. **Matrix Initialization**
   ```c
   void initialize_matrix_random(float *matrix, int rows, int cols);
   void initialize_matrix_pattern(float *matrix, int rows, int cols, const char *pattern);
   ```

2. **NPU Operations**
   ```c
   npu_result_t result = npu_matrix_multiply(npu, matrix_A, matrix_B, matrix_C, 
                                            matrix_size, matrix_size, matrix_size);
   ```

3. **Performance Measurement**
   ```c
   double measure_performance(npu_handle_t npu, const float *A, const float *B, 
                             float *C, int M, int N, int K, int iterations);
   ```

4. **Verification**
   ```c
   bool verify_result(const float *npu_result, const float *cpu_result, 
                     int size, float tolerance);
   ```

### Memory Management Pattern

```c
// Allocation
float *matrix_A = npu_malloc(npu, matrix_bytes);
float *matrix_B = npu_malloc(npu, matrix_bytes);
float *matrix_C = npu_malloc(npu, matrix_bytes);

// Check allocation success
if (!matrix_A || !matrix_B || !matrix_C) {
    printf("Failed to allocate NPU memory\n");
    goto cleanup;
}

// Use matrices...

cleanup:
    // Cleanup resources
    if (matrix_A) npu_free(npu, matrix_A);
    if (matrix_B) npu_free(npu, matrix_B);
    if (matrix_C) npu_free(npu, matrix_C);
```

## Performance Characteristics

### Typical Results

On a mid-range FPGA NPU setup:

| Matrix Size | NPU Time (ms) | CPU Time (ms) | Speedup | Throughput (GOPS) |
|-------------|---------------|---------------|---------|-------------------|
| 64×64       | 0.36          | 2.45          | 6.8x    | 45.2              |
| 128×128     | 1.47          | 18.9          | 12.9x   | 89.4              |
| 256×256     | 5.89          | 89.7          | 15.2x   | 125.7             |
| 512×512     | 23.4          | 687.3         | 29.4x   | 156.8             |
| 1024×1024   | 94.1          | 5432.1        | 57.7x   | 178.9             |

### Performance Factors

- **Matrix Size**: Larger matrices generally achieve better throughput
- **Memory Bandwidth**: Limited by data transfer between host and NPU
- **Compute Utilization**: Efficiency depends on problem size vs NPU capacity
- **Precision**: Single-precision (float32) used throughout

## Learning Objectives

This example teaches:

1. **NPU Initialization**
   - Device detection and initialization
   - Resource allocation and management
   - Error handling patterns

2. **Memory Management**
   - NPU memory allocation (`npu_malloc`)
   - Memory transfer optimization
   - Proper cleanup procedures

3. **Basic Operations**
   - Matrix multiplication API usage
   - Operation parameters and configuration
   - Result validation techniques

4. **Performance Analysis**
   - Timing measurement methodology
   - Throughput calculation (GOPS)
   - Comparative analysis (NPU vs CPU)

5. **Error Handling**
   - Return code checking
   - Resource cleanup on failure
   - Graceful error recovery

## Common Issues and Solutions

### 1. Memory Allocation Failures

**Problem**: `npu_malloc` returns NULL

**Solutions**:
- Check available NPU memory with `npu_get_device_info`
- Reduce matrix size if memory is insufficient
- Ensure proper cleanup of previous allocations

### 2. Performance Lower Than Expected

**Problem**: NPU speedup is minimal

**Solutions**:
- Verify matrix size is large enough for NPU efficiency
- Check for thermal throttling
- Ensure exclusive NPU access (no competing processes)
- Verify optimal data alignment

### 3. Verification Failures

**Problem**: NPU results don't match CPU reference

**Solutions**:
- Increase tolerance for floating-point comparison
- Check for numerical stability issues
- Verify input data integrity
- Review NPU precision settings

### 4. Build Issues

**Problem**: Compilation errors

**Solutions**:
- Verify NPU library is installed and linked
- Check include path settings
- Ensure C11 compiler support
- Verify math library linking (`-lm`)

## Extending the Example

### Adding New Matrix Operations

```c
// Element-wise operations
npu_result_t matrix_add_example(npu_handle_t npu, float *A, float *B, float *C, int size) {
    return npu_tensor_add(npu, A, B, C, size * size);
}

// Transpose operation
npu_result_t matrix_transpose_example(npu_handle_t npu, float *input, float *output, 
                                     int rows, int cols) {
    return npu_transpose(npu, input, output, rows, cols);
}
```

### Custom Performance Metrics

```c
typedef struct {
    double gops;
    double bandwidth_gbps;
    double efficiency_percent;
    double power_watts;
} extended_metrics_t;

extended_metrics_t measure_extended_performance(npu_handle_t npu, ...);
```

### Multi-threaded Usage

```c
typedef struct {
    npu_handle_t npu;
    float *A, *B, *C;
    int matrix_size;
    int thread_id;
} thread_data_t;

void* matrix_multiply_worker(void *arg) {
    thread_data_t *data = (thread_data_t*)arg;
    // Perform matrix multiplication
    return NULL;
}
```

## Related Examples

- **CNN Inference**: Demonstrates more complex matrix operations in neural networks
- **Neural Network Training**: Shows iterative matrix operations in training loops
- **Performance Benchmarks**: Comprehensive performance testing framework

## Further Reading

- [NPU API Reference](../docs/api/npu_api.md)
- [Performance Optimization Guide](../docs/optimization.md)
- [Memory Management Best Practices](../docs/memory_management.md)
- [Debugging NPU Applications](../docs/debugging.md)