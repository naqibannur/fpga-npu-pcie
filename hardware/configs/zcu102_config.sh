# Xilinx ZCU102 Board Configuration
# FPGA NPU PCIe Interface Design

# Board-specific parameters
BOARD_NAME="zcu102"
BOARD_PART="xilinx.com:zcu102:part0:3.4"
FPGA_PART="xczu9eg-ffvb1156-2-e"

# Clock configuration
SYS_CLK_FREQ=100.0
PCIE_CLK_FREQ=125.0
NPU_CLK_FREQ=300.0

# Memory configuration
DDR_SIZE="4GB"
DDR_WIDTH=64
DDR_SPEED=2400

# PCIe configuration
PCIE_LANES=4
PCIE_GEN=3
PCIE_MAX_PAYLOAD=256

# Resource constraints
MAX_LUT_UTIL=80
MAX_BRAM_UTIL=70
MAX_DSP_UTIL=85

# Performance targets
TARGET_FREQUENCY=300
WNS_THRESHOLD=0.0

# Board-specific IP cores
PCIE_IP="pcie4_uscale_plus"
DDR_IP="ddr4_uscale_plus"
CLOCKING_IP="clk_wiz"

# Constraint files
CONSTRAINTS_FILES=(
    "zcu102.xdc"
    "timing_constraints.xdc"
    "placement_constraints.xdc"
)

# Build strategy
SYNTH_STRATEGY="Flow_AreaOptimized_high"
IMPL_STRATEGY="Performance_ExplorePostRoutePhysOpt"

# Programming interface
PROG_INTERFACE="JTAG"
PROG_CABLE="auto"

# Debug configuration
ILA_ENABLE=false
VIO_ENABLE=false
DEBUG_NETS=""

# Board-specific notes
BOARD_NOTES="
ZCU102 Configuration Notes:
- Zynq UltraScale+ MPSoC with PS and PL
- Supports PCIe Gen3 x4 through PL
- DDR4 SODIMM for additional memory
- Multiple clock sources available
- Extensive I/O and connectivity options
"