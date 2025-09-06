#!/bin/bash

# FPGA NPU Health Check and Validation Script
# Comprehensive system validation and performance verification

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Test configuration
TEST_TIMEOUT=300  # 5 minutes per test
PERFORMANCE_THRESHOLD_GOPS=50.0
MEMORY_THRESHOLD_GB=1.0
TEMPERATURE_THRESHOLD_C=80

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Test results tracking
declare -A test_results
declare -A test_times
declare -A performance_metrics
total_tests=0
passed_tests=0
failed_tests=0
skipped_tests=0

# Logging functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_test() { echo -e "${CYAN}[TEST]${NC} $1"; }

# Test framework functions
start_test() {
    local test_name="$1"
    local description="$2"
    
    ((total_tests++))
    log_test "Starting: $test_name - $description"
    test_results["$test_name"]="RUNNING"
    test_times["$test_name"]=$(date +%s)
}

end_test() {
    local test_name="$1"
    local result="$2"
    local message="${3:-}"
    
    local end_time=$(date +%s)
    local duration=$((end_time - test_times["$test_name"]))
    test_times["$test_name"]=$duration
    test_results["$test_name"]="$result"
    
    case "$result" in
        "PASS")
            ((passed_tests++))
            log_success "$test_name PASSED (${duration}s) $message"
            ;;
        "FAIL")
            ((failed_tests++))
            log_error "$test_name FAILED (${duration}s) $message"
            ;;
        "SKIP")
            ((skipped_tests++))
            log_warn "$test_name SKIPPED (${duration}s) $message"
            ;;
    esac
}

# System validation tests
test_system_requirements() {
    start_test "system_requirements" "Validating system requirements"
    
    local errors=0
    
    # Check OS
    if [[ "$(uname)" != "Linux" ]]; then
        log_error "Unsupported OS: $(uname)"
        ((errors++))
    fi
    
    # Check architecture
    if [[ "$(uname -m)" != "x86_64" ]]; then
        log_warn "Unsupported architecture: $(uname -m)"
    fi
    
    # Check kernel version
    local kernel_version
    kernel_version=$(uname -r | cut -d. -f1-2)
    local min_kernel="4.19"
    if [[ "$(printf '%s\n' "$min_kernel" "$kernel_version" | sort -V | head -n1)" != "$min_kernel" ]]; then
        log_error "Kernel version $kernel_version too old (minimum: $min_kernel)"
        ((errors++))
    fi
    
    # Check memory
    local total_memory_gb
    total_memory_gb=$(awk '/MemTotal/ {printf "%.1f", $2/1024/1024}' /proc/meminfo)
    if (( $(echo "$total_memory_gb < $MEMORY_THRESHOLD_GB" | bc -l) )); then
        log_error "Insufficient memory: ${total_memory_gb}GB (minimum: ${MEMORY_THRESHOLD_GB}GB)"
        ((errors++))
    fi
    
    # Check disk space
    local available_space_gb
    available_space_gb=$(df / | awk 'NR==2 {printf "%.1f", $4/1024/1024}')
    if (( $(echo "$available_space_gb < 2.0" | bc -l) )); then
        log_error "Insufficient disk space: ${available_space_gb}GB (minimum: 2.0GB)"
        ((errors++))
    fi
    
    if [[ $errors -eq 0 ]]; then
        end_test "system_requirements" "PASS" "All system requirements met"
    else
        end_test "system_requirements" "FAIL" "$errors requirement(s) not met"
    fi
}

test_kernel_module() {
    start_test "kernel_module" "Testing kernel module functionality"
    
    # Check if module is loaded
    if ! lsmod | grep -q fpga_npu; then
        # Try to load module
        if ! sudo modprobe fpga_npu 2>/dev/null; then
            end_test "kernel_module" "FAIL" "Cannot load kernel module"
            return
        fi
    fi
    
    # Check module information
    if ! modinfo fpga_npu &>/dev/null; then
        end_test "kernel_module" "FAIL" "Module information not available"
        return
    fi
    
    # Check sysfs interface
    if [[ ! -d /sys/module/fpga_npu ]]; then
        end_test "kernel_module" "FAIL" "Sysfs interface not found"
        return
    fi
    
    end_test "kernel_module" "PASS" "Kernel module loaded and accessible"
}

