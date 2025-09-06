#!/bin/bash
# 
# Hardware Testbench Runner Script
# Automates compilation and execution of all SystemVerilog testbenches
#

set -e  # Exit on any error

# Configuration
SIMULATOR="modelsim"  # Options: modelsim, vivado, questasim, verilator
WORK_DIR="work"
SRC_DIR="../rtl"
TB_DIR="."
WAVE_FORMAT="vcd"  # Options: vcd, wlf, ghw

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[FAIL]${NC} $1"
}

# Function to check if required tools are available
check_tools() {
    print_info "Checking simulation tools..."
    
    case $SIMULATOR in
        "modelsim"|"questasim")
            if ! command -v vlog &> /dev/null; then
                print_error "ModelSim/QuestaSim not found. Please ensure it's in your PATH."
                exit 1
            fi
            ;;
        "vivado")
            if ! command -v xvlog &> /dev/null; then
                print_error "Vivado simulator not found. Please source Vivado settings."
                exit 1
            fi
            ;;
        "verilator")
            if ! command -v verilator &> /dev/null; then
                print_error "Verilator not found. Please install Verilator."
                exit 1
            fi
            ;;
        *)
            print_error "Unsupported simulator: $SIMULATOR"
            exit 1
            ;;
    esac
    
    print_success "Simulation tools verified"
}

# Function to setup work directory
setup_workspace() {
    print_info "Setting up workspace..."
    
    # Clean and create work directory
    if [ -d "$WORK_DIR" ]; then
        rm -rf "$WORK_DIR"
    fi
    mkdir -p "$WORK_DIR"
    
    # Create work library for ModelSim/QuestaSim
    if [[ "$SIMULATOR" == "modelsim" || "$SIMULATOR" == "questasim" ]]; then
        cd "$WORK_DIR"
        vlib work
        cd ..
    fi
    
    print_success "Workspace setup complete"
}

# Function to compile RTL sources
compile_rtl() {
    print_info "Compiling RTL sources..."
    
    local rtl_files=(
        "async_fifo.sv"
        "processing_element.sv"
        "pcie_controller.sv"
        "npu_core.sv"
        "npu_top.sv"
    )
    
    case $SIMULATOR in
        "modelsim"|"questasim")
            for file in "${rtl_files[@]}"; do
                print_info "Compiling $file..."
                vlog -work "$WORK_DIR/work" "$SRC_DIR/$file"
            done
            ;;
        "vivado")
            for file in "${rtl_files[@]}"; do
                print_info "Compiling $file..."
                xvlog -work "$WORK_DIR" "$SRC_DIR/$file"
            done
            ;;
        "verilator")
            print_warning "Verilator compilation not implemented yet"
            ;;
    esac
    
    print_success "RTL compilation complete"
}

# Function to compile and run a single testbench
run_testbench() {
    local tb_name=$1
    local tb_file="${tb_name}.sv"
    
    print_info "Running testbench: $tb_name"
    
    case $SIMULATOR in
        "modelsim"|"questasim")
            # Compile testbench
            vlog -work "$WORK_DIR/work" "$TB_DIR/$tb_file"
            
            # Run simulation
            cd "$WORK_DIR"
            vsim -c -do "run -all; quit" work.$tb_name > "${tb_name}_output.log" 2>&1
            local exit_code=$?
            cd ..
            
            # Check results
            if [ $exit_code -eq 0 ]; then
                if grep -q "All tests completed successfully" "$WORK_DIR/${tb_name}_output.log"; then
                    print_success "$tb_name: All tests passed"
                    return 0
                else
                    print_error "$tb_name: Some tests failed"
                    return 1
                fi
            else
                print_error "$tb_name: Simulation failed"
                return 1
            fi
            ;;
        "vivado")
            # Compile testbench
            xvlog -work "$WORK_DIR" "$TB_DIR/$tb_file"
            
            # Create simulation script
            cat > "$WORK_DIR/sim_${tb_name}.tcl" << EOF
run_simulation {
    launch_simulation
    run all
    close_sim
}
EOF
            
            # Run simulation
            cd "$WORK_DIR"
            xsim work.$tb_name -tclbatch "sim_${tb_name}.tcl" > "${tb_name}_output.log" 2>&1
            local exit_code=$?
            cd ..
            
            if [ $exit_code -eq 0 ]; then
                print_success "$tb_name: Simulation completed"
                return 0
            else
                print_error "$tb_name: Simulation failed"
                return 1
            fi
            ;;
        "verilator")
            print_warning "$tb_name: Verilator not implemented"
            return 1
            ;;
    esac
}

