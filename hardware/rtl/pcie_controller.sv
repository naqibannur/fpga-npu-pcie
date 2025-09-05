/**
 * PCIe Controller Module
 * 
 * Handles PCIe communication protocol and data transfer
 * between host system and NPU core.
 */

module pcie_controller #(
    parameter DATA_WIDTH = 32,
    parameter PCIE_DATA_WIDTH = 128,
    parameter FIFO_DEPTH = 512
) (
    input  wire clk,
    input  wire rst_n,
    input  wire pcie_clk,
    input  wire pcie_rst_n,
    
    // PCIe Physical Interface
    input  wire [PCIE_DATA_WIDTH-1:0] pcie_rx_data,
    input  wire pcie_rx_valid,
    output wire pcie_rx_ready,
    
    output wire [PCIE_DATA_WIDTH-1:0] pcie_tx_data,
    output wire pcie_tx_valid,
    input  wire pcie_tx_ready,
    
    // NPU Interface
    input  wire [DATA_WIDTH-1:0] npu_data_in,
    input  wire npu_data_in_valid,
    output wire npu_data_in_ready,
    
    output wire [DATA_WIDTH-1:0] npu_data_out,
    output wire npu_data_out_valid,
    input  wire npu_data_out_ready,
    
    // Status
    output wire [3:0] status
);

    // Clock domain crossing FIFOs
    wire [DATA_WIDTH-1:0] rx_fifo_din, rx_fifo_dout;
    wire rx_fifo_wr_en, rx_fifo_rd_en;
    wire rx_fifo_full, rx_fifo_empty;
    
    wire [DATA_WIDTH-1:0] tx_fifo_din, tx_fifo_dout;
    wire tx_fifo_wr_en, tx_fifo_rd_en;
    wire tx_fifo_full, tx_fifo_empty;
    
    // PCIe to NPU data path (RX)
    async_fifo #(
        .DATA_WIDTH(DATA_WIDTH),
        .FIFO_DEPTH(FIFO_DEPTH)
    ) u_rx_fifo (
        .wr_clk(pcie_clk),
        .wr_rst_n(pcie_rst_n),
        .wr_en(rx_fifo_wr_en),
        .wr_data(rx_fifo_din),
        .wr_full(rx_fifo_full),
        
        .rd_clk(clk),
        .rd_rst_n(rst_n),
        .rd_en(rx_fifo_rd_en),
        .rd_data(rx_fifo_dout),
        .rd_empty(rx_fifo_empty)
    );
    
    // NPU to PCIe data path (TX)
    async_fifo #(
        .DATA_WIDTH(DATA_WIDTH),
        .FIFO_DEPTH(FIFO_DEPTH)
    ) u_tx_fifo (
        .wr_clk(clk),
        .wr_rst_n(rst_n),
        .wr_en(tx_fifo_wr_en),
        .wr_data(tx_fifo_din),
        .wr_full(tx_fifo_full),
        
        .rd_clk(pcie_clk),
        .rd_rst_n(pcie_rst_n),
        .rd_en(tx_fifo_rd_en),
        .rd_data(tx_fifo_dout),
        .rd_empty(tx_fifo_empty)
    );
    
    // PCIe RX data processing
    reg [1:0] rx_word_count;
    reg [PCIE_DATA_WIDTH-1:0] rx_data_reg;
    
    always_ff @(posedge pcie_clk or negedge pcie_rst_n) begin
        if (!pcie_rst_n) begin
            rx_word_count <= 2'b00;
            rx_data_reg <= '0;
        end else if (pcie_rx_valid && pcie_rx_ready) begin
            rx_data_reg <= pcie_rx_data;
            rx_word_count <= rx_word_count + 1;
        end
    end
    
    // Extract 32-bit words from 128-bit PCIe data
    assign rx_fifo_din = rx_data_reg[DATA_WIDTH*rx_word_count +: DATA_WIDTH];
    assign rx_fifo_wr_en = pcie_rx_valid && !rx_fifo_full;
    assign pcie_rx_ready = !rx_fifo_full;
    
    // PCIe TX data processing
    reg [1:0] tx_word_count;
    reg [PCIE_DATA_WIDTH-1:0] tx_data_reg;
    
    always_ff @(posedge pcie_clk or negedge pcie_rst_n) begin
        if (!pcie_rst_n) begin
            tx_word_count <= 2'b00;
            tx_data_reg <= '0;
        end else if (tx_fifo_rd_en) begin
            tx_data_reg[DATA_WIDTH*tx_word_count +: DATA_WIDTH] <= tx_fifo_dout;
            tx_word_count <= tx_word_count + 1;
        end
    end
    
    assign pcie_tx_data = tx_data_reg;
    assign pcie_tx_valid = (tx_word_count == 2'b11) && !tx_fifo_empty;
    assign tx_fifo_rd_en = pcie_tx_ready && !tx_fifo_empty;
    
    // NPU interface connections
    assign npu_data_out = rx_fifo_dout;
    assign npu_data_out_valid = !rx_fifo_empty;
    assign rx_fifo_rd_en = npu_data_out_ready && !rx_fifo_empty;
    
    assign tx_fifo_din = npu_data_in;
    assign tx_fifo_wr_en = npu_data_in_valid && !tx_fifo_full;
    assign npu_data_in_ready = !tx_fifo_full;
    
    // Status indicators
    assign status = {rx_fifo_full, rx_fifo_empty, tx_fifo_full, tx_fifo_empty};

endmodule