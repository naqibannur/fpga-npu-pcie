#!/bin/bash

# =============================================================================
# FPGA NPU Simulation Test Execution Script
# 
# Comprehensive test execution with multiple modes, reporting, and CI integration
# =============================================================================

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "${SCRIPT_DIR}")")"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Default configuration
DEFAULT_SIMULATOR="vivado"
DEFAULT_TEST_MODE="full"
DEFAULT_COVERAGE=1
DEFAULT_ASSERTIONS=1
DEFAULT_PERFORMANCE=1
DEFAULT_TIMEOUT=1000000
DEFAULT_PARALLEL=0
DEFAULT_VERBOSE=0
DEFAULT_CLEAN=0

# Current configuration (will be updated by command line args)
SIMULATOR="${DEFAULT_SIMULATOR}"
TEST_MODE="${DEFAULT_TEST_MODE}"
ENABLE_COVERAGE="${DEFAULT_COVERAGE}"
ENABLE_ASSERTIONS="${DEFAULT_ASSERTIONS}"
ENABLE_PERFORMANCE="${DEFAULT_PERFORMANCE}"
TEST_TIMEOUT="${DEFAULT_TIMEOUT}"
PARALLEL_EXECUTION="${DEFAULT_PARALLEL}"
VERBOSE="${DEFAULT_VERBOSE}"
CLEAN_BEFORE_RUN="${DEFAULT_CLEAN}"

# Output directories
RESULTS_DIR="${SCRIPT_DIR}/results_${TIMESTAMP}"
LOGS_DIR="${RESULTS_DIR}/logs"
COVERAGE_DIR="${RESULTS_DIR}/coverage"
REPORTS_DIR="${RESULTS_DIR}/reports"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# =============================================================================
# Utility Functions
# =============================================================================

print_banner() {
    echo -e "${BLUE}"
    echo "=================================================================="
    echo "    FPGA NPU Simulation Test Execution Script"
    echo "=================================================================="
    echo -e "${NC}"
}

print_status() {
    echo -e "${CYAN}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_section() {
    echo ""
    echo -e "${PURPLE}=== $1 ===${NC}"
}

# Check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Setup directories
setup_directories() {
    print_status "Setting up output directories..."
    mkdir -p "${RESULTS_DIR}" "${LOGS_DIR}" "${COVERAGE_DIR}" "${REPORTS_DIR}"
    
    # Create symlink to latest results
    if [[ -L "${SCRIPT_DIR}/latest" ]]; then
        rm "${SCRIPT_DIR}/latest"
    fi
    ln -sf "results_${TIMESTAMP}" "${SCRIPT_DIR}/latest"
    
    print_success "Directories created: ${RESULTS_DIR}"
}

# Check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    local missing_deps=()
    
    # Check simulator
    case "${SIMULATOR}" in
        vivado)
            if ! command_exists xsim; then
                missing_deps+=("Vivado (xsim not found)")
            fi
            ;;
        modelsim|questa)
            if ! command_exists vsim; then
                missing_deps+=("ModelSim/Questa (vsim not found)")
            fi
            ;;
        vcs)
            if ! command_exists vcs; then
                missing_deps+=("VCS (vcs not found)")
            fi
            ;;
        verilator)
            if ! command_exists verilator; then
                missing_deps+=("Verilator (verilator not found)")
            fi
            ;;
        *)
            missing_deps+=("Unknown simulator: ${SIMULATOR}")
            ;;
    esac
    
    # Check other tools
    if ! command_exists make; then
        missing_deps+=("make")
    fi
    
    # Report missing dependencies
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        print_error "Missing dependencies:"
        for dep in "${missing_deps[@]}"; do
            echo "  - ${dep}"
        done
        exit 1
    fi
    
    print_success "All dependencies satisfied"
}

# =============================================================================
# Test Execution Functions
# =============================================================================

# Run quick tests
run_quick_tests() {
    print_section "Running Quick Tests"
    
    local make_args=(
        "test-quick"
        "SIM=${SIMULATOR}"
        "ENABLE_COVERAGE=0"
        "ENABLE_ASSERTIONS=${ENABLE_ASSERTIONS}"
        "ENABLE_PERFORMANCE_MONITORING=0"
        "TEST_TIMEOUT=100000"
    )
    
    if [[ "${VERBOSE}" -eq 1 ]]; then
        make_args+=("V=1")
    fi
    
    print_status "Executing: make ${make_args[*]}"
    
    if make "${make_args[@]}" 2>&1 | tee "${LOGS_DIR}/quick_tests.log"; then
        print_success "Quick tests completed successfully"
        return 0
    else
        print_error "Quick tests failed"
        return 1
    fi
}

