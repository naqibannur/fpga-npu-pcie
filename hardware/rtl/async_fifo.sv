/**
 * Asynchronous FIFO for Clock Domain Crossing
 * 
 * Dual-clock FIFO for safe data transfer between different clock domains.
 * Uses Gray code pointers to avoid metastability issues.
 */

module async_fifo #(
    parameter DATA_WIDTH = 32,
    parameter FIFO_DEPTH = 512,
    parameter ADDR_WIDTH = $clog2(FIFO_DEPTH)
) (
    // Write interface
    input  wire wr_clk,
    input  wire wr_rst_n,
    input  wire wr_en,
    input  wire [DATA_WIDTH-1:0] wr_data,
    output wire wr_full,
    
    // Read interface
    input  wire rd_clk,
    input  wire rd_rst_n,
    input  wire rd_en,
    output wire [DATA_WIDTH-1:0] rd_data,
    output wire rd_empty
);

    // Memory array
    reg [DATA_WIDTH-1:0] memory [0:FIFO_DEPTH-1];
    
    // Gray code pointers
    reg [ADDR_WIDTH:0] wr_ptr_gray, wr_ptr_gray_next;
    reg [ADDR_WIDTH:0] rd_ptr_gray, rd_ptr_gray_next;
    
    // Binary pointers
    reg [ADDR_WIDTH:0] wr_ptr_bin, wr_ptr_bin_next;
    reg [ADDR_WIDTH:0] rd_ptr_bin, rd_ptr_bin_next;
    
    // Synchronized pointers
    reg [ADDR_WIDTH:0] wr_ptr_gray_sync [1:0];
    reg [ADDR_WIDTH:0] rd_ptr_gray_sync [1:0];
    
    // Write domain logic
    always_ff @(posedge wr_clk or negedge wr_rst_n) begin
        if (!wr_rst_n) begin
            wr_ptr_bin <= '0;
            wr_ptr_gray <= '0;
        end else begin
            wr_ptr_bin <= wr_ptr_bin_next;
            wr_ptr_gray <= wr_ptr_gray_next;
        end
    end
    
    always_comb begin
        wr_ptr_bin_next = wr_ptr_bin + (wr_en & ~wr_full);
        wr_ptr_gray_next = (wr_ptr_bin_next >> 1) ^ wr_ptr_bin_next;
    end
    
    // Read domain logic
    always_ff @(posedge rd_clk or negedge rd_rst_n) begin
        if (!rd_rst_n) begin
            rd_ptr_bin <= '0;
            rd_ptr_gray <= '0;
        end else begin
            rd_ptr_bin <= rd_ptr_bin_next;
            rd_ptr_gray <= rd_ptr_gray_next;
        end
    end
    
    always_comb begin
        rd_ptr_bin_next = rd_ptr_bin + (rd_en & ~rd_empty);
        rd_ptr_gray_next = (rd_ptr_bin_next >> 1) ^ rd_ptr_bin_next;
    end
    
    // Synchronize read pointer to write domain
    always_ff @(posedge wr_clk or negedge wr_rst_n) begin
        if (!wr_rst_n) begin
            rd_ptr_gray_sync <= '{default: '0};
        end else begin
            rd_ptr_gray_sync[0] <= rd_ptr_gray;
            rd_ptr_gray_sync[1] <= rd_ptr_gray_sync[0];
        end
    end
    
    // Synchronize write pointer to read domain
    always_ff @(posedge rd_clk or negedge rd_rst_n) begin
        if (!rd_rst_n) begin
            wr_ptr_gray_sync <= '{default: '0};
        end else begin
            wr_ptr_gray_sync[0] <= wr_ptr_gray;
            wr_ptr_gray_sync[1] <= wr_ptr_gray_sync[0];
        end
    end
    
    // Memory write
    always_ff @(posedge wr_clk) begin
        if (wr_en && !wr_full) begin
            memory[wr_ptr_bin[ADDR_WIDTH-1:0]] <= wr_data;
        end
    end
    
    // Memory read
    assign rd_data = memory[rd_ptr_bin[ADDR_WIDTH-1:0]];
    
    // Status flags
    assign wr_full = (wr_ptr_gray_next == {~rd_ptr_gray_sync[1][ADDR_WIDTH:ADDR_WIDTH-1], 
                                          rd_ptr_gray_sync[1][ADDR_WIDTH-2:0]});
    assign rd_empty = (rd_ptr_gray == wr_ptr_gray_sync[1]);

endmodule