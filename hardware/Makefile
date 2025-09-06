# Hardware Build Makefile
# FPGA NPU PCIe Interface Design
# Comprehensive build system for multiple boards and configurations

# Default configuration
BOARD ?= zcu102
BUILD_TYPE ?= Release
JOBS ?= 4

# Directories
BUILD_DIR = build
SCRIPTS_DIR = scripts
CONFIGS_DIR = configs
CONSTRAINTS_DIR = constraints
RTL_DIR = rtl

# Include board-specific configuration
include $(CONFIGS_DIR)/$(BOARD)_config.sh

# Build targets
.PHONY: all clean build synthesis implementation bitstream program verify help debug
.PHONY: check-tools setup-env

# Default target
all: bitstream

# Help target
help:
	@echo "FPGA NPU Hardware Build System"
	@echo "=============================="
	@echo ""
	@echo "Targets:"
	@echo "  all                 - Build complete design (synthesis + implementation + bitstream)"
	@echo "  synthesis           - Run synthesis only"
	@echo "  implementation      - Run implementation (requires synthesis)"
	@echo "  bitstream           - Generate bitstream (requires implementation)"
	@echo "  program             - Program FPGA with generated bitstream"
	@echo "  verify              - Verify FPGA programming"
	@echo "  clean               - Clean build artifacts"
	@echo "  check-tools         - Check required tools"
	@echo "  setup-env           - Setup build environment"
	@echo "  debug               - Build with debug configuration"
	@echo ""
	@echo "Variables:"
	@echo "  BOARD=<board>       - Target board (zcu102, vcu118)"
	@echo "  BUILD_TYPE=<type>   - Build type (Debug, Release)"
	@echo "  JOBS=<n>           - Number of parallel jobs"
	@echo ""
	@echo "Examples:"
	@echo "  make                              # Build ZCU102 Release"
	@echo "  make BOARD=vcu118 BUILD_TYPE=Debug  # Build VCU118 Debug"
	@echo "  make synthesis BOARD=zcu102         # Synthesis only"
	@echo "  make program BOARD=vcu118           # Program VCU118"

# Check required tools
check-tools:
	@echo "Checking required tools..."
	@which vivado > /dev/null || (echo "ERROR: Vivado not found" && exit 1)
	@echo "✓ Vivado found: $$(vivado -version | head -1)"
	@echo "✓ All required tools available"

# Setup build environment
setup-env:
	@echo "Setting up build environment..."
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE)
	@echo "✓ Build directories created"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "✓ Build directory cleaned"

# Build complete design
build: check-tools setup-env
	@echo "Building FPGA design for $(BOARD) ($(BUILD_TYPE))..."
	@$(SCRIPTS_DIR)/build_fpga.sh -b $(BOARD) -t $(BUILD_TYPE) -j $(JOBS)

# Synthesis only
synthesis: check-tools setup-env
	@echo "Running synthesis for $(BOARD) ($(BUILD_TYPE))..."
	@$(SCRIPTS_DIR)/build_fpga.sh -b $(BOARD) -t $(BUILD_TYPE) -j $(JOBS) -s

# Implementation (assumes synthesis is done)
implementation: check-tools
	@echo "Running implementation for $(BOARD) ($(BUILD_TYPE))..."
	@if [ ! -d "$(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE)" ]; then \
		echo "ERROR: Synthesis not found. Run 'make synthesis' first."; \
		exit 1; \
	fi
	@$(SCRIPTS_DIR)/build_fpga.sh -b $(BOARD) -t $(BUILD_TYPE) -j $(JOBS)

# Generate bitstream
bitstream: build

# Program FPGA
program: check-tools
	@echo "Programming FPGA for $(BOARD) ($(BUILD_TYPE))..."
	@$(SCRIPTS_DIR)/program_fpga.sh -b $(BOARD) -t $(BUILD_TYPE)

# Verify programming
verify: check-tools
	@echo "Verifying FPGA programming..."
	@$(SCRIPTS_DIR)/program_fpga.sh -b $(BOARD) -t $(BUILD_TYPE) --no-program --verify-only

# Debug build
debug:
	@$(MAKE) BUILD_TYPE=Debug

# Board-specific targets
zcu102:
	@$(MAKE) BOARD=zcu102

vcu118:
	@$(MAKE) BOARD=vcu118

# Quick targets
quick-zcu102:
	@$(MAKE) BOARD=zcu102 BUILD_TYPE=Release synthesis

