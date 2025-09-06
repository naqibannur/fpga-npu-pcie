# Makefile for FPGA NPU Library Unit Tests
#
# Builds and runs comprehensive unit tests for the NPU library

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -g -O0
LDFLAGS = -Wl,--wrap=open,--wrap=close,--wrap=ioctl,--wrap=malloc,--wrap=mmap,--wrap=munmap
LIBS = -lm -lpthread

# Directories
SRCDIR = ../../software/userspace
TESTDIR = .
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj

# Source files
LIB_SOURCES = $(SRCDIR)/fpga_npu_lib.c
TEST_SOURCES = test_framework.c test_core.c test_memory.c test_tensor_ops.c test_main.c
SOURCES = $(LIB_SOURCES) $(TEST_SOURCES)

# Object files
LIB_OBJECTS = $(OBJDIR)/fpga_npu_lib.o
TEST_OBJECTS = $(addprefix $(OBJDIR)/, $(TEST_SOURCES:.c=.o))
OBJECTS = $(LIB_OBJECTS) $(TEST_OBJECTS)

# Target executable
TARGET = $(BUILDDIR)/npu_unit_tests

# Include directories
INCLUDES = -I$(SRCDIR) -I../../software/driver -I.

# Default target
all: $(TARGET)

# Create build directories
$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

# Build target executable
$(TARGET): $(BUILDDIR) $(OBJDIR) $(OBJECTS)
	@echo "Linking unit tests..."
	$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@
	@echo "Unit tests built successfully: $@"

# Compile library source
$(OBJDIR)/fpga_npu_lib.o: $(SRCDIR)/fpga_npu_lib.c
	@echo "Compiling library: $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile test sources
$(OBJDIR)/%.o: $(TESTDIR)/%.c
	@echo "Compiling test: $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Run tests
test: $(TARGET)
	@echo ""
	@echo "========================================="
	@echo "         Running Unit Tests"
	@echo "========================================="
	@./$(TARGET)

# Run tests with memory checking (if valgrind is available)
test-memory: $(TARGET)
	@echo ""
	@echo "========================================="
	@echo "    Running Tests with Memory Check"
	@echo "========================================="
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET); \
	else \
		echo "Valgrind not found, running tests without memory checking..."; \
		./$(TARGET); \
	fi

# Run tests with verbose output
test-verbose: $(TARGET)
	@echo ""
	@echo "========================================="
	@echo "      Running Tests (Verbose Mode)"
	@echo "========================================="
	@./$(TARGET) --verbose

# Generate code coverage report (requires gcov)
coverage: CFLAGS += --coverage
coverage: LDFLAGS += --coverage
coverage: clean $(TARGET)
	@echo "Running tests for coverage analysis..."
	@./$(TARGET) || true
	@echo "Generating coverage report..."
	@if command -v gcov >/dev/null 2>&1; then \
		gcov $(SOURCES); \
		echo "Coverage files generated (*.gcov)"; \
	else \
		echo "gcov not found, skipping coverage analysis"; \
	fi

# Static analysis (if cppcheck is available)
analyze:
	@echo "Running static analysis..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --std=c99 --suppress=missingIncludeSystem \
			$(INCLUDES) $(TEST_SOURCES) $(LIB_SOURCES); \
	else \
		echo "cppcheck not found, skipping static analysis"; \
	fi

# Format code (if clang-format is available)
format:
	@echo "Formatting code..."
	@if command -v clang-format >/dev/null 2>&1; then \
		clang-format -i $(TEST_SOURCES); \
		echo "Code formatted"; \
	else \
		echo "clang-format not found, skipping formatting"; \
	fi

# Debug build with additional flags
debug: CFLAGS += -DDEBUG -fsanitize=address -fsanitize=undefined
debug: LDFLAGS += -fsanitize=address -fsanitize=undefined
debug: clean $(TARGET)
	@echo "Debug build completed"

# Release build with optimizations
release: CFLAGS = -Wall -Wextra -O2 -DNDEBUG
release: clean $(TARGET)
	@echo "Release build completed"

# Benchmark mode (for performance testing)
benchmark: CFLAGS += -DBENCHMARK_MODE -O2
benchmark: $(TARGET)
	@echo ""
	@echo "========================================="
	@echo "       Running Benchmark Tests"
	@echo "========================================="
	@./$(TARGET)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILDDIR)
	@rm -f *.gcov *.gcda *.gcno
	@echo "Clean completed"

# Install test dependencies (placeholder)
install-deps:
	@echo "Installing test dependencies..."
	@echo "Note: This is a placeholder. Install manually:"
	@echo "  - valgrind (for memory checking)"
	@echo "  - cppcheck (for static analysis)"
	@echo "  - clang-format (for code formatting)"
	@echo "  - gcov (usually included with gcc)"

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build unit tests (default)"
	@echo "  test         - Run unit tests"
	@echo "  test-memory  - Run tests with memory checking (valgrind)"
	@echo "  test-verbose - Run tests with verbose output"
	@echo "  coverage     - Generate code coverage report"
	@echo "  analyze      - Run static analysis (cppcheck)"
	@echo "  format       - Format source code (clang-format)"
	@echo "  debug        - Build with debug flags and sanitizers"
	@echo "  release      - Build optimized release version"
	@echo "  benchmark    - Run performance benchmarks"
	@echo "  clean        - Remove build artifacts"
	@echo "  install-deps - Show dependency installation instructions"
	@echo "  help         - Show this help message"

# Declare phony targets
.PHONY: all test test-memory test-verbose coverage analyze format debug release benchmark clean install-deps help

# Dependencies (auto-generated, simplified)
$(OBJDIR)/test_core.o: test_core.c test_framework.h $(SRCDIR)/fpga_npu_lib.h
$(OBJDIR)/test_memory.o: test_memory.c test_framework.h $(SRCDIR)/fpga_npu_lib.h
$(OBJDIR)/test_tensor_ops.o: test_tensor_ops.c test_framework.h $(SRCDIR)/fpga_npu_lib.h
$(OBJDIR)/test_main.o: test_main.c test_framework.h
$(OBJDIR)/test_framework.o: test_framework.c test_framework.h
$(OBJDIR)/fpga_npu_lib.o: $(SRCDIR)/fpga_npu_lib.c $(SRCDIR)/fpga_npu_lib.h