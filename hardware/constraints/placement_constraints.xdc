# Placement Constraints File
# FPGA NPU PCIe Interface Design
# Physical placement and floorplanning constraints

# ========================================
# Global Placement Strategy
# ========================================

# Create main pblocks for system partitioning
create_pblock pblock_npu_system
create_pblock pblock_pcie_interface
create_pblock pblock_memory_interface

# ========================================
# NPU Core Placement
# ========================================

# NPU Core main pblock
add_cells_to_pblock [get_pblocks pblock_npu_system] [get_cells {dut/u_npu_core}]

# PE Array sub-placement (distribute across multiple sites)
create_pblock pblock_pe_array_quad0
add_cells_to_pblock [get_pblocks pblock_pe_array_quad0] [get_cells {dut/u_npu_core/pe_array[0].u_pe}]
add_cells_to_pblock [get_pblocks pblock_pe_array_quad0] [get_cells {dut/u_npu_core/pe_array[1].u_pe}]
add_cells_to_pblock [get_pblocks pblock_pe_array_quad0] [get_cells {dut/u_npu_core/pe_array[2].u_pe}]
add_cells_to_pblock [get_pblocks pblock_pe_array_quad0] [get_cells {dut/u_npu_core/pe_array[3].u_pe}]

create_pblock pblock_pe_array_quad1
add_cells_to_pblock [get_pblocks pblock_pe_array_quad1] [get_cells {dut/u_npu_core/pe_array[4].u_pe}]
add_cells_to_pblock [get_pblocks pblock_pe_array_quad1] [get_cells {dut/u_npu_core/pe_array[5].u_pe}]
add_cells_to_pblock [get_pblocks pblock_pe_array_quad1] [get_cells {dut/u_npu_core/pe_array[6].u_pe}]
add_cells_to_pblock [get_pblocks pblock_pe_array_quad1] [get_cells {dut/u_npu_core/pe_array[7].u_pe}]

create_pblock pblock_pe_array_quad2
add_cells_to_pblock [get_pblocks pblock_pe_array_quad2] [get_cells {dut/u_npu_core/pe_array[8].u_pe}]
add_cells_to_pblock [get_pblocks pblock_pe_array_quad2] [get_cells {dut/u_npu_core/pe_array[9].u_pe}]
add_cells_to_pblock [get_pblocks pblock_pe_array_quad2] [get_cells {dut/u_npu_core/pe_array[10].u_pe}]
add_cells_to_pblock [get_pblocks pblock_pe_array_quad2] [get_cells {dut/u_npu_core/pe_array[11].u_pe}]

create_pblock pblock_pe_array_quad3
add_cells_to_pblock [get_pblocks pblock_pe_array_quad3] [get_cells {dut/u_npu_core/pe_array[12].u_pe}]
add_cells_to_pblock [get_pblocks pblock_pe_array_quad3] [get_cells {dut/u_npu_core/pe_array[13].u_pe}]
add_cells_to_pblock [get_pblocks pblock_pe_array_quad3] [get_cells {dut/u_npu_core/pe_array[14].u_pe}]
add_cells_to_pblock [get_pblocks pblock_pe_array_quad3] [get_cells {dut/u_npu_core/pe_array[15].u_pe}]

# NPU Control Logic placement
create_pblock pblock_npu_control
add_cells_to_pblock [get_pblocks pblock_npu_control] [get_cells {dut/u_npu_core/current_state*}]
add_cells_to_pblock [get_pblocks pblock_npu_control] [get_cells {dut/u_npu_core/next_state*}]
add_cells_to_pblock [get_pblocks pblock_npu_control] [get_cells {dut/u_npu_core/instruction_reg*}]
add_cells_to_pblock [get_pblocks pblock_npu_control] [get_cells {dut/u_npu_core/operand_*}]

# ========================================
# PCIe Interface Placement
# ========================================

# PCIe Controller placement
add_cells_to_pblock [get_pblocks pblock_pcie_interface] [get_cells {dut/u_pcie_controller}]

# PCIe FIFO placement (separate region for clock domain crossing)
create_pblock pblock_pcie_fifos
add_cells_to_pblock [get_pblocks pblock_pcie_fifos] [get_cells {dut/u_pcie_controller/u_rx_fifo}]
add_cells_to_pblock [get_pblocks pblock_pcie_fifos] [get_cells {dut/u_pcie_controller/u_tx_fifo}]

# PCIe data processing logic
create_pblock pblock_pcie_datapath
add_cells_to_pblock [get_pblocks pblock_pcie_datapath] [get_cells {dut/u_pcie_controller/rx_*}]
add_cells_to_pblock [get_pblocks pblock_pcie_datapath] [get_cells {dut/u_pcie_controller/tx_*}]

# ========================================
# Memory Interface Placement
# ========================================

