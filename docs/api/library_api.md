# FPGA NPU API Documentation

## Overview

The FPGA NPU library provides a high-level C API for neural processing operations. The API abstracts the low-level hardware details and provides efficient interfaces for common deep learning operations.

## Core Concepts

### NPU Handle

All operations require an NPU handle obtained through `npu_init()`:

```c
npu_handle_t handle = npu_init();
if (!handle) {
    // Handle initialization error
}
```

### Tensors

Data is represented using tensor descriptors:

```c
typedef struct {
    void *data;           // Pointer to data
    size_t size;          // Total size in bytes
    uint32_t dims[4];     // Dimensions [N,C,H,W]
    npu_dtype_t dtype;    // Data type
} npu_tensor_t;
```

### Error Handling

All functions return error codes:

```c
#define NPU_SUCCESS           0
#define NPU_ERROR_INIT       -1
#define NPU_ERROR_DEVICE     -2
#define NPU_ERROR_MEMORY     -3
#define NPU_ERROR_TIMEOUT    -4
#define NPU_ERROR_INVALID    -5
```

## Initialization and Cleanup

### npu_init()

Initialize the NPU library and open the device.

```c
npu_handle_t npu_init(void);
```

**Returns**: NPU handle on success, NULL on failure.

**Example**:
```c
npu_handle_t npu = npu_init();
if (!npu) {
    fprintf(stderr, "Failed to initialize NPU\n");
    exit(1);
}
```

### npu_cleanup()

Clean up resources and close the NPU device.

```c
int npu_cleanup(npu_handle_t handle);
```

**Parameters**:
- `handle`: NPU handle

**Returns**: `NPU_SUCCESS` on success, error code on failure.

**Example**:
```c
int ret = npu_cleanup(npu);
if (ret != NPU_SUCCESS) {
    fprintf(stderr, "Cleanup failed: %d\n", ret);
}
```

## Memory Management

### npu_alloc()

Allocate memory accessible by the NPU.

```c
void* npu_alloc(npu_handle_t handle, size_t size);
```

**Parameters**:
- `handle`: NPU handle
- `size`: Size in bytes

**Returns**: Pointer to allocated memory, NULL on failure.

**Example**:
```c
float *data = (float*)npu_alloc(npu, 1024 * sizeof(float));
if (!data) {
    fprintf(stderr, "Memory allocation failed\n");
}
```

### npu_free()

Free NPU-accessible memory.

```c
void npu_free(npu_handle_t handle, void *ptr);
```

**Parameters**:
- `handle`: NPU handle
- `ptr`: Pointer to memory to free

**Example**:
```c
npu_free(npu, data);
```

## Tensor Operations

### npu_create_tensor()

Create a tensor descriptor.

```c
npu_tensor_t npu_create_tensor(void *data, uint32_t n, uint32_t c, 
                               uint32_t h, uint32_t w, npu_dtype_t dtype);
```

**Parameters**:
- `data`: Pointer to tensor data
- `n`: Batch dimension
- `c`: Channel dimension  
- `h`: Height dimension
- `w`: Width dimension
- `dtype`: Data type

**Returns**: Tensor descriptor.

**Example**:
```c
float *matrix_data = npu_alloc(npu, 64 * 64 * sizeof(float));
npu_tensor_t matrix = npu_create_tensor(matrix_data, 1, 1, 64, 64, NPU_DTYPE_FLOAT32);
```

## High-Level Operations

### npu_matrix_multiply()

Perform matrix multiplication: C = A × B

```c
int npu_matrix_multiply(npu_handle_t handle, const npu_tensor_t *a, 
                        const npu_tensor_t *b, npu_tensor_t *c);
```

**Parameters**:
- `handle`: NPU handle
- `a`: Input matrix A
- `b`: Input matrix B  
- `c`: Output matrix C

**Returns**: `NPU_SUCCESS` on success, error code on failure.

**Example**:
```c
// Create matrices A (64x32) and B (32x64)
npu_tensor_t a = npu_create_tensor(data_a, 1, 1, 64, 32, NPU_DTYPE_FLOAT32);
npu_tensor_t b = npu_create_tensor(data_b, 1, 1, 32, 64, NPU_DTYPE_FLOAT32);
npu_tensor_t c = npu_create_tensor(data_c, 1, 1, 64, 64, NPU_DTYPE_FLOAT32);

int ret = npu_matrix_multiply(npu, &a, &b, &c);
if (ret != NPU_SUCCESS) {
    fprintf(stderr, "Matrix multiplication failed: %d\n", ret);
}
```

### npu_conv2d()

Perform 2D convolution operation.

```c
int npu_conv2d(npu_handle_t handle, const npu_tensor_t *input, 
               const npu_tensor_t *weights, npu_tensor_t *output,
               uint32_t stride_h, uint32_t stride_w, 
               uint32_t pad_h, uint32_t pad_w);
```

**Parameters**:
- `handle`: NPU handle
- `input`: Input tensor (NCHW format)
- `weights`: Convolution weights
- `output`: Output tensor
- `stride_h`: Vertical stride
- `stride_w`: Horizontal stride
- `pad_h`: Vertical padding
- `pad_w`: Horizontal padding

