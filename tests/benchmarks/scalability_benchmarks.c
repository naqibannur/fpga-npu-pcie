/**
 * Scalability Benchmarks Implementation
 * 
 * Tests for multi-threaded performance and data size scaling
 * to evaluate NPU scalability characteristics
 */

#include "benchmark_framework.h"
#include <pthread.h>
#include <math.h>

// =============================================================================
// Multi-threaded Benchmark Infrastructure
// =============================================================================

typedef struct {
    benchmark_context_t *ctx;
    int thread_id;
    uint32_t iterations_per_thread;
    performance_metrics_t thread_metrics;
    pthread_barrier_t *start_barrier;
    pthread_barrier_t *end_barrier;
} thread_context_t;

// =============================================================================
// Multi-threaded Throughput Benchmarks
// =============================================================================

void* multithreaded_matmul_worker(void *arg)
{
    thread_context_t *tctx = (thread_context_t*)arg;
    benchmark_context_t *ctx = tctx->ctx;
    
    // Allocate per-thread matrices
    const size_t matrix_dim = 128;
    size_t matrix_size = matrix_dim * matrix_dim * sizeof(float);
    
    float *matrix_a = (float*)allocate_aligned_buffer(matrix_size, 64);
    float *matrix_b = (float*)allocate_aligned_buffer(matrix_size, 64);
    float *matrix_c = (float*)allocate_aligned_buffer(matrix_size, 64);
    
    if (!matrix_a || !matrix_b || !matrix_c) {
        fprintf(stderr, "Thread %d: Failed to allocate matrices\n", tctx->thread_id);
        return NULL;
    }
    
    // Initialize matrices
    initialize_matrix_random(matrix_a, matrix_dim, matrix_dim);
    initialize_matrix_random(matrix_b, matrix_dim, matrix_dim);
    
    // Wait for all threads to be ready
    pthread_barrier_wait(tctx->start_barrier);
    
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    uint64_t operations_performed = 0;
    
    // Perform matrix multiplications
    for (uint32_t i = 0; i < tctx->iterations_per_thread; i++) {
        npu_result_t result = npu_matrix_multiply(ctx->npu_handle,
                                                 matrix_a, matrix_b, matrix_c,
                                                 matrix_dim, matrix_dim, matrix_dim);
        if (result == NPU_SUCCESS) {
            operations_performed += 2ULL * matrix_dim * matrix_dim * matrix_dim;
        } else {
            tctx->thread_metrics.errors_count++;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Calculate thread metrics
    double duration = calculate_duration_seconds(start_time, end_time);
    tctx->thread_metrics.duration_seconds = duration;
    tctx->thread_metrics.operations_count = operations_performed;
    tctx->thread_metrics.throughput_gops = (double)operations_performed / (duration * 1e9);
    
    // Wait for all threads to complete
    pthread_barrier_wait(tctx->end_barrier);
    
    free_aligned_buffer(matrix_a);
    free_aligned_buffer(matrix_b);
    free_aligned_buffer(matrix_c);
    
    return NULL;
}

int benchmark_multithreaded_throughput(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    uint32_t num_threads = config->thread_count;
    if (num_threads == 0) num_threads = 4; // Default to 4 threads
    
    printf("Running multi-threaded throughput benchmark with %u threads\n", num_threads);
    
    // Allocate thread contexts
    thread_context_t *thread_contexts = calloc(num_threads, sizeof(thread_context_t));
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    
    if (!thread_contexts || !threads) {
        fprintf(stderr, "Failed to allocate thread resources\n");
        free(thread_contexts);
        free(threads);
        return -1;
    }
    
    // Initialize barriers
    pthread_barrier_t start_barrier, end_barrier;
    pthread_barrier_init(&start_barrier, NULL, num_threads);
    pthread_barrier_init(&end_barrier, NULL, num_threads);
    
    // Setup thread contexts
    uint32_t iterations_per_thread = config->iterations / num_threads;
    
    for (uint32_t i = 0; i < num_threads; i++) {
        thread_contexts[i].ctx = ctx;
        thread_contexts[i].thread_id = i;
        thread_contexts[i].iterations_per_thread = iterations_per_thread;
        thread_contexts[i].start_barrier = &start_barrier;
        thread_contexts[i].end_barrier = &end_barrier;
        memset(&thread_contexts[i].thread_metrics, 0, sizeof(performance_metrics_t));
    }
    
    printf("Starting %u threads with %u iterations each...\n", num_threads, iterations_per_thread);
    
    // Create and start threads
    struct timespec overall_start, overall_end;
    clock_gettime(CLOCK_MONOTONIC, &overall_start);
    
    for (uint32_t i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, multithreaded_matmul_worker, &thread_contexts[i]) != 0) {
            fprintf(stderr, "Failed to create thread %u\n", i);
            goto cleanup;
        }
    }
    
    // Wait for all threads to complete
    for (uint32_t i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &overall_end);
    
    // Aggregate results from all threads
    uint64_t total_operations = 0;
    uint64_t total_errors = 0;
    double max_thread_duration = 0.0;
    
    for (uint32_t i = 0; i < num_threads; i++) {
        total_operations += thread_contexts[i].thread_metrics.operations_count;
        total_errors += thread_contexts[i].thread_metrics.errors_count;
        if (thread_contexts[i].thread_metrics.duration_seconds > max_thread_duration) {
            max_thread_duration = thread_contexts[i].thread_metrics.duration_seconds;
        }
        
        printf("Thread %u: %.2f GOPS, %llu operations, %llu errors\n",
               i, thread_contexts[i].thread_metrics.throughput_gops,
               (unsigned long long)thread_contexts[i].thread_metrics.operations_count,
               (unsigned long long)thread_contexts[i].thread_metrics.errors_count);
    }
    
    // Calculate overall metrics
    double overall_duration = calculate_duration_seconds(overall_start, overall_end);
    metrics->duration_seconds = overall_duration;
    metrics->operations_count = total_operations;
    metrics->errors_count = total_errors;
    metrics->throughput_gops = (double)total_operations / (overall_duration * 1e9);
    
    printf("Multi-threaded results:\n");
    printf("  Total throughput: %.2f GOPS\n", metrics->throughput_gops);
    printf("  Total operations: %llu\n", (unsigned long long)total_operations);
    printf("  Total errors: %llu\n", (unsigned long long)total_errors);
    printf("  Overall duration: %.3f seconds\n", overall_duration);
    printf("  Max thread duration: %.3f seconds\n", max_thread_duration);
    
cleanup:
    pthread_barrier_destroy(&start_barrier);
    pthread_barrier_destroy(&end_barrier);
    free(thread_contexts);
    free(threads);
    
    return (total_errors == 0) ? 0 : -1;
}

