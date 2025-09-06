/**
 * Throughput Benchmarks Implementation
 * 
 * Comprehensive throughput testing for various NPU operations
 * including matrix multiplication, convolution, and element-wise operations
 */

#include "benchmark_framework.h"
#include <math.h>

// =============================================================================
// Matrix Multiplication Throughput Benchmarks
// =============================================================================

int benchmark_matmul_throughput(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    // Matrix dimensions based on benchmark size
    size_t matrix_dim;
    switch (config->size) {
        case BENCHMARK_SIZE_SMALL:  matrix_dim = 128; break;
        case BENCHMARK_SIZE_MEDIUM: matrix_dim = 256; break;
        case BENCHMARK_SIZE_LARGE:  matrix_dim = 512; break;
        case BENCHMARK_SIZE_XLARGE: matrix_dim = 1024; break;
        default: matrix_dim = 256; break;
    }
    
    size_t matrix_size = matrix_dim * matrix_dim * sizeof(float);
    
    // Allocate matrices
    float *matrix_a = (float*)allocate_aligned_buffer(matrix_size, 64);
    float *matrix_b = (float*)allocate_aligned_buffer(matrix_size, 64);
    float *matrix_c = (float*)allocate_aligned_buffer(matrix_size, 64);
    
    if (!matrix_a || !matrix_b || !matrix_c) {
        fprintf(stderr, "Failed to allocate matrices\n");
        return -1;
    }
    
    // Initialize matrices with random data
    initialize_matrix_random(matrix_a, matrix_dim, matrix_dim);
    initialize_matrix_random(matrix_b, matrix_dim, matrix_dim);
    
    printf("Running matrix multiplication benchmark (%zux%zu)\n", matrix_dim, matrix_dim);
    
    // Warmup iterations
    for (uint32_t i = 0; i < config->warmup_iterations; i++) {
        npu_result_t result = npu_matrix_multiply(ctx->npu_handle, 
                                                 matrix_a, matrix_b, matrix_c,
                                                 matrix_dim, matrix_dim, matrix_dim);
        if (result != NPU_SUCCESS) {
            fprintf(stderr, "Warmup iteration %u failed: %d\n", i, result);
            goto cleanup;
        }
    }
    
    // Performance measurement iterations
    uint64_t total_operations = 0;
    struct timespec start_time, end_time;
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    for (uint32_t i = 0; i < config->iterations; i++) {
        npu_result_t result = npu_matrix_multiply(ctx->npu_handle,
                                                 matrix_a, matrix_b, matrix_c,
                                                 matrix_dim, matrix_dim, matrix_dim);
        if (result != NPU_SUCCESS) {
            fprintf(stderr, "Benchmark iteration %u failed: %d\n", i, result);
            metrics->errors_count++;
        } else {
            total_operations += 2ULL * matrix_dim * matrix_dim * matrix_dim; // 2*M*N*K operations
        }
        
        // Progress reporting
        if (config->iterations > 10 && (i + 1) % (config->iterations / 10) == 0) {
            printf("Progress: %u/%u iterations completed\n", i + 1, config->iterations);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Calculate performance metrics
    double duration = calculate_duration_seconds(start_time, end_time);
    metrics->duration_seconds = duration;
    metrics->operations_count = total_operations;
    metrics->throughput_gops = (double)total_operations / (duration * 1e9);
    metrics->latency_ms = (duration * 1000.0) / config->iterations;
    
    printf("Matrix multiplication throughput: %.2f GOPS\n", metrics->throughput_gops);
    printf("Average latency: %.3f ms\n", metrics->latency_ms);
    
cleanup:
    free_aligned_buffer(matrix_a);
    free_aligned_buffer(matrix_b);
    free_aligned_buffer(matrix_c);
    
    return (metrics->errors_count == 0) ? 0 : -1;
}

// =============================================================================
// Convolution Throughput Benchmarks
// =============================================================================

int benchmark_conv2d_throughput(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    // Convolution parameters based on benchmark size
    size_t input_height, input_width, input_channels;
    size_t output_channels, kernel_size;
    
    switch (config->size) {
        case BENCHMARK_SIZE_SMALL:
            input_height = 32; input_width = 32; input_channels = 16;
            output_channels = 32; kernel_size = 3;
            break;
        case BENCHMARK_SIZE_MEDIUM:
            input_height = 64; input_width = 64; input_channels = 32;
            output_channels = 64; kernel_size = 3;
            break;
        case BENCHMARK_SIZE_LARGE:
            input_height = 128; input_width = 128; input_channels = 64;
            output_channels = 128; kernel_size = 3;
            break;
        case BENCHMARK_SIZE_XLARGE:
            input_height = 224; input_width = 224; input_channels = 128;
            output_channels = 256; kernel_size = 3;
            break;
        default:
            input_height = 64; input_width = 64; input_channels = 32;
            output_channels = 64; kernel_size = 3;
            break;
    }
    
    size_t input_size = input_height * input_width * input_channels * sizeof(float);
    size_t kernel_size_bytes = kernel_size * kernel_size * input_channels * output_channels * sizeof(float);
    size_t output_height = input_height - kernel_size + 1;
    size_t output_width = input_width - kernel_size + 1;
    size_t output_size = output_height * output_width * output_channels * sizeof(float);
    
    // Allocate tensors
    float *input = (float*)allocate_aligned_buffer(input_size, 64);
    float *kernel = (float*)allocate_aligned_buffer(kernel_size_bytes, 64);
    float *output = (float*)allocate_aligned_buffer(output_size, 64);
    
    if (!input || !kernel || !output) {
        fprintf(stderr, "Failed to allocate convolution tensors\n");
        return -1;
    }
    
    // Initialize with random data
    initialize_tensor_random(input, input_size / sizeof(float));
    initialize_tensor_random(kernel, kernel_size_bytes / sizeof(float));
    
    printf("Running 2D convolution benchmark (%zux%zux%zu -> %zux%zux%zu)\n",
           input_height, input_width, input_channels,
           output_height, output_width, output_channels);
    
    // Warmup iterations
    for (uint32_t i = 0; i < config->warmup_iterations; i++) {
        npu_result_t result = npu_conv2d(ctx->npu_handle,
                                        input, kernel, output,
                                        input_height, input_width, input_channels,
                                        output_channels, kernel_size, kernel_size,
                                        1, 1, 0, 0); // stride=1, padding=0
        if (result != NPU_SUCCESS) {
            fprintf(stderr, "Convolution warmup iteration %u failed: %d\n", i, result);
            goto cleanup;
        }
    }
    
    // Performance measurement iterations
    uint64_t total_operations = 0;
    struct timespec start_time, end_time;
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    for (uint32_t i = 0; i < config->iterations; i++) {
        npu_result_t result = npu_conv2d(ctx->npu_handle,
                                        input, kernel, output,
                                        input_height, input_width, input_channels,
                                        output_channels, kernel_size, kernel_size,
                                        1, 1, 0, 0);
        if (result != NPU_SUCCESS) {
            fprintf(stderr, "Convolution iteration %u failed: %d\n", i, result);
            metrics->errors_count++;
        } else {
            // Operations = output_elements * kernel_elements * input_channels
            total_operations += output_height * output_width * output_channels *
                               kernel_size * kernel_size * input_channels * 2; // MAC operations
        }
        
        // Progress reporting
        if (config->iterations > 10 && (i + 1) % (config->iterations / 10) == 0) {
            printf("Progress: %u/%u iterations completed\n", i + 1, config->iterations);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Calculate performance metrics
    double duration = calculate_duration_seconds(start_time, end_time);
    metrics->duration_seconds = duration;
    metrics->operations_count = total_operations;
    metrics->throughput_gops = (double)total_operations / (duration * 1e9);
    metrics->latency_ms = (duration * 1000.0) / config->iterations;
    
    printf("2D convolution throughput: %.2f GOPS\n", metrics->throughput_gops);
    printf("Average latency: %.3f ms\n", metrics->latency_ms);
    
cleanup:
    free_aligned_buffer(input);
    free_aligned_buffer(kernel);
    free_aligned_buffer(output);
    
    return (metrics->errors_count == 0) ? 0 : -1;
}

// =============================================================================
// Element-wise Operations Throughput Benchmarks
// =============================================================================

int benchmark_elementwise_throughput(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    // Tensor size based on benchmark size
    size_t tensor_elements;
    switch (config->size) {
        case BENCHMARK_SIZE_SMALL:  tensor_elements = 1024 * 1024; break;      // 1M elements
        case BENCHMARK_SIZE_MEDIUM: tensor_elements = 4 * 1024 * 1024; break;  // 4M elements
        case BENCHMARK_SIZE_LARGE:  tensor_elements = 16 * 1024 * 1024; break; // 16M elements
        case BENCHMARK_SIZE_XLARGE: tensor_elements = 64 * 1024 * 1024; break; // 64M elements
        default: tensor_elements = 4 * 1024 * 1024; break;
    }
    
    size_t tensor_size = tensor_elements * sizeof(float);
    
    // Allocate tensors
    float *tensor_a = (float*)allocate_aligned_buffer(tensor_size, 64);
    float *tensor_b = (float*)allocate_aligned_buffer(tensor_size, 64);
    float *tensor_c = (float*)allocate_aligned_buffer(tensor_size, 64);
    
    if (!tensor_a || !tensor_b || !tensor_c) {
        fprintf(stderr, "Failed to allocate element-wise tensors\n");
        return -1;
    }
    
    // Initialize with random data
    initialize_tensor_random(tensor_a, tensor_elements);
    initialize_tensor_random(tensor_b, tensor_elements);
    
    printf("Running element-wise operations benchmark (%zu elements)\n", tensor_elements);
    
    // Test different element-wise operations
    const char* operations[] = {"add", "multiply", "relu", "sigmoid"};
    const int num_operations = sizeof(operations) / sizeof(operations[0]);
    
    for (int op_idx = 0; op_idx < num_operations; op_idx++) {
        printf("Testing %s operation...\n", operations[op_idx]);
        
        // Warmup iterations
        for (uint32_t i = 0; i < config->warmup_iterations; i++) {
            npu_result_t result;
            switch (op_idx) {
                case 0: // Add
                    result = npu_tensor_add(ctx->npu_handle, tensor_a, tensor_b, tensor_c, tensor_elements);
                    break;
                case 1: // Multiply
                    result = npu_tensor_multiply(ctx->npu_handle, tensor_a, tensor_b, tensor_c, tensor_elements);
                    break;
                case 2: // ReLU
                    result = npu_relu(ctx->npu_handle, tensor_a, tensor_c, tensor_elements);
                    break;
                case 3: // Sigmoid
                    result = npu_sigmoid(ctx->npu_handle, tensor_a, tensor_c, tensor_elements);
                    break;
                default:
                    result = NPU_ERROR_INVALID_PARAMETER;
                    break;
            }
            
            if (result != NPU_SUCCESS) {
                fprintf(stderr, "%s warmup iteration %u failed: %d\n", operations[op_idx], i, result);
                continue;
            }
        }
        
        // Performance measurement iterations
        uint64_t operation_count = 0;
        struct timespec start_time, end_time;
        
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        
        for (uint32_t i = 0; i < config->iterations; i++) {
            npu_result_t result;
            switch (op_idx) {
                case 0: // Add
                    result = npu_tensor_add(ctx->npu_handle, tensor_a, tensor_b, tensor_c, tensor_elements);
                    break;
                case 1: // Multiply
                    result = npu_tensor_multiply(ctx->npu_handle, tensor_a, tensor_b, tensor_c, tensor_elements);
                    break;
                case 2: // ReLU
                    result = npu_relu(ctx->npu_handle, tensor_a, tensor_c, tensor_elements);
                    break;
                case 3: // Sigmoid
                    result = npu_sigmoid(ctx->npu_handle, tensor_a, tensor_c, tensor_elements);
                    break;
                default:
                    result = NPU_ERROR_INVALID_PARAMETER;
                    break;
            }
            
            if (result != NPU_SUCCESS) {
                fprintf(stderr, "%s iteration %u failed: %d\n", operations[op_idx], i, result);
                metrics->errors_count++;
            } else {
                operation_count += tensor_elements;
            }
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        
        // Calculate performance metrics for this operation
        double duration = calculate_duration_seconds(start_time, end_time);
        double throughput_gops = (double)operation_count / (duration * 1e9);
        double latency_ms = (duration * 1000.0) / config->iterations;
        
        printf("  %s: %.2f GOPS, %.3f ms latency\n", operations[op_idx], throughput_gops, latency_ms);
        
        // Accumulate overall metrics
        metrics->operations_count += operation_count;
        metrics->duration_seconds += duration;
    }
    
    // Calculate overall metrics
    metrics->throughput_gops = (double)metrics->operations_count / (metrics->duration_seconds * 1e9);
    metrics->latency_ms = (metrics->duration_seconds * 1000.0) / (config->iterations * num_operations);
    
    printf("Overall element-wise throughput: %.2f GOPS\n", metrics->throughput_gops);
    
    free_aligned_buffer(tensor_a);
    free_aligned_buffer(tensor_b);
    free_aligned_buffer(tensor_c);
    
    return (metrics->errors_count == 0) ? 0 : -1;
}

// =============================================================================
// Memory Bandwidth Benchmarks
// =============================================================================

int benchmark_memory_bandwidth(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    // Buffer size based on benchmark size
    size_t buffer_size;
    switch (config->size) {
        case BENCHMARK_SIZE_SMALL:  buffer_size = 1 * 1024 * 1024; break;     // 1 MB
        case BENCHMARK_SIZE_MEDIUM: buffer_size = 16 * 1024 * 1024; break;    // 16 MB
        case BENCHMARK_SIZE_LARGE:  buffer_size = 64 * 1024 * 1024; break;    // 64 MB
        case BENCHMARK_SIZE_XLARGE: buffer_size = 256 * 1024 * 1024; break;   // 256 MB
        default: buffer_size = 16 * 1024 * 1024; break;
    }
    
    // Allocate buffers
    void *src_buffer = allocate_aligned_buffer(buffer_size, 64);
    void *dst_buffer = allocate_aligned_buffer(buffer_size, 64);
    
    if (!src_buffer || !dst_buffer) {
        fprintf(stderr, "Failed to allocate memory bandwidth test buffers\n");
        return -1;
    }
    
    // Initialize source buffer
    memset(src_buffer, 0xAA, buffer_size);
    
    printf("Running memory bandwidth benchmark (%zu MB)\n", buffer_size / (1024 * 1024));
    
    // Warmup iterations
    for (uint32_t i = 0; i < config->warmup_iterations; i++) {
        npu_result_t result = npu_memory_copy(ctx->npu_handle, src_buffer, dst_buffer, buffer_size);
        if (result != NPU_SUCCESS) {
            fprintf(stderr, "Memory copy warmup iteration %u failed: %d\n", i, result);
            goto cleanup;
        }
    }
    
    // Performance measurement iterations
    uint64_t total_bytes = 0;
    struct timespec start_time, end_time;
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    for (uint32_t i = 0; i < config->iterations; i++) {
        npu_result_t result = npu_memory_copy(ctx->npu_handle, src_buffer, dst_buffer, buffer_size);
        if (result != NPU_SUCCESS) {
            fprintf(stderr, "Memory copy iteration %u failed: %d\n", i, result);
            metrics->errors_count++;
        } else {
            total_bytes += buffer_size;
        }
        
        // Progress reporting
        if (config->iterations > 10 && (i + 1) % (config->iterations / 10) == 0) {
            printf("Progress: %u/%u iterations completed\n", i + 1, config->iterations);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Calculate performance metrics
    double duration = calculate_duration_seconds(start_time, end_time);
    metrics->duration_seconds = duration;
    metrics->operations_count = total_bytes;
    metrics->bandwidth_gbps = (double)total_bytes / (duration * 1e9);
    metrics->latency_ms = (duration * 1000.0) / config->iterations;
    
    printf("Memory bandwidth: %.2f GB/s\n", metrics->bandwidth_gbps);
    printf("Average latency: %.3f ms\n", metrics->latency_ms);
    
cleanup:
    free_aligned_buffer(src_buffer);
    free_aligned_buffer(dst_buffer);
    
    return (metrics->errors_count == 0) ? 0 : -1;
}

// =============================================================================
// Helper Functions
// =============================================================================

void initialize_matrix_random(float *matrix, size_t rows, size_t cols)
{
    srand(time(NULL));
    for (size_t i = 0; i < rows * cols; i++) {
        matrix[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f; // Range [-1, 1]
    }
}

void initialize_tensor_random(float *tensor, size_t elements)
{
    srand(time(NULL));
    for (size_t i = 0; i < elements; i++) {
        tensor[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f; // Range [-1, 1]
    }
}

double calculate_duration_seconds(struct timespec start, struct timespec end)
{
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}