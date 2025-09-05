/**
 * Processing Element (PE) Module
 * 
 * Individual processing element for parallel computation.
 * Supports basic arithmetic operations: ADD, SUB, MUL, MAC
 */

module processing_element #(
    parameter DATA_WIDTH = 32
) (
    input  wire clk,
    input  wire rst_n,
    input  wire enable,
    
    input  wire [DATA_WIDTH-1:0] op_a,
    input  wire [DATA_WIDTH-1:0] op_b,
    input  wire [3:0] operation,
    
    output reg  [DATA_WIDTH-1:0] result,
    output reg  valid
);

    // Operation codes
    localparam OP_ADD = 4'h1;
    localparam OP_SUB = 4'h2;
    localparam OP_MUL = 4'h3;
    localparam OP_MAC = 4'h4;
    
    // Internal accumulator for MAC operations
    reg [DATA_WIDTH-1:0] accumulator;
    
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            result <= '0;
            valid <= 1'b0;
            accumulator <= '0;
        end else if (enable) begin
            valid <= 1'b1;
            case (operation)
                OP_ADD: begin
                    result <= op_a + op_b;
                end
                OP_SUB: begin
                    result <= op_a - op_b;
                end
                OP_MUL: begin
                    result <= op_a * op_b;
                end
                OP_MAC: begin
                    accumulator <= accumulator + (op_a * op_b);
                    result <= accumulator + (op_a * op_b);
                end
                default: begin
                    result <= '0;
                    valid <= 1'b0;
                end
            endcase
        end else begin
            valid <= 1'b0;
        end
    end

endmodule