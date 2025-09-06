# FPGA NPU Developer Guide

This guide provides comprehensive information for developers contributing to the FPGA NPU project, including architecture overview, development setup, coding standards, and contribution guidelines.

## Table of Contents

- [Project Architecture](#project-architecture)
- [Development Environment Setup](#development-environment-setup)
- [Coding Standards](#coding-standards)
- [Development Workflow](#development-workflow)
- [Testing Guidelines](#testing-guidelines)
- [Debugging and Profiling](#debugging-and-profiling)
- [Performance Optimization](#performance-optimization)
- [Contributing](#contributing)

## Project Architecture

### High-Level System Architecture

The FPGA NPU project consists of multiple interconnected components:

```
┌─────────────────────────────────────────────────────────────┐
│                     User Applications                       │
├─────────────────────────────────────────────────────────────┤
│                   NPU User Library                         │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │ Tensor Ops  │ │ Memory Mgmt │ │ Performance Monitor │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    Kernel Driver                           │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │ Device Mgmt │ │ DMA Engine  │ │ Memory Management   │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                  Hardware (FPGA)                          │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │ PCIe Ctrl   │ │ NPU Core    │ │ Processing Elements │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Hardware Architecture

#### NPU Core (`hardware/rtl/npu_core.sv`)
- **Purpose**: Central processing unit coordinating all NPU operations
- **Key Features**:
  - Command queue management
  - Operation scheduling
  - Resource allocation
  - Performance monitoring
- **Interface**: AXI4-Stream for data, AXI4-Lite for control

#### Processing Elements (`hardware/rtl/processing_element.sv`)
- **Purpose**: Parallel computation units for tensor operations
- **Capabilities**:
  - Matrix multiplication
  - Convolution operations
  - Activation functions
  - Element-wise operations
- **Architecture**: SIMD with configurable precision

#### PCIe Controller (`hardware/rtl/pcie_controller.sv`)
- **Purpose**: Host-device communication interface
- **Features**:
  - PCIe Gen3 x8 interface
  - DMA engines for bulk data transfer
  - Memory-mapped I/O for control
  - Interrupt handling

### Software Architecture

#### Kernel Driver (`software/driver/`)
- **Module**: `fpga_npu_driver.c`
- **Responsibilities**:
  - Device enumeration and initialization
  - Memory management (DMA buffers)
  - IOCTL command processing
  - Interrupt handling
  - Power management

#### User Library (`software/userspace/`)
- **Core Library**: `fpga_npu_lib.c`
- **API Layer**: High-level tensor operations
- **Memory Management**: Buffer allocation and caching
- **Performance**: Profiling and monitoring tools

## Development Environment Setup

### Prerequisites

#### Hardware Requirements
- x86_64 development machine
- FPGA development board (Xilinx Zynq UltraScale+ recommended)
- PCIe-capable host system for testing
- Minimum 16GB RAM, 100GB disk space

#### Software Dependencies

**FPGA Development Tools**:
```bash
# Xilinx Vivado (version 2022.1 or later)
# Download from Xilinx website with valid license

# Alternative: Open-source tools
sudo apt-get install yosys nextpnr-xilinx openocd
```

**Software Development Tools**:
```bash
# Essential build tools
sudo apt-get update
sudo apt-get install build-essential cmake git

# Kernel development
sudo apt-get install linux-headers-$(uname -r)
sudo apt-get install dkms

# Documentation tools
sudo apt-get install doxygen sphinx-build

# Testing and analysis tools
sudo apt-get install valgrind cppcheck clang-tidy
```

**Python Environment**:
```bash
# Python 3.8 or later
python3 -m pip install --user \
    numpy matplotlib pytest \
    sphinx sphinx-rtd-theme \
    cocotb pytest-xdist
```

### Environment Configuration

#### Clone and Setup Repository
```bash
# Clone the repository
git clone https://github.com/your-org/fpga_npu.git
cd fpga_npu

# Setup development environment
./scripts/setup_dev_env.sh

# Configure git hooks
cp scripts/git-hooks/* .git/hooks/
chmod +x .git/hooks/*
```

#### Build System Configuration
```bash
# Configure build system
mkdir build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_TESTING=ON \
    -DENABLE_COVERAGE=ON \
    -DENABLE_DOCUMENTATION=ON

# Build the project
make -j$(nproc)
```

#### Environment Variables
```bash
# Add to ~/.bashrc or ~/.zshrc
export FPGA_NPU_ROOT="/path/to/fpga_npu"
export VIVADO_ROOT="/tools/Xilinx/Vivado/2022.1"
export PATH="$VIVADO_ROOT/bin:$PATH"

# Source the environment
source ~/.bashrc
```

## Coding Standards

### C/C++ Coding Standards

#### Naming Conventions
```c
// Functions: snake_case with module prefix
npu_result_t npu_matrix_multiply(npu_handle_t handle, ...);
static int driver_init_device(struct pci_dev *pdev);

// Variables: snake_case
size_t matrix_size;
float *input_buffer;

// Constants: UPPER_CASE with module prefix
#define NPU_MAX_DEVICES 8
#define DRIVER_DEFAULT_TIMEOUT_MS 5000

// Structures: snake_case with _t suffix
typedef struct npu_device {
    struct pci_dev *pdev;
    void __iomem *bar0;
} npu_device_t;

// Enums: UPPER_CASE with module prefix
typedef enum {
    NPU_SUCCESS = 0,
    NPU_ERROR_INVALID_PARAMETER = -1
} npu_result_t;
```

#### Code Formatting
- **Indentation**: 4 spaces (no tabs)
- **Line Length**: Maximum 100 characters
- **Braces**: K&R style for functions, consistent style for control structures

```c
// Function definition style
npu_result_t npu_example_function(npu_handle_t handle,
                                 const float *input,
                                 float *output,
                                 size_t size)
{
    if (!handle || !input || !output) {
        return NPU_ERROR_INVALID_PARAMETER;
    }
    
    for (size_t i = 0; i < size; i++) {
        output[i] = process_element(input[i]);
    }
    
    return NPU_SUCCESS;
}
```

#### Documentation Standards
Use Doxygen-compatible comments for all public APIs:

```c
/**
 * @brief Performs matrix multiplication using NPU acceleration
 * 
 * Computes C = A * B where A is an M×K matrix, B is a K×N matrix,
 * and C is the resulting M×N matrix.
 * 
 * @param handle NPU device handle
 * @param matrix_a Input matrix A (M×K)
 * @param matrix_b Input matrix B (K×N)
 * @param matrix_c Output matrix C (M×N)
 * @param rows_a Number of rows in matrix A (M)
 * @param cols_a Number of columns in A / rows in B (K)
 * @param cols_b Number of columns in matrix B (N)
 * 
 * @return NPU_SUCCESS on success, error code on failure
 * 
 * @note All matrices must be allocated using npu_malloc()
 * @warning Ensure matrix dimensions are compatible
 * 
 * @example
 * @code
 * float *a = npu_malloc(npu, 256 * 256 * sizeof(float));
 * float *b = npu_malloc(npu, 256 * 256 * sizeof(float));
 * float *c = npu_malloc(npu, 256 * 256 * sizeof(float));
 * 
 * npu_result_t result = npu_matrix_multiply(npu, a, b, c, 256, 256, 256);
 * @endcode
 */
npu_result_t npu_matrix_multiply(npu_handle_t handle,
                                const float *matrix_a,
                                const float *matrix_b,
                                float *matrix_c,
                                size_t rows_a,
                                size_t cols_a,
                                size_t cols_b);
```

### SystemVerilog Coding Standards

#### Module Structure
```systemverilog
/**
 * Module: npu_processing_element
 * Description: Parallel processing element for tensor operations
 * Author: [Your Name]
 * Date: [Date]
 */
module npu_processing_element #(
    parameter int DATA_WIDTH = 32,
    parameter int ADDR_WIDTH = 32,
    parameter int PE_ID = 0
) (
    // Clock and reset
    input  logic                    clk,
    input  logic                    rst_n,
    
    // Control interface
    input  logic                    enable,
    input  logic [7:0]             opcode,
    output logic                    ready,
    output logic                    valid,
    
    // Data interfaces
    axi4s_if.slave                 s_axis_data,
    axi4s_if.master                m_axis_result
);

    // Local parameters
    localparam int FIFO_DEPTH = 16;
    localparam int PIPELINE_STAGES = 4;
    
    // Signal declarations
    logic [DATA_WIDTH-1:0] operand_a, operand_b;
    logic [DATA_WIDTH-1:0] result;
    logic operation_valid;
    
    // Implementation
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            ready <= 1'b0;
            valid <= 1'b0;
        end else begin
            // Implementation details
        end
    end

endmodule : npu_processing_element
```

#### Naming Conventions
- **Modules**: snake_case
- **Signals**: snake_case
- **Parameters**: UPPER_CASE
- **Interfaces**: lowercase with _if suffix
- **Clocks**: clk, clk_domain_name
- **Resets**: rst_n (active low), rst (active high)

### Python Coding Standards

Follow PEP 8 with project-specific additions:

```python
"""
FPGA NPU Test Framework
Comprehensive testing utilities for hardware and software validation
"""

import logging
import numpy as np
from typing import List, Dict, Optional, Tuple

class NPUTestCase:
    """Base class for NPU test cases with common utilities."""
    
    def __init__(self, test_name: str, timeout: int = 30):
        """
        Initialize test case.
        
        Args:
            test_name: Descriptive name for the test
            timeout: Test timeout in seconds
        """
        self.test_name = test_name
        self.timeout = timeout
        self.logger = logging.getLogger(f"npu.test.{test_name}")
    
    def setup(self) -> bool:
        """Setup test environment. Override in subclasses."""
        return True
    
    def run(self) -> Tuple[bool, str]:
        """
        Execute the test case.
        
        Returns:
            Tuple of (success, message)
        """
        raise NotImplementedError("Subclasses must implement run()")
    
    def teardown(self) -> None:
        """Cleanup test environment. Override in subclasses."""
        pass
```

## Development Workflow

### Git Workflow

#### Branch Strategy
```bash
# Main branches
main        # Stable releases
develop     # Integration branch
feature/*   # Feature development
hotfix/*    # Critical bug fixes
release/*   # Release preparation
```

#### Feature Development Process
```bash
# Start new feature
git checkout develop
git pull origin develop
git checkout -b feature/your-feature-name

# Make changes with descriptive commits
git add .
git commit -m "feat: add matrix multiplication optimization

- Implement blocked matrix multiplication for better cache usage
- Add performance benchmarks for matrix operations
- Update API documentation with usage examples

Resolves: #123"

# Push feature branch
git push origin feature/your-feature-name

# Create pull request to develop branch
```

#### Commit Message Format
```
type(scope): brief description

Detailed explanation of changes made and why.
Include any breaking changes or migration notes.

Resolves: #issue-number
```

**Types**: feat, fix, docs, style, refactor, test, chore
**Scopes**: hardware, driver, library, tests, docs, build

### Code Review Process

#### Pre-Review Checklist
- [ ] Code follows project coding standards
- [ ] All tests pass locally
- [ ] Documentation updated for API changes
- [ ] Performance impact assessed
- [ ] Security implications considered

#### Review Guidelines
1. **Functionality**: Does the code solve the intended problem?
2. **Design**: Is the solution well-architected?
3. **Performance**: Are there performance implications?
4. **Security**: Are there security vulnerabilities?
5. **Testing**: Is the code adequately tested?
6. **Documentation**: Is the code well-documented?

### Build and Test Integration

#### Local Development Cycle
```bash
# Quick development cycle
make clean && make -j$(nproc)
make test-unit
make test-integration

# Full validation cycle
make clean && make all BUILD_TYPE=Release
make test-all
make check-format
make static-analysis
```

#### Continuous Integration
The CI pipeline automatically runs on:
- Every push to feature branches
- Pull requests to develop/main
- Scheduled nightly builds

```yaml
# CI stages
- Code Quality (format, lint, static analysis)
- Build Matrix (debug/release, multiple environments)
- Unit Tests (software components)
- Integration Tests (end-to-end validation)
- Hardware Simulation (if tools available)
- Performance Benchmarks (on hardware)
```

## Testing Guidelines

### Unit Testing Strategy

#### Test Organization
```
tests/
├── unit/                   # Unit tests
│   ├── test_driver.c      # Kernel driver tests
│   ├── test_library.c     # User library tests
│   └── test_utils.c       # Utility function tests
├── integration/           # Integration tests
│   ├── test_e2e.c        # End-to-end tests
│   └── test_stress.c     # Stress tests
└── benchmarks/           # Performance tests
    ├── benchmark_ops.c    # Operation benchmarks
    └── benchmark_memory.c # Memory benchmarks
```

#### Writing Unit Tests
```c
#include "test_framework.h"
#include "fpga_npu_lib.h"

// Test fixture setup
static npu_handle_t test_npu;

bool setup_test_environment(void) {
    npu_result_t result = npu_init(&test_npu);
    ASSERT_TRUE(result == NPU_SUCCESS);
    return true;
}

// Individual test case
bool test_matrix_multiply_square(void) {
    const size_t size = 64;
    const size_t num_elements = size * size;
    
    // Allocate test matrices
    float *a = npu_malloc(test_npu, num_elements * sizeof(float));
    float *b = npu_malloc(test_npu, num_elements * sizeof(float));
    float *c = npu_malloc(test_npu, num_elements * sizeof(float));
    
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(c);
    
    // Initialize test data
    initialize_identity_matrix(a, size);
    initialize_test_matrix(b, size);
    
    // Perform operation
    npu_result_t result = npu_matrix_multiply(test_npu, a, b, c, size, size, size);
    ASSERT_EQ(NPU_SUCCESS, result);
    
    // Verify results
    bool matrices_equal = compare_matrices(b, c, size, size, 1e-6f);
    ASSERT_TRUE(matrices_equal);
    
    // Cleanup
    npu_free(test_npu, a);
    npu_free(test_npu, b);
    npu_free(test_npu, c);
    
    return true;
}

// Test registration
REGISTER_TEST("Matrix Multiply Square", test_matrix_multiply_square);
```

### Hardware Testing

#### SystemVerilog Testbenches
```systemverilog
/**
 * Testbench for NPU Core Module
 */
module npu_core_tb;
    
    // Test parameters
    localparam int CLK_PERIOD = 10; // 100MHz
    localparam int RESET_CYCLES = 10;
    
    // DUT signals
    logic clk, rst_n;
    axi4s_if #(.DATA_WIDTH(512)) axis_data();
    axi4lite_if #(.ADDR_WIDTH(32), .DATA_WIDTH(32)) axil_ctrl();
    
    // DUT instantiation
    npu_core #(
        .NUM_PROCESSING_ELEMENTS(16),
        .MEMORY_SIZE(1024*1024)
    ) dut (
        .clk(clk),
        .rst_n(rst_n),
        .s_axis_data(axis_data.slave),
        .s_axil_ctrl(axil_ctrl.slave)
    );
    
    // Clock generation
    initial begin
        clk = 0;
        forever #(CLK_PERIOD/2) clk = ~clk;
    end
    
    // Test sequence
    initial begin
        // Initialize
        rst_n = 0;
        axis_data.tvalid = 0;
        axil_ctrl.awvalid = 0;
        axil_ctrl.wvalid = 0;
        axil_ctrl.arvalid = 0;
        
        // Reset sequence
        repeat(RESET_CYCLES) @(posedge clk);
        rst_n = 1;
        
        // Test cases
        test_basic_operation();
        test_matrix_multiply();
        test_error_conditions();
        
        $display("All tests passed!");
        $finish;
    end
    
    // Test tasks
    task test_basic_operation();
        $display("Testing basic operation...");
        
        // Configure NPU
        write_register(32'h0000, 32'h0000_0001); // Enable
        write_register(32'h0004, 32'h0000_0010); // Command
        
        // Send data
        send_data_packet(512'h123456789ABCDEF);
        
        // Wait for completion
        wait_for_completion();
        
        // Verify results
        check_status_register();
    endtask
    
endmodule
```

### Integration Testing

#### End-to-End Test Framework
```c
// Integration test example
bool test_complete_workflow(void) {
    npu_handle_t npu;
    
    // Initialize system
    ASSERT_EQ(NPU_SUCCESS, npu_init(&npu));
    
    // Test data preparation
    const size_t matrix_size = 256;
    float *input_a = create_test_matrix(matrix_size, MATRIX_TYPE_RANDOM);
    float *input_b = create_test_matrix(matrix_size, MATRIX_TYPE_RANDOM);
    float *npu_result = allocate_matrix(matrix_size);
    float *cpu_result = allocate_matrix(matrix_size);
    
    // NPU computation
    auto start_npu = get_time();
    ASSERT_EQ(NPU_SUCCESS, npu_matrix_multiply(npu, input_a, input_b, npu_result, 
                                              matrix_size, matrix_size, matrix_size));
    auto end_npu = get_time();
    
    // CPU reference computation
    auto start_cpu = get_time();
    cpu_matrix_multiply(input_a, input_b, cpu_result, matrix_size);
    auto end_cpu = get_time();
    
    // Verify results match
    bool results_match = compare_matrices(npu_result, cpu_result, matrix_size, 
                                         matrix_size, 1e-5f);
    ASSERT_TRUE(results_match);
    
    // Performance validation
    double npu_time = time_diff_ms(start_npu, end_npu);
    double cpu_time = time_diff_ms(start_cpu, end_cpu);
    double speedup = cpu_time / npu_time;
    
    printf("NPU time: %.2f ms, CPU time: %.2f ms, Speedup: %.2fx\n", 
           npu_time, cpu_time, speedup);
    
    // Cleanup
    free(input_a);
    free(input_b);
    free(npu_result);
    free(cpu_result);
    npu_cleanup(npu);
    
    return true;
}
```

## Debugging and Profiling

### Software Debugging

#### GDB Integration
```bash
# Compile with debug symbols
make BUILD_TYPE=Debug

# Debug user applications
gdb ./examples/matrix_multiply/matrix_multiply_example
(gdb) set args --size 1024 --benchmark
(gdb) break npu_matrix_multiply
(gdb) run

# Debug kernel module
sudo gdb /proc/kcore
(gdb) add-symbol-file software/driver/fpga_npu_driver.ko <load_address>
```

#### Valgrind Memory Analysis
```bash
# Memory leak detection
valgrind --tool=memcheck --leak-check=full \
    ./examples/matrix_multiply/matrix_multiply_example

# Performance profiling
valgrind --tool=callgrind \
    ./examples/cnn_inference/cnn_inference_example
```

#### AddressSanitizer
```bash
# Compile with AddressSanitizer
make BUILD_TYPE=Debug ENABLE_ASAN=ON

# Run with memory error detection
export ASAN_OPTIONS="detect_leaks=1:abort_on_error=1"
./test_application
```

### Hardware Debugging

#### Vivado Integration
```tcl
# Connect to hardware
connect_hw_server
open_hw_target

# Program FPGA
set_property PROGRAM.FILE {fpga_npu.bit} [get_hw_devices]
program_hw_devices

# Add debug cores
create_debug_core u_debug_core ila
set_property port_width 512 [get_debug_ports u_debug_core/probe0]
connect_debug_port u_debug_core/probe0 [get_nets data_path/*]
```

#### Logic Analyzer Integration
```systemverilog
// ILA debug core instantiation
ila_0 debug_ila (
    .clk(clk),
    .probe0(processing_element_data),
    .probe1(command_queue_status),
    .probe2(memory_controller_signals),
    .probe3(performance_counters)
);
```

### Performance Profiling

#### NPU Performance Counters
```c
// Enable performance monitoring
npu_reset_performance_counters(npu);

// Execute operations
npu_matrix_multiply(npu, a, b, c, size, size, size);

// Read performance data
npu_performance_counters_t counters;
npu_get_performance_counters(npu, &counters);

printf("Operations: %lu, Cycles: %lu, Utilization: %.2f%%\n",
       counters.total_operations, counters.total_cycles, 
       counters.utilization_percent);
```

#### System-Level Profiling
```bash
# Monitor system resources
htop
iotop
nvidia-smi # If applicable

# Profile application performance
perf record -g ./your_application
perf report

# Monitor PCIe traffic
lspci -vvv | grep -A 20 "Neural"
cat /proc/interrupts | grep npu
```

## Performance Optimization

### Algorithm Optimization

#### Matrix Multiplication Optimization
```c
// Blocked matrix multiplication for cache efficiency
void optimized_matrix_multiply(const float *a, const float *b, float *c,
                              size_t n, size_t block_size) {
    for (size_t i = 0; i < n; i += block_size) {
        for (size_t j = 0; j < n; j += block_size) {
            for (size_t k = 0; k < n; k += block_size) {
                // Process block
                size_t max_i = min(i + block_size, n);
                size_t max_j = min(j + block_size, n);
                size_t max_k = min(k + block_size, n);
                
                multiply_block(a, b, c, i, j, k, max_i, max_j, max_k, n);
            }
        }
    }
}
```

#### Memory Access Optimization
```c
// Optimize memory layout for NPU
typedef struct {
    float *data;
    size_t rows, cols;
    size_t row_stride;  // For padding alignment
} aligned_matrix_t;

aligned_matrix_t* create_aligned_matrix(size_t rows, size_t cols) {
    aligned_matrix_t *matrix = malloc(sizeof(aligned_matrix_t));
    
    // Align to cache line boundaries
    size_t alignment = 64; // Cache line size
    size_t aligned_cols = (cols + alignment/sizeof(float) - 1) & 
                         ~(alignment/sizeof(float) - 1);
    
    matrix->data = aligned_alloc(alignment, rows * aligned_cols * sizeof(float));
    matrix->rows = rows;
    matrix->cols = cols;
    matrix->row_stride = aligned_cols;
    
    return matrix;
}
```

### Hardware Optimization

#### Pipeline Optimization
```systemverilog
// Deep pipeline for high throughput
module optimized_processing_element #(
    parameter int PIPELINE_STAGES = 8
) (
    input  logic clk,
    input  logic rst_n,
    input  logic [31:0] operand_a,
    input  logic [31:0] operand_b,
    output logic [31:0] result,
    output logic valid
);

    // Pipeline registers
    logic [31:0] pipeline_a [PIPELINE_STAGES];
    logic [31:0] pipeline_b [PIPELINE_STAGES];
    logic [31:0] pipeline_result [PIPELINE_STAGES];
    logic [PIPELINE_STAGES-1:0] pipeline_valid;
    
    // Pipeline implementation
    always_ff @(posedge clk) begin
        if (!rst_n) begin
            pipeline_valid <= '0;
        end else begin
            // Shift pipeline
            pipeline_a[0] <= operand_a;
            pipeline_b[0] <= operand_b;
            pipeline_valid <= {pipeline_valid[PIPELINE_STAGES-2:0], 1'b1};
            
            for (int i = 1; i < PIPELINE_STAGES; i++) begin
                pipeline_a[i] <= pipeline_a[i-1];
                pipeline_b[i] <= pipeline_b[i-1];
            end
            
            // Computation stages
            pipeline_result[0] <= pipeline_a[0] * pipeline_b[0]; // Stage 0: Multiply
            pipeline_result[1] <= pipeline_result[0];             // Stage 1: Buffer
            // Additional stages for complex operations
        end
    end
    
    assign result = pipeline_result[PIPELINE_STAGES-1];
    assign valid = pipeline_valid[PIPELINE_STAGES-1];

endmodule
```

### Memory Hierarchy Optimization

#### Cache-Friendly Data Structures
```c
// Structure of arrays for better vectorization
typedef struct {
    float *values;      // All values contiguous
    size_t *indices;    // All indices contiguous
    size_t capacity;
    size_t size;
} soa_sparse_matrix_t;

// Array of structures (less cache-friendly)
typedef struct {
    float value;
    size_t index;
} aos_element_t;

typedef struct {
    aos_element_t *elements;
    size_t capacity;
    size_t size;
} aos_sparse_matrix_t;
```

## Contributing

### Contribution Process

1. **Issue Discussion**: Discuss significant changes in GitHub issues
2. **Fork and Branch**: Create feature branch from develop
3. **Implementation**: Follow coding standards and write tests
4. **Documentation**: Update relevant documentation
5. **Testing**: Ensure all tests pass
6. **Pull Request**: Submit PR with clear description
7. **Code Review**: Address feedback from maintainers
8. **Merge**: Maintainer merges after approval

### Contribution Guidelines

#### What to Contribute
- **Bug Fixes**: Fix identified issues
- **Performance Improvements**: Optimize algorithms or hardware
- **New Features**: Add new NPU operations or capabilities
- **Documentation**: Improve guides, examples, or API docs
- **Tests**: Add test coverage or improve test framework

#### Code Quality Requirements
- [ ] All tests pass (unit, integration, hardware simulation)
- [ ] Code coverage maintained or improved
- [ ] No static analysis warnings
- [ ] Documentation updated for public APIs
- [ ] Performance impact assessed
- [ ] Security implications reviewed

### Recognition

Contributors are recognized in:
- `CONTRIBUTORS.md` file
- Release notes for significant contributions
- Annual contributor acknowledgments

## Support and Community

### Getting Help
- **Documentation**: Check API reference and user guide
- **GitHub Issues**: Report bugs or request features  
- **Discussions**: General questions and community support
- **Email**: Maintainer contact for security issues

### Communication Channels
- **GitHub**: Primary development communication
- **Slack/Discord**: Real-time community chat (if available)
- **Mailing List**: Announcements and major updates

This developer guide provides the foundation for contributing to the FPGA NPU project. For specific implementation details, refer to the API reference and examine existing code examples.