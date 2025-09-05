# Hardware Build Makefile
# Builds FPGA design using Xilinx Vivado

# Configuration
BOARD ?= zcu102
TOP_MODULE = npu_top
PROJECT_NAME = fpga_npu
VIVADO_VERSION = 2022.1

# Directories
RTL_DIR = rtl
IP_DIR = ip
CONSTRAINTS_DIR = constraints
BUILD_DIR = build
SCRIPTS_DIR = scripts

# Vivado settings
VIVADO = vivado
VIVADO_MODE = batch
VIVADO_OPTS = -mode $(VIVADO_MODE) -nojournal -nolog

# Source files
RTL_SOURCES = $(wildcard $(RTL_DIR)/*.sv $(RTL_DIR)/*.v)
CONSTRAINT_FILES = $(wildcard $(CONSTRAINTS_DIR)/*.xdc)

# Default target
all: bitstream

# Create project and synthesize
project:
	@echo "Creating Vivado project..."
	@mkdir -p $(BUILD_DIR)
	$(VIVADO) $(VIVADO_OPTS) -source $(SCRIPTS_DIR)/create_project.tcl -tclargs $(PROJECT_NAME) $(BUILD_DIR) $(BOARD) $(RTL_SOURCES) $(CONSTRAINT_FILES)

# Synthesis
synth: project
	@echo "Running synthesis..."
	$(VIVADO) $(VIVADO_OPTS) -source $(SCRIPTS_DIR)/synthesize.tcl -tclargs $(BUILD_DIR)/$(PROJECT_NAME)

# Implementation
impl: synth
	@echo "Running implementation..."
	$(VIVADO) $(VIVADO_OPTS) -source $(SCRIPTS_DIR)/implement.tcl -tclargs $(BUILD_DIR)/$(PROJECT_NAME)

# Generate bitstream
bitstream: impl
	@echo "Generating bitstream..."
	$(VIVADO) $(VIVADO_OPTS) -source $(SCRIPTS_DIR)/generate_bitstream.tcl -tclargs $(BUILD_DIR)/$(PROJECT_NAME)

# Program FPGA
program: bitstream
	@echo "Programming FPGA..."
	$(VIVADO) $(VIVADO_OPTS) -source $(SCRIPTS_DIR)/program_fpga.tcl -tclargs $(BUILD_DIR)/$(PROJECT_NAME)

# Reports
reports:
	@echo "Generating reports..."
	$(VIVADO) $(VIVADO_OPTS) -source $(SCRIPTS_DIR)/generate_reports.tcl -tclargs $(BUILD_DIR)/$(PROJECT_NAME)

# Simulation
sim:
	@echo "Running simulation..."
	$(MAKE) -C testbench

# Clean
clean:
	@echo "Cleaning hardware build..."
	rm -rf $(BUILD_DIR)
	rm -f *.log *.jou
	$(MAKE) -C testbench clean

.PHONY: all project synth impl bitstream program reports sim clean