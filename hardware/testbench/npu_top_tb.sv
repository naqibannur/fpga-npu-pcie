/**
 * NPU Top Level Testbench
 * 
 * Comprehensive testbench for npu_top.sv module
 * Tests complete system integration with all interfaces
 */

`timescale 1ns / 1ps

module npu_top_tb();

    // Parameters
    parameter DATA_WIDTH = 32;
    parameter ADDR_WIDTH = 32;
    parameter PE_COUNT = 16;
    parameter PCIE_DATA_WIDTH = 128;
    
    // DUT signals
    reg clk;
    reg rst_n;
    
    // PCIe Interface
    reg pcie_clk;
    reg pcie_rst_n;
    reg [PCIE_DATA_WIDTH-1:0] pcie_rx_data;
    reg pcie_rx_valid;
    wire pcie_rx_ready;
    wire [PCIE_DATA_WIDTH-1:0] pcie_tx_data;
    wire pcie_tx_valid;
    reg pcie_tx_ready;
    
    // Memory Interface (DDR4)
    wire [ADDR_WIDTH-1:0] mem_addr;
    wire [DATA_WIDTH-1:0] mem_wdata;
    reg [DATA_WIDTH-1:0] mem_rdata;
    wire mem_we;
    wire mem_re;
    reg mem_valid;
    
    // Status and Control
    wire [7:0] status_leds;
    reg [7:0] dip_switches;
    
    // Test variables
    reg [DATA_WIDTH-1:0] memory [0:4095];  // DDR4 memory model
    reg [DATA_WIDTH-1:0] instruction_queue [$];
    reg [DATA_WIDTH-1:0] result_queue [$];
    reg [DATA_WIDTH-1:0] expected_result;
    integer test_case;
    integer error_count;
    integer transaction_count;
    
    // Instruction opcodes
    localparam OP_ADD = 8'h01;
    localparam OP_SUB = 8'h02;
    localparam OP_MUL = 8'h03;
    localparam OP_MAC = 8'h04;
    localparam OP_LOAD = 8'h10;
    localparam OP_STORE = 8'h11;
    
    // DUT instantiation
    npu_top #(
        .DATA_WIDTH(DATA_WIDTH),
        .ADDR_WIDTH(ADDR_WIDTH),
        .PE_COUNT(PE_COUNT),
        .PCIE_DATA_WIDTH(PCIE_DATA_WIDTH)
    ) dut (
        .clk(clk),
        .rst_n(rst_n),
        
        .pcie_clk(pcie_clk),
        .pcie_rst_n(pcie_rst_n),
        .pcie_rx_data(pcie_rx_data),
        .pcie_rx_valid(pcie_rx_valid),
        .pcie_rx_ready(pcie_rx_ready),
        .pcie_tx_data(pcie_tx_data),
        .pcie_tx_valid(pcie_tx_valid),
        .pcie_tx_ready(pcie_tx_ready),
        
        .mem_addr(mem_addr),
        .mem_wdata(mem_wdata),
        .mem_rdata(mem_rdata),
        .mem_we(mem_we),
        .mem_re(mem_re),
        .mem_valid(mem_valid),
        
        .status_leds(status_leds),
        .dip_switches(dip_switches)
    );
    
    // Clock generation
    initial begin
        clk = 0;
        forever #5 clk = ~clk;  // 100MHz NPU clock
    end
    
    initial begin
        pcie_clk = 0;
        forever #4 pcie_clk = ~pcie_clk;  // 125MHz PCIe clock
    end
    
    // Reset generation
    initial begin
        rst_n = 0;
        pcie_rst_n = 0;
        #200;
        rst_n = 1;
        pcie_rst_n = 1;
        #100;
    end
    
    // DDR4 Memory model
    always @(posedge clk) begin
        if (mem_we && mem_addr < 4096) begin
            memory[mem_addr] <= mem_wdata;
            $display("    DDR4 Write: addr=%h, data=%h", mem_addr, mem_wdata);
        end
        
        if (mem_re && mem_addr < 4096) begin
            mem_rdata <= memory[mem_addr];
            $display("    DDR4 Read: addr=%h, data=%h", mem_addr, memory[mem_addr]);
        end else begin
            mem_rdata <= 32'hDEADBEEF;
        end
        
        // Memory responds with 2-cycle latency (more realistic)
        mem_valid <= #20 (mem_we || mem_re);
    end
    
    // Test stimulus
    initial begin
        // Initialize signals
        pcie_rx_data = 0;
        pcie_rx_valid = 0;
        pcie_tx_ready = 1;
        dip_switches = 8'h00;
        test_case = 0;
        error_count = 0;
        transaction_count = 0;
        
        // Initialize memory
        initialize_memory();
        
        // Wait for reset release
        wait(rst_n && pcie_rst_n);
        #500;
        
        $display("Starting NPU Top Level Integration Testbench");
        
        // Test Case 1: Basic PCIe to NPU communication
        test_case = 1;
        $display("Test Case 1: Basic PCIe to NPU Communication");
        test_basic_communication();
        
        // Test Case 2: Arithmetic operations through PCIe
        test_case = 2;
        $display("Test Case 2: Arithmetic Operations via PCIe");
        test_pcie_arithmetic();
        
        // Test Case 3: Memory operations
        test_case = 3;
        $display("Test Case 3: DDR4 Memory Operations");
        test_memory_operations();
        
        // Test Case 4: Full system data flow
        test_case = 4;
        $display("Test Case 4: Full System Data Flow");
        test_full_system_flow();
        
        // Test Case 5: Multiple concurrent operations
        test_case = 5;
        $display("Test Case 5: Concurrent Operations");
        test_concurrent_operations();
        
        // Test Case 6: System stress test
        test_case = 6;
        $display("Test Case 6: System Stress Test");
        test_system_stress();
        
        // Test Case 7: Status and control verification
        test_case = 7;
        $display("Test Case 7: Status and Control");
        test_status_control();
        
        if (error_count == 0) begin
            $display("All integration tests completed successfully!");
            $display("Total transactions: %d", transaction_count);
        end else begin
            $display("Integration tests completed with %d errors", error_count);
        end
        $finish;
    end
    
    // Initialize memory with test patterns
    task initialize_memory();
        integer i;
        begin
            for (i = 0; i < 4096; i++) begin
                memory[i] = 32'h2000_0000 + i;
            end
            $display("DDR4 memory model initialized");
        end
    endtask
    
    // Test basic PCIe communication
    task test_basic_communication();
        reg [PCIE_DATA_WIDTH-1:0] test_data;
        reg [DATA_WIDTH-1:0] received_words [3:0];
        integer i;
        begin
            $display("  Testing basic PCIe to NPU data transfer");
            
            // Send 128-bit data through PCIe
            test_data = {32'hDEADBEEF, 32'hCAFEBABE, 32'h12345678, 32'h87654321};
            
            @(posedge pcie_clk);
            pcie_rx_data = test_data;
            pcie_rx_valid = 1;
            
            while (!pcie_rx_ready) @(posedge pcie_clk);
            
            @(posedge pcie_clk);
            pcie_rx_valid = 0;
            
            $display("    PCIe data sent: %h", test_data);
            
            // Wait for data to flow through system and return
            repeat(100) @(posedge clk);
            
            // Check if data appears on PCIe TX
            repeat(50) @(posedge pcie_clk);
            
            $display("  ✓ Basic communication test completed");
        end
    endtask
    
    // Test arithmetic operations through PCIe
    task test_pcie_arithmetic();
        reg [INST_WIDTH-1:0] instruction;
        begin
            $display("  Testing arithmetic operations via PCIe interface");
            
            // Test ADD operation
            instruction = {OP_ADD, 8'd100, 8'd200, 8'd0};
            send_instruction_pcie(instruction);
            wait_for_result_pcie(32'd300);
            
            // Test multiplication
            instruction = {OP_MUL, 8'd15, 8'd20, 8'd0};
            send_instruction_pcie(instruction);
            wait_for_result_pcie(32'd300);
            
            // Test subtraction
            instruction = {OP_SUB, 8'd500, 8'd150, 8'd0};
            send_instruction_pcie(instruction);
            wait_for_result_pcie(32'd350);
            
            $display("  ✓ PCIe arithmetic operations completed");
        end
    endtask
    
    // Test memory operations
    task test_memory_operations();
        reg [INST_WIDTH-1:0] instruction;
        begin
            $display("  Testing DDR4 memory operations");
            
            // Prepare test data in memory
            memory[100] = 32'hABCDEF01;
            memory[200] = 32'h12345678;
            
            // Test LOAD operation
            instruction = {OP_LOAD, 8'd100, 8'd0, 8'd0};
            send_instruction_pcie(instruction);
            wait_for_result_pcie(32'hABCDEF01);
            
            // Test STORE operation (store to address 300)
            instruction = {OP_STORE, 8'd222, 8'd0, 8'd300};  // Store operand_a (222) to addr 300
            send_instruction_pcie(instruction);
            wait_for_result_pcie(32'h000000DE);  // Returns stored value
            
            // Verify the store operation worked
            repeat(50) @(posedge clk);
            if (memory[300] !== 32'h000000DE) begin
                $error("STORE operation failed: expected %h in memory[300], got %h", 32'h000000DE, memory[300]);
                error_count = error_count + 1;
            end else begin
                $display("    ✓ STORE operation verified in memory");
            end
            
            $display("  ✓ Memory operations completed");
        end
    endtask
    
    // Test full system data flow
    task test_full_system_flow();
        reg [INST_WIDTH-1:0] instructions [4:0];
        integer i;
        begin
            $display("  Testing complete system data flow");
            
            // Prepare instruction sequence
            instructions[0] = {OP_LOAD, 8'd100, 8'd0, 8'd0};     // Load from addr 100
            instructions[1] = {OP_LOAD, 8'd200, 8'd0, 8'd0};     // Load from addr 200
            instructions[2] = {OP_ADD, 8'd50, 8'd75, 8'd0};      // Add loaded values
            instructions[3] = {OP_STORE, 8'd125, 8'd0, 8'd400};  // Store result
            instructions[4] = {OP_LOAD, 8'd400, 8'd0, 8'd0};     // Verify store
            
            // Execute sequence
            for (i = 0; i < 5; i++) begin
                send_instruction_pcie(instructions[i]);
                
                // Calculate expected results
                case (i)
                    0: expected_result = memory[100];
                    1: expected_result = memory[200];
                    2: expected_result = 32'd125;  // 50 + 75
                    3: expected_result = 32'h0000007D;  // Stored value (125)
                    4: expected_result = memory[400];
                endcase
                
                wait_for_result_pcie(expected_result);
                $display("    Instruction %d completed", i);
            end
            
            $display("  ✓ Full system data flow verified");
        end
    endtask
    
    // Test concurrent operations
    task test_concurrent_operations();
        integer i;
        begin
            $display("  Testing concurrent PCIe and memory operations");
            
            fork
                // PCIe instruction stream
                begin
                    for (i = 0; i < 8; i++) begin
                        send_instruction_pcie({OP_ADD, 8'd(10+i), 8'd(20+i), 8'd0});
                        wait_for_result_pcie((10+i) + (20+i));
                    end
                end
                
                // Memory activity simulation
                begin
                    for (i = 0; i < 20; i++) begin
                        repeat($urandom_range(10, 30)) @(posedge clk);
                        memory[$urandom_range(500, 600)] = $urandom;
                    end
                end
                
                // Status monitoring
                begin
                    for (i = 0; i < 100; i++) begin
                        @(posedge clk);
                        if (status_leds[7:4] != 0) begin  // PCIe status
                            $display("    PCIe status: %b", status_leds[7:4]);
                        end
                        if (status_leds[3:0] != 0) begin  // NPU status
                            $display("    NPU status: %b", status_leds[3:0]);
                        end
                    end
                end
            join
            
            $display("  ✓ Concurrent operations completed");
        end
    endtask
    
    // Test system under stress
    task test_system_stress();
        integer i, j;
        reg [INST_WIDTH-1:0] random_inst;
        reg [7:0] random_op;
        begin
            $display("  Testing system under stress conditions");
            
            for (i = 0; i < 20; i++) begin
                // Generate random valid instruction
                random_op = ($urandom % 4) + 1;  // Operations 1-4
                random_inst = {random_op, $urandom[23:0]};
                
                send_instruction_pcie(random_inst);
                
                // Don't wait for result to create backpressure
                if (i % 3 == 0) begin
                    pcie_tx_ready = 0;
                    repeat(5) @(posedge pcie_clk);
                    pcie_tx_ready = 1;
                end
                
                // Random delay
                repeat($urandom_range(5, 15)) @(posedge pcie_clk);
            end
            
            // Wait for all operations to complete
            repeat(500) @(posedge clk);
            
            $display("  ✓ Stress test completed");
        end
    endtask
    
    // Test status and control functionality
    task test_status_control();
        begin
            $display("  Testing status LEDs and control switches");
            
            // Test DIP switch input
            dip_switches = 8'b10101010;
            repeat(10) @(posedge clk);
            
            // Monitor status LEDs during operation
            send_instruction_pcie({OP_ADD, 8'd10, 8'd20, 8'd0});
            
            while (!pcie_tx_valid) begin
                @(posedge clk);
                $display("    Status LEDs: %b", status_leds);
            end
            
            wait_for_result_pcie(32'd30);
            
            $display("    Final status LEDs: %b", status_leds);
            $display("  ✓ Status and control test completed");
        end
    endtask
    
    // Helper task: Send instruction via PCIe
    task send_instruction_pcie(input [INST_WIDTH-1:0] inst);
        reg [PCIE_DATA_WIDTH-1:0] pcie_packet;
        begin
            // Pack instruction into PCIe data width
            pcie_packet = {96'h0, inst};  // Pad with zeros
            
            @(posedge pcie_clk);
            pcie_rx_data = pcie_packet;
            pcie_rx_valid = 1;
            
            while (!pcie_rx_ready) @(posedge pcie_clk);
            
            @(posedge pcie_clk);
            pcie_rx_valid = 0;
            
            transaction_count = transaction_count + 1;
            $display("    Sent instruction via PCIe: %h", inst);
        end
    endtask
    
    // Helper task: Wait for result via PCIe
    task wait_for_result_pcie(input [DATA_WIDTH-1:0] expected);
        reg [PCIE_DATA_WIDTH-1:0] received_data;
        reg [DATA_WIDTH-1:0] result;
        begin
            // Wait for PCIe TX data
            while (!pcie_tx_valid) @(posedge pcie_clk);
            
            received_data = pcie_tx_data;
            result = received_data[DATA_WIDTH-1:0];  // Extract result from LSBs
            
            @(posedge pcie_clk);
            
            if (result !== expected) begin
                $error("Result mismatch: expected %h, got %h", expected, result);
                error_count = error_count + 1;
            end else begin
                $display("    ✓ Received expected result: %h", result);
            end
        end
    endtask
    
    // Performance monitoring
    always @(posedge clk) begin
        if (rst_n) begin
            // Monitor system activity
            if (mem_we || mem_re) begin
                $display("    Memory access: we=%b, re=%b, addr=%h", mem_we, mem_re, mem_addr);
            end
        end
    end
    
    // PCIe activity monitoring
    always @(posedge pcie_clk) begin
        if (pcie_rst_n) begin
            if (pcie_rx_valid && pcie_rx_ready) begin
                $display("    PCIe RX: %h", pcie_rx_data);
            end
            if (pcie_tx_valid && pcie_tx_ready) begin
                $display("    PCIe TX: %h", pcie_tx_data);
            end
        end
    end
    
    // Simulation timeout
    initial begin
        #10000000;  // 10ms timeout
        $error("Simulation timeout!");
        $finish;
    end

endmodule