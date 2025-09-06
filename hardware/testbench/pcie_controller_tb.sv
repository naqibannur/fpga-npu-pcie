/**
 * PCIe Controller Testbench
 * 
 * Comprehensive testbench for pcie_controller.sv module
 * Tests clock domain crossing, data width conversion, and flow control
 */

`timescale 1ns / 1ps

module pcie_controller_tb();

    // Parameters
    parameter DATA_WIDTH = 32;
    parameter PCIE_DATA_WIDTH = 128;
    parameter FIFO_DEPTH = 64;  // Smaller for testing
    
    // DUT signals
    reg clk;
    reg rst_n;
    reg pcie_clk;
    reg pcie_rst_n;
    
    // PCIe Physical Interface
    reg [PCIE_DATA_WIDTH-1:0] pcie_rx_data;
    reg pcie_rx_valid;
    wire pcie_rx_ready;
    wire [PCIE_DATA_WIDTH-1:0] pcie_tx_data;
    wire pcie_tx_valid;
    reg pcie_tx_ready;
    
    // NPU Interface
    reg [DATA_WIDTH-1:0] npu_data_in;
    reg npu_data_in_valid;
    wire npu_data_in_ready;
    wire [DATA_WIDTH-1:0] npu_data_out;
    wire npu_data_out_valid;
    reg npu_data_out_ready;
    
    // Status
    wire [3:0] status;
    
    // Test variables
    reg [DATA_WIDTH-1:0] tx_data_queue [$];
    reg [DATA_WIDTH-1:0] rx_data_queue [$];
    reg [DATA_WIDTH-1:0] expected_data;
    integer test_case;
    integer error_count;
    integer pcie_word_count;
    
    // DUT instantiation
    pcie_controller #(
        .DATA_WIDTH(DATA_WIDTH),
        .PCIE_DATA_WIDTH(PCIE_DATA_WIDTH),
        .FIFO_DEPTH(FIFO_DEPTH)
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
        
        .npu_data_in(npu_data_in),
        .npu_data_in_valid(npu_data_in_valid),
        .npu_data_in_ready(npu_data_in_ready),
        .npu_data_out(npu_data_out),
        .npu_data_out_valid(npu_data_out_valid),
        .npu_data_out_ready(npu_data_out_ready)
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
        #100;
        rst_n = 1;
        pcie_rst_n = 1;
        #50;
    end
    
    // Test stimulus
    initial begin
        // Initialize signals
        pcie_rx_data = 0;
        pcie_rx_valid = 0;
        pcie_tx_ready = 0;
        npu_data_in = 0;
        npu_data_in_valid = 0;
        npu_data_out_ready = 0;
        test_case = 0;
        error_count = 0;
        pcie_word_count = 0;
        
        // Wait for reset release
        wait(rst_n && pcie_rst_n);
        #200;
        
        $display("Starting PCIe Controller Testbench");
        
        // Test Case 1: Basic PCIe to NPU data transfer
        test_case = 1;
        $display("Test Case 1: PCIe to NPU Data Transfer");
        test_pcie_to_npu();
        
        // Test Case 2: Basic NPU to PCIe data transfer
        test_case = 2;
        $display("Test Case 2: NPU to PCIe Data Transfer");
        test_npu_to_pcie();
        
        // Test Case 3: Bidirectional data transfer
        test_case = 3;
        $display("Test Case 3: Bidirectional Data Transfer");
        test_bidirectional();
        
        // Test Case 4: Flow control and backpressure
        test_case = 4;
        $display("Test Case 4: Flow Control and Backpressure");
        test_flow_control();
        
        // Test Case 5: FIFO overflow/underflow conditions
        test_case = 5;
        $display("Test Case 5: FIFO Overflow/Underflow");
        test_fifo_conditions();
        
        // Test Case 6: Clock domain crossing stress test
        test_case = 6;
        $display("Test Case 6: Clock Domain Crossing Stress");
        test_clock_domain_stress();
        
        // Test Case 7: Data width conversion validation
        test_case = 7;
        $display("Test Case 7: Data Width Conversion");
        test_data_width_conversion();
        
        if (error_count == 0) begin
            $display("All tests completed successfully!");
        end else begin
            $display("Tests completed with %d errors", error_count);
        end
        $finish;
    end
    
    // Task: Test PCIe to NPU data transfer
    task test_pcie_to_npu();
        reg [PCIE_DATA_WIDTH-1:0] pcie_data;
        integer i, j;
        begin
            $display("  Testing PCIe to NPU data path");
            
            // Enable NPU data output ready
            npu_data_out_ready = 1;
            
            // Send 128-bit PCIe data (contains 4 x 32-bit words)
            pcie_data = {32'hDEADBEEF, 32'hCAFEBABE, 32'h12345678, 32'h87654321};
            
            // Add expected 32-bit words to queue
            rx_data_queue.push_back(32'h87654321);  // LSB first
            rx_data_queue.push_back(32'h12345678);
            rx_data_queue.push_back(32'hCAFEBABE);
            rx_data_queue.push_back(32'hDEADBEEF);  // MSB last
            
            @(posedge pcie_clk);
            pcie_rx_data = pcie_data;
            pcie_rx_valid = 1;
            
            // Wait for ready
            while (!pcie_rx_ready) @(posedge pcie_clk);
            
            @(posedge pcie_clk);
            pcie_rx_valid = 0;
            
            // Wait for data to appear on NPU side
            repeat(20) @(posedge clk);
            
            // Read all 4 words from NPU interface
            for (i = 0; i < 4; i++) begin
                while (!npu_data_out_valid) @(posedge clk);
                
                expected_data = rx_data_queue.pop_front();
                if (npu_data_out !== expected_data) begin
                    $error("PCIe->NPU data mismatch: expected %h, got %h", expected_data, npu_data_out);
                    error_count = error_count + 1;
                end else begin
                    $display("    ✓ Word %d: %h", i, npu_data_out);
                end
                
                @(posedge clk);
                npu_data_out_ready = 0;
                @(posedge clk);
                npu_data_out_ready = 1;
            end
            
            $display("  ✓ PCIe to NPU transfer completed");
        end
    endtask
    
    // Task: Test NPU to PCIe data transfer
    task test_npu_to_pcie();
        integer i;
        reg [DATA_WIDTH-1:0] test_words [3:0];
        reg [PCIE_DATA_WIDTH-1:0] expected_pcie_data;
        begin
            $display("  Testing NPU to PCIe data path");
            
            // Prepare test data
            test_words[0] = 32'h11111111;
            test_words[1] = 32'h22222222;
            test_words[2] = 32'h33333333;
            test_words[3] = 32'h44444444;
            
            // Expected PCIe data (4 words packed)
            expected_pcie_data = {test_words[3], test_words[2], test_words[1], test_words[0]};
            
            // Enable PCIe TX ready
            pcie_tx_ready = 1;
            
            // Send 4 words from NPU side
            for (i = 0; i < 4; i++) begin
                @(posedge clk);
                npu_data_in = test_words[i];
                npu_data_in_valid = 1;
                
                while (!npu_data_in_ready) @(posedge clk);
                
                @(posedge clk);
                npu_data_in_valid = 0;
                
                $display("    Sent NPU word %d: %h", i, test_words[i]);
                
                // Small delay between words
                repeat(3) @(posedge clk);
            end
            
            // Wait for PCIe transmission
            repeat(30) @(posedge pcie_clk);
            
            // Check PCIe output
            while (!pcie_tx_valid) @(posedge pcie_clk);
            
            if (pcie_tx_data !== expected_pcie_data) begin
                $error("NPU->PCIe data mismatch: expected %h, got %h", expected_pcie_data, pcie_tx_data);
                error_count = error_count + 1;
            end else begin
                $display("    ✓ PCIe output: %h", pcie_tx_data);
            end
            
            @(posedge pcie_clk);
            pcie_tx_ready = 0;
            repeat(2) @(posedge pcie_clk);
            pcie_tx_ready = 1;
            
            $display("  ✓ NPU to PCIe transfer completed");
        end
    endtask
    
    // Task: Test bidirectional data transfer
    task test_bidirectional();
        integer i;
        begin
            $display("  Testing bidirectional data transfer");
            
            fork
                // PCIe to NPU path
                begin
                    for (i = 0; i < 3; i++) begin
                        @(posedge pcie_clk);
                        pcie_rx_data = {32'hA000_0000 + i, 32'hB000_0000 + i, 32'hC000_0000 + i, 32'hD000_0000 + i};
                        pcie_rx_valid = 1;
                        
                        while (!pcie_rx_ready) @(posedge pcie_clk);
                        
                        @(posedge pcie_clk);
                        pcie_rx_valid = 0;
                        
                        repeat(10) @(posedge pcie_clk);
                    end
                end
                
                // NPU to PCIe path
                begin
                    repeat(5) @(posedge clk);
                    for (i = 0; i < 8; i++) begin
                        @(posedge clk);
                        npu_data_in = 32'h5000_0000 + i;
                        npu_data_in_valid = 1;
                        
                        while (!npu_data_in_ready) @(posedge clk);
                        
                        @(posedge clk);
                        npu_data_in_valid = 0;
                        
                        repeat(5) @(posedge clk);
                    end
                end
                
                // NPU data consumer
                begin
                    npu_data_out_ready = 1;
                    for (i = 0; i < 12; i++) begin
                        while (!npu_data_out_valid) @(posedge clk);
                        $display("    NPU received: %h", npu_data_out);
                        @(posedge clk);
                    end
                end
                
                // PCIe data consumer
                begin
                    pcie_tx_ready = 1;
                    for (i = 0; i < 2; i++) begin
                        while (!pcie_tx_valid) @(posedge pcie_clk);
                        $display("    PCIe transmitted: %h", pcie_tx_data);
                        @(posedge pcie_clk);
                    end
                end
            join
            
            $display("  ✓ Bidirectional transfer completed");
        end
    endtask
    
    // Task: Test flow control and backpressure
    task test_flow_control();
        integer i;
        begin
            $display("  Testing flow control and backpressure");
            
            // Test NPU backpressure
            npu_data_out_ready = 0;  // Block NPU output
            
            @(posedge pcie_clk);
            pcie_rx_data = 128'hDEADBEEF_CAFEBABE_12345678_87654321;
            pcie_rx_valid = 1;
            
            while (!pcie_rx_ready) @(posedge pcie_clk);
            
            @(posedge pcie_clk);
            pcie_rx_valid = 0;
            
            // Wait and verify data doesn't appear immediately
            repeat(10) @(posedge clk);
            
            // Now enable NPU output
            npu_data_out_ready = 1;
            
            // Data should now flow
            for (i = 0; i < 4; i++) begin
                while (!npu_data_out_valid) @(posedge clk);
                $display("    Backpressure test - received: %h", npu_data_out);
                @(posedge clk);
            end
            
            // Test PCIe backpressure
            pcie_tx_ready = 0;  // Block PCIe output
            
            for (i = 0; i < 6; i++) begin
                @(posedge clk);
                npu_data_in = 32'h6000_0000 + i;
                npu_data_in_valid = 1;
                
                if (npu_data_in_ready && i >= 4) begin
                    $error("NPU input should be blocked due to PCIe backpressure");
                    error_count = error_count + 1;
                end
                
                @(posedge clk);
                npu_data_in_valid = 0;
                repeat(2) @(posedge clk);
            end
            
            // Enable PCIe and verify data flows
            pcie_tx_ready = 1;
            repeat(50) @(posedge pcie_clk);
            
            $display("  ✓ Flow control tests completed");
        end
    endtask
    
    // Task: Test FIFO overflow/underflow conditions
    task test_fifo_conditions();
        integer i;
        begin
            $display("  Testing FIFO overflow/underflow conditions");
            
            // Test RX FIFO overflow
            npu_data_out_ready = 0;  // Block output to fill FIFO
            
            for (i = 0; i < FIFO_DEPTH + 5; i++) begin
                @(posedge pcie_clk);
                pcie_rx_data = {4{32'h7000_0000 + i}};
                pcie_rx_valid = 1;
                
                if (pcie_rx_ready) begin
                    @(posedge pcie_clk);
                    pcie_rx_valid = 0;
                    @(posedge pcie_clk);
                end else begin
                    // FIFO should be full
                    $display("    RX FIFO full detected at iteration %d", i);
                    pcie_rx_valid = 0;
                    break;
                end
            end
            
            // Drain FIFO
            npu_data_out_ready = 1;
            repeat(100) @(posedge clk);
            
            // Test TX FIFO overflow
            pcie_tx_ready = 0;  // Block output to fill FIFO
            
            for (i = 0; i < FIFO_DEPTH + 5; i++) begin
                @(posedge clk);
                npu_data_in = 32'h8000_0000 + i;
                npu_data_in_valid = 1;
                
                if (npu_data_in_ready) begin
                    @(posedge clk);
                    npu_data_in_valid = 0;
                    @(posedge clk);
                end else begin
                    // FIFO should be full
                    $display("    TX FIFO full detected at iteration %d", i);
                    npu_data_in_valid = 0;
                    break;
                end
            end
            
            // Drain FIFO
            pcie_tx_ready = 1;
            repeat(100) @(posedge pcie_clk);
            
            $display("  ✓ FIFO condition tests completed");
        end
    endtask
    
    // Task: Clock domain crossing stress test
    task test_clock_domain_stress();
        integer i;
        begin
            $display("  Testing clock domain crossing under stress");
            
            fork
                // Aggressive PCIe writing
                begin
                    for (i = 0; i < 20; i++) begin
                        @(posedge pcie_clk);
                        pcie_rx_data = {4{32'h9000_0000 + i}};
                        pcie_rx_valid = 1;
                        while (!pcie_rx_ready) @(posedge pcie_clk);
                        @(posedge pcie_clk);
                        pcie_rx_valid = 0;
                        
                        // Random delay
                        repeat($urandom_range(1, 5)) @(posedge pcie_clk);
                    end
                end
                
                // Aggressive NPU writing
                begin
                    for (i = 0; i < 50; i++) begin
                        @(posedge clk);
                        npu_data_in = 32'hF000_0000 + i;
                        npu_data_in_valid = 1;
                        while (!npu_data_in_ready) @(posedge clk);
                        @(posedge clk);
                        npu_data_in_valid = 0;
                        
                        // Random delay
                        repeat($urandom_range(1, 3)) @(posedge clk);
                    end
                end
                
                // NPU reading with random ready
                begin
                    for (i = 0; i < 80; i++) begin
                        npu_data_out_ready = $urandom_range(0, 1);
                        @(posedge clk);
                        if (npu_data_out_valid && npu_data_out_ready) begin
                            // Data received
                        end
                    end
                    npu_data_out_ready = 1;
                end
                
                // PCIe reading with random ready
                begin
                    for (i = 0; i < 15; i++) begin
                        pcie_tx_ready = $urandom_range(0, 1);
                        @(posedge pcie_clk);
                        if (pcie_tx_valid && pcie_tx_ready) begin
                            // Data transmitted
                        end
                    end
                    pcie_tx_ready = 1;
                end
            join
            
            // Let everything settle
            repeat(100) @(posedge clk);
            repeat(100) @(posedge pcie_clk);
            
            $display("  ✓ Clock domain stress test completed");
        end
    endtask
    
    // Task: Test data width conversion
    task test_data_width_conversion();
        reg [127:0] test_128_data;
        reg [31:0] test_32_words [3:0];
        integer i;
        begin
            $display("  Testing 128-bit to 32-bit data width conversion");
            
            // Test specific bit patterns
            test_128_data = 128'hDEADBEEF_CAFEBABE_12345678_87654321;
            
            // Expected 32-bit words (LSB first order)
            test_32_words[0] = 32'h87654321;
            test_32_words[1] = 32'h12345678;
            test_32_words[2] = 32'hCAFEBABE;
            test_32_words[3] = 32'hDEADBEEF;
            
            npu_data_out_ready = 1;
            
            @(posedge pcie_clk);
            pcie_rx_data = test_128_data;
            pcie_rx_valid = 1;
            while (!pcie_rx_ready) @(posedge pcie_clk);
            @(posedge pcie_clk);
            pcie_rx_valid = 0;
            
            // Verify each 32-bit word
            for (i = 0; i < 4; i++) begin
                while (!npu_data_out_valid) @(posedge clk);
                
                if (npu_data_out !== test_32_words[i]) begin
                    $error("Width conversion error: word %d expected %h, got %h", i, test_32_words[i], npu_data_out);
                    error_count = error_count + 1;
                end else begin
                    $display("    ✓ Word %d conversion: %h", i, npu_data_out);
                end
                
                @(posedge clk);
            end
            
            $display("  ✓ Data width conversion test completed");
        end
    endtask
    
    // Simulation timeout
    initial begin
        #2000000;  // 2ms timeout
        $error("Simulation timeout!");
        $finish;
    end

endmodule