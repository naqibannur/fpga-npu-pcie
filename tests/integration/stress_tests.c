/**
 * Stress and Reliability Tests
 * 
 * High-intensity testing for system stability, resource limits,
 * and long-term reliability validation
 */

#include "integration_test_framework.h"
#include <pthread.h>
#include <sys/resource.h>

// =============================================================================
// Test Configuration
// =============================================================================

#define STRESS_TEST_DURATION 60  // seconds
#define MAX_CONCURRENT_THREADS 8
#define LARGE_BUFFER_SIZE (16 * 1024 * 1024) // 16MB
#define STRESS_ITERATIONS 1000

// Thread data structure for concurrent testing
typedef struct {
    int thread_id;
    test_context_t *ctx;
    npu_handle_t npu_handle;
    bool stop_requested;
    uint64_t operations_completed;
    uint64_t errors_encountered;
} thread_data_t;

// =============================================================================
// Memory Stress Tests
// =============================================================================

/**
 * Test memory allocation limits and fragmentation
 */
test_result_t test_memory_stress(test_context_t *ctx)
{
    const size_t max_buffers = 64;
    const size_t buffer_size = 1024 * 1024; // 1MB each
    npu_buffer_handle_t buffers[max_buffers];
    void *mapped_ptrs[max_buffers];
    size_t allocated_count = 0;
    
    start_performance_monitoring(ctx);
    
    // Phase 1: Allocate maximum number of buffers
    for (size_t i = 0; i < max_buffers; i++) {
        buffers[i] = npu_buffer_alloc(ctx->npu_handle, buffer_size, NPU_ALLOC_COHERENT);
        if (!buffers[i]) {
            if (g_verbose_output) {
                printf("  Maximum allocation reached at %zu buffers\n", i);
            }
            break;
        }
        allocated_count++;
        
        // Map buffer and write test pattern
        mapped_ptrs[i] = npu_buffer_map(ctx->npu_handle, buffers[i]);
        if (mapped_ptrs[i]) {
            uint32_t *data = (uint32_t *)mapped_ptrs[i];
            for (size_t j = 0; j < buffer_size / sizeof(uint32_t); j++) {
                data[j] = (uint32_t)(i * 0x1000 + j);
            }
        }
    }
    
    ASSERT_TRUE(ctx, allocated_count > 0, "Failed to allocate any buffers");
    
    // Phase 2: Verify data integrity after allocation stress
    for (size_t i = 0; i < allocated_count; i++) {
        if (mapped_ptrs[i]) {
            uint32_t *data = (uint32_t *)mapped_ptrs[i];
            for (size_t j = 0; j < buffer_size / sizeof(uint32_t); j++) {
                uint32_t expected = (uint32_t)(i * 0x1000 + j);
                ASSERT_EQ(ctx, expected, data[j], 
                         "Data corruption in buffer %zu at offset %zu", i, j);
            }
        }
    }
    
    // Phase 3: Test fragmentation by freeing every other buffer
    for (size_t i = 1; i < allocated_count; i += 2) {
        if (mapped_ptrs[i]) {
            npu_buffer_unmap(ctx->npu_handle, buffers[i], mapped_ptrs[i]);
        }
        npu_buffer_free(ctx->npu_handle, buffers[i]);
        buffers[i] = NULL;
        mapped_ptrs[i] = NULL;
    }
    
    // Phase 4: Try to allocate new buffers in fragmented space
    size_t refragment_count = 0;
    for (size_t i = 1; i < allocated_count; i += 2) {
        buffers[i] = npu_buffer_alloc(ctx->npu_handle, buffer_size, NPU_ALLOC_COHERENT);
        if (buffers[i]) {
            refragment_count++;
        }
    }
    
    if (g_verbose_output) {
        printf("  Fragmentation test: reallocated %zu/%zu buffers\n", 
               refragment_count, allocated_count / 2);
    }
    
    // Cleanup all remaining buffers
    for (size_t i = 0; i < allocated_count; i++) {
        if (buffers[i]) {
            if (mapped_ptrs[i]) {
                npu_buffer_unmap(ctx->npu_handle, buffers[i], mapped_ptrs[i]);
            }
            npu_buffer_free(ctx->npu_handle, buffers[i]);
        }
    }
    
    update_performance_metrics(ctx, allocated_count * buffer_size);
    
    return TEST_RESULT_PASS;
}

