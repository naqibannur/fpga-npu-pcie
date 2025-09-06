/**
 * FPGA NPU Performance Benchmark Framework
 * 
 * Comprehensive benchmarking suite for measuring throughput, latency,
 * power efficiency, and scalability of FPGA NPU operations
 */

#ifndef BENCHMARK_FRAMEWORK_H
#define BENCHMARK_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

// Include NPU library
#include "../../software/userspace/fpga_npu_lib.h"

// =============================================================================
// Configuration Constants
// =============================================================================

#define MAX_BENCHMARK_NAME_LENGTH 128
#define MAX_DESCRIPTION_LENGTH 256
#define MAX_WARMUP_ITERATIONS 10
#define DEFAULT_BENCHMARK_ITERATIONS 100
#define MAX_DATA_POINTS 10000
#define TIMESTAMP_BUFFER_SIZE 1024

// Color codes for terminal output
#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_BOLD    "\033[1m"

// =============================================================================
// Data Structures
// =============================================================================

/**
 * Benchmark operation types
 */
typedef enum {
    BENCHMARK_MATRIX_MULT,
    BENCHMARK_CONV2D,
    BENCHMARK_ELEMENT_ADD,
    BENCHMARK_ELEMENT_MUL,
    BENCHMARK_MEMORY_BANDWIDTH,
    BENCHMARK_LATENCY,
    BENCHMARK_CUSTOM
} benchmark_type_t;

/**
 * Benchmark data sizes
 */
typedef enum {
    SIZE_SMALL,    // 16x16 matrices, 1KB buffers
    SIZE_MEDIUM,   // 64x64 matrices, 16KB buffers
    SIZE_LARGE,    // 256x256 matrices, 256KB buffers
    SIZE_XLARGE,   // 1024x1024 matrices, 4MB buffers
    SIZE_CUSTOM    // User-defined size
} benchmark_size_t;

/**
 * Performance metrics structure
 */
typedef struct {
    double throughput_gops;      // Giga operations per second
    double throughput_gflops;    // Giga floating-point operations per second
    double latency_ms;           // Average latency in milliseconds
    double latency_min_ms;       // Minimum latency
    double latency_max_ms;       // Maximum latency
    double latency_std_ms;       // Standard deviation of latency
    double bandwidth_gbps;       // Memory bandwidth in GB/s
    double power_watts;          // Power consumption in watts
    double efficiency_gops_watt; // Energy efficiency in GOPS/Watt
    double cpu_utilization;      // CPU utilization percentage
    double memory_utilization;   // Memory utilization percentage
    uint64_t operations_count;   // Total operations performed
    uint64_t data_transferred;   // Total data transferred in bytes
    double duration_seconds;     // Total benchmark duration
} performance_metrics_t;

/**
 * Benchmark configuration
 */
typedef struct {
    char name[MAX_BENCHMARK_NAME_LENGTH];
    char description[MAX_DESCRIPTION_LENGTH];
    benchmark_type_t type;
    benchmark_size_t size;
    uint32_t custom_size_x;      // Custom dimension X
    uint32_t custom_size_y;      // Custom dimension Y
    uint32_t custom_size_z;      // Custom dimension Z
    uint32_t iterations;         // Number of iterations
    uint32_t warmup_iterations;  // Warmup iterations
    bool enable_power_monitoring;
    bool enable_thermal_monitoring;
    bool enable_detailed_timing;
    bool enable_memory_profiling;
    uint32_t thread_count;       // For multi-threaded benchmarks
    double target_duration_sec;  // Target benchmark duration
} benchmark_config_t;

/**
 * Benchmark result structure
 */
typedef struct {
    benchmark_config_t config;
    performance_metrics_t metrics;
    double *latency_samples;     // Array of individual latency measurements
    uint32_t sample_count;       // Number of latency samples
    struct timespec start_time;  // Benchmark start timestamp
    struct timespec end_time;    // Benchmark end timestamp
    bool success;                // Whether benchmark completed successfully
    char error_message[256];     // Error message if failed
} benchmark_result_t;

/**
 * Benchmark context for execution
 */
typedef struct {
    npu_handle_t npu_handle;
    benchmark_config_t config;
    benchmark_result_t *result;
    void *test_data_a;           // Test data buffer A
    void *test_data_b;           // Test data buffer B
    void *result_data;           // Result data buffer
    size_t data_size;            // Size of each data buffer
    npu_buffer_handle_t buffer_a; // NPU buffer handle A
    npu_buffer_handle_t buffer_b; // NPU buffer handle B
    npu_buffer_handle_t buffer_result; // NPU result buffer handle
    bool stop_requested;         // Stop flag for long-running benchmarks
    pthread_mutex_t mutex;       // Thread synchronization
} benchmark_context_t;

