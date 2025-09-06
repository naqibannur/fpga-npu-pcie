/**
 * FPGA NPU User-space Library
 * 
 * C library providing high-level API for FPGA NPU operations.
 * Abstracts low-level driver interface for application developers.
 */

#ifndef FPGA_NPU_LIB_H
#define FPGA_NPU_LIB_H

#include <stdint.h>
#include <stddef.h>
#include "../driver/fpga_npu_enhanced.h"

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
#define NPU_SUCCESS           0
#define NPU_ERROR_INIT       -1
#define NPU_ERROR_DEVICE     -2
#define NPU_ERROR_MEMORY     -3
#define NPU_ERROR_TIMEOUT    -4
#define NPU_ERROR_INVALID    -5

// NPU operation types
typedef enum {
    NPU_OP_ADD = 1,
    NPU_OP_SUB = 2,
    NPU_OP_MUL = 3,
    NPU_OP_MAC = 4,
    NPU_OP_CONV = 5,
    NPU_OP_MATMUL = 6
} npu_operation_t;

// Data types
typedef enum {
    NPU_DTYPE_INT8,
    NPU_DTYPE_INT16,
    NPU_DTYPE_INT32,
    NPU_DTYPE_FLOAT32
} npu_dtype_t;

// Tensor descriptor
typedef struct {
    void *data;
    size_t size;
    uint32_t dims[4];  // NCHW format
    npu_dtype_t dtype;
} npu_tensor_t;

// NPU context handle
typedef struct npu_context* npu_handle_t;

// NPU buffer handle for managed memory
typedef struct npu_buffer* npu_buffer_handle_t;

// Buffer allocation flags
#define NPU_ALLOC_COHERENT    0x01  /* CPU coherent buffer */
#define NPU_ALLOC_STREAMING   0x02  /* Streaming DMA buffer */
#define NPU_ALLOC_READONLY    0x04  /* Read-only buffer */
#define NPU_ALLOC_WRITEONLY   0x08  /* Write-only buffer */

// NPU instruction
typedef struct {
    npu_operation_t op;
    uint32_t src1_addr;
    uint32_t src2_addr;
    uint32_t dst_addr;
    uint32_t size;
    uint32_t params[4];  // Operation-specific parameters
} npu_instruction_t;

/**
 * Initialize NPU library and open device
 * @return NPU handle on success, NULL on failure
 */
npu_handle_t npu_init(void);

/**
 * Cleanup and close NPU device
 * @param handle NPU handle
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_cleanup(npu_handle_t handle);

/**
 * Allocate memory buffer accessible by NPU
 * @param handle NPU handle
 * @param size Size in bytes
 * @return Pointer to allocated memory, NULL on failure
 */
void* npu_alloc(npu_handle_t handle, size_t size);

/**
 * Free NPU-accessible memory
 * @param handle NPU handle
 * @param ptr Pointer to memory to free
 */
void npu_free(npu_handle_t handle, void *ptr);

/**
 * Enhanced Memory Management Functions
 */

/**
 * Allocate managed DMA buffer with enhanced features
 * @param handle NPU handle
 * @param size Size in bytes
 * @param flags Allocation flags (NPU_ALLOC_*)
 * @return Buffer handle on success, NULL on failure
 */
npu_buffer_handle_t npu_buffer_alloc(npu_handle_t handle, size_t size, uint32_t flags);

/**
 * Free managed DMA buffer
 * @param handle NPU handle
 * @param buffer Buffer handle to free
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_buffer_free(npu_handle_t handle, npu_buffer_handle_t buffer);

/**
 * Get buffer information
 * @param handle NPU handle
 * @param buffer Buffer handle
 * @param info Pointer to buffer info structure
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_buffer_get_info(npu_handle_t handle, npu_buffer_handle_t buffer, struct npu_dma_buffer *info);

/**
 * Map buffer to user space
 * @param handle NPU handle
 * @param buffer Buffer handle
 * @return Pointer to mapped memory, NULL on failure
 */
void* npu_buffer_map(npu_handle_t handle, npu_buffer_handle_t buffer);

