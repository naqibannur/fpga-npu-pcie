/**
 * FPGA NPU Performance Benchmark Framework Implementation
 * 
 * Core implementation of benchmarking infrastructure
 */

#include "benchmark_framework.h"
#include <sys/resource.h>
#include <sys/sysinfo.h>

// =============================================================================
// Global Variables
// =============================================================================

bool g_benchmark_verbose = false;
bool g_benchmark_stop_on_error = false;
FILE *g_benchmark_log_file = NULL;

// Static variables
static bool framework_initialized = false;
static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;

// =============================================================================
// Core Framework Functions
// =============================================================================

int benchmark_framework_init(void)
{
    pthread_mutex_lock(&global_mutex);
    
    if (framework_initialized) {
        pthread_mutex_unlock(&global_mutex);
        return 0;
    }
    
    // Initialize random seed for reproducible tests
    srand(12345);
    
    framework_initialized = true;
    pthread_mutex_unlock(&global_mutex);
    
    printf(ANSI_CYAN "FPGA NPU Benchmark Framework Initialized" ANSI_RESET "\n");
    return 0;
}

void benchmark_framework_cleanup(void)
{
    pthread_mutex_lock(&global_mutex);
    
    if (!framework_initialized) {
        pthread_mutex_unlock(&global_mutex);
        return;
    }
    
    if (g_benchmark_log_file && g_benchmark_log_file != stdout && g_benchmark_log_file != stderr) {
        fclose(g_benchmark_log_file);
        g_benchmark_log_file = NULL;
    }
    
    framework_initialized = false;
    pthread_mutex_unlock(&global_mutex);
    
    printf(ANSI_CYAN "Benchmark Framework Cleanup Complete" ANSI_RESET "\n");
}

benchmark_context_t* create_benchmark_context(const benchmark_config_t *config)
{
    if (!config) {
        return NULL;
    }
    
    benchmark_context_t *ctx = calloc(1, sizeof(benchmark_context_t));
    if (!ctx) {
        return NULL;
    }
    
    // Copy configuration
    memcpy(&ctx->config, config, sizeof(benchmark_config_t));
    
    // Initialize mutex
    if (pthread_mutex_init(&ctx->mutex, NULL) != 0) {
        free(ctx);
        return NULL;
    }
    
    // Initialize NPU
    ctx->npu_handle = npu_init();
    if (!ctx->npu_handle) {
        printf(ANSI_RED "Failed to initialize NPU for benchmark" ANSI_RESET "\n");
        pthread_mutex_destroy(&ctx->mutex);
        free(ctx);
        return NULL;
    }
    
    // Determine data size based on benchmark type and size
    ctx->data_size = get_buffer_size(config->size);
    if (config->size == SIZE_CUSTOM) {
        size_t element_size = sizeof(float); // Assume float for most operations
        ctx->data_size = config->custom_size_x * config->custom_size_y * element_size;
    }
    
    // Allocate test data buffers
    ctx->test_data_a = aligned_alloc(64, ctx->data_size);
    ctx->test_data_b = aligned_alloc(64, ctx->data_size);
    ctx->result_data = aligned_alloc(64, ctx->data_size);
    
    if (!ctx->test_data_a || !ctx->test_data_b || !ctx->result_data) {
        printf(ANSI_RED "Failed to allocate test data buffers" ANSI_RESET "\n");
        destroy_benchmark_context(ctx);
        return NULL;
    }
    
    // Initialize test data
    initialize_benchmark_data(ctx->test_data_a, ctx->data_size, 0);
    initialize_benchmark_data(ctx->test_data_b, ctx->data_size, 1);
    memset(ctx->result_data, 0, ctx->data_size);
    
    // Allocate NPU buffers
    ctx->buffer_a = npu_buffer_alloc(ctx->npu_handle, ctx->data_size, NPU_ALLOC_COHERENT);
    ctx->buffer_b = npu_buffer_alloc(ctx->npu_handle, ctx->data_size, NPU_ALLOC_COHERENT);
    ctx->buffer_result = npu_buffer_alloc(ctx->npu_handle, ctx->data_size, NPU_ALLOC_COHERENT);
    
    if (!ctx->buffer_a || !ctx->buffer_b || !ctx->buffer_result) {
        printf(ANSI_RED "Failed to allocate NPU buffers" ANSI_RESET "\n");
        destroy_benchmark_context(ctx);
        return NULL;
    }
    
    // Create result structure
    ctx->result = calloc(1, sizeof(benchmark_result_t));
    if (!ctx->result) {
        destroy_benchmark_context(ctx);
        return NULL;
    }
    
    // Copy config to result
    memcpy(&ctx->result->config, config, sizeof(benchmark_config_t));
    
    // Allocate latency samples array
    ctx->result->latency_samples = calloc(config->iterations, sizeof(double));
    if (!ctx->result->latency_samples) {
        destroy_benchmark_context(ctx);
        return NULL;
    }
    
    ctx->stop_requested = false;
    
    return ctx;
}

