/**
 * NPU Core Comprehensive Test Scenarios
 * 
 * Advanced test cases for NPU core functionality validation
 * Includes basic operations, stress tests, and edge cases
 */

`ifndef NPU_CORE_TESTS_SV
`define NPU_CORE_TESTS_SV

`include "sim_test_framework.sv"

/**
 * NPU Core Basic Operation Test
 * Tests fundamental NPU operations and data flow
 */
`TEST_CLASS(npu_core_basic_test)
    // Test interface handles
    virtual npu_core_if npu_if;
    
    function new();
        super.new("npu_core_basic_test", SEV_HIGH, 5000);
    endfunction
    
    virtual task setup();
        super.setup();
        // Get interface handle from test bench
        if (!uvm_config_db#(virtual npu_core_if)::get(null, "*", "npu_if", npu_if)) begin
            $fatal("Failed to get NPU interface");
        end
        
        // Reset NPU
        npu_if.reset_n = 0;
        repeat(10) @(posedge npu_if.clk);
        npu_if.reset_n = 1;
        repeat(5) @(posedge npu_if.clk);
    endtask
    
    virtual task run();
        super.run();
        
        // Test 1: Basic initialization
        test_initialization();
        
        // Test 2: Simple addition operation
        test_add_operation();
        
        // Test 3: Matrix multiplication
        test_matrix_multiply();
        
        // Test 4: Status register reads
        test_status_operations();
    endtask
    
    task test_initialization();
        info("Testing NPU initialization");
        
        // Check reset state
        assert_eq(npu_if.status, 32'h1, "NPU should be ready after reset");
        assert_false(npu_if.busy, "NPU should not be busy after reset");
        
        // Check default register values
        assert_eq(npu_if.perf_cycles, 32'h0, "Performance cycles should be zero");
        assert_eq(npu_if.perf_ops, 32'h0, "Performance operations should be zero");
    endtask
    
    task test_add_operation();
        logic [31:0] operand_a = 32'h12345678;
        logic [31:0] operand_b = 32'h87654321;
        logic [31:0] expected = operand_a + operand_b;
        
        info("Testing addition operation");
        
        // Setup operation
        @(posedge npu_if.clk);
        npu_if.op_valid = 1;
        npu_if.op_code = 3'b000; // ADD operation
        npu_if.operand_a = operand_a;
        npu_if.operand_b = operand_b;
        
        // Wait for operation to start
        wait(npu_if.busy);
        npu_if.op_valid = 0;
        
        // Wait for completion
        wait(!npu_if.busy);
        
        // Check result
        assert_eq(npu_if.result, expected, "Addition result mismatch");
        assert_true(npu_if.result_valid, "Result should be valid");
        
        // Clear result valid
        @(posedge npu_if.clk);
        npu_if.result_ready = 1;
        @(posedge npu_if.clk);
        npu_if.result_ready = 0;
    endtask
    
    task test_matrix_multiply();
        info("Testing matrix multiplication operation");
        
        // Setup 2x2 matrix multiplication
        // Matrix A: [1, 2; 3, 4]
        // Matrix B: [5, 6; 7, 8]
        // Expected: [19, 22; 43, 50]
        
        logic [31:0] matrix_a [0:3] = '{32'd1, 32'd2, 32'd3, 32'd4};
        logic [31:0] matrix_b [0:3] = '{32'd5, 32'd6, 32'd7, 32'd8};
        logic [31:0] expected [0:3] = '{32'd19, 32'd22, 32'd43, 32'd50};
        
        // Load matrices to memory (simplified for simulation)
        for (int i = 0; i < 4; i++) begin
            @(posedge npu_if.clk);
            npu_if.mem_write = 1;
            npu_if.mem_addr = i;
            npu_if.mem_wdata = matrix_a[i];
            @(posedge npu_if.clk);
            npu_if.mem_write = 0;
            
            @(posedge npu_if.clk);
            npu_if.mem_write = 1;
            npu_if.mem_addr = i + 16;
            npu_if.mem_wdata = matrix_b[i];
            @(posedge npu_if.clk);
            npu_if.mem_write = 0;
        end
        
        // Execute matrix multiplication
        @(posedge npu_if.clk);
        npu_if.op_valid = 1;
        npu_if.op_code = 3'b101; // MATMUL operation
        npu_if.matrix_size = 2; // 2x2 matrix
        npu_if.src_addr_a = 0;
        npu_if.src_addr_b = 16;
        npu_if.dst_addr = 32;
        
        // Wait for operation
        wait(npu_if.busy);
        npu_if.op_valid = 0;
        wait(!npu_if.busy);
        
        // Verify results
        for (int i = 0; i < 4; i++) begin
            @(posedge npu_if.clk);
            npu_if.mem_read = 1;
            npu_if.mem_addr = 32 + i;
            @(posedge npu_if.clk);
            npu_if.mem_read = 0;
            @(posedge npu_if.clk);
            
            assert_eq(npu_if.mem_rdata, expected[i], 
                     $sformatf("Matrix result[%0d] mismatch", i));
        end
    endtask
    
    task test_status_operations();
        info("Testing status and control operations");
        
        // Test performance counter increment
        logic [31:0] initial_cycles = npu_if.perf_cycles;
        
        // Execute a simple operation
        @(posedge npu_if.clk);
        npu_if.op_valid = 1;
        npu_if.op_code = 3'b000; // ADD operation
        npu_if.operand_a = 32'h100;
        npu_if.operand_b = 32'h200;
        
        wait(npu_if.busy);
        npu_if.op_valid = 0;
        wait(!npu_if.busy);
        
        // Check performance counters
        assert_true(npu_if.perf_cycles > initial_cycles, 
                   "Performance cycles should increment");
        assert_eq(npu_if.perf_ops, 32'h1, 
                 "Performance operations should be 1");
        
        // Test performance counter reset
        @(posedge npu_if.clk);
        npu_if.perf_reset = 1;
        @(posedge npu_if.clk);
        npu_if.perf_reset = 0;
        
        assert_eq(npu_if.perf_cycles, 32'h0, 
                 "Performance cycles should reset to 0");
        assert_eq(npu_if.perf_ops, 32'h0, 
                 "Performance operations should reset to 0");
    endtask
`END_TEST_CLASS

/**
 * NPU Core Stress Test
 * High-intensity testing with random operations
 */
`TEST_CLASS(npu_core_stress_test)
    virtual npu_core_if npu_if;
    int num_operations = 1000;
    
    function new();
        super.new("npu_core_stress_test", SEV_MEDIUM, 50000);
    endfunction
    
    virtual task setup();
        super.setup();
        if (!uvm_config_db#(virtual npu_core_if)::get(null, "*", "npu_if", npu_if)) begin
            $fatal("Failed to get NPU interface");
        end
        
        // Reset NPU
        npu_if.reset_n = 0;
        repeat(10) @(posedge npu_if.clk);
        npu_if.reset_n = 1;
        repeat(5) @(posedge npu_if.clk);
    endtask
    
    virtual task run();
        super.run();
        
        info($sformatf("Starting stress test with %0d operations", num_operations));
        
        perf_mon.start_monitoring();
        
        for (int i = 0; i < num_operations; i++) begin
            execute_random_operation(i);
            
            if (i % 100 == 0) begin
                info($sformatf("Completed %0d/%0d operations", i, num_operations));
            end
        end
        
        perf_mon.stop_monitoring();
        perf_mon.report_performance();
        
        info("Stress test completed successfully");
    endtask
    
    task execute_random_operation(int iteration);
        logic [2:0] op_code;
        logic [31:0] operand_a, operand_b;
        
        // Randomize operation
        op_code = $urandom_range(0, 5);
        operand_a = rand_gen.get_random_data();
        operand_b = rand_gen.get_random_data();
        
        // Record for coverage
        cov_col.sample_operation(op_code, $urandom_range(1, 64));
        
        // Execute operation
        @(posedge npu_if.clk);
        npu_if.op_valid = 1;
        npu_if.op_code = op_code;
        npu_if.operand_a = operand_a;
        npu_if.operand_b = operand_b;
        
        // Wait for acceptance
        wait(npu_if.busy);
        npu_if.op_valid = 0;
        
        // Wait for completion
        wait(!npu_if.busy);
        
        perf_mon.record_transaction();
        
        // Verify result is valid
        assert_true(npu_if.result_valid, 
                   $sformatf("Result should be valid for operation %0d", iteration));
        
        // Clear result
        @(posedge npu_if.clk);
        npu_if.result_ready = 1;
        @(posedge npu_if.clk);
        npu_if.result_ready = 0;
    endtask
`END_TEST_CLASS

/**
 * NPU Core Pipeline Test
 * Tests operation pipelining and concurrent execution
 */
`TEST_CLASS(npu_core_pipeline_test)
    virtual npu_core_if npu_if;
    
    function new();
        super.new("npu_core_pipeline_test", SEV_HIGH, 10000);
    endfunction
    
    virtual task setup();
        super.setup();
        if (!uvm_config_db#(virtual npu_core_if)::get(null, "*", "npu_if", npu_if)) begin
            $fatal("Failed to get NPU interface");
        end
        
        // Reset and enable pipeline mode
        npu_if.reset_n = 0;
        repeat(10) @(posedge npu_if.clk);
        npu_if.reset_n = 1;
        npu_if.pipeline_enable = 1;
        repeat(5) @(posedge npu_if.clk);
    endtask
    
    virtual task run();
        super.run();
        
        info("Testing NPU pipeline functionality");
        
        // Test back-to-back operations
        test_back_to_back_operations();
        
        // Test pipeline stall conditions
        test_pipeline_stalls();
        
        // Test pipeline flush
        test_pipeline_flush();
    endtask
    
    task test_back_to_back_operations();
        info("Testing back-to-back operations");
        
        // Issue multiple operations quickly
        for (int i = 0; i < 5; i++) begin
            @(posedge npu_if.clk);
            npu_if.op_valid = 1;
            npu_if.op_code = 3'b000; // ADD
            npu_if.operand_a = 32'h100 + i;
            npu_if.operand_b = 32'h200 + i;
            
            // Don't wait for completion, issue next operation
            if (i < 4) begin
                @(posedge npu_if.clk);
            end
        end
        
        npu_if.op_valid = 0;
        
        // Wait for all operations to complete
        repeat(20) @(posedge npu_if.clk);
        
        // Verify all results
        for (int i = 0; i < 5; i++) begin
            wait(npu_if.result_valid);
            logic [31:0] expected = (32'h100 + i) + (32'h200 + i);
            assert_eq(npu_if.result, expected, 
                     $sformatf("Pipeline result %0d mismatch", i));
            
            @(posedge npu_if.clk);
            npu_if.result_ready = 1;
            @(posedge npu_if.clk);
            npu_if.result_ready = 0;
        end
    endtask
    
    task test_pipeline_stalls();
        info("Testing pipeline stall conditions");
        
        // Create memory contention to force stalls
        fork
            begin
                // Memory intensive operation
                @(posedge npu_if.clk);
                npu_if.op_valid = 1;
                npu_if.op_code = 3'b101; // MATMUL (memory intensive)
                npu_if.matrix_size = 8;
                npu_if.src_addr_a = 0;
                npu_if.src_addr_b = 64;
                npu_if.dst_addr = 128;
            end
            begin
                // Competing memory access
                repeat(5) @(posedge npu_if.clk);
                npu_if.mem_read = 1;
                npu_if.mem_addr = 32;
                repeat(10) @(posedge npu_if.clk);
                npu_if.mem_read = 0;
            end
        join
        
        npu_if.op_valid = 0;
        
        // Wait for operation completion
        wait(!npu_if.busy);
        
        // Verify pipeline handled stalls correctly
        assert_true(npu_if.result_valid, "Result should be valid after stall");
    endtask
    
    task test_pipeline_flush();
        info("Testing pipeline flush");
        
        // Start operation
        @(posedge npu_if.clk);
        npu_if.op_valid = 1;
        npu_if.op_code = 3'b010; // MUL
        npu_if.operand_a = 32'hAAAA;
        npu_if.operand_b = 32'hBBBB;
        
        // Wait for operation to enter pipeline
        wait(npu_if.busy);
        npu_if.op_valid = 0;
        
        // Flush pipeline
        @(posedge npu_if.clk);
        npu_if.pipeline_flush = 1;
        @(posedge npu_if.clk);
        npu_if.pipeline_flush = 0;
        
        // Verify pipeline is empty
        repeat(10) @(posedge npu_if.clk);
        assert_false(npu_if.busy, "NPU should not be busy after flush");
        assert_false(npu_if.result_valid, "No result should be valid after flush");
    endtask
`END_TEST_CLASS

/**
 * NPU Core Error Injection Test
 * Tests error handling and recovery mechanisms
 */
`TEST_CLASS(npu_core_error_test)
    virtual npu_core_if npu_if;
    
    function new();
        super.new("npu_core_error_test", SEV_CRITICAL, 5000);
    endfunction
    
    virtual task setup();
        super.setup();
        if (!uvm_config_db#(virtual npu_core_if)::get(null, "*", "npu_if", npu_if)) begin
            $fatal("Failed to get NPU interface");
        end
        
        // Reset NPU
        npu_if.reset_n = 0;
        repeat(10) @(posedge npu_if.clk);
        npu_if.reset_n = 1;
        repeat(5) @(posedge npu_if.clk);
    endtask
    
    virtual task run();
        super.run();
        
        info("Testing NPU error handling");
        
        // Test invalid operation codes
        test_invalid_operations();
        
        // Test memory access errors
        test_memory_errors();
        
        // Test overflow conditions
        test_overflow_conditions();
        
        // Test recovery mechanisms
        test_error_recovery();
    endtask
    
    task test_invalid_operations();
        info("Testing invalid operation handling");
        
        // Try invalid operation code
        @(posedge npu_if.clk);
        npu_if.op_valid = 1;
        npu_if.op_code = 3'b111; // Invalid operation
        npu_if.operand_a = 32'h123;
        npu_if.operand_b = 32'h456;
        
        repeat(10) @(posedge npu_if.clk);
        npu_if.op_valid = 0;
        
        // Should set error flag
        assert_true(npu_if.error_flag, "Error flag should be set for invalid operation");
        assert_eq(npu_if.error_code, 4'h1, "Invalid operation error code");
        
        // Clear error
        @(posedge npu_if.clk);
        npu_if.error_clear = 1;
        @(posedge npu_if.clk);
        npu_if.error_clear = 0;
        
        assert_false(npu_if.error_flag, "Error flag should clear");
    endtask
    
    task test_memory_errors();
        info("Testing memory error conditions");
        
        // Test out-of-bounds memory access
        @(posedge npu_if.clk);
        npu_if.mem_read = 1;
        npu_if.mem_addr = 32'hFFFFFFFF; // Invalid address
        @(posedge npu_if.clk);
        npu_if.mem_read = 0;
        
        repeat(5) @(posedge npu_if.clk);
        
        // Should generate memory error
        assert_true(npu_if.error_flag, "Memory error should be flagged");
        assert_eq(npu_if.error_code, 4'h2, "Memory error code");
        
        // Clear error
        @(posedge npu_if.clk);
        npu_if.error_clear = 1;
        @(posedge npu_if.clk);
        npu_if.error_clear = 0;
    endtask
    
    task test_overflow_conditions();
        info("Testing arithmetic overflow");
        
        // Test multiplication overflow
        @(posedge npu_if.clk);
        npu_if.op_valid = 1;
        npu_if.op_code = 3'b010; // MUL
        npu_if.operand_a = 32'hFFFFFFFF;
        npu_if.operand_b = 32'hFFFFFFFF;
        
        wait(npu_if.busy);
        npu_if.op_valid = 0;
        wait(!npu_if.busy);
        
        // Check overflow flag
        assert_true(npu_if.overflow_flag, "Overflow flag should be set");
        
        // Clear overflow
        @(posedge npu_if.clk);
        npu_if.error_clear = 1;
        @(posedge npu_if.clk);
        npu_if.error_clear = 0;
    endtask
    
    task test_error_recovery();
        info("Testing error recovery mechanisms");
        
        // Inject error
        @(posedge npu_if.clk);
        npu_if.error_inject = 1;
        npu_if.error_type = 3;
        @(posedge npu_if.clk);
        npu_if.error_inject = 0;
        
        // Verify error state
        assert_true(npu_if.error_flag, "Error should be injected");
        
        // Test soft reset recovery
        @(posedge npu_if.clk);
        npu_if.soft_reset = 1;
        @(posedge npu_if.clk);
        npu_if.soft_reset = 0;
        
        repeat(5) @(posedge npu_if.clk);
        
        // Verify recovery
        assert_false(npu_if.error_flag, "Error should clear after soft reset");
        assert_eq(npu_if.status, 32'h1, "NPU should be ready after recovery");
        
        // Test normal operation after recovery
        @(posedge npu_if.clk);
        npu_if.op_valid = 1;
        npu_if.op_code = 3'b000; // ADD
        npu_if.operand_a = 32'h10;
        npu_if.operand_b = 32'h20;
        
        wait(npu_if.busy);
        npu_if.op_valid = 0;
        wait(!npu_if.busy);
        
        assert_eq(npu_if.result, 32'h30, "Normal operation should work after recovery");
    endtask
`END_TEST_CLASS

// Test registration function
function void register_npu_core_tests();
    `RUN_TEST(npu_core_basic_test);
    `RUN_TEST(npu_core_stress_test);
    `RUN_TEST(npu_core_pipeline_test);
    `RUN_TEST(npu_core_error_test);
endfunction

`endif // NPU_CORE_TESTS_SV