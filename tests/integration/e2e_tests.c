/**
 * End-to-End Integration Tests
 * 
 * Comprehensive tests for complete FPGA NPU system validation
 * including hardware-software integration and data flow testing
 */

#include "integration_test_framework.h"

// =============================================================================
// Test Data and Configuration
// =============================================================================

#define TEST_MATRIX_SIZE 64
#define TEST_TENSOR_SIZE (TEST_MATRIX_SIZE * TEST_MATRIX_SIZE)
#define TEST_BUFFER_SIZE (TEST_TENSOR_SIZE * sizeof(float))
#define MAX_ITERATIONS 100

// Test matrices for validation
static float test_matrix_a[TEST_TENSOR_SIZE];
static float test_matrix_b[TEST_TENSOR_SIZE];
static float expected_result[TEST_TENSOR_SIZE];
static float actual_result[TEST_TENSOR_SIZE];

// =============================================================================
// Basic System Integration Tests
// =============================================================================

/**
 * Test NPU initialization and basic system health
 */
test_result_t test_system_initialization(test_context_t *ctx)
{
    // Check system health
    ASSERT_TRUE(ctx, check_system_health() == 0, 
                "System health check failed");
    
    // Initialize NPU
    ctx->npu_handle = npu_init();
    ASSERT_NOT_NULL(ctx, ctx->npu_handle, 
                    "NPU initialization failed");
    
    // Get device information
    struct npu_device_info device_info;
    int result = npu_get_device_info(ctx->npu_handle, &device_info);
    ASSERT_SUCCESS(ctx, result, "Failed to get device information");
    
    // Validate device information
    ASSERT_TRUE(ctx, device_info.pe_count > 0, 
                "Invalid PE count: %d", device_info.pe_count);
    ASSERT_TRUE(ctx, device_info.max_frequency > 0, 
                "Invalid max frequency: %d", device_info.max_frequency);
    ASSERT_TRUE(ctx, device_info.memory_size > 0, 
                "Invalid memory size: %lu", device_info.memory_size);
    
    // Check NPU status
    uint32_t status;
    result = npu_get_status(ctx->npu_handle, &status);
    ASSERT_SUCCESS(ctx, result, "Failed to get NPU status");
    ASSERT_TRUE(ctx, status & 0x1, "NPU not ready (status: 0x%x)", status);
    
    if (g_verbose_output) {
        printf("  Device Info: %d PEs, %d MHz, %lu MB memory\n",
               device_info.pe_count, device_info.max_frequency, 
               device_info.memory_size / (1024 * 1024));
    }
    
    return TEST_RESULT_PASS;
}

/**
 * Test memory allocation and basic buffer operations
 */
