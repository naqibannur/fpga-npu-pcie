# FPGA NPU Tutorial

This tutorial provides step-by-step guidance for getting started with the FPGA NPU, covering basic concepts, setup, programming patterns, and practical examples.

## Table of Contents

- [Getting Started](#getting-started)
- [Basic Concepts](#basic-concepts)
- [Your First NPU Program](#your-first-npu-program)
- [Common Programming Patterns](#common-programming-patterns)
- [Advanced Examples](#advanced-examples)
- [Troubleshooting](#troubleshooting)

## Getting Started

### Prerequisites

Before starting this tutorial, ensure you have:

- FPGA NPU hardware properly installed
- Linux system with kernel 4.19 or later
- GCC compiler and development tools
- Basic understanding of C programming
- Familiarity with neural network concepts (helpful but not required)

### Installation Verification

Let's verify your NPU installation:

```bash
# Check if the NPU driver is loaded
lsmod | grep fpga_npu

# Verify NPU device is detected
lspci | grep -i npu

# Check device permissions
ls -l /dev/fpga_npu*

# Test basic functionality
./scripts/test_installation.sh
```

If any of these commands fail, refer to the [User Guide](user_guide.md) for installation instructions.

## Basic Concepts

### NPU Architecture Overview

The FPGA NPU consists of several key components:

```
┌─────────────────────────────────────────────────────────┐
│                   Your Application                     │
├─────────────────────────────────────────────────────────┤
│                   NPU Library                          │
│  • Memory Management  • Tensor Operations              │
│  • Performance Monitor • Error Handling                │
├─────────────────────────────────────────────────────────┤
│                 Kernel Driver                          │
│  • Device Control    • DMA Management                  │
│  • Interrupt Handling • Power Management               │
├─────────────────────────────────────────────────────────┤
│                  NPU Hardware                          │
│  • Processing Elements (PEs) • Memory Hierarchy        │
│  • Command Processor • PCIe Interface                  │
└─────────────────────────────────────────────────────────┘
```

### Key Concepts

**NPU Handle**: A handle representing your connection to the NPU device
**Processing Elements (PEs)**: Parallel compute units that execute operations
**NPU Memory**: Fast on-device memory for storing data and intermediate results
**Commands**: Operations sent to the NPU for execution
**Performance Counters**: Hardware metrics for monitoring and optimization

### Memory Model

Understanding the NPU memory model is crucial for efficient programming:

```
Host Memory        NPU Memory
┌─────────────┐    ┌─────────────┐
│ Application │    │   L1 Cache  │ ← Fastest access
│    Data     │    │   (32KB)    │
└─────────────┘    ├─────────────┤
       │           │   L2 Cache  │ ← Shared between PEs
       │           │   (8MB)     │
       ▼           ├─────────────┤
┌─────────────┐    │ DDR4 Memory │ ← Largest capacity
│ PCIe Transfer│ ←→ │   (16GB)    │
└─────────────┘    └─────────────┘
```

## Your First NPU Program

Let's create a simple program that performs vector addition using the NPU.

### Step 1: Include Headers and Setup

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fpga_npu_lib.h"

int main() {
    printf("=== NPU Vector Addition Tutorial ===\n");
    
    // Initialize NPU
    npu_handle_t npu;
    npu_result_t result = npu_init(&npu);
    if (result != NPU_SUCCESS) {
        printf("Failed to initialize NPU: %s\n", npu_get_error_string(result));
        return -1;
    }
    
    printf("✅ NPU initialized successfully\n");
```

### Step 2: Allocate Memory

```c
    // Define vector size
    const size_t VECTOR_SIZE = 1024;
    const size_t BUFFER_SIZE = VECTOR_SIZE * sizeof(float);
    
    // Allocate NPU memory for input vectors and result
    float *vector_a = npu_malloc(npu, BUFFER_SIZE);
    float *vector_b = npu_malloc(npu, BUFFER_SIZE);
    float *vector_c = npu_malloc(npu, BUFFER_SIZE);
    
    if (!vector_a || !vector_b || !vector_c) {
        printf("❌ Failed to allocate NPU memory\n");
        npu_cleanup(npu);
        return -1;
    }
    
    printf("✅ Allocated %zu bytes for each vector\n", BUFFER_SIZE);
```

### Step 3: Initialize Data

```c
    // Initialize input vectors with test data
    printf("Initializing input vectors...\n");
    
    for (size_t i = 0; i < VECTOR_SIZE; i++) {
        vector_a[i] = (float)i;           // 0, 1, 2, 3, ...
        vector_b[i] = (float)(i * 2);     // 0, 2, 4, 6, ...
    }
    
    printf("✅ Input vectors initialized\n");
    printf("   vector_a[0] = %.1f, vector_a[1] = %.1f, ...\n", 
           vector_a[0], vector_a[1]);
    printf("   vector_b[0] = %.1f, vector_b[1] = %.1f, ...\n", 
           vector_b[0], vector_b[1]);
```

### Step 4: Perform NPU Operation

```c
    // Perform vector addition: C = A + B
    printf("Performing vector addition on NPU...\n");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    result = npu_tensor_add(npu, vector_a, vector_b, vector_c, VECTOR_SIZE);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    if (result != NPU_SUCCESS) {
        printf("❌ Vector addition failed: %s\n", npu_get_error_string(result));
        goto cleanup;
    }
    
    double duration_ms = (end.tv_sec - start.tv_sec) * 1000.0 + 
                        (end.tv_nsec - start.tv_nsec) / 1e6;
    
    printf("✅ Vector addition completed in %.3f ms\n", duration_ms);
```

### Step 5: Verify Results

```c
    // Verify results
    printf("Verifying results...\n");
    
    bool correct = true;
    for (size_t i = 0; i < VECTOR_SIZE && i < 10; i++) {
        float expected = vector_a[i] + vector_b[i];
        if (fabsf(vector_c[i] - expected) > 1e-6) {
            printf("❌ Mismatch at index %zu: expected %.1f, got %.1f\n", 
                   i, expected, vector_c[i]);
            correct = false;
        }
    }
    
    if (correct) {
        printf("✅ Results verified correct!\n");
        printf("   vector_c[0] = %.1f (%.1f + %.1f)\n", 
               vector_c[0], vector_a[0], vector_b[0]);
        printf("   vector_c[1] = %.1f (%.1f + %.1f)\n", 
               vector_c[1], vector_a[1], vector_b[1]);
    }
```

### Step 6: Cleanup

```c
cleanup:
    // Free NPU memory
    npu_free(npu, vector_a);
    npu_free(npu, vector_b);
    npu_free(npu, vector_c);
    
    // Cleanup NPU
    npu_cleanup(npu);
    
    printf("✅ Cleanup completed\n");
    return correct ? 0 : -1;
}
```

### Compiling and Running

```bash
# Compile the program
gcc -o vector_add_tutorial vector_add_tutorial.c -lfpga_npu -lm

# Run the program
./vector_add_tutorial
```

**Expected Output**:
```
=== NPU Vector Addition Tutorial ===
✅ NPU initialized successfully
✅ Allocated 4096 bytes for each vector
Initializing input vectors...
✅ Input vectors initialized
   vector_a[0] = 0.0, vector_a[1] = 1.0, ...
   vector_b[0] = 0.0, vector_b[1] = 2.0, ...
Performing vector addition on NPU...
✅ Vector addition completed in 0.123 ms
Verifying results...
✅ Results verified correct!
   vector_c[0] = 0.0 (0.0 + 0.0)
   vector_c[1] = 3.0 (1.0 + 2.0)
✅ Cleanup completed
```

## Common Programming Patterns

### Pattern 1: Error Handling

Always check return values and handle errors gracefully:

```c
npu_result_t safe_npu_operation(npu_handle_t npu) {
    float *buffer = NULL;
    npu_result_t result;
    
    // Allocate memory
    buffer = npu_malloc(npu, 1024 * sizeof(float));
    if (!buffer) {
        printf("Memory allocation failed\n");
        return NPU_ERROR_MEMORY_ALLOCATION;
    }
    
    // Perform operation
    result = npu_tensor_scale(npu, buffer, 2.0f, buffer, 1024);
    if (result != NPU_SUCCESS) {
        printf("Operation failed: %s\n", npu_get_error_string(result));
        goto cleanup;
    }
    
    printf("Operation completed successfully\n");
    
cleanup:
    if (buffer) {
        npu_free(npu, buffer);
    }
    return result;
}
```

### Pattern 2: Memory Management

Use RAII-style resource management for safer code:

```c
typedef struct {
    npu_handle_t npu;
    void **buffers;
    size_t num_buffers;
    size_t capacity;
} npu_memory_manager_t;

npu_memory_manager_t* create_memory_manager(npu_handle_t npu) {
    npu_memory_manager_t *mgr = malloc(sizeof(npu_memory_manager_t));
    mgr->npu = npu;
    mgr->num_buffers = 0;
    mgr->capacity = 16;
    mgr->buffers = malloc(mgr->capacity * sizeof(void*));
    return mgr;
}

void* managed_malloc(npu_memory_manager_t *mgr, size_t size) {
    void *ptr = npu_malloc(mgr->npu, size);
    if (ptr) {
        // Expand array if needed
        if (mgr->num_buffers >= mgr->capacity) {
            mgr->capacity *= 2;
            mgr->buffers = realloc(mgr->buffers, mgr->capacity * sizeof(void*));
        }
        mgr->buffers[mgr->num_buffers++] = ptr;
    }
    return ptr;
}

void destroy_memory_manager(npu_memory_manager_t *mgr) {
    for (size_t i = 0; i < mgr->num_buffers; i++) {
        npu_free(mgr->npu, mgr->buffers[i]);
    }
    free(mgr->buffers);
    free(mgr);
}
```

### Pattern 3: Performance Monitoring

Monitor performance to optimize your applications:

```c
typedef struct {
    clock_t start_time;
    npu_performance_counters_t start_counters;
    npu_handle_t npu;
} performance_timer_t;

performance_timer_t* start_timer(npu_handle_t npu) {
    performance_timer_t *timer = malloc(sizeof(performance_timer_t));
    timer->npu = npu;
    timer->start_time = clock();
    npu_get_performance_counters(npu, &timer->start_counters);
    return timer;
}

void print_performance(performance_timer_t *timer, const char *operation_name) {
    clock_t end_time = clock();
    npu_performance_counters_t end_counters;
    npu_get_performance_counters(timer->npu, &end_counters);
    
    double cpu_time = ((double)(end_time - timer->start_time)) / CLOCKS_PER_SEC;
    uint64_t operations = end_counters.total_operations - timer->start_counters.total_operations;
    
    printf("Performance Report: %s\n", operation_name);
    printf("  Wall time: %.3f ms\n", cpu_time * 1000);
    printf("  Operations: %lu\n", operations);
    printf("  Throughput: %.2f GOPS\n", operations / cpu_time / 1e9);
    
    free(timer);
}
```

### Pattern 4: Batch Processing

Process data in batches for better efficiency:

```c
npu_result_t process_data_batches(npu_handle_t npu, 
                                 float *input_data, 
                                 float *output_data, 
                                 size_t total_elements) {
    const size_t BATCH_SIZE = 1024;
    npu_result_t result = NPU_SUCCESS;
    
    // Process data in batches
    for (size_t offset = 0; offset < total_elements; offset += BATCH_SIZE) {
        size_t current_batch_size = min(BATCH_SIZE, total_elements - offset);
        
        result = npu_relu(npu, 
                         &input_data[offset], 
                         &output_data[offset], 
                         current_batch_size);
        
        if (result != NPU_SUCCESS) {
            printf("Batch processing failed at offset %zu: %s\n", 
                   offset, npu_get_error_string(result));
            break;
        }
        
        // Optional: Print progress
        if (offset % (BATCH_SIZE * 10) == 0) {
            printf("Processed %zu/%zu elements\n", offset, total_elements);
        }
    }
    
    return result;
}
```

## Advanced Examples

### Example 1: Matrix Multiplication with Timing

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "fpga_npu_lib.h"

void initialize_random_matrix(float *matrix, size_t rows, size_t cols) {
    srand(time(NULL));
    for (size_t i = 0; i < rows * cols; i++) {
        matrix[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f; // Range [-1, 1]
    }
}

void print_matrix_sample(const float *matrix, size_t rows, size_t cols, const char *name) {
    printf("%s sample (first 3x3):\n", name);
    for (size_t i = 0; i < min(3, rows); i++) {
        for (size_t j = 0; j < min(3, cols); j++) {
            printf("%8.3f ", matrix[i * cols + j]);
        }
        printf("\n");
    }
    printf("\n");
}

int matrix_multiply_example() {
    npu_handle_t npu;
    npu_result_t result;
    
    // Matrix dimensions
    const size_t M = 256, N = 256, K = 256;
    
    printf("=== Matrix Multiplication Example ===\n");
    printf("Computing C = A × B where:\n");
    printf("  A: %zux%zu matrix\n", M, K);
    printf("  B: %zux%zu matrix\n", K, N);
    printf("  C: %zux%zu matrix\n", M, N);
    
    // Initialize NPU
    result = npu_init(&npu);
    if (result != NPU_SUCCESS) {
        printf("NPU initialization failed: %s\n", npu_get_error_string(result));
        return -1;
    }
    
    // Allocate matrices
    float *matrix_a = npu_malloc(npu, M * K * sizeof(float));
    float *matrix_b = npu_malloc(npu, K * N * sizeof(float));
    float *matrix_c = npu_malloc(npu, M * N * sizeof(float));
    
    if (!matrix_a || !matrix_b || !matrix_c) {
        printf("Memory allocation failed\n");
        npu_cleanup(npu);
        return -1;
    }
    
    // Initialize matrices
    printf("Initializing matrices with random data...\n");
    initialize_random_matrix(matrix_a, M, K);
    initialize_random_matrix(matrix_b, K, N);
    
    print_matrix_sample(matrix_a, M, K, "Matrix A");
    print_matrix_sample(matrix_b, K, N, "Matrix B");
    
    // Perform matrix multiplication
    printf("Performing matrix multiplication...\n");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    result = npu_matrix_multiply(npu, matrix_a, matrix_b, matrix_c, M, K, N);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    if (result == NPU_SUCCESS) {
        double duration = (end.tv_sec - start.tv_sec) + 
                         (end.tv_nsec - start.tv_nsec) / 1e9;
        
        // Calculate performance metrics
        double operations = 2.0 * M * N * K; // Each multiply-add counts as 2 ops
        double gflops = operations / duration / 1e9;
        
        printf("✅ Matrix multiplication completed successfully!\n");
        printf("   Duration: %.3f ms\n", duration * 1000);
        printf("   Performance: %.2f GFLOPS\n", gflops);
        
        print_matrix_sample(matrix_c, M, N, "Result Matrix C");
        
        // Get performance counters
        npu_performance_counters_t counters;
        npu_get_performance_counters(npu, &counters);
        printf("   NPU Utilization: %.1f%%\n", counters.utilization_percent);
        printf("   Cache Hit Rate: %.1f%%\n", 
               100.0 * counters.cache_hits / (counters.cache_hits + counters.cache_misses));
    } else {
        printf("❌ Matrix multiplication failed: %s\n", npu_get_error_string(result));
    }
    
    // Cleanup
    npu_free(npu, matrix_a);
    npu_free(npu, matrix_b);
    npu_free(npu, matrix_c);
    npu_cleanup(npu);
    
    return (result == NPU_SUCCESS) ? 0 : -1;
}
```

### Example 2: Simple Neural Network Layer

```c
#include <math.h>
#include "fpga_npu_lib.h"

typedef struct {
    size_t input_size;
    size_t output_size;
    float *weights;    // output_size × input_size
    float *biases;     // output_size
    float *output;     // output_size
} neural_layer_t;

neural_layer_t* create_layer(npu_handle_t npu, size_t input_size, size_t output_size) {
    neural_layer_t *layer = malloc(sizeof(neural_layer_t));
    
    layer->input_size = input_size;
    layer->output_size = output_size;
    
    // Allocate NPU memory
    layer->weights = npu_malloc(npu, output_size * input_size * sizeof(float));
    layer->biases = npu_malloc(npu, output_size * sizeof(float));
    layer->output = npu_malloc(npu, output_size * sizeof(float));
    
    if (!layer->weights || !layer->biases || !layer->output) {
        printf("Failed to allocate layer memory\n");
        free(layer);
        return NULL;
    }
    
    // Initialize weights with Xavier initialization
    float weight_scale = sqrtf(2.0f / input_size);
    for (size_t i = 0; i < output_size * input_size; i++) {
        layer->weights[i] = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * weight_scale;
    }
    
    // Initialize biases to zero
    memset(layer->biases, 0, output_size * sizeof(float));
    
    return layer;
}

npu_result_t forward_pass(npu_handle_t npu, neural_layer_t *layer, const float *input) {
    npu_result_t result;
    
    // Fully connected operation: output = weights × input + biases
    result = npu_fully_connected(npu, input, layer->weights, layer->biases, 
                                layer->output, layer->input_size, layer->output_size);
    
    if (result != NPU_SUCCESS) {
        return result;
    }
    
    // Apply ReLU activation
    result = npu_relu(npu, layer->output, layer->output, layer->output_size);
    
    return result;
}

void destroy_layer(npu_handle_t npu, neural_layer_t *layer) {
    if (layer) {
        npu_free(npu, layer->weights);
        npu_free(npu, layer->biases);
        npu_free(npu, layer->output);
        free(layer);
    }
}

int neural_network_example() {
    npu_handle_t npu;
    npu_result_t result;
    
    printf("=== Simple Neural Network Example ===\n");
    
    // Initialize NPU
    result = npu_init(&npu);
    if (result != NPU_SUCCESS) {
        printf("NPU initialization failed\n");
        return -1;
    }
    
    // Create a simple 2-layer network: 784 → 128 → 10
    neural_layer_t *layer1 = create_layer(npu, 784, 128);
    neural_layer_t *layer2 = create_layer(npu, 128, 10);
    
    if (!layer1 || !layer2) {
        printf("Failed to create neural network layers\n");
        npu_cleanup(npu);
        return -1;
    }
    
    // Create input data (simulating 28x28 image)
    float *input = npu_malloc(npu, 784 * sizeof(float));
    if (!input) {
        printf("Failed to allocate input buffer\n");
        destroy_layer(npu, layer1);
        destroy_layer(npu, layer2);
        npu_cleanup(npu);
        return -1;
    }
    
    // Initialize input with random data
    for (size_t i = 0; i < 784; i++) {
        input[i] = (float)rand() / RAND_MAX;
    }
    
    printf("Network architecture: 784 → 128 → 10\n");
    printf("Processing input...\n");
    
    // Forward pass through layer 1
    result = forward_pass(npu, layer1, input);
    if (result != NPU_SUCCESS) {
        printf("Layer 1 forward pass failed: %s\n", npu_get_error_string(result));
        goto cleanup;
    }
    
    // Forward pass through layer 2
    result = forward_pass(npu, layer2, layer1->output);
    if (result != NPU_SUCCESS) {
        printf("Layer 2 forward pass failed: %s\n", npu_get_error_string(result));
        goto cleanup;
    }
    
    // Print output
    printf("✅ Neural network inference completed!\n");
    printf("Output probabilities:\n");
    for (size_t i = 0; i < 10; i++) {
        printf("  Class %zu: %.4f\n", i, layer2->output[i]);
    }
    
    // Find predicted class
    size_t predicted_class = 0;
    float max_prob = layer2->output[0];
    for (size_t i = 1; i < 10; i++) {
        if (layer2->output[i] > max_prob) {
            max_prob = layer2->output[i];
            predicted_class = i;
        }
    }
    
    printf("Predicted class: %zu (confidence: %.4f)\n", predicted_class, max_prob);
    
cleanup:
    npu_free(npu, input);
    destroy_layer(npu, layer1);
    destroy_layer(npu, layer2);
    npu_cleanup(npu);
    
    return (result == NPU_SUCCESS) ? 0 : -1;
}
```

## Troubleshooting

### Common Issues and Solutions

#### Issue 1: NPU Initialization Fails

**Error**: `npu_init()` returns `NPU_ERROR_DEVICE_NOT_FOUND`

**Solutions**:
```bash
# Check if driver is loaded
lsmod | grep fpga_npu

# Load driver if missing
sudo modprobe fpga_npu

# Check device permissions
ls -l /dev/fpga_npu*
sudo chmod 666 /dev/fpga_npu0

# Verify hardware detection
lspci | grep -i npu
dmesg | grep -i npu
```

#### Issue 2: Memory Allocation Failures

**Error**: `npu_malloc()` returns `NULL`

**Solutions**:
```c
// Check available memory
npu_memory_info_t mem_info;
npu_get_memory_info(npu, &mem_info);
printf("Available memory: %zu MB\n", mem_info.free_memory / (1024*1024));

// Use smaller allocations
const size_t MAX_ALLOC_SIZE = 64 * 1024 * 1024; // 64MB
if (requested_size > MAX_ALLOC_SIZE) {
    // Split into smaller chunks
    process_in_chunks(npu, data, requested_size, MAX_ALLOC_SIZE);
}
```

#### Issue 3: Performance Issues

**Symptoms**: Low throughput, high latency

**Debugging**:
```c
// Monitor performance counters
npu_performance_counters_t counters;
npu_get_performance_counters(npu, &counters);

if (counters.utilization_percent < 50) {
    printf("Low utilization - check for bottlenecks\n");
}

double cache_hit_rate = (double)counters.cache_hits / 
                       (counters.cache_hits + counters.cache_misses);
if (cache_hit_rate < 0.8) {
    printf("Poor cache performance - optimize memory access patterns\n");
}
```

#### Issue 4: Incorrect Results

**Debugging Steps**:
1. **Verify input data**: Check that input data is correctly initialized
2. **Check data types**: Ensure float precision is sufficient
3. **Validate intermediate results**: Use small test cases with known outputs
4. **Compare with CPU implementation**: Verify algorithm correctness

```c
// CPU reference implementation for verification
void cpu_matrix_multiply(const float *a, const float *b, float *c, 
                        size_t m, size_t k, size_t n) {
    for (size_t i = 0; i < m; i++) {
        for (size_t j = 0; j < n; j++) {
            float sum = 0.0f;
            for (size_t l = 0; l < k; l++) {
                sum += a[i * k + l] * b[l * n + j];
            }
            c[i * n + j] = sum;
        }
    }
}

bool compare_results(const float *npu_result, const float *cpu_result, 
                    size_t size, float tolerance) {
    for (size_t i = 0; i < size; i++) {
        if (fabsf(npu_result[i] - cpu_result[i]) > tolerance) {
            printf("Mismatch at index %zu: NPU=%.6f, CPU=%.6f\n", 
                   i, npu_result[i], cpu_result[i]);
            return false;
        }
    }
    return true;
}
```

### Debug Build Configuration

For development and debugging, compile with debug flags:

```bash
# Compile with debug information
gcc -g -O0 -DDEBUG -o debug_program program.c -lfpga_npu -lm

# Enable NPU library debug output
export NPU_DEBUG_LEVEL=2

# Run with debug output
./debug_program
```

### Getting Help

If you encounter issues not covered in this tutorial:

1. **Check the documentation**: API Reference, User Guide, and Developer Guide
2. **Search existing issues**: GitHub Issues page
3. **Create a minimal reproduction case**: Isolate the problem
4. **Provide detailed information**: Hardware, software versions, error messages
5. **Join the community**: Discussions forum for questions and tips

## Next Steps

After completing this tutorial, you should:

1. **Explore the examples**: Study the provided example applications
2. **Read the API Reference**: Learn about all available functions
3. **Try optimization techniques**: Follow the Performance Tuning Guide
4. **Build your own applications**: Start with simple algorithms and expand
5. **Contribute back**: Share improvements and report issues

Remember that NPU programming is about parallel thinking and memory efficiency. Start simple, measure performance, and optimize iteratively.

Happy NPU programming! 🚀