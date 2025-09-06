# Xilinx ZCU102 Constraints File
# FPGA NPU PCIe Interface Design
# Target: Zynq UltraScale+ ZU9EG-2FFVB1156E

# Global timing constraints
create_clock -period 10.000 -name sys_clk [get_ports clk]
create_clock -period 8.000 -name pcie_clk [get_ports pcie_clk]

# Clock domain crossing constraints
set_clock_groups -asynchronous \
    -group [get_clocks sys_clk] \
    -group [get_clocks pcie_clk]

# NPU core timing constraints (300MHz target)
create_clock -period 3.333 -name npu_clk [get_nets {dut/u_npu_core/clk}]
set_clock_groups -asynchronous \
    -group [get_clocks npu_clk] \
    -group [get_clocks pcie_clk]

# Reset constraints
set_false_path -from [get_ports rst_n]
set_false_path -from [get_ports pcie_rst_n]

# Input delay constraints for PCIe interface
set_input_delay -clock pcie_clk -max 2.0 [get_ports pcie_rx_data*]
set_input_delay -clock pcie_clk -min 0.5 [get_ports pcie_rx_data*]
set_input_delay -clock pcie_clk -max 2.0 [get_ports pcie_rx_valid]
set_input_delay -clock pcie_clk -min 0.5 [get_ports pcie_rx_valid]
set_input_delay -clock pcie_clk -max 2.0 [get_ports pcie_tx_ready]
set_input_delay -clock pcie_clk -min 0.5 [get_ports pcie_tx_ready]

# Output delay constraints for PCIe interface
set_output_delay -clock pcie_clk -max 2.0 [get_ports pcie_tx_data*]
set_output_delay -clock pcie_clk -min 0.5 [get_ports pcie_tx_data*]
set_output_delay -clock pcie_clk -max 2.0 [get_ports pcie_tx_valid]
set_output_delay -clock pcie_clk -min 0.5 [get_ports pcie_tx_valid]
set_output_delay -clock pcie_clk -max 2.0 [get_ports pcie_rx_ready]
set_output_delay -clock pcie_clk -min 0.5 [get_ports pcie_rx_ready]

# Memory interface timing constraints
set_input_delay -clock sys_clk -max 3.0 [get_ports mem_rdata*]
set_input_delay -clock sys_clk -min 1.0 [get_ports mem_rdata*]
set_input_delay -clock sys_clk -max 3.0 [get_ports mem_valid]
set_input_delay -clock sys_clk -min 1.0 [get_ports mem_valid]

set_output_delay -clock sys_clk -max 3.0 [get_ports mem_addr*]
set_output_delay -clock sys_clk -min 1.0 [get_ports mem_addr*]
set_output_delay -clock sys_clk -max 3.0 [get_ports mem_wdata*]
set_output_delay -clock sys_clk -min 1.0 [get_ports mem_wdata*]
set_output_delay -clock sys_clk -max 3.0 [get_ports mem_we]
set_output_delay -clock sys_clk -min 1.0 [get_ports mem_we]
set_output_delay -clock sys_clk -max 3.0 [get_ports mem_re]
set_output_delay -clock sys_clk -min 1.0 [get_ports mem_re]

# Pin assignments for ZCU102 board
# System clock (125MHz from PCIe)
set_property PACKAGE_PIN G21 [get_ports clk]
set_property IOSTANDARD LVDS [get_ports clk]

# PCIe clock (100MHz reference)
set_property PACKAGE_PIN AB6 [get_ports pcie_clk]
set_property IOSTANDARD DIFF_SSTL12 [get_ports pcie_clk]

# Reset signals
set_property PACKAGE_PIN AM13 [get_ports rst_n]
set_property IOSTANDARD LVCMOS33 [get_ports rst_n]

set_property PACKAGE_PIN AL13 [get_ports pcie_rst_n]
set_property IOSTANDARD LVCMOS33 [get_ports pcie_rst_n]

# Status LEDs (8 LEDs)
set_property PACKAGE_PIN AG14 [get_ports {status_leds[0]}]
set_property PACKAGE_PIN AF13 [get_ports {status_leds[1]}]
set_property PACKAGE_PIN AE13 [get_ports {status_leds[2]}]
set_property PACKAGE_PIN AJ14 [get_ports {status_leds[3]}]
set_property PACKAGE_PIN AJ15 [get_ports {status_leds[4]}]
set_property PACKAGE_PIN AH13 [get_ports {status_leds[5]}]
set_property PACKAGE_PIN AH14 [get_ports {status_leds[6]}]
set_property PACKAGE_PIN AL12 [get_ports {status_leds[7]}]

