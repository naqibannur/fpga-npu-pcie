/**
 * Main Unit Test Runner for FPGA NPU Library
 * 
 * Runs all unit test suites and reports results.
 */

#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>

// External test suite functions
extern void run_core_tests(void);
extern void run_memory_tests(void);
extern void run_tensor_tests(void);

/**
 * Print test banner
 */
void print_banner(void)
{
    printf(COLOR_BLUE);
    printf("=========================================\n");
    printf("      FPGA NPU Library Unit Tests       \n");
    printf("=========================================\n");
    printf(COLOR_RESET);
    printf("Testing core functionality, memory management,\n");
    printf("and tensor operations...\n\n");
}

/**
 * Print system information
 */
void print_system_info(void)
{
    printf(COLOR_YELLOW "System Information:" COLOR_RESET "\n");
    printf("- Compiler: %s\n", __VERSION__);
    printf("- Build date: %s %s\n", __DATE__, __TIME__);
    printf("- Architecture: ");
    
#ifdef __x86_64__
    printf("x86_64\n");
#elif defined(__aarch64__)
    printf("ARM64\n");
#elif defined(__arm__)
    printf("ARM32\n");
#else
    printf("Unknown\n");
#endif

    printf("- Pointer size: %zu bytes\n", sizeof(void*));
    printf("- Float size: %zu bytes\n", sizeof(float));
    printf("- Double size: %zu bytes\n\n", sizeof(double));
}

/**
 * Run performance tests
 */
void run_performance_tests(void)
{
    TEST_SUITE("Performance Tests");
    
    // Test large tensor creation performance
    TEST_CASE("large tensor creation");
    
    clock_t start = clock();
    for (int i = 0; i < 1000; i++) {
        float* data = malloc(1024 * sizeof(float));
        if (data) {
            npu_tensor_t tensor = npu_create_tensor(data, 1, 1, 32, 32, NPU_DTYPE_FLOAT32);
            (void)tensor; // Suppress unused variable warning
            free(data);
        }
    }
    clock_t end = clock();
    
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf(COLOR_GREEN "PASS" COLOR_RESET " (%.3f seconds for 1000 tensors)\n", elapsed);
    test_count++;
    test_passed++;
    
    // Test throughput calculation performance
    TEST_CASE("throughput calculations");
    
    start = clock();
    for (int i = 0; i < 100000; i++) {
        float throughput = npu_calculate_throughput(1000000ULL + i, 1000000000ULL + i);
        (void)throughput;
    }
    end = clock();
    
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf(COLOR_GREEN "PASS" COLOR_RESET " (%.3f seconds for 100k calculations)\n", elapsed);
    test_count++;
    test_passed++;
}

/**
 * Run stress tests
 */
void run_stress_tests(void)
{
    TEST_SUITE("Stress Tests");
    
    TEST_CASE("multiple init/cleanup cycles");
    
    // Test multiple initialization and cleanup cycles
    for (int i = 0; i < 100; i++) {
        mock_reset();
        npu_handle_t handle = npu_init();
        if (handle) {
            npu_cleanup(handle);
        }
    }
    
    printf(COLOR_GREEN "PASS" COLOR_RESET " (100 init/cleanup cycles)\n");
    test_count++;
    test_passed++;
    
    TEST_CASE("error string stress test");
    
    // Test error strings with many different codes
    for (int i = -100; i < 100; i++) {
        const char* error_str = npu_error_string(i);
        ASSERT_NOT_NULL(error_str);
        ASSERT_TRUE(strlen(error_str) > 0);
    }
    
    printf(COLOR_GREEN "PASS" COLOR_RESET "\n");
    test_count++;
    test_passed++;
}

/**
 * Run edge case tests
 */
void run_edge_case_tests(void)
{
    TEST_SUITE("Edge Case Tests");
    
    TEST_CASE("extreme tensor dimensions");
    
    // Test with very small tensors
    float small_data[1] = {42.0f};
    npu_tensor_t small_tensor = npu_create_tensor(small_data, 1, 1, 1, 1, NPU_DTYPE_FLOAT32);
    ASSERT_EQ(small_tensor.size, sizeof(float));
    
    // Test with zero dimensions (edge case)
    float zero_data[1] = {0.0f};
    npu_tensor_t zero_tensor = npu_create_tensor(zero_data, 0, 1, 1, 1, NPU_DTYPE_FLOAT32);
    ASSERT_EQ(zero_tensor.size, 0);
    
    printf(COLOR_GREEN "PASS" COLOR_RESET "\n");
    test_count++;
    test_passed++;
    
    TEST_CASE("throughput edge cases");
    
    // Test with zero operations
    float throughput = npu_calculate_throughput(0, 1000000000ULL);
    ASSERT_FLOAT_EQ(0.0f, throughput, 0.001f);
    
    // Test with very large numbers
    throughput = npu_calculate_throughput(UINT64_MAX, 1000000000ULL);
    ASSERT_TRUE(throughput > 0.0f);
    
    printf(COLOR_GREEN "PASS" COLOR_RESET "\n");
    test_count++;
    test_passed++;
}

/**
 * Main test function
 */
int main(int argc, char* argv[])
{
    (void)argc; (void)argv; // Suppress unused parameter warnings
    
    // Initialize test framework
    TEST_INIT();
    
    // Print information
    print_banner();
    print_system_info();
    
    // Run all test suites
    run_core_tests();
    run_memory_tests();
    run_tensor_tests();
    run_performance_tests();
    run_stress_tests();
    run_edge_case_tests();
    
    // Print final summary and exit
    TEST_EXIT();
    
    return 0; // Never reached due to TEST_EXIT()
}