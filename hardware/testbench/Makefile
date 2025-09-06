# Hardware Testbench Makefile
# Provides automated compilation and simulation of SystemVerilog testbenches

# Configuration
SIMULATOR ?= modelsim
SRC_DIR = ../rtl
TB_DIR = .
WORK_DIR = work

# RTL source files (in compilation order)
RTL_SOURCES = \
	$(SRC_DIR)/async_fifo.sv \
	$(SRC_DIR)/processing_element.sv \
	$(SRC_DIR)/pcie_controller.sv \
	$(SRC_DIR)/npu_core.sv \
	$(SRC_DIR)/npu_top.sv

# Testbench files
TB_SOURCES = \
	async_fifo_tb.sv \
	processing_element_tb.sv \
	pcie_controller_tb.sv \
	npu_core_tb.sv \
	npu_top_tb.sv

# Derived testbench names
TESTBENCHES = $(basename $(TB_SOURCES))

# Simulator-specific settings
ifeq ($(SIMULATOR),modelsim)
	VLOG = vlog
	VSIM = vsim
	VLIB = vlib
	COMPILE_OPTS = -sv
	SIM_OPTS = -c -do "run -all; quit"
else ifeq ($(SIMULATOR),questasim)
	VLOG = vlog
	VSIM = vsim
	VLIB = vlib
	COMPILE_OPTS = -sv
	SIM_OPTS = -c -do "run -all; quit"
else ifeq ($(SIMULATOR),vivado)
	VLOG = xvlog
	VSIM = xsim
	VLIB = echo
	COMPILE_OPTS = -sv
	SIM_OPTS = -runall
else
	$(error Unsupported simulator: $(SIMULATOR))
endif

# Phony targets
.PHONY: all clean setup compile test help
.PHONY: $(TESTBENCHES)

# Default target
all: test

# Help target
help:
	@echo "FPGA NPU Hardware Testbench Makefile"
	@echo "===================================="
	@echo ""
	@echo "Targets:"
	@echo "  all                    - Run all testbenches (default)"
	@echo "  setup                  - Setup work directory"
	@echo "  compile                - Compile RTL sources"
	@echo "  test                   - Run all testbenches"
	@echo "  clean                  - Clean work directory"
	@echo ""
	@echo "Individual testbenches:"
	@echo "  async_fifo_tb         - Test async FIFO module"
	@echo "  processing_element_tb - Test processing element"
	@echo "  pcie_controller_tb    - Test PCIe controller"
	@echo "  npu_core_tb          - Test NPU core"
	@echo "  npu_top_tb           - Test complete system"
	@echo ""
	@echo "Variables:"
	@echo "  SIMULATOR=<tool>      - Set simulator (modelsim, questasim, vivado)"
	@echo "  VERBOSE=1             - Enable verbose output"
	@echo ""
	@echo "Examples:"
	@echo "  make                          # Run all tests with default simulator"
	@echo "  make SIMULATOR=vivado         # Use Vivado simulator"
	@echo "  make npu_core_tb             # Run only NPU core test"
	@echo "  make clean test              # Clean and run all tests"

# Setup work directory
setup:
	@echo "Setting up work directory for $(SIMULATOR)..."
	@mkdir -p $(WORK_DIR)
ifeq ($(SIMULATOR),modelsim)
	@cd $(WORK_DIR) && $(VLIB) work
else ifeq ($(SIMULATOR),questasim)
	@cd $(WORK_DIR) && $(VLIB) work
endif
	@echo "Work directory setup complete"

# Compile RTL sources
compile: setup
	@echo "Compiling RTL sources..."
ifeq ($(SIMULATOR),modelsim)
	@for file in $(RTL_SOURCES); do \
		echo "Compiling $$file..."; \
		$(VLOG) $(COMPILE_OPTS) -work $(WORK_DIR)/work $$file || exit 1; \
	done
else ifeq ($(SIMULATOR),questasim)
	@for file in $(RTL_SOURCES); do \
		echo "Compiling $$file..."; \
		$(VLOG) $(COMPILE_OPTS) -work $(WORK_DIR)/work $$file || exit 1; \
	done
else ifeq ($(SIMULATOR),vivado)
	@for file in $(RTL_SOURCES); do \
		echo "Compiling $$file..."; \
		$(VLOG) $(COMPILE_OPTS) -work $(WORK_DIR) $$file || exit 1; \
	done