test_result_t test_memory_management(test_context_t *ctx)
{
    const size_t buffer_size = 4096;
    const uint32_t test_pattern = 0xDEADBEEF;
    
    // Test legacy allocation
    void *legacy_buffer = npu_alloc(ctx->npu_handle, buffer_size);
    ASSERT_NOT_NULL(ctx, legacy_buffer, "Legacy buffer allocation failed");
    
    // Test managed buffer allocation
    npu_buffer_handle_t managed_buffer = npu_buffer_alloc(
        ctx->npu_handle, buffer_size, NPU_ALLOC_COHERENT);
    ASSERT_NOT_NULL(ctx, managed_buffer, "Managed buffer allocation failed");
    
    // Test buffer mapping
    void *mapped_ptr = npu_buffer_map(ctx->npu_handle, managed_buffer);
    ASSERT_NOT_NULL(ctx, mapped_ptr, "Buffer mapping failed");
    
    // Test write/read operations
    uint32_t *data_ptr = (uint32_t *)mapped_ptr;
    for (size_t i = 0; i < buffer_size / sizeof(uint32_t); i++) {
        data_ptr[i] = test_pattern + i;
    }
    
    // Verify data integrity
    for (size_t i = 0; i < buffer_size / sizeof(uint32_t); i++) {
        ASSERT_EQ(ctx, test_pattern + i, data_ptr[i], 
                  "Data integrity check failed at index %zu", i);
    }
    
    // Test buffer synchronization
    int result = npu_buffer_sync(ctx->npu_handle, managed_buffer, 0);
    ASSERT_SUCCESS(ctx, result, "Buffer synchronization failed");
    
    // Test buffer information
    struct npu_dma_buffer buffer_info;
    result = npu_buffer_get_info(ctx->npu_handle, managed_buffer, &buffer_info);
    ASSERT_SUCCESS(ctx, result, "Failed to get buffer information");
    ASSERT_EQ(ctx, buffer_size, buffer_info.size, "Buffer size mismatch");
    
    // Test memory statistics
    size_t total_allocated, total_free;
    uint32_t buffer_count;
    result = npu_get_memory_stats(ctx->npu_handle, &total_allocated, 
                                  &total_free, &buffer_count);
    ASSERT_SUCCESS(ctx, result, "Failed to get memory statistics");
    ASSERT_TRUE(ctx, buffer_count > 0, "Buffer count should be greater than 0");
    
    // Cleanup
    result = npu_buffer_unmap(ctx->npu_handle, managed_buffer, mapped_ptr);
    ASSERT_SUCCESS(ctx, result, "Buffer unmapping failed");
    
    result = npu_buffer_free(ctx->npu_handle, managed_buffer);
    ASSERT_SUCCESS(ctx, result, "Managed buffer free failed");
    
    npu_free(ctx->npu_handle, legacy_buffer);
    
    return TEST_RESULT_PASS;
}

/**
 * Test matrix multiplication end-to-end
 */
test_result_t test_matrix_multiplication_e2e(test_context_t *ctx)
{
    const int matrix_size = 16; // 16x16 matrices for quick test
    const int num_elements = matrix_size * matrix_size;
    
    // Initialize test matrices
    for (int i = 0; i < num_elements; i++) {
        test_matrix_a[i] = (float)(i % 10 + 1); // Values 1-10
        test_matrix_b[i] = (float)((i + 5) % 10 + 1); // Values 1-10, offset
    }
    
    // Calculate expected result (simplified validation)
    // For a proper test, this would use a reference implementation
    memset(expected_result, 0, sizeof(expected_result));
    for (int i = 0; i < matrix_size; i++) {
        for (int j = 0; j < matrix_size; j++) {
            float sum = 0.0f;
            for (int k = 0; k < matrix_size; k++) {
                sum += test_matrix_a[i * matrix_size + k] * 
                       test_matrix_b[k * matrix_size + j];
            }
            expected_result[i * matrix_size + j] = sum;
        }
    }
    
    // Create tensors
    npu_tensor_t tensor_a = npu_create_tensor(
        test_matrix_a, 1, 1, matrix_size, matrix_size, NPU_DTYPE_FLOAT32);
    npu_tensor_t tensor_b = npu_create_tensor(
        test_matrix_b, 1, 1, matrix_size, matrix_size, NPU_DTYPE_FLOAT32);
    npu_tensor_t tensor_c = npu_create_tensor(
        actual_result, 1, 1, matrix_size, matrix_size, NPU_DTYPE_FLOAT32);
    
    // Validate tensor creation
    ASSERT_TRUE(ctx, tensor_a.data != NULL, "Tensor A creation failed");
    ASSERT_TRUE(ctx, tensor_b.data != NULL, "Tensor B creation failed");
    ASSERT_TRUE(ctx, tensor_c.data != NULL, "Tensor C creation failed");
    
    // Start performance monitoring
    start_performance_monitoring(ctx);
    
    // Perform matrix multiplication
    int result = npu_matrix_multiply(ctx->npu_handle, &tensor_a, &tensor_b, &tensor_c);
    ASSERT_SUCCESS(ctx, result, "Matrix multiplication failed");
    
    // Update performance metrics
    update_performance_metrics(ctx, num_elements * matrix_size * 2); // MACs
    
    // Verify results with tolerance for floating-point comparison
    const float tolerance = 0.001f;
    for (int i = 0; i < num_elements; i++) {
        float diff = fabsf(expected_result[i] - actual_result[i]);
        ASSERT_TRUE(ctx, diff <= tolerance,
                    "Result mismatch at index %d: expected %.3f, got %.3f (diff %.6f)",
                    i, expected_result[i], actual_result[i], diff);
    }
    
    if (g_verbose_output) {
        printf("  Matrix multiplication verified: %dx%d matrices\n", 
               matrix_size, matrix_size);
    }
    
    return TEST_RESULT_PASS;
}

