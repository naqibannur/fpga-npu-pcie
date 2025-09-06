/**
 * Comprehensive Simulation Test Framework
 * 
 * Advanced SystemVerilog test framework for FPGA NPU validation
 * Provides test management, coverage collection, and result reporting
 */

`ifndef SIM_TEST_FRAMEWORK_SV
`define SIM_TEST_FRAMEWORK_SV

// Test result codes
typedef enum {
    TEST_PASS,
    TEST_FAIL,
    TEST_TIMEOUT,
    TEST_ERROR
} test_result_t;

// Test severity levels
typedef enum {
    SEV_LOW,
    SEV_MEDIUM,
    SEV_HIGH,
    SEV_CRITICAL
} test_severity_t;

// Coverage collection modes
typedef enum {
    COV_NONE,
    COV_FUNCTIONAL,
    COV_CODE,
    COV_BOTH
} coverage_mode_t;

/**
 * Base Test Class
 * Provides common test infrastructure and utilities
 */
class base_test;
    string test_name;
    test_severity_t severity;
    int timeout_cycles;
    test_result_t result;
    string error_msg;
    coverage_mode_t cov_mode;
    
    // Test statistics
    int assertions_passed;
    int assertions_failed;
    int warnings_count;
    
    // Timing
    time start_time;
    time end_time;
    
    function new(string name = "base_test", 
                 test_severity_t sev = SEV_MEDIUM,
                 int timeout = 10000);
        this.test_name = name;
        this.severity = sev;
        this.timeout_cycles = timeout;
        this.result = TEST_PASS;
        this.error_msg = "";
        this.cov_mode = COV_FUNCTIONAL;
        this.assertions_passed = 0;
        this.assertions_failed = 0;
        this.warnings_count = 0;
    endfunction
    
    // Test lifecycle methods
    virtual task setup();
        $display("[%0t] Setting up test: %s", $time, test_name);
        start_time = $time;
    endtask
    
    virtual task run();
        $display("[%0t] Running test: %s", $time, test_name);
        // Override in derived classes
    endtask
    
    virtual task cleanup();
        $display("[%0t] Cleaning up test: %s", $time, test_name);
        end_time = $time;
    endtask
    
    // Assertion methods
    function void assert_eq(logic [31:0] actual, logic [31:0] expected, string msg = "");
        if (actual !== expected) begin
            assertions_failed++;
            result = TEST_FAIL;
            error_msg = $sformatf("ASSERTION FAILED: %s - Expected: 0x%h, Actual: 0x%h", 
                                msg, expected, actual);
            $error("[%0t] %s", $time, error_msg);
        end else begin
            assertions_passed++;
            $display("[%0t] ASSERTION PASSED: %s", $time, msg);
        end
    endfunction
    
    function void assert_true(logic condition, string msg = "");
        if (!condition) begin
            assertions_failed++;
            result = TEST_FAIL;
            error_msg = $sformatf("ASSERTION FAILED: %s - Condition is false", msg);
            $error("[%0t] %s", $time, error_msg);
        end else begin
            assertions_passed++;
            $display("[%0t] ASSERTION PASSED: %s", $time, msg);
        end
    endfunction
    
    function void assert_false(logic condition, string msg = "");
        if (condition) begin
            assertions_failed++;
            result = TEST_FAIL;
            error_msg = $sformatf("ASSERTION FAILED: %s - Condition is true", msg);
            $error("[%0t] %s", $time, error_msg);
        end else begin
            assertions_passed++;
            $display("[%0t] ASSERTION PASSED: %s", $time, msg);
        end
    endfunction
    
    function void warning(string msg);
        warnings_count++;
        $warning("[%0t] WARNING in %s: %s", $time, test_name, msg);
    endfunction
    
    function void info(string msg);
        $display("[%0t] INFO in %s: %s", $time, test_name, msg);
    endfunction
    
    // Test execution wrapper
    task execute();
        fork
            begin
                setup();
                run();
                cleanup();
            end
            begin
                #(timeout_cycles);
                if (result == TEST_PASS) begin
                    result = TEST_TIMEOUT;
                    error_msg = "Test timed out";
                    $error("[%0t] TEST TIMEOUT: %s", $time, test_name);
                end
            end
        join_any
        disable fork;
    endtask
    
    // Result reporting
    function void report();
        real duration_ns = (end_time - start_time);
        string status_str = (result == TEST_PASS) ? "PASS" : 
                           (result == TEST_FAIL) ? "FAIL" :
                           (result == TEST_TIMEOUT) ? "TIMEOUT" : "ERROR";
        
        $display("\n=== TEST REPORT: %s ===", test_name);
        $display("Status: %s", status_str);
        $display("Severity: %s", severity.name());
        $display("Duration: %.2f ns", duration_ns);
        $display("Assertions Passed: %0d", assertions_passed);
        $display("Assertions Failed: %0d", assertions_failed);
        $display("Warnings: %0d", warnings_count);
        if (result != TEST_PASS)
            $display("Error: %s", error_msg);
        $display("==========================\n");
    endfunction
