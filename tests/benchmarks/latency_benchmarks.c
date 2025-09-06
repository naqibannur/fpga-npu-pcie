/**
 * Latency Benchmarks Implementation
 * 
 * Comprehensive latency testing for NPU operations
 * focusing on single operation latency and batch processing latency
 */

#include "benchmark_framework.h"
#include <math.h>

// =============================================================================
// Single Operation Latency Benchmarks
// =============================================================================

int benchmark_single_operation_latency(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    printf("Running single operation latency benchmark\n");
    
    // Small matrix for single operation timing
    const size_t matrix_dim = 64;
    size_t matrix_size = matrix_dim * matrix_dim * sizeof(float);
    
    float *matrix_a = (float*)allocate_aligned_buffer(matrix_size, 64);
    float *matrix_b = (float*)allocate_aligned_buffer(matrix_size, 64);
    float *matrix_c = (float*)allocate_aligned_buffer(matrix_size, 64);
    
    if (!matrix_a || !matrix_b || !matrix_c) {
        fprintf(stderr, "Failed to allocate matrices for latency test\n");
        return -1;
    }
    
    // Initialize matrices
    initialize_matrix_random(matrix_a, matrix_dim, matrix_dim);
    initialize_matrix_random(matrix_b, matrix_dim, matrix_dim);
    
    // Warmup
    for (uint32_t i = 0; i < config->warmup_iterations; i++) {
        npu_matrix_multiply(ctx->npu_handle, matrix_a, matrix_b, matrix_c,
                           matrix_dim, matrix_dim, matrix_dim);
    }
    
    // Measure individual operation latencies
    double *latencies = malloc(config->iterations * sizeof(double));
    if (!latencies) {
        fprintf(stderr, "Failed to allocate latency array\n");
        goto cleanup;
    }
    
    printf("Measuring %u individual operations...\n", config->iterations);
    
    for (uint32_t i = 0; i < config->iterations; i++) {
        struct timespec start_time, end_time;
        
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        
        npu_result_t result = npu_matrix_multiply(ctx->npu_handle,
                                                 matrix_a, matrix_b, matrix_c,
                                                 matrix_dim, matrix_dim, matrix_dim);
        
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        
        if (result != NPU_SUCCESS) {
            fprintf(stderr, "Operation %u failed: %d\n", i, result);
            metrics->errors_count++;
            latencies[i] = 0.0;
        } else {
            latencies[i] = calculate_duration_seconds(start_time, end_time) * 1000.0; // Convert to ms
            metrics->operations_count++;
        }
        
        // Progress reporting for long tests
        if (config->iterations > 100 && (i + 1) % (config->iterations / 10) == 0) {
            printf("Progress: %u/%u operations completed\n", i + 1, config->iterations);
        }
    }
    
    // Calculate latency statistics
    calculate_latency_statistics(latencies, config->iterations, metrics);
    
    printf("Single operation latency statistics:\n");
    printf("  Average: %.3f ms\n", metrics->latency_ms);
    printf("  Minimum: %.3f ms\n", metrics->min_latency_ms);
    printf("  Maximum: %.3f ms\n", metrics->max_latency_ms);
    printf("  95th percentile: %.3f ms\n", metrics->p95_latency_ms);
    printf("  99th percentile: %.3f ms\n", metrics->p99_latency_ms);
    printf("  Standard deviation: %.3f ms\n", metrics->latency_stddev_ms);
    
    free(latencies);

cleanup:
    free_aligned_buffer(matrix_a);
    free_aligned_buffer(matrix_b);
    free_aligned_buffer(matrix_c);
    
    return (metrics->errors_count == 0) ? 0 : -1;
}

// =============================================================================
// Batch Operation Latency Benchmarks
// =============================================================================

