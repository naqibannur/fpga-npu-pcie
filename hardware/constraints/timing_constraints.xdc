# Global Timing Constraints File
# FPGA NPU PCIe Interface Design
# Common timing constraints for all target boards

# ========================================
# Clock Definitions and Relationships
# ========================================

# Primary clocks
create_clock -period 10.000 -name sys_clk [get_ports clk]
create_clock -period 8.000 -name pcie_clk [get_ports pcie_clk]

# Derived clocks
create_generated_clock -name npu_clk -source [get_ports clk] -divide_by 1 [get_pins {dut/u_npu_core/clk}]

# Clock group definitions (asynchronous domains)
set_clock_groups -asynchronous \
    -group [get_clocks sys_clk] \
    -group [get_clocks pcie_clk] \
    -group [get_clocks npu_clk]

# ========================================
# Reset and Control Signal Constraints
# ========================================

# Reset signal timing exceptions
set_false_path -from [get_ports rst_n]
set_false_path -from [get_ports pcie_rst_n]

# Configuration and status signals
set_false_path -from [get_ports {dip_switches[*]}]
set_false_path -to [get_ports {status_leds[*]}]

# ========================================
# Interface Timing Constraints
# ========================================

# PCIe Interface Timing
# Input constraints
set_input_delay -clock pcie_clk -max 2.0 [get_ports {pcie_rx_data[*]}]
set_input_delay -clock pcie_clk -min 0.5 [get_ports {pcie_rx_data[*]}]
set_input_delay -clock pcie_clk -max 2.0 [get_ports pcie_rx_valid]
set_input_delay -clock pcie_clk -min 0.5 [get_ports pcie_rx_valid]
set_input_delay -clock pcie_clk -max 2.0 [get_ports pcie_tx_ready]
set_input_delay -clock pcie_clk -min 0.5 [get_ports pcie_tx_ready]

# Output constraints
set_output_delay -clock pcie_clk -max 2.0 [get_ports {pcie_tx_data[*]}]
set_output_delay -clock pcie_clk -min 0.5 [get_ports {pcie_tx_data[*]}]
set_output_delay -clock pcie_clk -max 2.0 [get_ports pcie_tx_valid]
set_output_delay -clock pcie_clk -min 0.5 [get_ports pcie_tx_valid]
set_output_delay -clock pcie_clk -max 2.0 [get_ports pcie_rx_ready]
set_output_delay -clock pcie_clk -min 0.5 [get_ports pcie_rx_ready]

# Memory Interface Timing
# Input constraints
set_input_delay -clock sys_clk -max 3.0 [get_ports {mem_rdata[*]}]
set_input_delay -clock sys_clk -min 1.0 [get_ports {mem_rdata[*]}]
set_input_delay -clock sys_clk -max 3.0 [get_ports mem_valid]
set_input_delay -clock sys_clk -min 1.0 [get_ports mem_valid]

# Output constraints
set_output_delay -clock sys_clk -max 3.0 [get_ports {mem_addr[*]}]
set_output_delay -clock sys_clk -min 1.0 [get_ports {mem_addr[*]}]
set_output_delay -clock sys_clk -max 3.0 [get_ports {mem_wdata[*]}]
set_output_delay -clock sys_clk -min 1.0 [get_ports {mem_wdata[*]}]
set_output_delay -clock sys_clk -max 3.0 [get_ports mem_we]
set_output_delay -clock sys_clk -min 1.0 [get_ports mem_we]
set_output_delay -clock sys_clk -max 3.0 [get_ports mem_re]
set_output_delay -clock sys_clk -min 1.0 [get_ports mem_re]

# ========================================
# Clock Domain Crossing Constraints
# ========================================

# Maximum delay constraints for CDC paths
set_max_delay 8.0 -from [get_clocks sys_clk] -to [get_clocks pcie_clk]
set_max_delay 8.0 -from [get_clocks pcie_clk] -to [get_clocks sys_clk]
set_max_delay 8.0 -from [get_clocks sys_clk] -to [get_clocks npu_clk]
set_max_delay 8.0 -from [get_clocks npu_clk] -to [get_clocks sys_clk]
set_max_delay 8.0 -from [get_clocks pcie_clk] -to [get_clocks npu_clk]
set_max_delay 8.0 -from [get_clocks npu_clk] -to [get_clocks pcie_clk]

# Min delay constraints for CDC paths
set_min_delay 1.0 -from [get_clocks sys_clk] -to [get_clocks pcie_clk]
set_min_delay 1.0 -from [get_clocks pcie_clk] -to [get_clocks sys_clk]
set_min_delay 1.0 -from [get_clocks sys_clk] -to [get_clocks npu_clk]
set_min_delay 1.0 -from [get_clocks npu_clk] -to [get_clocks sys_clk]
set_min_delay 1.0 -from [get_clocks pcie_clk] -to [get_clocks npu_clk]
set_min_delay 1.0 -from [get_clocks npu_clk] -to [get_clocks pcie_clk]

# ========================================
# Multi-cycle Path Constraints
# ========================================

# Memory interface multi-cycle paths
set_multicycle_path -setup 2 -from [get_clocks sys_clk] -to [get_ports {mem_addr[*]}]
set_multicycle_path -hold 1 -from [get_clocks sys_clk] -to [get_ports {mem_addr[*]}]
set_multicycle_path -setup 2 -from [get_clocks sys_clk] -to [get_ports {mem_wdata[*]}]
set_multicycle_path -hold 1 -from [get_clocks sys_clk] -to [get_ports {mem_wdata[*]}]

