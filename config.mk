# Project Configuration File
# Global configuration settings for FPGA NPU project

# Project information
PROJECT_NAME = "FPGA NPU PCIe"
PROJECT_VERSION = "1.0.0"
PROJECT_AUTHOR = "Naqib Annur"

# Hardware configuration
DEFAULT_BOARD = "zcu102"
FPGA_FAMILY = "zynq_ultrascale_plus"
TARGET_FREQUENCY = "200"  # MHz

# NPU configuration
NPU_PE_COUNT = 16
NPU_DATA_WIDTH = 32
NPU_ADDR_WIDTH = 32
NPU_CACHE_SIZE = 32768  # bytes

# PCIe configuration
PCIE_VERSION = "3.0"
PCIE_LANES = 4
PCIE_DATA_WIDTH = 128
VENDOR_ID = "0x10EE"  # Xilinx
DEVICE_ID = "0x7024"  # Custom

# Memory configuration
DDR_SIZE = "2GB"
DDR_WIDTH = 64
DMA_BUFFER_SIZE = "64KB"

# Software configuration
KERNEL_MIN_VERSION = "4.15"
GCC_MIN_VERSION = "7.0"

# Build configuration
BUILD_TYPE = "Release"  # Debug, Release
VERBOSE_BUILD = false
PARALLEL_JOBS = 4

# Installation paths
INSTALL_PREFIX = "/usr/local"
DRIVER_INSTALL_PATH = "/lib/modules"
LIBRARY_INSTALL_PATH = "/usr/local/lib"
HEADER_INSTALL_PATH = "/usr/local/include"

# Documentation
DOCS_FORMAT = "markdown"  # markdown, latex, html
GENERATE_API_DOCS = true

# Testing
ENABLE_HARDWARE_TESTS = true
ENABLE_SOFTWARE_TESTS = true
TEST_TIMEOUT = 30  # seconds