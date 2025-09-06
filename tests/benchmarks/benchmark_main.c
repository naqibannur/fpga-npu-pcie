/**
 * Main Benchmark Runner Application
 * 
 * Comprehensive NPU performance benchmarking suite with command-line interface
 * and detailed reporting capabilities
 */

#include "benchmark_framework.h"
#include <getopt.h>

// External benchmark function declarations
extern int benchmark_matmul_throughput(benchmark_context_t *ctx);
extern int benchmark_conv2d_throughput(benchmark_context_t *ctx);
extern int benchmark_elementwise_throughput(benchmark_context_t *ctx);
extern int benchmark_memory_bandwidth(benchmark_context_t *ctx);

extern int benchmark_single_operation_latency(benchmark_context_t *ctx);
extern int benchmark_batch_operation_latency(benchmark_context_t *ctx);
extern int benchmark_memory_access_latency(benchmark_context_t *ctx);
extern int benchmark_context_switch_latency(benchmark_context_t *ctx);

extern int benchmark_multithreaded_throughput(benchmark_context_t *ctx);
extern int benchmark_data_size_scaling(benchmark_context_t *ctx);
extern int benchmark_concurrent_mixed_workload(benchmark_context_t *ctx);
extern int benchmark_load_balancing(benchmark_context_t *ctx);

extern int benchmark_power_efficiency_matmul(benchmark_context_t *ctx);
extern int benchmark_thermal_behavior(benchmark_context_t *ctx);
extern int benchmark_dvfs_efficiency(benchmark_context_t *ctx);
extern int benchmark_idle_power(benchmark_context_t *ctx);

// =============================================================================
// Benchmark Suite Definitions
// =============================================================================

typedef struct {
    const char *name;
    const char *description;
    benchmark_function_t function;
    benchmark_type_t type;
    benchmark_size_t default_size;
    uint32_t default_iterations;
    uint32_t default_warmup;
    bool requires_power_monitoring;
} benchmark_definition_t;

static const benchmark_definition_t g_benchmark_definitions[] = {
    // Throughput benchmarks
    {
        "matmul_throughput",
        "Matrix multiplication throughput",
        benchmark_matmul_throughput,
        BENCHMARK_TYPE_THROUGHPUT,
        BENCHMARK_SIZE_MEDIUM,
        100, 10, false
    },
    {
        "conv2d_throughput",
        "2D convolution throughput",
        benchmark_conv2d_throughput,
        BENCHMARK_TYPE_THROUGHPUT,
        BENCHMARK_SIZE_MEDIUM,
        50, 5, false
    },
    {
        "elementwise_throughput",
        "Element-wise operations throughput",
        benchmark_elementwise_throughput,
        BENCHMARK_TYPE_THROUGHPUT,
        BENCHMARK_SIZE_MEDIUM,
        100, 10, false
    },
    {
        "memory_bandwidth",
        "Memory bandwidth",
        benchmark_memory_bandwidth,
        BENCHMARK_TYPE_MEMORY_BANDWIDTH,
        BENCHMARK_SIZE_MEDIUM,
        50, 5, false
    },
    
    // Latency benchmarks
    {
        "single_op_latency",
        "Single operation latency",
        benchmark_single_operation_latency,
        BENCHMARK_TYPE_LATENCY,
        BENCHMARK_SIZE_SMALL,
        1000, 100, false
    },
    {
        "batch_op_latency",
        "Batch operation latency",
        benchmark_batch_operation_latency,
        BENCHMARK_TYPE_LATENCY,
        BENCHMARK_SIZE_MEDIUM,
        100, 10, false
    },
    {
        "memory_access_latency",
        "Memory access latency",
        benchmark_memory_access_latency,
        BENCHMARK_TYPE_LATENCY,
        BENCHMARK_SIZE_SMALL,
        500, 50, false
    },
    {
        "context_switch_latency",
        "Context switch latency",
        benchmark_context_switch_latency,
        BENCHMARK_TYPE_LATENCY,
        BENCHMARK_SIZE_SMALL,
        200, 20, false
    },
    
    // Scalability benchmarks
    {
        "multithreaded_throughput",
        "Multi-threaded throughput scaling",
        benchmark_multithreaded_throughput,
        BENCHMARK_TYPE_SCALABILITY,
        BENCHMARK_SIZE_MEDIUM,
        400, 20, false
    },
    {
        "data_size_scaling",
        "Data size scaling analysis",
        benchmark_data_size_scaling,
        BENCHMARK_TYPE_SCALABILITY,
        BENCHMARK_SIZE_LARGE,
        50, 5, false
    },
    {
        "concurrent_mixed_workload",
        "Concurrent mixed workload",
        benchmark_concurrent_mixed_workload,
        BENCHMARK_TYPE_SCALABILITY,
        BENCHMARK_SIZE_MEDIUM,
        200, 10, false
    },
    {
        "load_balancing",
        "Load balancing optimization",
        benchmark_load_balancing,
        BENCHMARK_TYPE_SCALABILITY,
        BENCHMARK_SIZE_MEDIUM,
        100, 5, false
    },
    
    // Power efficiency benchmarks
    {
        "power_efficiency_matmul",
        "Matrix multiplication power efficiency",
        benchmark_power_efficiency_matmul,
        BENCHMARK_TYPE_POWER_EFFICIENCY,
        BENCHMARK_SIZE_MEDIUM,
        50, 5, true
    },
    {
        "thermal_behavior",
        "Thermal behavior under load",
        benchmark_thermal_behavior,
        BENCHMARK_TYPE_POWER_EFFICIENCY,
        BENCHMARK_SIZE_SMALL,
        600, 10, true
    },
    {
        "dvfs_efficiency",
        "DVFS efficiency analysis",
        benchmark_dvfs_efficiency,
        BENCHMARK_TYPE_POWER_EFFICIENCY,
        BENCHMARK_SIZE_MEDIUM,
        20, 2, true
    },
    {
        "idle_power",
        "Idle power consumption",
        benchmark_idle_power,
        BENCHMARK_TYPE_POWER_EFFICIENCY,
        BENCHMARK_SIZE_SMALL,
        1, 0, true
    }
};

