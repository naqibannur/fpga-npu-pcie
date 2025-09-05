/**
 * NPU Core Module
 * 
 * Main NPU processing core containing:
 * - Processing Element array
 * - Instruction decoder
 * - Memory management unit
 * - Control logic
 */

module npu_core #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32,
    parameter PE_COUNT = 16,
    parameter INST_WIDTH = 32
) (
    input  wire clk,
    input  wire rst_n,
    
    // Host Interface
    input  wire [DATA_WIDTH-1:0] host_data_in,
    input  wire host_data_in_valid,
    output wire host_data_in_ready,
    
    output wire [DATA_WIDTH-1:0] host_data_out,
    output wire host_data_out_valid,
    input  wire host_data_out_ready,
    
    // Memory Interface
    output wire [ADDR_WIDTH-1:0] mem_addr,
    output wire [DATA_WIDTH-1:0] mem_wdata,
    input  wire [DATA_WIDTH-1:0] mem_rdata,
    output wire mem_we,
    output wire mem_re,
    input  wire mem_valid,
    
    // Status
    output wire [3:0] status
);

    // Internal state machine
    typedef enum logic [2:0] {
        IDLE,
        DECODE,
        EXECUTE,
        MEMORY_ACCESS,
        WRITEBACK
    } npu_state_t;
    
    npu_state_t current_state, next_state;
    
    // Internal registers
    reg [INST_WIDTH-1:0] instruction_reg;
    reg [DATA_WIDTH-1:0] operand_a, operand_b, result;
    reg [ADDR_WIDTH-1:0] mem_addr_reg;
    reg mem_we_reg, mem_re_reg;
    
    // Processing Element array
    wire [DATA_WIDTH-1:0] pe_results [PE_COUNT-1:0];
    wire [PE_COUNT-1:0] pe_valid;
    
    // Instruction fields
    wire [7:0] opcode = instruction_reg[31:24];
    wire [7:0] src1 = instruction_reg[23:16];
    wire [7:0] src2 = instruction_reg[15:8];
    wire [7:0] dst = instruction_reg[7:0];
    
    // State machine
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            current_state <= IDLE;
        end else begin
            current_state <= next_state;
        end
    end
    
    always_comb begin
        next_state = current_state;
        case (current_state)
            IDLE: begin
                if (host_data_in_valid) begin
                    next_state = DECODE;
                end
            end
            DECODE: begin
                next_state = EXECUTE;
            end
            EXECUTE: begin
                case (opcode)
                    8'h01, 8'h02: next_state = WRITEBACK; // ADD, SUB
                    8'h03, 8'h04: next_state = WRITEBACK; // MUL, MAC
                    8'h10, 8'h11: next_state = MEMORY_ACCESS; // LOAD, STORE
                    default: next_state = IDLE;
                endcase
            end
            MEMORY_ACCESS: begin
                if (mem_valid) begin
                    next_state = WRITEBACK;
                end
            end
            WRITEBACK: begin
                if (host_data_out_ready) begin
                    next_state = IDLE;
                end
            end
        endcase
    end
    
    // Instruction decode and execution
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            instruction_reg <= '0;
            operand_a <= '0;
            operand_b <= '0;
            result <= '0;
            mem_addr_reg <= '0;
            mem_we_reg <= 1'b0;
            mem_re_reg <= 1'b0;
        end else begin
            case (current_state)
                IDLE: begin
                    if (host_data_in_valid) begin
                        instruction_reg <= host_data_in;
                    end
                    mem_we_reg <= 1'b0;
                    mem_re_reg <= 1'b0;
                end
                DECODE: begin
                    // Decode operands (simplified)
                    operand_a <= {24'h0, src1};
                    operand_b <= {24'h0, src2};
                end
                EXECUTE: begin
                    case (opcode)
                        8'h01: result <= operand_a + operand_b; // ADD
                        8'h02: result <= operand_a - operand_b; // SUB
                        8'h03: result <= operand_a * operand_b; // MUL
                        8'h04: result <= result + (operand_a * operand_b); // MAC
                        8'h10: begin // LOAD
                            mem_addr_reg <= {24'h0, src1};
                            mem_re_reg <= 1'b1;
                        end
                        8'h11: begin // STORE
                            mem_addr_reg <= {24'h0, dst};
                            mem_we_reg <= 1'b1;
                        end
                    endcase
                end
                MEMORY_ACCESS: begin
                    if (mem_valid) begin
                        if (opcode == 8'h10) begin // LOAD
                            result <= mem_rdata;
                        end
                        mem_we_reg <= 1'b0;
                        mem_re_reg <= 1'b0;
                    end
                end
            endcase
        end
    end
    
    // Generate Processing Elements
    genvar i;
    generate
        for (i = 0; i < PE_COUNT; i++) begin : pe_array
            processing_element #(
                .DATA_WIDTH(DATA_WIDTH)
            ) u_pe (
                .clk(clk),
                .rst_n(rst_n),
                .enable(current_state == EXECUTE),
                .op_a(operand_a),
                .op_b(operand_b),
                .operation(opcode[3:0]),
                .result(pe_results[i]),
                .valid(pe_valid[i])
            );
        end
    endgenerate
    
    // Output assignments
    assign host_data_in_ready = (current_state == IDLE);
    assign host_data_out = result;
    assign host_data_out_valid = (current_state == WRITEBACK);
    
    assign mem_addr = mem_addr_reg;
    assign mem_wdata = operand_a; // For STORE operations
    assign mem_we = mem_we_reg;
    assign mem_re = mem_re_reg;
    
    assign status = {current_state[1:0], pe_valid[1:0]};

endmodule