# Function to generate coverage report
generate_coverage() {
    print_info "Generating coverage report..."
    
    case $SIMULATOR in
        "modelsim"|"questasim")
            if [ -f "$WORK_DIR/coverage.ucdb" ]; then
                vcover report -html "$WORK_DIR/coverage.ucdb"
                print_success "Coverage report generated in htmlcov/"
            else
                print_warning "No coverage data found"
            fi
            ;;
        *)
            print_warning "Coverage reporting not implemented for $SIMULATOR"
            ;;
    esac
}

# Function to display usage
usage() {
    echo "Usage: $0 [OPTIONS] [TESTBENCH]"
    echo ""
    echo "Options:"
    echo "  -s, --simulator TOOL    Set simulator (modelsim, vivado, questasim, verilator)"
    echo "  -c, --clean            Clean workspace before running"
    echo "  -v, --verbose          Enable verbose output"
    echo "  -w, --waves            Enable waveform generation"
    echo "  -h, --help             Show this help message"
    echo ""
    echo "Testbenches:"
    echo "  async_fifo_tb          Test asynchronous FIFO"
    echo "  processing_element_tb  Test processing element"
    echo "  pcie_controller_tb     Test PCIe controller"
    echo "  npu_core_tb           Test NPU core"
    echo "  npu_top_tb            Test complete NPU system"
    echo "  all                   Run all testbenches (default)"
    echo ""
    echo "Examples:"
    echo "  $0                              # Run all tests with ModelSim"
    echo "  $0 -s vivado npu_core_tb       # Run NPU core test with Vivado"
    echo "  $0 -c -w all                    # Clean, run all tests with waves"
}

# Parse command line arguments
CLEAN=false
VERBOSE=false
WAVES=false
TARGET_TB="all"

while [[ $# -gt 0 ]]; do
    case $1 in
        -s|--simulator)
            SIMULATOR="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -w|--waves)
            WAVES=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            TARGET_TB="$1"
            shift
            ;;
    esac
done

# Main execution
main() {
    print_info "FPGA NPU Testbench Runner"
    print_info "========================="
    print_info "Simulator: $SIMULATOR"
    print_info "Target: $TARGET_TB"
    
    # Check tools
    check_tools
    
    # Setup workspace
    if [ "$CLEAN" = true ] || [ ! -d "$WORK_DIR" ]; then
        setup_workspace
    fi
    
    # Compile RTL
    compile_rtl
    
    # List of all testbenches
    local all_testbenches=(
        "async_fifo_tb"
        "processing_element_tb"
        "pcie_controller_tb"
        "npu_core_tb"
        "npu_top_tb"
    )
    
    local failed_tests=()
    local passed_tests=()
    
    # Run specified testbench(es)
    if [ "$TARGET_TB" = "all" ]; then
        print_info "Running all testbenches..."
        for tb in "${all_testbenches[@]}"; do
            if run_testbench "$tb"; then
                passed_tests+=("$tb")
            else
                failed_tests+=("$tb")
            fi
        done
    else
        # Check if specified testbench exists
        if [[ " ${all_testbenches[*]} " =~ " $TARGET_TB " ]]; then
            if run_testbench "$TARGET_TB"; then
                passed_tests+=("$TARGET_TB")
            else
                failed_tests+=("$TARGET_TB")
            fi
        else
            print_error "Unknown testbench: $TARGET_TB"
            exit 1
        fi
    fi
    
    # Generate coverage if enabled
    if [ "$WAVES" = true ]; then
        generate_coverage
    fi
    
    # Summary
    print_info "Test Summary"
    print_info "============"
    print_success "Passed: ${#passed_tests[@]} tests"
    if [ ${#failed_tests[@]} -gt 0 ]; then
        print_error "Failed: ${#failed_tests[@]} tests"
        for test in "${failed_tests[@]}"; do
            print_error "  - $test"
        done
        exit 1
    else
        print_success "All tests passed!"
    fi
}

# Run main function
main "$@"