/**
 * Unit Tests for NPU Library Core Functions
 * 
 * Tests basic library initialization, cleanup, and core functionality.
 */

#include "test_framework.h"
#include "../../software/userspace/fpga_npu_lib.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// Mock file descriptor for testing
static int mock_fd = -1;

// Mock implementation of open() for testing
int __wrap_open(const char *pathname, int flags)
{
    if (mock_device.init_should_fail) {
        return -1;
    }
    return mock_device.mock_fd;
}

// Mock implementation of close() for testing
int __wrap_close(int fd)
{
    (void)fd;
    return 0;
}

// Mock implementation of ioctl() for testing
int __wrap_ioctl(int fd, unsigned long request, ...)
{
    (void)fd;
    (void)request;
    
    if (mock_device.ioctl_should_fail) {
        return -1;
    }
    return 0;
}

// Mock implementation of malloc() that can fail
void* __wrap_malloc(size_t size)
{
    if (mock_device.init_should_fail && size > 1000) {
        return NULL;
    }
    return __real_malloc(size);
}

/**
 * Test NPU library initialization
 */
bool test_npu_init(void)
{
    TEST_CASE("npu_init");
    
    // Reset mock state
    mock_reset();
    
    // Test successful initialization
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Cleanup
    int ret = npu_cleanup(handle);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    TEST_PASS();
}

/**
 * Test NPU library initialization failure
 */
bool test_npu_init_fail(void)
{
    TEST_CASE("npu_init failure cases");
    
    // Test device open failure
    mock_set_init_fail(true);
    npu_handle_t handle = npu_init();
    ASSERT_NULL(handle);
    
    mock_reset();
    TEST_PASS();
}

/**
 * Test NPU cleanup function
 */
bool test_npu_cleanup(void)
{
    TEST_CASE("npu_cleanup");
    
    mock_reset();
    
    // Test cleanup with valid handle
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    int ret = npu_cleanup(handle);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test cleanup with NULL handle
    ret = npu_cleanup(NULL);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    TEST_PASS();
}

/**
 * Test tensor creation
 */
bool test_tensor_creation(void)
{
    TEST_CASE("npu_create_tensor");
    
    // Test data
    float test_data[24]; // 2x3x2x2 tensor
    for (int i = 0; i < 24; i++) {
        test_data[i] = (float)i;
    }
    
    // Create tensor
    npu_tensor_t tensor = npu_create_tensor(test_data, 2, 3, 2, 2, NPU_DTYPE_FLOAT32);
    
    // Verify tensor properties
    ASSERT_EQ(tensor.data, test_data);
    ASSERT_EQ(tensor.dims[0], 2);
    ASSERT_EQ(tensor.dims[1], 3);
    ASSERT_EQ(tensor.dims[2], 2);
    ASSERT_EQ(tensor.dims[3], 2);
    ASSERT_EQ(tensor.dtype, NPU_DTYPE_FLOAT32);
    ASSERT_EQ(tensor.size, 24 * sizeof(float));
    
    TEST_PASS();
}

/**
 * Test tensor validation
 */
bool test_tensor_validation(void)
{
    TEST_CASE("npu_validate_tensor");
    
    // Test valid tensor
    float test_data[8];
    npu_tensor_t valid_tensor = npu_create_tensor(test_data, 1, 1, 2, 4, NPU_DTYPE_FLOAT32);
    int ret = npu_validate_tensor(&valid_tensor);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test NULL tensor
    ret = npu_validate_tensor(NULL);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    // Test tensor with NULL data
    npu_tensor_t null_data_tensor = valid_tensor;
    null_data_tensor.data = NULL;
    ret = npu_validate_tensor(&null_data_tensor);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    // Test tensor with zero size
    npu_tensor_t zero_size_tensor = valid_tensor;
    zero_size_tensor.size = 0;
    ret = npu_validate_tensor(&zero_size_tensor);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    // Test tensor with invalid data type
    npu_tensor_t invalid_dtype_tensor = valid_tensor;
    invalid_dtype_tensor.dtype = (npu_dtype_t)999;
    ret = npu_validate_tensor(&invalid_dtype_tensor);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    TEST_PASS();
}

/**
 * Test performance counter functions
 */
