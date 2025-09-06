# FPGA NPU Integration Test Makefile
# 
# Build system for comprehensive integration testing suite

# =============================================================================
# Configuration
# =============================================================================

# Compiler and flags
CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -Werror -g -O2
LDFLAGS := -lpthread -lm -lrt

# Directories
SRC_DIR := .
BUILD_DIR := build
BIN_DIR := bin
USERSPACE_DIR := ../../software/userspace

# Include paths
INCLUDES := -I$(USERSPACE_DIR) -I.

# Library paths and libraries
LIBPATH := -L$(USERSPACE_DIR)
LIBS := -lfpga_npu

# Source files
FRAMEWORK_SOURCES := integration_test_framework.c
TEST_SOURCES := e2e_tests.c stress_tests.c
MAIN_SOURCE := integration_test_main.c

ALL_SOURCES := $(FRAMEWORK_SOURCES) $(TEST_SOURCES) $(MAIN_SOURCE)
OBJECTS := $(ALL_SOURCES:%.c=$(BUILD_DIR)/%.o)

# Target executable
TARGET := $(BIN_DIR)/integration_test

# =============================================================================
# Build Targets
# =============================================================================

.PHONY: all clean install test run help debug release
.DEFAULT_GOAL := all

# Default build target
all: setup $(TARGET)

# Setup build directories
setup:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
	@echo "Build directories created"

# Build main target
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJECTS) $(LIBPATH) $(LIBS) $(LDFLAGS) -o $@
	@echo "Build complete: $(TARGET)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Create directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# =============================================================================
# Test Execution Targets
# =============================================================================

# Run all tests
test: $(TARGET)
	@echo "Running integration tests..."
	@./$(TARGET) --all --verbose

# Run end-to-end tests only
test-e2e: $(TARGET)
	@echo "Running end-to-end tests..."
	@./$(TARGET) --e2e --verbose

# Run stress tests only
test-stress: $(TARGET)
	@echo "Running stress tests..."
	@./$(TARGET) --stress --verbose

# Run tests with custom configuration
test-custom: $(TARGET)
	@echo "Running custom test configuration..."
	@./$(TARGET) --all --verbose --html --json --output ./custom_results

# Run quick tests (E2E only, no stress)
test-quick: $(TARGET)
	@echo "Running quick tests..."
	@./$(TARGET) --e2e --timeout 10

# =============================================================================
# Build Variants
# =============================================================================

# Debug build
debug: CFLAGS += -DDEBUG -g3 -O0
debug: $(TARGET)
	@echo "Debug build complete"

# Release build
release: CFLAGS += -DNDEBUG -O3
release: $(TARGET)
	@echo "Release build complete"

# Static analysis build
analyze: CFLAGS += -fanalyzer
analyze: $(TARGET)
	@echo "Static analysis build complete"

# =============================================================================
# Development Targets
# =============================================================================

# Install to system (requires sudo)
install: $(TARGET)
	@echo "Installing integration test suite..."
	@sudo cp $(TARGET) /usr/local/bin/fpga_npu_integration_test
	@sudo chmod +x /usr/local/bin/fpga_npu_integration_test
	@echo "Installation complete"

# Uninstall from system (requires sudo)
uninstall:
	@echo "Uninstalling integration test suite..."
	@sudo rm -f /usr/local/bin/fpga_npu_integration_test
	@echo "Uninstall complete"

# Check code style
check-style:
	@echo "Checking code style..."
	@which clang-format > /dev/null || (echo "clang-format not found" && exit 1)
	@clang-format --dry-run --Werror $(ALL_SOURCES)
	@echo "Code style check passed"

# Format code
format:
	@echo "Formatting code..."
	@which clang-format > /dev/null || (echo "clang-format not found" && exit 1)
	@clang-format -i $(ALL_SOURCES)
	@echo "Code formatting complete"

# Static analysis with cppcheck
lint:
	@echo "Running static analysis..."
	@which cppcheck > /dev/null || (echo "cppcheck not found" && exit 1)
	@cppcheck --enable=all --std=c99 --platform=unix64 \
		--suppressions-list=/dev/null \
		$(INCLUDES) $(ALL_SOURCES)
	@echo "Static analysis complete"

# Memory leak check (requires valgrind)
memcheck: $(TARGET)
	@echo "Running memory leak check..."
	@which valgrind > /dev/null || (echo "valgrind not found" && exit 1)
	@valgrind --leak-check=full --show-leak-kinds=all \
		--track-origins=yes --verbose \
		--log-file=valgrind.log \
		./$(TARGET) --e2e --timeout 5
	@echo "Memory check complete, see valgrind.log"

# Performance profiling (requires gprof)
profile: CFLAGS += -pg
profile: LDFLAGS += -pg
profile: $(TARGET)
	@echo "Running performance profiling..."
	@./$(TARGET) --e2e --timeout 5
	@gprof $(TARGET) gmon.out > profile.txt
	@echo "Performance profile generated: profile.txt"

# =============================================================================
# Documentation Targets
# =============================================================================