/**
 * Test large buffer operations
 */
test_result_t test_large_buffer_operations(test_context_t *ctx)
{
    const size_t large_size = LARGE_BUFFER_SIZE;
    
    // Allocate large buffer
    npu_buffer_handle_t large_buffer = npu_buffer_alloc(
        ctx->npu_handle, large_size, NPU_ALLOC_COHERENT);
    ASSERT_NOT_NULL(ctx, large_buffer, "Failed to allocate large buffer");
    
    // Map buffer
    void *mapped_ptr = npu_buffer_map(ctx->npu_handle, large_buffer);
    ASSERT_NOT_NULL(ctx, mapped_ptr, "Failed to map large buffer");
    
    uint8_t *data = (uint8_t *)mapped_ptr;
    
    start_performance_monitoring(ctx);
    
    // Phase 1: Write test pattern
    for (size_t i = 0; i < large_size; i++) {
        data[i] = (uint8_t)(i & 0xFF);
    }
    
    // Phase 2: Verify pattern
    for (size_t i = 0; i < large_size; i++) {
        uint8_t expected = (uint8_t)(i & 0xFF);
        ASSERT_EQ(ctx, expected, data[i], 
                 "Large buffer data corruption at offset %zu", i);
    }
    
    // Phase 3: Test buffer synchronization with large data
    int result = npu_buffer_sync(ctx->npu_handle, large_buffer, 0);
    ASSERT_SUCCESS(ctx, result, "Large buffer sync failed");
    
    // Phase 4: Stress test with random access pattern
    const size_t random_accesses = 10000;
    for (size_t i = 0; i < random_accesses; i++) {
        size_t offset = rand() % large_size;
        uint8_t expected = (uint8_t)(offset & 0xFF);
        ASSERT_EQ(ctx, expected, data[offset], 
                 "Random access verification failed at offset %zu", offset);
    }
    
    // Cleanup
    npu_buffer_unmap(ctx->npu_handle, large_buffer, mapped_ptr);
    npu_buffer_free(ctx->npu_handle, large_buffer);
    
    update_performance_metrics(ctx, large_size + random_accesses);
    
    if (g_verbose_output) {
        printf("  Large buffer test: %zu MB processed\n", large_size / (1024 * 1024));
    }
    
    return TEST_RESULT_PASS;
}

// =============================================================================
// Computational Stress Tests
// =============================================================================

/**
 * Worker thread for concurrent operations
 */
static void* stress_worker_thread(void *arg)
{
    thread_data_t *thread_data = (thread_data_t *)arg;
    const int matrix_size = 32;
    const int tensor_size = matrix_size * matrix_size;
    
    // Allocate thread-local data
    float *matrix_a = allocate_test_buffer(tensor_size * sizeof(float), 64);
    float *matrix_b = allocate_test_buffer(tensor_size * sizeof(float), 64);
    float *result = allocate_test_buffer(tensor_size * sizeof(float), 64);
    
    if (!matrix_a || !matrix_b || !result) {
        thread_data->errors_encountered++;
        return NULL;
    }
    
    // Initialize matrices
    for (int i = 0; i < tensor_size; i++) {
        matrix_a[i] = (float)(rand() % 100) / 10.0f;
        matrix_b[i] = (float)(rand() % 100) / 10.0f;
    }
    
    npu_tensor_t tensor_a = npu_create_tensor(
        matrix_a, 1, 1, matrix_size, matrix_size, NPU_DTYPE_FLOAT32);
    npu_tensor_t tensor_b = npu_create_tensor(
        matrix_b, 1, 1, matrix_size, matrix_size, NPU_DTYPE_FLOAT32);
    npu_tensor_t tensor_result = npu_create_tensor(
        result, 1, 1, matrix_size, matrix_size, NPU_DTYPE_FLOAT32);
    
    // Perform operations until stop is requested
    while (!thread_data->stop_requested) {
        // Matrix multiplication
        int ret = npu_matrix_multiply(thread_data->npu_handle, 
                                     &tensor_a, &tensor_b, &tensor_result);
        if (ret != NPU_SUCCESS) {
            thread_data->errors_encountered++;
        } else {
            thread_data->operations_completed++;
        }
        
        // Element-wise addition
        ret = npu_add(thread_data->npu_handle, &tensor_a, &tensor_b, &tensor_result);
        if (ret != NPU_SUCCESS) {
            thread_data->errors_encountered++;
        } else {
            thread_data->operations_completed++;
        }
        
        // Small delay to prevent overwhelming the system
        usleep(1000); // 1ms
    }
    
    // Cleanup
    free_test_buffer(matrix_a);
    free_test_buffer(matrix_b);
    free_test_buffer(result);
    
    return NULL;
}