static const size_t g_num_benchmarks = sizeof(g_benchmark_definitions) / sizeof(g_benchmark_definitions[0]);

// =============================================================================
// Command Line Configuration
// =============================================================================

typedef struct {
    bool run_all_benchmarks;
    bool run_throughput_benchmarks;
    bool run_latency_benchmarks;
    bool run_scalability_benchmarks;
    bool run_power_benchmarks;
    char specific_benchmark[128];
    benchmark_size_t benchmark_size;
    uint32_t iterations;
    uint32_t warmup_iterations;
    uint32_t thread_count;
    bool enable_power_monitoring;
    bool enable_thermal_monitoring;
    bool verbose_output;
    char output_directory[256];
    char log_file[256];
    bool generate_csv_report;
    bool generate_json_report;
    bool help_requested;
} benchmark_config_options_t;

static benchmark_config_options_t g_config = {
    .run_all_benchmarks = true,
    .run_throughput_benchmarks = false,
    .run_latency_benchmarks = false,
    .run_scalability_benchmarks = false,
    .run_power_benchmarks = false,
    .specific_benchmark = "",
    .benchmark_size = BENCHMARK_SIZE_MEDIUM,
    .iterations = 0, // Use benchmark defaults
    .warmup_iterations = 0, // Use benchmark defaults
    .thread_count = 4,
    .enable_power_monitoring = false,
    .enable_thermal_monitoring = false,
    .verbose_output = false,
    .output_directory = "./benchmark_results",
    .log_file = "",
    .generate_csv_report = true,
    .generate_json_report = false,
    .help_requested = false
};

// =============================================================================
// Command Line Processing
// =============================================================================

