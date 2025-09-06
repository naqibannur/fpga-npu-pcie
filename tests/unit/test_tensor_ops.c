/**
 * Unit Tests for NPU Tensor Operations
 * 
 * Tests mathematical operations, activation functions, and tensor manipulations.
 */

#include "test_framework.h"
#include "../../software/userspace/fpga_npu_lib.h"
#include <math.h>

/**
 * Test matrix multiplication operation
 */
bool test_matrix_multiply(void)
{
    TEST_CASE("matrix multiplication");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Create test matrices: 2x3 * 3x2 = 2x2
    float a_data[6] = {1, 2, 3, 4, 5, 6}; // 2x3
    float b_data[6] = {1, 2, 3, 4, 5, 6}; // 3x2
    float c_data[4] = {0};                 // 2x2 result
    
    npu_tensor_t a = npu_create_tensor(a_data, 1, 1, 2, 3, NPU_DTYPE_FLOAT32);
    npu_tensor_t b = npu_create_tensor(b_data, 1, 1, 3, 2, NPU_DTYPE_FLOAT32);
    npu_tensor_t c = npu_create_tensor(c_data, 1, 1, 2, 2, NPU_DTYPE_FLOAT32);
    
    // Test matrix multiplication
    int ret = npu_matrix_multiply(handle, &a, &b, &c);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test with NULL parameters
    ret = npu_matrix_multiply(handle, NULL, &b, &c);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_matrix_multiply(handle, &a, NULL, &c);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_matrix_multiply(handle, &a, &b, NULL);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_matrix_multiply(NULL, &a, &b, &c);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test 2D convolution operation
 */
bool test_conv2d(void)
{
    TEST_CASE("2D convolution");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Create test tensors
    float input_data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}; // 1x1x4x4
    float weight_data[9] = {1, 0, -1, 1, 0, -1, 1, 0, -1}; // 1x1x3x3 kernel
    float output_data[4] = {0}; // 1x1x2x2 output
    
    npu_tensor_t input = npu_create_tensor(input_data, 1, 1, 4, 4, NPU_DTYPE_FLOAT32);
    npu_tensor_t weights = npu_create_tensor(weight_data, 1, 1, 3, 3, NPU_DTYPE_FLOAT32);
    npu_tensor_t output = npu_create_tensor(output_data, 1, 1, 2, 2, NPU_DTYPE_FLOAT32);
    
    // Test convolution with stride=1, padding=0
    int ret = npu_conv2d(handle, &input, &weights, &output, 1, 1, 0, 0);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test with different stride and padding
    ret = npu_conv2d(handle, &input, &weights, &output, 2, 2, 1, 1);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test with NULL parameters
    ret = npu_conv2d(handle, NULL, &weights, &output, 1, 1, 0, 0);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_conv2d(handle, &input, NULL, &output, 1, 1, 0, 0);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_conv2d(handle, &input, &weights, NULL, 1, 1, 0, 0);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_conv2d(NULL, &input, &weights, &output, 1, 1, 0, 0);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test element-wise operations
 */
bool test_elementwise_ops(void)
{
    TEST_CASE("element-wise operations");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Create test tensors
    float a_data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    float b_data[8] = {2, 3, 4, 5, 6, 7, 8, 9};
    float c_data[8] = {0};
    
    npu_tensor_t a = npu_create_tensor(a_data, 1, 1, 2, 4, NPU_DTYPE_FLOAT32);
    npu_tensor_t b = npu_create_tensor(b_data, 1, 1, 2, 4, NPU_DTYPE_FLOAT32);
    npu_tensor_t c = npu_create_tensor(c_data, 1, 1, 2, 4, NPU_DTYPE_FLOAT32);
    
    // Test addition
    int ret = npu_add(handle, &a, &b, &c);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test multiplication
    ret = npu_multiply(handle, &a, &b, &c);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test with NULL parameters
    ret = npu_add(handle, NULL, &b, &c);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_multiply(NULL, &a, &b, &c);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test activation functions
 */
bool test_activation_functions(void)
{
    TEST_CASE("activation functions");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Create test data with both positive and negative values
    float input_data[8] = {-2, -1, -0.5, 0, 0.5, 1, 2, 3};
    float output_data[8] = {0};
    
    npu_tensor_t input = npu_create_tensor(input_data, 1, 1, 2, 4, NPU_DTYPE_FLOAT32);
    npu_tensor_t output = npu_create_tensor(output_data, 1, 1, 2, 4, NPU_DTYPE_FLOAT32);
    
    // Test ReLU
    int ret = npu_relu(handle, &input, &output);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test Leaky ReLU
    ret = npu_leaky_relu(handle, &input, &output, 0.1f);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test Sigmoid
    ret = npu_sigmoid(handle, &input, &output);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test Tanh
    ret = npu_tanh(handle, &input, &output);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test Softmax
    ret = npu_softmax(handle, &input, &output, 1);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test with mismatched tensor sizes
    npu_tensor_t small_output = npu_create_tensor(output_data, 1, 1, 1, 2, NPU_DTYPE_FLOAT32);
    ret = npu_relu(handle, &input, &small_output);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    // Test with NULL parameters
    ret = npu_relu(handle, NULL, &output);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_sigmoid(NULL, &input, &output);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test pooling operations
 */
bool test_pooling_operations(void)
{
    TEST_CASE("pooling operations");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Create 4x4 input tensor
    float input_data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    float output_data[4] = {0}; // 2x2 output for 2x2 pooling
    
    npu_tensor_t input = npu_create_tensor(input_data, 1, 1, 4, 4, NPU_DTYPE_FLOAT32);
    npu_tensor_t output = npu_create_tensor(output_data, 1, 1, 2, 2, NPU_DTYPE_FLOAT32);
    
    // Test max pooling
    int ret = npu_max_pool2d(handle, &input, &output, 2, 2, 2, 2, 0, 0);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test average pooling
    ret = npu_avg_pool2d(handle, &input, &output, 2, 2, 2, 2, 0, 0);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test global average pooling
    float global_output_data[1] = {0};
    npu_tensor_t global_output = npu_create_tensor(global_output_data, 1, 1, 1, 1, NPU_DTYPE_FLOAT32);
    ret = npu_global_avg_pool2d(handle, &input, &global_output);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test with NULL parameters
    ret = npu_max_pool2d(handle, NULL, &output, 2, 2, 2, 2, 0, 0);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_avg_pool2d(NULL, &input, &output, 2, 2, 2, 2, 0, 0);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test normalization operations
 */
bool test_normalization_ops(void)
{
    TEST_CASE("normalization operations");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Create test data
    float input_data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    float scale_data[2] = {1.0, 1.0};
    float bias_data[2] = {0.0, 0.0};
    float mean_data[2] = {2.5, 6.5}; // Channel means
    float var_data[2] = {1.25, 1.25}; // Channel variances
    float output_data[8] = {0};
    
    npu_tensor_t input = npu_create_tensor(input_data, 1, 2, 2, 2, NPU_DTYPE_FLOAT32);
    npu_tensor_t scale = npu_create_tensor(scale_data, 2, 1, 1, 1, NPU_DTYPE_FLOAT32);
    npu_tensor_t bias = npu_create_tensor(bias_data, 2, 1, 1, 1, NPU_DTYPE_FLOAT32);
    npu_tensor_t mean = npu_create_tensor(mean_data, 2, 1, 1, 1, NPU_DTYPE_FLOAT32);
    npu_tensor_t variance = npu_create_tensor(var_data, 2, 1, 1, 1, NPU_DTYPE_FLOAT32);
    npu_tensor_t output = npu_create_tensor(output_data, 1, 2, 2, 2, NPU_DTYPE_FLOAT32);
    
    // Test batch normalization
    int ret = npu_batch_norm(handle, &input, &scale, &bias, &mean, &variance, &output, 1e-5f);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test layer normalization
    ret = npu_layer_norm(handle, &input, &scale, &bias, &output, 1e-5f);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test with NULL parameters
    ret = npu_batch_norm(handle, NULL, &scale, &bias, &mean, &variance, &output, 1e-5f);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_layer_norm(NULL, &input, &scale, &bias, &output, 1e-5f);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test tensor utility operations
 */
bool test_tensor_utilities(void)
{
    TEST_CASE("tensor utility operations");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Create test data
    float input_data[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    float output_data[12] = {0};
    
    npu_tensor_t input = npu_create_tensor(input_data, 1, 1, 3, 4, NPU_DTYPE_FLOAT32);
    npu_tensor_t output = npu_create_tensor(output_data, 1, 1, 3, 4, NPU_DTYPE_FLOAT32);
    
    // Test dropout (should be identity operation)
    int ret = npu_dropout(handle, &input, &output, 0.5f);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test transpose
    int perm[4] = {0, 1, 3, 2}; // Transpose last two dimensions
    npu_tensor_t transposed = npu_create_tensor(output_data, 1, 1, 4, 3, NPU_DTYPE_FLOAT32);
    ret = npu_transpose(handle, &input, &transposed, perm);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test reshape
    uint32_t new_shape[4] = {1, 2, 2, 3};
    npu_tensor_t reshaped = npu_create_tensor(output_data, 1, 2, 2, 3, NPU_DTYPE_FLOAT32);
    ret = npu_reshape(handle, &input, &reshaped, new_shape, 4);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test concatenation
    const npu_tensor_t* inputs[2] = {&input, &input};
    npu_tensor_t concat_output = npu_create_tensor(output_data, 1, 1, 6, 4, NPU_DTYPE_FLOAT32);
    ret = npu_concat(handle, inputs, 2, &concat_output, 2); // Concatenate along height
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test with NULL parameters
    ret = npu_dropout(handle, NULL, &output, 0.5f);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_transpose(NULL, &input, &transposed, perm);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_reshape(handle, &input, &reshaped, NULL, 4);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_concat(handle, NULL, 2, &concat_output, 2);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test softmax implementation (detailed)
 */
bool test_softmax_detailed(void)
{
    TEST_CASE("softmax detailed implementation");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Create simple test case: [1, 2, 3]
    float input_data[3] = {1.0f, 2.0f, 3.0f};
    float output_data[3] = {0};
    
    npu_tensor_t input = npu_create_tensor(input_data, 1, 1, 1, 3, NPU_DTYPE_FLOAT32);
    npu_tensor_t output = npu_create_tensor(output_data, 1, 1, 1, 3, NPU_DTYPE_FLOAT32);
    
    // Apply softmax
    int ret = npu_softmax(handle, &input, &output, 0);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Check that probabilities sum to 1.0 (approximately)
    float sum = output_data[0] + output_data[1] + output_data[2];
    ASSERT_FLOAT_EQ(1.0f, sum, 0.001f);
    
    // Check that all values are positive
    ASSERT_TRUE(output_data[0] > 0.0f);
    ASSERT_TRUE(output_data[1] > 0.0f);
    ASSERT_TRUE(output_data[2] > 0.0f);
    
    // Check ordering (should be monotonically increasing)
    ASSERT_TRUE(output_data[0] < output_data[1]);
    ASSERT_TRUE(output_data[1] < output_data[2]);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Run all tensor operation tests
 */
void run_tensor_tests(void)
{
    TEST_SUITE("Tensor Operations");
    
    RUN_TEST(test_matrix_multiply);
    RUN_TEST(test_conv2d);
    RUN_TEST(test_elementwise_ops);
    RUN_TEST(test_activation_functions);
    RUN_TEST(test_pooling_operations);
    RUN_TEST(test_normalization_ops);
    RUN_TEST(test_tensor_utilities);
    RUN_TEST(test_softmax_detailed);
}