/**
 * Test concurrent operations with multiple threads
 */
test_result_t test_concurrent_operations(test_context_t *ctx)
{
    const int num_threads = 4;
    const int test_duration = 10; // seconds
    
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];
    
    start_performance_monitoring(ctx);
    
    // Initialize thread data
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].ctx = ctx;
        thread_data[i].npu_handle = ctx->npu_handle;
        thread_data[i].stop_requested = false;
        thread_data[i].operations_completed = 0;
        thread_data[i].errors_encountered = 0;
    }
    
    // Start worker threads
    for (int i = 0; i < num_threads; i++) {
        int ret = pthread_create(&threads[i], NULL, stress_worker_thread, &thread_data[i]);
        ASSERT_EQ(ctx, 0, ret, "Failed to create worker thread %d", i);
    }
    
    // Let threads run for test duration
    sleep(test_duration);
    
    // Signal threads to stop
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].stop_requested = true;
    }
    
    // Wait for threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Collect results
    uint64_t total_operations = 0;
    uint64_t total_errors = 0;
    
    for (int i = 0; i < num_threads; i++) {
        total_operations += thread_data[i].operations_completed;
        total_errors += thread_data[i].errors_encountered;
        
        if (g_verbose_output) {
            printf("  Thread %d: %lu ops, %lu errors\n", 
                   i, thread_data[i].operations_completed, 
                   thread_data[i].errors_encountered);
        }
    }
    
    // Validate results
    ASSERT_TRUE(ctx, total_operations > 0, "No operations completed");
    ASSERT_TRUE(ctx, total_errors < total_operations / 10, 
                "Too many errors: %lu/%lu (>10%%)", total_errors, total_operations);
    
    update_performance_metrics(ctx, total_operations);
    
    if (g_verbose_output) {
        printf("  Concurrent test: %lu total ops, %lu errors, %.1f%% success rate\n",
               total_operations, total_errors, 
               (double)(total_operations - total_errors) / total_operations * 100.0);
    }
    
    return TEST_RESULT_PASS;
}

/**
 * Test intensive computational workload
 */