void destroy_benchmark_context(benchmark_context_t *ctx)
{
    if (!ctx) {
        return;
    }
    
    // Free NPU buffers
    if (ctx->buffer_a) {
        npu_buffer_free(ctx->npu_handle, ctx->buffer_a);
    }
    if (ctx->buffer_b) {
        npu_buffer_free(ctx->npu_handle, ctx->buffer_b);
    }
    if (ctx->buffer_result) {
        npu_buffer_free(ctx->npu_handle, ctx->buffer_result);
    }
    
    // Cleanup NPU
    if (ctx->npu_handle) {
        npu_cleanup(ctx->npu_handle);
    }
    
    // Free test data
    if (ctx->test_data_a) free(ctx->test_data_a);
    if (ctx->test_data_b) free(ctx->test_data_b);
    if (ctx->result_data) free(ctx->result_data);
    
    // Free result structure
    if (ctx->result) {
        if (ctx->result->latency_samples) {
            free(ctx->result->latency_samples);
        }
        free(ctx->result);
    }
    
    // Destroy mutex
    pthread_mutex_destroy(&ctx->mutex);
    
    free(ctx);
}

int execute_benchmark(benchmark_function_t benchmark_func, benchmark_context_t *ctx)
{
    if (!benchmark_func || !ctx) {
        return -1;
    }
    
    printf(ANSI_BLUE "[BENCHMARK]" ANSI_RESET " Starting: %s\n", ctx->config.name);
    
    // Record start time
    clock_gettime(CLOCK_MONOTONIC, &ctx->result->start_time);
    
    // Execute warmup iterations
    if (ctx->config.warmup_iterations > 0) {
        printf("  Warming up (%d iterations)...\n", ctx->config.warmup_iterations);
        
        benchmark_config_t warmup_config = ctx->config;
        warmup_config.iterations = ctx->config.warmup_iterations;
        
        benchmark_context_t *warmup_ctx = create_benchmark_context(&warmup_config);
        if (warmup_ctx) {
            benchmark_func(warmup_ctx);
            destroy_benchmark_context(warmup_ctx);
        }
    }
    
    // Start performance monitoring
    start_performance_monitoring(ctx);
    
    // Execute main benchmark
    int result = benchmark_func(ctx);
    
    // Stop performance monitoring
    stop_performance_monitoring(ctx);
    
    // Record end time
    clock_gettime(CLOCK_MONOTONIC, &ctx->result->end_time);
    
    // Calculate final metrics
    ctx->result->metrics.duration_seconds = calculate_elapsed_time(
        ctx->result->start_time, ctx->result->end_time);
    
    // Set success flag
    ctx->result->success = (result == 0);
    
    // Print result summary
    if (result == 0) {
        printf(ANSI_GREEN "[PASS]" ANSI_RESET " %s\n", ctx->config.name);
        if (g_benchmark_verbose) {
            print_performance_metrics(&ctx->result->metrics);
        }
    } else {
        printf(ANSI_RED "[FAIL]" ANSI_RESET " %s: %s\n", 
               ctx->config.name, ctx->result->error_message);
    }
    
    return result;
}

