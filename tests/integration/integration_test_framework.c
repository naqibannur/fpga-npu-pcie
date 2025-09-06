/**
 * Integration Test Framework Implementation
 * 
 * Core implementation of the integration testing framework for FPGA NPU
 */

#include "integration_test_framework.h"
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

// Global variables
test_statistics_t g_test_stats = {0};
bool g_verbose_output = false;
bool g_stop_on_first_failure = false;
FILE *g_log_file = NULL;

// Static variables
static bool framework_initialized = false;
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
static void (*cleanup_functions[16])(void) = {NULL};
static size_t cleanup_function_count = 0;

// =============================================================================
// Core Framework Functions
// =============================================================================

int integration_test_init(void)
{
    if (framework_initialized) {
        return 0; // Already initialized
    }
    
    // Initialize global statistics
    memset(&g_test_stats, 0, sizeof(g_test_stats));
    
    // Setup signal handlers
    setup_signal_handlers();
    
    // Initialize random seed
    srand(time(NULL));
    
    framework_initialized = true;
    
    printf(COLOR_CYAN "Integration Test Framework Initialized" COLOR_RESET "\n");
    return 0;
}

void integration_test_cleanup(void)
{
    if (!framework_initialized) {
        return;
    }
    
    // Call registered cleanup functions
    for (size_t i = 0; i < cleanup_function_count; i++) {
        if (cleanup_functions[i]) {
            cleanup_functions[i]();
        }
    }
    
    // Close log file if open
    if (g_log_file && g_log_file != stdout && g_log_file != stderr) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    
    framework_initialized = false;
    
    printf(COLOR_CYAN "Integration Test Framework Cleanup Complete" COLOR_RESET "\n");
}

test_context_t* create_test_context(const test_config_t *config)
{
    if (!config) {
        return NULL;
    }
    
    test_context_t *ctx = calloc(1, sizeof(test_context_t));
    if (!ctx) {
        return NULL;
    }
    
    // Copy configuration
    memcpy(&ctx->config, config, sizeof(test_config_t));
    
    // Initialize context
    ctx->result = TEST_RESULT_PASS;
    ctx->is_running = false;
    ctx->npu_handle = NULL;
    ctx->test_data = NULL;
    ctx->test_data_size = 0;
    
    // Initialize performance metrics
    memset(&ctx->performance, 0, sizeof(performance_metrics_t));
    
    return ctx;
}

void destroy_test_context(test_context_t *ctx)
{
    if (!ctx) {
        return;
    }
    
    // Cleanup NPU handle if allocated
    if (ctx->npu_handle) {
        npu_cleanup(ctx->npu_handle);
        ctx->npu_handle = NULL;
    }
    
    // Free test data if allocated
    if (ctx->test_data) {
        free_test_buffer(ctx->test_data);
        ctx->test_data = NULL;
    }
    
    free(ctx);
}

test_result_t execute_test(test_function_t test_func, test_context_t *ctx)
{
    if (!test_func || !ctx) {
        return TEST_RESULT_ERROR;
    }
    
    printf(COLOR_BLUE "[START]" COLOR_RESET " %s\n", ctx->config.name);
    
    // Record start time
    clock_gettime(CLOCK_MONOTONIC, &ctx->start_time);
    ctx->is_running = true;
    
    // Start performance monitoring if enabled
    if (ctx->config.enable_performance_monitoring) {
        start_performance_monitoring(ctx);
    }
    
    // Execute the test
    test_result_t result = test_func(ctx);
    
    // Record end time
    clock_gettime(CLOCK_MONOTONIC, &ctx->end_time);
    ctx->is_running = false;
    
    // Stop performance monitoring
    if (ctx->config.enable_performance_monitoring) {
        stop_performance_monitoring(ctx);
    }
    
    // Update context result
    ctx->result = result;
    
    // Print result
    const char *result_str;
    const char *color;
    
    switch (result) {
        case TEST_RESULT_PASS:
            result_str = "PASS";
            color = COLOR_GREEN;
            break;
        case TEST_RESULT_FAIL:
            result_str = "FAIL";
            color = COLOR_RED;
            break;
        case TEST_RESULT_TIMEOUT:
            result_str = "TIMEOUT";
            color = COLOR_YELLOW;
            break;
        case TEST_RESULT_SKIP:
            result_str = "SKIP";
            color = COLOR_CYAN;
            break;
        default:
            result_str = "ERROR";
            color = COLOR_MAGENTA;
            break;
    }
    
    double duration = calculate_duration_seconds(ctx->start_time, ctx->end_time);
    printf("%s[%s]%s %s (%.3fs)\n", color, result_str, COLOR_RESET, 
           ctx->config.name, duration);
    
    if (result != TEST_RESULT_PASS && strlen(ctx->error_message) > 0) {
        printf("  Error: %s\n", ctx->error_message);
    }
    
    if (ctx->config.enable_performance_monitoring && result == TEST_RESULT_PASS) {
        printf("  Performance: %.2f GOPS, %.2f ms latency\n",
               ctx->performance.throughput_gops, ctx->performance.latency_ms);
    }
    
    return result;
}