test_result_t test_computational_intensity(test_context_t *ctx)
{
    const int iterations = STRESS_ITERATIONS;
    const int matrix_size = 64;
    const int tensor_size = matrix_size * matrix_size;
    
    // Allocate test matrices
    float *matrix_a = allocate_test_buffer(tensor_size * sizeof(float), 64);
    float *matrix_b = allocate_test_buffer(tensor_size * sizeof(float), 64);
    float *result = allocate_test_buffer(tensor_size * sizeof(float), 64);
    
    ASSERT_NOT_NULL(ctx, matrix_a, "Failed to allocate matrix A");
    ASSERT_NOT_NULL(ctx, matrix_b, "Failed to allocate matrix B");
    ASSERT_NOT_NULL(ctx, result, "Failed to allocate result matrix");
    
    // Initialize with random data
    initialize_test_data(matrix_a, tensor_size * sizeof(float), PATTERN_RANDOM);
    initialize_test_data(matrix_b, tensor_size * sizeof(float), PATTERN_RANDOM);
    
    npu_tensor_t tensor_a = npu_create_tensor(
        matrix_a, 1, 1, matrix_size, matrix_size, NPU_DTYPE_FLOAT32);
    npu_tensor_t tensor_b = npu_create_tensor(
        matrix_b, 1, 1, matrix_size, matrix_size, NPU_DTYPE_FLOAT32);
    npu_tensor_t tensor_result = npu_create_tensor(
        result, 1, 1, matrix_size, matrix_size, NPU_DTYPE_FLOAT32);
    
    start_performance_monitoring(ctx);
    
    uint32_t error_count = 0;
    
    // Perform intensive computation
    for (int i = 0; i < iterations; i++) {
        // Matrix multiplication
        int ret = npu_matrix_multiply(ctx->npu_handle, &tensor_a, &tensor_b, &tensor_result);
        if (ret != NPU_SUCCESS) {
            error_count++;
        }
        
        // Use result as input for next iteration (creates dependency chain)
        memcpy(matrix_a, result, tensor_size * sizeof(float));
        
        // Check for thermal throttling periodically
        if (i % 100 == 0) {
            if (check_thermal_throttling()) {
                printf("  Thermal throttling detected at iteration %d\n", i);
            }
        }
        
        // Progress indicator
        if (g_verbose_output && i % (iterations / 10) == 0) {
            printf("  Progress: %d/%d iterations\n", i, iterations);
        }
    }
    
    // Validate final result is reasonable
    bool has_valid_data = false;
    for (int i = 0; i < tensor_size && !has_valid_data; i++) {
        if (isfinite(result[i]) && result[i] != 0.0f) {
            has_valid_data = true;
        }
    }
    
    ASSERT_TRUE(ctx, has_valid_data, "Result contains no valid data after computation");
    ASSERT_TRUE(ctx, error_count < iterations / 20, 
                "Too many errors during computation: %d/%d", error_count, iterations);
    
    update_performance_metrics(ctx, (uint64_t)iterations * tensor_size * matrix_size * 2);
    
    // Cleanup
    free_test_buffer(matrix_a);
    free_test_buffer(matrix_b);
    free_test_buffer(result);
    
    if (g_verbose_output) {
        printf("  Computational intensity: %d iterations, %d errors\n", 
               iterations, error_count);
    }
    
    return TEST_RESULT_PASS;
}

// =============================================================================
// Resource Exhaustion Tests
// =============================================================================

/**
 * Test system behavior under resource pressure
 */