int benchmark_batch_operation_latency(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    printf("Running batch operation latency benchmark\n");
    
    // Batch parameters based on benchmark size
    size_t batch_size;
    size_t matrix_dim = 32; // Smaller matrices for batch processing
    
    switch (config->size) {
        case BENCHMARK_SIZE_SMALL:  batch_size = 4; break;
        case BENCHMARK_SIZE_MEDIUM: batch_size = 16; break;
        case BENCHMARK_SIZE_LARGE:  batch_size = 64; break;
        case BENCHMARK_SIZE_XLARGE: batch_size = 256; break;
        default: batch_size = 16; break;
    }
    
    size_t matrix_size = matrix_dim * matrix_dim * sizeof(float);
    size_t batch_matrix_size = batch_size * matrix_size;
    
    // Allocate batch matrices
    float *batch_a = (float*)allocate_aligned_buffer(batch_matrix_size, 64);
    float *batch_b = (float*)allocate_aligned_buffer(batch_matrix_size, 64);
    float *batch_c = (float*)allocate_aligned_buffer(batch_matrix_size, 64);
    
    if (!batch_a || !batch_b || !batch_c) {
        fprintf(stderr, "Failed to allocate batch matrices\n");
        return -1;
    }
    
    // Initialize batch matrices
    for (size_t i = 0; i < batch_size; i++) {
        initialize_matrix_random(batch_a + i * matrix_dim * matrix_dim, matrix_dim, matrix_dim);
        initialize_matrix_random(batch_b + i * matrix_dim * matrix_dim, matrix_dim, matrix_dim);
    }
    
    printf("Testing batch size: %zu matrices (%ux%u each)\n", batch_size, (unsigned)matrix_dim, (unsigned)matrix_dim);
    
    // Warmup
    for (uint32_t i = 0; i < config->warmup_iterations; i++) {
        npu_batch_matrix_multiply(ctx->npu_handle, batch_a, batch_b, batch_c,
                                 batch_size, matrix_dim, matrix_dim, matrix_dim);
    }
    
    // Measure batch operation latencies
    double *batch_latencies = malloc(config->iterations * sizeof(double));
    if (!batch_latencies) {
        fprintf(stderr, "Failed to allocate batch latency array\n");
        goto cleanup;
    }
    
    printf("Measuring %u batch operations...\n", config->iterations);
    
    for (uint32_t i = 0; i < config->iterations; i++) {
        struct timespec start_time, end_time;
        
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        
        npu_result_t result = npu_batch_matrix_multiply(ctx->npu_handle,
                                                       batch_a, batch_b, batch_c,
                                                       batch_size, matrix_dim, matrix_dim, matrix_dim);
        
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        
        if (result != NPU_SUCCESS) {
            fprintf(stderr, "Batch operation %u failed: %d\n", i, result);
            metrics->errors_count++;
            batch_latencies[i] = 0.0;
        } else {
            batch_latencies[i] = calculate_duration_seconds(start_time, end_time) * 1000.0; // Convert to ms
            metrics->operations_count += batch_size;
        }
    }
    
    // Calculate batch latency statistics
    calculate_latency_statistics(batch_latencies, config->iterations, metrics);
    
    // Calculate per-operation latency within batch
    double avg_per_op_latency = metrics->latency_ms / batch_size;
    
    printf("Batch operation latency statistics:\n");
    printf("  Batch average: %.3f ms\n", metrics->latency_ms);
    printf("  Per-operation in batch: %.3f ms\n", avg_per_op_latency);
    printf("  Batch minimum: %.3f ms\n", metrics->min_latency_ms);
    printf("  Batch maximum: %.3f ms\n", metrics->max_latency_ms);
    printf("  Batch 95th percentile: %.3f ms\n", metrics->p95_latency_ms);
    printf("  Batch 99th percentile: %.3f ms\n", metrics->p99_latency_ms);
    
    free(batch_latencies);

cleanup:
    free_aligned_buffer(batch_a);
    free_aligned_buffer(batch_b);
    free_aligned_buffer(batch_c);
    
    return (metrics->errors_count == 0) ? 0 : -1;
}

// =============================================================================
// Memory Access Latency Benchmarks
// =============================================================================