/**
 * Unmap buffer from user space
 * @param handle NPU handle
 * @param buffer Buffer handle
 * @param ptr Mapped pointer
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_buffer_unmap(npu_handle_t handle, npu_buffer_handle_t buffer, void *ptr);

/**
 * Synchronize buffer contents (flush/invalidate cache)
 * @param handle NPU handle
 * @param buffer Buffer handle
 * @param direction 0: to device, 1: from device
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_buffer_sync(npu_handle_t handle, npu_buffer_handle_t buffer, uint32_t direction);

/**
 * Copy data to buffer
 * @param handle NPU handle
 * @param buffer Buffer handle
 * @param offset Offset within buffer
 * @param src Source data pointer
 * @param size Size to copy
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_buffer_write(npu_handle_t handle, npu_buffer_handle_t buffer, size_t offset, const void *src, size_t size);

/**
 * Copy data from buffer
 * @param handle NPU handle
 * @param buffer Buffer handle
 * @param offset Offset within buffer
 * @param dst Destination data pointer
 * @param size Size to copy
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_buffer_read(npu_handle_t handle, npu_buffer_handle_t buffer, size_t offset, void *dst, size_t size);

/**
 * Create tensor from managed buffer
 * @param buffer Buffer handle
 * @param offset Offset within buffer
 * @param n Batch dimension
 * @param c Channel dimension
 * @param h Height dimension
 * @param w Width dimension
 * @param dtype Data type
 * @return Tensor descriptor
 */
npu_tensor_t npu_tensor_from_buffer(npu_buffer_handle_t buffer, size_t offset, uint32_t n, uint32_t c, uint32_t h, uint32_t w, npu_dtype_t dtype);

/**
 * Get memory usage statistics
 * @param handle NPU handle
 * @param total_allocated Total bytes allocated
 * @param total_free Total bytes free
 * @param buffer_count Number of active buffers
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_get_memory_stats(npu_handle_t handle, size_t *total_allocated, size_t *total_free, uint32_t *buffer_count);

/**
 * Create tensor descriptor
 * @param data Pointer to data
 * @param n Batch dimension
 * @param c Channel dimension
 * @param h Height dimension
 * @param w Width dimension
 * @param dtype Data type
 * @return Tensor descriptor
 */
npu_tensor_t npu_create_tensor(void *data, uint32_t n, uint32_t c, uint32_t h, uint32_t w, npu_dtype_t dtype);

/**
 * Execute single NPU instruction
 * @param handle NPU handle
 * @param inst Instruction to execute
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_execute_instruction(npu_handle_t handle, const npu_instruction_t *inst);

/**
 * Execute batch of NPU instructions
 * @param handle NPU handle
 * @param instructions Array of instructions
 * @param count Number of instructions
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_execute_batch(npu_handle_t handle, const npu_instruction_t *instructions, size_t count);

/**
 * Wait for NPU operation completion
 * @param handle NPU handle
 * @param timeout_ms Timeout in milliseconds (0 = infinite)
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_wait_completion(npu_handle_t handle, uint32_t timeout_ms);

/**
 * Get NPU status
 * @param handle NPU handle
 * @param status Pointer to status variable
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_get_status(npu_handle_t handle, uint32_t *status);

/**
 * Matrix multiplication: C = A * B
 * @param handle NPU handle
 * @param a Input matrix A
 * @param b Input matrix B
 * @param c Output matrix C
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_matrix_multiply(npu_handle_t handle, const npu_tensor_t *a, const npu_tensor_t *b, npu_tensor_t *c);

/**
 * 2D Convolution operation
 * @param handle NPU handle
 * @param input Input tensor (NCHW)
 * @param weights Convolution weights
 * @param output Output tensor
 * @param stride_h Vertical stride
 * @param stride_w Horizontal stride
 * @param pad_h Vertical padding
 * @param pad_w Horizontal padding
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_conv2d(npu_handle_t handle, const npu_tensor_t *input, const npu_tensor_t *weights, 
               npu_tensor_t *output, uint32_t stride_h, uint32_t stride_w, 
               uint32_t pad_h, uint32_t pad_w);

/**
 * Element-wise addition: C = A + B
 * @param handle NPU handle
 * @param a Input tensor A
 * @param b Input tensor B
 * @param c Output tensor C
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_add(npu_handle_t handle, const npu_tensor_t *a, const npu_tensor_t *b, npu_tensor_t *c);

/**
 * Element-wise multiplication: C = A * B
 * @param handle NPU handle
 * @param a Input tensor A
 * @param b Input tensor B
 * @param c Output tensor C
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_multiply(npu_handle_t handle, const npu_tensor_t *a, const npu_tensor_t *b, npu_tensor_t *c);

/**
 * Get NPU performance counters
 * @param handle NPU handle
 * @param cycles Total clock cycles
 * @param operations Total operations executed
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_get_performance_counters(npu_handle_t handle, uint64_t *cycles, uint64_t *operations);

/**
 * Reset NPU performance counters
 * @param handle NPU handle
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_reset_performance_counters(npu_handle_t handle);

/**
 * Performance monitoring and profiling functions
 */