# Generate documentation (requires doxygen)
docs:
	@echo "Generating documentation..."
	@which doxygen > /dev/null || (echo "doxygen not found" && exit 1)
	@doxygen Doxyfile
	@echo "Documentation generated in docs/"

# =============================================================================
# Dependency Management
# =============================================================================

# Check dependencies
check-deps:
	@echo "Checking dependencies..."
	@echo "Checking userspace library..."
	@test -f $(USERSPACE_DIR)/libfpga_npu.a || \
		(echo "NPU library not found. Run 'make -C $(USERSPACE_DIR)' first" && exit 1)
	@echo "All dependencies satisfied"

# Build userspace library if needed
$(USERSPACE_DIR)/libfpga_npu.a:
	@echo "Building userspace library..."
	@$(MAKE) -C $(USERSPACE_DIR)

# =============================================================================
# Packaging Targets
# =============================================================================

# Create distribution package
dist: clean
	@echo "Creating distribution package..."
	@tar -czf fpga_npu_integration_tests.tar.gz \
		$(ALL_SOURCES) Makefile README.md
	@echo "Distribution package created: fpga_npu_integration_tests.tar.gz"

# Create development snapshot
snapshot:
	@echo "Creating development snapshot..."
	@mkdir -p snapshots
	@DATE=$$(date +%Y%m%d_%H%M%S); \
	tar -czf snapshots/integration_tests_$$DATE.tar.gz \
		$(ALL_SOURCES) Makefile README.md build/ bin/ || true
	@echo "Snapshot created in snapshots/"

# =============================================================================
# Continuous Integration Targets
# =============================================================================

# CI build target
ci: check-deps debug test lint
	@echo "CI build and test complete"

# CI quick check
ci-quick: check-deps $(TARGET) test-quick
	@echo "CI quick check complete"

# Pre-commit check
pre-commit: check-style lint test-quick
	@echo "Pre-commit checks passed"

# =============================================================================
# Utility Targets
# =============================================================================

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@rm -f valgrind.log profile.txt gmon.out
	@rm -rf test_results custom_results
	@echo "Clean complete"

# Clean everything including distribution files
distclean: clean
	@echo "Cleaning distribution files..."
	@rm -f *.tar.gz
	@rm -rf snapshots docs
	@echo "Distribution clean complete"

# Show build information
info:
	@echo "Build Information:"
	@echo "  Source Directory:    $(SRC_DIR)"
	@echo "  Build Directory:     $(BUILD_DIR)"
	@echo "  Binary Directory:    $(BIN_DIR)"
	@echo "  Target Executable:   $(TARGET)"
	@echo "  Compiler:            $(CC)"
	@echo "  Compiler Flags:      $(CFLAGS)"
	@echo "  Linker Flags:        $(LDFLAGS)"
	@echo "  Include Paths:       $(INCLUDES)"
	@echo "  Library Paths:       $(LIBPATH)"
	@echo "  Libraries:           $(LIBS)"
	@echo "  Source Files:        $(ALL_SOURCES)"

# Show available targets
help:
	@echo "FPGA NPU Integration Test Makefile"
	@echo "=================================="
	@echo ""
	@echo "Build Targets:"
	@echo "  all              Build integration test suite (default)"
	@echo "  debug            Build with debug symbols and no optimization"
	@echo "  release          Build optimized release version"
	@echo "  analyze          Build with static analysis enabled"
	@echo ""
	@echo "Test Targets:"
	@echo "  test             Run all integration tests"
	@echo "  test-e2e         Run end-to-end tests only"
	@echo "  test-stress      Run stress tests only"
	@echo "  test-quick       Run quick tests (E2E only, short timeout)"
	@echo "  test-custom      Run tests with custom configuration"
	@echo ""
	@echo "Development Targets:"
	@echo "  install          Install to system (/usr/local/bin)"
	@echo "  uninstall        Remove from system"
	@echo "  check-style      Check code formatting"
	@echo "  format           Format source code"
	@echo "  lint             Run static analysis"
	@echo "  memcheck         Run memory leak check with valgrind"
	@echo "  profile          Generate performance profile"
	@echo ""
	@echo "CI/CD Targets:"
	@echo "  ci               Complete CI build and test"
	@echo "  ci-quick         Quick CI check"
	@echo "  pre-commit       Pre-commit validation"
	@echo ""
	@echo "Utility Targets:"
	@echo "  clean            Clean build artifacts"
	@echo "  distclean        Clean everything including distributions"
	@echo "  info             Show build configuration"
	@echo "  help             Show this help message"
	@echo ""
	@echo "Packaging Targets:"
	@echo "  dist             Create distribution package"
	@echo "  snapshot         Create development snapshot"
	@echo ""

# =============================================================================
# Dependencies
# =============================================================================

# Make build depend on userspace library
$(TARGET): $(USERSPACE_DIR)/libfpga_npu.a

# Include dependency files if they exist
-include $(OBJECTS:.o=.d)

# Generate dependency files
$(BUILD_DIR)/%.d: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) $(INCLUDES) -MM -MT $(@:.d=.o) $< > $@