endclass

/**
 * Test Manager Class
 * Manages test execution, results collection, and reporting
 */
class test_manager;
    base_test test_queue[$];
    int tests_run;
    int tests_passed;
    int tests_failed;
    int tests_timeout;
    int tests_error;
    
    function new();
        tests_run = 0;
        tests_passed = 0;
        tests_failed = 0;
        tests_timeout = 0;
        tests_error = 0;
    endfunction
    
    function void add_test(base_test test);
        test_queue.push_back(test);
    endfunction
    
    task run_all_tests();
        $display("\n========================================");
        $display("    FPGA NPU Simulation Test Suite");
        $display("========================================");
        $display("Starting %0d tests...\n", test_queue.size());
        
        foreach (test_queue[i]) begin
            base_test test = test_queue[i];
            $display("\n--- Running Test %0d/%0d: %s ---", 
                    i+1, test_queue.size(), test.test_name);
            
            test.execute();
            test.report();
            
            tests_run++;
            case (test.result)
                TEST_PASS: tests_passed++;
                TEST_FAIL: tests_failed++;
                TEST_TIMEOUT: tests_timeout++;
                TEST_ERROR: tests_error++;
            endcase
        end
        
        generate_final_report();
    endtask
    
    function void generate_final_report();
        real pass_rate = (tests_run > 0) ? (real'(tests_passed) / real'(tests_run)) * 100.0 : 0.0;
        
        $display("\n========================================");
        $display("        FINAL TEST REPORT");
        $display("========================================");
        $display("Total Tests: %0d", tests_run);
        $display("Passed: %0d", tests_passed);
        $display("Failed: %0d", tests_failed);
        $display("Timeout: %0d", tests_timeout);
        $display("Error: %0d", tests_error);
        $display("Pass Rate: %.1f%%", pass_rate);
        $display("========================================");
        
        if (tests_failed > 0 || tests_timeout > 0 || tests_error > 0) begin
            $error("SIMULATION FAILED: %0d test(s) did not pass", 
                   tests_failed + tests_timeout + tests_error);
            $finish(1);
        end else begin
            $display("SIMULATION PASSED: All tests completed successfully");
            $finish(0);
        end
    endfunction
endclass

/**
 * Random Data Generator Utility
 */
class random_data_gen;
    randc bit [31:0] data;
    randc bit [15:0] addr;
    randc bit [7:0] size;
    
    constraint valid_addr {
        addr inside {[0:1023]};
    }
    
    constraint valid_size {
        size inside {[1:64]};
    }
    
    function bit [31:0] get_random_data();
        if (!this.randomize()) begin
            $error("Failed to randomize data");
            return 32'h0;
        end
        return data;
    endfunction
    
    function bit [15:0] get_random_addr();
        if (!this.randomize()) begin
            $error("Failed to randomize addr");
            return 16'h0;
        end
        return addr;
    endfunction
    
    function bit [7:0] get_random_size();
        if (!this.randomize()) begin
            $error("Failed to randomize size");
            return 8'h1;
        end
        return size;
    endfunction
endclass

/**
 * Performance Monitor
 * Tracks performance metrics during simulation
 */
