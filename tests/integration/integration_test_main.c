/**
 * Integration Test Main Runner
 * 
 * Main application for executing comprehensive integration tests
 * with command-line configuration and detailed reporting
 */

#include "integration_test_framework.h"
#include <getopt.h>

// External test suite creation functions
extern test_suite_t* create_e2e_test_suite(void);
extern test_suite_t* create_stress_test_suite(void);

// =============================================================================
// Configuration and Command Line Options
// =============================================================================

typedef struct {
    bool run_e2e_tests;
    bool run_stress_tests;
    bool run_all_tests;
    bool verbose_output;
    bool stop_on_failure;
    bool generate_html_report;
    bool generate_json_report;
    char output_directory[256];
    char log_file[256];
    int test_timeout;
    bool help_requested;
} test_config_options_t;

static test_config_options_t g_config = {
    .run_e2e_tests = false,
    .run_stress_tests = false,
    .run_all_tests = true,
    .verbose_output = false,
    .stop_on_failure = false,
    .generate_html_report = true,
    .generate_json_report = false,
    .output_directory = "./test_results",
    .log_file = "",
    .test_timeout = DEFAULT_TEST_TIMEOUT,
    .help_requested = false
};

// =============================================================================
// Command Line Processing
// =============================================================================

void print_usage(const char *program_name)
{
    printf("FPGA NPU Integration Test Suite\n");
    printf("===============================\n\n");
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Test Selection Options:\n");
    printf("  -e, --e2e              Run end-to-end integration tests\n");
    printf("  -s, --stress           Run stress and reliability tests\n");
    printf("  -a, --all              Run all test suites (default)\n");
    printf("\n");
    printf("Execution Options:\n");
    printf("  -v, --verbose          Enable verbose output\n");
    printf("  -f, --stop-on-failure  Stop execution on first test failure\n");
    printf("  -t, --timeout SECONDS  Set test timeout (default: %d)\n", DEFAULT_TEST_TIMEOUT);
    printf("\n");
    printf("Output Options:\n");
    printf("  -o, --output DIR       Output directory for reports (default: %s)\n", g_config.output_directory);
    printf("  -l, --log FILE         Log file path (default: stdout)\n");
    printf("  --html                 Generate HTML report (default: enabled)\n");
    printf("  --json                 Generate JSON report (default: disabled)\n");
    printf("  --no-html              Disable HTML report generation\n");
    printf("\n");
    printf("Other Options:\n");
    printf("  -h, --help             Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                     # Run all tests with default settings\n", program_name);
    printf("  %s -e -v               # Run E2E tests with verbose output\n", program_name);
    printf("  %s -s -o ./results     # Run stress tests, output to ./results\n", program_name);
    printf("  %s -a -f --json        # Run all tests, stop on failure, generate JSON\n", program_name);
    printf("\n");
}