set_property IOSTANDARD LVCMOS33 [get_ports {status_leds[*]}]
set_property DRIVE 8 [get_ports {status_leds[*]}]

# DIP switches (8 switches)
set_property PACKAGE_PIN AN14 [get_ports {dip_switches[0]}]
set_property PACKAGE_PIN AP14 [get_ports {dip_switches[1]}]
set_property PACKAGE_PIN AM14 [get_ports {dip_switches[2]}]
set_property PACKAGE_PIN AN13 [get_ports {dip_switches[3]}]
set_property PACKAGE_PIN AN12 [get_ports {dip_switches[4]}]
set_property PACKAGE_PIN AP12 [get_ports {dip_switches[5]}]
set_property PACKAGE_PIN AL14 [get_ports {dip_switches[6]}]
set_property PACKAGE_PIN AK13 [get_ports {dip_switches[7]}]

set_property IOSTANDARD LVCMOS33 [get_ports {dip_switches[*]}]

# PCIe interface pins (example mapping)
# Note: Actual PCIe pins should be mapped according to board schematic
set_property PACKAGE_PIN AA4 [get_ports {pcie_rx_data[0]}]
set_property PACKAGE_PIN AA3 [get_ports {pcie_rx_data[1]}]
# ... (additional PCIe data pins would be mapped here)

set_property IOSTANDARD DIFF_SSTL12 [get_ports {pcie_rx_data[*]}]
set_property IOSTANDARD DIFF_SSTL12 [get_ports {pcie_tx_data[*]}]

# DDR4 memory interface pins (mapped to PS DDR4)
# Note: These would typically connect to PS side, not PL pins
# set_property PACKAGE_PIN ... [get_ports {mem_addr[*]}]
# set_property IOSTANDARD ... [get_ports {mem_addr[*]}]

# Placement constraints for critical paths
# NPU core placement
create_pblock pblock_npu_core
add_cells_to_pblock [get_pblocks pblock_npu_core] [get_cells {dut/u_npu_core}]
resize_pblock [get_pblocks pblock_npu_core] -add {SLICE_X50Y50:SLICE_X100Y100}
resize_pblock [get_pblocks pblock_npu_core] -add {RAMB36_X5Y10:RAMB36_X10Y20}
resize_pblock [get_pblocks pblock_npu_core] -add {DSP48E2_X10Y20:DSP48E2_X20Y40}

# PCIe controller placement
create_pblock pblock_pcie_ctrl
add_cells_to_pblock [get_pblocks pblock_pcie_ctrl] [get_cells {dut/u_pcie_controller}]
resize_pblock [get_pblocks pblock_pcie_ctrl] -add {SLICE_X0Y0:SLICE_X30Y50}

# Processing element array placement
create_pblock pblock_pe_array
add_cells_to_pblock [get_pblocks pblock_pe_array] [get_cells {dut/u_npu_core/pe_array[*]}]
resize_pblock [get_pblocks pblock_pe_array] -add {SLICE_X60Y60:SLICE_X90Y90}
resize_pblock [get_pblocks pblock_pe_array] -add {DSP48E2_X12Y24:DSP48E2_X18Y36}

# Timing exceptions for asynchronous resets
set_false_path -from [get_ports rst_n] -to [all_registers]
set_false_path -from [get_ports pcie_rst_n] -to [all_registers]

# Timing exceptions for configuration signals
set_false_path -from [get_ports {dip_switches[*]}] -to [all_registers]
set_false_path -from [all_registers] -to [get_ports {status_leds[*]}]

# Maximum delay constraints for critical paths
set_max_delay 3.0 -from [get_clocks sys_clk] -to [get_clocks npu_clk]
set_max_delay 3.0 -from [get_clocks npu_clk] -to [get_clocks sys_clk]

# Multicycle path constraints for memory interface
set_multicycle_path -setup 2 -from [get_clocks sys_clk] -to [get_ports mem_addr*]
set_multicycle_path -hold 1 -from [get_clocks sys_clk] -to [get_ports mem_addr*]

# Configuration constraints
set_property CONFIG_VOLTAGE 1.8 [current_design]
set_property CFGBVS GND [current_design]

# Bitstream configuration
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 33 [current_design]
set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design]

# Power optimization
set_property POWER_OPT.PAR_NUM_CRITICAL_PATHS 20 [current_design]
set_property POWER_OPT.PAR_PATH_BASED true [current_design]