int execute_benchmark_suite(benchmark_suite_t *suite)
{
    if (!suite) {
        return -1;
    }
    
    printf(ANSI_MAGENTA "\n=== Executing Benchmark Suite: %s ===" ANSI_RESET "\n", suite->name);
    printf("Total benchmarks: %zu\n\n", suite->benchmark_count);
    
    int failures = 0;
    double total_duration = 0;
    
    for (size_t i = 0; i < suite->benchmark_count; i++) {
        printf("Progress: %zu/%zu\n", i + 1, suite->benchmark_count);
        
        benchmark_context_t *ctx = create_benchmark_context(&suite->configs[i]);
        if (!ctx) {
            printf(ANSI_RED "Failed to create context for benchmark %zu" ANSI_RESET "\n", i);
            failures++;
            continue;
        }
        
        int result = execute_benchmark(suite->functions[i], ctx);
        
        // Copy result
        memcpy(&suite->results[i], ctx->result, sizeof(benchmark_result_t));
        
        if (result != 0) {
            failures++;
            if (g_benchmark_stop_on_error) {
                destroy_benchmark_context(ctx);
                break;
            }
        }
        
        total_duration += suite->results[i].metrics.duration_seconds;
        destroy_benchmark_context(ctx);
    }
    
    // Calculate overall metrics
    suite->overall_metrics.duration_seconds = total_duration;
    
    // Print suite summary
    printf(ANSI_MAGENTA "\n=== Benchmark Suite Summary: %s ===" ANSI_RESET "\n", suite->name);
    printf("Completed: %zu/%zu benchmarks\n", suite->benchmark_count - failures, suite->benchmark_count);
    printf("Total duration: %.3f seconds\n", total_duration);
    printf("Failures: %d\n", failures);
    
    if (failures == 0) {
        printf(ANSI_GREEN "Suite Result: ALL BENCHMARKS PASSED" ANSI_RESET "\n\n");
        return 0;
    } else {
        printf(ANSI_RED "Suite Result: %d BENCHMARKS FAILED" ANSI_RESET "\n\n", failures);
        return -1;
    }
}

// =============================================================================
// Performance Measurement Functions
// =============================================================================

uint64_t get_timestamp_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

double calculate_elapsed_time(struct timespec start, struct timespec end)
{
    double start_sec = (double)start.tv_sec + (double)start.tv_nsec / 1e9;
    double end_sec = (double)end.tv_sec + (double)end.tv_nsec / 1e9;
    return end_sec - start_sec;
}

void start_performance_monitoring(benchmark_context_t *ctx)
{
    if (!ctx) return;
    
    // Reset performance counters
    if (ctx->npu_handle) {
        npu_reset_performance_counters(ctx->npu_handle);
    }
    
    // Initialize metrics
    memset(&ctx->result->metrics, 0, sizeof(performance_metrics_t));
}

void stop_performance_monitoring(benchmark_context_t *ctx)
{
    if (!ctx) return;
    
    // Get NPU performance counters
    if (ctx->npu_handle) {
        uint64_t cycles, operations;
        if (npu_get_performance_counters(ctx->npu_handle, &cycles, &operations) == NPU_SUCCESS) {
            ctx->result->metrics.operations_count = operations;
        }
        
        // Get thermal info if available
        struct npu_thermal_info thermal;
        if (npu_get_thermal_info(ctx->npu_handle, &thermal) == NPU_SUCCESS) {
            ctx->result->metrics.power_watts = thermal.power_consumption_mw / 1000.0;
        }
    }
    
    // Calculate derived metrics
    double duration = ctx->result->metrics.duration_seconds;
    if (duration > 0) {
        uint64_t ops = calculate_operations_count(ctx->config.type, 
                                                ctx->config.custom_size_x,
                                                ctx->config.custom_size_y, 
                                                ctx->config.custom_size_z);
        ops *= ctx->config.iterations;
        
        ctx->result->metrics.throughput_gops = (double)ops / (duration * 1e9);
        ctx->result->metrics.throughput_gflops = ctx->result->metrics.throughput_gops;
        
        if (ctx->result->metrics.power_watts > 0) {
            ctx->result->metrics.efficiency_gops_watt = 
                ctx->result->metrics.throughput_gops / ctx->result->metrics.power_watts;
        }
        
        // Calculate latency statistics
        if (ctx->result->sample_count > 0) {
            calculate_latency_statistics(ctx->result->latency_samples, 
                                       ctx->result->sample_count,
                                       &ctx->result->metrics.latency_ms,
                                       &ctx->result->metrics.latency_std_ms,
                                       &ctx->result->metrics.latency_min_ms,
                                       &ctx->result->metrics.latency_max_ms);
        }
        
        // Calculate memory bandwidth
        ctx->result->metrics.data_transferred = ctx->data_size * ctx->config.iterations * 2; // Read + Write
        ctx->result->metrics.bandwidth_gbps = 
            (double)ctx->result->metrics.data_transferred / (duration * 1e9);
    }
}