# Run full tests
run_full_tests() {
    print_section "Running Full Tests"
    
    local make_args=(
        "test-full"
        "SIM=${SIMULATOR}"
        "ENABLE_COVERAGE=${ENABLE_COVERAGE}"
        "ENABLE_ASSERTIONS=${ENABLE_ASSERTIONS}"
        "ENABLE_PERFORMANCE_MONITORING=${ENABLE_PERFORMANCE}"
        "TEST_TIMEOUT=${TEST_TIMEOUT}"
    )
    
    if [[ "${VERBOSE}" -eq 1 ]]; then
        make_args+=("V=1")
    fi
    
    print_status "Executing: make ${make_args[*]}"
    
    if make "${make_args[@]}" 2>&1 | tee "${LOGS_DIR}/full_tests.log"; then
        print_success "Full tests completed successfully"
        return 0
    else
        print_error "Full tests failed"
        return 1
    fi
}

# Run regression tests
run_regression_tests() {
    print_section "Running Regression Tests"
    
    local simulators=("vivado" "modelsim" "vcs")
    local failed_sims=()
    
    for sim in "${simulators[@]}"; do
        if command_exists "$(get_simulator_command "${sim}")"; then
            print_status "Running regression with ${sim}..."
            
            local make_args=(
                "test-full"
                "SIM=${sim}"
                "ENABLE_COVERAGE=${ENABLE_COVERAGE}"
                "ENABLE_ASSERTIONS=${ENABLE_ASSERTIONS}"
                "ENABLE_PERFORMANCE_MONITORING=${ENABLE_PERFORMANCE}"
                "TEST_TIMEOUT=${TEST_TIMEOUT}"
            )
            
            if make "${make_args[@]}" 2>&1 | tee "${LOGS_DIR}/regression_${sim}.log"; then
                print_success "Regression with ${sim} completed successfully"
            else
                print_error "Regression with ${sim} failed"
                failed_sims+=("${sim}")
            fi
        else
            print_warning "Skipping ${sim} (not available)"
        fi
    done
    
    if [[ ${#failed_sims[@]} -eq 0 ]]; then
        print_success "All regression tests completed successfully"
        return 0
    else
        print_error "Regression tests failed for: ${failed_sims[*]}"
        return 1
    fi
}

# Run stress tests
run_stress_tests() {
    print_section "Running Stress Tests"
    
    local make_args=(
        "test-full"
        "SIM=${SIMULATOR}"
        "ENABLE_COVERAGE=${ENABLE_COVERAGE}"
        "ENABLE_ASSERTIONS=${ENABLE_ASSERTIONS}"
        "ENABLE_PERFORMANCE_MONITORING=1"
        "TEST_TIMEOUT=$((TEST_TIMEOUT * 5))"  # Longer timeout for stress tests
    )
    
    print_status "Executing: make ${make_args[*]}"
    
    if make "${make_args[@]}" 2>&1 | tee "${LOGS_DIR}/stress_tests.log"; then
        print_success "Stress tests completed successfully"
        return 0
    else
        print_error "Stress tests failed"
        return 1
    fi
}

# Get simulator command
get_simulator_command() {
    case "$1" in
        vivado) echo "xsim" ;;
        modelsim|questa) echo "vsim" ;;
        vcs) echo "vcs" ;;
        verilator) echo "verilator" ;;
        *) echo "unknown" ;;
    esac
}

# =============================================================================
# Analysis and Reporting Functions
# =============================================================================