endif
	@echo "RTL compilation complete"

# Run all testbenches
test: compile $(TESTBENCHES)
	@echo ""
	@echo "========================================="
	@echo "All testbenches completed"
	@echo "========================================="
	@echo "Check individual log files in $(WORK_DIR)/ for details"

# Individual testbench targets
async_fifo_tb: compile
	@echo "Running async_fifo_tb..."
	@$(MAKE) run-testbench TB=async_fifo_tb

processing_element_tb: compile
	@echo "Running processing_element_tb..."
	@$(MAKE) run-testbench TB=processing_element_tb

pcie_controller_tb: compile
	@echo "Running pcie_controller_tb..."
	@$(MAKE) run-testbench TB=pcie_controller_tb

npu_core_tb: compile
	@echo "Running npu_core_tb..."
	@$(MAKE) run-testbench TB=npu_core_tb

npu_top_tb: compile
	@echo "Running npu_top_tb..."
	@$(MAKE) run-testbench TB=npu_top_tb

# Generic testbench runner
run-testbench:
	@echo "Compiling testbench $(TB)..."
ifeq ($(SIMULATOR),modelsim)
	@$(VLOG) $(COMPILE_OPTS) -work $(WORK_DIR)/work $(TB_DIR)/$(TB).sv
	@echo "Running simulation..."
	@cd $(WORK_DIR) && $(VSIM) $(SIM_OPTS) work.$(TB) > $(TB)_output.log 2>&1
	@if grep -q "All tests completed successfully" $(WORK_DIR)/$(TB)_output.log; then \
		echo "✓ $(TB): PASSED"; \
	else \
		echo "✗ $(TB): FAILED"; \
		echo "Check $(WORK_DIR)/$(TB)_output.log for details"; \
	fi
else ifeq ($(SIMULATOR),questasim)
	@$(VLOG) $(COMPILE_OPTS) -work $(WORK_DIR)/work $(TB_DIR)/$(TB).sv
	@echo "Running simulation..."
	@cd $(WORK_DIR) && $(VSIM) $(SIM_OPTS) work.$(TB) > $(TB)_output.log 2>&1
	@if grep -q "All tests completed successfully" $(WORK_DIR)/$(TB)_output.log; then \
		echo "✓ $(TB): PASSED"; \
	else \
		echo "✗ $(TB): FAILED"; \
		echo "Check $(WORK_DIR)/$(TB)_output.log for details"; \
	fi
else ifeq ($(SIMULATOR),vivado)
	@$(VLOG) $(COMPILE_OPTS) -work $(WORK_DIR) $(TB_DIR)/$(TB).sv
	@echo "Running simulation..."
	@cd $(WORK_DIR) && $(VSIM) work.$(TB) $(SIM_OPTS) > $(TB)_output.log 2>&1
	@if grep -q "All tests completed successfully" $(WORK_DIR)/$(TB)_output.log; then \
		echo "✓ $(TB): PASSED"; \
	else \
		echo "✗ $(TB): FAILED"; \
		echo "Check $(WORK_DIR)/$(TB)_output.log for details"; \
	fi
endif

# Clean work directory
clean:
	@echo "Cleaning work directory..."
	@rm -rf $(WORK_DIR)
	@echo "Clean complete"

# Check if simulator is available
check-simulator:
ifeq ($(SIMULATOR),modelsim)
	@which vlog > /dev/null || (echo "Error: ModelSim not found in PATH" && exit 1)
else ifeq ($(SIMULATOR),questasim)
	@which vlog > /dev/null || (echo "Error: QuestaSim not found in PATH" && exit 1)
else ifeq ($(SIMULATOR),vivado)
	@which xvlog > /dev/null || (echo "Error: Vivado not found in PATH" && exit 1)
endif
	@echo "Simulator $(SIMULATOR) found"

# Debug target to show variables
debug:
	@echo "SIMULATOR: $(SIMULATOR)"
	@echo "SRC_DIR: $(SRC_DIR)"
	@echo "TB_DIR: $(TB_DIR)"
	@echo "WORK_DIR: $(WORK_DIR)"
	@echo "RTL_SOURCES: $(RTL_SOURCES)"
	@echo "TB_SOURCES: $(TB_SOURCES)"
	@echo "TESTBENCHES: $(TESTBENCHES)"