void print_usage(const char *program_name)
{
    printf("FPGA NPU Performance Benchmarking Suite\n");
    printf("=======================================\n\n");
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    
    printf("Benchmark Selection:\n");
    printf("  -a, --all                  Run all benchmarks (default)\n");
    printf("  -t, --throughput           Run throughput benchmarks\n");
    printf("  -l, --latency              Run latency benchmarks\n");
    printf("  -s, --scalability          Run scalability benchmarks\n");
    printf("  -p, --power                Run power efficiency benchmarks\n");
    printf("  -b, --benchmark NAME       Run specific benchmark\n");
    printf("\n");
    
    printf("Benchmark Configuration:\n");
    printf("  --size SIZE                Benchmark size (small, medium, large, xlarge)\n");
    printf("  --iterations N             Number of iterations (default: benchmark-specific)\n");
    printf("  --warmup N                 Warmup iterations (default: benchmark-specific)\n");
    printf("  --threads N                Thread count for scalability tests (default: 4)\n");
    printf("\n");
    
    printf("Monitoring Options:\n");
    printf("  --enable-power             Enable power monitoring\n");
    printf("  --enable-thermal           Enable thermal monitoring\n");
    printf("\n");
    
    printf("Output Options:\n");
    printf("  -v, --verbose              Enable verbose output\n");
    printf("  -o, --output DIR           Output directory (default: %s)\n", g_config.output_directory);
    printf("  --log FILE                 Log file path (default: stdout)\n");
    printf("  --csv                      Generate CSV report (default: enabled)\n");
    printf("  --json                     Generate JSON report\n");
    printf("  --no-csv                   Disable CSV report\n");
    printf("\n");
    
    printf("Other Options:\n");
    printf("  -h, --help                 Show this help message\n");
    printf("\n");
    
    printf("Available Benchmarks:\n");
    printf("  Name                      | Type         | Description\n");
    printf("  --------------------------|--------------|----------------------------------\n");
    for (size_t i = 0; i < g_num_benchmarks; i++) {
        const char *type_str;
        switch (g_benchmark_definitions[i].type) {
            case BENCHMARK_TYPE_THROUGHPUT: type_str = "Throughput"; break;
            case BENCHMARK_TYPE_LATENCY: type_str = "Latency"; break;
            case BENCHMARK_TYPE_SCALABILITY: type_str = "Scalability"; break;
            case BENCHMARK_TYPE_POWER_EFFICIENCY: type_str = "Power"; break;
            case BENCHMARK_TYPE_MEMORY_BANDWIDTH: type_str = "Memory"; break;
            default: type_str = "Unknown"; break;
        }
        printf("  %-25s | %-12s | %s\n",
               g_benchmark_definitions[i].name, type_str, g_benchmark_definitions[i].description);
    }
    printf("\n");
    
    printf("Examples:\n");
    printf("  %s                                    # Run all benchmarks\n", program_name);
    printf("  %s -t --size large                    # Run throughput benchmarks, large size\n", program_name);
    printf("  %s -b matmul_throughput --iterations 500  # Run specific benchmark\n", program_name);
    printf("  %s -p --enable-power --enable-thermal     # Run power benchmarks with monitoring\n", program_name);
    printf("\n");
}

benchmark_size_t parse_benchmark_size(const char *size_str)
{
    if (strcmp(size_str, "small") == 0) return BENCHMARK_SIZE_SMALL;
    if (strcmp(size_str, "medium") == 0) return BENCHMARK_SIZE_MEDIUM;
    if (strcmp(size_str, "large") == 0) return BENCHMARK_SIZE_LARGE;
    if (strcmp(size_str, "xlarge") == 0) return BENCHMARK_SIZE_XLARGE;
    
    fprintf(stderr, "Invalid benchmark size: %s\n", size_str);
    return BENCHMARK_SIZE_MEDIUM; // Default
}