// Timeout handler for test execution
static void *test_timeout_thread(void *arg)
{
    test_context_t *ctx = (test_context_t *)arg;
    
    sleep(ctx->config.timeout_seconds);
    
    if (ctx->is_running) {
        ctx->result = TEST_RESULT_TIMEOUT;
        snprintf(ctx->error_message, MAX_ERROR_MESSAGE_LENGTH,
                "Test timed out after %d seconds", ctx->config.timeout_seconds);
        
        // Cancel the test thread
        pthread_cancel(ctx->thread_id);
    }
    
    return NULL;
}

test_result_t execute_test_with_timeout(test_function_t test_func, test_context_t *ctx)
{
    if (!test_func || !ctx) {
        return TEST_RESULT_ERROR;
    }
    
    // Create timeout thread
    pthread_t timeout_thread;
    if (pthread_create(&timeout_thread, NULL, test_timeout_thread, ctx) != 0) {
        return TEST_RESULT_ERROR;
    }
    
    // Execute test
    test_result_t result = execute_test(test_func, ctx);
    
    // Cancel timeout thread
    pthread_cancel(timeout_thread);
    pthread_join(timeout_thread, NULL);
    
    return result;
}

int execute_test_suite(test_suite_t *suite)
{
    if (!suite || !suite->tests || !suite->configs) {
        return -1;
    }
    
    printf(COLOR_MAGENTA "\n=== Executing Test Suite: %s ===" COLOR_RESET "\n", suite->name);
    printf("Total tests: %zu\n\n", suite->test_count);
    
    suite->tests_passed = 0;
    suite->tests_failed = 0;
    suite->tests_skipped = 0;
    
    struct timespec suite_start, suite_end;
    clock_gettime(CLOCK_MONOTONIC, &suite_start);
    
    for (size_t i = 0; i < suite->test_count; i++) {
        print_test_progress(i + 1, suite->test_count, suite->configs[i].name);
        
        test_context_t *ctx = create_test_context(&suite->configs[i]);
        if (!ctx) {
            printf(COLOR_RED "[ERROR]" COLOR_RESET " Failed to create test context\n");
            continue;
        }
        
        test_result_t result = execute_test_with_timeout(suite->tests[i], ctx);
        
        // Update suite statistics
        switch (result) {
            case TEST_RESULT_PASS:
                suite->tests_passed++;
                break;
            case TEST_RESULT_FAIL:
            case TEST_RESULT_TIMEOUT:
            case TEST_RESULT_ERROR:
                suite->tests_failed++;
                if (g_stop_on_first_failure) {
                    destroy_test_context(ctx);
                    return -1;
                }
                break;
            case TEST_RESULT_SKIP:
                suite->tests_skipped++;
                break;
        }
        
        // Update global statistics
        pthread_mutex_lock(&stats_mutex);
        g_test_stats.total_tests++;
        switch (result) {
            case TEST_RESULT_PASS:
                g_test_stats.tests_passed++;
                break;
            case TEST_RESULT_FAIL:
                g_test_stats.tests_failed++;
                break;
            case TEST_RESULT_TIMEOUT:
                g_test_stats.tests_timeout++;
                break;
            case TEST_RESULT_SKIP:
                g_test_stats.tests_skipped++;
                break;
            default:
                break;
        }
        pthread_mutex_unlock(&stats_mutex);
        
        destroy_test_context(ctx);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &suite_end);
    double suite_duration = calculate_duration_seconds(suite_start, suite_end);
    
    // Print suite summary
    printf("\n" COLOR_MAGENTA "=== Test Suite Summary: %s ===" COLOR_RESET "\n", suite->name);
    printf("Duration: %.3f seconds\n", suite_duration);
    printf("Passed:   %zu/%zu\n", suite->tests_passed, suite->test_count);
    printf("Failed:   %zu/%zu\n", suite->tests_failed, suite->test_count);
    printf("Skipped:  %zu/%zu\n", suite->tests_skipped, suite->test_count);
    
    if (suite->tests_failed == 0) {
        printf(COLOR_GREEN "Suite Result: PASSED" COLOR_RESET "\n\n");
        return 0;
    } else {
        printf(COLOR_RED "Suite Result: FAILED" COLOR_RESET "\n\n");
        return -1;
    }
}

// =============================================================================
// Performance Monitoring Functions
// =============================================================================