bool test_performance_counters(void)
{
    TEST_CASE("performance counters");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Test getting performance counters
    uint64_t cycles, operations;
    int ret = npu_get_performance_counters(handle, &cycles, &operations);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test with NULL parameters
    ret = npu_get_performance_counters(handle, NULL, &operations);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_get_performance_counters(handle, &cycles, NULL);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_get_performance_counters(NULL, &cycles, &operations);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    // Test reset performance counters
    ret = npu_reset_performance_counters(handle);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    ret = npu_reset_performance_counters(NULL);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test error handling functions
 */
bool test_error_handling(void)
{
    TEST_CASE("error handling");
    
    // Test error string function
    const char* error_str = npu_error_string(NPU_SUCCESS);
    ASSERT_STR_EQ("Success", error_str);
    
    error_str = npu_error_string(NPU_ERROR_INVALID);
    ASSERT_STR_EQ("Invalid parameter", error_str);
    
    error_str = npu_error_string(NPU_ERROR_MEMORY);
    ASSERT_STR_EQ("Memory error", error_str);
    
    error_str = npu_error_string(999); // Unknown error
    ASSERT_STR_EQ("Unknown error", error_str);
    
    // Test log level functions
    npu_log_level_t original_level = npu_get_log_level();
    
    npu_set_log_level(NPU_LOG_DEBUG);
    ASSERT_EQ(NPU_LOG_DEBUG, npu_get_log_level());
    
    npu_set_log_level(NPU_LOG_ERROR);
    ASSERT_EQ(NPU_LOG_ERROR, npu_get_log_level());
    
    // Restore original level
    npu_set_log_level(original_level);
    
    TEST_PASS();
}

/**
 * Test throughput and efficiency calculations
 */
bool test_calculations(void)
{
    TEST_CASE("throughput and efficiency calculations");
    
    // Test throughput calculation
    float throughput = npu_calculate_throughput(1000000000ULL, 1000000000ULL); // 1G ops in 1 second
    ASSERT_FLOAT_EQ(1.0f, throughput, 0.001f);
    
    throughput = npu_calculate_throughput(500000000ULL, 1000000000ULL); // 500M ops in 1 second
    ASSERT_FLOAT_EQ(0.5f, throughput, 0.001f);
    
    // Test division by zero
    throughput = npu_calculate_throughput(1000ULL, 0ULL);
    ASSERT_FLOAT_EQ(0.0f, throughput, 0.001f);
    
    // Test efficiency calculation
    float efficiency = npu_calculate_efficiency(100.0f, 50.0f); // 100 GOPS / 50 watts
    ASSERT_FLOAT_EQ(2.0f, efficiency, 0.001f);
    
    efficiency = npu_calculate_efficiency(75.0f, 25.0f);
    ASSERT_FLOAT_EQ(3.0f, efficiency, 0.001f);
    
    // Test division by zero
    efficiency = npu_calculate_efficiency(100.0f, 0.0f);
    ASSERT_FLOAT_EQ(0.0f, efficiency, 0.001f);
    
    TEST_PASS();
}

/**
 * Test device status functions
 */
bool test_device_status(void)
{
    TEST_CASE("device status functions");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Test get status
    uint32_t status;
    int ret = npu_get_status(handle, &status);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test with NULL parameters
    ret = npu_get_status(handle, NULL);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_get_status(NULL, &status);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    // Test wait completion
    ret = npu_wait_completion(handle, 1000);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    ret = npu_wait_completion(NULL, 1000);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test instruction execution
 */
bool test_instruction_execution(void)
{
    TEST_CASE("instruction execution");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Create test instruction
    npu_instruction_t inst = {
        .op = NPU_OP_ADD,
        .src1_addr = 0x1000,
        .src2_addr = 0x2000,
        .dst_addr = 0x3000,
        .size = 1024,
        .params = {0}
    };
    
    // Test single instruction execution
    int ret = npu_execute_instruction(handle, &inst);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test with NULL parameters
    ret = npu_execute_instruction(handle, NULL);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_execute_instruction(NULL, &inst);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    // Test batch execution
    npu_instruction_t batch[3] = {inst, inst, inst};
    ret = npu_execute_batch(handle, batch, 3);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test batch with NULL parameters
    ret = npu_execute_batch(handle, NULL, 3);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_execute_batch(handle, batch, 0);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_execute_batch(NULL, batch, 3);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Run all core function tests
 */
void run_core_tests(void)
{
    TEST_SUITE("Core Functions");
    
    RUN_TEST(test_npu_init);
    RUN_TEST(test_npu_init_fail);
    RUN_TEST(test_npu_cleanup);
    RUN_TEST(test_tensor_creation);
    RUN_TEST(test_tensor_validation);
    RUN_TEST(test_performance_counters);
    RUN_TEST(test_error_handling);
    RUN_TEST(test_calculations);
    RUN_TEST(test_device_status);
    RUN_TEST(test_instruction_execution);
}