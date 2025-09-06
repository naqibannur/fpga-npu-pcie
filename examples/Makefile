# FPGA NPU Examples Makefile
# 
# Build system for NPU example applications
# Demonstrates various NPU capabilities and usage patterns

# =============================================================================
# Configuration
# =============================================================================

# Build configuration
BUILD_TYPE ?= release
CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror
LDFLAGS := 
LIBS := -lfpga_npu -lm -lpthread

# Directories
EXAMPLES_DIR := .
NPU_LIB_DIR := ../software/userspace
BUILD_DIR := build
BIN_DIR := bin

# Example directories
MATRIX_MULTIPLY_DIR := matrix_multiply
CNN_INFERENCE_DIR := cnn_inference
NEURAL_NETWORK_DIR := neural_network

# Include paths
CPPFLAGS := -I$(NPU_LIB_DIR) -I.

# Build type specific flags
ifeq ($(BUILD_TYPE), debug)
    CFLAGS += -g -O0 -DDEBUG
else ifeq ($(BUILD_TYPE), release)
    CFLAGS += -O3 -DNDEBUG
else ifeq ($(BUILD_TYPE), profile)
    CFLAGS += -g -O2 -pg
    LDFLAGS += -pg
endif

# Library path
LDFLAGS += -L$(NPU_LIB_DIR)

# =============================================================================
# Targets
# =============================================================================

.PHONY: all clean help
.PHONY: matrix-multiply cnn-inference neural-network
.PHONY: test test-all run-examples
.PHONY: install package

# Default target
all: matrix-multiply cnn-inference neural-network

# Create directories
$(BUILD_DIR) $(BIN_DIR):
	@mkdir -p $@

# =============================================================================
# Individual Example Targets
# =============================================================================

# Matrix Multiplication Example
matrix-multiply: $(BIN_DIR)/matrix_multiply_example

$(BIN_DIR)/matrix_multiply_example: $(MATRIX_MULTIPLY_DIR)/matrix_multiply_example.c | $(BIN_DIR)
	@echo "Building matrix multiplication example..."
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)
	@echo "Matrix multiplication example built successfully."

# CNN Inference Example
cnn-inference: $(BIN_DIR)/cnn_inference_example

$(BIN_DIR)/cnn_inference_example: $(CNN_INFERENCE_DIR)/cnn_inference_example.c | $(BIN_DIR)
	@echo "Building CNN inference example..."
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)
	@echo "CNN inference example built successfully."

# Neural Network Training Example
neural-network: $(BIN_DIR)/neural_network_example

$(BIN_DIR)/neural_network_example: $(NEURAL_NETWORK_DIR)/neural_network_example.c | $(BIN_DIR)
	@echo "Building neural network training example..."
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)
	@echo "Neural network training example built successfully."

# =============================================================================
# Testing Targets
# =============================================================================

# Test individual examples
test-matrix-multiply: matrix-multiply
	@echo "Testing matrix multiplication example..."
	./$(BIN_DIR)/matrix_multiply_example --size 64 --no-verify

test-cnn-inference: cnn-inference
	@echo "Testing CNN inference example..."
	./$(BIN_DIR)/cnn_inference_example

test-neural-network: neural-network
	@echo "Testing neural network training example..."
	./$(BIN_DIR)/neural_network_example

# Test all examples
test-all: test-matrix-multiply test-cnn-inference test-neural-network
	@echo "All example tests completed."

# Quick test (alias)
test: test-all

# =============================================================================
# Demo Targets
# =============================================================================

# Run examples with different configurations
demo-matrix-small:
	@echo "Running small matrix multiplication demo..."
	./$(BIN_DIR)/matrix_multiply_example --size 128 --verbose

demo-matrix-large:
	@echo "Running large matrix multiplication demo..."
	./$(BIN_DIR)/matrix_multiply_example --size 512 --performance

demo-cnn-verbose:
	@echo "Running CNN inference demo with verbose output..."
	./$(BIN_DIR)/cnn_inference_example --verbose --show-input

demo-cnn-benchmark:
	@echo "Running CNN inference benchmark..."
	./$(BIN_DIR)/cnn_inference_example --benchmark

demo-neural-verbose:
	@echo "Running neural network training demo with verbose output..."
	./$(BIN_DIR)/neural_network_example --verbose

demo-neural-benchmark:
	@echo "Running neural network training benchmark..."
	./$(BIN_DIR)/neural_network_example --benchmark

# Run all demos
demo-all: demo-matrix-small demo-cnn-verbose demo-neural-verbose
	@echo "All demos completed."

# =============================================================================
# Performance Testing
# =============================================================================

# Performance benchmarks
perf-matrix:
	@echo "Running matrix multiplication performance test..."
	./$(BIN_DIR)/matrix_multiply_example --size 1024 --performance --no-verify

perf-cnn:
	@echo "Running CNN inference performance test..."
	./$(BIN_DIR)/cnn_inference_example --benchmark

perf-neural:
	@echo "Running neural network training performance test..."
	./$(BIN_DIR)/neural_network_example --benchmark

perf-all: perf-matrix perf-cnn perf-neural
	@echo "All performance tests completed."

# =============================================================================
# Documentation and Help
# =============================================================================

# Generate example documentation
docs:
	@echo "Generating example documentation..."
	@doxygen examples.doxyfile 2>/dev/null || echo "Doxygen not available"

# =============================================================================
# Installation and Packaging
# =============================================================================