void start_performance_monitoring(test_context_t *ctx)
{
    if (!ctx) return;
    
    // Reset performance counters
    if (ctx->npu_handle) {
        npu_reset_performance_counters(ctx->npu_handle);
    }
    
    // Initialize performance metrics
    ctx->performance.operations_count = 0;
    ctx->performance.errors_count = 0;
    ctx->performance.throughput_gops = 0.0;
    ctx->performance.latency_ms = 0.0;
    ctx->performance.power_watts = 0.0;
    ctx->performance.efficiency_gops_watt = 0.0;
}

void stop_performance_monitoring(test_context_t *ctx)
{
    if (!ctx) return;
    
    // Calculate test duration
    ctx->performance.duration_seconds = calculate_duration_seconds(ctx->start_time, ctx->end_time);
    
    // Get NPU performance counters
    if (ctx->npu_handle) {
        uint64_t cycles, operations;
        if (npu_get_performance_counters(ctx->npu_handle, &cycles, &operations) == NPU_SUCCESS) {
            ctx->performance.operations_count = operations;
        }
        
        // Get thermal and power information if available
        struct npu_thermal_info thermal;
        if (npu_get_thermal_info(ctx->npu_handle, &thermal) == NPU_SUCCESS) {
            ctx->performance.power_watts = thermal.power_consumption_mw / 1000.0;
        }
    }
    
    // Calculate derived metrics
    if (ctx->performance.duration_seconds > 0) {
        ctx->performance.throughput_gops = calculate_throughput_gops(
            ctx->performance.operations_count, ctx->performance.duration_seconds);
        
        ctx->performance.latency_ms = ctx->performance.duration_seconds * 1000.0;
        
        if (ctx->performance.power_watts > 0) {
            ctx->performance.efficiency_gops_watt = calculate_efficiency(
                ctx->performance.throughput_gops, ctx->performance.power_watts);
        }
    }
}

void update_performance_metrics(test_context_t *ctx, uint64_t operations_performed)
{
    if (!ctx) return;
    
    ctx->performance.operations_count += operations_performed;
}

double calculate_throughput_gops(uint64_t operations, double duration_seconds)
{
    if (duration_seconds <= 0) {
        return 0.0;
    }
    
    return (double)operations / (duration_seconds * 1e9);
}

double calculate_efficiency(double throughput_gops, double power_watts)
{
    if (power_watts <= 0) {
        return 0.0;
    }
    
    return throughput_gops / power_watts;
}

// =============================================================================
// Utility Functions
// =============================================================================

uint64_t get_timestamp_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

double calculate_duration_seconds(struct timespec start, struct timespec end)
{
    double start_seconds = (double)start.tv_sec + (double)start.tv_nsec / 1e9;
    double end_seconds = (double)end.tv_sec + (double)end.tv_nsec / 1e9;
    return end_seconds - start_seconds;
}

void generate_random_data(void *buffer, size_t size)
{
    if (!buffer || size == 0) return;
    
    uint8_t *bytes = (uint8_t *)buffer;
    for (size_t i = 0; i < size; i++) {
        bytes[i] = rand() & 0xFF;
    }
}

bool compare_buffers(const void *buffer1, const void *buffer2, size_t size)
{
    return memcmp(buffer1, buffer2, size) == 0;
}

void print_test_progress(size_t current, size_t total, const char *test_name)
{
    double progress = (double)current / (double)total * 100.0;
    printf(COLOR_CYAN "[%3.0f%%]" COLOR_RESET " (%zu/%zu) %s\n", 
           progress, current, total, test_name);
}

void print_performance_summary(const performance_metrics_t *metrics)
{
    if (!metrics) return;
    
    printf(COLOR_YELLOW "=== Performance Summary ===" COLOR_RESET "\n");
    printf("Duration:     %.3f seconds\n", metrics->duration_seconds);
    printf("Operations:   %lu\n", metrics->operations_count);
    printf("Throughput:   %.2f GOPS\n", metrics->throughput_gops);
    printf("Latency:      %.2f ms\n", metrics->latency_ms);
    printf("Power:        %.2f W\n", metrics->power_watts);
    printf("Efficiency:   %.2f GOPS/W\n", metrics->efficiency_gops_watt);
    printf("Errors:       %lu\n", metrics->errors_count);
    printf("\n");
}

