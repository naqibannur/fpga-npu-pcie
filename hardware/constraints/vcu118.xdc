# Xilinx VCU118 Constraints File
# FPGA NPU PCIe Interface Design
# Target: Virtex UltraScale+ VU9P-2FLGA2104E

# Global timing constraints
create_clock -period 10.000 -name sys_clk [get_ports clk]
create_clock -period 8.000 -name pcie_clk [get_ports pcie_clk]

# High-performance NPU clock (400MHz for VU9P)
create_clock -period 2.500 -name npu_clk [get_nets {dut/u_npu_core/clk}]

# Clock domain crossing constraints
set_clock_groups -asynchronous \
    -group [get_clocks sys_clk] \
    -group [get_clocks pcie_clk] \
    -group [get_clocks npu_clk]

# Reset constraints
set_false_path -from [get_ports rst_n]
set_false_path -from [get_ports pcie_rst_n]

# PCIe interface timing constraints (higher performance)
set_input_delay -clock pcie_clk -max 1.5 [get_ports pcie_rx_data*]
set_input_delay -clock pcie_clk -min 0.3 [get_ports pcie_rx_data*]
set_input_delay -clock pcie_clk -max 1.5 [get_ports pcie_rx_valid]
set_input_delay -clock pcie_clk -min 0.3 [get_ports pcie_rx_valid]
set_input_delay -clock pcie_clk -max 1.5 [get_ports pcie_tx_ready]
set_input_delay -clock pcie_clk -min 0.3 [get_ports pcie_tx_ready]

set_output_delay -clock pcie_clk -max 1.5 [get_ports pcie_tx_data*]
set_output_delay -clock pcie_clk -min 0.3 [get_ports pcie_tx_data*]
set_output_delay -clock pcie_clk -max 1.5 [get_ports pcie_tx_valid]
set_output_delay -clock pcie_clk -min 0.3 [get_ports pcie_tx_valid]
set_output_delay -clock pcie_clk -max 1.5 [get_ports pcie_rx_ready]
set_output_delay -clock pcie_clk -min 0.3 [get_ports pcie_rx_ready]

# High-speed memory interface constraints
set_input_delay -clock sys_clk -max 2.0 [get_ports mem_rdata*]
set_input_delay -clock sys_clk -min 0.5 [get_ports mem_rdata*]
set_input_delay -clock sys_clk -max 2.0 [get_ports mem_valid]
set_input_delay -clock sys_clk -min 0.5 [get_ports mem_valid]

set_output_delay -clock sys_clk -max 2.0 [get_ports mem_addr*]
set_output_delay -clock sys_clk -min 0.5 [get_ports mem_addr*]
set_output_delay -clock sys_clk -max 2.0 [get_ports mem_wdata*]
set_output_delay -clock sys_clk -min 0.5 [get_ports mem_wdata*]
set_output_delay -clock sys_clk -max 2.0 [get_ports mem_we]
set_output_delay -clock sys_clk -min 0.5 [get_ports mem_we]
set_output_delay -clock sys_clk -max 2.0 [get_ports mem_re]
set_output_delay -clock sys_clk -min 0.5 [get_ports mem_re]

# Pin assignments for VCU118 board
# System clock (differential 156.25MHz)
set_property PACKAGE_PIN G31 [get_ports clk_p]
set_property PACKAGE_PIN F31 [get_ports clk_n]
set_property IOSTANDARD DIFF_SSTL12 [get_ports clk_p]
set_property IOSTANDARD DIFF_SSTL12 [get_ports clk_n]

# PCIe reference clock (100MHz differential)
set_property PACKAGE_PIN AB8 [get_ports pcie_clk_p]
set_property PACKAGE_PIN AB7 [get_ports pcie_clk_n]
set_property IOSTANDARD DIFF_SSTL12 [get_ports pcie_clk_p]
set_property IOSTANDARD DIFF_SSTL12 [get_ports pcie_clk_n]

# Reset signals
set_property PACKAGE_PIN L19 [get_ports rst_n]
set_property IOSTANDARD LVCMOS12 [get_ports rst_n]

set_property PACKAGE_PIN L20 [get_ports pcie_rst_n]
set_property IOSTANDARD LVCMOS12 [get_ports pcie_rst_n]

# Status LEDs (8 LEDs on VCU118)
set_property PACKAGE_PIN AT32 [get_ports {status_leds[0]}]
set_property PACKAGE_PIN AV34 [get_ports {status_leds[1]}]
set_property PACKAGE_PIN AY30 [get_ports {status_leds[2]}]
set_property PACKAGE_PIN BB32 [get_ports {status_leds[3]}]
set_property PACKAGE_PIN BF32 [get_ports {status_leds[4]}]
set_property PACKAGE_PIN AU37 [get_ports {status_leds[5]}]
set_property PACKAGE_PIN AV36 [get_ports {status_leds[6]}]
set_property PACKAGE_PIN BA37 [get_ports {status_leds[7]}]

