# Xilinx VCU118 Board Configuration
# FPGA NPU PCIe Interface Design

# Board-specific parameters
BOARD_NAME="vcu118"
BOARD_PART="xilinx.com:vcu118:part0:2.4"
FPGA_PART="xcvu9p-flga2104-2L-e"

# Clock configuration
SYS_CLK_FREQ=156.25
PCIE_CLK_FREQ=100.0
NPU_CLK_FREQ=400.0

# Memory configuration
DDR_SIZE="16GB"
DDR_WIDTH=72
DDR_SPEED=2666

# PCIe configuration
PCIE_LANES=16
PCIE_GEN=4
PCIE_MAX_PAYLOAD=512

# Resource constraints (VU9P has more resources)
MAX_LUT_UTIL=75
MAX_BRAM_UTIL=60
MAX_DSP_UTIL=80

# Performance targets (higher for VU9P)
TARGET_FREQUENCY=400
WNS_THRESHOLD=0.2

# Board-specific IP cores
PCIE_IP="pcie4_uscale_plus"
DDR_IP="ddr4_uscale_plus"
CLOCKING_IP="clk_wiz"

# Constraint files
CONSTRAINTS_FILES=(
    "vcu118.xdc"
    "timing_constraints.xdc"
    "placement_constraints.xdc"
)

# Build strategy (optimized for performance)
SYNTH_STRATEGY="Flow_PerfOptimized_high"
IMPL_STRATEGY="Performance_ExploreWithRemap"

# Programming interface
PROG_INTERFACE="JTAG"
PROG_CABLE="auto"

# Debug configuration
ILA_ENABLE=false
VIO_ENABLE=false
DEBUG_NETS=""

# SLR (Super Logic Region) configuration for VU9P
SLR_STRATEGY="auto"
SLR_ASSIGNMENTS="
    NPU_CORE:SLR0
    PCIE_CTRL:SLR1
    MEMORY_IF:SLR2
"

# Advanced features
HBM_ENABLE=false
NETWORKING_ENABLE=false

# Board-specific notes
BOARD_NOTES="
VCU118 Configuration Notes:
- Virtex UltraScale+ VU9P FPGA
- High-performance evaluation platform
- PCIe Gen4 x16 support
- DDR4 SODIMM and HBM memory options
- Multiple SLRs for large designs
- Advanced clocking and I/O capabilities
- Suitable for high-performance AI acceleration
"