int parse_command_line(int argc, char *argv[])
{
    static struct option long_options[] = {
        {"all",             no_argument,       0, 'a'},
        {"throughput",      no_argument,       0, 't'},
        {"latency",         no_argument,       0, 'l'},
        {"scalability",     no_argument,       0, 's'},
        {"power",           no_argument,       0, 'p'},
        {"benchmark",       required_argument, 0, 'b'},
        {"size",            required_argument, 0, 1001},
        {"iterations",      required_argument, 0, 1002},
        {"warmup",          required_argument, 0, 1003},
        {"threads",         required_argument, 0, 1004},
        {"enable-power",    no_argument,       0, 1005},
        {"enable-thermal",  no_argument,       0, 1006},
        {"verbose",         no_argument,       0, 'v'},
        {"output",          required_argument, 0, 'o'},
        {"log",             required_argument, 0, 1007},
        {"csv",             no_argument,       0, 1008},
        {"json",            no_argument,       0, 1009},
        {"no-csv",          no_argument,       0, 1010},
        {"help",            no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    int c;
    int option_index = 0;
    
    while ((c = getopt_long(argc, argv, "atlspb:vo:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 'a':
                g_config.run_all_benchmarks = true;
                break;
                
            case 't':
                g_config.run_throughput_benchmarks = true;
                g_config.run_all_benchmarks = false;
                break;
                
            case 'l':
                g_config.run_latency_benchmarks = true;
                g_config.run_all_benchmarks = false;
                break;
                
            case 's':
                g_config.run_scalability_benchmarks = true;
                g_config.run_all_benchmarks = false;
                break;
                
            case 'p':
                g_config.run_power_benchmarks = true;
                g_config.run_all_benchmarks = false;
                break;
                
            case 'b':
                strncpy(g_config.specific_benchmark, optarg, sizeof(g_config.specific_benchmark) - 1);
                g_config.run_all_benchmarks = false;
                break;
                
            case 1001: // --size
                g_config.benchmark_size = parse_benchmark_size(optarg);
                break;
                
            case 1002: // --iterations
                g_config.iterations = (uint32_t)atoi(optarg);
                break;
                
            case 1003: // --warmup
                g_config.warmup_iterations = (uint32_t)atoi(optarg);
                break;
                
            case 1004: // --threads
                g_config.thread_count = (uint32_t)atoi(optarg);
                break;
                
            case 1005: // --enable-power
                g_config.enable_power_monitoring = true;
                break;
                
            case 1006: // --enable-thermal
                g_config.enable_thermal_monitoring = true;
                break;
                
            case 'v':
                g_config.verbose_output = true;
                break;
                
            case 'o':
                strncpy(g_config.output_directory, optarg, sizeof(g_config.output_directory) - 1);
                break;
                
            case 1007: // --log
                strncpy(g_config.log_file, optarg, sizeof(g_config.log_file) - 1);
                break;
                
            case 1008: // --csv
                g_config.generate_csv_report = true;
                break;
                
            case 1009: // --json
                g_config.generate_json_report = true;
                break;
                
            case 1010: // --no-csv
                g_config.generate_csv_report = false;
                break;
                
            case 'h':
                g_config.help_requested = true;
                return 0;
                
            case '?':
                return -1;
                
            default:
                fprintf(stderr, "Unknown option\n");
                return -1;
        }
    }
    
    return 0;
}

// =============================================================================
// Benchmark Execution
// =============================================================================

bool should_run_benchmark(const benchmark_definition_t *benchmark)
{
    if (g_config.run_all_benchmarks) return true;
    
    if (strlen(g_config.specific_benchmark) > 0) {
        return strcmp(benchmark->name, g_config.specific_benchmark) == 0;
    }
    
    if (g_config.run_throughput_benchmarks && 
        (benchmark->type == BENCHMARK_TYPE_THROUGHPUT || benchmark->type == BENCHMARK_TYPE_MEMORY_BANDWIDTH)) {
        return true;
    }
    
    if (g_config.run_latency_benchmarks && benchmark->type == BENCHMARK_TYPE_LATENCY) {
        return true;
    }
    
    if (g_config.run_scalability_benchmarks && benchmark->type == BENCHMARK_TYPE_SCALABILITY) {
        return true;
    }
    
    if (g_config.run_power_benchmarks && benchmark->type == BENCHMARK_TYPE_POWER_EFFICIENCY) {
        return true;
    }
    
    return false;
}

int run_benchmark_suite(npu_handle_t npu_handle)
{
    size_t benchmarks_run = 0;
    size_t benchmarks_passed = 0;
    size_t benchmarks_failed = 0;
    
    printf("Starting NPU Performance Benchmark Suite\n");
    printf("==========================================\n\n");
    
    for (size_t i = 0; i < g_num_benchmarks; i++) {
        const benchmark_definition_t *benchmark = &g_benchmark_definitions[i];
        
        if (!should_run_benchmark(benchmark)) {
            continue;
        }
        
        // Skip power benchmarks if power monitoring not enabled
        if (benchmark->requires_power_monitoring && !g_config.enable_power_monitoring) {
            printf("Skipping %s (requires power monitoring)\n", benchmark->name);
            continue;
        }
        
        benchmarks_run++;
        
        printf("Running benchmark: %s\n", benchmark->name);
        printf("Description: %s\n", benchmark->description);
        printf("----------------------------------------\n");
        
        // Setup benchmark configuration
        benchmark_config_t config = {
            .type = benchmark->type,
            .size = g_config.benchmark_size,
            .iterations = g_config.iterations ? g_config.iterations : benchmark->default_iterations,
            .warmup_iterations = g_config.warmup_iterations ? g_config.warmup_iterations : benchmark->default_warmup,
            .enable_power_monitoring = g_config.enable_power_monitoring,
            .enable_thermal_monitoring = g_config.enable_thermal_monitoring,
            .thread_count = g_config.thread_count,
            .target_duration_sec = 0.0 // Let benchmark determine
        };
        
        // Create benchmark context
        performance_metrics_t metrics = {0};
        benchmark_context_t ctx = {
            .config = config,
            .npu_handle = npu_handle,
            .result = &metrics
        };
        
        // Execute benchmark
        struct timespec start_time, end_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        
        int result = benchmark->function(&ctx);
        
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        
        // Report results
        if (result == 0) {
            benchmarks_passed++;
            printf("BENCHMARK PASSED\n");
        } else {
            benchmarks_failed++;
            printf("BENCHMARK FAILED\n");
        }
        
        printf("Execution time: %.3f seconds\n", calculate_duration_seconds(start_time, end_time));
        printf("\n");
    }
    
    // Print summary
    printf("Benchmark Suite Summary\n");
    printf("=======================\n");
    printf("Benchmarks run: %zu\n", benchmarks_run);
    printf("Benchmarks passed: %zu\n", benchmarks_passed);
    printf("Benchmarks failed: %zu\n", benchmarks_failed);
    printf("Success rate: %.1f%%\n", benchmarks_run > 0 ? (double)benchmarks_passed / benchmarks_run * 100.0 : 0.0);
    
    return (benchmarks_failed == 0) ? 0 : -1;
}

// =============================================================================
// Main Function
// =============================================================================

int main(int argc, char *argv[])
{
    int result = 0;
    
    // Parse command line arguments
    if (parse_command_line(argc, argv) != 0) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    if (g_config.help_requested) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }
    
    // Print banner
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                                                              ║\n");
    printf("║              FPGA NPU Performance Benchmark Suite           ║\n");
    printf("║                                                              ║\n");
    printf("║  Comprehensive throughput, latency, scalability, and        ║\n");
    printf("║  power efficiency testing for FPGA-based NPU acceleration   ║\n");
    printf("║                                                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
    
    // Display configuration
    printf("Benchmark Configuration:\n");
    printf("  Benchmark types:    ");
    if (g_config.run_all_benchmarks) printf("All");
    else {
        bool first = true;
        if (g_config.run_throughput_benchmarks) { printf("Throughput"); first = false; }
        if (g_config.run_latency_benchmarks) { printf("%sLatency", first ? "" : ", "); first = false; }
        if (g_config.run_scalability_benchmarks) { printf("%sScalability", first ? "" : ", "); first = false; }
        if (g_config.run_power_benchmarks) { printf("%sPower", first ? "" : ", "); first = false; }
        if (strlen(g_config.specific_benchmark) > 0) { printf("%s%s", first ? "" : ", ", g_config.specific_benchmark); }
    }
    printf("\n");
    printf("  Benchmark size:     %s\n", 
           g_config.benchmark_size == BENCHMARK_SIZE_SMALL ? "Small" :
           g_config.benchmark_size == BENCHMARK_SIZE_MEDIUM ? "Medium" :
           g_config.benchmark_size == BENCHMARK_SIZE_LARGE ? "Large" : "XLarge");
    printf("  Thread count:       %u\n", g_config.thread_count);
    printf("  Power monitoring:   %s\n", g_config.enable_power_monitoring ? "Enabled" : "Disabled");
    printf("  Thermal monitoring: %s\n", g_config.enable_thermal_monitoring ? "Enabled" : "Disabled");
    printf("  Verbose output:     %s\n", g_config.verbose_output ? "Enabled" : "Disabled");
    printf("  Output directory:   %s\n", g_config.output_directory);
    printf("\n");
    
    // Initialize benchmark framework
    if (initialize_benchmark_framework() != 0) {
        fprintf(stderr, "Failed to initialize benchmark framework\n");
        return EXIT_FAILURE;
    }
    
    // Initialize NPU
    npu_handle_t npu_handle;
    if (npu_init(&npu_handle) != NPU_SUCCESS) {
        fprintf(stderr, "Failed to initialize NPU\n");
        cleanup_benchmark_framework();
        return EXIT_FAILURE;
    }
    
    // Create output directory
    if (mkdir(g_config.output_directory, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Failed to create output directory: %s\n", g_config.output_directory);
    }
    
    // Run benchmark suite
    result = run_benchmark_suite(npu_handle);
    
    // Cleanup
    npu_cleanup(npu_handle);
    cleanup_benchmark_framework();
    
    if (result == 0) {
        printf("🎉 All benchmarks completed successfully! 🎉\n");
    } else {
        printf("❌ Some benchmarks failed. Check the output for details.\n");
    }
    
    printf("Benchmark results saved to: %s\n", g_config.output_directory);
    
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}