# Generate coverage report
generate_coverage_report() {
    if [[ "${ENABLE_COVERAGE}" -eq 1 ]]; then
        print_section "Generating Coverage Report"
        
        if make coverage-report 2>&1 | tee "${LOGS_DIR}/coverage_report.log"; then
            print_success "Coverage report generated"
            
            # Copy coverage files to results directory
            if [[ -d "coverage" ]]; then
                cp -r coverage/* "${COVERAGE_DIR}/" 2>/dev/null || true
            fi
        else
            print_warning "Coverage report generation failed"
        fi
    else
        print_status "Coverage collection disabled, skipping coverage report"
    fi
}

# Generate performance report
generate_performance_report() {
    if [[ "${ENABLE_PERFORMANCE}" -eq 1 ]]; then
        print_section "Generating Performance Report"
        
        if make performance 2>&1 | tee "${LOGS_DIR}/performance_report.log"; then
            print_success "Performance report generated"
        else
            print_warning "Performance report generation failed"
        fi
    else
        print_status "Performance monitoring disabled, skipping performance report"
    fi
}

# Generate comprehensive report
generate_comprehensive_report() {
    print_section "Generating Comprehensive Report"
    
    local report_file="${REPORTS_DIR}/comprehensive_report.html"
    
    # Create HTML report
    cat > "${report_file}" << EOF
<!DOCTYPE html>
<html>
<head>
    <title>FPGA NPU Simulation Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background-color: #f0f0f0; padding: 20px; border-radius: 5px; }
        .section { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
        .success { color: green; }
        .error { color: red; }
        .warning { color: orange; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        pre { background-color: #f8f8f8; padding: 10px; border-radius: 3px; overflow-x: auto; }
    </style>
</head>
<body>
    <div class="header">
        <h1>FPGA NPU Simulation Test Report</h1>
        <p><strong>Generated:</strong> $(date)</p>
        <p><strong>Simulator:</strong> ${SIMULATOR}</p>
        <p><strong>Test Mode:</strong> ${TEST_MODE}</p>
    </div>
    
    <div class="section">
        <h2>Test Configuration</h2>
        <table>
            <tr><th>Parameter</th><th>Value</th></tr>
            <tr><td>Simulator</td><td>${SIMULATOR}</td></tr>
            <tr><td>Test Mode</td><td>${TEST_MODE}</td></tr>
            <tr><td>Coverage Enabled</td><td>${ENABLE_COVERAGE}</td></tr>
            <tr><td>Assertions Enabled</td><td>${ENABLE_ASSERTIONS}</td></tr>
            <tr><td>Performance Monitoring</td><td>${ENABLE_PERFORMANCE}</td></tr>
            <tr><td>Test Timeout</td><td>${TEST_TIMEOUT}</td></tr>
            <tr><td>Parallel Execution</td><td>${PARALLEL_EXECUTION}</td></tr>
        </table>
    </div>
    
    <div class="section">
        <h2>Test Results</h2>
        <p>Test execution logs and detailed results are available in the logs directory.</p>
        <ul>
EOF

    # Add links to log files
    for log_file in "${LOGS_DIR}"/*.log; do
        if [[ -f "${log_file}" ]]; then
            local base_name=$(basename "${log_file}")
            echo "            <li><a href=\"logs/${base_name}\">${base_name}</a></li>" >> "${report_file}"
        fi
    done

    cat >> "${report_file}" << EOF
        </ul>
    </div>
    
    <div class="section">
        <h2>Coverage Analysis</h2>
EOF

    if [[ "${ENABLE_COVERAGE}" -eq 1 ]] && [[ -d "${COVERAGE_DIR}" ]]; then
        echo "        <p>Coverage reports are available in the coverage directory.</p>" >> "${report_file}"
        echo "        <ul>" >> "${report_file}"
        for cov_file in "${COVERAGE_DIR}"/*.html; do
            if [[ -f "${cov_file}" ]]; then
                local base_name=$(basename "${cov_file}")
                echo "            <li><a href=\"coverage/${base_name}\">${base_name}</a></li>" >> "${report_file}"
            fi
        done
        echo "        </ul>" >> "${report_file}"
    else
        echo "        <p>Coverage collection was disabled for this run.</p>" >> "${report_file}"
    fi

    cat >> "${report_file}" << EOF
    </div>
    
    <div class="section">
        <h2>Performance Analysis</h2>
EOF

    if [[ "${ENABLE_PERFORMANCE}" -eq 1 ]]; then
        echo "        <p>Performance monitoring was enabled. See logs for detailed metrics.</p>" >> "${report_file}"
    else
        echo "        <p>Performance monitoring was disabled for this run.</p>" >> "${report_file}"
    fi

    cat >> "${report_file}" << EOF
    </div>
    
    <div class="section">
        <h2>Environment Information</h2>
        <pre>
Hostname: $(hostname)
User: $(whoami)
Date: $(date)
Working Directory: $(pwd)
Git Commit: $(git rev-parse HEAD 2>/dev/null || echo "Not available")
Git Branch: $(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "Not available")
        </pre>
    </div>
</body>
</html>
EOF

    print_success "Comprehensive report generated: ${report_file}"
}

# =============================================================================
# Main Execution Function
# =============================================================================

run_tests() {
    print_banner
    
    # Setup
    setup_directories
    check_dependencies
    
    # Clean if requested
    if [[ "${CLEAN_BEFORE_RUN}" -eq 1 ]]; then
        print_status "Cleaning previous build artifacts..."
        make clean
    fi
    
    # Change to simulation directory
    cd "${SCRIPT_DIR}"
    
    # Execute tests based on mode
    local test_result=0
    
    case "${TEST_MODE}" in
        quick)
            run_quick_tests || test_result=1
            ;;
        full)
            run_full_tests || test_result=1
            ;;
        regression)
            run_regression_tests || test_result=1
            ;;
        stress)
            run_stress_tests || test_result=1
            ;;
        all)
            run_quick_tests || test_result=1
            run_full_tests || test_result=1
            run_stress_tests || test_result=1
            ;;
        *)
            print_error "Unknown test mode: ${TEST_MODE}"
            test_result=1
            ;;
    esac
    
    # Generate reports
    generate_coverage_report
    generate_performance_report
    generate_comprehensive_report
    
    # Final status
    if [[ ${test_result} -eq 0 ]]; then
        print_success "All tests completed successfully!"
        print_status "Results available in: ${RESULTS_DIR}"
    else
        print_error "Some tests failed!"
        print_status "Check logs in: ${LOGS_DIR}"
        exit 1
    fi
}

# =============================================================================
# Command Line Argument Processing
# =============================================================================

show_help() {
    cat << EOF
FPGA NPU Simulation Test Execution Script

Usage: $0 [OPTIONS]

OPTIONS:
    -s, --simulator SIMULATOR    Simulator to use (vivado, modelsim, vcs, verilator)
                                 Default: ${DEFAULT_SIMULATOR}
    
    -m, --mode MODE              Test mode (quick, full, regression, stress, all)
                                 Default: ${DEFAULT_TEST_MODE}
    
    -c, --coverage BOOL          Enable coverage collection (0|1)
                                 Default: ${DEFAULT_COVERAGE}
    
    -a, --assertions BOOL        Enable assertion checking (0|1)
                                 Default: ${DEFAULT_ASSERTIONS}
    
    -p, --performance BOOL       Enable performance monitoring (0|1)
                                 Default: ${DEFAULT_PERFORMANCE}
    
    -t, --timeout CYCLES         Test timeout in cycles
                                 Default: ${DEFAULT_TIMEOUT}
    
    --parallel                   Enable parallel test execution
    
    --clean                      Clean before running tests
    
    -v, --verbose                Enable verbose output
    
    -h, --help                   Show this help message

EXAMPLES:
    $0                                    # Run default full tests with Vivado
    $0 -s modelsim -m quick              # Quick tests with ModelSim
    $0 -m regression --clean             # Clean regression tests
    $0 -s verilator -c 0 -p 0            # Verilator without coverage/performance
    $0 -m all -v                         # All test modes with verbose output

EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -s|--simulator)
            SIMULATOR="$2"
            shift 2
            ;;
        -m|--mode)
            TEST_MODE="$2"
            shift 2
            ;;
        -c|--coverage)
            ENABLE_COVERAGE="$2"
            shift 2
            ;;
        -a|--assertions)
            ENABLE_ASSERTIONS="$2"
            shift 2
            ;;
        -p|--performance)
            ENABLE_PERFORMANCE="$2"
            shift 2
            ;;
        -t|--timeout)
            TEST_TIMEOUT="$2"
            shift 2
            ;;
        --parallel)
            PARALLEL_EXECUTION=1
            shift
            ;;
        --clean)
            CLEAN_BEFORE_RUN=1
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# =============================================================================
# Main Entry Point
# =============================================================================

# Validate arguments
case "${SIMULATOR}" in
    vivado|modelsim|questa|vcs|verilator) ;;
    *) print_error "Invalid simulator: ${SIMULATOR}"; exit 1 ;;
esac

case "${TEST_MODE}" in
    quick|full|regression|stress|all) ;;
    *) print_error "Invalid test mode: ${TEST_MODE}"; exit 1 ;;
esac

# Run the tests
run_tests