/**
 * Test tensor operations with various data types
 */
test_result_t test_tensor_operations(test_context_t *ctx)
{
    const int tensor_size = 256;
    float *input_a = allocate_test_buffer(tensor_size * sizeof(float), 64);
    float *input_b = allocate_test_buffer(tensor_size * sizeof(float), 64);
    float *output = allocate_test_buffer(tensor_size * sizeof(float), 64);
    
    ASSERT_NOT_NULL(ctx, input_a, "Failed to allocate input_a");
    ASSERT_NOT_NULL(ctx, input_b, "Failed to allocate input_b");
    ASSERT_NOT_NULL(ctx, output, "Failed to allocate output");
    
    // Initialize test data
    for (int i = 0; i < tensor_size; i++) {
        input_a[i] = (float)(i % 100) / 10.0f; // 0.0 to 9.9
        input_b[i] = (float)((i + 50) % 100) / 10.0f; // Offset values
        output[i] = 0.0f;
    }
    
    // Create tensors
    npu_tensor_t tensor_a = npu_create_tensor(
        input_a, 1, 1, 16, 16, NPU_DTYPE_FLOAT32);
    npu_tensor_t tensor_b = npu_create_tensor(
        input_b, 1, 1, 16, 16, NPU_DTYPE_FLOAT32);
    npu_tensor_t tensor_out = npu_create_tensor(
        output, 1, 1, 16, 16, NPU_DTYPE_FLOAT32);
    
    // Test element-wise addition
    int result = npu_add(ctx->npu_handle, &tensor_a, &tensor_b, &tensor_out);
    ASSERT_SUCCESS(ctx, result, "Element-wise addition failed");
    
    // Verify addition results
    const float tolerance = 0.001f;
    for (int i = 0; i < tensor_size; i++) {
        float expected = input_a[i] + input_b[i];
        float diff = fabsf(expected - output[i]);
        ASSERT_TRUE(ctx, diff <= tolerance,
                    "Addition result mismatch at index %d: expected %.3f, got %.3f",
                    i, expected, output[i]);
    }
    
    // Reset output for multiplication test
    memset(output, 0, tensor_size * sizeof(float));
    
    // Test element-wise multiplication
    result = npu_multiply(ctx->npu_handle, &tensor_a, &tensor_b, &tensor_out);
    ASSERT_SUCCESS(ctx, result, "Element-wise multiplication failed");
    
    // Verify multiplication results
    for (int i = 0; i < tensor_size; i++) {
        float expected = input_a[i] * input_b[i];
        float diff = fabsf(expected - output[i]);
        ASSERT_TRUE(ctx, diff <= tolerance,
                    "Multiplication result mismatch at index %d: expected %.3f, got %.3f",
                    i, expected, output[i]);
    }
    
    // Cleanup
    free_test_buffer(input_a);
    free_test_buffer(input_b);
    free_test_buffer(output);
    
    update_performance_metrics(ctx, tensor_size * 2); // Two operations
    
    return TEST_RESULT_PASS;
}

/**
 * Test convolution operations
 */