test_result_t test_resource_exhaustion(test_context_t *ctx)
{
    struct rusage usage_start, usage_end;
    getrusage(RUSAGE_SELF, &usage_start);
    
    start_performance_monitoring(ctx);
    
    // Phase 1: Memory exhaustion test
    const size_t num_buffers = 128;
    const size_t buffer_size = 512 * 1024; // 512KB each
    npu_buffer_handle_t *buffers = malloc(num_buffers * sizeof(npu_buffer_handle_t));
    ASSERT_NOT_NULL(ctx, buffers, "Failed to allocate buffer array");
    
    size_t successful_allocations = 0;
    for (size_t i = 0; i < num_buffers; i++) {
        buffers[i] = npu_buffer_alloc(ctx->npu_handle, buffer_size, NPU_ALLOC_COHERENT);
        if (buffers[i]) {
            successful_allocations++;
        } else {
            break; // Stop on first failure
        }
    }
    
    ASSERT_TRUE(ctx, successful_allocations > 0, "Failed to allocate any buffers");
    
    if (g_verbose_output) {
        printf("  Allocated %zu/%zu buffers before exhaustion\n", 
               successful_allocations, num_buffers);
    }
    
    // Phase 2: Test operations under memory pressure
    if (successful_allocations >= 2) {
        void *ptr_a = npu_buffer_map(ctx->npu_handle, buffers[0]);
        void *ptr_b = npu_buffer_map(ctx->npu_handle, buffers[1]);
        
        if (ptr_a && ptr_b) {
            initialize_test_data(ptr_a, buffer_size, PATTERN_RANDOM);
            initialize_test_data(ptr_b, buffer_size, PATTERN_SEQUENCE);
            
            // Try operations under memory pressure
            const int small_size = 16;
            npu_tensor_t tensor_a = npu_create_tensor(ptr_a, 1, 1, small_size, small_size, NPU_DTYPE_FLOAT32);
            npu_tensor_t tensor_b = npu_create_tensor(ptr_b, 1, 1, small_size, small_size, NPU_DTYPE_FLOAT32);
            
            int result = npu_add(ctx->npu_handle, &tensor_a, &tensor_b, &tensor_a);
            ASSERT_SUCCESS(ctx, result, "Operation failed under memory pressure");
            
            npu_buffer_unmap(ctx->npu_handle, buffers[0], ptr_a);
            npu_buffer_unmap(ctx->npu_handle, buffers[1], ptr_b);
        }
    }
    
    // Phase 3: Clean up and test recovery
    for (size_t i = 0; i < successful_allocations; i++) {
        npu_buffer_free(ctx->npu_handle, buffers[i]);
    }
    free(buffers);
    
    // Test that system recovers after resource exhaustion
    npu_buffer_handle_t recovery_buffer = npu_buffer_alloc(
        ctx->npu_handle, 1024, NPU_ALLOC_COHERENT);
    ASSERT_NOT_NULL(ctx, recovery_buffer, "System failed to recover from resource exhaustion");
    npu_buffer_free(ctx->npu_handle, recovery_buffer);
    
    getrusage(RUSAGE_SELF, &usage_end);
    
    // Check resource usage
    long memory_increase = usage_end.ru_maxrss - usage_start.ru_maxrss;
    if (g_verbose_output) {
        printf("  Memory usage increase: %ld KB\n", memory_increase);
    }
    
    update_performance_metrics(ctx, successful_allocations * buffer_size);
    
    return TEST_RESULT_PASS;
}

// =============================================================================
// Long-term Reliability Tests
// =============================================================================

/**
 * Test long-term stability and reliability
 */
test_result_t test_long_term_stability(test_context_t *ctx)
{
    const int duration_seconds = 30; // Reduced for faster testing
    const int operation_interval_ms = 100;
    
    time_t start_time = time(NULL);
    uint64_t total_operations = 0;
    uint64_t error_count = 0;
    
    // Allocate test data
    const int matrix_size = 32;
    const int tensor_size = matrix_size * matrix_size;
    float *test_data = allocate_test_buffer(tensor_size * sizeof(float), 64);
    ASSERT_NOT_NULL(ctx, test_data, "Failed to allocate test data");
    
    initialize_test_data(test_data, tensor_size * sizeof(float), PATTERN_RANDOM);
    
    npu_tensor_t tensor = npu_create_tensor(
        test_data, 1, 1, matrix_size, matrix_size, NPU_DTYPE_FLOAT32);
    
    start_performance_monitoring(ctx);
    
    printf("  Running stability test for %d seconds...\n", duration_seconds);
    
    while ((time(NULL) - start_time) < duration_seconds) {
        // Perform matrix multiplication with itself
        int result = npu_matrix_multiply(ctx->npu_handle, &tensor, &tensor, &tensor);
        if (result == NPU_SUCCESS) {
            total_operations++;
        } else {
            error_count++;
        }
        
        // Check system health periodically
        if (total_operations % 10 == 0) {
            uint32_t health_status;
            if (npu_check_device_health(ctx->npu_handle, &health_status) != NPU_SUCCESS) {
                error_count++;
            }
        }
        
        // Monitor for thermal issues
        if (total_operations % 50 == 0) {
            if (check_thermal_throttling()) {
                printf("  Thermal throttling detected during stability test\n");
            }
        }
        
        usleep(operation_interval_ms * 1000);
    }
    
    // Validate results
    ASSERT_TRUE(ctx, total_operations > 0, "No operations completed during stability test");
    
    double error_rate = (double)error_count / (double)total_operations;
    ASSERT_TRUE(ctx, error_rate < 0.05, // Less than 5% error rate
                "High error rate during stability test: %.2f%% (%lu/%lu)",
                error_rate * 100.0, error_count, total_operations);
    
    // Check final system state
    uint32_t final_status;
    int status_result = npu_get_status(ctx->npu_handle, &final_status);
    ASSERT_SUCCESS(ctx, status_result, "Failed to get final NPU status");
    ASSERT_TRUE(ctx, final_status & 0x1, "NPU not ready after stability test");
    
    update_performance_metrics(ctx, total_operations * tensor_size * matrix_size * 2);
    
    free_test_buffer(test_data);
    
    if (g_verbose_output) {
        printf("  Stability test: %lu operations, %lu errors (%.2f%% success)\n",
               total_operations, error_count, 
               (double)(total_operations - error_count) / total_operations * 100.0);
    }
    
    return TEST_RESULT_PASS;
}