void record_latency_sample(benchmark_context_t *ctx, double latency_ms)
{
    if (!ctx || !ctx->result || ctx->result->sample_count >= ctx->config.iterations) {
        return;
    }
    
    ctx->result->latency_samples[ctx->result->sample_count] = latency_ms;
    ctx->result->sample_count++;
}

void calculate_latency_statistics(double *samples, uint32_t count,
                                double *mean, double *std_dev,
                                double *min_val, double *max_val)
{
    if (!samples || count == 0) {
        if (mean) *mean = 0;
        if (std_dev) *std_dev = 0;
        if (min_val) *min_val = 0;
        if (max_val) *max_val = 0;
        return;
    }
    
    // Calculate mean
    double sum = 0;
    double min_v = samples[0];
    double max_v = samples[0];
    
    for (uint32_t i = 0; i < count; i++) {
        sum += samples[i];
        if (samples[i] < min_v) min_v = samples[i];
        if (samples[i] > max_v) max_v = samples[i];
    }
    
    double avg = sum / count;
    
    // Calculate standard deviation
    double variance_sum = 0;
    for (uint32_t i = 0; i < count; i++) {
        double diff = samples[i] - avg;
        variance_sum += diff * diff;
    }
    
    double variance = variance_sum / count;
    double std_deviation = sqrt(variance);
    
    // Set output values
    if (mean) *mean = avg;
    if (std_dev) *std_dev = std_deviation;
    if (min_val) *min_val = min_v;
    if (max_val) *max_val = max_v;
}

// =============================================================================
// Utility Functions
// =============================================================================

void get_matrix_dimensions(benchmark_size_t size, uint32_t *rows, uint32_t *cols)
{
    switch (size) {
        case SIZE_SMALL:
            *rows = *cols = 16;
            break;
        case SIZE_MEDIUM:
            *rows = *cols = 64;
            break;
        case SIZE_LARGE:
            *rows = *cols = 256;
            break;
        case SIZE_XLARGE:
            *rows = *cols = 1024;
            break;
        default:
            *rows = *cols = 64; // Default
            break;
    }
}

size_t get_buffer_size(benchmark_size_t size)
{
    uint32_t rows, cols;
    get_matrix_dimensions(size, &rows, &cols);
    return rows * cols * sizeof(float);
}

void initialize_benchmark_data(void *buffer, size_t size, int pattern)
{
    if (!buffer || size == 0) return;
    
    float *data = (float *)buffer;
    size_t float_count = size / sizeof(float);
    
    switch (pattern) {
        case 0: // Random data
            for (size_t i = 0; i < float_count; i++) {
                data[i] = (float)(rand() % 1000) / 100.0f; // 0.0 to 9.99
            }
            break;
            
        case 1: // Sequential data
            for (size_t i = 0; i < float_count; i++) {
                data[i] = (float)(i % 100) / 10.0f; // 0.0 to 9.9 repeating
            }
            break;
            
        case 2: // Constant data
            for (size_t i = 0; i < float_count; i++) {
                data[i] = 1.0f;
            }
            break;
            
        default:
            memset(buffer, 0, size);
            break;
    }
}

bool validate_benchmark_results(const void *expected, const void *actual, 
                               size_t size, double tolerance)
{
    const float *exp = (const float *)expected;
    const float *act = (const float *)actual;
    size_t float_count = size / sizeof(float);
    
    for (size_t i = 0; i < float_count; i++) {
        if (fabs(exp[i] - act[i]) > tolerance) {
            return false;
        }
    }
    
    return true;
}

uint64_t calculate_operations_count(benchmark_type_t type, uint32_t size_x, 
                                   uint32_t size_y, uint32_t size_z)
{
    switch (type) {
        case BENCHMARK_MATRIX_MULT:
            return (uint64_t)size_x * size_y * size_z * 2; // Multiply-add operations
            
        case BENCHMARK_CONV2D:
            // Simplified calculation: output_size * kernel_size * channels
            return (uint64_t)size_x * size_y * 9 * 2; // 3x3 kernel assumed
            
        case BENCHMARK_ELEMENT_ADD:
        case BENCHMARK_ELEMENT_MUL:
            return (uint64_t)size_x * size_y;
            
        case BENCHMARK_MEMORY_BANDWIDTH:
            return 0; // Memory operations don't count as compute ops
            
        default:
            return (uint64_t)size_x * size_y;
    }
}

