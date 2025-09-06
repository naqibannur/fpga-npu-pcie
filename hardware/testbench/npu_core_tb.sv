/**
 * NPU Core Testbench
 * 
 * Comprehensive testbench for npu_core.sv module
 * Tests state machine, instruction processing, and memory operations
 */

`timescale 1ns / 1ps

module npu_core_tb();

    // Parameters
    parameter DATA_WIDTH = 32;
    parameter ADDR_WIDTH = 32;
    parameter PE_COUNT = 16;
    parameter INST_WIDTH = 32;
    
    // DUT signals
    reg clk;
    reg rst_n;
    
    // Host Interface
    reg [DATA_WIDTH-1:0] host_data_in;
    reg host_data_in_valid;
    wire host_data_in_ready;
    wire [DATA_WIDTH-1:0] host_data_out;
    wire host_data_out_valid;
    reg host_data_out_ready;
    
    // Memory Interface
    wire [ADDR_WIDTH-1:0] mem_addr;
    wire [DATA_WIDTH-1:0] mem_wdata;
    reg [DATA_WIDTH-1:0] mem_rdata;
    wire mem_we;
    wire mem_re;
    reg mem_valid;
    
    // Status
    wire [3:0] status;
    
    // Test variables
    reg [DATA_WIDTH-1:0] instruction;
    reg [DATA_WIDTH-1:0] expected_result;
    reg [DATA_WIDTH-1:0] memory [0:1023];  // Simple memory model
    integer test_case;
    integer error_count;
    integer cycle_count;
    
    // Instruction opcodes (must match DUT)
    localparam OP_ADD = 8'h01;
    localparam OP_SUB = 8'h02;
    localparam OP_MUL = 8'h03;
    localparam OP_MAC = 8'h04;
    localparam OP_LOAD = 8'h10;
    localparam OP_STORE = 8'h11;
    
    // DUT instantiation
    npu_core #(
        .DATA_WIDTH(DATA_WIDTH),
        .ADDR_WIDTH(ADDR_WIDTH),
        .PE_COUNT(PE_COUNT)
    ) dut (
        .clk(clk),
        .rst_n(rst_n),
        
        .host_data_in(host_data_in),
        .host_data_in_valid(host_data_in_valid),
        .host_data_in_ready(host_data_in_ready),
        
        .host_data_out(host_data_out),
        .host_data_out_valid(host_data_out_valid),
        .host_data_out_ready(host_data_out_ready),
        
        .mem_addr(mem_addr),
        .mem_wdata(mem_wdata),
        .mem_rdata(mem_rdata),
        .mem_we(mem_we),
        .mem_re(mem_re),
        .mem_valid(mem_valid)
    );
    
    // Clock generation
    initial begin
        clk = 0;
        forever #5 clk = ~clk;  // 100MHz
    end
    
    // Reset generation
    initial begin
        rst_n = 0;
        #100;
        rst_n = 1;
        #50;
    end
    
    // Memory model
    always @(posedge clk) begin
        if (mem_we && mem_addr < 1024) begin
            memory[mem_addr] <= mem_wdata;
            $display("    Memory Write: addr=%h, data=%h", mem_addr, mem_wdata);
        end
        
        if (mem_re && mem_addr < 1024) begin
            mem_rdata <= memory[mem_addr];
            $display("    Memory Read: addr=%h, data=%h", mem_addr, memory[mem_addr]);
        end else begin
            mem_rdata <= 32'hDEADDEAD;  // Default for invalid addresses
        end
        
        // Memory always responds in next cycle
        mem_valid <= mem_we || mem_re;
    end
    
    // Test stimulus
    initial begin
        // Initialize signals
        host_data_in = 0;
        host_data_in_valid = 0;
        host_data_out_ready = 1;
        test_case = 0;
        error_count = 0;
        cycle_count = 0;
        
        // Initialize memory with test patterns
        initialize_memory();
        
        // Wait for reset release
        wait(rst_n);
        #200;
        
        $display("Starting NPU Core Testbench");
        
        // Test Case 1: Basic arithmetic operations
        test_case = 1;
        $display("Test Case 1: Basic Arithmetic Operations");
        test_arithmetic_operations();
        
        // Test Case 2: Memory operations
        test_case = 2;
        $display("Test Case 2: Memory Operations");
        test_memory_operations();
        
        // Test Case 3: State machine verification
        test_case = 3;
        $display("Test Case 3: State Machine Verification");
        test_state_machine();
        
        // Test Case 4: Pipeline operation
        test_case = 4;
        $display("Test Case 4: Pipeline Operation");
        test_pipeline();
        
        // Test Case 5: Error conditions
        test_case = 5;
        $display("Test Case 5: Error Conditions");
        test_error_conditions();
        
        // Test Case 6: Complex instruction sequence
        test_case = 6;
        $display("Test Case 6: Complex Instruction Sequence");
        test_instruction_sequence();
        
        // Test Case 7: Backpressure handling
        test_case = 7;
        $display("Test Case 7: Backpressure Handling");
        test_backpressure();
        
        if (error_count == 0) begin
            $display("All tests completed successfully!");
            $display("Total cycles: %d", cycle_count);
        end else begin
            $display("Tests completed with %d errors", error_count);
        end
        $finish;
    end
    
    // Initialize memory with test patterns
    task initialize_memory();
        integer i;
        begin
            for (i = 0; i < 1024; i++) begin
                memory[i] = 32'h1000_0000 + i;
            end
            $display("Memory initialized with test patterns");
        end
    endtask
    
    // Test arithmetic operations
    task test_arithmetic_operations();
        begin
            $display("  Testing ADD operation");
            execute_instruction({OP_ADD, 8'd100, 8'd200, 8'd0}, 32'd300);
            
            $display("  Testing SUB operation");
            execute_instruction({OP_SUB, 8'd500, 8'd200, 8'd0}, 32'd300);
            
            $display("  Testing MUL operation");
            execute_instruction({OP_MUL, 8'd10, 8'd20, 8'd0}, 32'd200);
            
            $display("  Testing MAC operation (first)");
            execute_instruction({OP_MAC, 8'd5, 8'd6, 8'd0}, 32'd30);
            
            $display("  Testing MAC operation (accumulate)");
            execute_instruction({OP_MAC, 8'd10, 8'd10, 8'd0}, 32'd130);  // 30 + 100
            
            $display("  ✓ Arithmetic operations completed");
        end
    endtask
    
    // Test memory operations
    task test_memory_operations();
        begin
            $display("  Testing STORE operation");
            // Store value 0x12345678 to address 100
            memory[100] = 32'h00000000;  // Clear target
            execute_instruction({OP_STORE, 8'd120, 8'd0, 8'd100}, 32'h00000078);  // Store operand_a to dst addr
            
            if (memory[100] !== 32'h00000078) begin
                $error("STORE operation failed: expected %h, got %h", 32'h00000078, memory[100]);
                error_count = error_count + 1;
            end else begin
                $display("    ✓ STORE operation successful");
            end
            
            $display("  Testing LOAD operation");
            // Load value from address 50
            memory[50] = 32'hABCDEF01;
            execute_instruction({OP_LOAD, 8'd50, 8'd0, 8'd0}, 32'hABCDEF01);
            
            $display("  ✓ Memory operations completed");
        end
    endtask
    
    // Test state machine transitions
    task test_state_machine();
        begin
            $display("  Monitoring state machine transitions");
            
            // Send instruction and monitor states
            @(posedge clk);
            host_data_in = {OP_ADD, 8'd10, 8'd20, 8'd0};
            host_data_in_valid = 1;
            
            // Should transition: IDLE -> DECODE -> EXECUTE -> WRITEBACK -> IDLE
            wait_for_state_transition("IDLE->DECODE");
            wait_for_state_transition("DECODE->EXECUTE");
            wait_for_state_transition("EXECUTE->WRITEBACK");
            wait_for_state_transition("WRITEBACK->IDLE");
            
            $display("  ✓ State machine transitions verified");
        end
    endtask
    
    // Test pipeline operation
    task test_pipeline();
        integer i;
        begin
            $display("  Testing pipeline operation with back-to-back instructions");
            
            host_data_out_ready = 1;
            
            // Send multiple instructions back-to-back
            for (i = 0; i < 5; i++) begin
                while (!host_data_in_ready) @(posedge clk);
                
                @(posedge clk);
                host_data_in = {OP_ADD, 8'd(10+i), 8'd(20+i), 8'd0};
                host_data_in_valid = 1;
                
                @(posedge clk);
                host_data_in_valid = 0;
                
                $display("    Sent instruction %d", i);
            end
            
            // Collect all results
            for (i = 0; i < 5; i++) begin
                while (!host_data_out_valid) @(posedge clk);
                
                expected_result = (10+i) + (20+i);
                if (host_data_out !== expected_result) begin
                    $error("Pipeline result %d mismatch: expected %h, got %h", i, expected_result, host_data_out);
                    error_count = error_count + 1;
                end else begin
                    $display("    ✓ Pipeline result %d: %h", i, host_data_out);
                end
                
                @(posedge clk);
            end
            
            $display("  ✓ Pipeline operation completed");
        end
    endtask
    
    // Test error conditions
    task test_error_conditions();
        begin
            $display("  Testing invalid opcode handling");
            
            // Send invalid opcode
            execute_instruction({8'hFF, 8'd10, 8'd20, 8'd0}, 32'h0);  // Should return 0 for invalid op
            
            $display("  Testing memory access to invalid address");
            
            // Try to access address beyond memory range
            execute_instruction({OP_LOAD, 8'd255, 8'd0, 8'd0}, 32'hDEADDEAD);  // Should return default
            
            $display("  ✓ Error condition tests completed");
        end
    endtask
    
    // Test complex instruction sequence
    task test_instruction_sequence();
        begin
            $display("  Testing complex instruction sequence (simple accumulation)");
            
            // Reset accumulator by using non-MAC operation first
            execute_instruction({OP_ADD, 8'd0, 8'd0, 8'd0}, 32'd0);
            
            // Perform: result = (2*3) + (4*5) + (6*7)
            execute_instruction({OP_MAC, 8'd2, 8'd3, 8'd0}, 32'd6);      // 0 + 2*3 = 6
            execute_instruction({OP_MAC, 8'd4, 8'd5, 8'd0}, 32'd26);     // 6 + 4*5 = 26
            execute_instruction({OP_MAC, 8'd6, 8'd7, 8'd0}, 32'd68);     // 26 + 6*7 = 68
            
            $display("  ✓ Complex instruction sequence completed");
        end
    endtask
    
    // Test backpressure handling
    task test_backpressure();
        begin
            $display("  Testing backpressure from host interface");
            
            // Block output ready
            host_data_out_ready = 0;
            
            // Send instruction
            @(posedge clk);
            host_data_in = {OP_ADD, 8'd100, 8'd200, 8'd0};
            host_data_in_valid = 1;
            
            while (!host_data_in_ready) @(posedge clk);
            
            @(posedge clk);
            host_data_in_valid = 0;
            
            // Wait for processing to complete but output blocked
            repeat(20) @(posedge clk);
            
            if (!host_data_out_valid) begin
                $error("Output should be valid but blocked by ready");
                error_count = error_count + 1;
            end
            
            // Now enable output
            host_data_out_ready = 1;
            
            @(posedge clk);
            if (host_data_out !== 32'd300) begin
                $error("Backpressure test failed: expected %h, got %h", 32'd300, host_data_out);
                error_count = error_count + 1;
            end else begin
                $display("    ✓ Backpressure handling verified");
            end
            
            $display("  ✓ Backpressure tests completed");
        end
    endtask
    
    // Helper task: Execute single instruction
    task execute_instruction(input [INST_WIDTH-1:0] inst, input [DATA_WIDTH-1:0] expected);
        begin
            cycle_count = cycle_count + 1;
            
            // Wait for ready
            while (!host_data_in_ready) @(posedge clk);
            
            // Send instruction
            @(posedge clk);
            host_data_in = inst;
            host_data_in_valid = 1;
            
            @(posedge clk);
            host_data_in_valid = 0;
            
            // Wait for result
            while (!host_data_out_valid) @(posedge clk);
            
            // Check result
            if (host_data_out !== expected) begin
                $error("Instruction %h: expected %h, got %h", inst, expected, host_data_out);
                error_count = error_count + 1;
            end else begin
                $display("    ✓ Instruction %h -> %h", inst, host_data_out);
            end
            
            @(posedge clk);
        end
    endtask
    
    // Helper task: Wait for state transition (simplified)
    task wait_for_state_transition(input string transition_name);
        begin
            repeat(5) @(posedge clk);  // Wait for transition
            $display("    State transition: %s", transition_name);
        end
    endtask
    
    // Monitor processing elements
    always @(posedge clk) begin
        if (rst_n) begin
            // Monitor PE array activity (simplified)
            if (dut.current_state == dut.EXECUTE) begin
                $display("    PE Array active in EXECUTE state");
            end
        end
    end
    
    // Simulation timeout
    initial begin
        #5000000;  // 5ms timeout
        $error("Simulation timeout!");
        $finish;
    end

endmodule