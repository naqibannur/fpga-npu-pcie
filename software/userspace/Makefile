# User-space Library Makefile

# Library information
LIB_NAME = libfpga_npu
LIB_VERSION = 1.0.0
SHARED_LIB = $(LIB_NAME).so.$(LIB_VERSION)
STATIC_LIB = $(LIB_NAME).a

# Compiler settings
CC ?= gcc
AR = ar
CFLAGS = -Wall -Wextra -fPIC -O2 -std=c99
LDFLAGS = -shared
INCLUDES = -I.

# Installation directories
PREFIX ?= /usr/local
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

# Source files
SOURCES = fpga_npu_lib.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = fpga_npu_lib.h

# Build targets
all: $(SHARED_LIB) $(STATIC_LIB)

$(SHARED_LIB): $(OBJECTS)
	$(CC) $(LDFLAGS) -Wl,-soname,$(LIB_NAME).so.1 -o $@ $^
	ln -sf $(SHARED_LIB) $(LIB_NAME).so.1
	ln -sf $(SHARED_LIB) $(LIB_NAME).so

$(STATIC_LIB): $(OBJECTS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Installation
install: all
	install -d $(LIBDIR)
	install -d $(INCLUDEDIR)
	install -m 755 $(SHARED_LIB) $(LIBDIR)/
	install -m 644 $(STATIC_LIB) $(LIBDIR)/
	install -m 644 $(HEADERS) $(INCLUDEDIR)/
	ln -sf $(LIBDIR)/$(SHARED_LIB) $(LIBDIR)/$(LIB_NAME).so.1
	ln -sf $(LIBDIR)/$(SHARED_LIB) $(LIBDIR)/$(LIB_NAME).so
	ldconfig

# Uninstall
uninstall:
	rm -f $(LIBDIR)/$(SHARED_LIB)
	rm -f $(LIBDIR)/$(STATIC_LIB)
	rm -f $(LIBDIR)/$(LIB_NAME).so*
	rm -f $(INCLUDEDIR)/$(HEADERS)
	ldconfig

# Clean
clean:
	rm -f $(OBJECTS) $(SHARED_LIB) $(STATIC_LIB) $(LIB_NAME).so*

# Package configuration
pkgconfig:
	@echo "prefix=$(PREFIX)" > fpga_npu.pc
	@echo "exec_prefix=\$${prefix}" >> fpga_npu.pc
	@echo "libdir=$(LIBDIR)" >> fpga_npu.pc
	@echo "includedir=$(INCLUDEDIR)" >> fpga_npu.pc
	@echo "" >> fpga_npu.pc
	@echo "Name: FPGA NPU Library" >> fpga_npu.pc
	@echo "Description: User-space library for FPGA NPU operations" >> fpga_npu.pc
	@echo "Version: $(LIB_VERSION)" >> fpga_npu.pc
	@echo "Libs: -L\$${libdir} -lfpga_npu" >> fpga_npu.pc
	@echo "Cflags: -I\$${includedir}" >> fpga_npu.pc

.PHONY: all install uninstall clean pkgconfig