# Install examples
install: all
	@echo "Installing examples..."
	@mkdir -p $(DESTDIR)/usr/local/bin/fpga_npu_examples
	@cp $(BIN_DIR)/* $(DESTDIR)/usr/local/bin/fpga_npu_examples/
	@echo "Examples installed to $(DESTDIR)/usr/local/bin/fpga_npu_examples/"

# Create package
package: all
	@echo "Creating examples package..."
	@mkdir -p package/fpga-npu-examples
	@cp -r $(BIN_DIR) package/fpga-npu-examples/
	@cp README.md package/fpga-npu-examples/
	@cp -r $(MATRIX_MULTIPLY_DIR)/README.md package/fpga-npu-examples/matrix_multiply_README.md
	@cp -r $(CNN_INFERENCE_DIR)/README.md package/fpga-npu-examples/cnn_inference_README.md
	@cp -r $(NEURAL_NETWORK_DIR)/README.md package/fpga-npu-examples/neural_network_README.md
	@tar -czf fpga-npu-examples.tar.gz -C package fpga-npu-examples
	@echo "Package created: fpga-npu-examples.tar.gz"

# =============================================================================
# Utility Targets
# =============================================================================

# Clean build artifacts
clean:
	@echo "Cleaning examples..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@rm -rf package fpga-npu-examples.tar.gz
	@echo "Clean completed."

# Check dependencies
check-deps:
	@echo "Checking dependencies..."
	@test -f $(NPU_LIB_DIR)/libfpga_npu.so || test -f $(NPU_LIB_DIR)/libfpga_npu.a || \
		(echo "Error: NPU library not found in $(NPU_LIB_DIR)" && exit 1)
	@$(CC) --version > /dev/null || (echo "Error: C compiler not found" && exit 1)
	@echo "All dependencies found."

# Display build information
info:
	@echo "FPGA NPU Examples Build Information:"
	@echo "  Build type: $(BUILD_TYPE)"
	@echo "  Compiler: $(CC)"
	@echo "  CFLAGS: $(CFLAGS)"
	@echo "  CPPFLAGS: $(CPPFLAGS)"
	@echo "  LDFLAGS: $(LDFLAGS)"
	@echo "  LIBS: $(LIBS)"
	@echo "  NPU library: $(NPU_LIB_DIR)"

# Display help
help:
	@echo "FPGA NPU Examples Makefile"
	@echo "=========================="
	@echo ""
	@echo "Build Targets:"
	@echo "  all                 - Build all examples (default)"
	@echo "  matrix-multiply     - Build matrix multiplication example"
	@echo "  cnn-inference       - Build CNN inference example"
	@echo "  neural-network      - Build neural network training example"
	@echo ""
	@echo "Test Targets:"
	@echo "  test               - Run basic tests for all examples"
	@echo "  test-all           - Run comprehensive tests"
	@echo "  test-matrix-multiply - Test matrix multiplication"
	@echo "  test-cnn-inference - Test CNN inference"
	@echo "  test-neural-network - Test neural network training"
	@echo ""
	@echo "Demo Targets:"
	@echo "  demo-all           - Run all demos"
	@echo "  demo-matrix-small  - Small matrix multiplication demo"
	@echo "  demo-matrix-large  - Large matrix multiplication demo"
	@echo "  demo-cnn-verbose   - CNN inference with verbose output"
	@echo "  demo-cnn-benchmark - CNN inference benchmark"
	@echo "  demo-neural-verbose - Neural network training verbose"
	@echo "  demo-neural-benchmark - Neural network benchmark"
	@echo ""
	@echo "Performance Targets:"
	@echo "  perf-all           - Run all performance tests"
	@echo "  perf-matrix        - Matrix multiplication performance"
	@echo "  perf-cnn           - CNN inference performance"
	@echo "  perf-neural        - Neural network training performance"
	@echo ""
	@echo "Utility Targets:"
	@echo "  clean              - Clean build artifacts"
	@echo "  install            - Install examples"
	@echo "  package            - Create distribution package"
	@echo "  check-deps         - Check build dependencies"
	@echo "  docs               - Generate documentation"
	@echo "  info               - Display build information"
	@echo "  help               - Display this help"
	@echo ""
	@echo "Configuration Variables:"
	@echo "  BUILD_TYPE=<debug|release|profile> - Build configuration"
	@echo ""
	@echo "Examples:"
	@echo "  make                              # Build all examples"
	@echo "  make test                         # Test all examples"
	@echo "  make BUILD_TYPE=debug all         # Debug build"
	@echo "  make demo-all                     # Run all demos"
	@echo "  make perf-all                     # Performance testing"

# =============================================================================
# Advanced Targets
# =============================================================================

# Cross-compilation example (for ARM)
cross-arm:
	$(MAKE) CC=aarch64-linux-gnu-gcc all

# Static analysis
analyze:
	@echo "Running static analysis on examples..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --inconclusive $(MATRIX_MULTIPLY_DIR)/*.c $(CNN_INFERENCE_DIR)/*.c $(NEURAL_NETWORK_DIR)/*.c; \
	else \
		echo "cppcheck not found. Please install cppcheck."; \
	fi

# Memory leak check
memcheck: all
	@echo "Running memory leak check..."
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind --leak-check=full ./$(BIN_DIR)/matrix_multiply_example --size 64; \
	else \
		echo "valgrind not found. Please install valgrind."; \
	fi

# Continuous integration target
ci: check-deps all test analyze
	@echo "CI pipeline completed successfully."

# Default error handling
.DEFAULT:
	@echo "Unknown target: $@"
	@echo "Run 'make help' for available targets."