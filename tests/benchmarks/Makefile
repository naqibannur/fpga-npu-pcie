# FPGA NPU Performance Benchmark Suite Makefile
# 
# Comprehensive build system for NPU performance benchmarking
# with support for various build configurations and test modes

# =============================================================================
# Configuration Variables
# =============================================================================

# Build configuration
BUILD_TYPE ?= release
ENABLE_POWER_MONITORING ?= 1
ENABLE_THERMAL_MONITORING ?= 1
ENABLE_PROFILING ?= 0

# Directories
BENCHMARK_DIR := .
FRAMEWORK_DIR := .
SRC_DIR := .
BUILD_DIR := build
BIN_DIR := bin
RESULTS_DIR := results
NPU_LIB_DIR := ../../software/userspace
DRIVER_DIR := ../../software/driver

# Compiler and tools
CC := gcc
CXX := g++
AR := ar
INSTALL := install

# Source files
FRAMEWORK_SOURCES := benchmark_framework.c
BENCHMARK_SOURCES := throughput_benchmarks.c \
                    latency_benchmarks.c \
                    scalability_benchmarks.c \
                    power_efficiency_benchmarks.c
MAIN_SOURCE := benchmark_main.c

# All source files
ALL_SOURCES := $(FRAMEWORK_SOURCES) $(BENCHMARK_SOURCES) $(MAIN_SOURCE)

# Object files
FRAMEWORK_OBJECTS := $(FRAMEWORK_SOURCES:%.c=$(BUILD_DIR)/%.o)
BENCHMARK_OBJECTS := $(BENCHMARK_SOURCES:%.c=$(BUILD_DIR)/%.o)
MAIN_OBJECT := $(BUILD_DIR)/benchmark_main.o

# =============================================================================
# Compiler Flags
# =============================================================================

# Base compiler flags
CFLAGS := -std=c11 -Wall -Wextra -Werror
CPPFLAGS := -I$(FRAMEWORK_DIR) -I$(NPU_LIB_DIR) -I$(DRIVER_DIR)
LDFLAGS := -L$(NPU_LIB_DIR)
LIBS := -lfpga_npu -lm -lpthread -lrt

# Build type specific flags
ifeq ($(BUILD_TYPE), debug)
    CFLAGS += -g -O0 -DDEBUG
    CPPFLAGS += -DBENCHMARK_DEBUG
else ifeq ($(BUILD_TYPE), release)
    CFLAGS += -O3 -DNDEBUG
    CPPFLAGS += -DBENCHMARK_RELEASE
else ifeq ($(BUILD_TYPE), profile)
    CFLAGS += -g -O2 -pg
    LDFLAGS += -pg
    CPPFLAGS += -DBENCHMARK_PROFILE
endif

# Feature flags
ifeq ($(ENABLE_POWER_MONITORING), 1)
    CPPFLAGS += -DENABLE_POWER_MONITORING
endif

ifeq ($(ENABLE_THERMAL_MONITORING), 1)
    CPPFLAGS += -DENABLE_THERMAL_MONITORING
endif

ifeq ($(ENABLE_PROFILING), 1)
    CPPFLAGS += -DENABLE_PROFILING
    LIBS += -lprofiler
endif

# Platform-specific flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Linux)
    CPPFLAGS += -D_GNU_SOURCE
    LIBS += -ldl
endif

# =============================================================================
# Build Targets
# =============================================================================

.PHONY: all clean install test help
.PHONY: benchmark-suite framework benchmarks
.PHONY: debug release profile
.PHONY: run-quick run-full run-throughput run-latency run-scalability run-power

# Default target
all: benchmark-suite

# Main benchmark suite
benchmark-suite: $(BIN_DIR)/npu_benchmark

$(BIN_DIR)/npu_benchmark: $(FRAMEWORK_OBJECTS) $(BENCHMARK_OBJECTS) $(MAIN_OBJECT) | $(BIN_DIR)
	@echo "Linking benchmark suite..."
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Benchmark suite built successfully."

