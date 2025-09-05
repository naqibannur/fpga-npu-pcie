/**
 * NPU Top Level Module
 * 
 * This is the top-level module for the FPGA NPU with PCIe interface.
 * It instantiates the NPU core, PCIe interface, and memory controller.
 */

module npu_top #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32,
    parameter PE_COUNT = 16,
    parameter PCIE_DATA_WIDTH = 128
) (
    // Clock and Reset
    input  wire clk,
    input  wire rst_n,
    
    // PCIe Interface
    input  wire         pcie_clk,
    input  wire         pcie_rst_n,
    input  wire [PCIE_DATA_WIDTH-1:0] pcie_rx_data,
    input  wire         pcie_rx_valid,
    output wire         pcie_rx_ready,
    output wire [PCIE_DATA_WIDTH-1:0] pcie_tx_data,
    output wire         pcie_tx_valid,
    input  wire         pcie_tx_ready,
    
    // Memory Interface (DDR4)
    output wire [ADDR_WIDTH-1:0] mem_addr,
    output wire [DATA_WIDTH-1:0] mem_wdata,
    input  wire [DATA_WIDTH-1:0] mem_rdata,
    output wire         mem_we,
    output wire         mem_re,
    input  wire         mem_valid,
    
    // Status and Control
    output wire [7:0]   status_leds,
    input  wire [7:0]   dip_switches
);

    // Internal signals
    wire [DATA_WIDTH-1:0] npu_to_pcie_data;
    wire npu_to_pcie_valid;
    wire npu_to_pcie_ready;
    
    wire [DATA_WIDTH-1:0] pcie_to_npu_data;
    wire pcie_to_npu_valid;
    wire pcie_to_npu_ready;
    
    wire [ADDR_WIDTH-1:0] npu_mem_addr;
    wire [DATA_WIDTH-1:0] npu_mem_wdata;
    wire [DATA_WIDTH-1:0] npu_mem_rdata;
    wire npu_mem_we;
    wire npu_mem_re;
    wire npu_mem_valid;
    
    // NPU Core Instance
    npu_core #(
        .DATA_WIDTH(DATA_WIDTH),
        .ADDR_WIDTH(ADDR_WIDTH),
        .PE_COUNT(PE_COUNT)
    ) u_npu_core (
        .clk(clk),
        .rst_n(rst_n),
        
        // Host Interface
        .host_data_in(pcie_to_npu_data),
        .host_data_in_valid(pcie_to_npu_valid),
        .host_data_in_ready(pcie_to_npu_ready),
        
        .host_data_out(npu_to_pcie_data),
        .host_data_out_valid(npu_to_pcie_valid),
        .host_data_out_ready(npu_to_pcie_ready),
        
        // Memory Interface
        .mem_addr(npu_mem_addr),
        .mem_wdata(npu_mem_wdata),
        .mem_rdata(npu_mem_rdata),
        .mem_we(npu_mem_we),
        .mem_re(npu_mem_re),
        .mem_valid(npu_mem_valid),
        
        // Status
        .status(status_leds[3:0])
    );
    
    // PCIe Interface Controller
    pcie_controller #(
        .DATA_WIDTH(DATA_WIDTH),
        .PCIE_DATA_WIDTH(PCIE_DATA_WIDTH)
    ) u_pcie_controller (
        .clk(clk),
        .rst_n(rst_n),
        .pcie_clk(pcie_clk),
        .pcie_rst_n(pcie_rst_n),
        
        // PCIe Physical Interface
        .pcie_rx_data(pcie_rx_data),
        .pcie_rx_valid(pcie_rx_valid),
        .pcie_rx_ready(pcie_rx_ready),
        .pcie_tx_data(pcie_tx_data),
        .pcie_tx_valid(pcie_tx_valid),
        .pcie_tx_ready(pcie_tx_ready),
        
        // NPU Interface
        .npu_data_in(npu_to_pcie_data),
        .npu_data_in_valid(npu_to_pcie_valid),
        .npu_data_in_ready(npu_to_pcie_ready),
        
        .npu_data_out(pcie_to_npu_data),
        .npu_data_out_valid(pcie_to_npu_valid),
        .npu_data_out_ready(pcie_to_npu_ready),
        
        // Status
        .status(status_leds[7:4])
    );
    
    // Memory Controller (simplified interface)
    assign mem_addr = npu_mem_addr;
    assign mem_wdata = npu_mem_wdata;
    assign npu_mem_rdata = mem_rdata;
    assign mem_we = npu_mem_we;
    assign mem_re = npu_mem_re;
    assign npu_mem_valid = mem_valid;

endmodule