/**
 * Performance profiling structure
 */
typedef struct {
    uint64_t start_time;
    uint64_t end_time;
    uint64_t cycles;
    uint64_t operations;
    uint64_t memory_reads;
    uint64_t memory_writes;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint32_t temperature;
    uint32_t power_consumption;
    uint32_t utilization;
    float throughput_gops;
    float efficiency_percent;
} npu_perf_profile_t;

/**
 * Get comprehensive performance counters
 * @param handle NPU handle
 * @param perf Pointer to performance counters structure
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_get_comprehensive_perf_counters(npu_handle_t handle, struct npu_performance_counters *perf);

/**
 * Get device information
 * @param handle NPU handle
 * @param info Pointer to device info structure
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_get_device_info(npu_handle_t handle, struct npu_device_info *info);

/**
 * Get thermal information
 * @param handle NPU handle
 * @param thermal Pointer to thermal info structure
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_get_thermal_info(npu_handle_t handle, struct npu_thermal_info *thermal);

/**
 * Start performance profiling session
 * @param handle NPU handle
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_start_profiling(npu_handle_t handle);

/**
 * Stop performance profiling session and get results
 * @param handle NPU handle
 * @param profile Pointer to profiling results structure
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_stop_profiling(npu_handle_t handle, npu_perf_profile_t *profile);

/**
 * Calculate throughput in GOPS (Giga Operations Per Second)
 * @param operations Number of operations executed
 * @param time_ns Time elapsed in nanoseconds
 * @return Throughput in GOPS
 */
float npu_calculate_throughput(uint64_t operations, uint64_t time_ns);

/**
 * Calculate power efficiency in GOPS/Watt
 * @param throughput_gops Throughput in GOPS
 * @param power_watts Power consumption in watts
 * @return Efficiency in GOPS/Watt
 */
float npu_calculate_efficiency(float throughput_gops, float power_watts);

/**
 * Benchmark a specific operation
 * @param handle NPU handle
 * @param operation Operation type to benchmark
 * @param iterations Number of iterations to run
 * @param profile Pointer to profiling results
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_benchmark_operation(npu_handle_t handle, npu_operation_t operation, uint32_t iterations, npu_perf_profile_t *profile);

/**
 * Error Handling and Debugging Functions
 */

/**
 * Log levels for debugging
 */
typedef enum {
    NPU_LOG_ERROR = 0,
    NPU_LOG_WARN = 1,
    NPU_LOG_INFO = 2,
    NPU_LOG_DEBUG = 3,
    NPU_LOG_TRACE = 4
} npu_log_level_t;

/**
 * Error information structure
 */
typedef struct {
    int error_code;
    char message[256];
    char function[64];
    char file[128];
    int line;
    uint64_t timestamp;
} npu_error_info_t;

/**
 * Set global log level
 * @param level Log level to set
 */
void npu_set_log_level(npu_log_level_t level);

/**
 * Get current log level
 * @return Current log level
 */
npu_log_level_t npu_get_log_level(void);

/**
 * Enable or disable file logging
 * @param enable True to enable file logging
 * @param filename Log file path (NULL for default)
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_set_log_file(bool enable, const char *filename);

/**
 * Get detailed error information from driver
 * @param handle NPU handle
 * @param error_info Pointer to error info structure
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_get_error_info(npu_handle_t handle, npu_error_info_t *error_info);

/**
 * Get human-readable error string
 * @param error_code Error code
 * @return Error description string
 */
const char* npu_error_string(int error_code);

/**
 * Set error callback function
 * @param callback Callback function pointer
 */
void npu_set_error_callback(void (*callback)(const npu_error_info_t *error));

/**
 * Validate tensor parameters
 * @param tensor Tensor to validate
 * @return NPU_SUCCESS if valid, error code if invalid
 */
int npu_validate_tensor(const npu_tensor_t *tensor);

/**
 * Check device health and status
 * @param handle NPU handle
 * @param health_status Pointer to health status output
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_check_device_health(npu_handle_t handle, uint32_t *health_status);

/**
 * Enable debug mode for verbose output
 * @param handle NPU handle
 * @param enable True to enable debug mode
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_set_debug_mode(npu_handle_t handle, bool enable);

/**
 * Dump register values for debugging
 * @param handle NPU handle
 * @param registers Array to store register values
 * @param count Number of registers to read
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_dump_registers(npu_handle_t handle, uint32_t *registers, uint32_t count);

/**
 * Perform basic functionality test
 * @param handle NPU handle
 * @return NPU_SUCCESS if all tests pass, error code on failure
 */
int npu_self_test(npu_handle_t handle);