test_device_detection() {
    start_test "device_detection" "Testing NPU device detection"
    
    # Check for PCI devices
    local pci_devices
    pci_devices=$(lspci | grep -i npu | wc -l || true)
    
    # Check for device nodes
    local device_nodes
    device_nodes=$(ls /dev/fpga_npu* 2>/dev/null | wc -l || true)
    
    if [[ $pci_devices -gt 0 ]]; then
        log_info "Found $pci_devices NPU PCI device(s)"
        performance_metrics["detected_devices"]=$pci_devices
        end_test "device_detection" "PASS" "Hardware devices detected"
    elif [[ $device_nodes -gt 0 ]]; then
        log_info "Found $device_nodes NPU device node(s)"
        performance_metrics["detected_devices"]=$device_nodes
        end_test "device_detection" "PASS" "Device nodes available"
    else
        log_warn "No hardware devices found, using simulation mode"
        performance_metrics["detected_devices"]=0
        end_test "device_detection" "SKIP" "No hardware, simulation mode"
    fi
}

test_library_loading() {
    start_test "library_loading" "Testing NPU library loading"
    
    # Check if library exists
    if [[ ! -f "/usr/local/lib/libfpga_npu.so" ]]; then
        end_test "library_loading" "FAIL" "NPU library not found"
        return
    fi
    
    # Check library dependencies
    if ! ldd /usr/local/lib/libfpga_npu.so &>/dev/null; then
        end_test "library_loading" "FAIL" "Library dependency check failed"
        return
    fi
    
    # Test library loading with a simple program
    cat > /tmp/test_lib_load.c << 'EOF'
#include <stdio.h>
#include <dlfcn.h>

int main() {
    void *handle = dlopen("/usr/local/lib/libfpga_npu.so", RTLD_LAZY);
    if (!handle) {
        printf("ERROR: %s\n", dlerror());
        return 1;
    }
    dlclose(handle);
    return 0;
}
EOF
    
    if gcc /tmp/test_lib_load.c -o /tmp/test_lib_load -ldl 2>/dev/null; then
        if /tmp/test_lib_load; then
            end_test "library_loading" "PASS" "Library loads successfully"
        else
            end_test "library_loading" "FAIL" "Library loading test failed"
        fi
    else
        end_test "library_loading" "FAIL" "Cannot compile library test"
    fi
    
    rm -f /tmp/test_lib_load.c /tmp/test_lib_load
}

test_basic_functionality() {
    start_test "basic_functionality" "Testing basic NPU functionality"
    
    # Check if NPU utilities exist
    local utilities=("npu-info" "npu-test")
    for util in "${utilities[@]}"; do
        if [[ ! -x "/usr/local/bin/$util" ]]; then
            end_test "basic_functionality" "FAIL" "Utility $util not found"
            return
        fi
    done
    
    # Test npu-info
    if ! timeout $TEST_TIMEOUT /usr/local/bin/npu-info --version &>/dev/null; then
        end_test "basic_functionality" "FAIL" "npu-info version check failed"
        return
    fi
    
    # Test basic device access
    if ! timeout $TEST_TIMEOUT /usr/local/bin/npu-info --system &>/dev/null; then
        end_test "basic_functionality" "FAIL" "System info check failed"
        return
    fi
    
    end_test "basic_functionality" "PASS" "Basic functionality verified"
}

test_unit_tests() {
    start_test "unit_tests" "Running unit test suite"
    
    local test_binary="/usr/local/bin/npu-test"
    if [[ ! -x "$test_binary" ]]; then
        end_test "unit_tests" "SKIP" "Test binary not found"
        return
    fi
    
    # Run quick unit tests
    local test_output
    if test_output=$(timeout $TEST_TIMEOUT "$test_binary" --quick 2>&1); then
        local test_count
        test_count=$(echo "$test_output" | grep -o "passed: [0-9]*" | awk '{print $2}' || echo "0")
        
        if [[ $test_count -gt 0 ]]; then
            performance_metrics["unit_tests_passed"]=$test_count
            end_test "unit_tests" "PASS" "$test_count tests passed"
        else
            end_test "unit_tests" "FAIL" "No tests passed"
        fi
    else
        end_test "unit_tests" "FAIL" "Unit tests failed or timed out"
    fi
}