// =============================================================================
// Test Configuration and Registration
// =============================================================================

static test_config_t stress_test_configs[] = {
    {
        .name = "Memory Stress Test",
        .category = TEST_CATEGORY_STRESS,
        .severity = TEST_SEVERITY_HIGH,
        .timeout_seconds = 60,
        .enable_performance_monitoring = true,
        .enable_stress_testing = true,
        .enable_concurrent_execution = false,
        .iterations = 1,
        .data_size_bytes = 64 * 1024 * 1024
    },
    {
        .name = "Large Buffer Operations",
        .category = TEST_CATEGORY_STRESS,
        .severity = TEST_SEVERITY_MEDIUM,
        .timeout_seconds = 45,
        .enable_performance_monitoring = true,
        .enable_stress_testing = true,
        .enable_concurrent_execution = false,
        .iterations = 1,
        .data_size_bytes = LARGE_BUFFER_SIZE
    },
    {
        .name = "Concurrent Operations",
        .category = TEST_CATEGORY_STRESS,
        .severity = TEST_SEVERITY_HIGH,
        .timeout_seconds = 30,
        .enable_performance_monitoring = true,
        .enable_stress_testing = true,
        .enable_concurrent_execution = true,
        .iterations = 1,
        .data_size_bytes = 4096
    },
    {
        .name = "Computational Intensity",
        .category = TEST_CATEGORY_PERFORMANCE,
        .severity = TEST_SEVERITY_MEDIUM,
        .timeout_seconds = 120,
        .enable_performance_monitoring = true,
        .enable_stress_testing = true,
        .enable_concurrent_execution = false,
        .iterations = STRESS_ITERATIONS,
        .data_size_bytes = 16384
    },
    {
        .name = "Resource Exhaustion",
        .category = TEST_CATEGORY_RELIABILITY,
        .severity = TEST_SEVERITY_HIGH,
        .timeout_seconds = 60,
        .enable_performance_monitoring = true,
        .enable_stress_testing = true,
        .enable_concurrent_execution = false,
        .iterations = 1,
        .data_size_bytes = 128 * 512 * 1024
    },
    {
        .name = "Long-term Stability",
        .category = TEST_CATEGORY_RELIABILITY,
        .severity = TEST_SEVERITY_CRITICAL,
        .timeout_seconds = 90,
        .enable_performance_monitoring = true,
        .enable_stress_testing = true,
        .enable_concurrent_execution = false,
        .iterations = 1,
        .data_size_bytes = 4096
    }
};

static test_function_t stress_test_functions[] = {
    test_memory_stress,
    test_large_buffer_operations,
    test_concurrent_operations,
    test_computational_intensity,
    test_resource_exhaustion,
    test_long_term_stability
};

test_suite_t* create_stress_test_suite(void)
{
    test_suite_t *suite = malloc(sizeof(test_suite_t));
    if (!suite) {
        return NULL;
    }
    
    strcpy(suite->name, "Stress and Reliability Tests");
    suite->tests = stress_test_functions;
    suite->configs = stress_test_configs;
    suite->test_count = sizeof(stress_test_functions) / sizeof(stress_test_functions[0]);
    suite->tests_passed = 0;
    suite->tests_failed = 0;
    suite->tests_skipped = 0;
    
    return suite;
}