set_property IOSTANDARD LVCMOS12 [get_ports {status_leds[*]}]
set_property DRIVE 8 [get_ports {status_leds[*]}]

# DIP switches (8 switches)
set_property PACKAGE_PIN B17 [get_ports {dip_switches[0]}]
set_property PACKAGE_PIN G16 [get_ports {dip_switches[1]}]
set_property PACKAGE_PIN J16 [get_ports {dip_switches[2]}]
set_property PACKAGE_PIN D21 [get_ports {dip_switches[3]}]
set_property PACKAGE_PIN K18 [get_ports {dip_switches[4]}]
set_property PACKAGE_PIN K19 [get_ports {dip_switches[5]}]
set_property PACKAGE_PIN L16 [get_ports {dip_switches[6]}]
set_property PACKAGE_PIN M17 [get_ports {dip_switches[7]}]

set_property IOSTANDARD LVCMOS12 [get_ports {dip_switches[*]}]

# PCIe Gen4 interface pins (x16 configuration)
# PCIe TX pins
set_property PACKAGE_PIN D1 [get_ports {pcie_tx_p[0]}]
set_property PACKAGE_PIN D2 [get_ports {pcie_tx_n[0]}]
set_property PACKAGE_PIN F1 [get_ports {pcie_tx_p[1]}]
set_property PACKAGE_PIN F2 [get_ports {pcie_tx_n[1]}]
set_property PACKAGE_PIN H1 [get_ports {pcie_tx_p[2]}]
set_property PACKAGE_PIN H2 [get_ports {pcie_tx_n[2]}]
set_property PACKAGE_PIN K1 [get_ports {pcie_tx_p[3]}]
set_property PACKAGE_PIN K2 [get_ports {pcie_tx_n[3]}]
set_property PACKAGE_PIN M1 [get_ports {pcie_tx_p[4]}]
set_property PACKAGE_PIN M2 [get_ports {pcie_tx_n[4]}]
set_property PACKAGE_PIN P1 [get_ports {pcie_tx_p[5]}]
set_property PACKAGE_PIN P2 [get_ports {pcie_tx_n[5]}]
set_property PACKAGE_PIN R1 [get_ports {pcie_tx_p[6]}]
set_property PACKAGE_PIN R2 [get_ports {pcie_tx_n[6]}]
set_property PACKAGE_PIN T1 [get_ports {pcie_tx_p[7]}]
set_property PACKAGE_PIN T2 [get_ports {pcie_tx_n[7]}]

# PCIe RX pins
set_property PACKAGE_PIN E3 [get_ports {pcie_rx_p[0]}]
set_property PACKAGE_PIN E4 [get_ports {pcie_rx_n[0]}]
set_property PACKAGE_PIN G3 [get_ports {pcie_rx_p[1]}]
set_property PACKAGE_PIN G4 [get_ports {pcie_rx_n[1]}]
set_property PACKAGE_PIN J3 [get_ports {pcie_rx_p[2]}]
set_property PACKAGE_PIN J4 [get_ports {pcie_rx_n[2]}]
set_property PACKAGE_PIN L3 [get_ports {pcie_rx_p[3]}]
set_property PACKAGE_PIN L4 [get_ports {pcie_rx_n[3]}]
set_property PACKAGE_PIN N3 [get_ports {pcie_rx_p[4]}]
set_property PACKAGE_PIN N4 [get_ports {pcie_rx_n[4]}]
set_property PACKAGE_PIN R3 [get_ports {pcie_rx_p[5]}]
set_property PACKAGE_PIN R4 [get_ports {pcie_rx_n[5]}]
set_property PACKAGE_PIN T3 [get_ports {pcie_rx_p[6]}]
set_property PACKAGE_PIN T4 [get_ports {pcie_rx_n[6]}]
set_property PACKAGE_PIN U1 [get_ports {pcie_rx_p[7]}]
set_property PACKAGE_PIN U2 [get_ports {pcie_rx_n[7]}]

# DDR4 memory interface (SODIMM connector)
# Address pins
set_property PACKAGE_PIN AM27 [get_ports {mem_addr[0]}]
set_property PACKAGE_PIN AL27 [get_ports {mem_addr[1]}]
set_property PACKAGE_PIN AN28 [get_ports {mem_addr[2]}]
set_property PACKAGE_PIN AM28 [get_ports {mem_addr[3]}]
set_property PACKAGE_PIN AP26 [get_ports {mem_addr[4]}]
set_property PACKAGE_PIN AN26 [get_ports {mem_addr[5]}]
# ... (additional address pins)

