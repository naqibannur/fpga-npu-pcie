#!/bin/bash

# FPGA NPU Library Unit Test Runner Script
# 
# Comprehensive test execution script with multiple test modes and reporting

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TEST_EXECUTABLE="$BUILD_DIR/npu_unit_tests"
REPORT_DIR="$BUILD_DIR/reports"
LOG_FILE="$REPORT_DIR/test_results.log"

# Default options
RUN_BASIC=true
RUN_MEMORY=false
RUN_COVERAGE=false
RUN_STATIC=false
RUN_BENCHMARK=false
VERBOSE=false
CLEAN_FIRST=false

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
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

# Function to print usage
usage() {
    cat << EOF
FPGA NPU Library Unit Test Runner

Usage: $0 [OPTIONS]

OPTIONS:
    -h, --help          Show this help message
    -v, --verbose       Enable verbose output
    -c, --clean         Clean build artifacts before running tests
    -a, --all           Run all test modes (basic, memory, coverage, static)
    -b, --basic         Run basic unit tests (default)
    -m, --memory        Run tests with memory checking (valgrind)
    -g, --coverage      Generate code coverage report
    -s, --static        Run static analysis
    -p, --benchmark     Run performance benchmarks
    -r, --report        Generate detailed test report

EXAMPLES:
    $0                  # Run basic tests
    $0 -a               # Run all test modes
    $0 -m -g            # Run memory tests and coverage
    $0 -c -v            # Clean build and run with verbose output
    $0 --benchmark      # Run performance benchmarks only

EOF
}

# Function to setup directories
setup_dirs() {
    print_status "Setting up directories..."
    mkdir -p "$REPORT_DIR"
    mkdir -p "$BUILD_DIR/obj"
}

# Function to check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    local missing_deps=()
    
    # Check for required tools
    if ! command -v gcc >/dev/null 2>&1; then
        missing_deps+=("gcc")
    fi
    
    if ! command -v make >/dev/null 2>&1; then
        missing_deps+=("make")
    fi
    
    # Check for optional tools
    if [ "$RUN_MEMORY" = true ] && ! command -v valgrind >/dev/null 2>&1; then
        print_warning "valgrind not found - memory checking will be skipped"
        RUN_MEMORY=false
    fi
    
    if [ "$RUN_STATIC" = true ] && ! command -v cppcheck >/dev/null 2>&1; then
        print_warning "cppcheck not found - static analysis will be skipped"
        RUN_STATIC=false
    fi
    
    if [ "$RUN_COVERAGE" = true ] && ! command -v gcov >/dev/null 2>&1; then
        print_warning "gcov not found - coverage analysis will be skipped"
        RUN_COVERAGE=false
    fi
    
    # Report missing required dependencies
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing required dependencies: ${missing_deps[*]}"
        print_error "Please install missing tools and try again"
        exit 1
    fi
    
    print_success "Dependencies check completed"
}

# Function to build tests
build_tests() {
    print_status "Building unit tests..."
    
    cd "$SCRIPT_DIR"
    
    if [ "$CLEAN_FIRST" = true ]; then
        print_status "Cleaning previous build..."
        make clean
    fi
    
    local build_cmd="make"
    if [ "$VERBOSE" = true ]; then
        build_cmd="$build_cmd V=1"
    fi
    
    if [ "$RUN_COVERAGE" = true ]; then
        build_cmd="make coverage"
    fi
    
    if ! $build_cmd 2>&1 | tee "$REPORT_DIR/build.log"; then
        print_error "Build failed! Check $REPORT_DIR/build.log for details"
        exit 1
    fi
    
    print_success "Build completed successfully"
}

# Function to run basic tests
run_basic_tests() {
    print_status "Running basic unit tests..."
    
    local test_cmd="$TEST_EXECUTABLE"
    local output_file="$REPORT_DIR/basic_tests.log"
    
    if [ "$VERBOSE" = true ]; then
        test_cmd="$test_cmd --verbose"
    fi
    
    echo "=== Basic Unit Tests ===" > "$output_file"
    echo "Date: $(date)" >> "$output_file"
    echo "Command: $test_cmd" >> "$output_file"
    echo "" >> "$output_file"
    
    if $test_cmd 2>&1 | tee -a "$output_file"; then
        print_success "Basic tests passed"
        return 0
    else
        print_error "Basic tests failed! Check $output_file for details"
        return 1
    fi
}