/**
 * Benchmark function pointer type
 */
typedef int (*benchmark_function_t)(benchmark_context_t *ctx);

/**
 * Benchmark suite structure
 */
typedef struct {
    char name[MAX_BENCHMARK_NAME_LENGTH];
    benchmark_config_t *configs;
    benchmark_function_t *functions;
    benchmark_result_t *results;
    size_t benchmark_count;
    performance_metrics_t overall_metrics;
} benchmark_suite_t;

// =============================================================================
// Core Framework Functions
// =============================================================================

/**
 * Initialize benchmark framework
 * @return 0 on success, negative on error
 */
int benchmark_framework_init(void);

/**
 * Cleanup benchmark framework
 */
void benchmark_framework_cleanup(void);

/**
 * Create benchmark context
 * @param config Benchmark configuration
 * @return Pointer to context, NULL on error
 */
benchmark_context_t* create_benchmark_context(const benchmark_config_t *config);

/**
 * Destroy benchmark context and free resources
 * @param ctx Benchmark context to destroy
 */
void destroy_benchmark_context(benchmark_context_t *ctx);

/**
 * Execute single benchmark
 * @param benchmark_func Benchmark function to execute
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int execute_benchmark(benchmark_function_t benchmark_func, benchmark_context_t *ctx);

/**
 * Execute benchmark suite
 * @param suite Benchmark suite to execute
 * @return 0 on success, negative on error
 */
int execute_benchmark_suite(benchmark_suite_t *suite);

// =============================================================================
// Performance Measurement Functions
// =============================================================================

/**
 * Get high-resolution timestamp
 * @return Timestamp in nanoseconds
 */
uint64_t get_timestamp_ns(void);

/**
 * Calculate elapsed time between timestamps
 * @param start Start timestamp
 * @param end End timestamp
 * @return Elapsed time in seconds
 */
double calculate_elapsed_time(struct timespec start, struct timespec end);

/**
 * Start performance monitoring
 * @param ctx Benchmark context
 */
void start_performance_monitoring(benchmark_context_t *ctx);

/**
 * Stop performance monitoring and calculate metrics
 * @param ctx Benchmark context
 */
void stop_performance_monitoring(benchmark_context_t *ctx);

/**
 * Record single latency measurement
 * @param ctx Benchmark context
 * @param latency_ms Latency in milliseconds
 */
void record_latency_sample(benchmark_context_t *ctx, double latency_ms);

/**
 * Calculate statistics from latency samples
 * @param samples Array of latency samples
 * @param count Number of samples
 * @param mean Pointer to store mean value
 * @param std_dev Pointer to store standard deviation
 * @param min_val Pointer to store minimum value
 * @param max_val Pointer to store maximum value
 */
void calculate_latency_statistics(double *samples, uint32_t count,
                                double *mean, double *std_dev,
                                double *min_val, double *max_val);

// =============================================================================
// Throughput Benchmarks
// =============================================================================

/**
 * Matrix multiplication throughput benchmark
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int benchmark_matrix_multiplication(benchmark_context_t *ctx);

/**
 * 2D convolution throughput benchmark
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int benchmark_convolution_2d(benchmark_context_t *ctx);

/**
 * Element-wise operations throughput benchmark
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int benchmark_element_wise_operations(benchmark_context_t *ctx);

/**
 * Memory bandwidth benchmark
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int benchmark_memory_bandwidth(benchmark_context_t *ctx);

// =============================================================================
// Latency Benchmarks
// =============================================================================

/**
 * Single operation latency benchmark
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int benchmark_single_operation_latency(benchmark_context_t *ctx);

/**
 * Batch operation latency benchmark
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int benchmark_batch_operation_latency(benchmark_context_t *ctx);

/**
 * Memory access latency benchmark
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int benchmark_memory_access_latency(benchmark_context_t *ctx);

// =============================================================================
// Scalability Benchmarks
// =============================================================================

/**
 * Multi-threaded scalability benchmark
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int benchmark_multithreaded_scalability(benchmark_context_t *ctx);

/**
 * Data size scalability benchmark
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int benchmark_data_size_scalability(benchmark_context_t *ctx);

/**
 * Concurrent workload benchmark
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int benchmark_concurrent_workload(benchmark_context_t *ctx);

// =============================================================================
// Power and Efficiency Benchmarks
// =============================================================================

/**
 * Power consumption benchmark
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int benchmark_power_consumption(benchmark_context_t *ctx);

/**
 * Energy efficiency benchmark
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int benchmark_energy_efficiency(benchmark_context_t *ctx);

/**
 * Thermal behavior benchmark
 * @param ctx Benchmark context
 * @return 0 on success, negative on error
 */
int benchmark_thermal_behavior(benchmark_context_t *ctx);

