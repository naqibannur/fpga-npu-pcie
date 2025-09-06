/**
 * Simple Unit Test Framework for FPGA NPU Library
 * 
 * Provides basic assertion macros and test organization.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// Test statistics
extern int test_count;
extern int test_passed;
extern int test_failed;

// Color codes for output
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"

// Test macros
#define TEST_INIT() do { \
    test_count = 0; \
    test_passed = 0; \
    test_failed = 0; \
    printf(COLOR_BLUE "=== Starting Unit Tests ===" COLOR_RESET "\n"); \
} while(0)

#define TEST_SUITE(name) \
    printf(COLOR_YELLOW "\n--- Test Suite: %s ---" COLOR_RESET "\n", name)

#define TEST_CASE(name) \
    printf("Testing %s... ", name); \
    fflush(stdout); \
    test_count++

#define ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        printf(COLOR_RED "FAIL" COLOR_RESET " - Assertion failed: %s (line %d)\n", #condition, __LINE__); \
        test_failed++; \
        return false; \
    } \
} while(0)

#define ASSERT_FALSE(condition) do { \
    if ((condition)) { \
        printf(COLOR_RED "FAIL" COLOR_RESET " - Assertion failed: !(%s) (line %d)\n", #condition, __LINE__); \
        test_failed++; \
        return false; \
    } \
} while(0)

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf(COLOR_RED "FAIL" COLOR_RESET " - Expected %ld, got %ld (line %d)\n", (long)(expected), (long)(actual), __LINE__); \
        test_failed++; \
        return false; \
    } \
} while(0)

#define ASSERT_NEQ(not_expected, actual) do { \
    if ((not_expected) == (actual)) { \
        printf(COLOR_RED "FAIL" COLOR_RESET " - Expected not %ld, but got %ld (line %d)\n", (long)(not_expected), (long)(actual), __LINE__); \
        test_failed++; \
        return false; \
    } \
} while(0)

#define ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        printf(COLOR_RED "FAIL" COLOR_RESET " - Expected NULL, got %p (line %d)\n", (ptr), __LINE__); \
        test_failed++; \
        return false; \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf(COLOR_RED "FAIL" COLOR_RESET " - Expected not NULL, got NULL (line %d)\n", __LINE__); \
        test_failed++; \
        return false; \
    } \
} while(0)

#define ASSERT_STR_EQ(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf(COLOR_RED "FAIL" COLOR_RESET " - Expected \"%s\", got \"%s\" (line %d)\n", (expected), (actual), __LINE__); \
        test_failed++; \
        return false; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(expected, actual, tolerance) do { \
    float diff = (float)(expected) - (float)(actual); \
    if (diff < 0) diff = -diff; \
    if (diff > (tolerance)) { \
        printf(COLOR_RED "FAIL" COLOR_RESET " - Expected %.6f, got %.6f (diff %.6f > %.6f) (line %d)\n", \
               (float)(expected), (float)(actual), diff, (float)(tolerance), __LINE__); \
        test_failed++; \
        return false; \
    } \
} while(0)

#define TEST_PASS() do { \
    printf(COLOR_GREEN "PASS" COLOR_RESET "\n"); \
    test_passed++; \
    return true; \
} while(0)

#define RUN_TEST(test_func) do { \
    if ((test_func)()) { \
        /* Test passed */ \
    } else { \
        /* Test failed - already handled in macros */ \
    } \
} while(0)

#define TEST_SUMMARY() do { \
    printf(COLOR_BLUE "\n=== Test Summary ===" COLOR_RESET "\n"); \
    printf("Total tests: %d\n", test_count); \
    printf(COLOR_GREEN "Passed: %d" COLOR_RESET "\n", test_passed); \
    if (test_failed > 0) { \
        printf(COLOR_RED "Failed: %d" COLOR_RESET "\n", test_failed); \
    } else { \
        printf("Failed: 0\n"); \
    } \
    printf("Success rate: %.1f%%\n", test_count > 0 ? (100.0 * test_passed / test_count) : 0.0); \
    if (test_failed == 0) { \
        printf(COLOR_GREEN "All tests passed!" COLOR_RESET "\n"); \
    } \
} while(0)

#define TEST_EXIT() do { \
    TEST_SUMMARY(); \
    exit(test_failed > 0 ? 1 : 0); \
} while(0)

// Mock framework for device functions
typedef struct {
    bool init_should_fail;
    bool ioctl_should_fail;
    bool mmap_should_fail;
    int mock_fd;
    uint32_t mock_status;
    uint64_t mock_cycles;
    uint64_t mock_operations;
} mock_device_t;

extern mock_device_t mock_device;

// Mock function declarations
void mock_reset(void);
void mock_set_init_fail(bool should_fail);
void mock_set_ioctl_fail(bool should_fail);
void mock_set_status(uint32_t status);
void mock_set_performance_counters(uint64_t cycles, uint64_t operations);

#endif // TEST_FRAMEWORK_H