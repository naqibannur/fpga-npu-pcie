/**
 * Integration Test Framework Header
 * 
 * Comprehensive framework for end-to-end system testing of FPGA NPU
 * including hardware-software integration, performance validation,
 * and system reliability testing.
 */

#ifndef INTEGRATION_TEST_FRAMEWORK_H
#define INTEGRATION_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

// Include NPU library
#include "../../software/userspace/fpga_npu_lib.h"

// Test framework configuration
#define MAX_TEST_NAME_LENGTH 128
#define MAX_ERROR_MESSAGE_LENGTH 512
#define MAX_CONCURRENT_TESTS 16
#define DEFAULT_TEST_TIMEOUT 30 // seconds

// Color codes for output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"

// Test result enumeration
typedef enum {
    TEST_RESULT_PASS,
    TEST_RESULT_FAIL,
    TEST_RESULT_TIMEOUT,
    TEST_RESULT_SKIP,
    TEST_RESULT_ERROR
} test_result_t;

// Test severity levels
typedef enum {
    TEST_SEVERITY_LOW,
    TEST_SEVERITY_MEDIUM,
    TEST_SEVERITY_HIGH,
    TEST_SEVERITY_CRITICAL
} test_severity_t;

// Test category enumeration
typedef enum {
    TEST_CATEGORY_BASIC,
    TEST_CATEGORY_FUNCTIONAL,
    TEST_CATEGORY_PERFORMANCE,
    TEST_CATEGORY_STRESS,
    TEST_CATEGORY_RELIABILITY,
    TEST_CATEGORY_COMPATIBILITY
} test_category_t;

// Performance metrics structure
typedef struct {
    double throughput_gops;      // Giga operations per second
    double latency_ms;           // Average latency in milliseconds
    double power_watts;          // Power consumption
    double efficiency_gops_watt; // Energy efficiency
    uint64_t operations_count;   // Total operations executed
    uint64_t errors_count;       // Number of errors encountered
    double duration_seconds;     // Test duration
} performance_metrics_t;

// Test configuration structure
typedef struct {
    char name[MAX_TEST_NAME_LENGTH];
    test_category_t category;
    test_severity_t severity;
    uint32_t timeout_seconds;
    bool enable_performance_monitoring;
    bool enable_stress_testing;
    bool enable_concurrent_execution;
    uint32_t iterations;
    uint32_t data_size_bytes;
} test_config_t;

// Test context structure
typedef struct {
    test_config_t config;
    npu_handle_t npu_handle;
    test_result_t result;
    char error_message[MAX_ERROR_MESSAGE_LENGTH];
    performance_metrics_t performance;
    struct timespec start_time;
    struct timespec end_time;
    pthread_t thread_id;
    bool is_running;
    void *test_data;
    size_t test_data_size;
} test_context_t;

// Test function pointer type
typedef test_result_t (*test_function_t)(test_context_t *ctx);

// Test suite structure
typedef struct {
    char name[MAX_TEST_NAME_LENGTH];
    test_function_t *tests;
    test_config_t *configs;
    size_t test_count;
    size_t tests_passed;
    size_t tests_failed;
    size_t tests_skipped;
    performance_metrics_t overall_performance;
} test_suite_t;

// Global test statistics
typedef struct {
    size_t total_tests;
    size_t tests_passed;
    size_t tests_failed;
    size_t tests_skipped;
    size_t tests_timeout;
    double total_duration;
    performance_metrics_t overall_performance;
} test_statistics_t;

// =============================================================================
// Core Test Framework Functions
// =============================================================================

/**
 * Initialize the integration test framework
 * @return 0 on success, negative on error
 */
int integration_test_init(void);

/**
 * Cleanup integration test framework
 */
void integration_test_cleanup(void);

/**
 * Create a new test context
 * @param config Test configuration
 * @return Pointer to test context, NULL on error
 */
test_context_t* create_test_context(const test_config_t *config);

/**
 * Destroy test context and free resources
 * @param ctx Test context to destroy
 */
void destroy_test_context(test_context_t *ctx);

/**
 * Execute a single test
 * @param test_func Test function to execute
 * @param ctx Test context
 * @return Test result
 */
test_result_t execute_test(test_function_t test_func, test_context_t *ctx);

/**
 * Execute test with timeout
 * @param test_func Test function to execute
 * @param ctx Test context
 * @return Test result
 */
test_result_t execute_test_with_timeout(test_function_t test_func, test_context_t *ctx);