test_result_t test_convolution_operations(test_context_t *ctx)
{
    // Test configuration
    const int input_height = 32;
    const int input_width = 32;
    const int input_channels = 3;
    const int output_channels = 16;
    const int kernel_size = 3;
    const int stride = 1;
    const int padding = 1;
    
    const int input_size = input_channels * input_height * input_width;
    const int kernel_size_total = output_channels * input_channels * kernel_size * kernel_size;
    const int output_size = output_channels * input_height * input_width; // Same size due to padding
    
    // Allocate buffers
    float *input_data = allocate_test_buffer(input_size * sizeof(float), 64);
    float *kernel_data = allocate_test_buffer(kernel_size_total * sizeof(float), 64);
    float *output_data = allocate_test_buffer(output_size * sizeof(float), 64);
    
    ASSERT_NOT_NULL(ctx, input_data, "Failed to allocate input data");
    ASSERT_NOT_NULL(ctx, kernel_data, "Failed to allocate kernel data");
    ASSERT_NOT_NULL(ctx, output_data, "Failed to allocate output data");
    
    // Initialize test data
    initialize_test_data(input_data, input_size * sizeof(float), PATTERN_RANDOM);
    initialize_test_data(kernel_data, kernel_size_total * sizeof(float), PATTERN_RANDOM);
    memset(output_data, 0, output_size * sizeof(float));
    
    // Create tensors
    npu_tensor_t input_tensor = npu_create_tensor(
        input_data, 1, input_channels, input_height, input_width, NPU_DTYPE_FLOAT32);
    npu_tensor_t kernel_tensor = npu_create_tensor(
        kernel_data, output_channels, input_channels, kernel_size, kernel_size, NPU_DTYPE_FLOAT32);
    npu_tensor_t output_tensor = npu_create_tensor(
        output_data, 1, output_channels, input_height, input_width, NPU_DTYPE_FLOAT32);
    
    // Perform convolution
    start_performance_monitoring(ctx);
    
    int result = npu_conv2d(ctx->npu_handle, &input_tensor, &kernel_tensor, &output_tensor,
                            stride, stride, padding, padding);
    ASSERT_SUCCESS(ctx, result, "2D convolution failed");
    
    // Calculate operations count for performance monitoring
    uint64_t ops_count = (uint64_t)output_channels * input_channels * 
                        kernel_size * kernel_size * input_height * input_width * 2; // MACs
    update_performance_metrics(ctx, ops_count);
    
    // Basic validation - check that output is not all zeros
    bool has_non_zero = false;
    for (int i = 0; i < output_size; i++) {
        if (output_data[i] != 0.0f) {
            has_non_zero = true;
            break;
        }
    }
    ASSERT_TRUE(ctx, has_non_zero, "Convolution output is all zeros");
    
    // Check for reasonable output range (heuristic validation)
    float min_val = output_data[0], max_val = output_data[0];
    for (int i = 1; i < output_size; i++) {
        if (output_data[i] < min_val) min_val = output_data[i];
        if (output_data[i] > max_val) max_val = output_data[i];
    }
    
    // Reasonable range check (values shouldn't be extremely large or small)
    ASSERT_TRUE(ctx, fabsf(min_val) < 1000.0f && fabsf(max_val) < 1000.0f,
                "Convolution output values out of reasonable range: [%.3f, %.3f]",
                min_val, max_val);
    
    if (g_verbose_output) {
        printf("  Convolution: %dx%dx%d -> %dx%dx%d, output range [%.3f, %.3f]\n",
               input_height, input_width, input_channels,
               input_height, input_width, output_channels,
               min_val, max_val);
    }
    
    // Cleanup
    free_test_buffer(input_data);
    free_test_buffer(kernel_data);
    free_test_buffer(output_data);
    
    return TEST_RESULT_PASS;
}

/**
 * Test performance monitoring and profiling
 */