int benchmark_memory_access_latency(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    printf("Running memory access latency benchmark\n");
    
    // Test different memory access patterns
    const size_t test_sizes[] = {4, 64, 1024, 4096, 65536}; // 4B to 64KB
    const size_t num_test_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (size_t size_idx = 0; size_idx < num_test_sizes; size_idx++) {
        size_t buffer_size = test_sizes[size_idx];
        
        void *src_buffer = allocate_aligned_buffer(buffer_size, 64);
        void *dst_buffer = allocate_aligned_buffer(buffer_size, 64);
        
        if (!src_buffer || !dst_buffer) {
            fprintf(stderr, "Failed to allocate buffers for size %zu\n", buffer_size);
            continue;
        }
        
        // Initialize source buffer
        memset(src_buffer, 0x55, buffer_size);
        
        printf("Testing memory access for %zu bytes...\n", buffer_size);
        
        // Warmup
        for (uint32_t i = 0; i < config->warmup_iterations; i++) {
            npu_memory_copy(ctx->npu_handle, src_buffer, dst_buffer, buffer_size);
        }
        
        // Measure memory access latencies
        double *mem_latencies = malloc(config->iterations * sizeof(double));
        if (!mem_latencies) {
            fprintf(stderr, "Failed to allocate memory latency array\n");
            free_aligned_buffer(src_buffer);
            free_aligned_buffer(dst_buffer);
            continue;
        }
        
        for (uint32_t i = 0; i < config->iterations; i++) {
            struct timespec start_time, end_time;
            
            clock_gettime(CLOCK_MONOTONIC, &start_time);
            
            npu_result_t result = npu_memory_copy(ctx->npu_handle, src_buffer, dst_buffer, buffer_size);
            
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            
            if (result != NPU_SUCCESS) {
                fprintf(stderr, "Memory copy %u failed for size %zu: %d\n", i, buffer_size, result);
                metrics->errors_count++;
                mem_latencies[i] = 0.0;
            } else {
                mem_latencies[i] = calculate_duration_seconds(start_time, end_time) * 1000000.0; // Convert to microseconds
                metrics->operations_count++;
            }
        }
        
        // Calculate memory access statistics
        performance_metrics_t size_metrics = {0};
        calculate_latency_statistics(mem_latencies, config->iterations, &size_metrics);
        
        printf("  %zu bytes - Average: %.1f μs, Min: %.1f μs, Max: %.1f μs, P95: %.1f μs\n",
               buffer_size, size_metrics.latency_ms * 1000.0,
               size_metrics.min_latency_ms * 1000.0,
               size_metrics.max_latency_ms * 1000.0,
               size_metrics.p95_latency_ms * 1000.0);
        
        // Accumulate overall metrics
        metrics->latency_ms += size_metrics.latency_ms;
        metrics->duration_seconds += size_metrics.duration_seconds;
        
        free(mem_latencies);
        free_aligned_buffer(src_buffer);
        free_aligned_buffer(dst_buffer);
    }
    
    // Average across all memory sizes
    metrics->latency_ms /= num_test_sizes;
    
    return (metrics->errors_count == 0) ? 0 : -1;
}

// =============================================================================
// Context Switch Latency Benchmarks
// =============================================================================