test_performance_benchmarks() {
    start_test "performance_benchmarks" "Running performance benchmarks"
    
    local benchmark_binary="/usr/local/bin/npu-benchmark"
    if [[ ! -x "$benchmark_binary" ]]; then
        end_test "performance_benchmarks" "SKIP" "Benchmark binary not found"
        return
    fi
    
    # Run matrix multiplication benchmark
    local benchmark_output
    if benchmark_output=$(timeout $TEST_TIMEOUT "$benchmark_binary" --matrix-multiply --size 256 --iterations 10 2>&1); then
        # Extract performance metrics
        local gflops
        gflops=$(echo "$benchmark_output" | grep -o "[0-9]*\.[0-9]* GFLOPS" | awk '{print $1}' || echo "0")
        
        if [[ -n "$gflops" ]] && (( $(echo "$gflops > 0" | bc -l) )); then
            performance_metrics["matrix_multiply_gflops"]=$gflops
            
            if (( $(echo "$gflops >= $PERFORMANCE_THRESHOLD_GOPS" | bc -l) )); then
                end_test "performance_benchmarks" "PASS" "Performance: ${gflops} GFLOPS"
            else
                end_test "performance_benchmarks" "WARN" "Low performance: ${gflops} GFLOPS (threshold: ${PERFORMANCE_THRESHOLD_GOPS})"
            fi
        else
            end_test "performance_benchmarks" "FAIL" "Could not extract performance metrics"
        fi
    else
        end_test "performance_benchmarks" "FAIL" "Benchmark failed or timed out"
    fi
}

test_memory_functionality() {
    start_test "memory_functionality" "Testing memory management"
    
    # Create simple memory test program
    cat > /tmp/test_memory.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include "fpga_npu_lib.h"

int main() {
    npu_handle_t npu;
    
    if (npu_init(&npu) != NPU_SUCCESS) {
        printf("Failed to initialize NPU\n");
        return 1;
    }
    
    // Test memory allocation
    void *buffer = npu_malloc(npu, 1024 * 1024); // 1MB
    if (!buffer) {
        printf("Memory allocation failed\n");
        npu_cleanup(npu);
        return 1;
    }
    
    // Test memory operations
    npu_result_t result = npu_memory_set(npu, buffer, 0, 1024 * 1024);
    if (result != NPU_SUCCESS) {
        printf("Memory set failed\n");
        npu_free(npu, buffer);
        npu_cleanup(npu);
        return 1;
    }
    
    npu_free(npu, buffer);
    npu_cleanup(npu);
    printf("Memory test passed\n");
    return 0;
}
EOF
    
    if gcc /tmp/test_memory.c -o /tmp/test_memory -lfpga_npu 2>/dev/null; then
        if timeout $TEST_TIMEOUT /tmp/test_memory; then
            end_test "memory_functionality" "PASS" "Memory operations verified"
        else
            end_test "memory_functionality" "FAIL" "Memory test program failed"
        fi
    else
        end_test "memory_functionality" "FAIL" "Cannot compile memory test"
    fi
    
    rm -f /tmp/test_memory.c /tmp/test_memory
}