test_result_t test_performance_monitoring(test_context_t *ctx)
{
    // Start profiling session
    int result = npu_start_profiling(ctx->npu_handle);
    ASSERT_SUCCESS(ctx, result, "Failed to start profiling");
    
    // Perform several operations to generate performance data
    const int num_operations = 10;
    const int matrix_size = 32;
    const int tensor_size = matrix_size * matrix_size;
    
    float *test_data = allocate_test_buffer(tensor_size * sizeof(float), 64);
    ASSERT_NOT_NULL(ctx, test_data, "Failed to allocate test data");
    
    initialize_test_data(test_data, tensor_size * sizeof(float), PATTERN_RANDOM);
    
    npu_tensor_t tensor = npu_create_tensor(
        test_data, 1, 1, matrix_size, matrix_size, NPU_DTYPE_FLOAT32);
    
    start_performance_monitoring(ctx);
    
    for (int i = 0; i < num_operations; i++) {
        // Perform matrix multiplication with itself
        result = npu_matrix_multiply(ctx->npu_handle, &tensor, &tensor, &tensor);
        ASSERT_SUCCESS(ctx, result, "Matrix multiplication %d failed", i);
        
        update_performance_metrics(ctx, tensor_size * matrix_size * 2);
    }
    
    // Stop profiling session
    npu_perf_profile_t profile;
    result = npu_stop_profiling(ctx->npu_handle, &profile);
    ASSERT_SUCCESS(ctx, result, "Failed to stop profiling");
    
    // Validate performance data
    ASSERT_TRUE(ctx, profile.cycles > 0, "Performance cycles should be greater than 0");
    ASSERT_TRUE(ctx, profile.operations >= num_operations, 
                "Operations count should be at least %d, got %lu", 
                num_operations, profile.operations);
    ASSERT_TRUE(ctx, profile.throughput_gops >= 0, 
                "Throughput should be non-negative, got %.3f", profile.throughput_gops);
    
    // Get comprehensive performance counters
    struct npu_performance_counters perf_counters;
    result = npu_get_comprehensive_perf_counters(ctx->npu_handle, &perf_counters);
    ASSERT_SUCCESS(ctx, result, "Failed to get comprehensive performance counters");
    
    // Reset performance counters
    result = npu_reset_performance_counters(ctx->npu_handle);
    ASSERT_SUCCESS(ctx, result, "Failed to reset performance counters");
    
    // Verify reset
    uint64_t cycles, operations;
    result = npu_get_performance_counters(ctx->npu_handle, &cycles, &operations);
    ASSERT_SUCCESS(ctx, result, "Failed to get performance counters after reset");
    ASSERT_EQ(ctx, 0, cycles, "Cycles should be 0 after reset");
    ASSERT_EQ(ctx, 0, operations, "Operations should be 0 after reset");
    
    if (g_verbose_output) {
        printf("  Performance: %.2f GOPS, %lu cycles, %lu operations\n",
               profile.throughput_gops, profile.cycles, profile.operations);
    }
    
    free_test_buffer(test_data);
    
    return TEST_RESULT_PASS;
}

/**
 * Test error handling and recovery
 */
test_result_t test_error_handling(test_context_t *ctx)
{
    // Test invalid tensor operations
    npu_tensor_t invalid_tensor = {0};
    invalid_tensor.data = NULL;
    invalid_tensor.size = 0;
    
    npu_tensor_t valid_tensor = npu_create_tensor(
        test_matrix_a, 1, 1, 4, 4, NPU_DTYPE_FLOAT32);
    
    // These operations should fail gracefully
    int result = npu_matrix_multiply(ctx->npu_handle, &invalid_tensor, &valid_tensor, &valid_tensor);
    ASSERT_TRUE(ctx, result != NPU_SUCCESS, "Invalid tensor operation should fail");
    
    result = npu_add(ctx->npu_handle, NULL, &valid_tensor, &valid_tensor);
    ASSERT_TRUE(ctx, result != NPU_SUCCESS, "NULL tensor operation should fail");
    
    // Test device health check
    uint32_t health_status;
    result = npu_check_device_health(ctx->npu_handle, &health_status);
    ASSERT_SUCCESS(ctx, result, "Device health check failed");
    
    // Test error information retrieval
    npu_error_info_t error_info;
    result = npu_get_error_info(ctx->npu_handle, &error_info);
    // This may or may not succeed depending on implementation
    
    // Test self-test functionality
    result = npu_self_test(ctx->npu_handle);
    ASSERT_SUCCESS(ctx, result, "NPU self-test failed");
    
    // Verify system is still functional after error tests
    uint32_t status;
    result = npu_get_status(ctx->npu_handle, &status);
    ASSERT_SUCCESS(ctx, result, "Failed to get status after error tests");
    ASSERT_TRUE(ctx, status & 0x1, "NPU not ready after error tests");
    
    return TEST_RESULT_PASS;
}

