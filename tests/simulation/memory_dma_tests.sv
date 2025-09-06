/**
 * Memory Management and DMA Test Scenarios
 * 
 * Comprehensive tests for memory subsystem including DMA transfers,
 * buffer management, and memory coherency validation
 */

`ifndef MEMORY_DMA_TESTS_SV
`define MEMORY_DMA_TESTS_SV

`include "sim_test_framework.sv"

/**
 * DMA Basic Operations Test
 * Tests fundamental DMA transfer functionality
 */
`TEST_CLASS(dma_basic_test)
    virtual dma_if dma_if;
    virtual memory_if mem_if;
    
    function new();
        super.new("dma_basic_test", SEV_HIGH, 10000);
    endfunction
    
    virtual task setup();
        super.setup();
        
        if (!uvm_config_db#(virtual dma_if)::get(null, "*", "dma_if", dma_if)) begin
            $fatal("Failed to get DMA interface");
        end
        
        if (!uvm_config_db#(virtual memory_if)::get(null, "*", "mem_if", mem_if)) begin
            $fatal("Failed to get memory interface");
        end
        
        // Reset DMA controller
        dma_if.reset_n = 0;
        mem_if.reset_n = 0;
        repeat(10) @(posedge dma_if.clk);
        dma_if.reset_n = 1;
        mem_if.reset_n = 1;
        repeat(5) @(posedge dma_if.clk);
    endtask
    
    virtual task run();
        super.run();
        
        info("Testing DMA basic operations");
        
        // Test single transfer
        test_single_transfer();
        
        // Test burst transfers
        test_burst_transfers();
        
        // Test scatter-gather
        test_scatter_gather();
        
        // Test transfer status
        test_transfer_status();
    endtask
    
    task test_single_transfer();
        logic [31:0] src_data = 32'hDEADBEEF;
        logic [31:0] dst_data;
        
        info("Testing single DMA transfer");
        
        // Write test data to source address
        write_memory(32'h1000, src_data);
        
        // Setup DMA transfer
        @(posedge dma_if.clk);
        dma_if.transfer_valid = 1;
        dma_if.src_addr = 32'h1000;
        dma_if.dst_addr = 32'h2000;
        dma_if.transfer_size = 4; // 4 bytes
        dma_if.transfer_type = 2'b00; // Single transfer
        
        // Wait for transfer to start
        wait(dma_if.transfer_busy);
        dma_if.transfer_valid = 0;
        
        // Wait for completion
        wait(!dma_if.transfer_busy);
        wait(dma_if.transfer_done);
        
        // Verify transfer
        dst_data = read_memory(32'h2000);
        assert_eq(dst_data, src_data, "DMA transfer data mismatch");
        assert_false(dma_if.transfer_error, "DMA transfer should not have error");
        
        // Clear done flag
        @(posedge dma_if.clk);
        dma_if.transfer_ack = 1;
        @(posedge dma_if.clk);
        dma_if.transfer_ack = 0;
        
        cov_col.sample_memory_access(1, 32'h1000, 1); // Read
        cov_col.sample_memory_access(0, 32'h2000, 1); // Write
    endtask
    
    task test_burst_transfers();
        logic [31:0] src_data [0:15];
        logic [31:0] dst_data [0:15];
        
        info("Testing DMA burst transfers");
        
        // Generate test data
        for (int i = 0; i < 16; i++) begin
            src_data[i] = 32'h10000000 + i;
            write_memory(32'h3000 + (i*4), src_data[i]);
        end
        
        // Setup burst transfer
        @(posedge dma_if.clk);
        dma_if.transfer_valid = 1;
        dma_if.src_addr = 32'h3000;
        dma_if.dst_addr = 32'h4000;
        dma_if.transfer_size = 64; // 16 words * 4 bytes
        dma_if.transfer_type = 2'b01; // Burst transfer
        dma_if.burst_length = 16;
        
        // Wait for transfer
        wait(dma_if.transfer_busy);
        dma_if.transfer_valid = 0;
        wait(!dma_if.transfer_busy);
        wait(dma_if.transfer_done);
        
        // Verify all data
        for (int i = 0; i < 16; i++) begin
            dst_data[i] = read_memory(32'h4000 + (i*4));
            assert_eq(dst_data[i], src_data[i], 
                     $sformatf("Burst transfer data[%0d] mismatch", i));
        end
        
        assert_false(dma_if.transfer_error, "Burst transfer should not have error");
        
        // Clear done flag
        @(posedge dma_if.clk);
        dma_if.transfer_ack = 1;
        @(posedge dma_if.clk);
        dma_if.transfer_ack = 0;
        
        cov_col.sample_memory_access(1, 32'h3000, 16); // Burst read
        cov_col.sample_memory_access(0, 32'h4000, 16); // Burst write
    endtask
    
    task test_scatter_gather();
        info("Testing scatter-gather DMA");
        
        // Setup scatter-gather descriptor table
        logic [31:0] sg_table [0:7]; // 2 descriptors, 4 words each
        
        // Descriptor 1: src=0x5000, dst=0x6000, size=16
        sg_table[0] = 32'h5000; // src_addr
        sg_table[1] = 32'h6000; // dst_addr
        sg_table[2] = 16;       // size
        sg_table[3] = 32'h1;    // flags (valid)
        
        // Descriptor 2: src=0x5010, dst=0x6010, size=16
        sg_table[4] = 32'h5010; // src_addr
        sg_table[5] = 32'h6010; // dst_addr
        sg_table[6] = 16;       // size
        sg_table[7] = 32'h0;    // flags (last)
        
        // Write descriptor table to memory
        for (int i = 0; i < 8; i++) begin
            write_memory(32'h7000 + (i*4), sg_table[i]);
        end
        
        // Write test data
        for (int i = 0; i < 8; i++) begin
            write_memory(32'h5000 + (i*4), 32'hA0000000 + i);
        end
        
        // Start scatter-gather transfer
        @(posedge dma_if.clk);
        dma_if.transfer_valid = 1;
        dma_if.sg_table_addr = 32'h7000;
        dma_if.transfer_type = 2'b10; // Scatter-gather
        
        wait(dma_if.transfer_busy);
        dma_if.transfer_valid = 0;
        wait(!dma_if.transfer_busy);
        wait(dma_if.transfer_done);
        
        // Verify both transfers completed
        for (int i = 0; i < 4; i++) begin
            logic [31:0] expected = 32'hA0000000 + i;
            logic [31:0] actual1 = read_memory(32'h6000 + (i*4));
            logic [31:0] actual2 = read_memory(32'h6010 + (i*4));
            
            assert_eq(actual1, expected, 
                     $sformatf("SG transfer 1 data[%0d] mismatch", i));
            assert_eq(actual2, expected + 4, 
                     $sformatf("SG transfer 2 data[%0d] mismatch", i));
        end
        
        // Clear done flag
        @(posedge dma_if.clk);
        dma_if.transfer_ack = 1;
        @(posedge dma_if.clk);
        dma_if.transfer_ack = 0;
    endtask
    
    task test_transfer_status();
        info("Testing DMA transfer status monitoring");
        
        // Start a transfer
        @(posedge dma_if.clk);
        dma_if.transfer_valid = 1;
        dma_if.src_addr = 32'h8000;
        dma_if.dst_addr = 32'h9000;
        dma_if.transfer_size = 32;
        dma_if.transfer_type = 2'b01; // Burst
        dma_if.burst_length = 8;
        
        // Monitor transfer progress
        wait(dma_if.transfer_busy);
        dma_if.transfer_valid = 0;
        
        assert_true(dma_if.transfer_busy, "Transfer should be busy");
        assert_true(dma_if.bytes_transferred >= 0, "Bytes transferred should be valid");
        
        // Wait for completion
        wait(!dma_if.transfer_busy);
        
        assert_eq(dma_if.bytes_transferred, 32, "All bytes should be transferred");
        assert_true(dma_if.transfer_done, "Transfer done should be asserted");
        
        // Clear status
        @(posedge dma_if.clk);
        dma_if.transfer_ack = 1;
        @(posedge dma_if.clk);
        dma_if.transfer_ack = 0;
    endtask
    
    // Helper functions
    function void write_memory(logic [31:0] addr, logic [31:0] data);
        @(posedge mem_if.clk);
        mem_if.write_enable = 1;
        mem_if.address = addr;
        mem_if.write_data = data;
        @(posedge mem_if.clk);
        mem_if.write_enable = 0;
    endfunction
    
    function logic [31:0] read_memory(logic [31:0] addr);
        @(posedge mem_if.clk);
        mem_if.read_enable = 1;
        mem_if.address = addr;
        @(posedge mem_if.clk);
        mem_if.read_enable = 0;
        @(posedge mem_if.clk);
        return mem_if.read_data;
    endfunction
`END_TEST_CLASS

/**
 * Memory Coherency Test
 * Tests cache coherency and memory synchronization
 */
`TEST_CLASS(memory_coherency_test)
    virtual memory_if mem_if;
    virtual cache_if cache_if;
    
    function new();
        super.new("memory_coherency_test", SEV_CRITICAL, 15000);
    endfunction
    
    virtual task setup();
        super.setup();
        
        if (!uvm_config_db#(virtual memory_if)::get(null, "*", "mem_if", mem_if)) begin
            $fatal("Failed to get memory interface");
        end
        
        if (!uvm_config_db#(virtual cache_if)::get(null, "*", "cache_if", cache_if)) begin
            $fatal("Failed to get cache interface");
        end
        
        // Reset memory and cache
        mem_if.reset_n = 0;
        cache_if.reset_n = 0;
        repeat(10) @(posedge mem_if.clk);
        mem_if.reset_n = 1;
        cache_if.reset_n = 1;
        repeat(5) @(posedge mem_if.clk);
    endtask
    
    virtual task run();
        super.run();
        
        info("Testing memory coherency");
        
        // Test cache coherency protocols
        test_write_through_coherency();
        test_write_back_coherency();
        test_cache_invalidation();
        test_memory_barriers();
    endtask
    
    task test_write_through_coherency();
        logic [31:0] test_addr = 32'h10000;
        logic [31:0] test_data = 32'h12345678;
        
        info("Testing write-through coherency");
        
        // Enable write-through mode
        cache_if.coherency_mode = 2'b00; // Write-through
        
        // Write to cache
        @(posedge cache_if.clk);
        cache_if.write_enable = 1;
        cache_if.address = test_addr;
        cache_if.write_data = test_data;
        @(posedge cache_if.clk);
        cache_if.write_enable = 0;
        
        // Verify write propagated to memory immediately
        repeat(5) @(posedge mem_if.clk);
        
        logic [31:0] mem_data = read_memory_direct(test_addr);
        assert_eq(mem_data, test_data, "Write-through should update memory immediately");
        
        // Verify cache also has the data
        logic [31:0] cache_data = read_cache_direct(test_addr);
        assert_eq(cache_data, test_data, "Cache should have the written data");
    endtask
    
    task test_write_back_coherency();
        logic [31:0] test_addr = 32'h20000;
        logic [31:0] test_data1 = 32'hAABBCCDD;
        logic [31:0] test_data2 = 32'hEEFF0011;
        
        info("Testing write-back coherency");
        
        // Enable write-back mode
        cache_if.coherency_mode = 2'b01; // Write-back
        
        // Write to cache (should not immediately update memory)
        @(posedge cache_if.clk);
        cache_if.write_enable = 1;
        cache_if.address = test_addr;
        cache_if.write_data = test_data1;
        @(posedge cache_if.clk);
        cache_if.write_enable = 0;
        
        // Memory should not be updated yet
        repeat(5) @(posedge mem_if.clk);
        logic [31:0] mem_data = read_memory_direct(test_addr);
        assert_neq(mem_data, test_data1, "Write-back should not update memory immediately");
        
        // Modify same address again
        @(posedge cache_if.clk);
        cache_if.write_enable = 1;
        cache_if.address = test_addr;
        cache_if.write_data = test_data2;
        @(posedge cache_if.clk);
        cache_if.write_enable = 0;
        
        // Force cache flush
        @(posedge cache_if.clk);
        cache_if.flush_cache = 1;
        @(posedge cache_if.clk);
        cache_if.flush_cache = 0;
        
        // Wait for flush to complete
        wait(cache_if.flush_done);
        
        // Now memory should have the latest data
        mem_data = read_memory_direct(test_addr);
        assert_eq(mem_data, test_data2, "Memory should have latest data after flush");
    endtask
    
    task test_cache_invalidation();
        logic [31:0] test_addr = 32'h30000;
        logic [31:0] mem_data = 32'h11223344;
        logic [31:0] new_data = 32'h55667788;
        
        info("Testing cache invalidation");
        
        // Write directly to memory
        write_memory_direct(test_addr, mem_data);
        
        // Read from cache (should load from memory)
        logic [31:0] cache_data = read_cache(test_addr);
        assert_eq(cache_data, mem_data, "Cache should load data from memory");
        
        // Update memory directly (simulating external update)
        write_memory_direct(test_addr, new_data);
        
        // Cache should still have old data
        cache_data = read_cache_direct(test_addr);
        assert_eq(cache_data, mem_data, "Cache should still have old data");
        
        // Invalidate cache line
        @(posedge cache_if.clk);
        cache_if.invalidate_enable = 1;
        cache_if.invalidate_addr = test_addr;
        @(posedge cache_if.clk);
        cache_if.invalidate_enable = 0;
        
        // Read from cache again (should fetch new data from memory)
        cache_data = read_cache(test_addr);
        assert_eq(cache_data, new_data, "Cache should fetch new data after invalidation");
    endtask
    
    task test_memory_barriers();
        logic [31:0] addr1 = 32'h40000;
        logic [31:0] addr2 = 32'h40004;
        logic [31:0] data1 = 32'h12345678;
        logic [31:0] data2 = 32'h87654321;
        
        info("Testing memory barriers");
        
        // Write to two addresses
        @(posedge cache_if.clk);
        cache_if.write_enable = 1;
        cache_if.address = addr1;
        cache_if.write_data = data1;
        @(posedge cache_if.clk);
        cache_if.write_enable = 0;
        
        @(posedge cache_if.clk);
        cache_if.write_enable = 1;
        cache_if.address = addr2;
        cache_if.write_data = data2;
        @(posedge cache_if.clk);
        cache_if.write_enable = 0;
        
        // Insert memory barrier
        @(posedge cache_if.clk);
        cache_if.memory_barrier = 1;
        @(posedge cache_if.clk);
        cache_if.memory_barrier = 0;
        
        // Wait for barrier completion
        wait(cache_if.barrier_complete);
        
        // Verify both writes completed before barrier
        logic [31:0] mem_data1 = read_memory_direct(addr1);
        logic [31:0] mem_data2 = read_memory_direct(addr2);
        
        assert_eq(mem_data1, data1, "First write should complete before barrier");
        assert_eq(mem_data2, data2, "Second write should complete before barrier");
    endtask
    
    // Helper functions
    function void write_memory_direct(logic [31:0] addr, logic [31:0] data);
        @(posedge mem_if.clk);
        mem_if.direct_write_enable = 1;
        mem_if.address = addr;
        mem_if.write_data = data;
        @(posedge mem_if.clk);
        mem_if.direct_write_enable = 0;
    endfunction
    
    function logic [31:0] read_memory_direct(logic [31:0] addr);
        @(posedge mem_if.clk);
        mem_if.direct_read_enable = 1;
        mem_if.address = addr;
        @(posedge mem_if.clk);
        mem_if.direct_read_enable = 0;
        @(posedge mem_if.clk);
        return mem_if.read_data;
    endfunction
    
    function logic [31:0] read_cache(logic [31:0] addr);
        @(posedge cache_if.clk);
        cache_if.read_enable = 1;
        cache_if.address = addr;
        @(posedge cache_if.clk);
        cache_if.read_enable = 0;
        @(posedge cache_if.clk);
        return cache_if.read_data;
    endfunction
    
    function logic [31:0] read_cache_direct(logic [31:0] addr);
        @(posedge cache_if.clk);
        cache_if.direct_read_enable = 1;
        cache_if.address = addr;
        @(posedge cache_if.clk);
        cache_if.direct_read_enable = 0;
        @(posedge cache_if.clk);
        return cache_if.read_data;
    endfunction
`END_TEST_CLASS

/**
 * Memory Stress Test
 * High-intensity memory operations for reliability testing
 */
`TEST_CLASS(memory_stress_test)
    virtual memory_if mem_if;
    virtual dma_if dma_if;
    int num_iterations = 500;
    
    function new();
        super.new("memory_stress_test", SEV_MEDIUM, 100000);
    endfunction
    
    virtual task setup();
        super.setup();
        
        if (!uvm_config_db#(virtual memory_if)::get(null, "*", "mem_if", mem_if)) begin
            $fatal("Failed to get memory interface");
        end
        
        if (!uvm_config_db#(virtual dma_if)::get(null, "*", "dma_if", dma_if)) begin
            $fatal("Failed to get DMA interface");
        end
        
        // Reset
        mem_if.reset_n = 0;
        dma_if.reset_n = 0;
        repeat(10) @(posedge mem_if.clk);
        mem_if.reset_n = 1;
        dma_if.reset_n = 1;
        repeat(5) @(posedge mem_if.clk);
    endtask
    
    virtual task run();
        super.run();
        
        info($sformatf("Starting memory stress test with %0d iterations", num_iterations));
        
        perf_mon.start_monitoring();
        
        fork
            // Concurrent memory operations
            random_memory_operations();
            random_dma_transfers();
            memory_pattern_tests();
        join
        
        perf_mon.stop_monitoring();
        perf_mon.report_performance();
        
        info("Memory stress test completed");
    endtask
    
    task random_memory_operations();
        for (int i = 0; i < num_iterations; i++) begin
            logic [31:0] addr = rand_gen.get_random_addr() << 2; // Word aligned
            logic [31:0] data = rand_gen.get_random_data();
            
            // Random read or write
            if ($urandom_range(0, 1)) begin
                // Write operation
                @(posedge mem_if.clk);
                mem_if.write_enable = 1;
                mem_if.address = addr;
                mem_if.write_data = data;
                @(posedge mem_if.clk);
                mem_if.write_enable = 0;
                
                cov_col.sample_memory_access(0, addr, 1);
            end else begin
                // Read operation
                @(posedge mem_if.clk);
                mem_if.read_enable = 1;
                mem_if.address = addr;
                @(posedge mem_if.clk);
                mem_if.read_enable = 0;
                @(posedge mem_if.clk);
                
                cov_col.sample_memory_access(1, addr, 1);
            end
            
            perf_mon.record_transaction();
            
            if (i % 100 == 0) begin
                info($sformatf("Memory operations: %0d/%0d", i, num_iterations));
            end
        end
    endtask
    
    task random_dma_transfers();
        for (int i = 0; i < num_iterations/10; i++) begin
            logic [31:0] src_addr = rand_gen.get_random_addr() << 2;
            logic [31:0] dst_addr = rand_gen.get_random_addr() << 2;
            logic [31:0] size = rand_gen.get_random_size() << 2; // Word aligned
            
            // Ensure addresses don't overlap
            if (dst_addr < src_addr + size) begin
                dst_addr = src_addr + size + 64;
            end
            
            // Start DMA transfer
            @(posedge dma_if.clk);
            dma_if.transfer_valid = 1;
            dma_if.src_addr = src_addr;
            dma_if.dst_addr = dst_addr;
            dma_if.transfer_size = size;
            dma_if.transfer_type = $urandom_range(0, 1); // Single or burst
            
            wait(dma_if.transfer_busy);
            dma_if.transfer_valid = 0;
            wait(!dma_if.transfer_busy);
            
            // Clear done flag
            @(posedge dma_if.clk);
            dma_if.transfer_ack = 1;
            @(posedge dma_if.clk);
            dma_if.transfer_ack = 0;
            
            perf_mon.record_transaction();
        end
    endtask
    
    task memory_pattern_tests();
        // Test walking 1s and 0s patterns
        logic [31:0] base_addr = 32'h100000;
        
        for (int pattern = 0; pattern < 32; pattern++) begin
            logic [31:0] test_data = 1 << pattern;
            
            // Write pattern
            @(posedge mem_if.clk);
            mem_if.write_enable = 1;
            mem_if.address = base_addr + (pattern * 4);
            mem_if.write_data = test_data;
            @(posedge mem_if.clk);
            mem_if.write_enable = 0;
            
            // Read back and verify
            @(posedge mem_if.clk);
            mem_if.read_enable = 1;
            mem_if.address = base_addr + (pattern * 4);
            @(posedge mem_if.clk);
            mem_if.read_enable = 0;
            @(posedge mem_if.clk);
            
            assert_eq(mem_if.read_data, test_data, 
                     $sformatf("Pattern test failed for bit %0d", pattern));
        end
    endtask
`END_TEST_CLASS

// Test registration function
function void register_memory_dma_tests();
    `RUN_TEST(dma_basic_test);
    `RUN_TEST(memory_coherency_test);
    `RUN_TEST(memory_stress_test);
endfunction

`endif // MEMORY_DMA_TESTS_SV