test_thermal_monitoring() {
    start_test "thermal_monitoring" "Testing thermal monitoring"
    
    # Check if thermal monitoring is available
    local thermal_info
    if thermal_info=$(/usr/local/bin/npu-info --thermal 2>/dev/null || true); then
        if [[ -n "$thermal_info" ]]; then
            # Extract temperature if available
            local temperature
            temperature=$(echo "$thermal_info" | grep -o "[0-9]*°C" | head -1 | grep -o "[0-9]*" || echo "")
            
            if [[ -n "$temperature" ]]; then
                performance_metrics["temperature_c"]=$temperature
                
                if [[ $temperature -gt $TEMPERATURE_THRESHOLD_C ]]; then
                    end_test "thermal_monitoring" "WARN" "High temperature: ${temperature}°C"
                else
                    end_test "thermal_monitoring" "PASS" "Temperature: ${temperature}°C"
                fi
            else
                end_test "thermal_monitoring" "PASS" "Thermal monitoring available"
            fi
        else
            end_test "thermal_monitoring" "SKIP" "No thermal data available"
        fi
    else
        end_test "thermal_monitoring" "SKIP" "Thermal monitoring not supported"
    fi
}

test_power_management() {
    start_test "power_management" "Testing power management"
    
    # Check if power management is available
    local power_info
    if power_info=$(/usr/local/bin/npu-info --power 2>/dev/null || true); then
        if [[ -n "$power_info" ]]; then
            # Extract power consumption if available
            local power_watts
            power_watts=$(echo "$power_info" | grep -o "[0-9]*\.[0-9]*W" | head -1 | grep -o "[0-9]*\.[0-9]*" || echo "")
            
            if [[ -n "$power_watts" ]]; then
                performance_metrics["power_watts"]=$power_watts
                end_test "power_management" "PASS" "Power consumption: ${power_watts}W"
            else
                end_test "power_management" "PASS" "Power management available"
            fi
        else
            end_test "power_management" "SKIP" "No power data available"
        fi
    else
        end_test "power_management" "SKIP" "Power management not supported"
    fi
}

test_error_handling() {
    start_test "error_handling" "Testing error handling"
    
    # Create error handling test program
    cat > /tmp/test_errors.c << 'EOF'
#include <stdio.h>
#include "fpga_npu_lib.h"

int main() {
    npu_handle_t npu;
    
    // Test invalid initialization
    if (npu_init(NULL) == NPU_SUCCESS) {
        printf("ERROR: Invalid init should fail\n");
        return 1;
    }
    
    // Initialize properly
    if (npu_init(&npu) != NPU_SUCCESS) {
        printf("Failed to initialize NPU\n");
        return 1;
    }
    
    // Test invalid operations
    npu_result_t result = npu_tensor_add(npu, NULL, NULL, NULL, 0);
    if (result == NPU_SUCCESS) {
        printf("ERROR: Invalid tensor operation should fail\n");
        npu_cleanup(npu);
        return 1;
    }
    
    // Test error string function
    const char *error_str = npu_get_error_string(result);
    if (!error_str || strlen(error_str) == 0) {
        printf("ERROR: Error string function failed\n");
        npu_cleanup(npu);
        return 1;
    }
    
    npu_cleanup(npu);
    printf("Error handling test passed\n");
    return 0;
}
EOF
    
    if gcc /tmp/test_errors.c -o /tmp/test_errors -lfpga_npu 2>/dev/null; then
        if timeout $TEST_TIMEOUT /tmp/test_errors; then
            end_test "error_handling" "PASS" "Error handling verified"
        else
            end_test "error_handling" "FAIL" "Error handling test failed"
        fi
    else
        end_test "error_handling" "FAIL" "Cannot compile error handling test"
    fi
    
    rm -f /tmp/test_errors.c /tmp/test_errors
}