// =============================================================================
// Test Configuration and Registration
// =============================================================================

// Test configurations
static test_config_t test_configs[] = {
    {
        .name = "System Initialization",
        .category = TEST_CATEGORY_BASIC,
        .severity = TEST_SEVERITY_CRITICAL,
        .timeout_seconds = 10,
        .enable_performance_monitoring = false,
        .enable_stress_testing = false,
        .enable_concurrent_execution = false,
        .iterations = 1,
        .data_size_bytes = 0
    },
    {
        .name = "Memory Management",
        .category = TEST_CATEGORY_FUNCTIONAL,
        .severity = TEST_SEVERITY_HIGH,
        .timeout_seconds = 15,
        .enable_performance_monitoring = true,
        .enable_stress_testing = false,
        .enable_concurrent_execution = false,
        .iterations = 1,
        .data_size_bytes = 4096
    },
    {
        .name = "Matrix Multiplication E2E",
        .category = TEST_CATEGORY_FUNCTIONAL,
        .severity = TEST_SEVERITY_HIGH,
        .timeout_seconds = 20,
        .enable_performance_monitoring = true,
        .enable_stress_testing = false,
        .enable_concurrent_execution = false,
        .iterations = 1,
        .data_size_bytes = TEST_BUFFER_SIZE
    },
    {
        .name = "Tensor Operations",
        .category = TEST_CATEGORY_FUNCTIONAL,
        .severity = TEST_SEVERITY_MEDIUM,
        .timeout_seconds = 20,
        .enable_performance_monitoring = true,
        .enable_stress_testing = false,
        .enable_concurrent_execution = false,
        .iterations = 1,
        .data_size_bytes = 1024
    },
    {
        .name = "Convolution Operations",
        .category = TEST_CATEGORY_PERFORMANCE,
        .severity = TEST_SEVERITY_MEDIUM,
        .timeout_seconds = 30,
        .enable_performance_monitoring = true,
        .enable_stress_testing = false,
        .enable_concurrent_execution = false,
        .iterations = 1,
        .data_size_bytes = 32768
    },
    {
        .name = "Performance Monitoring",
        .category = TEST_CATEGORY_PERFORMANCE,
        .severity = TEST_SEVERITY_MEDIUM,
        .timeout_seconds = 25,
        .enable_performance_monitoring = true,
        .enable_stress_testing = false,
        .enable_concurrent_execution = false,
        .iterations = 1,
        .data_size_bytes = 4096
    },
    {
        .name = "Error Handling",
        .category = TEST_CATEGORY_RELIABILITY,
        .severity = TEST_SEVERITY_HIGH,
        .timeout_seconds = 15,
        .enable_performance_monitoring = false,
        .enable_stress_testing = false,
        .enable_concurrent_execution = false,
        .iterations = 1,
        .data_size_bytes = 256
    }
};

// Test functions array
static test_function_t test_functions[] = {
    test_system_initialization,
    test_memory_management,
    test_matrix_multiplication_e2e,
    test_tensor_operations,
    test_convolution_operations,
    test_performance_monitoring,
    test_error_handling
};

// Create end-to-end test suite
test_suite_t* create_e2e_test_suite(void)
{
    test_suite_t *suite = malloc(sizeof(test_suite_t));
    if (!suite) {
        return NULL;
    }
    
    strcpy(suite->name, "End-to-End Integration Tests");
    suite->tests = test_functions;
    suite->configs = test_configs;
    suite->test_count = sizeof(test_functions) / sizeof(test_functions[0]);
    suite->tests_passed = 0;
    suite->tests_failed = 0;
    suite->tests_skipped = 0;
    
    return suite;
}