# Memory controller logic (if implemented in PL)
add_cells_to_pblock [get_pblocks pblock_memory_interface] [get_cells {dut/*mem*}]

# ========================================
# Resource-Specific Placement
# ========================================

# DSP placement for processing elements
# Each PE gets dedicated DSP slices for multiplication
set_property LOC DSP48E2_X0Y0 [get_cells {dut/u_npu_core/pe_array[0].u_pe/*mult*}]
set_property LOC DSP48E2_X0Y1 [get_cells {dut/u_npu_core/pe_array[1].u_pe/*mult*}]
set_property LOC DSP48E2_X0Y2 [get_cells {dut/u_npu_core/pe_array[2].u_pe/*mult*}]
set_property LOC DSP48E2_X0Y3 [get_cells {dut/u_npu_core/pe_array[3].u_pe/*mult*}]

set_property LOC DSP48E2_X1Y0 [get_cells {dut/u_npu_core/pe_array[4].u_pe/*mult*}]
set_property LOC DSP48E2_X1Y1 [get_cells {dut/u_npu_core/pe_array[5].u_pe/*mult*}]
set_property LOC DSP48E2_X1Y2 [get_cells {dut/u_npu_core/pe_array[6].u_pe/*mult*}]
set_property LOC DSP48E2_X1Y3 [get_cells {dut/u_npu_core/pe_array[7].u_pe/*mult*}]

set_property LOC DSP48E2_X2Y0 [get_cells {dut/u_npu_core/pe_array[8].u_pe/*mult*}]
set_property LOC DSP48E2_X2Y1 [get_cells {dut/u_npu_core/pe_array[9].u_pe/*mult*}]
set_property LOC DSP48E2_X2Y2 [get_cells {dut/u_npu_core/pe_array[10].u_pe/*mult*}]
set_property LOC DSP48E2_X2Y3 [get_cells {dut/u_npu_core/pe_array[11].u_pe/*mult*}]

set_property LOC DSP48E2_X3Y0 [get_cells {dut/u_npu_core/pe_array[12].u_pe/*mult*}]
set_property LOC DSP48E2_X3Y1 [get_cells {dut/u_npu_core/pe_array[13].u_pe/*mult*}]
set_property LOC DSP48E2_X3Y2 [get_cells {dut/u_npu_core/pe_array[14].u_pe/*mult*}]
set_property LOC DSP48E2_X3Y3 [get_cells {dut/u_npu_core/pe_array[15].u_pe/*mult*}]

# BRAM placement for FIFO memories
set_property LOC RAMB36_X0Y0 [get_cells {dut/u_pcie_controller/u_rx_fifo/memory_reg}]
set_property LOC RAMB36_X0Y1 [get_cells {dut/u_pcie_controller/u_tx_fifo/memory_reg}]

# ========================================
# Clock Resource Placement
# ========================================

# Clock buffer placement
set_property LOC BUFGCE_X0Y0 [get_cells {dut/*sys_clk_buf*}]
set_property LOC BUFGCE_X0Y1 [get_cells {dut/*pcie_clk_buf*}]
set_property LOC BUFGCE_X0Y2 [get_cells {dut/*npu_clk_buf*}]

# MMCM/PLL placement
set_property LOC MMCME4_ADV_X0Y0 [get_cells {dut/*clk_mmcm*}]

# ========================================
# Routing Constraints
# ========================================

# High-priority nets for critical timing
set_property CLOCK_DEDICATED_ROUTE BACKBONE [get_nets {dut/sys_clk}]
set_property CLOCK_DEDICATED_ROUTE BACKBONE [get_nets {dut/pcie_clk}]
set_property CLOCK_DEDICATED_ROUTE BACKBONE [get_nets {dut/npu_clk}]

# Reset distribution
set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets {dut/rst_n}]
set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets {dut/pcie_rst_n}]

# Critical data paths
set_property HIGH_PRIORITY true [get_nets {dut/u_npu_core/operand_a*}]
set_property HIGH_PRIORITY true [get_nets {dut/u_npu_core/operand_b*}]
set_property HIGH_PRIORITY true [get_nets {dut/u_npu_core/result*}]

# ========================================
# Physical Optimization Constraints
# ========================================

# Enable physical optimization
set_property PHYS_OPT_DESIGN.IS_ENABLED true [current_design]

# Critical path optimization
set_property CRITICAL_NETS true [get_nets {dut/u_npu_core/pe_array[*]/u_pe/result*}]

# Placement optimization
set_property PLACE_OPT.DIRECTIVE ExploreWithRemap [current_design]

# Routing optimization
set_property ROUTE_OPT.DIRECTIVE Explore [current_design]

# ========================================
# Area and Utilization Constraints
# ========================================

# Set target utilization
set_property TARGET_UTILIZATION 85 [current_design]

# Resource allocation
set_property MAX_BRAM 50 [get_pblocks pblock_pcie_interface]
set_property MAX_DSP 64 [get_pblocks pblock_npu_system]
set_property MAX_LUT 10000 [get_pblocks pblock_npu_control]

# ========================================
# Implementation Strategy Constraints
# ========================================

# Synthesis strategy
set_property SYNTH_OPT.DIRECTIVE AreaOptimized_high [current_design]

# Implementation strategy
set_property IMPL_OPT.DIRECTIVE ExploreSequentialArea [current_design]

# Place and route strategy
set_property PLACE_OPT.DIRECTIVE ExploreWithRemap [current_design]
set_property ROUTE_OPT.DIRECTIVE Explore [current_design]

# ========================================
# Hierarchical Placement
# ========================================

# Keep hierarchy for better placement control
set_property KEEP_HIERARCHY SOFT [get_cells {dut/u_npu_core}]
set_property KEEP_HIERARCHY SOFT [get_cells {dut/u_pcie_controller}]
set_property KEEP_HIERARCHY SOFT [get_cells {dut/u_npu_core/pe_array[*]}]

# Flatten for optimization where appropriate
set_property KEEP_HIERARCHY FALSE [get_cells {dut/u_pcie_controller/u_rx_fifo}]
set_property KEEP_HIERARCHY FALSE [get_cells {dut/u_pcie_controller/u_tx_fifo}]