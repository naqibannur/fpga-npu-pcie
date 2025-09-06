# FPGA NPU Simulation Test Makefile
# 
# Comprehensive build system for simulation tests with multiple simulators,
# coverage collection, and automated regression testing

# =============================================================================
# Configuration Variables
# =============================================================================

# Simulator selection (modelsim, questa, vivado, verilator, vcs)
SIM ?= vivado

# Test configuration
ENABLE_COVERAGE ?= 1
ENABLE_ASSERTIONS ?= 1
ENABLE_PERFORMANCE_MONITORING ?= 1
TEST_TIMEOUT ?= 1000000

# Directories
SIM_DIR := .
RTL_DIR := ../../hardware/rtl
TB_DIR := ../../hardware/testbench
WORK_DIR := work
RESULTS_DIR := results
COVERAGE_DIR := coverage
LOGS_DIR := logs

# Test files
TEST_FRAMEWORK := sim_test_framework.sv
TEST_SOURCES := npu_core_tests.sv memory_dma_tests.sv pcie_tests.sv
TEST_MAIN := sim_test_main.sv

# RTL source files
RTL_SOURCES := $(RTL_DIR)/npu_top.sv \
               $(RTL_DIR)/npu_core.sv \
               $(RTL_DIR)/processing_element.sv \
               $(RTL_DIR)/pcie_controller.sv \
               $(RTL_DIR)/async_fifo.sv

# Testbench files
TB_SOURCES := $(TB_DIR)/npu_top_tb.sv \
              $(TB_DIR)/npu_core_tb.sv \
              $(TB_DIR)/pcie_controller_tb.sv \
              $(TB_DIR)/processing_element_tb.sv \
              $(TB_DIR)/async_fifo_tb.sv

# All source files
ALL_SOURCES := $(RTL_SOURCES) $(TB_SOURCES) $(TEST_FRAMEWORK) $(TEST_SOURCES) $(TEST_MAIN)

# =============================================================================
# Simulator-specific Configuration
# =============================================================================

# Vivado Simulator Configuration
ifeq ($(SIM), vivado)
    SIMULATOR := xsim
    COMPILE_CMD := xvlog -sv
    ELABORATE_CMD := xelab
    SIMULATE_CMD := xsim
    COVERAGE_FLAGS := --coverage
    SIM_FLAGS := --testplusarg ENABLE_COVERAGE=$(ENABLE_COVERAGE) \
                 --testplusarg ENABLE_ASSERTIONS=$(ENABLE_ASSERTIONS) \
                 --testplusarg ENABLE_PERFORMANCE_MONITORING=$(ENABLE_PERFORMANCE_MONITORING) \
                 --testplusarg TEST_TIMEOUT=$(TEST_TIMEOUT)
endif

# ModelSim/Questa Configuration
ifeq ($(SIM), modelsim)
    SIMULATOR := vsim
    COMPILE_CMD := vlog -sv
    ELABORATE_CMD := # Not needed for ModelSim
    SIMULATE_CMD := vsim
    COVERAGE_FLAGS := +cover
    SIM_FLAGS := +ENABLE_COVERAGE=$(ENABLE_COVERAGE) \
                 +ENABLE_ASSERTIONS=$(ENABLE_ASSERTIONS) \
                 +ENABLE_PERFORMANCE_MONITORING=$(ENABLE_PERFORMANCE_MONITORING) \
                 +TEST_TIMEOUT=$(TEST_TIMEOUT)
endif

# VCS Configuration
ifeq ($(SIM), vcs)
    SIMULATOR := vcs
    COMPILE_CMD := vcs -sverilog
    ELABORATE_CMD := # Built into compile for VCS
    SIMULATE_CMD := ./simv
    COVERAGE_FLAGS := -cm line+cond+fsm+branch+tgl
    SIM_FLAGS := +ENABLE_COVERAGE=$(ENABLE_COVERAGE) \
                 +ENABLE_ASSERTIONS=$(ENABLE_ASSERTIONS) \
                 +ENABLE_PERFORMANCE_MONITORING=$(ENABLE_PERFORMANCE_MONITORING) \
                 +TEST_TIMEOUT=$(TEST_TIMEOUT)
endif

# Verilator Configuration
ifeq ($(SIM), verilator)
    SIMULATOR := verilator
    COMPILE_CMD := verilator --cc --exe -sv
    ELABORATE_CMD := make -C obj_dir -f V$(basename $(TEST_MAIN)).mk
    SIMULATE_CMD := obj_dir/V$(basename $(TEST_MAIN))
    COVERAGE_FLAGS := --coverage
    SIM_FLAGS := # Verilator uses different format
endif

# =============================================================================
# Build Targets
# =============================================================================

.PHONY: all compile elaborate simulate clean help
.PHONY: test test-quick test-full test-regression
.PHONY: coverage coverage-report coverage-merge
.PHONY: setup check-env

# Default target
all: test