set_property IOSTANDARD SSTL12_DCI [get_ports {mem_addr[*]}]

# Data pins
set_property PACKAGE_PIN AR27 [get_ports {mem_wdata[0]}]
set_property PACKAGE_PIN AP27 [get_ports {mem_wdata[1]}]
# ... (additional data pins)

set_property IOSTANDARD POD12_DCI [get_ports {mem_wdata[*]}]
set_property IOSTANDARD POD12_DCI [get_ports {mem_rdata[*]}]

# Advanced placement constraints for VU9P
# NPU core placement (utilizing SLR0)
create_pblock pblock_npu_core
add_cells_to_pblock [get_pblocks pblock_npu_core] [get_cells {dut/u_npu_core}]
resize_pblock [get_pblocks pblock_npu_core] -add {SLR0}
resize_pblock [get_pblocks pblock_npu_core] -add {SLICE_X100Y0:SLICE_X200Y240}
resize_pblock [get_pblocks pblock_npu_core] -add {RAMB36_X10Y0:RAMB36_X20Y48}
resize_pblock [get_pblocks pblock_npu_core] -add {DSP48E2_X20Y0:DSP48E2_X40Y96}

# PCIe controller placement (utilizing SLR1)
create_pblock pblock_pcie_ctrl
add_cells_to_pblock [get_pblocks pblock_pcie_ctrl] [get_cells {dut/u_pcie_controller}]
resize_pblock [get_pblocks pblock_pcie_ctrl] -add {SLICE_X0Y240:SLICE_X100Y480}

# Processing element array - distributed across multiple regions
create_pblock pblock_pe_array_0
add_cells_to_pblock [get_pblocks pblock_pe_array_0] [get_cells {dut/u_npu_core/pe_array[0].u_pe dut/u_npu_core/pe_array[1].u_pe dut/u_npu_core/pe_array[2].u_pe dut/u_npu_core/pe_array[3].u_pe}]
resize_pblock [get_pblocks pblock_pe_array_0] -add {SLICE_X120Y60:SLICE_X160Y120}
resize_pblock [get_pblocks pblock_pe_array_0] -add {DSP48E2_X24Y24:DSP48E2_X32Y48}

create_pblock pblock_pe_array_1
add_cells_to_pblock [get_pblocks pblock_pe_array_1] [get_cells {dut/u_npu_core/pe_array[4].u_pe dut/u_npu_core/pe_array[5].u_pe dut/u_npu_core/pe_array[6].u_pe dut/u_npu_core/pe_array[7].u_pe}]
resize_pblock [get_pblocks pblock_pe_array_1] -add {SLICE_X160Y60:SLICE_X200Y120}
resize_pblock [get_pblocks pblock_pe_array_1] -add {DSP48E2_X32Y24:DSP48E2_X40Y48}

# High-speed timing constraints
set_max_delay 2.0 -from [get_clocks sys_clk] -to [get_clocks npu_clk]
set_max_delay 2.0 -from [get_clocks npu_clk] -to [get_clocks sys_clk]
set_max_delay 1.5 -from [get_clocks pcie_clk] -to [get_clocks npu_clk]
set_max_delay 1.5 -from [get_clocks npu_clk] -to [get_clocks pcie_clk]

# Timing exceptions for asynchronous paths
set_false_path -from [get_ports {dip_switches[*]}] -to [all_registers]
set_false_path -from [all_registers] -to [get_ports {status_leds[*]}]

# Multi-cycle paths for memory interface
set_multicycle_path -setup 2 -from [get_clocks sys_clk] -to [get_ports mem_addr*]
set_multicycle_path -hold 1 -from [get_clocks sys_clk] -to [get_ports mem_addr*]
set_multicycle_path -setup 2 -from [get_clocks sys_clk] -to [get_ports mem_wdata*]
set_multicycle_path -hold 1 -from [get_clocks sys_clk] -to [get_ports mem_wdata*]

# Power and thermal management
set_operating_conditions -grade extended
set_property POWER_OPT.PAR_NUM_CRITICAL_PATHS 50 [current_design]
set_property POWER_OPT.PAR_PATH_BASED true [current_design]

# Configuration settings for VCU118
set_property CONFIG_VOLTAGE 1.8 [current_design]
set_property CFGBVS GND [current_design]

# Bitstream optimizations
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 50 [current_design]
set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 8 [current_design]
set_property BITSTREAM.CONFIG.EXTMASTERCCLK_EN DISABLE [current_design]

# Advanced routing constraints
set_property CLOCK_DEDICATED_ROUTE BACKBONE [get_nets sys_clk]
set_property CLOCK_DEDICATED_ROUTE BACKBONE [get_nets pcie_clk]
set_property CLOCK_DEDICATED_ROUTE BACKBONE [get_nets npu_clk]