class performance_monitor;
    int cycle_count;
    int transaction_count;
    int error_count;
    real start_time;
    real end_time;
    
    function new();
        cycle_count = 0;
        transaction_count = 0;
        error_count = 0;
        start_time = 0.0;
        end_time = 0.0;
    endfunction
    
    function void start_monitoring();
        start_time = $realtime;
        cycle_count = 0;
        transaction_count = 0;
        error_count = 0;
    endfunction
    
    function void stop_monitoring();
        end_time = $realtime;
    endfunction
    
    function void record_cycle();
        cycle_count++;
    endfunction
    
    function void record_transaction();
        transaction_count++;
    endfunction
    
    function void record_error();
        error_count++;
    endfunction
    
    function void report_performance();
        real duration = end_time - start_time;
        real throughput = (duration > 0) ? (real'(transaction_count) / duration) * 1e9 : 0.0;
        
        $display("\n=== PERFORMANCE REPORT ===");
        $display("Duration: %.2f ns", duration);
        $display("Cycles: %0d", cycle_count);
        $display("Transactions: %0d", transaction_count);
        $display("Errors: %0d", error_count);
        $display("Throughput: %.2f trans/sec", throughput);
        $display("===========================\n");
    endfunction
endclass

/**
 * Coverage Collector
 * Collects functional and code coverage data
 */
class coverage_collector;
    // Functional coverage groups
    covergroup npu_operations_cg;
        option.per_instance = 1;
        option.name = "npu_operations";
        
        operation: coverpoint operation_type {
            bins add_op = {0};
            bins sub_op = {1};
            bins mul_op = {2};
            bins mac_op = {3};
            bins conv_op = {4};
            bins matmul_op = {5};
        }
        
        data_size: coverpoint data_size_val {
            bins small = {[1:16]};
            bins medium = {[17:64]};
            bins large = {[65:256]};
            bins xlarge = {[257:1024]};
        }
        
        operation_x_size: cross operation, data_size;
    endgroup
    
    covergroup memory_access_cg;
        option.per_instance = 1;
        option.name = "memory_access";
        
        access_type: coverpoint mem_access_type {
            bins read = {0};
            bins write = {1};
        }
        
        address_range: coverpoint mem_address {
            bins low_addr = {[0:1023]};
            bins mid_addr = {[1024:2047]};
            bins high_addr = {[2048:4095]};
        }
        
        burst_length: coverpoint mem_burst_len {
            bins single = {1};
            bins burst_2 = {2};
            bins burst_4 = {4};
            bins burst_8 = {8};
            bins burst_16 = {16};
        }
        
        access_x_addr: cross access_type, address_range;
        access_x_burst: cross access_type, burst_length;
    endgroup
    
    // Coverage variables
    int operation_type;
    int data_size_val;
    int mem_access_type;
    int mem_address;
    int mem_burst_len;
    
    function new();
        npu_operations_cg = new();
        memory_access_cg = new();
    endfunction
    
    function void sample_operation(int op_type, int size);
        operation_type = op_type;
        data_size_val = size;
        npu_operations_cg.sample();
    endfunction
    
    function void sample_memory_access(int access_type, int address, int burst_len);
        mem_access_type = access_type;
        mem_address = address;
        mem_burst_len = burst_len;
        memory_access_cg.sample();
    endfunction
    
    function void report_coverage();
        real op_cov = npu_operations_cg.get_coverage();
        real mem_cov = memory_access_cg.get_coverage();
        real overall_cov = (op_cov + mem_cov) / 2.0;
        
        $display("\n=== COVERAGE REPORT ===");
        $display("NPU Operations Coverage: %.1f%%", op_cov);
        $display("Memory Access Coverage: %.1f%%", mem_cov);
        $display("Overall Coverage: %.1f%%", overall_cov);
        $display("========================\n");
    endfunction
endclass

// Global test utilities
test_manager test_mgr;
random_data_gen rand_gen;
performance_monitor perf_mon;
coverage_collector cov_col;

// Initialization task
task initialize_test_framework();
    test_mgr = new();
    rand_gen = new();
    perf_mon = new();
    cov_col = new();
    $display("Test framework initialized");
endtask

// Macros for test definition
`define TEST_CLASS(name) \
    class name extends base_test; \
        function new(); \
            super.new(`"name`"); \
        endfunction

`define END_TEST_CLASS \
    endclass

`define RUN_TEST(test_class) \
    begin \
        test_class test_inst = new(); \
        test_mgr.add_test(test_inst); \
    end

`endif // SIM_TEST_FRAMEWORK_SV