// =============================================================================
// Data Size Scaling Benchmarks
// =============================================================================

int benchmark_data_size_scaling(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    printf("Running data size scaling benchmark\n");
    
    // Test different matrix sizes to evaluate scaling
    const size_t matrix_sizes[] = {32, 64, 128, 256, 512, 1024};
    const size_t num_sizes = sizeof(matrix_sizes) / sizeof(matrix_sizes[0]);
    
    double scaling_metrics[num_sizes];
    
    for (size_t size_idx = 0; size_idx < num_sizes; size_idx++) {
        size_t matrix_dim = matrix_sizes[size_idx];
        size_t matrix_size = matrix_dim * matrix_dim * sizeof(float);
        
        printf("Testing matrix size: %zux%zu\n", matrix_dim, matrix_dim);
        
        // Allocate matrices for current size
        float *matrix_a = (float*)allocate_aligned_buffer(matrix_size, 64);
        float *matrix_b = (float*)allocate_aligned_buffer(matrix_size, 64);
        float *matrix_c = (float*)allocate_aligned_buffer(matrix_size, 64);
        
        if (!matrix_a || !matrix_b || !matrix_c) {
            fprintf(stderr, "Failed to allocate matrices for size %zu\n", matrix_dim);
            scaling_metrics[size_idx] = 0.0;
            continue;
        }
        
        // Initialize matrices
        initialize_matrix_random(matrix_a, matrix_dim, matrix_dim);
        initialize_matrix_random(matrix_b, matrix_dim, matrix_dim);
        
        // Warmup for current size
        for (uint32_t i = 0; i < config->warmup_iterations; i++) {
            npu_matrix_multiply(ctx->npu_handle, matrix_a, matrix_b, matrix_c,
                               matrix_dim, matrix_dim, matrix_dim);
        }
        
        // Measure performance for current size
        struct timespec start_time, end_time;
        uint64_t operations_performed = 0;
        uint32_t size_iterations = config->iterations;
        
        // Adjust iterations for larger matrices to maintain reasonable test time
        if (matrix_dim >= 512) {
            size_iterations = config->iterations / 4;
        } else if (matrix_dim >= 256) {
            size_iterations = config->iterations / 2;
        }
        
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        
        for (uint32_t i = 0; i < size_iterations; i++) {
            npu_result_t result = npu_matrix_multiply(ctx->npu_handle,
                                                     matrix_a, matrix_b, matrix_c,
                                                     matrix_dim, matrix_dim, matrix_dim);
            if (result == NPU_SUCCESS) {
                operations_performed += 2ULL * matrix_dim * matrix_dim * matrix_dim;
            }
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        
        double duration = calculate_duration_seconds(start_time, end_time);
        double throughput = (double)operations_performed / (duration * 1e9);
        scaling_metrics[size_idx] = throughput;
        
        printf("  %zux%zu: %.2f GOPS (%.3f s, %u iterations)\n",
               matrix_dim, matrix_dim, throughput, duration, size_iterations);
        
        free_aligned_buffer(matrix_a);
        free_aligned_buffer(matrix_b);
        free_aligned_buffer(matrix_c);
    }
    
    // Analyze scaling behavior
    printf("\nScaling Analysis:\n");
    printf("Matrix Size  | Throughput | Scaling Factor\n");
    printf("-------------|------------|---------------\n");
    
    for (size_t i = 0; i < num_sizes; i++) {
        double scaling_factor = (i > 0) ? (scaling_metrics[i] / scaling_metrics[0]) : 1.0;
        printf("%4zux%-4zu   | %8.2f   | %8.2fx\n",
               matrix_sizes[i], matrix_sizes[i], scaling_metrics[i], scaling_factor);
    }
    
    // Set overall metrics to the largest size result
    metrics->throughput_gops = scaling_metrics[num_sizes - 1];
    metrics->operations_count = 1; // Placeholder
    
    return 0;
}

// =============================================================================
// Concurrent Workload Benchmarks
// =============================================================================

void* concurrent_mixed_workload_worker(void *arg)
{
    thread_context_t *tctx = (thread_context_t*)arg;
    benchmark_context_t *ctx = tctx->ctx;
    
    // Each thread performs different types of operations
    const size_t vector_size = 1024;
    const size_t matrix_dim = 64;
    
    size_t vector_bytes = vector_size * sizeof(float);
    size_t matrix_bytes = matrix_dim * matrix_dim * sizeof(float);
    
    // Allocate buffers
    float *vector_a = (float*)allocate_aligned_buffer(vector_bytes, 64);
    float *vector_b = (float*)allocate_aligned_buffer(vector_bytes, 64);
    float *vector_result = (float*)allocate_aligned_buffer(vector_bytes, 64);
    float *matrix_a = (float*)allocate_aligned_buffer(matrix_bytes, 64);
    float *matrix_b = (float*)allocate_aligned_buffer(matrix_bytes, 64);
    float *matrix_result = (float*)allocate_aligned_buffer(matrix_bytes, 64);
    
    if (!vector_a || !vector_b || !vector_result || 
        !matrix_a || !matrix_b || !matrix_result) {
        fprintf(stderr, "Thread %d: Failed to allocate workload buffers\n", tctx->thread_id);
        return NULL;
    }
    
    // Initialize data
    initialize_tensor_random(vector_a, vector_size);
    initialize_tensor_random(vector_b, vector_size);
    initialize_matrix_random(matrix_a, matrix_dim, matrix_dim);
    initialize_matrix_random(matrix_b, matrix_dim, matrix_dim);
    
    // Wait for all threads to be ready
    pthread_barrier_wait(tctx->start_barrier);
    
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    uint64_t operations_performed = 0;
    
    // Mixed workload: alternate between different operation types
    for (uint32_t i = 0; i < tctx->iterations_per_thread; i++) {
        npu_result_t result;
        
        switch (i % 4) {
            case 0: // Vector addition
                result = npu_tensor_add(ctx->npu_handle, vector_a, vector_b, vector_result, vector_size);
                if (result == NPU_SUCCESS) operations_performed += vector_size;
                break;
                
            case 1: // Vector multiplication
                result = npu_tensor_multiply(ctx->npu_handle, vector_a, vector_b, vector_result, vector_size);
                if (result == NPU_SUCCESS) operations_performed += vector_size;
                break;
                
            case 2: // Matrix multiplication
                result = npu_matrix_multiply(ctx->npu_handle, matrix_a, matrix_b, matrix_result,
                                           matrix_dim, matrix_dim, matrix_dim);
                if (result == NPU_SUCCESS) operations_performed += 2ULL * matrix_dim * matrix_dim * matrix_dim;
                break;
                
            case 3: // Activation function (ReLU)
                result = npu_relu(ctx->npu_handle, vector_a, vector_result, vector_size);
                if (result == NPU_SUCCESS) operations_performed += vector_size;
                break;
                
            default:
                result = NPU_ERROR_INVALID_PARAMETER;
                break;
        }
        
        if (result != NPU_SUCCESS) {
            tctx->thread_metrics.errors_count++;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Calculate thread metrics
    double duration = calculate_duration_seconds(start_time, end_time);
    tctx->thread_metrics.duration_seconds = duration;
    tctx->thread_metrics.operations_count = operations_performed;
    tctx->thread_metrics.throughput_gops = (double)operations_performed / (duration * 1e9);
    
    // Wait for all threads to complete
    pthread_barrier_wait(tctx->end_barrier);
    
    // Cleanup
    free_aligned_buffer(vector_a);
    free_aligned_buffer(vector_b);
    free_aligned_buffer(vector_result);
    free_aligned_buffer(matrix_a);
    free_aligned_buffer(matrix_b);
    free_aligned_buffer(matrix_result);
    
    return NULL;
}

int benchmark_concurrent_mixed_workload(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    uint32_t num_threads = config->thread_count;
    if (num_threads == 0) num_threads = 4; // Default to 4 threads
    
    printf("Running concurrent mixed workload benchmark with %u threads\n", num_threads);
    
    // Allocate thread contexts
    thread_context_t *thread_contexts = calloc(num_threads, sizeof(thread_context_t));
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    
    if (!thread_contexts || !threads) {
        fprintf(stderr, "Failed to allocate thread resources\n");
        free(thread_contexts);
        free(threads);
        return -1;
    }
    
    // Initialize barriers
    pthread_barrier_t start_barrier, end_barrier;
    pthread_barrier_init(&start_barrier, NULL, num_threads);
    pthread_barrier_init(&end_barrier, NULL, num_threads);
    
    // Setup thread contexts
    uint32_t iterations_per_thread = config->iterations / num_threads;
    
    for (uint32_t i = 0; i < num_threads; i++) {
        thread_contexts[i].ctx = ctx;
        thread_contexts[i].thread_id = i;
        thread_contexts[i].iterations_per_thread = iterations_per_thread;
        thread_contexts[i].start_barrier = &start_barrier;
        thread_contexts[i].end_barrier = &end_barrier;
        memset(&thread_contexts[i].thread_metrics, 0, sizeof(performance_metrics_t));
    }
    
    printf("Starting %u threads with mixed workloads (%u iterations each)...\n", 
           num_threads, iterations_per_thread);
    
    // Create and start threads
    struct timespec overall_start, overall_end;
    clock_gettime(CLOCK_MONOTONIC, &overall_start);
    
    for (uint32_t i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, concurrent_mixed_workload_worker, &thread_contexts[i]) != 0) {
            fprintf(stderr, "Failed to create thread %u\n", i);
            goto cleanup;
        }
    }
    
    // Wait for all threads to complete
    for (uint32_t i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &overall_end);
    
    // Aggregate results
    uint64_t total_operations = 0;
    uint64_t total_errors = 0;
    
    for (uint32_t i = 0; i < num_threads; i++) {
        total_operations += thread_contexts[i].thread_metrics.operations_count;
        total_errors += thread_contexts[i].thread_metrics.errors_count;
        
        printf("Thread %u: %.2f GOPS, %llu operations, %llu errors\n",
               i, thread_contexts[i].thread_metrics.throughput_gops,
               (unsigned long long)thread_contexts[i].thread_metrics.operations_count,
               (unsigned long long)thread_contexts[i].thread_metrics.errors_count);
    }
    
    // Calculate overall metrics
    double overall_duration = calculate_duration_seconds(overall_start, overall_end);
    metrics->duration_seconds = overall_duration;
    metrics->operations_count = total_operations;
    metrics->errors_count = total_errors;
    metrics->throughput_gops = (double)total_operations / (overall_duration * 1e9);
    
    printf("Concurrent mixed workload results:\n");
    printf("  Total throughput: %.2f GOPS\n", metrics->throughput_gops);
    printf("  Total operations: %llu\n", (unsigned long long)total_operations);
    printf("  Total errors: %llu\n", (unsigned long long)total_errors);
    printf("  Overall duration: %.3f seconds\n", overall_duration);
    
cleanup:
    pthread_barrier_destroy(&start_barrier);
    pthread_barrier_destroy(&end_barrier);
    free(thread_contexts);
    free(threads);
    
    return (total_errors == 0) ? 0 : -1;
}

// =============================================================================
// Load Balancing Benchmarks
// =============================================================================

int benchmark_load_balancing(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    printf("Running load balancing benchmark\n");
    
    // Test different numbers of threads to find optimal load
    const uint32_t thread_counts[] = {1, 2, 4, 8, 16};
    const size_t num_thread_counts = sizeof(thread_counts) / sizeof(thread_counts[0]);
    
    double thread_throughputs[num_thread_counts];
    
    for (size_t tc_idx = 0; tc_idx < num_thread_counts; tc_idx++) {
        uint32_t num_threads = thread_counts[tc_idx];
        
        printf("Testing with %u threads...\n", num_threads);
        
        // Create temporary config for this thread count
        benchmark_config_t temp_config = *config;
        temp_config.thread_count = num_threads;
        temp_config.iterations = config->iterations / 4; // Reduce iterations for efficiency
        
        benchmark_context_t temp_ctx = *ctx;
        temp_ctx.config = temp_config;
        
        performance_metrics_t temp_metrics = {0};
        temp_ctx.result = &temp_metrics;
        
        // Run multi-threaded benchmark
        if (benchmark_multithreaded_throughput(&temp_ctx) == 0) {
            thread_throughputs[tc_idx] = temp_metrics.throughput_gops;
        } else {
            thread_throughputs[tc_idx] = 0.0;
        }
        
        printf("  %u threads: %.2f GOPS\n", num_threads, thread_throughputs[tc_idx]);
    }
    
    // Analyze load balancing
    printf("\nLoad Balancing Analysis:\n");
    printf("Threads | Throughput | Efficiency\n");
    printf("--------|------------|----------\n");
    
    double single_thread_throughput = thread_throughputs[0];
    
    for (size_t i = 0; i < num_thread_counts; i++) {
        double efficiency = (single_thread_throughput > 0) ? 
            (thread_throughputs[i] / (thread_counts[i] * single_thread_throughput)) * 100.0 : 0.0;
        
        printf("%7u | %8.2f   | %7.1f%%\n",
               thread_counts[i], thread_throughputs[i], efficiency);
    }
    
    // Find optimal thread count
    size_t optimal_idx = 0;
    for (size_t i = 1; i < num_thread_counts; i++) {
        if (thread_throughputs[i] > thread_throughputs[optimal_idx]) {
            optimal_idx = i;
        }
    }
    
    printf("\nOptimal configuration: %u threads (%.2f GOPS)\n",
           thread_counts[optimal_idx], thread_throughputs[optimal_idx]);
    
    // Set overall metrics to optimal result
    metrics->throughput_gops = thread_throughputs[optimal_idx];
    metrics->operations_count = 1; // Placeholder
    
    return 0;
}