int benchmark_context_switch_latency(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    printf("Running context switch latency benchmark\n");
    
    // Simple operations for context switching
    const size_t vector_size = 1024;
    size_t buffer_size = vector_size * sizeof(float);
    
    float *vector_a = (float*)allocate_aligned_buffer(buffer_size, 64);
    float *vector_b = (float*)allocate_aligned_buffer(buffer_size, 64);
    float *result_vector = (float*)allocate_aligned_buffer(buffer_size, 64);
    
    if (!vector_a || !vector_b || !result_vector) {
        fprintf(stderr, "Failed to allocate vectors for context switch test\n");
        return -1;
    }
    
    // Initialize vectors
    initialize_tensor_random(vector_a, vector_size);
    initialize_tensor_random(vector_b, vector_size);
    
    // Create multiple contexts to switch between
    const int num_contexts = 4;
    npu_context_t contexts[num_contexts];
    
    for (int i = 0; i < num_contexts; i++) {
        if (npu_create_context(ctx->npu_handle, &contexts[i]) != NPU_SUCCESS) {
            fprintf(stderr, "Failed to create context %d\n", i);
            goto cleanup;
        }
    }
    
    printf("Testing context switches between %d contexts\n", num_contexts);
    
    // Warmup
    for (uint32_t i = 0; i < config->warmup_iterations; i++) {
        int ctx_idx = i % num_contexts;
        npu_set_context(ctx->npu_handle, contexts[ctx_idx]);
        npu_tensor_add(ctx->npu_handle, vector_a, vector_b, result_vector, vector_size);
    }
    
    // Measure context switch latencies
    double *switch_latencies = malloc(config->iterations * sizeof(double));
    if (!switch_latencies) {
        fprintf(stderr, "Failed to allocate switch latency array\n");
        goto cleanup_contexts;
    }
    
    for (uint32_t i = 0; i < config->iterations; i++) {
        int current_ctx = i % num_contexts;
        int next_ctx = (i + 1) % num_contexts;
        
        // Set initial context
        npu_set_context(ctx->npu_handle, contexts[current_ctx]);
        
        struct timespec start_time, end_time;
        
        // Measure time to switch context and execute operation
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        
        npu_result_t result1 = npu_set_context(ctx->npu_handle, contexts[next_ctx]);
        npu_result_t result2 = npu_tensor_add(ctx->npu_handle, vector_a, vector_b, result_vector, vector_size);
        
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        
        if (result1 != NPU_SUCCESS || result2 != NPU_SUCCESS) {
            fprintf(stderr, "Context switch %u failed: ctx=%d, op=%d\n", i, result1, result2);
            metrics->errors_count++;
            switch_latencies[i] = 0.0;
        } else {
            switch_latencies[i] = calculate_duration_seconds(start_time, end_time) * 1000000.0; // Convert to microseconds
            metrics->operations_count++;
        }
    }
    
    // Calculate context switch statistics
    calculate_latency_statistics(switch_latencies, config->iterations, metrics);
    
    printf("Context switch latency statistics:\n");
    printf("  Average: %.1f μs\n", metrics->latency_ms * 1000.0);
    printf("  Minimum: %.1f μs\n", metrics->min_latency_ms * 1000.0);
    printf("  Maximum: %.1f μs\n", metrics->max_latency_ms * 1000.0);
    printf("  95th percentile: %.1f μs\n", metrics->p95_latency_ms * 1000.0);
    printf("  99th percentile: %.1f μs\n", metrics->p99_latency_ms * 1000.0);
    
    free(switch_latencies);

cleanup_contexts:
    for (int i = 0; i < num_contexts; i++) {
        npu_destroy_context(ctx->npu_handle, contexts[i]);
    }

cleanup:
    free_aligned_buffer(vector_a);
    free_aligned_buffer(vector_b);
    free_aligned_buffer(result_vector);
    
    return (metrics->errors_count == 0) ? 0 : -1;
}

// =============================================================================
// Helper Functions
// =============================================================================

void calculate_latency_statistics(double *latencies, uint32_t count, performance_metrics_t *metrics)
{
    if (count == 0) return;
    
    // Sort latencies for percentile calculation
    qsort(latencies, count, sizeof(double), compare_double);
    
    // Calculate basic statistics
    double sum = 0.0;
    double min_val = latencies[0];
    double max_val = latencies[0];
    
    for (uint32_t i = 0; i < count; i++) {
        if (latencies[i] > 0.0) { // Skip failed operations
            sum += latencies[i];
            if (latencies[i] < min_val) min_val = latencies[i];
            if (latencies[i] > max_val) max_val = latencies[i];
        }
    }
    
    metrics->latency_ms = sum / count;
    metrics->min_latency_ms = min_val;
    metrics->max_latency_ms = max_val;
    
    // Calculate percentiles
    uint32_t p95_idx = (uint32_t)(count * 0.95);
    uint32_t p99_idx = (uint32_t)(count * 0.99);
    metrics->p95_latency_ms = latencies[p95_idx];
    metrics->p99_latency_ms = latencies[p99_idx];
    
    // Calculate standard deviation
    double variance_sum = 0.0;
    for (uint32_t i = 0; i < count; i++) {
        if (latencies[i] > 0.0) {
            double diff = latencies[i] - metrics->latency_ms;
            variance_sum += diff * diff;
        }
    }
    metrics->latency_stddev_ms = sqrt(variance_sum / count);
}

int compare_double(const void *a, const void *b)
{
    double da = *(const double*)a;
    double db = *(const double*)b;
    
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}