/**
 * Advanced activation functions
 */

/**
 * ReLU activation function: output = max(0, input)
 * @param handle NPU handle
 * @param input Input tensor
 * @param output Output tensor
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_relu(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output);

/**
 * Leaky ReLU activation: output = max(alpha*input, input)
 * @param handle NPU handle
 * @param input Input tensor
 * @param output Output tensor
 * @param alpha Negative slope coefficient
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_leaky_relu(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output, float alpha);

/**
 * Sigmoid activation function: output = 1 / (1 + exp(-input))
 * @param handle NPU handle
 * @param input Input tensor
 * @param output Output tensor
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_sigmoid(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output);

/**
 * Tanh activation function: output = tanh(input)
 * @param handle NPU handle
 * @param input Input tensor
 * @param output Output tensor
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_tanh(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output);

/**
 * Softmax activation function
 * @param handle NPU handle
 * @param input Input tensor
 * @param output Output tensor
 * @param axis Axis along which to apply softmax
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_softmax(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output, int axis);

/**
 * Batch normalization operation
 * @param handle NPU handle
 * @param input Input tensor
 * @param scale Scale parameters (gamma)
 * @param bias Bias parameters (beta)
 * @param mean Running mean
 * @param variance Running variance
 * @param output Output tensor
 * @param epsilon Small constant for numerical stability
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_batch_norm(npu_handle_t handle, const npu_tensor_t *input, 
                   const npu_tensor_t *scale, const npu_tensor_t *bias,
                   const npu_tensor_t *mean, const npu_tensor_t *variance,
                   npu_tensor_t *output, float epsilon);

/**
 * Max pooling operation
 * @param handle NPU handle
 * @param input Input tensor
 * @param output Output tensor
 * @param kernel_h Pooling kernel height
 * @param kernel_w Pooling kernel width
 * @param stride_h Vertical stride
 * @param stride_w Horizontal stride
 * @param pad_h Vertical padding
 * @param pad_w Horizontal padding
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_max_pool2d(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output,
                   uint32_t kernel_h, uint32_t kernel_w, 
                   uint32_t stride_h, uint32_t stride_w,
                   uint32_t pad_h, uint32_t pad_w);

/**
 * Average pooling operation
 * @param handle NPU handle
 * @param input Input tensor
 * @param output Output tensor
 * @param kernel_h Pooling kernel height
 * @param kernel_w Pooling kernel width
 * @param stride_h Vertical stride
 * @param stride_w Horizontal stride
 * @param pad_h Vertical padding
 * @param pad_w Horizontal padding
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_avg_pool2d(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output,
                   uint32_t kernel_h, uint32_t kernel_w,
                   uint32_t stride_h, uint32_t stride_w,
                   uint32_t pad_h, uint32_t pad_w);

/**
 * Global average pooling
 * @param handle NPU handle
 * @param input Input tensor
 * @param output Output tensor
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_global_avg_pool2d(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output);

/**
 * Dropout operation (for inference - identity operation)
 * @param handle NPU handle
 * @param input Input tensor
 * @param output Output tensor
 * @param dropout_rate Dropout rate (ignored in inference)
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_dropout(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output, float dropout_rate);

/**
 * Layer normalization
 * @param handle NPU handle
 * @param input Input tensor
 * @param weight Scale parameters
 * @param bias Bias parameters
 * @param output Output tensor
 * @param epsilon Small constant for numerical stability
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_layer_norm(npu_handle_t handle, const npu_tensor_t *input,
                   const npu_tensor_t *weight, const npu_tensor_t *bias,
                   npu_tensor_t *output, float epsilon);

/**
 * Concatenation operation
 * @param handle NPU handle
 * @param inputs Array of input tensors
 * @param num_inputs Number of input tensors
 * @param output Output tensor
 * @param axis Axis along which to concatenate
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_concat(npu_handle_t handle, const npu_tensor_t *inputs[], int num_inputs,
               npu_tensor_t *output, int axis);

/**
 * Transpose operation
 * @param handle NPU handle
 * @param input Input tensor
 * @param output Output tensor
 * @param perm Permutation of dimensions
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_transpose(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output, const int *perm);

/**
 * Reshape operation
 * @param handle NPU handle
 * @param input Input tensor
 * @param output Output tensor
 * @param new_shape New shape dimensions
 * @param num_dims Number of dimensions in new shape
 * @return NPU_SUCCESS on success, error code on failure
 */
int npu_reshape(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output,
                const uint32_t *new_shape, int num_dims);

#ifdef __cplusplus
}
#endif

#endif // FPGA_NPU_LIB_H