# Setup working directories
setup:
	@echo "Setting up simulation environment..."
	@mkdir -p $(WORK_DIR) $(RESULTS_DIR) $(COVERAGE_DIR) $(LOGS_DIR)
	@echo "Setup complete."

# Check environment
check-env:
	@echo "Checking simulation environment..."
	@echo "Simulator: $(SIM)"
	@echo "Coverage: $(ENABLE_COVERAGE)"
	@echo "Assertions: $(ENABLE_ASSERTIONS)"
	@echo "Performance Monitoring: $(ENABLE_PERFORMANCE_MONITORING)"
	@echo "Test Timeout: $(TEST_TIMEOUT)"
	@which $(SIMULATOR) > /dev/null || (echo "ERROR: $(SIMULATOR) not found in PATH" && exit 1)
	@echo "Environment check passed."

# Compile all sources
compile: setup check-env
	@echo "Compiling sources for $(SIM)..."
	@cd $(WORK_DIR) && \
	$(COMPILE_CMD) $(ALL_SOURCES) $(COVERAGE_FLAGS) 2>&1 | tee ../$(LOGS_DIR)/compile.log
	@echo "Compilation complete."

# Elaborate design (for simulators that require it)
elaborate: compile
ifeq ($(ELABORATE_CMD),)
	@echo "Elaboration not required for $(SIM)"
else
	@echo "Elaborating design for $(SIM)..."
	@cd $(WORK_DIR) && \
	$(ELABORATE_CMD) $(basename $(TEST_MAIN)) $(COVERAGE_FLAGS) 2>&1 | tee ../$(LOGS_DIR)/elaborate.log
	@echo "Elaboration complete."
endif

# Run simulation
simulate: elaborate
	@echo "Running simulation with $(SIM)..."
	@cd $(WORK_DIR) && \
	$(SIMULATE_CMD) $(basename $(TEST_MAIN)) $(SIM_FLAGS) $(COVERAGE_FLAGS) \
	2>&1 | tee ../$(LOGS_DIR)/simulate.log
	@echo "Simulation complete."

# =============================================================================
# Test Execution Targets
# =============================================================================

# Quick test (basic functionality)
test-quick: ENABLE_COVERAGE=0
test-quick: ENABLE_PERFORMANCE_MONITORING=0
test-quick: TEST_TIMEOUT=100000
test-quick: simulate
	@echo "Quick test completed."

# Full test (all features enabled)
test-full: ENABLE_COVERAGE=1
test-full: ENABLE_ASSERTIONS=1
test-full: ENABLE_PERFORMANCE_MONITORING=1
test-full: simulate coverage-report
	@echo "Full test completed."

# Regression test (comprehensive testing)
test-regression:
	@echo "Starting regression test suite..."
	@$(MAKE) test-full SIM=vivado
	@$(MAKE) test-full SIM=modelsim
	@$(MAKE) coverage-merge
	@echo "Regression test suite completed."

# Default test target
test: test-full

# =============================================================================
# Coverage Targets
# =============================================================================

# Generate coverage report
coverage-report:
ifeq ($(ENABLE_COVERAGE), 1)
	@echo "Generating coverage report..."
	@mkdir -p $(COVERAGE_DIR)
ifeq ($(SIM), vivado)
	@cd $(WORK_DIR) && xcrg -report_format html -dir xsim.covdb -report_dir ../$(COVERAGE_DIR)
else ifeq ($(SIM), modelsim)
	@cd $(WORK_DIR) && vcover report -html -htmldir ../$(COVERAGE_DIR) coverage.ucdb
else ifeq ($(SIM), vcs)
	@cd $(WORK_DIR) && urg -dir simv.vdb -report ../$(COVERAGE_DIR)
endif
	@echo "Coverage report generated in $(COVERAGE_DIR)/"
else
	@echo "Coverage collection not enabled."
endif

# Merge coverage from multiple runs
coverage-merge:
	@echo "Merging coverage databases..."
	@mkdir -p $(COVERAGE_DIR)/merged
ifeq ($(SIM), vivado)
	@xcrg -merge -dir $(COVERAGE_DIR)/merged/merged.covdb \
	$(COVERAGE_DIR)/vivado/xsim.covdb $(COVERAGE_DIR)/modelsim/coverage.ucdb
else ifeq ($(SIM), modelsim)
	@vcover merge $(COVERAGE_DIR)/merged/merged.ucdb \
	$(COVERAGE_DIR)/*/coverage.ucdb
else ifeq ($(SIM), vcs)
	@urg -merge -dbname $(COVERAGE_DIR)/merged/merged.vdb \
	$(COVERAGE_DIR)/*/simv.vdb
endif
	@echo "Coverage merge completed."

# =============================================================================
# Analysis and Reporting Targets
# =============================================================================

# Generate comprehensive test report
report:
	@echo "Generating comprehensive test report..."
	@./generate_test_report.py \
	--logs $(LOGS_DIR) \
	--coverage $(COVERAGE_DIR) \
	--results $(RESULTS_DIR) \
	--output $(RESULTS_DIR)/comprehensive_report.html
	@echo "Test report generated: $(RESULTS_DIR)/comprehensive_report.html"

