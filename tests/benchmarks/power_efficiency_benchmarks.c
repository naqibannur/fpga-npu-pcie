/**
 * Power Efficiency Benchmarks Implementation
 * 
 * Comprehensive power consumption and thermal testing
 * for evaluating NPU energy efficiency
 */

#include "benchmark_framework.h"
#include <math.h>

// =============================================================================
// Power Monitoring Infrastructure
// =============================================================================

typedef struct {
    double voltage_v;
    double current_a;
    double power_w;
    double temperature_c;
    struct timespec timestamp;
} power_sample_t;

typedef struct {
    power_sample_t *samples;
    size_t capacity;
    size_t count;
    double sampling_interval_ms;
    bool monitoring_active;
    pthread_t monitoring_thread;
    pthread_mutex_t data_mutex;
} power_monitor_t;

static power_monitor_t g_power_monitor = {0};

// Power monitoring thread
void* power_monitoring_thread(void *arg)
{
    benchmark_context_t *ctx = (benchmark_context_t*)arg;
    
    while (g_power_monitor.monitoring_active) {
        power_sample_t sample = {0};
        
        // Read power metrics from NPU
        npu_power_info_t power_info;
        if (npu_get_power_info(ctx->npu_handle, &power_info) == NPU_SUCCESS) {
            sample.voltage_v = power_info.voltage_v;
            sample.current_a = power_info.current_a;
            sample.power_w = power_info.power_w;
            sample.temperature_c = power_info.temperature_c;
            clock_gettime(CLOCK_MONOTONIC, &sample.timestamp);
            
            pthread_mutex_lock(&g_power_monitor.data_mutex);
            if (g_power_monitor.count < g_power_monitor.capacity) {
                g_power_monitor.samples[g_power_monitor.count++] = sample;
            }
            pthread_mutex_unlock(&g_power_monitor.data_mutex);
        }
        
        // Wait for next sampling interval
        usleep((useconds_t)(g_power_monitor.sampling_interval_ms * 1000));
    }
    
    return NULL;
}

int start_power_monitoring(benchmark_context_t *ctx, double sampling_interval_ms)
{
    // Initialize power monitor
    g_power_monitor.capacity = 10000; // Store up to 10000 samples
    g_power_monitor.samples = malloc(g_power_monitor.capacity * sizeof(power_sample_t));
    g_power_monitor.count = 0;
    g_power_monitor.sampling_interval_ms = sampling_interval_ms;
    g_power_monitor.monitoring_active = true;
    
    if (!g_power_monitor.samples) {
        fprintf(stderr, "Failed to allocate power monitoring buffer\n");
        return -1;
    }
    
    pthread_mutex_init(&g_power_monitor.data_mutex, NULL);
    
    // Start monitoring thread
    if (pthread_create(&g_power_monitor.monitoring_thread, NULL, power_monitoring_thread, ctx) != 0) {
        fprintf(stderr, "Failed to create power monitoring thread\n");
        free(g_power_monitor.samples);
        return -1;
    }
    
    return 0;
}

void stop_power_monitoring(void)
{
    if (g_power_monitor.monitoring_active) {
        g_power_monitor.monitoring_active = false;
        pthread_join(g_power_monitor.monitoring_thread, NULL);
        pthread_mutex_destroy(&g_power_monitor.data_mutex);
    }
}

void calculate_power_metrics(performance_metrics_t *metrics)
{
    if (g_power_monitor.count == 0) {
        metrics->power_watts = 0.0;
        metrics->efficiency_gops_watt = 0.0;
        return;
    }
    
    double total_power = 0.0;
    double max_power = 0.0;
    double max_temperature = 0.0;
    
    pthread_mutex_lock(&g_power_monitor.data_mutex);
    
    for (size_t i = 0; i < g_power_monitor.count; i++) {
        total_power += g_power_monitor.samples[i].power_w;
        if (g_power_monitor.samples[i].power_w > max_power) {
            max_power = g_power_monitor.samples[i].power_w;
        }
        if (g_power_monitor.samples[i].temperature_c > max_temperature) {
            max_temperature = g_power_monitor.samples[i].temperature_c;
        }
    }
    
    pthread_mutex_unlock(&g_power_monitor.data_mutex);
    
    metrics->power_watts = total_power / g_power_monitor.count;
    metrics->max_power_watts = max_power;
    metrics->max_temperature_c = max_temperature;
    
    if (metrics->power_watts > 0.0) {
        metrics->efficiency_gops_watt = metrics->throughput_gops / metrics->power_watts;
    }
}

void cleanup_power_monitoring(void)
{
    stop_power_monitoring();
    free(g_power_monitor.samples);
    memset(&g_power_monitor, 0, sizeof(power_monitor_t));
}

// =============================================================================
// Power Efficiency Benchmarks
// =============================================================================

