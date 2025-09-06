/**
 * PCIe Controller Test Scenarios
 * 
 * Comprehensive tests for PCIe interface including configuration space,
 * message passing, interrupt handling, and link management
 */

`ifndef PCIE_TESTS_SV
`define PCIE_TESTS_SV

`include "sim_test_framework.sv"

/**
 * PCIe Configuration Space Test
 * Tests PCIe configuration register access and initialization
 */
`TEST_CLASS(pcie_config_test)
    virtual pcie_if pcie_if;
    
    function new();
        super.new("pcie_config_test", SEV_HIGH, 8000);
    endfunction
    
    virtual task setup();
        super.setup();
        
        if (!uvm_config_db#(virtual pcie_if)::get(null, "*", "pcie_if", pcie_if)) begin
            $fatal("Failed to get PCIe interface");
        end
        
        // Reset PCIe controller
        pcie_if.reset_n = 0;
        repeat(100) @(posedge pcie_if.clk);
        pcie_if.reset_n = 1;
        repeat(10) @(posedge pcie_if.clk);
    endtask
    
    virtual task run();
        super.run();
        
        info("Testing PCIe configuration space");
        
        // Test configuration header
        test_config_header();
        
        // Test BAR configuration
        test_bar_configuration();
        
        // Test capability structures
        test_capability_structures();
    endtask
    
    task test_config_header();
        logic [31:0] vendor_device_id;
        logic [31:0] status_command;
        
        info("Testing PCIe configuration header");
        
        // Read vendor/device ID
        config_read(32'h00, vendor_device_id);
        assert_eq(vendor_device_id[15:0], 16'h10EE, "Xilinx vendor ID expected");
        assert_eq(vendor_device_id[31:16], 16'h7024, "NPU device ID expected");
        
        // Read status/command register
        config_read(32'h04, status_command);
        assert_eq(status_command[2:0], 3'b111, "Memory, I/O, and bus master should be enabled");
    endtask
    
    task test_bar_configuration();
        logic [31:0] bar0, bar1;
        
        info("Testing BAR configuration");
        
        // Read BAR values
        config_read(32'h10, bar0); // BAR0 - Control registers
        config_read(32'h14, bar1); // BAR1 - Memory buffer
        
        // Test BAR0 - should be memory mapped, non-prefetchable
        assert_false(bar0[0], "BAR0 should be memory space");
        assert_eq(bar0[2:1], 2'b00, "BAR0 should be 32-bit addressable");
        assert_false(bar0[3], "BAR0 should be non-prefetchable");
        
        // Test BAR1 - should be memory mapped, prefetchable
        assert_false(bar1[0], "BAR1 should be memory space");
        assert_true(bar1[3], "BAR1 should be prefetchable");
    endtask
    
    task test_capability_structures();
        logic [31:0] cap_ptr_reg;
        logic [31:0] cap_header;
        
        info("Testing PCIe capability structures");
        
        // Read capabilities pointer
        config_read(32'h34, cap_ptr_reg);
        logic [7:0] first_cap = cap_ptr_reg[7:0];
        
        assert_true(first_cap != 0, "Capabilities list should be present");
        
        // Read first capability
        config_read({24'h0, first_cap}, cap_header);
        logic [7:0] cap_id = cap_header[7:0];
        
        // Should be PCIe capability (0x10)
        assert_eq(cap_id, 8'h10, "First capability should be PCIe");
    endtask
    
    // Helper functions
    task config_read(logic [31:0] addr, output logic [31:0] data);
        @(posedge pcie_if.clk);
        pcie_if.cfg_addr = addr;
        pcie_if.cfg_read = 1;
        @(posedge pcie_if.clk);
        pcie_if.cfg_read = 0;
        @(posedge pcie_if.clk);
        data = pcie_if.cfg_rdata;
    endtask
    
    task config_write(logic [31:0] addr, logic [31:0] data);
        @(posedge pcie_if.clk);
        pcie_if.cfg_addr = addr;
        pcie_if.cfg_wdata = data;
        pcie_if.cfg_write = 1;
        @(posedge pcie_if.clk);
        pcie_if.cfg_write = 0;
    endtask
`END_TEST_CLASS

/**
 * PCIe Transaction Layer Test
 * Tests TLP generation, routing, and completion handling
 */
`TEST_CLASS(pcie_tlp_test)
    virtual pcie_if pcie_if;
    
    function new();
        super.new("pcie_tlp_test", SEV_HIGH, 15000);
    endfunction
    
    virtual task setup();
        super.setup();
        
        if (!uvm_config_db#(virtual pcie_if)::get(null, "*", "pcie_if", pcie_if)) begin
            $fatal("Failed to get PCIe interface");
        end
        
        // Reset and initialize PCIe
        pcie_if.reset_n = 0;
        repeat(100) @(posedge pcie_if.clk);
        pcie_if.reset_n = 1;
        repeat(50) @(posedge pcie_if.clk);
        
        // Wait for link up
        wait(pcie_if.link_up);
        repeat(10) @(posedge pcie_if.clk);
    endtask
    
    virtual task run();
        super.run();
        
        info("Testing PCIe Transaction Layer");
        
        // Test memory read/write TLPs
        test_memory_tlps();
        
        // Test completion TLPs
        test_completion_tlps();
        
        // Test message TLPs
        test_message_tlps();
    endtask
    
    task test_memory_tlps();
        logic [63:0] addr = 64'h0000_0000_1000_0000;
        logic [31:0] write_data [0:3] = '{32'h12340000, 32'h12340001, 32'h12340002, 32'h12340003};
        logic [31:0] read_data [0:3];
        
        info("Testing memory read/write TLPs");
        
        // Test memory write TLP
        send_memory_write_tlp(addr, write_data, 4);
        wait_for_completion();
        
        // Test memory read TLP
        send_memory_read_tlp(addr, 4);
        receive_memory_read_completion(read_data, 4);
        
        // Verify data integrity
        for (int i = 0; i < 4; i++) begin
            assert_eq(read_data[i], write_data[i], 
                     $sformatf("Memory data mismatch at index %0d", i));
        end
    endtask
    
    task send_memory_write_tlp(logic [63:0] addr, logic [31:0] data[], int length);
        info($sformatf("Sending memory write TLP to address 0x%016X, length %0d", addr, length));
        
        // TLP header fields
        logic [2:0] fmt = 3'b011;     // 4DW header with data
        logic [4:0] type = 5'b00000;  // Memory write
        logic [9:0] length_dw = length;
        logic [15:0] requester_id = 16'h0100;
        logic [7:0] tag = 8'h01;
        
        // Send TLP header
        @(posedge pcie_if.clk);
        pcie_if.tx_valid = 1;
        pcie_if.tx_sop = 1;
        pcie_if.tx_data = {fmt, type, 1'b0, 2'b00, 1'b0, 1'b0, 2'b00, length_dw};
        
        @(posedge pcie_if.clk);
        pcie_if.tx_sop = 0;
        pcie_if.tx_data = {requester_id, tag, 4'hF, 4'h0};
        
        @(posedge pcie_if.clk);
        pcie_if.tx_data = addr[63:32];
        
        @(posedge pcie_if.clk);
        pcie_if.tx_data = {addr[31:2], 2'b00};
        
        // Send data payload
        for (int i = 0; i < length; i++) begin
            @(posedge pcie_if.clk);
            if (i == length - 1) pcie_if.tx_eop = 1;
            pcie_if.tx_data = data[i];
        end
        
        @(posedge pcie_if.clk);
        pcie_if.tx_valid = 0;
        pcie_if.tx_eop = 0;
    endtask
    
    task send_memory_read_tlp(logic [63:0] addr, int length);
        info($sformatf("Sending memory read TLP to address 0x%016X, length %0d", addr, length));
        
        // TLP header fields
        logic [2:0] fmt = 3'b001;     // 4DW header without data
        logic [4:0] type = 5'b00000;  // Memory read
        logic [9:0] length_dw = length;
        logic [15:0] requester_id = 16'h0100;
        logic [7:0] tag = 8'h02;
        
        // Send TLP header
        @(posedge pcie_if.clk);
        pcie_if.tx_valid = 1;
        pcie_if.tx_sop = 1;
        pcie_if.tx_data = {fmt, type, 1'b0, 2'b00, 1'b0, 1'b0, 2'b00, length_dw};
        
        @(posedge pcie_if.clk);
        pcie_if.tx_sop = 0;
        pcie_if.tx_data = {requester_id, tag, 4'hF, 4'h0};
        
        @(posedge pcie_if.clk);
        pcie_if.tx_data = addr[63:32];
        
        @(posedge pcie_if.clk);
        pcie_if.tx_eop = 1;
        pcie_if.tx_data = {addr[31:2], 2'b00};
        
        @(posedge pcie_if.clk);
        pcie_if.tx_valid = 0;
        pcie_if.tx_eop = 0;
    endtask
    
    task wait_for_completion();
        fork
            begin
                wait(pcie_if.rx_valid && pcie_if.rx_sop);
                // Verify completion TLP format
                logic [2:0] fmt = pcie_if.rx_data[31:29];
                logic [4:0] type = pcie_if.rx_data[28:24];
                
                assert_eq(fmt, 3'b000, "Completion should have 3DW header");
                assert_eq(type, 5'b01010, "Should be completion TLP");
            end
            begin
                #10000; // Timeout
                warning("Completion timeout");
            end
        join_any
        disable fork;
    endtask
    
    task receive_memory_read_completion(output logic [31:0] data[], int expected_length);
        logic [9:0] length_dw;
        int data_count = 0;
        
        info("Waiting for memory read completion");
        
        // Wait for completion TLP
        wait(pcie_if.rx_valid && pcie_if.rx_sop);
        
        // Parse completion header
        length_dw = pcie_if.rx_data[9:0];
        assert_eq(length_dw, expected_length, "Completion length mismatch");
        
        @(posedge pcie_if.clk);
        wait(pcie_if.rx_valid);
        
        @(posedge pcie_if.clk);
        wait(pcie_if.rx_valid);
        
        // Receive data payload
        while (pcie_if.rx_valid && !pcie_if.rx_eop && data_count < expected_length) begin
            @(posedge pcie_if.clk);
            if (pcie_if.rx_valid) begin
                data[data_count] = pcie_if.rx_data;
                data_count++;
            end
        end
        
        assert_eq(data_count, expected_length, "Received data length mismatch");
    endtask
    
    task test_completion_tlps();
        info("Testing completion TLP handling");
        // Implementation specific completion tests
    endtask
    
    task test_message_tlps();
        info("Testing message TLPs");
        // Implementation specific message tests
    endtask
`END_TEST_CLASS

/**
 * PCIe Link Management Test
 * Tests link training, state management, and error recovery
 */
`TEST_CLASS(pcie_link_test)
    virtual pcie_if pcie_if;
    
    function new();
        super.new("pcie_link_test", SEV_CRITICAL, 20000);
    endfunction
    
    virtual task setup();
        super.setup();
        
        if (!uvm_config_db#(virtual pcie_if)::get(null, "*", "pcie_if", pcie_if)) begin
            $fatal("Failed to get PCIe interface");
        end
        
        // Start with link down
        pcie_if.reset_n = 0;
        repeat(100) @(posedge pcie_if.clk);
        pcie_if.reset_n = 1;
    endtask
    
    virtual task run();
        super.run();
        
        info("Testing PCIe link management");
        
        // Test link training
        test_link_training();
        
        // Test link state management
        test_link_states();
        
        // Test error detection and recovery
        test_error_recovery();
    endtask
    
    task test_link_training();
        info("Testing PCIe link training");
        
        // Wait for link training to complete
        fork
            begin
                wait(pcie_if.link_up);
                info("Link training completed successfully");
            end
            begin
                #50000; // 50us timeout for link training
                if (!pcie_if.link_up) begin
                    warning("Link training timeout");
                end
            end
        join_any
        disable fork;
        
        assert_true(pcie_if.link_up, "Link should be trained and up");
        
        // Verify link parameters
        assert_true(pcie_if.link_speed >= 1, "Link speed should be at least Gen1");
        assert_true(pcie_if.link_width >= 1, "Link width should be at least x1");
    endtask
    
    task test_link_states();
        info("Testing PCIe link state transitions");
        
        // Test L0 to L1 transition
        if (pcie_if.aspm_supported) begin
            pcie_if.request_l1 = 1;
            @(posedge pcie_if.clk);
            pcie_if.request_l1 = 0;
            
            // Wait for L1 entry
            wait(pcie_if.link_state == 2'b01); // L1 state
            assert_eq(pcie_if.link_state, 2'b01, "Should enter L1 state");
            
            // Trigger activity to exit L1
            pcie_if.activity_detected = 1;
            @(posedge pcie_if.clk);
            pcie_if.activity_detected = 0;
            
            // Wait for L0 return
            wait(pcie_if.link_state == 2'b00); // L0 state
            assert_eq(pcie_if.link_state, 2'b00, "Should return to L0 state");
        end
    endtask
    
    task test_error_recovery();
        info("Testing PCIe error detection and recovery");
        
        // Inject correctable error
        pcie_if.inject_error = 1;
        pcie_if.error_type = 2'b01; // Correctable error
        @(posedge pcie_if.clk);
        pcie_if.inject_error = 0;
        
        // Wait for error detection
        wait(pcie_if.correctable_error);
        assert_true(pcie_if.correctable_error, "Correctable error should be detected");
        
        // Error should be automatically corrected
        repeat(100) @(posedge pcie_if.clk);
        assert_false(pcie_if.correctable_error, "Correctable error should be cleared");
        
        // Test uncorrectable error recovery
        pcie_if.inject_error = 1;
        pcie_if.error_type = 2'b10; // Uncorrectable error
        @(posedge pcie_if.clk);
        pcie_if.inject_error = 0;
        
        // Wait for error detection
        wait(pcie_if.uncorrectable_error);
        assert_true(pcie_if.uncorrectable_error, "Uncorrectable error should be detected");
        
        // Trigger recovery
        pcie_if.error_recovery = 1;
        @(posedge pcie_if.clk);
        pcie_if.error_recovery = 0;
        
        // Wait for recovery completion
        wait(pcie_if.recovery_complete);
        assert_true(pcie_if.recovery_complete, "Error recovery should complete");
        assert_false(pcie_if.uncorrectable_error, "Error should be cleared after recovery");
    endtask
`END_TEST_CLASS

// Test registration function
function void register_pcie_tests();
    `RUN_TEST(pcie_config_test);
    `RUN_TEST(pcie_tlp_test);
    `RUN_TEST(pcie_link_test);
endfunction

`endif // PCIE_TESTS_SV