# Performance analysis
performance:
	@echo "Running performance analysis..."
	@./analyze_performance.py \
	--logs $(LOGS_DIR)/simulate.log \
	--output $(RESULTS_DIR)/performance_analysis.html
	@echo "Performance analysis completed."

# Static analysis
lint:
	@echo "Running static analysis..."
	@verilator --lint-only -sv $(RTL_SOURCES) 2>&1 | tee $(LOGS_DIR)/lint.log
	@echo "Static analysis completed."

# =============================================================================
# Continuous Integration Targets
# =============================================================================

# CI target (optimized for automated builds)
ci: setup check-env lint test-full report
	@echo "CI pipeline completed successfully."

# Nightly regression target
nightly: clean test-regression performance report
	@echo "Nightly regression completed."

# =============================================================================
# Utility Targets
# =============================================================================

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(WORK_DIR) $(RESULTS_DIR) $(COVERAGE_DIR) $(LOGS_DIR)
	@rm -rf *.log *.wlf *.vstf transcript
	@rm -rf simv simv.daidir csrc
	@rm -rf obj_dir
	@echo "Clean completed."

# Display help
help:
	@echo "FPGA NPU Simulation Test Makefile"
	@echo "=================================="
	@echo ""
	@echo "Main Targets:"
	@echo "  test          - Run full test suite (default)"
	@echo "  test-quick    - Run quick tests (no coverage/performance)"
	@echo "  test-full     - Run full tests with all features"
	@echo "  test-regression - Run regression across multiple simulators"
	@echo ""
	@echo "Build Targets:"
	@echo "  compile       - Compile all sources"
	@echo "  elaborate     - Elaborate design (if required)"
	@echo "  simulate      - Run simulation"
	@echo ""
	@echo "Coverage Targets:"
	@echo "  coverage-report - Generate coverage report"
	@echo "  coverage-merge  - Merge coverage from multiple runs"
	@echo ""
	@echo "Analysis Targets:"
	@echo "  report        - Generate comprehensive test report"
	@echo "  performance   - Run performance analysis"
	@echo "  lint          - Run static analysis"
	@echo ""
	@echo "CI Targets:"
	@echo "  ci            - Continuous integration pipeline"
	@echo "  nightly       - Nightly regression tests"
	@echo ""
	@echo "Utility Targets:"
	@echo "  setup         - Setup working directories"
	@echo "  clean         - Clean build artifacts"
	@echo "  help          - Display this help"
	@echo ""
	@echo "Configuration Variables:"
	@echo "  SIM=<simulator>                    - Simulator to use (vivado, modelsim, vcs, verilator)"
	@echo "  ENABLE_COVERAGE=<0|1>             - Enable coverage collection"
	@echo "  ENABLE_ASSERTIONS=<0|1>           - Enable assertion checking"
	@echo "  ENABLE_PERFORMANCE_MONITORING=<0|1> - Enable performance monitoring"
	@echo "  TEST_TIMEOUT=<cycles>             - Test timeout in cycles"
	@echo ""
	@echo "Examples:"
	@echo "  make test SIM=vivado              - Run tests with Vivado simulator"
	@echo "  make test-quick ENABLE_COVERAGE=0 - Run quick tests without coverage"
	@echo "  make test-regression              - Run full regression across simulators"

# =============================================================================
# Advanced Targets
# =============================================================================

# Parallel test execution
test-parallel:
	@echo "Running tests in parallel..."
	@$(MAKE) -j4 test-quick SIM=vivado &
	@$(MAKE) -j4 test-quick SIM=modelsim &
	@$(MAKE) -j4 test-quick SIM=vcs &
	@wait
	@echo "Parallel tests completed."

# Memory leak check (with Valgrind if using Verilator)
memcheck:
ifeq ($(SIM), verilator)
	@echo "Running memory leak check..."
	@valgrind --leak-check=full --show-leak-kinds=all \
	$(SIMULATE_CMD) 2>&1 | tee $(LOGS_DIR)/memcheck.log
	@echo "Memory check completed."
else
	@echo "Memory check only available with Verilator."
endif

# Waveform generation
waves: COVERAGE_FLAGS += --debug wave
waves: simulate
	@echo "Waveforms generated."

# Interactive debugging
debug:
	@echo "Starting interactive debug session..."
ifeq ($(SIM), vivado)
	@cd $(WORK_DIR) && xsim $(basename $(TEST_MAIN)) -gui
else ifeq ($(SIM), modelsim)
	@cd $(WORK_DIR) && vsim $(basename $(TEST_MAIN)) -gui
else
	@echo "Interactive debugging not configured for $(SIM)."
endif

# Default error handling
.DEFAULT:
	@echo "Unknown target: $@"
	@echo "Run 'make help' for available targets."