const char* benchmark_type_to_string(benchmark_type_t type)
{
    switch (type) {
        case BENCHMARK_MATRIX_MULT: return "Matrix Multiplication";
        case BENCHMARK_CONV2D: return "2D Convolution";
        case BENCHMARK_ELEMENT_ADD: return "Element-wise Addition";
        case BENCHMARK_ELEMENT_MUL: return "Element-wise Multiplication";
        case BENCHMARK_MEMORY_BANDWIDTH: return "Memory Bandwidth";
        case BENCHMARK_LATENCY: return "Latency";
        case BENCHMARK_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

const char* benchmark_size_to_string(benchmark_size_t size)
{
    switch (size) {
        case SIZE_SMALL: return "Small (16x16)";
        case SIZE_MEDIUM: return "Medium (64x64)";
        case SIZE_LARGE: return "Large (256x256)";
        case SIZE_XLARGE: return "X-Large (1024x1024)";
        case SIZE_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

// =============================================================================
// Configuration Helpers
// =============================================================================

benchmark_config_t create_default_config(benchmark_type_t type, benchmark_size_t size)
{
    benchmark_config_t config = {0};
    
    snprintf(config.name, sizeof(config.name), "%s_%s", 
             benchmark_type_to_string(type), benchmark_size_to_string(size));
    snprintf(config.description, sizeof(config.description), 
             "Benchmark for %s with %s data size", 
             benchmark_type_to_string(type), benchmark_size_to_string(size));
    
    config.type = type;
    config.size = size;
    config.iterations = DEFAULT_BENCHMARK_ITERATIONS;
    config.warmup_iterations = MAX_WARMUP_ITERATIONS;
    config.enable_power_monitoring = true;
    config.enable_thermal_monitoring = true;
    config.enable_detailed_timing = true;
    config.enable_memory_profiling = true;
    config.thread_count = 1;
    config.target_duration_sec = 0; // No target duration
    
    // Set custom sizes based on benchmark size
    uint32_t rows, cols;
    get_matrix_dimensions(size, &rows, &cols);
    config.custom_size_x = rows;
    config.custom_size_y = cols;
    config.custom_size_z = rows; // For matrix mult: A(MxK) * B(KxN) = C(MxN)
    
    return config;
}

// =============================================================================
// Reporting Functions
// =============================================================================

void print_benchmark_config(const benchmark_config_t *config)
{
    if (!config) return;
    
    printf("Benchmark Configuration:\n");
    printf("  Name: %s\n", config->name);
    printf("  Description: %s\n", config->description);
    printf("  Type: %s\n", benchmark_type_to_string(config->type));
    printf("  Size: %s\n", benchmark_size_to_string(config->size));
    printf("  Iterations: %u\n", config->iterations);
    printf("  Warmup: %u\n", config->warmup_iterations);
    printf("  Threads: %u\n", config->thread_count);
    if (config->size == SIZE_CUSTOM) {
        printf("  Custom Dimensions: %ux%ux%u\n", 
               config->custom_size_x, config->custom_size_y, config->custom_size_z);
    }
    printf("\n");
}

void print_performance_metrics(const performance_metrics_t *metrics)
{
    if (!metrics) return;
    
    printf("Performance Metrics:\n");
    printf("  Duration: %.3f seconds\n", metrics->duration_seconds);
    printf("  Throughput: %.2f GOPS\n", metrics->throughput_gops);
    printf("  Latency: %.3f ± %.3f ms (min: %.3f, max: %.3f)\n",
           metrics->latency_ms, metrics->latency_std_ms,
           metrics->latency_min_ms, metrics->latency_max_ms);
    printf("  Bandwidth: %.2f GB/s\n", metrics->bandwidth_gbps);
    if (metrics->power_watts > 0) {
        printf("  Power: %.2f W\n", metrics->power_watts);
        printf("  Efficiency: %.2f GOPS/W\n", metrics->efficiency_gops_watt);
    }
    printf("  Operations: %lu\n", metrics->operations_count);
    printf("  Data Transferred: %.2f MB\n", (double)metrics->data_transferred / (1024*1024));
    printf("\n");
}