# Function to run memory tests
run_memory_tests() {
    print_status "Running memory tests with valgrind..."
    
    local output_file="$REPORT_DIR/memory_tests.log"
    local valgrind_cmd="valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --xml=yes --xml-file=$REPORT_DIR/valgrind.xml"
    
    echo "=== Memory Tests (Valgrind) ===" > "$output_file"
    echo "Date: $(date)" >> "$output_file"
    echo "Command: $valgrind_cmd $TEST_EXECUTABLE" >> "$output_file"
    echo "" >> "$output_file"
    
    if $valgrind_cmd "$TEST_EXECUTABLE" 2>&1 | tee -a "$output_file"; then
        print_success "Memory tests completed"
        
        # Check for memory leaks
        if grep -q "definitely lost: 0 bytes" "$output_file" && grep -q "possibly lost: 0 bytes" "$output_file"; then
            print_success "No memory leaks detected"
            return 0
        else
            print_warning "Potential memory leaks detected - check $output_file"
            return 1
        fi
    else
        print_error "Memory tests failed! Check $output_file for details"
        return 1
    fi
}

# Function to run static analysis
run_static_analysis() {
    print_status "Running static analysis with cppcheck..."
    
    local output_file="$REPORT_DIR/static_analysis.log"
    
    echo "=== Static Analysis (cppcheck) ===" > "$output_file"
    echo "Date: $(date)" >> "$output_file"
    echo "" >> "$output_file"
    
    cd "$SCRIPT_DIR"
    if make analyze 2>&1 | tee -a "$output_file"; then
        print_success "Static analysis completed"
        return 0
    else
        print_warning "Static analysis found issues - check $output_file"
        return 1
    fi
}