int benchmark_power_efficiency_matmul(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    printf("Running matrix multiplication power efficiency benchmark\n");
    
    // Matrix dimensions based on benchmark size
    size_t matrix_dim;
    switch (config->size) {
        case BENCHMARK_SIZE_SMALL:  matrix_dim = 128; break;
        case BENCHMARK_SIZE_MEDIUM: matrix_dim = 256; break;
        case BENCHMARK_SIZE_LARGE:  matrix_dim = 512; break;
        case BENCHMARK_SIZE_XLARGE: matrix_dim = 1024; break;
        default: matrix_dim = 256; break;
    }
    
    size_t matrix_size = matrix_dim * matrix_dim * sizeof(float);
    
    // Allocate matrices
    float *matrix_a = (float*)allocate_aligned_buffer(matrix_size, 64);
    float *matrix_b = (float*)allocate_aligned_buffer(matrix_size, 64);
    float *matrix_c = (float*)allocate_aligned_buffer(matrix_size, 64);
    
    if (!matrix_a || !matrix_b || !matrix_c) {
        fprintf(stderr, "Failed to allocate matrices\n");
        return -1;
    }
    
    // Initialize matrices
    initialize_matrix_random(matrix_a, matrix_dim, matrix_dim);
    initialize_matrix_random(matrix_b, matrix_dim, matrix_dim);
    
    printf("Testing %zux%zu matrix multiplication with power monitoring\n", matrix_dim, matrix_dim);
    
    // Start power monitoring
    if (start_power_monitoring(ctx, 10.0) != 0) { // Sample every 10ms
        fprintf(stderr, "Failed to start power monitoring\n");
        goto cleanup;
    }
    
    // Warmup iterations
    for (uint32_t i = 0; i < config->warmup_iterations; i++) {
        npu_matrix_multiply(ctx->npu_handle, matrix_a, matrix_b, matrix_c,
                           matrix_dim, matrix_dim, matrix_dim);
    }
    
    // Performance measurement with power monitoring
    uint64_t total_operations = 0;
    struct timespec start_time, end_time;
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    for (uint32_t i = 0; i < config->iterations; i++) {
        npu_result_t result = npu_matrix_multiply(ctx->npu_handle,
                                                 matrix_a, matrix_b, matrix_c,
                                                 matrix_dim, matrix_dim, matrix_dim);
        if (result == NPU_SUCCESS) {
            total_operations += 2ULL * matrix_dim * matrix_dim * matrix_dim;
        } else {
            metrics->errors_count++;
        }
        
        // Progress reporting
        if (config->iterations > 10 && (i + 1) % (config->iterations / 10) == 0) {
            printf("Progress: %u/%u iterations completed\n", i + 1, config->iterations);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Stop power monitoring
    stop_power_monitoring();
    
    // Calculate performance metrics
    double duration = calculate_duration_seconds(start_time, end_time);
    metrics->duration_seconds = duration;
    metrics->operations_count = total_operations;
    metrics->throughput_gops = (double)total_operations / (duration * 1e9);
    
    // Calculate power metrics
    calculate_power_metrics(metrics);
    
    printf("Matrix multiplication power efficiency results:\n");
    printf("  Throughput: %.2f GOPS\n", metrics->throughput_gops);
    printf("  Average power: %.2f W\n", metrics->power_watts);
    printf("  Peak power: %.2f W\n", metrics->max_power_watts);
    printf("  Max temperature: %.1f °C\n", metrics->max_temperature_c);
    printf("  Energy efficiency: %.2f GOPS/W\n", metrics->efficiency_gops_watt);
    
cleanup:
    cleanup_power_monitoring();
    free_aligned_buffer(matrix_a);
    free_aligned_buffer(matrix_b);
    free_aligned_buffer(matrix_c);
    
    return (metrics->errors_count == 0) ? 0 : -1;
}

// =============================================================================
// Thermal Benchmarks
// =============================================================================

int benchmark_thermal_behavior(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    printf("Running thermal behavior benchmark\n");
    
    // Extended duration for thermal testing
    const uint32_t thermal_test_duration_sec = 60; // 1 minute stress test
    const uint32_t iterations_per_sec = 10;
    uint32_t total_iterations = thermal_test_duration_sec * iterations_per_sec;
    
    // Small matrices for sustained load
    const size_t matrix_dim = 128;
    size_t matrix_size = matrix_dim * matrix_dim * sizeof(float);
    
    float *matrix_a = (float*)allocate_aligned_buffer(matrix_size, 64);
    float *matrix_b = (float*)allocate_aligned_buffer(matrix_size, 64);
    float *matrix_c = (float*)allocate_aligned_buffer(matrix_size, 64);
    
    if (!matrix_a || !matrix_b || !matrix_c) {
        fprintf(stderr, "Failed to allocate matrices for thermal test\n");
        return -1;
    }
    
    // Initialize matrices
    initialize_matrix_random(matrix_a, matrix_dim, matrix_dim);
    initialize_matrix_random(matrix_b, matrix_dim, matrix_dim);
    
    printf("Running thermal stress test for %u seconds...\n", thermal_test_duration_sec);
    
    // Start high-frequency power monitoring for thermal tracking
    if (start_power_monitoring(ctx, 100.0) != 0) { // Sample every 100ms
        fprintf(stderr, "Failed to start thermal monitoring\n");
        goto cleanup;
    }
    
    // Sustained load test
    uint64_t total_operations = 0;
    struct timespec start_time, end_time;
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    for (uint32_t i = 0; i < total_iterations; i++) {
        npu_result_t result = npu_matrix_multiply(ctx->npu_handle,
                                                 matrix_a, matrix_b, matrix_c,
                                                 matrix_dim, matrix_dim, matrix_dim);
        if (result == NPU_SUCCESS) {
            total_operations += 2ULL * matrix_dim * matrix_dim * matrix_dim;
        } else {
            metrics->errors_count++;
        }
        
        // Check for thermal throttling
        npu_power_info_t power_info;
        if (npu_get_power_info(ctx->npu_handle, &power_info) == NPU_SUCCESS) {
            if (power_info.thermal_throttling) {
                printf("Thermal throttling detected at iteration %u (%.1f °C)\n", 
                       i, power_info.temperature_c);
            }
        }
        
        // Progress reporting every 10 seconds
        if ((i + 1) % (iterations_per_sec * 10) == 0) {
            printf("Progress: %u/%u seconds completed\n", 
                   (i + 1) / iterations_per_sec, thermal_test_duration_sec);
        }
        
        // Small delay to control iteration rate
        usleep(100000); // 100ms delay
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Stop monitoring
    stop_power_monitoring();
    
    // Calculate thermal metrics
    double duration = calculate_duration_seconds(start_time, end_time);
    metrics->duration_seconds = duration;
    metrics->operations_count = total_operations;
    metrics->throughput_gops = (double)total_operations / (duration * 1e9);
    
    calculate_power_metrics(metrics);
    
    // Analyze thermal behavior
    double temp_rise = metrics->max_temperature_c - 25.0; // Assume 25°C ambient
    
    printf("Thermal behavior results:\n");
    printf("  Test duration: %.1f seconds\n", duration);
    printf("  Sustained throughput: %.2f GOPS\n", metrics->throughput_gops);
    printf("  Average power: %.2f W\n", metrics->power_watts);
    printf("  Peak power: %.2f W\n", metrics->max_power_watts);
    printf("  Maximum temperature: %.1f °C\n", metrics->max_temperature_c);
    printf("  Temperature rise: %.1f °C\n", temp_rise);
    printf("  Thermal efficiency: %.2f GOPS/W\n", metrics->efficiency_gops_watt);
    
    // Check for thermal issues
    if (metrics->max_temperature_c > 85.0) {
        printf("WARNING: High operating temperature detected (>85°C)\n");
    }
    
    if (temp_rise > 40.0) {
        printf("WARNING: High temperature rise detected (>40°C)\n");
    }
    
cleanup:
    cleanup_power_monitoring();
    free_aligned_buffer(matrix_a);
    free_aligned_buffer(matrix_b);
    free_aligned_buffer(matrix_c);
    
    return (metrics->errors_count == 0) ? 0 : -1;
}

// =============================================================================
// Dynamic Voltage and Frequency Scaling (DVFS) Benchmarks
// =============================================================================

int benchmark_dvfs_efficiency(benchmark_context_t *ctx)
{
    const benchmark_config_t *config = &ctx->config;
    performance_metrics_t *metrics = ctx->result;
    
    printf("Running DVFS efficiency benchmark\n");
    
    // Test different frequency and voltage settings
    const struct {
        uint32_t frequency_mhz;
        double voltage_v;
        const char *name;
    } dvfs_settings[] = {
        {100, 0.8, "Low Power"},
        {200, 0.9, "Medium"},
        {400, 1.0, "High Performance"},
        {500, 1.1, "Maximum"}
    };
    
    const size_t num_settings = sizeof(dvfs_settings) / sizeof(dvfs_settings[0]);
    
    // Test matrices
    const size_t matrix_dim = 256;
    size_t matrix_size = matrix_dim * matrix_dim * sizeof(float);
    
    float *matrix_a = (float*)allocate_aligned_buffer(matrix_size, 64);
    float *matrix_b = (float*)allocate_aligned_buffer(matrix_size, 64);
    float *matrix_c = (float*)allocate_aligned_buffer(matrix_size, 64);
    
    if (!matrix_a || !matrix_b || !matrix_c) {
        fprintf(stderr, "Failed to allocate matrices for DVFS test\n");
        return -1;
    }
    
    // Initialize matrices
    initialize_matrix_random(matrix_a, matrix_dim, matrix_dim);
    initialize_matrix_random(matrix_b, matrix_dim, matrix_dim);
    
    printf("Testing DVFS settings:\n");
    printf("Setting           | Freq    | Voltage | Throughput | Power  | Efficiency\n");
    printf("------------------|---------|---------|------------|--------|-----------\n");
    
    double best_efficiency = 0.0;
    size_t best_setting_idx = 0;
    
    for (size_t setting_idx = 0; setting_idx < num_settings; setting_idx++) {
        const auto *setting = &dvfs_settings[setting_idx];
        
        // Set NPU frequency and voltage
        npu_dvfs_config_t dvfs_config = {
            .frequency_mhz = setting->frequency_mhz,
            .voltage_v = setting->voltage_v
        };
        
        if (npu_set_dvfs_config(ctx->npu_handle, &dvfs_config) != NPU_SUCCESS) {
            printf("Failed to set DVFS configuration for %s\n", setting->name);
            continue;
        }
        
        // Wait for frequency change to stabilize
        usleep(100000); // 100ms
        
        // Start power monitoring
        if (start_power_monitoring(ctx, 10.0) != 0) {
            printf("Failed to start power monitoring for %s\n", setting->name);
            continue;
        }
        
        // Performance test
        uint64_t operations_performed = 0;
        struct timespec start_time, end_time;
        
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        
        for (uint32_t i = 0; i < config->iterations; i++) {
            npu_result_t result = npu_matrix_multiply(ctx->npu_handle,
                                                     matrix_a, matrix_b, matrix_c,
                                                     matrix_dim, matrix_dim, matrix_dim);
            if (result == NPU_SUCCESS) {
                operations_performed += 2ULL * matrix_dim * matrix_dim * matrix_dim;
            }
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        
        stop_power_monitoring();
        
        // Calculate metrics for this setting
        double duration = calculate_duration_seconds(start_time, end_time);
        double throughput = (double)operations_performed / (duration * 1e9);
        
        performance_metrics_t setting_metrics = {0};
        setting_metrics.throughput_gops = throughput;
        calculate_power_metrics(&setting_metrics);
        
        printf("%-17s | %6u  | %7.1f | %8.2f   | %6.2f | %9.2f\n",
               setting->name, setting->frequency_mhz, setting->voltage_v,
               throughput, setting_metrics.power_watts, setting_metrics.efficiency_gops_watt);
        
        // Track best efficiency
        if (setting_metrics.efficiency_gops_watt > best_efficiency) {
            best_efficiency = setting_metrics.efficiency_gops_watt;
            best_setting_idx = setting_idx;
            
            // Update overall metrics with best result
            metrics->throughput_gops = throughput;
            metrics->power_watts = setting_metrics.power_watts;
            metrics->efficiency_gops_watt = setting_metrics.efficiency_gops_watt;
        }
        
        cleanup_power_monitoring();
    }
    
    printf("\nOptimal DVFS setting: %s (%.2f GOPS/W)\n",
           dvfs_settings[best_setting_idx].name, best_efficiency);
    
    metrics->operations_count = 1; // Placeholder
    
    free_aligned_buffer(matrix_a);
    free_aligned_buffer(matrix_b);
    free_aligned_buffer(matrix_c);
    
    return 0;
}

// =============================================================================
// Idle Power Benchmarks
// =============================================================================

int benchmark_idle_power(benchmark_context_t *ctx)
{
    performance_metrics_t *metrics = ctx->result;
    
    printf("Running idle power benchmark\n");
    
    // Ensure NPU is idle
    npu_wait_idle(ctx->npu_handle);
    
    printf("Measuring idle power for 30 seconds...\n");
    
    // Start power monitoring for idle measurement
    if (start_power_monitoring(ctx, 100.0) != 0) { // Sample every 100ms
        fprintf(stderr, "Failed to start idle power monitoring\n");
        return -1;
    }
    
    // Wait for 30 seconds while measuring idle power
    sleep(30);
    
    stop_power_monitoring();
    
    // Calculate idle power metrics
    calculate_power_metrics(metrics);
    
    printf("Idle power results:\n");
    printf("  Average idle power: %.3f W\n", metrics->power_watts);
    printf("  Peak idle power: %.3f W\n", metrics->max_power_watts);
    printf("  Idle temperature: %.1f °C\n", metrics->max_temperature_c);
    
    metrics->throughput_gops = 0.0; // No operations during idle
    metrics->operations_count = 0;
    
    cleanup_power_monitoring();
    
    return 0;
}