quick-vcu118:
	@$(MAKE) BOARD=vcu118 BUILD_TYPE=Release synthesis

# Test targets
test-synthesis: synthesis
	@echo "Synthesis test completed for $(BOARD)"

test-build: build
	@echo "Build test completed for $(BOARD)"

# Development targets
dev-clean-build: clean build

dev-quick:
	@$(MAKE) synthesis JOBS=8

# Advanced targets
explore-timing:
	@echo "Running timing exploration..."
	@$(SCRIPTS_DIR)/build_fpga.sh -b $(BOARD) -t $(BUILD_TYPE) -j $(JOBS) --explore-timing

optimize-area:
	@echo "Running area optimization..."
	@$(SCRIPTS_DIR)/build_fpga.sh -b $(BOARD) -t $(BUILD_TYPE) -j $(JOBS) --optimize-area

# Status and information targets
status:
	@echo "Build Status"
	@echo "============"
	@echo "Board: $(BOARD)"
	@echo "Build Type: $(BUILD_TYPE)"
	@echo "Jobs: $(JOBS)"
	@echo "Build Directory: $(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE)"
	@if [ -d "$(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE)" ]; then \
		echo "✓ Build directory exists"; \
		if [ -f "$(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE)"/*.bit ]; then \
			echo "✓ Bitstream available"; \
		else \
			echo "⚠ No bitstream found"; \
		fi \
	else \
		echo "⚠ Build directory not found"; \
	fi

list-builds:
	@echo "Available builds:"
	@if [ -d "$(BUILD_DIR)" ]; then \
		ls -la $(BUILD_DIR)/; \
	else \
		echo "No builds found"; \
	fi

# Report targets
timing-report:
	@if [ -f "$(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE)/timing_summary_impl.rpt" ]; then \
		cat "$(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE)/timing_summary_impl.rpt"; \
	else \
		echo "No timing report found. Run implementation first."; \
	fi

utilization-report:
	@if [ -f "$(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE)/utilization_impl.rpt" ]; then \
		cat "$(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE)/utilization_impl.rpt"; \
	else \
		echo "No utilization report found. Run implementation first."; \
	fi

power-report:
	@if [ -f "$(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE)/power_impl.rpt" ]; then \
		cat "$(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE)/power_impl.rpt"; \
	else \
		echo "No power report found. Run implementation first."; \
	fi

# Maintenance targets
archive:
	@echo "Creating build archive..."
	@tar -czf fpga_npu_$(BOARD)_$(BUILD_TYPE)_$$(date +%Y%m%d_%H%M%S).tar.gz $(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE)
	@echo "✓ Archive created"

backup:
	@echo "Backing up build artifacts..."
	@mkdir -p backup
	@cp -r $(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE) backup/$(BOARD)_$(BUILD_TYPE)_$$(date +%Y%m%d_%H%M%S)
	@echo "✓ Backup created"

# Validation targets
validate-constraints:
	@echo "Validating constraint files..."
	@for file in $(CONSTRAINTS_FILES); do \
		if [ -f "$(CONSTRAINTS_DIR)/$$file" ]; then \
			echo "✓ $$file found"; \
		else \
			echo "✗ $$file missing"; \
		fi \
	done

validate-rtl:
	@echo "Validating RTL files..."
	@for file in $(RTL_DIR)/*.sv; do \
		if [ -f "$$file" ]; then \
			echo "✓ $$(basename $$file) found"; \
		fi \
	done

# Debug and analysis targets
show-config:
	@echo "Current Configuration"
	@echo "===================="
	@echo "Board: $(BOARD_NAME)"
	@echo "Part: $(FPGA_PART)"
	@echo "System Clock: $(SYS_CLK_FREQ) MHz"
	@echo "PCIe Clock: $(PCIE_CLK_FREQ) MHz"
	@echo "NPU Clock: $(NPU_CLK_FREQ) MHz"
	@echo "PCIe: Gen$(PCIE_GEN) x$(PCIE_LANES)"
	@echo "Memory: $(DDR_SIZE) DDR$(DDR_WIDTH)"
	@echo "Target Frequency: $(TARGET_FREQUENCY) MHz"

# Interactive targets (for development)
interactive-build:
	@echo "Starting interactive build session..."
	vivado -mode gui -source $(SCRIPTS_DIR)/interactive_build.tcl

monitor-build:
	@echo "Monitoring build progress..."
	@tail -f $(BUILD_DIR)/$(BOARD)_$(BUILD_TYPE)/vivado_build.log