/**
 * Execute test suite
 * @param suite Test suite to execute
 * @return 0 on success, negative on error
 */
int execute_test_suite(test_suite_t *suite);

// =============================================================================
// Assertion and Validation Macros
// =============================================================================

#define ASSERT_TRUE(ctx, condition, message, ...) \
    do { \
        if (!(condition)) { \
            snprintf((ctx)->error_message, MAX_ERROR_MESSAGE_LENGTH, \
                    "ASSERTION FAILED: " message, ##__VA_ARGS__); \
            printf(COLOR_RED "[FAIL]" COLOR_RESET " %s: %s\n", \
                   (ctx)->config.name, (ctx)->error_message); \
            return TEST_RESULT_FAIL; \
        } \
    } while(0)

#define ASSERT_FALSE(ctx, condition, message, ...) \
    ASSERT_TRUE(ctx, !(condition), message, ##__VA_ARGS__)

#define ASSERT_EQ(ctx, expected, actual, message, ...) \
    do { \
        if ((expected) != (actual)) { \
            snprintf((ctx)->error_message, MAX_ERROR_MESSAGE_LENGTH, \
                    "ASSERTION FAILED: Expected %ld, got %ld - " message, \
                    (long)(expected), (long)(actual), ##__VA_ARGS__); \
            printf(COLOR_RED "[FAIL]" COLOR_RESET " %s: %s\n", \
                   (ctx)->config.name, (ctx)->error_message); \
            return TEST_RESULT_FAIL; \
        } \
    } while(0)

#define ASSERT_NEQ(ctx, not_expected, actual, message, ...) \
    do { \
        if ((not_expected) == (actual)) { \
            snprintf((ctx)->error_message, MAX_ERROR_MESSAGE_LENGTH, \
                    "ASSERTION FAILED: Values should not be equal (%ld) - " message, \
                    (long)(actual), ##__VA_ARGS__); \
            printf(COLOR_RED "[FAIL]" COLOR_RESET " %s: %s\n", \
                   (ctx)->config.name, (ctx)->error_message); \
            return TEST_RESULT_FAIL; \
        } \
    } while(0)

#define ASSERT_NULL(ctx, ptr, message, ...) \
    ASSERT_TRUE(ctx, (ptr) == NULL, message, ##__VA_ARGS__)

#define ASSERT_NOT_NULL(ctx, ptr, message, ...) \
    ASSERT_TRUE(ctx, (ptr) != NULL, message, ##__VA_ARGS__)

#define ASSERT_SUCCESS(ctx, result, message, ...) \
    ASSERT_EQ(ctx, NPU_SUCCESS, result, message, ##__VA_ARGS__)

#define ASSERT_FLOAT_EQ(ctx, expected, actual, tolerance, message, ...) \
    do { \
        double diff = fabs((double)(expected) - (double)(actual)); \
        if (diff > (tolerance)) { \
            snprintf((ctx)->error_message, MAX_ERROR_MESSAGE_LENGTH, \
                    "ASSERTION FAILED: Expected %.6f, got %.6f (diff %.6f > %.6f) - " message, \
                    (double)(expected), (double)(actual), diff, (double)(tolerance), ##__VA_ARGS__); \
            printf(COLOR_RED "[FAIL]" COLOR_RESET " %s: %s\n", \
                   (ctx)->config.name, (ctx)->error_message); \
            return TEST_RESULT_FAIL; \
        } \
    } while(0)

// =============================================================================
// Performance Monitoring Functions
// =============================================================================

/**
 * Start performance monitoring for a test
 * @param ctx Test context
 */
void start_performance_monitoring(test_context_t *ctx);

/**
 * Stop performance monitoring and calculate metrics
 * @param ctx Test context
 */
void stop_performance_monitoring(test_context_t *ctx);

/**
 * Update performance metrics during test execution
 * @param ctx Test context
 * @param operations_performed Number of operations performed
 */
void update_performance_metrics(test_context_t *ctx, uint64_t operations_performed);

/**
 * Calculate throughput in GOPS
 * @param operations Number of operations
 * @param duration_seconds Duration in seconds
 * @return Throughput in GOPS
 */
double calculate_throughput_gops(uint64_t operations, double duration_seconds);

/**
 * Calculate energy efficiency
 * @param throughput_gops Throughput in GOPS
 * @param power_watts Power consumption in watts
 * @return Efficiency in GOPS/Watt
 */
double calculate_efficiency(double throughput_gops, double power_watts);

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Get current timestamp in nanoseconds
 * @return Timestamp in nanoseconds
 */
uint64_t get_timestamp_ns(void);

/**
 * Calculate time difference between two timestamps
 * @param start Start timestamp
 * @param end End timestamp
 * @return Duration in seconds
 */
double calculate_duration_seconds(struct timespec start, struct timespec end);

/**
 * Generate random test data
 * @param buffer Buffer to fill with random data
 * @param size Size of buffer in bytes
 */
void generate_random_data(void *buffer, size_t size);

/**
 * Compare two data buffers
 * @param buffer1 First buffer
 * @param buffer2 Second buffer
 * @param size Size to compare
 * @return true if buffers are identical, false otherwise
 */
bool compare_buffers(const void *buffer1, const void *buffer2, size_t size);

/**
 * Print test progress
 * @param current Current test number
 * @param total Total number of tests
 * @param test_name Current test name
 */
void print_test_progress(size_t current, size_t total, const char *test_name);

/**
 * Print performance summary
 * @param metrics Performance metrics to print
 */
void print_performance_summary(const performance_metrics_t *metrics);

/**
 * Print test statistics
 * @param stats Test statistics to print
 */
void print_test_statistics(const test_statistics_t *stats);

// =============================================================================
// Test Data Management
// =============================================================================

/**
 * Allocate aligned test data buffer
 * @param size Size of buffer in bytes
 * @param alignment Memory alignment requirement
 * @return Pointer to allocated buffer, NULL on error
 */
void* allocate_test_buffer(size_t size, size_t alignment);

/**
 * Free test data buffer
 * @param buffer Buffer to free
 */
void free_test_buffer(void *buffer);

/**
 * Initialize test data with specific pattern
 * @param buffer Buffer to initialize
 * @param size Size of buffer
 * @param pattern Data pattern type
 */
void initialize_test_data(void *buffer, size_t size, int pattern);

// Data pattern types
#define PATTERN_ZEROS     0
#define PATTERN_ONES      1
#define PATTERN_RANDOM    2
#define PATTERN_SEQUENCE  3
#define PATTERN_CHECKERBOARD 4

// =============================================================================
// System Health Monitoring
// =============================================================================

/**
 * Check system health before test execution
 * @return 0 if system is healthy, negative on issues
 */
int check_system_health(void);

/**
 * Monitor system resources during test
 * @param ctx Test context
 */
void monitor_system_resources(test_context_t *ctx);

/**
 * Check for thermal throttling
 * @return true if thermal throttling detected
 */
bool check_thermal_throttling(void);

/**
 * Get system memory usage
 * @param total_mb Total system memory in MB
 * @param available_mb Available memory in MB
 * @return 0 on success, negative on error
 */
int get_memory_usage(uint64_t *total_mb, uint64_t *available_mb);

// =============================================================================
// Logging and Reporting
// =============================================================================

/**
 * Initialize test logging
 * @param log_file Log file path, NULL for stdout only
 * @return 0 on success, negative on error
 */
int init_test_logging(const char *log_file);

/**
 * Log test message with timestamp
 * @param level Log level
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void log_test_message(int level, const char *format, ...);

/**
 * Generate HTML test report
 * @param output_file Output HTML file path
 * @param stats Test statistics
 * @return 0 on success, negative on error
 */
int generate_html_report(const char *output_file, const test_statistics_t *stats);

/**
 * Generate JSON test report
 * @param output_file Output JSON file path
 * @param stats Test statistics
 * @return 0 on success, negative on error
 */
int generate_json_report(const char *output_file, const test_statistics_t *stats);

// Log levels
#define LOG_ERROR   0
#define LOG_WARNING 1
#define LOG_INFO    2
#define LOG_DEBUG   3

// =============================================================================
// Signal Handling and Cleanup
// =============================================================================

/**
 * Setup signal handlers for graceful shutdown
 */
void setup_signal_handlers(void);

/**
 * Emergency cleanup function
 */
void emergency_cleanup(void);

/**
 * Register cleanup function
 * @param cleanup_func Function to call during cleanup
 */
void register_cleanup_function(void (*cleanup_func)(void));

// =============================================================================
// Global Variables (extern declarations)
// =============================================================================

extern test_statistics_t g_test_stats;
extern bool g_verbose_output;
extern bool g_stop_on_first_failure;
extern FILE *g_log_file;

#endif // INTEGRATION_TEST_FRAMEWORK_H