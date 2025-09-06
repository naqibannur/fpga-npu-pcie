# FPGA NPU API Reference

This document provides comprehensive API reference for the FPGA NPU library, including all functions, data structures, and usage patterns.

## Table of Contents

- [Overview](#overview)
- [Data Types and Structures](#data-types-and-structures)
- [Core Functions](#core-functions)
- [Memory Management](#memory-management)
- [Tensor Operations](#tensor-operations)
- [Neural Network Operations](#neural-network-operations)
- [Performance Monitoring](#performance-monitoring)
- [Error Handling](#error-handling)
- [Examples](#examples)

## Overview

The FPGA NPU API provides a high-level interface for neural processing unit operations. It abstracts hardware complexities while providing efficient access to NPU capabilities.

### Library Initialization

```c
#include "fpga_npu_lib.h"

// Link with: -lfpga_npu -lm -lpthread
```

### Quick Start

```c
npu_handle_t npu;
npu_result_t result = npu_init(&npu);
if (result == NPU_SUCCESS) {
    // Use NPU operations
    npu_cleanup(npu);
}
```

## Data Types and Structures

### Basic Types

#### `npu_handle_t`
Opaque handle representing an NPU device instance.

```c
typedef struct npu_device* npu_handle_t;
```

#### `npu_result_t`
Return code for all NPU operations.

```c
typedef enum {
    NPU_SUCCESS = 0,                    // Operation completed successfully
    NPU_ERROR_INVALID_PARAMETER = -1,   // Invalid parameter provided
    NPU_ERROR_DEVICE_NOT_FOUND = -2,    // NPU device not found
    NPU_ERROR_MEMORY_ALLOCATION = -3,   // Memory allocation failed
    NPU_ERROR_OPERATION_FAILED = -4,    // Operation execution failed
    NPU_ERROR_TIMEOUT = -5,             // Operation timed out
    NPU_ERROR_DEVICE_BUSY = -6,         // Device is busy
    NPU_ERROR_UNSUPPORTED = -7,         // Operation not supported
    NPU_ERROR_DRIVER_ERROR = -8,        // Driver communication error
    NPU_ERROR_HARDWARE_ERROR = -9       // Hardware error detected
} npu_result_t;
```

### Device Information

#### `npu_device_info_t`
Structure containing NPU device information.

```c
typedef struct {
    uint16_t device_id;                 // PCI device ID
    uint16_t vendor_id;                 // PCI vendor ID
    uint32_t revision;                  // Hardware revision
    size_t memory_size;                 // Available memory in bytes
    uint32_t max_frequency_mhz;         // Maximum operating frequency
    uint32_t num_processing_elements;   // Number of processing elements
    char device_name[64];               // Human-readable device name
    char driver_version[32];            // Driver version string
    char firmware_version[32];          // Firmware version string
} npu_device_info_t;
```

#### `npu_power_info_t`
Structure for power monitoring information.

```c
typedef struct {
    double voltage_v;                   // Supply voltage in volts
    double current_a;                   // Current consumption in amperes
    double power_w;                     // Power consumption in watts
    double temperature_c;               // Temperature in Celsius
    bool thermal_throttling;            // Thermal throttling active
    uint32_t frequency_mhz;             // Current operating frequency
} npu_power_info_t;
```

### Performance Monitoring

#### `npu_performance_counters_t`
Performance monitoring counters.

```c
typedef struct {
    uint64_t total_operations;          // Total operations executed
    uint64_t total_cycles;              // Total clock cycles
    uint64_t memory_reads;              // Memory read operations
    uint64_t memory_writes;             // Memory write operations
    uint64_t cache_hits;                // Cache hit count
    uint64_t cache_misses;              // Cache miss count
    double utilization_percent;         // Overall utilization percentage
} npu_performance_counters_t;
```

### Memory Management

#### `npu_memory_info_t`
Memory usage information.

```c
typedef struct {
    size_t total_memory;                // Total available memory
    size_t used_memory;                 // Currently used memory
    size_t free_memory;                 // Available free memory
    size_t largest_free_block;          // Largest contiguous free block
    uint32_t allocation_count;          // Number of active allocations
} npu_memory_info_t;
```

## Core Functions

### Device Management

#### `npu_init`
Initialize NPU device and create handle.

```c
npu_result_t npu_init(npu_handle_t *handle);
```

**Parameters:**
- `handle`: Pointer to receive NPU handle

**Returns:**
- `NPU_SUCCESS`: Device initialized successfully
- `NPU_ERROR_DEVICE_NOT_FOUND`: No NPU device found
- `NPU_ERROR_DRIVER_ERROR`: Driver communication failed
- `NPU_ERROR_INVALID_PARAMETER`: Invalid handle pointer

**Example:**
```c
npu_handle_t npu;
npu_result_t result = npu_init(&npu);
if (result != NPU_SUCCESS) {
    printf("NPU initialization failed: %d\n", result);
    return -1;
}
```

#### `npu_cleanup`
Clean up NPU resources and close device.

```c
void npu_cleanup(npu_handle_t handle);
```

**Parameters:**
- `handle`: NPU handle to cleanup

**Example:**
```c
npu_cleanup(npu);
```

#### `npu_get_device_info`
Retrieve device information.

```c
npu_result_t npu_get_device_info(npu_handle_t handle, npu_device_info_t *info);
```

**Parameters:**
- `handle`: NPU handle
- `info`: Pointer to device info structure

**Returns:**
- `NPU_SUCCESS`: Information retrieved successfully
- `NPU_ERROR_INVALID_PARAMETER`: Invalid parameters

**Example:**
```c
npu_device_info_t info;
if (npu_get_device_info(npu, &info) == NPU_SUCCESS) {
    printf("Device: %s\n", info.device_name);
    printf("Memory: %zu MB\n", info.memory_size / (1024 * 1024));
}
```

### Device Status and Control

#### `npu_reset`
Reset NPU device to initial state.

```c
npu_result_t npu_reset(npu_handle_t handle);
```

#### `npu_wait_idle`
Wait for NPU to become idle.

```c
npu_result_t npu_wait_idle(npu_handle_t handle);
```

#### `npu_get_status`
Get current device status.

```c
npu_result_t npu_get_status(npu_handle_t handle, uint32_t *status);
```

**Status bits:**
- `0x01`: Device ready
- `0x02`: Operation in progress
- `0x04`: Error condition
- `0x08`: Thermal throttling active

## Memory Management

### Memory Allocation

#### `npu_malloc`
Allocate memory on NPU device.

```c
void* npu_malloc(npu_handle_t handle, size_t size);
```

**Parameters:**
- `handle`: NPU handle
- `size`: Size in bytes to allocate

**Returns:**
- Pointer to allocated memory on success
- `NULL` on allocation failure

**Example:**
```c
size_t matrix_size = 1024 * 1024 * sizeof(float);
float *matrix = npu_malloc(npu, matrix_size);
if (!matrix) {
    printf("Memory allocation failed\n");
    return -1;
}
```

#### `npu_free`
Free NPU memory.

```c
void npu_free(npu_handle_t handle, void *ptr);
```

**Parameters:**
- `handle`: NPU handle
- `ptr`: Pointer to memory to free

**Example:**
```c
npu_free(npu, matrix);
```

#### `npu_malloc_aligned`
Allocate aligned memory on NPU.

```c
void* npu_malloc_aligned(npu_handle_t handle, size_t size, size_t alignment);
```

**Parameters:**
- `handle`: NPU handle
- `size`: Size in bytes to allocate
- `alignment`: Memory alignment requirement (power of 2)

### Memory Operations

#### `npu_memory_copy`
Copy memory between NPU buffers.

```c
npu_result_t npu_memory_copy(npu_handle_t handle, const void *src, void *dst, size_t size);
```

#### `npu_memory_set`
Set NPU memory to specific value.

```c
npu_result_t npu_memory_set(npu_handle_t handle, void *ptr, int value, size_t size);
```

#### `npu_get_memory_info`
Get memory usage information.

```c
npu_result_t npu_get_memory_info(npu_handle_t handle, npu_memory_info_t *info);
```

## Tensor Operations

### Basic Operations

#### `npu_tensor_add`
Element-wise tensor addition: C = A + B.

```c
npu_result_t npu_tensor_add(npu_handle_t handle, 
                           const float *input_a, 
                           const float *input_b, 
                           float *output, 
                           size_t num_elements);
```

**Parameters:**
- `handle`: NPU handle
- `input_a`: First input tensor
- `input_b`: Second input tensor
- `output`: Output tensor
- `num_elements`: Number of elements in tensors

**Example:**
```c
float *a = npu_malloc(npu, 1000 * sizeof(float));
float *b = npu_malloc(npu, 1000 * sizeof(float));
float *c = npu_malloc(npu, 1000 * sizeof(float));

npu_result_t result = npu_tensor_add(npu, a, b, c, 1000);
```

#### `npu_tensor_multiply`
Element-wise tensor multiplication: C = A ⊙ B.

```c
npu_result_t npu_tensor_multiply(npu_handle_t handle,
                                const float *input_a,
                                const float *input_b,
                                float *output,
                                size_t num_elements);
```

#### `npu_tensor_subtract`
Element-wise tensor subtraction: C = A - B.

```c
npu_result_t npu_tensor_subtract(npu_handle_t handle,
                                const float *input_a,
                                const float *input_b,
                                float *output,
                                size_t num_elements);
```

#### `npu_tensor_scale`
Scale tensor by scalar: B = α × A.

```c
npu_result_t npu_tensor_scale(npu_handle_t handle,
                             const float *input,
                             float scale,
                             float *output,
                             size_t num_elements);
```

### Matrix Operations

#### `npu_matrix_multiply`
Matrix multiplication: C = A × B.

```c
npu_result_t npu_matrix_multiply(npu_handle_t handle,
                                const float *matrix_a,
                                const float *matrix_b,
                                float *matrix_c,
                                size_t rows_a,
                                size_t cols_a,
                                size_t cols_b);
```

**Parameters:**
- `handle`: NPU handle
- `matrix_a`: Input matrix A (rows_a × cols_a)
- `matrix_b`: Input matrix B (cols_a × cols_b)
- `matrix_c`: Output matrix C (rows_a × cols_b)
- `rows_a`: Number of rows in matrix A
- `cols_a`: Number of columns in A / rows in B
- `cols_b`: Number of columns in matrix B

**Example:**
```c
// Multiply 256x512 matrix with 512x128 matrix
npu_result_t result = npu_matrix_multiply(npu, a, b, c, 256, 512, 128);
```

#### `npu_batch_matrix_multiply`
Batched matrix multiplication for multiple matrices.

```c
npu_result_t npu_batch_matrix_multiply(npu_handle_t handle,
                                      const float *batch_a,
                                      const float *batch_b,
                                      float *batch_c,
                                      size_t batch_size,
                                      size_t rows_a,
                                      size_t cols_a,
                                      size_t cols_b);
```

#### `npu_transpose`
Matrix transpose: B = Aᵀ.

```c
npu_result_t npu_transpose(npu_handle_t handle,
                          const float *input,
                          float *output,
                          size_t rows,
                          size_t cols);
```

## Neural Network Operations

### Convolution Operations

#### `npu_conv2d`
2D convolution operation.

```c
npu_result_t npu_conv2d(npu_handle_t handle,
                       const float *input,
                       const float *kernel,
                       float *output,
                       size_t input_height,
                       size_t input_width,
                       size_t input_channels,
                       size_t output_channels,
                       size_t kernel_height,
                       size_t kernel_width,
                       size_t stride_y,
                       size_t stride_x,
                       size_t padding_y,
                       size_t padding_x);
```

**Parameters:**
- `input`: Input tensor (H × W × C_in)
- `kernel`: Convolution kernels (K_h × K_w × C_in × C_out)
- `output`: Output tensor (H_out × W_out × C_out)
- `stride_y`, `stride_x`: Convolution strides
- `padding_y`, `padding_x`: Zero padding

#### `npu_conv1d`
1D convolution operation.

```c
npu_result_t npu_conv1d(npu_handle_t handle,
                       const float *input,
                       const float *kernel,
                       float *output,
                       size_t input_length,
                       size_t input_channels,
                       size_t output_channels,
                       size_t kernel_size,
                       size_t stride,
                       size_t padding);
```

### Pooling Operations

#### `npu_maxpool2d`
2D max pooling operation.

```c
npu_result_t npu_maxpool2d(npu_handle_t handle,
                          const float *input,
                          float *output,
                          size_t input_height,
                          size_t input_width,
                          size_t channels,
                          size_t pool_height,
                          size_t pool_width,
                          size_t stride_y,
                          size_t stride_x);
```

#### `npu_avgpool2d`
2D average pooling operation.

```c
npu_result_t npu_avgpool2d(npu_handle_t handle,
                          const float *input,
                          float *output,
                          size_t input_height,
                          size_t input_width,
                          size_t channels,
                          size_t pool_height,
                          size_t pool_width,
                          size_t stride_y,
                          size_t stride_x);
```

### Activation Functions

#### `npu_relu`
ReLU activation: f(x) = max(0, x).

```c
npu_result_t npu_relu(npu_handle_t handle,
                     const float *input,
                     float *output,
                     size_t num_elements);
```

#### `npu_sigmoid`
Sigmoid activation: f(x) = 1/(1 + e^(-x)).

```c
npu_result_t npu_sigmoid(npu_handle_t handle,
                        const float *input,
                        float *output,
                        size_t num_elements);
```

#### `npu_tanh`
Hyperbolic tangent activation.

```c
npu_result_t npu_tanh(npu_handle_t handle,
                     const float *input,
                     float *output,
                     size_t num_elements);
```

#### `npu_softmax`
Softmax activation for classification.

```c
npu_result_t npu_softmax(npu_handle_t handle,
                        const float *input,
                        float *output,
                        size_t num_elements);
```

### Normalization Operations

#### `npu_batch_norm`
Batch normalization.

```c
npu_result_t npu_batch_norm(npu_handle_t handle,
                           const float *input,
                           const float *gamma,
                           const float *beta,
                           const float *mean,
                           const float *variance,
                           float *output,
                           size_t batch_size,
                           size_t channels,
                           size_t height,
                           size_t width,
                           float epsilon);
```

#### `npu_layer_norm`
Layer normalization.

```c
npu_result_t npu_layer_norm(npu_handle_t handle,
                           const float *input,
                           const float *gamma,
                           const float *beta,
                           float *output,
                           size_t batch_size,
                           size_t feature_size,
                           float epsilon);
```

### Fully Connected Operations

#### `npu_fully_connected`
Fully connected (dense) layer: Y = XW + b.

```c
npu_result_t npu_fully_connected(npu_handle_t handle,
                                const float *input,
                                const float *weights,
                                const float *biases,
                                float *output,
                                size_t input_size,
                                size_t output_size);
```

#### `npu_add_bias`
Add bias to tensor.

```c
npu_result_t npu_add_bias(npu_handle_t handle,
                         const float *input,
                         const float *bias,
                         float *output,
                         size_t num_elements);
```

## Performance Monitoring

### Performance Counters

#### `npu_get_performance_counters`
Retrieve performance monitoring counters.

```c
npu_result_t npu_get_performance_counters(npu_handle_t handle,
                                         npu_performance_counters_t *counters);
```

#### `npu_reset_performance_counters`
Reset performance counters to zero.

```c
npu_result_t npu_reset_performance_counters(npu_handle_t handle);
```

### Power Monitoring

#### `npu_get_power_info`
Get power consumption information.

```c
npu_result_t npu_get_power_info(npu_handle_t handle, npu_power_info_t *power);
```

#### `npu_set_power_limit`
Set power consumption limit.

```c
npu_result_t npu_set_power_limit(npu_handle_t handle, double power_limit_watts);
```

### Profiling

#### `npu_start_profiling`
Start operation profiling.

```c
npu_result_t npu_start_profiling(npu_handle_t handle);
```

#### `npu_stop_profiling`
Stop profiling and retrieve results.

```c
npu_result_t npu_stop_profiling(npu_handle_t handle, char *profile_data, size_t buffer_size);
```

## Error Handling

### Error Code Conversion

#### `npu_get_error_string`
Convert error code to human-readable string.

```c
const char* npu_get_error_string(npu_result_t error);
```

**Example:**
```c
npu_result_t result = npu_matrix_multiply(npu, a, b, c, 256, 256, 256);
if (result != NPU_SUCCESS) {
    printf("Operation failed: %s\n", npu_get_error_string(result));
}
```

### Error Handling Patterns

#### Basic Error Checking
```c
npu_result_t result = npu_operation(handle, ...);
if (result != NPU_SUCCESS) {
    printf("Operation failed: %s\n", npu_get_error_string(result));
    return result;
}
```

#### Resource Cleanup on Error
```c
float *buffer1 = npu_malloc(npu, size1);
float *buffer2 = npu_malloc(npu, size2);

if (!buffer1 || !buffer2) {
    if (buffer1) npu_free(npu, buffer1);
    if (buffer2) npu_free(npu, buffer2);
    return NPU_ERROR_MEMORY_ALLOCATION;
}

npu_result_t result = npu_operation(npu, buffer1, buffer2, ...);
if (result != NPU_SUCCESS) {
    npu_free(npu, buffer1);
    npu_free(npu, buffer2);
    return result;
}
```

## Examples

### Complete Matrix Multiplication Example

```c
#include "fpga_npu_lib.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Initialize NPU
    npu_handle_t npu;
    npu_result_t result = npu_init(&npu);
    if (result != NPU_SUCCESS) {
        printf("NPU initialization failed: %s\n", npu_get_error_string(result));
        return -1;
    }
    
    // Allocate matrices
    const size_t size = 256;
    const size_t matrix_bytes = size * size * sizeof(float);
    
    float *a = npu_malloc(npu, matrix_bytes);
    float *b = npu_malloc(npu, matrix_bytes);
    float *c = npu_malloc(npu, matrix_bytes);
    
    if (!a || !b || !c) {
        printf("Memory allocation failed\n");
        goto cleanup;
    }
    
    // Initialize matrices (implementation specific)
    initialize_matrix(a, size, size);
    initialize_matrix(b, size, size);
    
    // Perform matrix multiplication
    result = npu_matrix_multiply(npu, a, b, c, size, size, size);
    if (result != NPU_SUCCESS) {
        printf("Matrix multiplication failed: %s\n", npu_get_error_string(result));
        goto cleanup;
    }
    
    printf("Matrix multiplication completed successfully\n");
    
cleanup:
    if (a) npu_free(npu, a);
    if (b) npu_free(npu, b);
    if (c) npu_free(npu, c);
    npu_cleanup(npu);
    
    return (result == NPU_SUCCESS) ? 0 : -1;
}
```

### Neural Network Layer Example

```c
// Implement a simple neural network layer
npu_result_t neural_network_layer(npu_handle_t npu,
                                 const float *input,
                                 const float *weights,
                                 const float *biases,
                                 float *output,
                                 size_t input_size,
                                 size_t output_size) {
    // Fully connected operation
    npu_result_t result = npu_fully_connected(npu, input, weights, biases, 
                                             output, input_size, output_size);
    if (result != NPU_SUCCESS) {
        return result;
    }
    
    // Apply ReLU activation
    return npu_relu(npu, output, output, output_size);
}
```

## Best Practices

### 1. Memory Management
- Always check return values from `npu_malloc`
- Free all allocated memory with `npu_free`
- Use aligned allocation for better performance
- Monitor memory usage with `npu_get_memory_info`

### 2. Error Handling
- Check return codes from all NPU operations
- Use `npu_get_error_string` for user-friendly error messages
- Implement proper cleanup on errors
- Log errors for debugging

### 3. Performance Optimization
- Use batch operations when possible
- Minimize memory transfers between host and NPU
- Align data to optimal boundaries
- Monitor performance with profiling tools

### 4. Resource Management
- Initialize NPU once per application
- Reuse memory allocations when possible
- Use `npu_wait_idle` before cleanup
- Monitor device status for errors

## Thread Safety

The NPU library is **not thread-safe** by default. For multi-threaded applications:

1. Use separate NPU handles per thread, or
2. Implement external synchronization (mutexes, etc.)

```c
// Example: Thread-safe wrapper
static pthread_mutex_t npu_mutex = PTHREAD_MUTEX_INITIALIZER;

npu_result_t thread_safe_matrix_multiply(npu_handle_t npu, ...) {
    pthread_mutex_lock(&npu_mutex);
    npu_result_t result = npu_matrix_multiply(npu, ...);
    pthread_mutex_unlock(&npu_mutex);
    return result;
}
```

## Troubleshooting

### Common Issues

1. **NPU_ERROR_DEVICE_NOT_FOUND**: Check driver installation and hardware connection
2. **NPU_ERROR_MEMORY_ALLOCATION**: Monitor memory usage and free unused allocations
3. **NPU_ERROR_TIMEOUT**: Check for deadlocks or increase timeout values
4. **NPU_ERROR_HARDWARE_ERROR**: Check for thermal issues or hardware faults

### Debugging Tips

1. Enable verbose logging in debug builds
2. Use `npu_get_device_info` to verify hardware status
3. Monitor performance counters for anomalies
4. Check power and thermal status regularly