int parse_command_line(int argc, char *argv[])
{
    static struct option long_options[] = {
        {"e2e",             no_argument,       0, 'e'},
        {"stress",          no_argument,       0, 's'},
        {"all",             no_argument,       0, 'a'},
        {"verbose",         no_argument,       0, 'v'},
        {"stop-on-failure", no_argument,       0, 'f'},
        {"timeout",         required_argument, 0, 't'},
        {"output",          required_argument, 0, 'o'},
        {"log",             required_argument, 0, 'l'},
        {"html",            no_argument,       0, 1001},
        {"json",            no_argument,       0, 1002},
        {"no-html",         no_argument,       0, 1003},
        {"help",            no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    int c;
    int option_index = 0;
    
    while ((c = getopt_long(argc, argv, "esavft:o:l:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 'e':
                g_config.run_e2e_tests = true;
                g_config.run_all_tests = false;
                break;
                
            case 's':
                g_config.run_stress_tests = true;
                g_config.run_all_tests = false;
                break;
                
            case 'a':
                g_config.run_all_tests = true;
                break;
                
            case 'v':
                g_config.verbose_output = true;
                break;
                
            case 'f':
                g_config.stop_on_failure = true;
                break;
                
            case 't':
                g_config.test_timeout = atoi(optarg);
                if (g_config.test_timeout <= 0) {
                    fprintf(stderr, "Error: Invalid timeout value: %s\n", optarg);
                    return -1;
                }
                break;
                
            case 'o':
                strncpy(g_config.output_directory, optarg, sizeof(g_config.output_directory) - 1);
                g_config.output_directory[sizeof(g_config.output_directory) - 1] = '\0';
                break;
                
            case 'l':
                strncpy(g_config.log_file, optarg, sizeof(g_config.log_file) - 1);
                g_config.log_file[sizeof(g_config.log_file) - 1] = '\0';
                break;
                
            case 1001: // --html
                g_config.generate_html_report = true;
                break;
                
            case 1002: // --json
                g_config.generate_json_report = true;
                break;
                
            case 1003: // --no-html
                g_config.generate_html_report = false;
                break;
                
            case 'h':
                g_config.help_requested = true;
                return 0;
                
            case '?':
                return -1;
                
            default:
                fprintf(stderr, "Error: Unknown option\n");
                return -1;
        }
    }
    
    // If neither specific test type is selected, run all
    if (!g_config.run_e2e_tests && !g_config.run_stress_tests) {
        g_config.run_all_tests = true;
    }
    
    return 0;
}

// =============================================================================
// Test Environment Setup
// =============================================================================

int setup_test_environment(void)
{
    // Create output directory
    if (mkdir(g_config.output_directory, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error: Failed to create output directory: %s\n", 
                g_config.output_directory);
        return -1;
    }
    
    // Initialize logging
    if (strlen(g_config.log_file) > 0) {
        if (init_test_logging(g_config.log_file) != 0) {
            fprintf(stderr, "Warning: Failed to initialize log file: %s\n", 
                    g_config.log_file);
        }
    }
    
    // Set global configuration
    g_verbose_output = g_config.verbose_output;
    g_stop_on_first_failure = g_config.stop_on_failure;
    
    return 0;
}

// =============================================================================
// Report Generation
// =============================================================================

int generate_reports(void)
{
    char report_path[512];
    int result = 0;
    
    // Generate HTML report
    if (g_config.generate_html_report) {
        snprintf(report_path, sizeof(report_path), "%s/test_report.html", 
                g_config.output_directory);
        
        if (generate_html_report(report_path, &g_test_stats) == 0) {
            printf("HTML report generated: %s\n", report_path);
        } else {
            fprintf(stderr, "Warning: Failed to generate HTML report\n");
            result = -1;
        }
    }
    
    // Generate JSON report
    if (g_config.generate_json_report) {
        snprintf(report_path, sizeof(report_path), "%s/test_report.json", 
                g_config.output_directory);
        
        if (generate_json_report(report_path, &g_test_stats) == 0) {
            printf("JSON report generated: %s\n", report_path);
        } else {
            fprintf(stderr, "Warning: Failed to generate JSON report\n");
            result = -1;
        }
    }
    
    return result;
}

// =============================================================================
// Test Suite Execution
// =============================================================================

int run_test_suites(void)
{
    int overall_result = 0;
    struct timespec start_time, end_time;
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Run end-to-end tests
    if (g_config.run_all_tests || g_config.run_e2e_tests) {
        printf(COLOR_CYAN "\n╔══════════════════════════════════════════════════════════════╗\n");
        printf("║                    End-to-End Integration Tests              ║\n");
        printf("╚══════════════════════════════════════════════════════════════╝" COLOR_RESET "\n");
        
        test_suite_t *e2e_suite = create_e2e_test_suite();
        if (e2e_suite) {
            if (execute_test_suite(e2e_suite) != 0) {
                overall_result = -1;
                if (g_config.stop_on_failure) {
                    free(e2e_suite);
                    return overall_result;
                }
            }
            free(e2e_suite);
        } else {
            fprintf(stderr, "Error: Failed to create E2E test suite\n");
            overall_result = -1;
        }
    }
    
    // Run stress tests
    if (g_config.run_all_tests || g_config.run_stress_tests) {
        printf(COLOR_CYAN "\n╔══════════════════════════════════════════════════════════════╗\n");
        printf("║                    Stress and Reliability Tests             ║\n");
        printf("╚══════════════════════════════════════════════════════════════╝" COLOR_RESET "\n");
        
        test_suite_t *stress_suite = create_stress_test_suite();
        if (stress_suite) {
            if (execute_test_suite(stress_suite) != 0) {
                overall_result = -1;
                if (g_config.stop_on_failure) {
                    free(stress_suite);
                    return overall_result;
                }
            }
            free(stress_suite);
        } else {
            fprintf(stderr, "Error: Failed to create stress test suite\n");
            overall_result = -1;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    g_test_stats.total_duration = calculate_duration_seconds(start_time, end_time);
    
    return overall_result;
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
    printf(COLOR_BLUE);
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                                                              ║\n");
    printf("║              FPGA NPU Integration Test Suite                 ║\n");
    printf("║                                                              ║\n");
    printf("║  Comprehensive end-to-end validation and stress testing     ║\n");
    printf("║                                                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET "\n");
    
    // Display configuration
    printf("Test Configuration:\n");
    printf("  Test Suites:      %s%s%s\n", 
           g_config.run_all_tests ? "All" : "",
           g_config.run_e2e_tests ? "E2E " : "",
           g_config.run_stress_tests ? "Stress " : "");
    printf("  Verbose Output:   %s\n", g_config.verbose_output ? "Enabled" : "Disabled");
    printf("  Stop on Failure:  %s\n", g_config.stop_on_failure ? "Enabled" : "Disabled");
    printf("  Test Timeout:     %d seconds\n", g_config.test_timeout);
    printf("  Output Directory: %s\n", g_config.output_directory);
    printf("  Log File:         %s\n", strlen(g_config.log_file) > 0 ? g_config.log_file : "stdout");
    printf("  HTML Report:      %s\n", g_config.generate_html_report ? "Enabled" : "Disabled");
    printf("  JSON Report:      %s\n", g_config.generate_json_report ? "Enabled" : "Disabled");
    printf("\n");
    
    // Initialize test framework
    if (integration_test_init() != 0) {
        fprintf(stderr, "Error: Failed to initialize test framework\n");
        return EXIT_FAILURE;
    }
    
    // Setup test environment
    if (setup_test_environment() != 0) {
        fprintf(stderr, "Error: Failed to setup test environment\n");
        integration_test_cleanup();
        return EXIT_FAILURE;
    }
    
    // Run system health check
    printf("Performing system health check...\n");
    if (check_system_health() != 0) {
        printf(COLOR_YELLOW "Warning: System health check detected issues" COLOR_RESET "\n");
    } else {
        printf(COLOR_GREEN "System health check passed" COLOR_RESET "\n");
    }
    printf("\n");
    
    // Execute test suites
    result = run_test_suites();
    
    // Print final statistics
    printf(COLOR_WHITE "\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                         Final Results                        ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝" COLOR_RESET "\n");
    
    print_test_statistics(&g_test_stats);
    
    // Generate reports
    printf("Generating test reports...\n");
    generate_reports();
    
    // Final status
    if (result == 0) {
        printf(COLOR_GREEN "\n🎉 ALL TESTS COMPLETED SUCCESSFULLY! 🎉" COLOR_RESET "\n");
        printf("Test execution finished with no failures.\n");
    } else {
        printf(COLOR_RED "\n❌ SOME TESTS FAILED ❌" COLOR_RESET "\n");
        printf("Check the detailed logs and reports for more information.\n");
    }
    
    printf("\nTest reports available in: %s\n", g_config.output_directory);
    
    // Cleanup
    integration_test_cleanup();
    
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}