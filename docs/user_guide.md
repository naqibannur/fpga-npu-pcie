# FPGA NPU User Guide

A comprehensive guide for using the FPGA NPU system, from installation to advanced usage patterns.

## Table of Contents

- [Introduction](#introduction)
- [System Requirements](#system-requirements)
- [Installation](#installation)
- [Getting Started](#getting-started)
- [Programming Guide](#programming-guide)
- [Performance Optimization](#performance-optimization)
- [Troubleshooting](#troubleshooting)
- [Advanced Topics](#advanced-topics)

## Introduction

The FPGA NPU (Neural Processing Unit) is a specialized hardware accelerator designed for high-performance neural network computations. This guide covers everything you need to know to effectively use the NPU system.

### Key Features

- **High Performance**: Optimized for neural network workloads
- **Flexible Architecture**: Supports various network topologies
- **Memory Efficient**: Advanced memory management and caching
- **Power Efficient**: Optimized for performance-per-watt
- **Easy Integration**: Simple C API for application development

### Supported Operations

- Matrix operations (multiplication, transpose, etc.)
- Convolution (1D, 2D)
- Pooling (max, average)
- Activation functions (ReLU, sigmoid, tanh, softmax)
- Normalization (batch norm, layer norm)
- Fully connected layers

## System Requirements

### Hardware Requirements

- **FPGA Board**: Compatible Xilinx or Intel FPGA development board
- **Memory**: Minimum 8GB system RAM (16GB recommended)
- **PCIe**: PCIe 3.0 x8 slot or better
- **Power**: Adequate PSU for FPGA board (typically 75-150W)
- **Cooling**: Proper ventilation or active cooling

### Supported FPGA Boards

- Xilinx Kintex UltraScale+ KCU116
- Xilinx Virtex UltraScale+ VCU118
- Intel Stratix 10 Development Kit
- Intel Arria 10 SoC Development Kit

### Software Requirements

- **Operating System**: Linux (Ubuntu 18.04+, CentOS 7+, RHEL 8+)
- **Compiler**: GCC 7.0+ or Clang 8.0+
- **Build Tools**: Make, CMake 3.10+
- **Python**: Python 3.6+ (for scripts and tools)

### Development Tools (Optional)

- **Vivado**: 2020.1+ (for Xilinx FPGAs)
- **Quartus Prime**: 20.1+ (for Intel FPGAs)
- **ModelSim/Questa**: For simulation
- **Git**: For version control

## Installation

### Quick Installation

```bash
# Download and extract the NPU package
wget https://releases.example.com/fpga-npu-latest.tar.gz
tar -xzf fpga-npu-latest.tar.gz
cd fpga-npu

# Run automated installation
sudo ./install.sh
```

### Manual Installation

#### Step 1: Install Dependencies

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake git dkms linux-headers-$(uname -r)

# CentOS/RHEL
sudo yum install gcc gcc-c++ make cmake git dkms kernel-devel
```

#### Step 2: Build and Install Driver

```bash
cd software/driver
make clean
make
sudo make install

# Load the driver
sudo modprobe fpga_npu
```

#### Step 3: Build User Library

```bash
cd software/userspace
make clean
make
sudo make install
```

#### Step 4: Verify Installation

```bash
# Check if driver is loaded
lsmod | grep fpga_npu

# Check if devices are detected
ls /dev/fpga_npu*

# Test basic functionality
cd examples/matrix_multiply
make
./bin/matrix_multiply_example --size 64
```

### Installation Verification

Run the verification script to ensure everything is working:

```bash
./scripts/verify_installation.sh
```

Expected output:
```
✓ Driver loaded successfully
✓ NPU device detected
✓ User library accessible
✓ Basic operations working
✓ Installation verified
```

## Getting Started

### Your First NPU Program

Create a simple program that performs matrix multiplication:

```c
// hello_npu.c
#include <stdio.h>
#include "fpga_npu_lib.h"

int main() {
    npu_handle_t npu;
    
    // Initialize NPU
    if (npu_init(&npu) != NPU_SUCCESS) {
        printf("Failed to initialize NPU\n");
        return -1;
    }
    
    printf("NPU initialized successfully!\n");
    
    // Get device information
    npu_device_info_t info;
    if (npu_get_device_info(npu, &info) == NPU_SUCCESS) {
        printf("Device: %s\n", info.device_name);
        printf("Memory: %zu MB\n", info.memory_size / (1024 * 1024));
    }
    
    // Cleanup
    npu_cleanup(npu);
    return 0;
}
```

Compile and run:

```bash
gcc -o hello_npu hello_npu.c -lfpga_npu
./hello_npu
```

### Basic Matrix Multiplication

```c
#include <stdio.h>
#include <stdlib.h>
#include "fpga_npu_lib.h"

int main() {
    npu_handle_t npu;
    npu_init(&npu);
    
    // Allocate 4x4 matrices
    float *a = npu_malloc(npu, 16 * sizeof(float));
    float *b = npu_malloc(npu, 16 * sizeof(float));
    float *c = npu_malloc(npu, 16 * sizeof(float));
    
    // Initialize matrices
    for (int i = 0; i < 16; i++) {
        a[i] = i + 1.0f;
        b[i] = (i % 4) + 1.0f;
    }
    
    // Perform multiplication: C = A × B
    npu_matrix_multiply(npu, a, b, c, 4, 4, 4);
    
    // Print result
    printf("Result matrix C:\n");
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            printf("%8.2f ", c[i * 4 + j]);
        }
        printf("\n");
    }
    
    // Cleanup
    npu_free(npu, a);
    npu_free(npu, b);
    npu_free(npu, c);
    npu_cleanup(npu);
    return 0;
}
```

### Error Handling Example

```c
#include "fpga_npu_lib.h"

npu_result_t safe_matrix_multiply(npu_handle_t npu, 
                                 const float *a, const float *b, float *c,
                                 size_t rows_a, size_t cols_a, size_t cols_b) {
    // Validate parameters
    if (!a || !b || !c) {
        printf("Error: NULL pointer passed to matrix multiply\n");
        return NPU_ERROR_INVALID_PARAMETER;
    }
    
    if (rows_a == 0 || cols_a == 0 || cols_b == 0) {
        printf("Error: Invalid matrix dimensions\n");
        return NPU_ERROR_INVALID_PARAMETER;
    }
    
    // Perform operation
    npu_result_t result = npu_matrix_multiply(npu, a, b, c, rows_a, cols_a, cols_b);
    if (result != NPU_SUCCESS) {
        printf("Matrix multiplication failed: %s\n", npu_get_error_string(result));
        return result;
    }
    
    return NPU_SUCCESS;
}
```

## Programming Guide

### Memory Management

#### NPU Memory Allocation

NPU memory is separate from system memory and must be explicitly managed:

```c
// Allocate NPU memory
float *tensor = npu_malloc(npu, size * sizeof(float));
if (!tensor) {
    printf("NPU memory allocation failed\n");
    return -1;
}

// Use tensor...

// Free NPU memory
npu_free(npu, tensor);
```

#### Memory Alignment

For optimal performance, use aligned memory allocation:

```c
// Allocate 64-byte aligned memory
float *aligned_tensor = npu_malloc_aligned(npu, size * sizeof(float), 64);
```

#### Memory Usage Monitoring

```c
npu_memory_info_t mem_info;
npu_get_memory_info(npu, &mem_info);

printf("Total memory: %zu MB\n", mem_info.total_memory / (1024 * 1024));
printf("Used memory: %zu MB\n", mem_info.used_memory / (1024 * 1024));
printf("Free memory: %zu MB\n", mem_info.free_memory / (1024 * 1024));
```

### Tensor Operations

#### Element-wise Operations

```c
// Addition: C = A + B
npu_tensor_add(npu, tensor_a, tensor_b, result, num_elements);

// Multiplication: C = A ⊙ B (Hadamard product)
npu_tensor_multiply(npu, tensor_a, tensor_b, result, num_elements);

// Scaling: B = α × A
npu_tensor_scale(npu, tensor_a, scale_factor, result, num_elements);
```

#### Matrix Operations

```c
// Matrix multiplication: C = A × B
npu_matrix_multiply(npu, matrix_a, matrix_b, result, rows_a, cols_a, cols_b);

// Matrix transpose: B = Aᵀ
npu_transpose(npu, matrix_a, result, rows, cols);

// Batch matrix multiplication
npu_batch_matrix_multiply(npu, batch_a, batch_b, batch_c, 
                         batch_size, rows_a, cols_a, cols_b);
```

### Neural Network Operations

#### Convolution

```c
// 2D Convolution
npu_conv2d(npu, 
          input,                    // Input tensor
          kernel,                   // Convolution kernel
          output,                   // Output tensor
          input_height, input_width, input_channels,
          output_channels,
          kernel_height, kernel_width,
          stride_y, stride_x,       // Stride
          padding_y, padding_x);    // Padding
```

#### Pooling

```c
// Max pooling
npu_maxpool2d(npu, input, output,
             input_height, input_width, channels,
             pool_height, pool_width,
             stride_y, stride_x);

// Average pooling
npu_avgpool2d(npu, input, output,
             input_height, input_width, channels,
             pool_height, pool_width,
             stride_y, stride_x);
```

#### Activation Functions

```c
// ReLU: f(x) = max(0, x)
npu_relu(npu, input, output, num_elements);

// Sigmoid: f(x) = 1/(1 + e^(-x))
npu_sigmoid(npu, input, output, num_elements);

// Softmax (for classification)
npu_softmax(npu, input, output, num_elements);
```

#### Fully Connected Layers

```c
// Dense layer: Y = XW + b
npu_fully_connected(npu, 
                   input,           // Input vector
                   weights,         // Weight matrix
                   biases,          // Bias vector
                   output,          // Output vector
                   input_size, output_size);
```

### Building Neural Networks

#### Simple Multi-Layer Perceptron

```c
typedef struct {
    float *weights1, *biases1;  // Hidden layer
    float *weights2, *biases2;  // Output layer
    float *hidden_output;       // Intermediate buffer
} mlp_network_t;

npu_result_t mlp_forward(npu_handle_t npu, mlp_network_t *net,
                        const float *input, float *output,
                        size_t input_size, size_t hidden_size, size_t output_size) {
    // Hidden layer
    npu_fully_connected(npu, input, net->weights1, net->biases1, 
                       net->hidden_output, input_size, hidden_size);
    npu_relu(npu, net->hidden_output, net->hidden_output, hidden_size);
    
    // Output layer
    npu_fully_connected(npu, net->hidden_output, net->weights2, net->biases2,
                       output, hidden_size, output_size);
    
    return NPU_SUCCESS;
}
```

#### Convolutional Neural Network

```c
typedef struct {
    float *conv1_weights, *conv1_biases;
    float *conv2_weights, *conv2_biases;
    float *fc_weights, *fc_biases;
    float *conv1_output, *pool1_output;
    float *conv2_output, *pool2_output;
} cnn_network_t;

npu_result_t cnn_forward(npu_handle_t npu, cnn_network_t *net,
                        const float *input, float *output) {
    // First convolution + pooling
    npu_conv2d(npu, input, net->conv1_weights, net->conv1_output,
              28, 28, 1, 32, 5, 5, 1, 1, 0, 0);
    npu_relu(npu, net->conv1_output, net->conv1_output, 24*24*32);
    npu_maxpool2d(npu, net->conv1_output, net->pool1_output,
                 24, 24, 32, 2, 2, 2, 2);
    
    // Second convolution + pooling
    npu_conv2d(npu, net->pool1_output, net->conv2_weights, net->conv2_output,
              12, 12, 32, 64, 5, 5, 1, 1, 0, 0);
    npu_relu(npu, net->conv2_output, net->conv2_output, 8*8*64);
    npu_maxpool2d(npu, net->conv2_output, net->pool2_output,
                 8, 8, 64, 2, 2, 2, 2);
    
    // Fully connected layer
    npu_fully_connected(npu, net->pool2_output, net->fc_weights, net->fc_biases,
                       output, 4*4*64, 10);
    npu_softmax(npu, output, output, 10);
    
    return NPU_SUCCESS;
}
```

## Performance Optimization

### Memory Optimization

#### 1. Memory Layout

Organize data for optimal memory access patterns:

```c
// Good: Contiguous memory layout
float *data = npu_malloc(npu, batch_size * channels * height * width * sizeof(float));

// Bad: Scattered allocations
float **batch = malloc(batch_size * sizeof(float*));
for (int i = 0; i < batch_size; i++) {
    batch[i] = npu_malloc(npu, channels * height * width * sizeof(float));
}
```

#### 2. Memory Reuse

Reuse memory buffers when possible:

```c
// Allocate once, reuse for multiple operations
float *temp_buffer = npu_malloc(npu, max_intermediate_size);

// Use for different operations
npu_conv2d(npu, input, kernel1, temp_buffer, ...);
npu_relu(npu, temp_buffer, temp_buffer, size1);

npu_conv2d(npu, temp_buffer, kernel2, temp_buffer, ...);
npu_relu(npu, temp_buffer, temp_buffer, size2);
```

#### 3. Memory Alignment

Use proper alignment for better performance:

```c
// Align to cache line boundaries (64 bytes)
float *aligned_data = npu_malloc_aligned(npu, size, 64);
```

### Computation Optimization

#### 1. Batch Operations

Use batch operations for better throughput:

```c
// Efficient: Batch matrix multiplication
npu_batch_matrix_multiply(npu, batch_a, batch_b, batch_c, 
                         batch_size, m, n, k);

// Less efficient: Individual operations
for (int i = 0; i < batch_size; i++) {
    npu_matrix_multiply(npu, &batch_a[i*m*n], &batch_b[i*n*k], &batch_c[i*m*k],
                       m, n, k);
}
```

#### 2. Operation Fusion

Combine operations when possible:

```c
// Fused convolution + bias + ReLU
npu_conv2d_fused(npu, input, kernel, bias, output, ..., NPU_ACTIVATION_RELU);

// Instead of separate operations:
npu_conv2d(npu, input, kernel, temp, ...);
npu_add_bias(npu, temp, bias, temp, size);
npu_relu(npu, temp, output, size);
```

#### 3. Data Types

Use appropriate precision:

```c
// Use float16 for memory-intensive operations (if supported)
half *input_fp16 = npu_malloc(npu, size * sizeof(half));
npu_conv2d_fp16(npu, input_fp16, kernel_fp16, output_fp16, ...);
```

### Performance Monitoring

#### 1. Basic Timing

```c
#include <time.h>

struct timespec start, end;
clock_gettime(CLOCK_MONOTONIC, &start);

// NPU operations
npu_matrix_multiply(npu, a, b, c, m, n, k);

clock_gettime(CLOCK_MONOTONIC, &end);
double duration = (end.tv_sec - start.tv_sec) + 
                 (end.tv_nsec - start.tv_nsec) / 1e9;
printf("Operation took %.3f ms\n", duration * 1000);
```

#### 2. Performance Counters

```c
npu_performance_counters_t counters;
npu_reset_performance_counters(npu);

// Perform operations
npu_matrix_multiply(npu, a, b, c, m, n, k);

npu_get_performance_counters(npu, &counters);
printf("Operations: %llu\n", counters.total_operations);
printf("Utilization: %.1f%%\n", counters.utilization_percent);
```

#### 3. Power Monitoring

```c
npu_power_info_t power;
npu_get_power_info(npu, &power);

printf("Power consumption: %.2f W\n", power.power_w);
printf("Temperature: %.1f °C\n", power.temperature_c);
if (power.thermal_throttling) {
    printf("Warning: Thermal throttling active\n");
}
```

### Optimization Guidelines

1. **Memory Access**: Minimize data transfers between host and NPU
2. **Batch Size**: Use larger batches for better utilization
3. **Data Layout**: Use NCHW format for optimal performance
4. **Operation Order**: Group similar operations together
5. **Memory Reuse**: Minimize allocations in hot paths
6. **Precision**: Use appropriate numerical precision

## Troubleshooting

### Common Issues

#### 1. Device Not Found

**Problem**: `NPU_ERROR_DEVICE_NOT_FOUND` during initialization

**Solutions**:
```bash
# Check if driver is loaded
lsmod | grep fpga_npu

# Check device nodes
ls -l /dev/fpga_npu*

# Check PCI devices
lspci | grep -i fpga

# Reload driver
sudo rmmod fpga_npu
sudo modprobe fpga_npu
```

#### 2. Memory Allocation Failures

**Problem**: `npu_malloc` returns NULL

**Solutions**:
```c
// Check available memory
npu_memory_info_t info;
npu_get_memory_info(npu, &info);
printf("Free memory: %zu bytes\n", info.free_memory);

// Reduce allocation size or free unused memory
npu_free(npu, unused_buffer);
```

#### 3. Performance Issues

**Problem**: Lower than expected performance

**Diagnosis**:
```c
// Check for thermal throttling
npu_power_info_t power;
npu_get_power_info(npu, &power);
if (power.thermal_throttling) {
    printf("Performance reduced due to thermal throttling\n");
}

// Check utilization
npu_performance_counters_t counters;
npu_get_performance_counters(npu, &counters);
printf("NPU utilization: %.1f%%\n", counters.utilization_percent);
```

#### 4. Build Issues

**Problem**: Compilation errors

**Solutions**:
```bash
# Check compiler version
gcc --version

# Verify library installation
pkg-config --libs fpga_npu

# Check include paths
find /usr -name "fpga_npu_lib.h" 2>/dev/null
```

### Debugging Tools

#### 1. Enable Debug Mode

```c
// Compile with debug flags
// gcc -DDEBUG -g -o program program.c -lfpga_npu

#ifdef DEBUG
    printf("Debug: Allocating %zu bytes\n", size);
#endif
```

#### 2. Memory Debugging

```bash
# Use valgrind for memory leak detection
valgrind --leak-check=full ./your_program

# Use AddressSanitizer
gcc -fsanitize=address -g -o program program.c -lfpga_npu
```

#### 3. NPU State Inspection

```c
// Check device status
uint32_t status;
npu_get_status(npu, &status);
printf("Device status: 0x%08x\n", status);

if (status & 0x04) {
    printf("Error condition detected\n");
}
```

### Performance Debugging

#### 1. Profile Your Application

```c
npu_start_profiling(npu);

// Your operations here
npu_matrix_multiply(npu, a, b, c, m, n, k);

char profile_data[4096];
npu_stop_profiling(npu, profile_data, sizeof(profile_data));
printf("Profile data: %s\n", profile_data);
```

#### 2. Identify Bottlenecks

```c
// Time individual operations
double times[NUM_OPERATIONS];
for (int i = 0; i < NUM_OPERATIONS; i++) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    perform_operation(npu, i);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    times[i] = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

// Find slowest operation
int slowest = 0;
for (int i = 1; i < NUM_OPERATIONS; i++) {
    if (times[i] > times[slowest]) slowest = i;
}
printf("Slowest operation: %d (%.3f ms)\n", slowest, times[slowest] * 1000);
```

## Advanced Topics

### Multi-Device Usage

```c
// Enumerate available devices
int num_devices = npu_get_device_count();
printf("Found %d NPU devices\n", num_devices);

// Use multiple devices
npu_handle_t npus[num_devices];
for (int i = 0; i < num_devices; i++) {
    npu_init_device(&npus[i], i);
}

// Distribute work across devices
// ... parallel processing logic ...

// Cleanup all devices
for (int i = 0; i < num_devices; i++) {
    npu_cleanup(npus[i]);
}
```

### Custom Operations

```c
// Implement custom operation using basic primitives
npu_result_t custom_layer_norm(npu_handle_t npu,
                              const float *input,
                              float *output,
                              size_t batch_size,
                              size_t feature_size) {
    // Custom implementation using NPU primitives
    // 1. Calculate mean
    // 2. Calculate variance
    // 3. Normalize
    // 4. Apply scale and bias
    
    return NPU_SUCCESS;
}
```

### Integration with Frameworks

#### PyTorch Integration Example

```python
import torch
import ctypes

# Load NPU library
npu_lib = ctypes.CDLL('./libnpu_pytorch.so')

class NPUMatMul(torch.autograd.Function):
    @staticmethod
    def forward(ctx, input_a, input_b):
        # Call NPU implementation
        result = npu_lib.npu_matmul(input_a.data_ptr(), input_b.data_ptr(), ...)
        return result
    
    @staticmethod
    def backward(ctx, grad_output):
        # Implement backward pass
        pass

# Use in PyTorch model
npu_matmul = NPUMatMul.apply
```

This user guide provides comprehensive coverage of the FPGA NPU system usage. For specific API details, refer to the [API Reference](api_reference.md), and for development information, see the [Developer Guide](developer_guide.md).