# Framework library
framework: $(BUILD_DIR)/libbenchmark_framework.a

$(BUILD_DIR)/libbenchmark_framework.a: $(FRAMEWORK_OBJECTS)
	@echo "Creating framework library..."
	$(AR) rcs $@ $^

# Individual benchmark modules
benchmarks: $(BENCHMARK_OBJECTS)

# =============================================================================
# Object File Rules
# =============================================================================

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# =============================================================================
# Directory Creation
# =============================================================================

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(RESULTS_DIR):
	@mkdir -p $(RESULTS_DIR)

# =============================================================================
# Build Type Targets
# =============================================================================

debug:
	$(MAKE) BUILD_TYPE=debug all

release:
	$(MAKE) BUILD_TYPE=release all

profile:
	$(MAKE) BUILD_TYPE=profile all

# =============================================================================
# Test Execution Targets
# =============================================================================

# Quick test run (basic functionality)
run-quick: benchmark-suite | $(RESULTS_DIR)
	@echo "Running quick benchmark suite..."
	./$(BIN_DIR)/npu_benchmark --size small --iterations 10 --warmup 2 \
		--output $(RESULTS_DIR) --csv

# Full test run (comprehensive)
run-full: benchmark-suite | $(RESULTS_DIR)
	@echo "Running full benchmark suite..."
	./$(BIN_DIR)/npu_benchmark --all --size medium --output $(RESULTS_DIR) \
		--csv --json --verbose

# Throughput benchmarks
run-throughput: benchmark-suite | $(RESULTS_DIR)
	@echo "Running throughput benchmarks..."
	./$(BIN_DIR)/npu_benchmark --throughput --size large --output $(RESULTS_DIR) \
		--csv --verbose

# Latency benchmarks
run-latency: benchmark-suite | $(RESULTS_DIR)
	@echo "Running latency benchmarks..."
	./$(BIN_DIR)/npu_benchmark --latency --size small --iterations 1000 \
		--output $(RESULTS_DIR) --csv --verbose

# Scalability benchmarks
run-scalability: benchmark-suite | $(RESULTS_DIR)
	@echo "Running scalability benchmarks..."
	./$(BIN_DIR)/npu_benchmark --scalability --size medium --threads 8 \
		--output $(RESULTS_DIR) --csv --verbose

# Power efficiency benchmarks
run-power: benchmark-suite | $(RESULTS_DIR)
	@echo "Running power efficiency benchmarks..."
	./$(BIN_DIR)/npu_benchmark --power --size medium --enable-power \
		--enable-thermal --output $(RESULTS_DIR) --csv --verbose

# =============================================================================
# Specific Benchmark Targets
# =============================================================================

run-matmul:
	./$(BIN_DIR)/npu_benchmark --benchmark matmul_throughput --size large \
		--iterations 100 --output $(RESULTS_DIR)

run-conv2d:
	./$(BIN_DIR)/npu_benchmark --benchmark conv2d_throughput --size medium \
		--iterations 50 --output $(RESULTS_DIR)

run-memory:
	./$(BIN_DIR)/npu_benchmark --benchmark memory_bandwidth --size xlarge \
		--iterations 20 --output $(RESULTS_DIR)

run-thermal:
	./$(BIN_DIR)/npu_benchmark --benchmark thermal_behavior --enable-power \
		--enable-thermal --output $(RESULTS_DIR)

# =============================================================================
# Analysis and Reporting Targets
# =============================================================================

# Generate performance report
report: | $(RESULTS_DIR)
	@echo "Generating performance analysis report..."
	@if [ -f "analyze_results.py" ]; then \
		python3 analyze_results.py --input $(RESULTS_DIR) --output $(RESULTS_DIR)/analysis.html; \
	else \
		echo "Analysis script not found. Please run benchmarks first."; \
	fi

# Compare results between runs
compare:
	@echo "Comparing benchmark results..."
	@if [ -f "compare_results.py" ]; then \
		python3 compare_results.py --dir $(RESULTS_DIR); \
	else \
		echo "Comparison script not found."; \
	fi

