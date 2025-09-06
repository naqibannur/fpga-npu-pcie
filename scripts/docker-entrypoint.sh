#!/bin/bash

# Docker entrypoint script for FPGA NPU containers

set -euo pipefail

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }

# Default values
NPU_DEBUG=${NPU_DEBUG:-0}
NPU_LOG_LEVEL=${NPU_LOG_LEVEL:-INFO}
NPU_CONFIG_FILE=${NPU_CONFIG_FILE:-/etc/fpga-npu/npu.conf}

# Check if running in privileged mode
check_privileges() {
    if [[ ! -w /dev ]]; then
        log_warn "Container not running in privileged mode"
        log_warn "Hardware access may be limited"
    fi
}

# Initialize NPU configuration
init_config() {
    log_info "Initializing NPU configuration..."
    
    # Create config directory if it doesn't exist
    mkdir -p "$(dirname "$NPU_CONFIG_FILE")"
    
    # Create default config if it doesn't exist
    if [[ ! -f "$NPU_CONFIG_FILE" ]]; then
        cat > "$NPU_CONFIG_FILE" << EOF
# FPGA NPU Configuration File

[general]
log_level = $NPU_LOG_LEVEL
debug = $NPU_DEBUG
max_devices = 8

[device]
enable_msi = true
dma_coherent = true
power_management = true

[performance]
auto_frequency_scaling = true
thermal_throttling = true
performance_monitoring = true

[security]
allow_all_users = false
require_authentication = false
EOF
        log_info "Created default configuration: $NPU_CONFIG_FILE"
    fi
}

# Load kernel module if available
load_kernel_module() {
    log_info "Checking kernel module..."
    
    if command -v modprobe &> /dev/null; then
        if modprobe fpga_npu 2>/dev/null; then
            log_success "Kernel module loaded successfully"
        else
            log_warn "Failed to load kernel module (this is normal in containers)"
        fi
    else
        log_warn "modprobe not available"
    fi
}

# Check NPU devices
check_devices() {
    log_info "Checking for NPU devices..."
    
    local device_count=0
    
    # Check for PCI devices
    if command -v lspci &> /dev/null; then
        local pci_devices
        pci_devices=$(lspci | grep -i npu | wc -l)
        if [[ $pci_devices -gt 0 ]]; then
            log_success "Found $pci_devices NPU PCI device(s)"
            device_count=$((device_count + pci_devices))
        fi
    fi
    
    # Check for device nodes
    local dev_nodes
    dev_nodes=$(ls /dev/fpga_npu* 2>/dev/null | wc -l)
    if [[ $dev_nodes -gt 0 ]]; then
        log_success "Found $dev_nodes NPU device node(s)"
        device_count=$((device_count + dev_nodes))
    fi
    
    if [[ $device_count -eq 0 ]]; then
        log_warn "No NPU devices found"
        log_warn "Running in simulation mode"
        export NPU_SIMULATION_MODE=1
    else
        log_success "NPU devices available"
    fi
}

# Set up logging
setup_logging() {
    log_info "Setting up logging..."
    
    # Create log directory
    mkdir -p /var/log/fpga-npu
    
    # Set log file permissions
    chmod 755 /var/log/fpga-npu
    
    # Export logging environment variables
    export NPU_LOG_FILE="/var/log/fpga-npu/npu.log"
    export NPU_LOG_LEVEL="$NPU_LOG_LEVEL"
    export NPU_DEBUG="$NPU_DEBUG"
}

# Start NPU daemon
start_daemon() {
    log_info "Starting NPU daemon..."
    
    # Check if daemon binary exists
    if [[ ! -x "/usr/local/bin/npu-daemon" ]]; then
        log_error "NPU daemon binary not found"
        exit 1
    fi
    
    # Start daemon with appropriate options
    local daemon_args=()
    
    if [[ "$NPU_DEBUG" == "1" ]]; then
        daemon_args+=("--debug")
    fi
    
    if [[ -n "${NPU_CONFIG_FILE:-}" ]]; then
        daemon_args+=("--config" "$NPU_CONFIG_FILE")
    fi
    
    exec /usr/local/bin/npu-daemon "${daemon_args[@]}"
}

# Run NPU utility
run_utility() {
    local utility="$1"
    shift
    
    log_info "Running NPU utility: $utility"
    
    local utility_path="/usr/local/bin/npu-$utility"
    if [[ ! -x "$utility_path" ]]; then
        log_error "NPU utility not found: $utility_path"
        exit 1
    fi
    
    exec "$utility_path" "$@"
}

# Run shell with NPU environment
run_shell() {
    log_info "Starting interactive shell with NPU environment..."
    
    # Set up environment
    export PATH="/usr/local/bin:$PATH"
    export LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH:-}"
    export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
    
    # Source profile if available
    [[ -f /etc/profile ]] && source /etc/profile
    [[ -f ~/.bashrc ]] && source ~/.bashrc
    
    exec /bin/bash
}

# Run tests
run_tests() {
    log_info "Running NPU tests..."
    
    if [[ -x "/usr/local/bin/npu-test" ]]; then
        exec /usr/local/bin/npu-test "$@"
    elif [[ -f "/workspace/build/tests/run_tests.py" ]]; then
        cd /workspace
        exec python3 build/tests/run_tests.py "$@"
    else
        log_error "No test runner found"
        exit 1
    fi
}

# Show help
show_help() {
    cat << EOF
FPGA NPU Container Entrypoint

Usage: docker run [docker-options] fpga-npu [command] [args...]

Commands:
    daemon          Start NPU daemon (default)
    shell           Start interactive shell
    test            Run test suite
    info            Show device information
    benchmark       Run benchmarks
    util <name>     Run NPU utility
    help            Show this help

Environment Variables:
    NPU_DEBUG              Enable debug mode (0|1)
    NPU_LOG_LEVEL          Log level (DEBUG|INFO|WARN|ERROR)
    NPU_CONFIG_FILE        Configuration file path
    NPU_SIMULATION_MODE    Force simulation mode (0|1)

Examples:
    # Start daemon
    docker run --privileged fpga-npu

    # Interactive shell
    docker run --privileged -it fpga-npu shell

    # Run tests
    docker run --privileged fpga-npu test

    # Run specific utility
    docker run --privileged fpga-npu util info

EOF
}

# Main function
main() {
    log_info "FPGA NPU Container Starting..."
    
    # Common initialization
    check_privileges
    setup_logging
    init_config
    load_kernel_module
    check_devices
    
    # Parse command
    local command="${1:-daemon}"
    
    case "$command" in
        daemon)
            start_daemon
            ;;
        shell)
            run_shell
            ;;
        test)
            shift
            run_tests "$@"
            ;;
        info|benchmark|status)
            run_utility "$command" "${@:2}"
            ;;
        util)
            if [[ $# -lt 2 ]]; then
                log_error "Usage: util <utility_name> [args...]"
                exit 1
            fi
            run_utility "${@:2}"
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            log_error "Unknown command: $command"
            show_help
            exit 1
            ;;
    esac
}

# Signal handling
trap 'log_info "Received signal, shutting down..."; exit 0' SIGTERM SIGINT

# Run main function
main "$@"