# NPU core internal multi-cycle paths
set_multicycle_path -setup 2 -from [get_cells {dut/u_npu_core/operand_*}] -to [get_cells {dut/u_npu_core/result}]
set_multicycle_path -hold 1 -from [get_cells {dut/u_npu_core/operand_*}] -to [get_cells {dut/u_npu_core/result}]

# Processing element accumulator paths
set_multicycle_path -setup 2 -from [get_cells {dut/u_npu_core/pe_array[*]/u_pe/accumulator}] -to [get_cells {dut/u_npu_core/pe_array[*]/u_pe/result}]
set_multicycle_path -hold 1 -from [get_cells {dut/u_npu_core/pe_array[*]/u_pe/accumulator}] -to [get_cells {dut/u_npu_core/pe_array[*]/u_pe/result}]

# ========================================
# False Path Constraints
# ========================================

# Asynchronous FIFO constraints
set_false_path -from [get_cells {dut/u_pcie_controller/u_rx_fifo/wr_ptr_gray*}] -to [get_cells {dut/u_pcie_controller/u_rx_fifo/rd_ptr_gray_sync*}]
set_false_path -from [get_cells {dut/u_pcie_controller/u_rx_fifo/rd_ptr_gray*}] -to [get_cells {dut/u_pcie_controller/u_rx_fifo/wr_ptr_gray_sync*}]
set_false_path -from [get_cells {dut/u_pcie_controller/u_tx_fifo/wr_ptr_gray*}] -to [get_cells {dut/u_pcie_controller/u_tx_fifo/rd_ptr_gray_sync*}]
set_false_path -from [get_cells {dut/u_pcie_controller/u_tx_fifo/rd_ptr_gray*}] -to [get_cells {dut/u_pcie_controller/u_tx_fifo/wr_ptr_gray_sync*}]

# Reset synchronizer false paths
set_false_path -from [get_ports rst_n] -to [get_pins {*/rst_sync_reg*/D}]
set_false_path -from [get_ports pcie_rst_n] -to [get_pins {*/pcie_rst_sync_reg*/D}]

# Configuration register false paths
set_false_path -from [get_ports {dip_switches[*]}] -to [all_registers]

# Status output false paths
set_false_path -from [all_registers] -to [get_ports {status_leds[*]}]

# ========================================
# Critical Path Constraints
# ========================================

# NPU core critical timing paths
set_max_delay 10.0 -from [get_cells {dut/u_npu_core/instruction_reg*}] -to [get_cells {dut/u_npu_core/current_state*}]
set_max_delay 8.0 -from [get_cells {dut/u_npu_core/operand_*}] -to [get_cells {dut/u_npu_core/pe_array[*]/u_pe/op_*}]

# Processing element timing constraints
set_max_delay 5.0 -from [get_cells {dut/u_npu_core/pe_array[*]/u_pe/op_*}] -to [get_cells {dut/u_npu_core/pe_array[*]/u_pe/result*}]

# PCIe controller timing constraints
set_max_delay 6.0 -from [get_cells {dut/u_pcie_controller/rx_*}] -to [get_cells {dut/u_pcie_controller/u_rx_fifo/*}]
set_max_delay 6.0 -from [get_cells {dut/u_pcie_controller/u_tx_fifo/*}] -to [get_cells {dut/u_pcie_controller/tx_*}]

# ========================================
# Datapath Timing Constraints
# ========================================

# Arithmetic operation timing
set_max_delay 3.0 -from [get_pins {dut/u_npu_core/pe_array[*]/u_pe/op_a*}] -to [get_pins {dut/u_npu_core/pe_array[*]/u_pe/result*}]
set_max_delay 3.0 -from [get_pins {dut/u_npu_core/pe_array[*]/u_pe/op_b*}] -to [get_pins {dut/u_npu_core/pe_array[*]/u_pe/result*}]

# Memory access timing
set_max_delay 15.0 -from [get_cells {dut/u_npu_core/mem_addr_reg*}] -to [get_ports {mem_addr[*]}]
set_max_delay 15.0 -from [get_cells {dut/u_npu_core/mem_wdata*}] -to [get_ports {mem_wdata[*]}]

# ========================================
# Setup and Hold Time Constraints
# ========================================

# Input setup/hold times
set_input_delay -clock sys_clk -max 2.0 [get_ports {dip_switches[*]}]
set_input_delay -clock sys_clk -min -1.0 [get_ports {dip_switches[*]}]

# Output setup/hold times
set_output_delay -clock sys_clk -max 2.0 [get_ports {status_leds[*]}]
set_output_delay -clock sys_clk -min -1.0 [get_ports {status_leds[*]}]

# ========================================
# Clock Uncertainty and Jitter
# ========================================

# Clock uncertainty for timing analysis
set_clock_uncertainty 0.200 [get_clocks sys_clk]
set_clock_uncertainty 0.150 [get_clocks pcie_clk]
set_clock_uncertainty 0.100 [get_clocks npu_clk]

# Input jitter specifications
set_input_jitter [get_clocks sys_clk] 0.100
set_input_jitter [get_clocks pcie_clk] 0.050

# ========================================
# Temperature and Voltage Constraints
# ========================================

# Operating conditions
set_operating_conditions -grade commercial
set_case_analysis 0 [get_ports rst_n]
set_case_analysis 0 [get_ports pcie_rst_n]