// =============================================================================
// Data Management Functions
// =============================================================================

/**
 * Get matrix dimensions for benchmark size
 * @param size Benchmark size enum
 * @param rows Pointer to store row count
 * @param cols Pointer to store column count
 */
void get_matrix_dimensions(benchmark_size_t size, uint32_t *rows, uint32_t *cols);

/**
 * Get buffer size for benchmark size
 * @param size Benchmark size enum
 * @return Buffer size in bytes
 */
size_t get_buffer_size(benchmark_size_t size);

/**
 * Initialize test data with patterns
 * @param buffer Buffer to initialize
 * @param size Buffer size in bytes
 * @param pattern Data pattern type
 */
void initialize_benchmark_data(void *buffer, size_t size, int pattern);

/**
 * Validate benchmark results
 * @param expected Expected result buffer
 * @param actual Actual result buffer
 * @param size Buffer size in bytes
 * @param tolerance Floating-point tolerance
 * @return true if results are valid, false otherwise
 */
bool validate_benchmark_results(const void *expected, const void *actual, 
                               size_t size, double tolerance);

// =============================================================================
// Reporting and Analysis Functions
// =============================================================================

/**
 * Print benchmark configuration
 * @param config Benchmark configuration to print
 */
void print_benchmark_config(const benchmark_config_t *config);

/**
 * Print benchmark results
 * @param result Benchmark result to print
 */
void print_benchmark_result(const benchmark_result_t *result);

/**
 * Print performance metrics
 * @param metrics Performance metrics to print
 */
void print_performance_metrics(const performance_metrics_t *metrics);

/**
 * Generate CSV report
 * @param filename Output CSV filename
 * @param results Array of benchmark results
 * @param count Number of results
 * @return 0 on success, negative on error
 */
int generate_csv_report(const char *filename, benchmark_result_t *results, size_t count);

/**
 * Generate HTML report
 * @param filename Output HTML filename
 * @param suite Benchmark suite results
 * @return 0 on success, negative on error
 */
int generate_html_report(const char *filename, const benchmark_suite_t *suite);

/**
 * Generate JSON report
 * @param filename Output JSON filename
 * @param suite Benchmark suite results
 * @return 0 on success, negative on error
 */
int generate_json_report(const char *filename, const benchmark_suite_t *suite);

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Calculate operations count for given benchmark type and size
 * @param type Benchmark type
 * @param size_x Dimension X
 * @param size_y Dimension Y
 * @param size_z Dimension Z (for 3D operations)
 * @return Number of operations
 */
uint64_t calculate_operations_count(benchmark_type_t type, uint32_t size_x, 
                                   uint32_t size_y, uint32_t size_z);

/**
 * Convert benchmark type to string
 * @param type Benchmark type
 * @return String representation
 */
const char* benchmark_type_to_string(benchmark_type_t type);

/**
 * Convert benchmark size to string
 * @param size Benchmark size
 * @return String representation
 */
const char* benchmark_size_to_string(benchmark_size_t size);

/**
 * Get system information for benchmark context
 * @param info Buffer to store system information
 * @param size Buffer size
 * @return 0 on success, negative on error
 */
int get_system_information(char *info, size_t size);

/**
 * Monitor system resources during benchmark
 * @param ctx Benchmark context
 */
void monitor_system_resources(benchmark_context_t *ctx);

// =============================================================================
// Configuration Helpers
// =============================================================================

/**
 * Create default benchmark configuration
 * @param type Benchmark type
 * @param size Benchmark size
 * @return Default configuration
 */
benchmark_config_t create_default_config(benchmark_type_t type, benchmark_size_t size);

/**
 * Create throughput benchmark suite
 * @return Pointer to benchmark suite, NULL on error
 */
benchmark_suite_t* create_throughput_benchmark_suite(void);

/**
 * Create latency benchmark suite
 * @return Pointer to benchmark suite, NULL on error
 */
benchmark_suite_t* create_latency_benchmark_suite(void);

/**
 * Create scalability benchmark suite
 * @return Pointer to benchmark suite, NULL on error
 */
benchmark_suite_t* create_scalability_benchmark_suite(void);

/**
 * Create power efficiency benchmark suite
 * @return Pointer to benchmark suite, NULL on error
 */
benchmark_suite_t* create_power_benchmark_suite(void);

/**
 * Destroy benchmark suite and free resources
 * @param suite Benchmark suite to destroy
 */
void destroy_benchmark_suite(benchmark_suite_t *suite);

// =============================================================================
// Global Variables (extern declarations)
// =============================================================================

extern bool g_benchmark_verbose;
extern bool g_benchmark_stop_on_error;
extern FILE *g_benchmark_log_file;

#endif // BENCHMARK_FRAMEWORK_H