# Function to generate coverage report
generate_coverage() {
    print_status "Generating code coverage report..."
    
    local output_file="$REPORT_DIR/coverage.log"
    
    echo "=== Code Coverage Report ===" > "$output_file"
    echo "Date: $(date)" >> "$output_file"
    echo "" >> "$output_file"
    
    cd "$SCRIPT_DIR"
    
    # Run tests for coverage (already built with coverage flags)
    "$TEST_EXECUTABLE" > /dev/null 2>&1 || true
    
    # Generate coverage files
    if gcov *.c 2>&1 | tee -a "$output_file"; then
        # Move coverage files to report directory
        mv *.gcov "$REPORT_DIR/" 2>/dev/null || true
        
        # Generate summary
        echo "" >> "$output_file"
        echo "Coverage Summary:" >> "$output_file"
        grep -h "Lines executed:" "$REPORT_DIR"/*.gcov >> "$output_file" 2>/dev/null || true
        
        print_success "Coverage report generated in $REPORT_DIR"
        return 0
    else
        print_error "Coverage generation failed! Check $output_file for details"
        return 1
    fi
}

# Function to run benchmarks
run_benchmarks() {
    print_status "Running performance benchmarks..."
    
    local output_file="$REPORT_DIR/benchmarks.log"
    
    echo "=== Performance Benchmarks ===" > "$output_file"
    echo "Date: $(date)" >> "$output_file"
    echo "System: $(uname -a)" >> "$output_file"
    echo "" >> "$output_file"
    
    cd "$SCRIPT_DIR"
    if make benchmark 2>&1 | tee -a "$output_file"; then
        print_success "Benchmarks completed"
        return 0
    else
        print_error "Benchmarks failed! Check $output_file for details"
        return 1
    fi
}

# Function to generate final report
generate_report() {
    print_status "Generating test report..."
    
    local report_file="$REPORT_DIR/test_summary.html"
    
    cat > "$report_file" << EOF
<!DOCTYPE html>
<html>
<head>
    <title>FPGA NPU Library Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background-color: #f0f0f0; padding: 10px; border-radius: 5px; }
        .success { color: green; }
        .warning { color: orange; }
        .error { color: red; }
        .section { margin: 20px 0; border: 1px solid #ddd; padding: 10px; border-radius: 5px; }
        pre { background-color: #f5f5f5; padding: 10px; overflow-x: auto; }
    </style>
</head>
<body>
    <div class="header">
        <h1>FPGA NPU Library Test Report</h1>
        <p>Generated on: $(date)</p>
        <p>System: $(uname -a)</p>
    </div>
EOF

    # Add sections for each test type
    for log_file in "$REPORT_DIR"/*.log; do
        if [ -f "$log_file" ]; then
            local section_name=$(basename "$log_file" .log)
            echo "    <div class=\"section\">" >> "$report_file"
            echo "        <h2>$(echo $section_name | tr '_' ' ' | sed 's/\b\w/\U&/g')</h2>" >> "$report_file"
            echo "        <pre>$(cat "$log_file")</pre>" >> "$report_file"
            echo "    </div>" >> "$report_file"
        fi
    done
    
    echo "</body></html>" >> "$report_file"
    
    print_success "Test report generated: $report_file"
}

# Main execution function
main() {
    print_status "Starting FPGA NPU Library test execution..."
    print_status "Script directory: $SCRIPT_DIR"
    
    setup_dirs
    check_dependencies
    
    # Initialize results
    local basic_result=0
    local memory_result=0
    local static_result=0
    local coverage_result=0
    local benchmark_result=0
    
    # Build tests
    build_tests
    
    # Run selected test modes
    if [ "$RUN_BASIC" = true ]; then
        run_basic_tests
        basic_result=$?
    fi
    
    if [ "$RUN_MEMORY" = true ]; then
        run_memory_tests
        memory_result=$?
    fi
    
    if [ "$RUN_STATIC" = true ]; then
        run_static_analysis
        static_result=$?
    fi
    
    if [ "$RUN_COVERAGE" = true ]; then
        generate_coverage
        coverage_result=$?
    fi
    
    if [ "$RUN_BENCHMARK" = true ]; then
        run_benchmarks
        benchmark_result=$?
    fi
    
    # Generate report if requested
    generate_report
    
    # Summary
    echo ""
    print_status "=== Test Execution Summary ==="
    
    local total_tests=0
    local passed_tests=0
    
    if [ "$RUN_BASIC" = true ]; then
        total_tests=$((total_tests + 1))
        if [ $basic_result -eq 0 ]; then
            print_success "Basic tests: PASSED"
            passed_tests=$((passed_tests + 1))
        else
            print_error "Basic tests: FAILED"
        fi
    fi
    
    if [ "$RUN_MEMORY" = true ]; then
        total_tests=$((total_tests + 1))
        if [ $memory_result -eq 0 ]; then
            print_success "Memory tests: PASSED"
            passed_tests=$((passed_tests + 1))
        else
            print_error "Memory tests: FAILED"
        fi
    fi
    
    if [ "$RUN_STATIC" = true ]; then
        total_tests=$((total_tests + 1))
        if [ $static_result -eq 0 ]; then
            print_success "Static analysis: PASSED"
            passed_tests=$((passed_tests + 1))
        else
            print_warning "Static analysis: ISSUES FOUND"
        fi
    fi
    
    if [ "$RUN_COVERAGE" = true ]; then
        total_tests=$((total_tests + 1))
        if [ $coverage_result -eq 0 ]; then
            print_success "Coverage generation: PASSED"
            passed_tests=$((passed_tests + 1))
        else
            print_error "Coverage generation: FAILED"
        fi
    fi
    
    if [ "$RUN_BENCHMARK" = true ]; then
        total_tests=$((total_tests + 1))
        if [ $benchmark_result -eq 0 ]; then
            print_success "Benchmarks: COMPLETED"
            passed_tests=$((passed_tests + 1))
        else
            print_error "Benchmarks: FAILED"
        fi
    fi
    
    echo ""
    print_status "Test suites completed: $passed_tests/$total_tests"
    print_status "Reports available in: $REPORT_DIR"
    
    # Exit with appropriate code
    if [ $passed_tests -eq $total_tests ]; then
        print_success "All test suites completed successfully!"
        exit 0
    else
        print_error "Some test suites failed or had issues"
        exit 1
    fi
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -c|--clean)
            CLEAN_FIRST=true
            shift
            ;;
        -a|--all)
            RUN_BASIC=true
            RUN_MEMORY=true
            RUN_COVERAGE=true
            RUN_STATIC=true
            shift
            ;;
        -b|--basic)
            RUN_BASIC=true
            shift
            ;;
        -m|--memory)
            RUN_MEMORY=true
            shift
            ;;
        -g|--coverage)
            RUN_COVERAGE=true
            shift
            ;;
        -s|--static)
            RUN_STATIC=true
            shift
            ;;
        -p|--benchmark)
            RUN_BASIC=false
            RUN_BENCHMARK=true
            shift
            ;;
        -r|--report)
            # Report generation is always enabled
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Run main function
main