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

#ifdef __cplusplus
}
#endif

#endif // FPGA_NPU_LIB_H