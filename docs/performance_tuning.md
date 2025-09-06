# FPGA NPU Performance Tuning Guide

This guide provides comprehensive techniques for optimizing performance when using the FPGA NPU, covering algorithm optimization, memory management, hardware configuration, and profiling strategies.

## Table of Contents

- [Performance Overview](#performance-overview)
- [Algorithm Optimization](#algorithm-optimization)
- [Memory Optimization](#memory-optimization)
- [Hardware Configuration](#hardware-configuration)
- [Profiling and Analysis](#profiling-and-analysis)
- [Common Performance Issues](#common-performance-issues)
- [Best Practices](#best-practices)

## Performance Overview

### Key Performance Metrics

| Metric | Description | Target Range |
|--------|-------------|--------------|
| **Throughput** | Operations per second | 500-1000+ GOPS |
| **Latency** | Time per operation | <10ms typical |
| **Utilization** | PE usage efficiency | >80% |
| **Memory Bandwidth** | Data transfer rate | >70% of peak |
| **Power Efficiency** | GOPS per Watt | 10-15 GOPS/W |

### Performance Bottlenecks

Common bottlenecks and their indicators:

1. **Memory Bandwidth**: High cache miss rates, low PE utilization
2. **Computation Bound**: High PE utilization, low memory bandwidth
3. **Synchronization**: Uneven PE utilization, frequent barriers
4. **Host Transfer**: High PCIe utilization, NPU idle time

## Algorithm Optimization

### Matrix Multiplication Optimization

#### Blocked Algorithm Implementation
```c
// Optimized blocked matrix multiplication
void optimized_matrix_multiply(npu_handle_t npu,
                              const float *A, const float *B, float *C,
                              size_t M, size_t N, size_t K) {
    const size_t BLOCK_SIZE = 64; // Optimized for L1 cache
    
    for (size_t i = 0; i < M; i += BLOCK_SIZE) {
        for (size_t j = 0; j < N; j += BLOCK_SIZE) {
            for (size_t k = 0; k < K; k += BLOCK_SIZE) {
                size_t max_i = min(i + BLOCK_SIZE, M);
                size_t max_j = min(j + BLOCK_SIZE, N);
                size_t max_k = min(k + BLOCK_SIZE, K);
                
                // Process block on NPU
                npu_matrix_multiply_block(npu,
                    &A[i * K + k], &B[k * N + j], &C[i * N + j],
                    max_i - i, max_j - j, max_k - k,
                    K, N, N); // Strides
            }
        }
    }
}
```

#### Performance Optimization Tips
- **Block Size Selection**: Use 32-128 elements for optimal cache usage
- **Data Layout**: Prefer row-major layout for better spatial locality
- **Batch Operations**: Combine multiple small operations into larger batches

### Convolution Optimization

#### Efficient Convolution Implementation
```c
// Optimized 2D convolution with im2col transformation
npu_result_t optimized_conv2d(npu_handle_t npu,
                             const float *input, const float *kernel, float *output,
                             conv_params_t *params) {
    // Calculate optimal tile size based on memory constraints
    size_t tile_size = calculate_optimal_tile_size(params);
    
    // Process input in tiles to maximize cache efficiency
    for (size_t tile_y = 0; tile_y < params->output_height; tile_y += tile_size) {
        for (size_t tile_x = 0; tile_x < params->output_width; tile_x += tile_size) {
            // Extract tile with overlap for kernel computation
            extract_input_tile(input, tile_buffer, tile_y, tile_x, params);
            
            // Perform convolution on tile
            npu_conv2d_tile(npu, tile_buffer, kernel, output_tile, params);
            
            // Copy result back to output
            copy_output_tile(output_tile, output, tile_y, tile_x, params);
        }
    }
    
    return NPU_SUCCESS;
}

// Calculate optimal tile size based on cache capacity
size_t calculate_optimal_tile_size(conv_params_t *params) {
    size_t l1_cache_size = 16 * 1024; // 16KB per PE
    size_t element_size = sizeof(float);
    
    // Account for input, kernel, and output data
    size_t total_data_per_tile = 
        params->kernel_size * params->kernel_size * params->input_channels +
        params->input_channels + params->output_channels;
    
    return sqrt(l1_cache_size / (total_data_per_tile * element_size));
}
```

#### Convolution Optimization Strategies
- **Kernel Transformation**: Use FFT for large kernels (>7x7)
- **Channel Parallelization**: Distribute channels across PEs
- **Depth-wise Separation**: Optimize separable convolutions separately

### Activation Function Optimization

#### Vectorized Activation Functions
```c
// Optimized ReLU implementation using SIMD
npu_result_t optimized_relu(npu_handle_t npu, const float *input, float *output, size_t size) {
    const size_t VECTOR_SIZE = 16; // Process 16 elements at once
    size_t vector_count = size / VECTOR_SIZE;
    size_t remainder = size % VECTOR_SIZE;
    
    // Process vectorized portion
    for (size_t i = 0; i < vector_count; i++) {
        npu_vector_relu(npu, &input[i * VECTOR_SIZE], &output[i * VECTOR_SIZE], VECTOR_SIZE);
    }
    
    // Process remaining elements
    if (remainder > 0) {
        npu_relu(npu, &input[vector_count * VECTOR_SIZE], 
                 &output[vector_count * VECTOR_SIZE], remainder);
    }
    
    return NPU_SUCCESS;
}

// Fused activation with preceding operation
npu_result_t fused_conv_relu(npu_handle_t npu,
                            const float *input, const float *kernel, float *output,
                            conv_params_t *params) {
    // Perform convolution with immediate ReLU activation
    return npu_conv2d_with_activation(npu, input, kernel, output, params, NPU_ACTIVATION_RELU);
}
```

## Memory Optimization

### Memory Layout Optimization

#### Data Structure Alignment
```c
// Aligned data structure for optimal memory access
typedef struct __attribute__((aligned(64))) {  // Cache line alignment
    float data[16][16];  // Tile size matching vector width
} aligned_tile_t;

// Memory pool for aligned allocations
typedef struct {
    void *base_addr;
    size_t total_size;
    size_t alignment;
    struct free_block *free_list;
} aligned_memory_pool_t;

aligned_memory_pool_t* create_aligned_pool(size_t size, size_t alignment) {
    aligned_memory_pool_t *pool = malloc(sizeof(aligned_memory_pool_t));
    
    pool->base_addr = aligned_alloc(alignment, size);
    pool->total_size = size;
    pool->alignment = alignment;
    pool->free_list = initialize_free_list(pool->base_addr, size);
    
    return pool;
}
```

#### Memory Access Patterns
```c
// Sequential access optimization
void optimize_sequential_access(float *matrix, size_t rows, size_t cols) {
    // Process in row-major order for better spatial locality
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            process_element(&matrix[i * cols + j]);
        }
    }
}

// Blocked access for large matrices
void optimize_blocked_access(float *matrix, size_t rows, size_t cols) {
    const size_t BLOCK_SIZE = 64; // Optimal block size
    
    for (size_t bi = 0; bi < rows; bi += BLOCK_SIZE) {
        for (size_t bj = 0; bj < cols; bj += BLOCK_SIZE) {
            // Process block
            for (size_t i = bi; i < min(bi + BLOCK_SIZE, rows); i++) {
                for (size_t j = bj; j < min(bj + BLOCK_SIZE, cols); j++) {
                    process_element(&matrix[i * cols + j]);
                }
            }
        }
    }
}
```

### Cache Optimization

#### Cache-Friendly Data Structures
```c
// Structure of Arrays (SoA) for better vectorization
typedef struct {
    float *weights;     // All weights contiguous
    float *biases;      // All biases contiguous
    float *gradients;   // All gradients contiguous
    size_t count;
} soa_layer_t;

// Array of Structures (AoS) - less cache-friendly
typedef struct {
    float weight;
    float bias;
    float gradient;
} aos_neuron_t;

typedef struct {
    aos_neuron_t *neurons;
    size_t count;
} aos_layer_t;

// Conversion functions for optimal access patterns
void convert_aos_to_soa(aos_layer_t *aos, soa_layer_t *soa) {
    for (size_t i = 0; i < aos->count; i++) {
        soa->weights[i] = aos->neurons[i].weight;
        soa->biases[i] = aos->neurons[i].bias;
        soa->gradients[i] = aos->neurons[i].gradient;
    }
}
```

#### Prefetching Strategies
```c
// Software prefetching for large data sets
void prefetch_optimized_operation(npu_handle_t npu, float *data, size_t size) {
    const size_t PREFETCH_DISTANCE = 1024; // Elements to prefetch ahead
    
    for (size_t i = 0; i < size; i++) {
        // Prefetch data ahead of current processing
        if (i + PREFETCH_DISTANCE < size) {
            __builtin_prefetch(&data[i + PREFETCH_DISTANCE], 0, 3); // Temporal locality
        }
        
        // Process current element
        process_data_element(npu, &data[i]);
    }
}
```

### Memory Pool Management

#### Custom Memory Allocator
```c
typedef struct memory_pool {
    void *base_address;
    size_t total_size;
    size_t block_size;
    uint8_t *allocation_bitmap;
    struct mutex pool_mutex;
    size_t allocated_blocks;
    size_t free_blocks;
} memory_pool_t;

memory_pool_t* create_memory_pool(size_t total_size, size_t block_size) {
    memory_pool_t *pool = malloc(sizeof(memory_pool_t));
    
    pool->base_address = npu_malloc_aligned(npu, total_size, 64);
    pool->total_size = total_size;
    pool->block_size = block_size;
    pool->free_blocks = total_size / block_size;
    pool->allocated_blocks = 0;
    
    size_t bitmap_size = (pool->free_blocks + 7) / 8; // Bits to bytes
    pool->allocation_bitmap = calloc(bitmap_size, 1);
    
    mutex_init(&pool->pool_mutex);
    
    return pool;
}

void* pool_alloc(memory_pool_t *pool) {
    mutex_lock(&pool->pool_mutex);
    
    // Find first free block
    for (size_t i = 0; i < pool->free_blocks; i++) {
        if (!is_bit_set(pool->allocation_bitmap, i)) {
            set_bit(pool->allocation_bitmap, i);
            pool->allocated_blocks++;
            pool->free_blocks--;
            
            void *addr = (char*)pool->base_address + i * pool->block_size;
            mutex_unlock(&pool->pool_mutex);
            return addr;
        }
    }
    
    mutex_unlock(&pool->pool_mutex);
    return NULL; // Pool exhausted
}
```

## Hardware Configuration

### Processing Element Configuration

#### Optimal PE Utilization
```c
// Configure PEs for different workload types
typedef struct {
    uint32_t pe_mask;           // Which PEs to use
    uint32_t frequency_mhz;     // Operating frequency
    uint32_t power_limit_mw;    // Power limit per PE
    pe_mode_t mode;             // Operating mode
} pe_config_t;

npu_result_t configure_pes_for_workload(npu_handle_t npu, workload_type_t workload) {
    pe_config_t config;
    
    switch (workload) {
        case WORKLOAD_MATRIX_MULTIPLY:
            config.pe_mask = 0xFFF;         // Use all 12 PEs
            config.frequency_mhz = 400;      // Maximum frequency
            config.power_limit_mw = 6000;    // 72W total
            config.mode = PE_MODE_FP32;
            break;
            
        case WORKLOAD_CONVOLUTION:
            config.pe_mask = 0xFF0;         // Use 8 PEs
            config.frequency_mhz = 350;      // Moderate frequency
            config.power_limit_mw = 5000;    // 40W total
            config.mode = PE_MODE_INT8;
            break;
            
        case WORKLOAD_INFERENCE:
            config.pe_mask = 0x0FF;         // Use 8 PEs
            config.frequency_mhz = 300;      // Lower frequency
            config.power_limit_mw = 4000;    // 32W total
            config.mode = PE_MODE_MIXED;
            break;
    }
    
    return npu_configure_processing_elements(npu, &config);
}
```

#### Dynamic Frequency Scaling
```c
// Adaptive frequency scaling based on workload
typedef struct {
    uint32_t current_frequency;
    uint32_t target_frequency;
    uint32_t utilization_percent;
    uint32_t temperature_celsius;
    bool thermal_throttling;
} frequency_control_t;

void update_frequency_scaling(npu_handle_t npu, frequency_control_t *ctrl) {
    npu_performance_counters_t perf;
    npu_power_info_t power;
    
    npu_get_performance_counters(npu, &perf);
    npu_get_power_info(npu, &power);
    
    ctrl->utilization_percent = (uint32_t)perf.utilization_percent;
    ctrl->temperature_celsius = (uint32_t)power.temperature_c;
    ctrl->thermal_throttling = power.thermal_throttling;
    
    // Determine target frequency
    if (ctrl->thermal_throttling || ctrl->temperature_celsius > 80) {
        // Reduce frequency for thermal management
        ctrl->target_frequency = max(250, ctrl->current_frequency - 50);
    } else if (ctrl->utilization_percent > 90) {
        // Increase frequency for high utilization
        ctrl->target_frequency = min(400, ctrl->current_frequency + 50);
    } else if (ctrl->utilization_percent < 50) {
        // Reduce frequency for power saving
        ctrl->target_frequency = max(250, ctrl->current_frequency - 25);
    }
    
    // Apply frequency change if needed
    if (ctrl->target_frequency != ctrl->current_frequency) {
        npu_set_frequency(npu, ctrl->target_frequency);
        ctrl->current_frequency = ctrl->target_frequency;
    }
}
```

### Memory Controller Tuning

#### Memory Access Optimization
```c
// Configure memory controller for optimal performance
typedef struct {
    uint32_t read_priority;      // Read request priority
    uint32_t write_priority;     // Write request priority
    uint32_t burst_length;       // Burst transfer length
    uint32_t refresh_interval;   // Memory refresh interval
    bool enable_ecc;             // Error correction enable
} memory_config_t;

npu_result_t optimize_memory_controller(npu_handle_t npu) {
    memory_config_t config = {
        .read_priority = 3,      // High priority for reads
        .write_priority = 2,     // Medium priority for writes
        .burst_length = 8,       // 8-beat bursts for efficiency
        .refresh_interval = 7800, // Optimized refresh rate
        .enable_ecc = true       // Enable error correction
    };
    
    return npu_configure_memory_controller(npu, &config);
}
```

## Profiling and Analysis

### Performance Counter Analysis

#### Comprehensive Performance Monitoring
```c
typedef struct {
    uint64_t start_time;
    uint64_t end_time;
    npu_performance_counters_t start_counters;
    npu_performance_counters_t end_counters;
    char operation_name[64];
} performance_measurement_t;

performance_measurement_t* start_performance_measurement(npu_handle_t npu, const char *op_name) {
    performance_measurement_t *measurement = malloc(sizeof(performance_measurement_t));
    
    measurement->start_time = get_timestamp_ns();
    npu_get_performance_counters(npu, &measurement->start_counters);
    strncpy(measurement->operation_name, op_name, sizeof(measurement->operation_name) - 1);
    
    return measurement;
}

void end_performance_measurement(npu_handle_t npu, performance_measurement_t *measurement) {
    measurement->end_time = get_timestamp_ns();
    npu_get_performance_counters(npu, &measurement->end_counters);
    
    // Calculate metrics
    uint64_t duration_ns = measurement->end_time - measurement->start_time;
    uint64_t operations = measurement->end_counters.total_operations - 
                         measurement->start_counters.total_operations;
    uint64_t cycles = measurement->end_counters.total_cycles - 
                     measurement->start_counters.total_cycles;
    
    double throughput_gops = (double)operations / (duration_ns / 1e9) / 1e9;
    double efficiency = (double)operations / cycles;
    
    printf("Operation: %s\n", measurement->operation_name);
    printf("  Duration: %.3f ms\n", duration_ns / 1e6);
    printf("  Throughput: %.2f GOPS\n", throughput_gops);
    printf("  Efficiency: %.2f ops/cycle\n", efficiency);
    printf("  Cache Hit Rate: %.1f%%\n", 
           100.0 * measurement->end_counters.cache_hits / 
           (measurement->end_counters.cache_hits + measurement->end_counters.cache_misses));
}
```

#### Bottleneck Detection
```c
typedef enum {
    BOTTLENECK_NONE,
    BOTTLENECK_MEMORY_BANDWIDTH,
    BOTTLENECK_COMPUTATION,
    BOTTLENECK_SYNCHRONIZATION,
    BOTTLENECK_HOST_TRANSFER
} bottleneck_type_t;

bottleneck_type_t detect_bottleneck(npu_performance_counters_t *counters, 
                                   npu_power_info_t *power) {
    double utilization = counters->utilization_percent;
    double cache_hit_rate = (double)counters->cache_hits / 
                           (counters->cache_hits + counters->cache_misses);
    double memory_bandwidth_util = calculate_memory_bandwidth_utilization(counters);
    
    if (cache_hit_rate < 0.7 && memory_bandwidth_util > 0.8) {
        return BOTTLENECK_MEMORY_BANDWIDTH;
    } else if (utilization > 0.9 && cache_hit_rate > 0.9) {
        return BOTTLENECK_COMPUTATION;
    } else if (utilization < 0.5 && cache_hit_rate > 0.8) {
        return BOTTLENECK_SYNCHRONIZATION;
    } else if (power->power_w < 20 && utilization < 0.3) {
        return BOTTLENECK_HOST_TRANSFER;
    }
    
    return BOTTLENECK_NONE;
}

void print_optimization_suggestions(bottleneck_type_t bottleneck) {
    switch (bottleneck) {
        case BOTTLENECK_MEMORY_BANDWIDTH:
            printf("Memory bandwidth bottleneck detected:\n");
            printf("  - Increase data reuse\n");
            printf("  - Use blocking algorithms\n");
            printf("  - Optimize data layout\n");
            break;
            
        case BOTTLENECK_COMPUTATION:
            printf("Computation bottleneck detected:\n");
            printf("  - Algorithm is well-optimized\n");
            printf("  - Consider using lower precision\n");
            printf("  - Increase parallelism if possible\n");
            break;
            
        case BOTTLENECK_SYNCHRONIZATION:
            printf("Synchronization bottleneck detected:\n");
            printf("  - Reduce barriers and locks\n");
            printf("  - Improve load balancing\n");
            printf("  - Use asynchronous operations\n");
            break;
            
        case BOTTLENECK_HOST_TRANSFER:
            printf("Host transfer bottleneck detected:\n");
            printf("  - Batch transfers\n");
            printf("  - Use asynchronous transfers\n");
            printf("  - Minimize host-device communication\n");
            break;
    }
}
```

## Common Performance Issues

### Issue 1: Poor Cache Locality

**Symptoms**:
- High cache miss rates (>30%)
- Low memory bandwidth utilization
- High latency per operation

**Solutions**:
```c
// Problem: Random memory access
void poor_cache_usage(float *matrix, size_t size) {
    for (size_t i = 0; i < size; i++) {
        size_t random_idx = rand() % size;
        process_element(&matrix[random_idx]); // Poor locality
    }
}

// Solution: Sequential access with blocking
void improved_cache_usage(float *matrix, size_t rows, size_t cols) {
    const size_t BLOCK_SIZE = 64;
    
    for (size_t bi = 0; bi < rows; bi += BLOCK_SIZE) {
        for (size_t bj = 0; bj < cols; bj += BLOCK_SIZE) {
            for (size_t i = bi; i < min(bi + BLOCK_SIZE, rows); i++) {
                for (size_t j = bj; j < min(bj + BLOCK_SIZE, cols); j++) {
                    process_element(&matrix[i * cols + j]);
                }
            }
        }
    }
}
```

### Issue 2: Inefficient Memory Transfers

**Symptoms**:
- High PCIe utilization
- NPU idle time
- Poor overall throughput

**Solutions**:
```c
// Problem: Many small transfers
void inefficient_transfers(npu_handle_t npu) {
    for (int i = 0; i < 1000; i++) {
        float data[16];
        // Many small transfers
        npu_memory_copy(npu, host_data + i*16, npu_buffer + i*16, 16*sizeof(float));
    }
}

// Solution: Batched transfer
void efficient_transfers(npu_handle_t npu) {
    // Single large transfer
    npu_memory_copy(npu, host_data, npu_buffer, 1000*16*sizeof(float));
}

// Asynchronous transfer pattern
void async_transfer_pattern(npu_handle_t npu) {
    // Start transfer
    npu_async_memory_copy(npu, host_data, npu_buffer, size, &transfer_handle);
    
    // Do other work while transfer completes
    prepare_next_operation();
    
    // Wait for completion
    npu_wait_transfer(npu, transfer_handle);
    
    // Process data
    npu_process_data(npu, npu_buffer);
}
```

### Issue 3: Load Imbalance

**Symptoms**:
- Uneven PE utilization
- Some PEs idle while others are busy
- Poor scaling with number of PEs

**Solutions**:
```c
// Dynamic work distribution
typedef struct {
    size_t start_idx;
    size_t end_idx;
    bool completed;
} work_item_t;

void dynamic_load_balancing(npu_handle_t npu, work_item_t *work_items, size_t num_items) {
    atomic_size_t next_work_item = ATOMIC_VAR_INIT(0);
    
    // Each PE gets work dynamically
    for (size_t pe_id = 0; pe_id < num_pes; pe_id++) {
        npu_submit_work(npu, pe_id, get_next_work_item, &next_work_item, work_items);
    }
}

work_item_t* get_next_work_item(atomic_size_t *next_item, work_item_t *work_items) {
    size_t item_idx = atomic_fetch_add(next_item, 1);
    return &work_items[item_idx];
}
```

## Best Practices

### 1. Algorithm Design
- **Use blocked algorithms** for large matrices to improve cache locality
- **Fuse operations** when possible to reduce memory traffic
- **Minimize data movement** between host and device
- **Batch small operations** into larger ones

### 2. Memory Management
- **Align data structures** to cache line boundaries (64 bytes)
- **Use memory pools** for frequent allocations/deallocations
- **Prefer sequential access patterns** over random access
- **Reuse buffers** when possible to reduce allocation overhead

### 3. Hardware Utilization
- **Balance computation and memory access** to avoid bottlenecks
- **Use all available PEs** through proper parallelization
- **Monitor thermal conditions** and adjust frequency accordingly
- **Enable power management** for energy efficiency

### 4. Profiling and Monitoring
- **Profile regularly** during development to catch performance regressions
- **Use performance counters** to identify bottlenecks
- **Monitor cache hit rates** as a key performance indicator
- **Track power consumption** for thermal and efficiency analysis

### 5. Code Optimization
- **Vectorize operations** when possible using SIMD instructions
- **Minimize synchronization points** between PEs
- **Use appropriate data types** (INT8 vs FP16 vs FP32) based on accuracy requirements
- **Pipeline operations** to overlap computation and data transfer

By following these optimization techniques and best practices, you can achieve maximum performance from the FPGA NPU while maintaining code maintainability and reliability.