# Generate comprehensive report
generate_report() {
    local report_file="/tmp/npu_validation_report_$(date +%Y%m%d_%H%M%S).txt"
    
    {
        echo "FPGA NPU Validation Report"
        echo "=========================="
        echo "Generated: $(date)"
        echo "Hostname: $(hostname)"
        echo "Kernel: $(uname -r)"
        echo "Architecture: $(uname -m)"
        echo ""
        
        echo "Test Summary"
        echo "------------"
        echo "Total Tests: $total_tests"
        echo "Passed: $passed_tests"
        echo "Failed: $failed_tests"
        echo "Skipped: $skipped_tests"
        echo "Success Rate: $(( passed_tests * 100 / total_tests ))%"
        echo ""
        
        echo "Test Results"
        echo "------------"
        for test_name in "${!test_results[@]}"; do
            printf "%-25s %-8s (%3ds)\n" "$test_name" "${test_results[$test_name]}" "${test_times[$test_name]}"
        done
        echo ""
        
        echo "Performance Metrics"
        echo "-------------------"
        for metric in "${!performance_metrics[@]}"; do
            printf "%-25s %s\n" "$metric" "${performance_metrics[$metric]}"
        done
        echo ""
        
        echo "System Information"
        echo "------------------"
        echo "CPU Info:"
        grep "model name" /proc/cpuinfo | head -1 | cut -d: -f2 | xargs echo
        echo "Memory: $(grep MemTotal /proc/meminfo | awk '{printf "%.1f GB", $2/1024/1024}')"
        echo "Disk Space: $(df -h / | awk 'NR==2 {print $4}')"
        echo ""
        
        if command -v lspci &>/dev/null; then
            echo "PCI Devices:"
            lspci | grep -i npu || echo "No NPU devices found"
            echo ""
        fi
        
        echo "Kernel Modules:"
        lsmod | grep -E "(fpga|npu)" || echo "No NPU modules loaded"
        echo ""
        
    } > "$report_file"
    
    echo "Validation report saved to: $report_file"
    
    # Also save to project directory if available
    if [[ -d "$PROJECT_ROOT" ]]; then
        cp "$report_file" "$PROJECT_ROOT/validation_report.txt"
        echo "Report also saved to: $PROJECT_ROOT/validation_report.txt"
    fi
}

# Main validation function
main() {
    local start_time
    start_time=$(date +%s)
    
    echo "╔══════════════════════════════════════════════════════════════════╗"
    echo "║                    FPGA NPU System Validation                   ║"
    echo "║                                                                  ║"
    echo "║  This script performs comprehensive validation of the FPGA NPU  ║"
    echo "║  system to ensure proper installation and functionality.        ║"
    echo "╚══════════════════════════════════════════════════════════════════╝"
    echo ""
    
    log_info "Starting FPGA NPU system validation..."
    echo ""
    
    # Run all validation tests
    test_system_requirements
    test_kernel_module
    test_device_detection
    test_library_loading
    test_basic_functionality
    test_unit_tests
    test_performance_benchmarks
    test_memory_functionality
    test_thermal_monitoring
    test_power_management
    test_error_handling
    
    local end_time
    end_time=$(date +%s)
    local total_duration=$((end_time - start_time))
    
    echo ""
    echo "╔══════════════════════════════════════════════════════════════════╗"
    echo "║                        Validation Summary                       ║"
    echo "╚══════════════════════════════════════════════════════════════════╝"
    
    printf "Total Tests:   %3d\n" $total_tests
    printf "Passed:        %3d (%3d%%)\n" $passed_tests $(( passed_tests * 100 / total_tests ))
    printf "Failed:        %3d (%3d%%)\n" $failed_tests $(( failed_tests * 100 / total_tests ))
    printf "Skipped:       %3d (%3d%%)\n" $skipped_tests $(( skipped_tests * 100 / total_tests ))
    printf "Duration:      %3d seconds\n" $total_duration
    
    echo ""
    
    if [[ $failed_tests -eq 0 ]]; then
        log_success "✅ ALL VALIDATION TESTS PASSED!"
        echo ""
        echo "Your FPGA NPU system is properly installed and ready for use."
        echo "You can now start developing applications using the NPU API."
        echo ""
        echo "Next steps:"
        echo "• Try the examples: /usr/local/share/fpga-npu/examples/"
        echo "• Read the documentation: /usr/local/share/doc/fpga-npu/"
        echo "• Run benchmarks: npu-benchmark --all"
        echo ""
    else
        log_error "❌ VALIDATION FAILED!"
        echo ""
        echo "Some tests failed. Please check the results above and:"
        echo "• Review the installation steps"
        echo "• Check system requirements"
        echo "• Consult the troubleshooting guide"
        echo "• Report issues if problems persist"
        echo ""
    fi
    
    # Generate detailed report
    generate_report
    
    # Exit with appropriate code
    exit $failed_tests
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi