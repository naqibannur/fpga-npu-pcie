#!/bin/bash
#
# FPGA Programming Script
# Programs the FPGA with generated bitstream and performs verification
#

set -e

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

# Default values
BOARD="zcu102"
BUILD_TYPE="Release"
BITSTREAM=""
VERIFY=true
RESET_AFTER=false

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

FPGA Programming Script - Program and verify FPGA configuration

Options:
  -b, --board BOARD       Target board (zcu102, vcu118, default: zcu102)
  -t, --build-type TYPE   Build type (Debug, Release, default: Release)
  -f, --bitstream FILE    Specific bitstream file to program
  -n, --no-verify        Skip post-programming verification
  -r, --reset            Reset FPGA after programming
  -h, --help             Show this help message

Examples:
  $0                                    # Program latest ZCU102 Release build
  $0 -b vcu118 -t Debug               # Program VCU118 Debug build
  $0 -f path/to/bitstream.bit          # Program specific bitstream
  $0 -r                                # Program and reset FPGA
EOF
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--board)
            BOARD="$2"
            shift 2
            ;;
        -t|--build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -f|--bitstream)
            BITSTREAM="$2"
            shift 2
            ;;
        -n|--no-verify)
            VERIFY=false
            shift
            ;;
        -r|--reset)
            RESET_AFTER=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Check Vivado installation
check_vivado() {
    if ! command -v vivado &> /dev/null; then
        log_error "Vivado not found in PATH"
        exit 1
    fi
}

# Find bitstream file
find_bitstream() {
    if [ -n "$BITSTREAM" ]; then
        if [ ! -f "$BITSTREAM" ]; then
            log_error "Specified bitstream file not found: $BITSTREAM"
            exit 1
        fi
        return
    fi
    
    # Look for bitstream in build directory
    local build_subdir="$BUILD_DIR/${BOARD}_${BUILD_TYPE}"
    local bit_pattern="$build_subdir/**/*.bit"
    
    BITSTREAM=$(find "$build_subdir" -name "*.bit" -type f | head -1 2>/dev/null || true)
    
    if [ -z "$BITSTREAM" ]; then
        log_error "No bitstream found in build directory: $build_subdir"
        log_error "Please build the project first or specify bitstream with -f"
        exit 1
    fi
    
    log_info "Found bitstream: $BITSTREAM"
}

# Check hardware connection
check_hardware() {
    log_info "Checking hardware connection..."
    
    cat > "/tmp/check_hw.tcl" << EOF
open_hw_manager
connect_hw_server
if {[llength [get_hw_targets]] == 0} {
    puts "ERROR: No hardware targets found"
    exit 1
}
open_hw_target
if {[llength [get_hw_devices]] == 0} {
    puts "ERROR: No hardware devices found"
    exit 1
}
puts "SUCCESS: Hardware connection verified"
foreach device [get_hw_devices] {
    puts "Device: [get_property PART \$device]"
}
close_hw_manager
exit 0
EOF
    
    if ! vivado -mode batch -source "/tmp/check_hw.tcl" > /dev/null 2>&1; then
        log_error "Hardware connection check failed"
        log_error "Please ensure:"
        log_error "  1. FPGA board is powered on"
        log_error "  2. USB/JTAG cable is connected"
        log_error "  3. Board drivers are installed"
        exit 1
    fi
    
    log_success "Hardware connection verified"
    rm -f "/tmp/check_hw.tcl"
}

# Program FPGA
program_fpga() {
    log_info "Programming FPGA with bitstream: $BITSTREAM"
    
    local tcl_file="/tmp/program_fpga.tcl"
    
    cat > "$tcl_file" << EOF
# FPGA Programming Script
open_hw_manager
connect_hw_server

# Open hardware target
open_hw_target [get_hw_targets]

# Get first device (assuming single FPGA)
set device [lindex [get_hw_devices] 0]
current_hw_device \$device

# Set programming properties
set_property PROGRAM.FILE {$BITSTREAM} \$device

# Additional programming options
set_property PROBES.FILE {} \$device
set_property FULL_PROBES.FILE {} \$device

# Program the device
puts "Programming device: [get_property PART \$device]"
puts "Bitstream: $BITSTREAM"

program_hw_devices \$device

# Verify programming
if {[get_property PROGRAM.STATE \$device] == "PROGRAMMED"} {
    puts "SUCCESS: FPGA programming completed successfully"
} else {
    puts "ERROR: FPGA programming failed"
    exit 1
}

close_hw_manager
exit 0
EOF
    
    if vivado -mode batch -source "$tcl_file" -log "/tmp/program.log" -journal "/tmp/program.jou"; then
        log_success "FPGA programming completed successfully"
    else
        log_error "FPGA programming failed"
        log_error "Check log file: /tmp/program.log"
        exit 1
    fi
    
    rm -f "$tcl_file"
}