# =============================================================================
# Installation Targets
# =============================================================================

# Install benchmark suite
install: benchmark-suite
	@echo "Installing benchmark suite..."
	$(INSTALL) -d $(DESTDIR)/usr/local/bin
	$(INSTALL) -m 755 $(BIN_DIR)/npu_benchmark $(DESTDIR)/usr/local/bin/
	@echo "Installation complete."

# Uninstall
uninstall:
	@echo "Uninstalling benchmark suite..."
	rm -f $(DESTDIR)/usr/local/bin/npu_benchmark
	@echo "Uninstallation complete."

# =============================================================================
# Development Targets
# =============================================================================

# Format source code
format:
	@echo "Formatting source code..."
	@if command -v clang-format >/dev/null 2>&1; then \
		clang-format -i $(ALL_SOURCES) *.h; \
		echo "Code formatting complete."; \
	else \
		echo "clang-format not found. Please install clang-format."; \
	fi

# Static analysis
analyze:
	@echo "Running static analysis..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --inconclusive --std=c11 $(ALL_SOURCES); \
	else \
		echo "cppcheck not found. Please install cppcheck."; \
	fi

# Memory leak check
memcheck: debug
	@echo "Running memory leak check..."
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
		./$(BIN_DIR)/npu_benchmark --benchmark single_op_latency --size small --iterations 10; \
	else \
		echo "valgrind not found. Please install valgrind."; \
	fi

# =============================================================================
# Continuous Integration Targets
# =============================================================================

# CI pipeline
ci: clean all test analyze
	@echo "CI pipeline completed successfully."

# Automated testing
test: benchmark-suite
	@echo "Running automated tests..."
	./$(BIN_DIR)/npu_benchmark --benchmark single_op_latency --size small --iterations 5
	./$(BIN_DIR)/npu_benchmark --benchmark matmul_throughput --size small --iterations 5
	@echo "Basic functionality tests passed."

# Performance regression check
regression: benchmark-suite | $(RESULTS_DIR)
	@echo "Running performance regression check..."
	./$(BIN_DIR)/npu_benchmark --throughput --size medium --iterations 20 \
		--output $(RESULTS_DIR)/regression
	@if [ -f "check_regression.py" ]; then \
		python3 check_regression.py --current $(RESULTS_DIR)/regression --baseline baseline_results; \
	else \
		echo "Regression check script not found."; \
	fi

# =============================================================================
# Utility Targets
# =============================================================================

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean completed."

# Clean everything including results
clean-all: clean
	@echo "Cleaning all artifacts..."
	rm -rf $(RESULTS_DIR)
	@echo "Clean all completed."

# Print build information
info:
	@echo "Build Configuration:"
	@echo "  Build type: $(BUILD_TYPE)"
	@echo "  Compiler: $(CC)"
	@echo "  CFLAGS: $(CFLAGS)"
	@echo "  CPPFLAGS: $(CPPFLAGS)"
	@echo "  LDFLAGS: $(LDFLAGS)"
	@echo "  LIBS: $(LIBS)"
	@echo "  Power monitoring: $(ENABLE_POWER_MONITORING)"
	@echo "  Thermal monitoring: $(ENABLE_THERMAL_MONITORING)"
	@echo "  Profiling: $(ENABLE_PROFILING)"