void print_test_statistics(const test_statistics_t *stats)
{
    if (!stats) return;
    
    double pass_rate = (stats->total_tests > 0) ? 
        (double)stats->tests_passed / (double)stats->total_tests * 100.0 : 0.0;
    
    printf(COLOR_WHITE "=== Final Test Statistics ===" COLOR_RESET "\n");
    printf("Total Tests:  %zu\n", stats->total_tests);
    printf("Passed:       %zu (%.1f%%)\n", stats->tests_passed, pass_rate);
    printf("Failed:       %zu\n", stats->tests_failed);
    printf("Timeout:      %zu\n", stats->tests_timeout);
    printf("Skipped:      %zu\n", stats->tests_skipped);
    printf("Duration:     %.3f seconds\n", stats->total_duration);
    
    if (stats->tests_failed == 0 && stats->tests_timeout == 0) {
        printf(COLOR_GREEN "Overall Result: ALL TESTS PASSED" COLOR_RESET "\n");
    } else {
        printf(COLOR_RED "Overall Result: SOME TESTS FAILED" COLOR_RESET "\n");
    }
    printf("\n");
}

// =============================================================================
// Test Data Management
// =============================================================================

void* allocate_test_buffer(size_t size, size_t alignment)
{
    void *buffer;
    
    if (posix_memalign(&buffer, alignment, size) != 0) {
        return NULL;
    }
    
    return buffer;
}

void free_test_buffer(void *buffer)
{
    if (buffer) {
        free(buffer);
    }
}

void initialize_test_data(void *buffer, size_t size, int pattern)
{
    if (!buffer || size == 0) return;
    
    uint8_t *bytes = (uint8_t *)buffer;
    
    switch (pattern) {
        case PATTERN_ZEROS:
            memset(buffer, 0, size);
            break;
            
        case PATTERN_ONES:
            memset(buffer, 0xFF, size);
            break;
            
        case PATTERN_RANDOM:
            generate_random_data(buffer, size);
            break;
            
        case PATTERN_SEQUENCE:
            for (size_t i = 0; i < size; i++) {
                bytes[i] = i & 0xFF;
            }
            break;
            
        case PATTERN_CHECKERBOARD:
            for (size_t i = 0; i < size; i++) {
                bytes[i] = (i % 2) ? 0xAA : 0x55;
            }
            break;
            
        default:
            memset(buffer, 0, size);
            break;
    }
}

// =============================================================================
// System Health Monitoring
// =============================================================================

int check_system_health(void)
{
    // Check memory availability
    uint64_t total_mb, available_mb;
    if (get_memory_usage(&total_mb, &available_mb) != 0) {
        printf(COLOR_YELLOW "[WARNING]" COLOR_RESET " Could not check memory usage\n");
    } else if (available_mb < 1024) { // Less than 1GB available
        printf(COLOR_YELLOW "[WARNING]" COLOR_RESET " Low memory: %lu MB available\n", available_mb);
    }
    
    // Check for thermal throttling
    if (check_thermal_throttling()) {
        printf(COLOR_YELLOW "[WARNING]" COLOR_RESET " Thermal throttling detected\n");
        return -1;
    }
    
    return 0;
}

void monitor_system_resources(test_context_t *ctx)
{
    // This would typically monitor CPU, memory, temperature, etc.
    // Implementation depends on system-specific APIs
    (void)ctx; // Suppress unused parameter warning
}

bool check_thermal_throttling(void)
{
    // Implementation would check system thermal status
    // This is a placeholder implementation
    return false;
}

int get_memory_usage(uint64_t *total_mb, uint64_t *available_mb)
{
    FILE *meminfo = fopen("/proc/meminfo", "r");
    if (!meminfo) {
        return -1;
    }
    
    char line[256];
    uint64_t mem_total = 0, mem_available = 0;
    
    while (fgets(line, sizeof(line), meminfo)) {
        if (sscanf(line, "MemTotal: %lu kB", &mem_total) == 1) {
            continue;
        }
        if (sscanf(line, "MemAvailable: %lu kB", &mem_available) == 1) {
            continue;
        }
    }
    
    fclose(meminfo);
    
    if (total_mb) *total_mb = mem_total / 1024;
    if (available_mb) *available_mb = mem_available / 1024;
    
    return 0;
}

// =============================================================================
// Signal Handling
// =============================================================================

static void signal_handler(int signum)
{
    switch (signum) {
        case SIGINT:
        case SIGTERM:
            printf("\n" COLOR_YELLOW "[SIGNAL]" COLOR_RESET " Received termination signal, cleaning up...\n");
            emergency_cleanup();
            exit(1);
            break;
            
        case SIGSEGV:
            printf("\n" COLOR_RED "[FATAL]" COLOR_RESET " Segmentation fault detected\n");
            emergency_cleanup();
            abort();
            break;
            
        default:
            break;
    }
}

void setup_signal_handlers(void)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);
}

void emergency_cleanup(void)
{
    printf("Performing emergency cleanup...\n");
    integration_test_cleanup();
}

void register_cleanup_function(void (*cleanup_func)(void))
{
    if (cleanup_function_count < 16 && cleanup_func) {
        cleanup_functions[cleanup_function_count++] = cleanup_func;
    }
}