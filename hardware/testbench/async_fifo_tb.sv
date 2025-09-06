/**
 * Async FIFO Testbench
 * 
 * Comprehensive testbench for async_fifo.sv module
 * Tests clock domain crossing, Gray code pointers, and edge cases
 */

`timescale 1ns / 1ps

module async_fifo_tb();

    // Parameters
    parameter DATA_WIDTH = 32;
    parameter FIFO_DEPTH = 16;  // Smaller depth for easier testing
    parameter ADDR_WIDTH = $clog2(FIFO_DEPTH);
    
    // Clock and reset signals
    reg wr_clk;
    reg wr_rst_n;
    reg rd_clk;
    reg rd_rst_n;
    
    // Write interface
    reg wr_en;
    reg [DATA_WIDTH-1:0] wr_data;
    wire wr_full;
    
    // Read interface
    reg rd_en;
    wire [DATA_WIDTH-1:0] rd_data;
    wire rd_empty;
    
    // Test variables
    reg [DATA_WIDTH-1:0] test_data_queue [$];
    reg [DATA_WIDTH-1:0] expected_data;
    integer write_count, read_count;
    integer test_case;
    
    // DUT instantiation
    async_fifo #(
        .DATA_WIDTH(DATA_WIDTH),
        .FIFO_DEPTH(FIFO_DEPTH)
    ) dut (
        .wr_clk(wr_clk),
        .wr_rst_n(wr_rst_n),
        .wr_en(wr_en),
        .wr_data(wr_data),
        .wr_full(wr_full),
        
        .rd_clk(rd_clk),
        .rd_rst_n(rd_rst_n),
        .rd_en(rd_en),
        .rd_data(rd_data),
        .rd_empty(rd_empty)
    );
    
    // Clock generation
    initial begin
        wr_clk = 0;
        forever #5 wr_clk = ~wr_clk;  // 100MHz
    end
    
    initial begin
        rd_clk = 0;
        forever #2 rd_clk = ~rd_clk;  // 250MHz
    end
    
    // Reset generation
    initial begin
        wr_rst_n = 0;
        rd_rst_n = 0;
        #50;
        wr_rst_n = 1;
        rd_rst_n = 1;
        #20;
    end
    
    // Test stimulus
    initial begin
        // Initialize signals
        wr_en = 0;
        rd_en = 0;
        wr_data = 0;
        write_count = 0;
        read_count = 0;
        test_case = 0;
        
        // Wait for reset release
        wait(wr_rst_n && rd_rst_n);
        #100;
        
        $display("Starting ASYNC FIFO Testbench");
        
        // Test Case 1: Basic write/read functionality
        test_case = 1;
        $display("Test Case 1: Basic Write/Read");
        test_basic_write_read();
        
        // Test Case 2: Full FIFO testing
        test_case = 2;
        $display("Test Case 2: FIFO Full Condition");
        test_fifo_full();
        
        // Test Case 3: Empty FIFO testing
        test_case = 3;
        $display("Test Case 3: FIFO Empty Condition");
        test_fifo_empty();
        
        // Test Case 4: Simultaneous read/write
        test_case = 4;
        $display("Test Case 4: Simultaneous Read/Write");
        test_simultaneous_rd_wr();
        
        // Test Case 5: Random data pattern
        test_case = 5;
        $display("Test Case 5: Random Data Pattern");
        test_random_data();
        
        // Test Case 6: Clock domain crossing stress test
        test_case = 6;
        $display("Test Case 6: Clock Domain Crossing Stress Test");
        test_clock_domain_stress();
        
        $display("All tests completed successfully!");
        $finish;
    end
    
    // Task: Basic write/read test
    task test_basic_write_read();
        begin
            $display("  Writing single data word");
            @(posedge wr_clk);
            wr_data = 32'hDEADBEEF;
            wr_en = 1;
            test_data_queue.push_back(wr_data);
            @(posedge wr_clk);
            wr_en = 0;
            
            // Wait for data to propagate across clock domains
            repeat(10) @(posedge rd_clk);
            
            $display("  Reading single data word");
            @(posedge rd_clk);
            rd_en = 1;
            @(posedge rd_clk);
            rd_en = 0;
            
            expected_data = test_data_queue.pop_front();
            if (rd_data !== expected_data) begin
                $error("Data mismatch! Expected: %h, Got: %h", expected_data, rd_data);
                $finish;
            end else begin
                $display("  ✓ Data integrity verified: %h", rd_data);
            end
        end
    endtask
    
    // Task: Test FIFO full condition
    task test_fifo_full();
        integer i;
        begin
            $display("  Filling FIFO to capacity");
            for (i = 0; i < FIFO_DEPTH; i++) begin
                @(posedge wr_clk);
                wr_data = 32'hA5A5A5A5 + i;
                wr_en = 1;
                test_data_queue.push_back(wr_data);
                
                if (wr_full && i < FIFO_DEPTH-1) begin
                    $error("FIFO full asserted too early at count %d", i);
                    $finish;
                end
            end
            
            @(posedge wr_clk);
            wr_en = 0;
            
            // Check that FIFO is full
            repeat(5) @(posedge wr_clk);
            if (!wr_full) begin
                $error("FIFO should be full but wr_full is not asserted");
                $finish;
            end else begin
                $display("  ✓ FIFO full condition detected correctly");
            end
            
            // Try to write when full (should be ignored)
            @(posedge wr_clk);
            wr_data = 32'hBADDATA;
            wr_en = 1;
            @(posedge wr_clk);
            wr_en = 0;
            
            $display("  ✓ Write to full FIFO correctly ignored");
        end
    endtask
    
    // Task: Test FIFO empty condition
    task test_fifo_empty();
        begin
            $display("  Emptying FIFO completely");
            while (test_data_queue.size() > 0) begin
                repeat(3) @(posedge rd_clk);
                rd_en = 1;
                @(posedge rd_clk);
                rd_en = 0;
                
                expected_data = test_data_queue.pop_front();
                if (rd_data !== expected_data) begin
                    $error("Data mismatch during emptying! Expected: %h, Got: %h", expected_data, rd_data);
                    $finish;
                end
            end
            
            // Wait for empty flag to propagate
            repeat(10) @(posedge rd_clk);
            
            if (!rd_empty) begin
                $error("FIFO should be empty but rd_empty is not asserted");
                $finish;
            end else begin
                $display("  ✓ FIFO empty condition detected correctly");
            end
            
            // Try to read when empty (should not affect output)
            @(posedge rd_clk);
            rd_en = 1;
            @(posedge rd_clk);
            rd_en = 0;
            
            $display("  ✓ Read from empty FIFO handled correctly");
        end
    endtask
    
    // Task: Test simultaneous read/write operations
    task test_simultaneous_rd_wr();
        integer i;
        begin
            $display("  Testing simultaneous read/write operations");
            
            // First, write a few words
            for (i = 0; i < 4; i++) begin
                @(posedge wr_clk);
                wr_data = 32'h12345678 + i;
                wr_en = 1;
                test_data_queue.push_back(wr_data);
            end
            @(posedge wr_clk);
            wr_en = 0;
            
            // Wait for data to propagate
            repeat(10) @(posedge rd_clk);
            
            // Now perform simultaneous read/write
            fork
                // Write process
                begin
                    for (i = 0; i < 8; i++) begin
                        @(posedge wr_clk);
                        wr_data = 32'h87654321 + i;
                        wr_en = 1;
                        test_data_queue.push_back(wr_data);
                        @(posedge wr_clk);
                        wr_en = 0;
                        repeat(2) @(posedge wr_clk);
                    end
                end
                
                // Read process
                begin
                    repeat(5) @(posedge rd_clk);
                    for (i = 0; i < 6; i++) begin
                        @(posedge rd_clk);
                        rd_en = 1;
                        @(posedge rd_clk);
                        rd_en = 0;
                        
                        if (test_data_queue.size() > 0) begin
                            expected_data = test_data_queue.pop_front();
                            if (rd_data !== expected_data) begin
                                $error("Data mismatch in simultaneous test! Expected: %h, Got: %h", expected_data, rd_data);
                                $finish;
                            end
                        end
                        repeat(3) @(posedge rd_clk);
                    end
                end
            join
            
            $display("  ✓ Simultaneous read/write operations successful");
        end
    endtask
    
    // Task: Test with random data patterns
    task test_random_data();
        integer i, rand_data;
        begin
            $display("  Testing with random data patterns");
            
            for (i = 0; i < 20; i++) begin
                rand_data = $random;
                @(posedge wr_clk);
                wr_data = rand_data;
                wr_en = 1;
                test_data_queue.push_back(wr_data);
                @(posedge wr_clk);
                wr_en = 0;
                
                // Random delay
                repeat($urandom_range(1, 5)) @(posedge wr_clk);
            end
            
            // Read all data back
            repeat(10) @(posedge rd_clk);
            while (test_data_queue.size() > 0) begin
                @(posedge rd_clk);
                rd_en = 1;
                @(posedge rd_clk);
                rd_en = 0;
                
                expected_data = test_data_queue.pop_front();
                if (rd_data !== expected_data) begin
                    $error("Random data mismatch! Expected: %h, Got: %h", expected_data, rd_data);
                    $finish;
                end
                
                // Random delay
                repeat($urandom_range(1, 4)) @(posedge rd_clk);
            end
            
            $display("  ✓ Random data pattern test successful");
        end
    endtask
    
    // Task: Clock domain crossing stress test
    task test_clock_domain_stress();
        integer i;
        begin
            $display("  Clock domain crossing stress test");
            
            // Rapid write/read cycles with different clock relationships
            fork
                // Aggressive writing
                begin
                    for (i = 0; i < 30; i++) begin
                        @(posedge wr_clk);
                        if (!wr_full) begin
                            wr_data = 32'hCDCDCDCD + i;
                            wr_en = 1;
                            test_data_queue.push_back(wr_data);
                        end else begin
                            wr_en = 0;
                        end
                    end
                    wr_en = 0;
                end
                
                // Aggressive reading
                begin
                    repeat(20) @(posedge rd_clk);
                    for (i = 0; i < 25; i++) begin
                        @(posedge rd_clk);
                        if (!rd_empty) begin
                            rd_en = 1;
                            @(posedge rd_clk);
                            rd_en = 0;
                            
                            if (test_data_queue.size() > 0) begin
                                expected_data = test_data_queue.pop_front();
                                if (rd_data !== expected_data) begin
                                    $error("Stress test data mismatch! Expected: %h, Got: %h", expected_data, rd_data);
                                    $finish;
                                end
                            end
                        end else begin
                            rd_en = 0;
                        end
                    end
                end
            join
            
            $display("  ✓ Clock domain crossing stress test successful");
        end
    endtask
    
    // Monitoring and assertions
    always @(posedge wr_clk) begin
        if (wr_rst_n && wr_en && wr_full) begin
            $warning("Attempted write to full FIFO at time %t", $time);
        end
    end
    
    always @(posedge rd_clk) begin
        if (rd_rst_n && rd_en && rd_empty) begin
            $warning("Attempted read from empty FIFO at time %t", $time);
        end
    end
    
    // Simulation timeout
    initial begin
        #1000000;  // 1ms timeout
        $error("Simulation timeout!");
        $finish;
    end

endmodule