**Returns**: `NPU_SUCCESS` on success, error code on failure.

**Example**:
```c
// 3x3 convolution with stride 1, padding 1
npu_tensor_t input = npu_create_tensor(input_data, 1, 3, 224, 224, NPU_DTYPE_FLOAT32);
npu_tensor_t weights = npu_create_tensor(weight_data, 64, 3, 3, 3, NPU_DTYPE_FLOAT32);
npu_tensor_t output = npu_create_tensor(output_data, 1, 64, 224, 224, NPU_DTYPE_FLOAT32);

int ret = npu_conv2d(npu, &input, &weights, &output, 1, 1, 1, 1);
```

### npu_add()

Perform element-wise addition: C = A + B

```c
int npu_add(npu_handle_t handle, const npu_tensor_t *a, 
            const npu_tensor_t *b, npu_tensor_t *c);
```

**Parameters**:
- `handle`: NPU handle
- `a`: Input tensor A
- `b`: Input tensor B
- `c`: Output tensor C

**Returns**: `NPU_SUCCESS` on success, error code on failure.

### npu_multiply()

Perform element-wise multiplication: C = A * B

```c
int npu_multiply(npu_handle_t handle, const npu_tensor_t *a, 
                 const npu_tensor_t *b, npu_tensor_t *c);
```

## Low-Level Operations

### npu_execute_instruction()

Execute a single NPU instruction.

```c
int npu_execute_instruction(npu_handle_t handle, const npu_instruction_t *inst);
```

**Parameters**:
- `handle`: NPU handle
- `inst`: Instruction to execute

**Returns**: `NPU_SUCCESS` on success, error code on failure.

### npu_execute_batch()

Execute a batch of NPU instructions.

```c
int npu_execute_batch(npu_handle_t handle, const npu_instruction_t *instructions, 
                      size_t count);
```

**Parameters**:
- `handle`: NPU handle
- `instructions`: Array of instructions
- `count`: Number of instructions

**Returns**: `NPU_SUCCESS` on success, error code on failure.

## Synchronization

### npu_wait_completion()

Wait for NPU operation completion.

```c
int npu_wait_completion(npu_handle_t handle, uint32_t timeout_ms);
```

**Parameters**:
- `handle`: NPU handle
- `timeout_ms`: Timeout in milliseconds (0 = infinite)

**Returns**: `NPU_SUCCESS` on success, error code on failure.

### npu_get_status()

Get current NPU status.

```c
int npu_get_status(npu_handle_t handle, uint32_t *status);
```

**Parameters**:
- `handle`: NPU handle
- `status`: Pointer to status variable

**Returns**: `NPU_SUCCESS` on success, error code on failure.

**Status Bits**:
- Bit 0: Ready
- Bit 1: Busy  
- Bit 2: Error
- Bit 3: Done

## Performance Monitoring

### npu_get_performance_counters()

Get NPU performance counters.

```c
int npu_get_performance_counters(npu_handle_t handle, uint64_t *cycles, 
                                 uint64_t *operations);
```

**Parameters**:
- `handle`: NPU handle
- `cycles`: Total clock cycles
- `operations`: Total operations executed

**Returns**: `NPU_SUCCESS` on success, error code on failure.

### npu_reset_performance_counters()

Reset NPU performance counters.

```c
int npu_reset_performance_counters(npu_handle_t handle);
```

## Complete Example

```c
#include "fpga_npu_lib.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Initialize NPU
    npu_handle_t npu = npu_init();
    if (!npu) {
        fprintf(stderr, "Failed to initialize NPU\n");
        return 1;
    }
    
    // Allocate matrices
    float *a_data = npu_alloc(npu, 64 * 32 * sizeof(float));
    float *b_data = npu_alloc(npu, 32 * 64 * sizeof(float));
    float *c_data = npu_alloc(npu, 64 * 64 * sizeof(float));
    
    // Initialize input data
    for (int i = 0; i < 64 * 32; i++) {
        a_data[i] = (float)rand() / RAND_MAX;
    }
    for (int i = 0; i < 32 * 64; i++) {
        b_data[i] = (float)rand() / RAND_MAX;
    }
    
    // Create tensors
    npu_tensor_t a = npu_create_tensor(a_data, 1, 1, 64, 32, NPU_DTYPE_FLOAT32);
    npu_tensor_t b = npu_create_tensor(b_data, 1, 1, 32, 64, NPU_DTYPE_FLOAT32);
    npu_tensor_t c = npu_create_tensor(c_data, 1, 1, 64, 64, NPU_DTYPE_FLOAT32);
    
    // Perform matrix multiplication
    int ret = npu_matrix_multiply(npu, &a, &b, &c);
    if (ret != NPU_SUCCESS) {
        fprintf(stderr, "Matrix multiplication failed: %d\n", ret);
        goto cleanup;
    }
    
    printf("Matrix multiplication completed successfully\n");
    
cleanup:
    // Free memory
    npu_free(npu, a_data);
    npu_free(npu, b_data);
    npu_free(npu, c_data);
    
    // Cleanup NPU
    npu_cleanup(npu);
    
    return ret == NPU_SUCCESS ? 0 : 1;
}
```