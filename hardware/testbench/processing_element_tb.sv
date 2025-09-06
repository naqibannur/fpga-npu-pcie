/**
 * Processing Element Testbench
 * 
 * Comprehensive testbench for processing_element.sv module
 * Tests all arithmetic operations, accumulator, and edge cases
 */

`timescale 1ns / 1ps

module processing_element_tb();

    // Parameters
    parameter DATA_WIDTH = 32;
    
    // DUT signals
    reg clk;
    reg rst_n;
    reg enable;
    reg [DATA_WIDTH-1:0] op_a;
    reg [DATA_WIDTH-1:0] op_b;
    reg [3:0] operation;
    wire [DATA_WIDTH-1:0] result;
    wire valid;
    
    // Test variables
    reg [DATA_WIDTH-1:0] expected_result;
    reg [DATA_WIDTH-1:0] accumulator_ref;
    integer test_case;
    integer error_count;
    
    // Operation codes (must match DUT)
    localparam OP_ADD = 4'h1;
    localparam OP_SUB = 4'h2;
    localparam OP_MUL = 4'h3;
    localparam OP_MAC = 4'h4;
    
    // DUT instantiation
    processing_element #(
        .DATA_WIDTH(DATA_WIDTH)
    ) dut (
        .clk(clk),
        .rst_n(rst_n),
        .enable(enable),
        .op_a(op_a),
        .op_b(op_b),
        .operation(operation),
        .result(result),
        .valid(valid)
    );
    
    // Clock generation
    initial begin
        clk = 0;
        forever #5 clk = ~clk;  // 100MHz
    end
    
    // Reset generation
    initial begin
        rst_n = 0;
        #50;
        rst_n = 1;
        #20;
    end
    
    // Test stimulus
    initial begin
        // Initialize signals
        enable = 0;
        op_a = 0;
        op_b = 0;
        operation = 0;
        test_case = 0;
        error_count = 0;
        accumulator_ref = 0;
        
        // Wait for reset release
        wait(rst_n);
        #100;
        
        $display("Starting Processing Element Testbench");
        
        // Test Case 1: Addition operation
        test_case = 1;
        $display("Test Case 1: Addition Operations");
        test_addition();
        
        // Test Case 2: Subtraction operation
        test_case = 2;
        $display("Test Case 2: Subtraction Operations");
        test_subtraction();
        
        // Test Case 3: Multiplication operation
        test_case = 3;
        $display("Test Case 3: Multiplication Operations");
        test_multiplication();
        
        // Test Case 4: Multiply-Accumulate operation
        test_case = 4;
        $display("Test Case 4: Multiply-Accumulate Operations");
        test_mac();
        
        // Test Case 5: Edge cases and overflow
        test_case = 5;
        $display("Test Case 5: Edge Cases and Overflow");
        test_edge_cases();
        
        // Test Case 6: Enable/disable functionality
        test_case = 6;
        $display("Test Case 6: Enable/Disable Functionality");
        test_enable_disable();
        
        // Test Case 7: Invalid operations
        test_case = 7;
        $display("Test Case 7: Invalid Operations");
        test_invalid_operations();
        
        // Test Case 8: Random operation sequence
        test_case = 8;
        $display("Test Case 8: Random Operation Sequence");
        test_random_sequence();
        
        if (error_count == 0) begin
            $display("All tests completed successfully!");
        end else begin
            $display("Tests completed with %d errors", error_count);
        end
        $finish;
    end
    
    // Task: Test addition operations
    task test_addition();
        begin
            $display("  Testing basic addition operations");
            
            // Test 1: Simple addition
            perform_operation(OP_ADD, 32'd100, 32'd200, 32'd300);
            
            // Test 2: Zero addition
            perform_operation(OP_ADD, 32'd0, 32'd500, 32'd500);
            perform_operation(OP_ADD, 32'd500, 32'd0, 32'd500);
            
            // Test 3: Negative numbers (2's complement)
            perform_operation(OP_ADD, 32'hFFFFFFFF, 32'd1, 32'd0);  // -1 + 1 = 0
            perform_operation(OP_ADD, 32'hFFFFFFFE, 32'd2, 32'd0);  // -2 + 2 = 0
            
            // Test 4: Large numbers
            perform_operation(OP_ADD, 32'h7FFFFFFF, 32'd1, 32'h80000000);  // Overflow
            
            $display("  ✓ Addition tests completed");
        end
    endtask
    
    // Task: Test subtraction operations
    task test_subtraction();
        begin
            $display("  Testing basic subtraction operations");
            
            // Test 1: Simple subtraction
            perform_operation(OP_SUB, 32'd500, 32'd200, 32'd300);
            
            // Test 2: Zero subtraction
            perform_operation(OP_SUB, 32'd500, 32'd0, 32'd500);
            perform_operation(OP_SUB, 32'd500, 32'd500, 32'd0);
            
            // Test 3: Negative result
            perform_operation(OP_SUB, 32'd100, 32'd200, 32'hFFFFFF9C);  // -100
            
            // Test 4: Underflow
            perform_operation(OP_SUB, 32'h80000000, 32'd1, 32'h7FFFFFFF);
            
            $display("  ✓ Subtraction tests completed");
        end
    endtask
    
    // Task: Test multiplication operations
    task test_multiplication();
        begin
            $display("  Testing basic multiplication operations");
            
            // Test 1: Simple multiplication
            perform_operation(OP_MUL, 32'd10, 32'd20, 32'd200);
            
            // Test 2: Zero multiplication
            perform_operation(OP_MUL, 32'd0, 32'd100, 32'd0);
            perform_operation(OP_MUL, 32'd100, 32'd0, 32'd0);
            
            // Test 3: One multiplication
            perform_operation(OP_MUL, 32'd1, 32'd100, 32'd100);
            perform_operation(OP_MUL, 32'd100, 32'd1, 32'd100);
            
            // Test 4: Power of 2 multiplication
            perform_operation(OP_MUL, 32'd8, 32'd16, 32'd128);
            
            // Test 5: Large number multiplication (will overflow)
            perform_operation(OP_MUL, 32'h10000, 32'h10000, 32'h0);  // Lower 32 bits
            
            $display("  ✓ Multiplication tests completed");
        end
    endtask
    
    // Task: Test MAC operations
    task test_mac();
        begin
            $display("  Testing multiply-accumulate operations");
            
            // Reset accumulator reference
            accumulator_ref = 0;
            
            // Reset DUT to clear internal accumulator
            @(posedge clk);
            rst_n = 0;
            @(posedge clk);
            rst_n = 1;
            @(posedge clk);
            
            // Test 1: Simple MAC sequence
            perform_mac_operation(32'd2, 32'd3);    // acc = 0 + 2*3 = 6
            perform_mac_operation(32'd4, 32'd5);    // acc = 6 + 4*5 = 26
            perform_mac_operation(32'd1, 32'd10);   // acc = 26 + 1*10 = 36
            
            // Test 2: MAC with zero
            perform_mac_operation(32'd0, 32'd100);  // acc = 36 + 0*100 = 36
            perform_mac_operation(32'd100, 32'd0);  // acc = 36 + 100*0 = 36
            
            // Test 3: Negative MAC
            perform_mac_operation(32'hFFFFFFFF, 32'd1);  // acc = 36 + (-1)*1 = 35
            
            $display("  ✓ MAC tests completed");
        end
    endtask
    
    // Task: Test edge cases
    task test_edge_cases();
        begin
            $display("  Testing edge cases and boundary conditions");
            
            // Test maximum values
            perform_operation(OP_ADD, 32'hFFFFFFFF, 32'hFFFFFFFF, 32'hFFFFFFFE);
            perform_operation(OP_MUL, 32'hFFFF, 32'hFFFF, 32'hFFFE0001);
            
            // Test minimum values
            perform_operation(OP_SUB, 32'h80000000, 32'h80000000, 32'h0);
            
            // Test alternating bit patterns
            perform_operation(OP_ADD, 32'hAAAAAAAA, 32'h55555555, 32'hFFFFFFFF);
            perform_operation(OP_SUB, 32'hAAAAAAAA, 32'h55555555, 32'h55555555);
            
            $display("  ✓ Edge case tests completed");
        end
    endtask
    
    // Task: Test enable/disable functionality
    task test_enable_disable();
        begin
            $display("  Testing enable/disable functionality");
            
            // Test with enable = 0
            @(posedge clk);
            enable = 0;
            op_a = 32'd100;
            op_b = 32'd200;
            operation = OP_ADD;
            @(posedge clk);
            
            if (valid !== 1'b0) begin
                $error("Valid should be 0 when enable is 0");
                error_count = error_count + 1;
            end else begin
                $display("    ✓ Correctly disabled when enable = 0");
            end
            
            // Test enable transition
            @(posedge clk);
            enable = 1;
            @(posedge clk);
            
            if (valid !== 1'b1) begin
                $error("Valid should be 1 when enable is 1");
                error_count = error_count + 1;
            end else if (result !== 32'd300) begin
                $error("Result incorrect after enable: expected %d, got %d", 32'd300, result);
                error_count = error_count + 1;
            end else begin
                $display("    ✓ Correctly enabled and computed result");
            end
            
            $display("  ✓ Enable/disable tests completed");
        end
    endtask
    
    // Task: Test invalid operations
    task test_invalid_operations();
        begin
            $display("  Testing invalid operation codes");
            
            // Test various invalid operation codes
            perform_invalid_operation(4'h0);
            perform_invalid_operation(4'h5);
            perform_invalid_operation(4'hF);
            
            $display("  ✓ Invalid operation tests completed");
        end
    endtask
    
    // Task: Test random operation sequence
    task test_random_sequence();
        integer i, rand_op, rand_a, rand_b;
        begin
            $display("  Testing random operation sequence");
            
            for (i = 0; i < 20; i++) begin
                rand_op = $urandom_range(1, 4);  // Valid operations only
                rand_a = $urandom;
                rand_b = $urandom;
                
                case (rand_op)
                    1: expected_result = rand_a + rand_b;
                    2: expected_result = rand_a - rand_b;
                    3: expected_result = rand_a * rand_b;
                    4: begin
                        accumulator_ref = accumulator_ref + (rand_a * rand_b);
                        expected_result = accumulator_ref;
                    end
                endcase
                
                perform_operation(rand_op, rand_a, rand_b, expected_result);
            end
            
            $display("  ✓ Random sequence tests completed");
        end
    endtask
    
    // Helper task: Perform single operation
    task perform_operation(input [3:0] op, input [DATA_WIDTH-1:0] a, input [DATA_WIDTH-1:0] b, input [DATA_WIDTH-1:0] expected);
        begin
            @(posedge clk);
            enable = 1;
            operation = op;
            op_a = a;
            op_b = b;
            
            @(posedge clk);
            
            if (!valid) begin
                $error("Valid not asserted for operation %h", op);
                error_count = error_count + 1;
            end else if (result !== expected) begin
                $error("Operation %h: A=%h, B=%h, Expected=%h, Got=%h", op, a, b, expected, result);
                error_count = error_count + 1;
            end else begin
                $display("    ✓ Op %h: %h ○ %h = %h", op, a, b, result);
            end
            
            enable = 0;
        end
    endtask
    
    // Helper task: Perform MAC operation
    task perform_mac_operation(input [DATA_WIDTH-1:0] a, input [DATA_WIDTH-1:0] b);
        begin
            accumulator_ref = accumulator_ref + (a * b);
            perform_operation(OP_MAC, a, b, accumulator_ref);
        end
    endtask
    
    // Helper task: Perform invalid operation
    task perform_invalid_operation(input [3:0] op);
        begin
            @(posedge clk);
            enable = 1;
            operation = op;
            op_a = 32'd100;
            op_b = 32'd200;
            
            @(posedge clk);
            
            if (valid !== 1'b0) begin
                $error("Valid should be 0 for invalid operation %h", op);
                error_count = error_count + 1;
            end else begin
                $display("    ✓ Invalid operation %h correctly rejected", op);
            end
            
            enable = 0;
        end
    endtask
    
    // Simulation timeout
    initial begin
        #500000;  // 500us timeout
        $error("Simulation timeout!");
        $finish;
    end

endmodule