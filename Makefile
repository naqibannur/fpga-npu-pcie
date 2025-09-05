# FPGA NPU PCIe Project Makefile
# Top-level makefile for building hardware and software components

.PHONY: all clean hardware software driver userspace docs help

# Default target
all: hardware software

# Build all components
hardware:
	@echo "Building hardware design..."
	$(MAKE) -C hardware

software: driver userspace

driver:
	@echo "Building kernel driver..."
	$(MAKE) -C software/driver

userspace:
	@echo "Building user-space library..."
	$(MAKE) -C software/userspace

# Documentation
docs:
	@echo "Building documentation..."
	$(MAKE) -C docs

# Testing
test:
	@echo "Running tests..."
	$(MAKE) -C software/tests

# Installation
install: software
	@echo "Installing driver and library..."
	$(MAKE) -C software/driver install
	$(MAKE) -C software/userspace install

# Clean all components
clean:
	@echo "Cleaning all build artifacts..."
	$(MAKE) -C hardware clean
	$(MAKE) -C software/driver clean
	$(MAKE) -C software/userspace clean
	$(MAKE) -C software/tests clean
	$(MAKE) -C docs clean

# Help
help:
	@echo "FPGA NPU PCIe Project Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build all components (default)"
	@echo "  hardware   - Build FPGA hardware design"
	@echo "  software   - Build all software components"
	@echo "  driver     - Build kernel driver only"
	@echo "  userspace  - Build user-space library only"
	@echo "  docs       - Build documentation"
	@echo "  test       - Run test suite"
	@echo "  install    - Install driver and library"
	@echo "  clean      - Clean all build artifacts"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Configuration:"
	@echo "  BOARD      - Target FPGA board (e.g., zcu102, vcu118)"
	@echo "  CROSS_COMPILE - Cross-compilation prefix"
	@echo "  KERNEL_DIR - Linux kernel source directory"