# Verify programming
verify_programming() {
    if [ "$VERIFY" = false ]; then
        return
    fi
    
    log_info "Verifying FPGA programming..."
    
    cat > "/tmp/verify.tcl" << EOF
open_hw_manager
connect_hw_server
open_hw_target [get_hw_targets]

set device [lindex [get_hw_devices] 0]
current_hw_device \$device

# Check programming state
set state [get_property PROGRAM.STATE \$device]
if {\$state == "PROGRAMMED"} {
    puts "SUCCESS: FPGA is programmed"
    
    # Additional checks
    set part [get_property PART \$device]
    puts "Device part: \$part"
    
    set done_pin [get_property REGISTER.IR.BIT31_DONE \$device]
    if {\$done_pin == 1} {
        puts "SUCCESS: DONE pin is high"
    } else {
        puts "WARNING: DONE pin is not high"
    }
    
} else {
    puts "ERROR: FPGA is not programmed (state: \$state)"
    exit 1
}

close_hw_manager
exit 0
EOF
    
    if vivado -mode batch -source "/tmp/verify.tcl" > /dev/null 2>&1; then
        log_success "FPGA programming verification passed"
    else
        log_warning "FPGA programming verification failed"
    fi
    
    rm -f "/tmp/verify.tcl"
}

# Reset FPGA
reset_fpga() {
    if [ "$RESET_AFTER" = false ]; then
        return
    fi
    
    log_info "Resetting FPGA..."
    
    cat > "/tmp/reset.tcl" << EOF
open_hw_manager
connect_hw_server
open_hw_target [get_hw_targets]

set device [lindex [get_hw_devices] 0]
current_hw_device \$device

# Reset the device
reset_hw_device \$device

puts "SUCCESS: FPGA reset completed"
close_hw_manager
exit 0
EOF
    
    if vivado -mode batch -source "/tmp/reset.tcl" > /dev/null 2>&1; then
        log_success "FPGA reset completed"
    else
        log_warning "FPGA reset failed"
    fi
    
    rm -f "/tmp/reset.tcl"
}

# Check bitstream properties
check_bitstream_properties() {
    local bitstream_size=$(stat -c%s "$BITSTREAM" 2>/dev/null || stat -f%z "$BITSTREAM" 2>/dev/null || echo "unknown")
    local bitstream_date=$(stat -c%y "$BITSTREAM" 2>/dev/null || stat -f%Sm "$BITSTREAM" 2>/dev/null || echo "unknown")
    
    log_info "Bitstream properties:"
    log_info "  File: $(basename "$BITSTREAM")"
    log_info "  Size: $bitstream_size bytes"
    log_info "  Date: $bitstream_date"
}

# Generate programming report
generate_report() {
    local report_file="/tmp/programming_report.txt"
    
    cat > "$report_file" << EOF
FPGA Programming Report
=======================

Date: $(date)
User: $(whoami)
Host: $(hostname)

Configuration:
- Board: $BOARD
- Build Type: $BUILD_TYPE
- Bitstream: $BITSTREAM
- Verify: $VERIFY
- Reset After: $RESET_AFTER

Programming Status: SUCCESS
EOF
    
    log_info "Programming report saved: $report_file"
    
    if [ "$VERIFY" = true ]; then
        cat "$report_file"
    fi
}

# Main execution
main() {
    log_info "FPGA Programming Script"
    log_info "======================="
    log_info "Board: $BOARD"
    log_info "Build Type: $BUILD_TYPE"
    
    # Check prerequisites
    check_vivado
    
    # Find bitstream
    find_bitstream
    check_bitstream_properties
    
    # Hardware checks
    check_hardware
    
    # Program FPGA
    program_fpga
    
    # Post-programming actions
    verify_programming
    reset_fpga
    
    # Generate report
    generate_report
    
    log_success "FPGA programming process completed!"
}

# Execute main function
main "$@"