# Display help
help:
	@echo "FPGA NPU Benchmark Suite Makefile"
	@echo "================================="
	@echo ""
	@echo "Main Targets:"
	@echo "  all               - Build benchmark suite (default)"
	@echo "  benchmark-suite   - Build main benchmark executable"
	@echo "  framework         - Build framework library"
	@echo "  benchmarks        - Build benchmark modules"
	@echo ""
	@echo "Build Types:"
	@echo "  debug             - Debug build with symbols"
	@echo "  release           - Optimized release build"
	@echo "  profile           - Profiling build"
	@echo ""
	@echo "Test Execution:"
	@echo "  run-quick         - Quick functionality test"
	@echo "  run-full          - Comprehensive benchmark run"
	@echo "  run-throughput    - Throughput benchmarks only"
	@echo "  run-latency       - Latency benchmarks only"
	@echo "  run-scalability   - Scalability benchmarks only"
	@echo "  run-power         - Power efficiency benchmarks only"
	@echo ""
	@echo "Specific Benchmarks:"
	@echo "  run-matmul        - Matrix multiplication benchmark"
	@echo "  run-conv2d        - 2D convolution benchmark"
	@echo "  run-memory        - Memory bandwidth benchmark"
	@echo "  run-thermal       - Thermal behavior benchmark"
	@echo ""
	@echo "Analysis:"
	@echo "  report            - Generate performance report"
	@echo "  compare           - Compare benchmark results"
	@echo ""
	@echo "Installation:"
	@echo "  install           - Install benchmark suite"
	@echo "  uninstall         - Uninstall benchmark suite"
	@echo ""
	@echo "Development:"
	@echo "  format            - Format source code"
	@echo "  analyze           - Run static analysis"
	@echo "  memcheck          - Run memory leak check"
	@echo ""
	@echo "CI/Testing:"
	@echo "  ci                - Run CI pipeline"
	@echo "  test              - Run basic functionality tests"
	@echo "  regression        - Run performance regression check"
	@echo ""
	@echo "Utility:"
	@echo "  clean             - Clean build artifacts"
	@echo "  clean-all         - Clean all artifacts including results"
	@echo "  info              - Display build configuration"
	@echo "  help              - Display this help"
	@echo ""
	@echo "Configuration Variables:"
	@echo "  BUILD_TYPE=<debug|release|profile>    - Build configuration"
	@echo "  ENABLE_POWER_MONITORING=<0|1>        - Enable power monitoring"
	@echo "  ENABLE_THERMAL_MONITORING=<0|1>      - Enable thermal monitoring"
	@echo "  ENABLE_PROFILING=<0|1>               - Enable profiling support"
	@echo ""
	@echo "Examples:"
	@echo "  make                                  - Build benchmark suite"
	@echo "  make debug                            - Build debug version"
	@echo "  make run-full                         - Run all benchmarks"
	@echo "  make run-throughput BUILD_TYPE=release - Run throughput tests (release)"
	@echo "  make install                          - Install benchmark suite"

# =============================================================================
# Advanced Targets
# =============================================================================

# Parallel build
parallel:
	$(MAKE) -j$(shell nproc) all

# Cross-compilation (example for ARM)
cross-arm:
	$(MAKE) CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++ all

# Package for distribution
package: release
	@echo "Creating distribution package..."
	@mkdir -p dist/npu-benchmark-suite
	@cp $(BIN_DIR)/npu_benchmark dist/npu-benchmark-suite/
	@cp README.md dist/npu-benchmark-suite/ 2>/dev/null || true
	@cp LICENSE dist/npu-benchmark-suite/ 2>/dev/null || true
	@tar -czf npu-benchmark-suite.tar.gz -C dist npu-benchmark-suite
	@echo "Package created: npu-benchmark-suite.tar.gz"

# Dependency tracking
-include $(ALL_SOURCES:%.c=$(BUILD_DIR)/%.d)

$(BUILD_DIR)/%.d: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@$(CC) $(CPPFLAGS) -MM -MT $(BUILD_DIR)/$*.o $< > $@

# =============================================================================
# Error Handling
# =============================================================================

# Check for required tools and libraries
check-deps:
	@echo "Checking dependencies..."
	@command -v $(CC) >/dev/null 2>&1 || (echo "Error: $(CC) not found" && exit 1)
	@test -f $(NPU_LIB_DIR)/libfpga_npu.so || test -f $(NPU_LIB_DIR)/libfpga_npu.a || \
		(echo "Error: NPU library not found in $(NPU_LIB_DIR)" && exit 1)
	@echo "All dependencies found."

# Default error handling
.DEFAULT:
	@echo "Unknown target: $@"
	@echo "Run 'make help' for available targets."