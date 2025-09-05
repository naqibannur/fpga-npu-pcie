/**
 * FPGA NPU User-space Library Implementation
 * 
 * Implementation of the high-level API for FPGA NPU operations.
 */

#include "fpga_npu_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>

#define DEVICE_PATH "/dev/fpga_npu"
#define MAX_BUFFER_SIZE (1024 * 1024)  // 1MB

// NPU context structure
struct npu_context {
    int fd;                    // Device file descriptor
    void *buffer;              // Shared buffer
    size_t buffer_size;        // Buffer size
    uint32_t buffer_offset;    // Current buffer offset
};

// Status register bits (must match driver)
#define STATUS_READY    (1 << 0)
#define STATUS_BUSY     (1 << 1)
#define STATUS_ERROR    (1 << 2)
#define STATUS_DONE     (1 << 3)

// Internal helper functions
static int calculate_tensor_size(const npu_tensor_t *tensor);
static int copy_tensor_to_buffer(struct npu_context *ctx, const npu_tensor_t *tensor, uint32_t *offset);
static int copy_tensor_from_buffer(struct npu_context *ctx, npu_tensor_t *tensor, uint32_t offset);

/**
 * Initialize NPU library and open device
 */
npu_handle_t npu_init(void)
{
    struct npu_context *ctx;
    
    ctx = malloc(sizeof(struct npu_context));
    if (!ctx) {
        fprintf(stderr, "NPU: Failed to allocate context\n");
        return NULL;
    }
    
    // Open device
    ctx->fd = open(DEVICE_PATH, O_RDWR);
    if (ctx->fd < 0) {
        fprintf(stderr, "NPU: Failed to open device %s: %s\n", DEVICE_PATH, strerror(errno));
        free(ctx);
        return NULL;
    }
    
    // Allocate shared buffer
    ctx->buffer_size = MAX_BUFFER_SIZE;
    ctx->buffer = malloc(ctx->buffer_size);
    if (!ctx->buffer) {
        fprintf(stderr, "NPU: Failed to allocate buffer\n");
        close(ctx->fd);
        free(ctx);
        return NULL;
    }
    
    ctx->buffer_offset = 0;
    
    printf("NPU: Initialized successfully\n");
    return (npu_handle_t)ctx;
}

/**
 * Cleanup and close NPU device
 */
int npu_cleanup(npu_handle_t handle)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    
    if (!ctx) {
        return NPU_ERROR_INVALID;
    }
    
    if (ctx->buffer) {
        free(ctx->buffer);
    }
    
    if (ctx->fd >= 0) {
        close(ctx->fd);
    }
    
    free(ctx);
    
    printf("NPU: Cleanup completed\n");
    return NPU_SUCCESS;
}

/**
 * Allocate memory buffer accessible by NPU
 */
void* npu_alloc(npu_handle_t handle, size_t size)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    
    if (!ctx || size == 0) {
        return NULL;
    }
    
    if (ctx->buffer_offset + size > ctx->buffer_size) {
        fprintf(stderr, "NPU: Not enough buffer space\n");
        return NULL;
    }
    
    void *ptr = (char *)ctx->buffer + ctx->buffer_offset;
    ctx->buffer_offset += size;
    
    return ptr;
}

/**
 * Free NPU-accessible memory (simplified implementation)
 */
void npu_free(npu_handle_t handle, void *ptr)
{
    // In this simplified implementation, we don't track individual allocations
    // Real implementation would need a proper memory manager
    (void)handle;
    (void)ptr;
}

/**
 * Create tensor descriptor
 */
npu_tensor_t npu_create_tensor(void *data, uint32_t n, uint32_t c, uint32_t h, uint32_t w, npu_dtype_t dtype)
{
    npu_tensor_t tensor;
    size_t element_size;
    
    tensor.data = data;
    tensor.dims[0] = n;
    tensor.dims[1] = c;
    tensor.dims[2] = h;
    tensor.dims[3] = w;
    tensor.dtype = dtype;
    
    // Calculate element size
    switch (dtype) {
        case NPU_DTYPE_INT8:   element_size = 1; break;
        case NPU_DTYPE_INT16:  element_size = 2; break;
        case NPU_DTYPE_INT32:  element_size = 4; break;
        case NPU_DTYPE_FLOAT32: element_size = 4; break;
        default: element_size = 1; break;
    }
    
    tensor.size = n * c * h * w * element_size;
    
    return tensor;
}

/**
 * Execute single NPU instruction
 */
int npu_execute_instruction(npu_handle_t handle, const npu_instruction_t *inst)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    ssize_t bytes_written;
    
    if (!ctx || !inst) {
        return NPU_ERROR_INVALID;
    }
    
    // Copy instruction to buffer
    memcpy(ctx->buffer, inst, sizeof(npu_instruction_t));
    
    // Send to device
    bytes_written = write(ctx->fd, ctx->buffer, sizeof(npu_instruction_t));
    if (bytes_written != sizeof(npu_instruction_t)) {
        fprintf(stderr, "NPU: Failed to write instruction\n");
        return NPU_ERROR_DEVICE;
    }
    
    return NPU_SUCCESS;
}

/**
 * Execute batch of NPU instructions
 */
int npu_execute_batch(npu_handle_t handle, const npu_instruction_t *instructions, size_t count)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    size_t batch_size = count * sizeof(npu_instruction_t);
    ssize_t bytes_written;
    
    if (!ctx || !instructions || count == 0) {
        return NPU_ERROR_INVALID;
    }
    
    if (batch_size > ctx->buffer_size) {
        return NPU_ERROR_MEMORY;
    }
    
    // Copy instructions to buffer
    memcpy(ctx->buffer, instructions, batch_size);
    
    // Send to device
    bytes_written = write(ctx->fd, ctx->buffer, batch_size);
    if (bytes_written != (ssize_t)batch_size) {
        fprintf(stderr, "NPU: Failed to write instruction batch\n");
        return NPU_ERROR_DEVICE;
    }
    
    return NPU_SUCCESS;
}

/**
 * Wait for NPU operation completion
 */
int npu_wait_completion(npu_handle_t handle, uint32_t timeout_ms)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    
    if (!ctx) {
        return NPU_ERROR_INVALID;
    }
    
    // Use ioctl to wait for completion
    if (ioctl(ctx->fd, 1, 0) < 0) {
        return NPU_ERROR_DEVICE;
    }
    
    return NPU_SUCCESS;
}

/**
 * Get NPU status
 */
int npu_get_status(npu_handle_t handle, uint32_t *status)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    
    if (!ctx || !status) {
        return NPU_ERROR_INVALID;
    }
    
    if (ioctl(ctx->fd, 0, status) < 0) {
        return NPU_ERROR_DEVICE;
    }
    
    return NPU_SUCCESS;
}

/**
 * Matrix multiplication: C = A * B
 */
int npu_matrix_multiply(npu_handle_t handle, const npu_tensor_t *a, const npu_tensor_t *b, npu_tensor_t *c)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    npu_instruction_t inst;
    uint32_t offset_a, offset_b, offset_c;
    int ret;
    
    if (!ctx || !a || !b || !c) {
        return NPU_ERROR_INVALID;
    }
    
    // Reset buffer offset
    ctx->buffer_offset = 0;
    
    // Copy tensors to buffer
    ret = copy_tensor_to_buffer(ctx, a, &offset_a);
    if (ret != NPU_SUCCESS) return ret;
    
    ret = copy_tensor_to_buffer(ctx, b, &offset_b);
    if (ret != NPU_SUCCESS) return ret;
    
    ret = copy_tensor_to_buffer(ctx, c, &offset_c);
    if (ret != NPU_SUCCESS) return ret;
    
    // Prepare instruction
    memset(&inst, 0, sizeof(inst));
    inst.op = NPU_OP_MATMUL;
    inst.src1_addr = offset_a;
    inst.src2_addr = offset_b;
    inst.dst_addr = offset_c;
    inst.size = c->size;
    inst.params[0] = a->dims[2];  // M
    inst.params[1] = a->dims[3];  // K
    inst.params[2] = b->dims[3];  // N
    
    // Execute
    ret = npu_execute_instruction(handle, &inst);
    if (ret != NPU_SUCCESS) return ret;
    
    ret = npu_wait_completion(handle, 0);
    if (ret != NPU_SUCCESS) return ret;
    
    // Copy result back
    return copy_tensor_from_buffer(ctx, c, offset_c);
}

/**
 * 2D Convolution operation
 */
int npu_conv2d(npu_handle_t handle, const npu_tensor_t *input, const npu_tensor_t *weights, 
               npu_tensor_t *output, uint32_t stride_h, uint32_t stride_w, 
               uint32_t pad_h, uint32_t pad_w)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    npu_instruction_t inst;
    uint32_t offset_input, offset_weights, offset_output;
    int ret;
    
    if (!ctx || !input || !weights || !output) {
        return NPU_ERROR_INVALID;
    }
    
    // Reset buffer offset
    ctx->buffer_offset = 0;
    
    // Copy tensors to buffer
    ret = copy_tensor_to_buffer(ctx, input, &offset_input);
    if (ret != NPU_SUCCESS) return ret;
    
    ret = copy_tensor_to_buffer(ctx, weights, &offset_weights);
    if (ret != NPU_SUCCESS) return ret;
    
    ret = copy_tensor_to_buffer(ctx, output, &offset_output);
    if (ret != NPU_SUCCESS) return ret;
    
    // Prepare instruction
    memset(&inst, 0, sizeof(inst));
    inst.op = NPU_OP_CONV;
    inst.src1_addr = offset_input;
    inst.src2_addr = offset_weights;
    inst.dst_addr = offset_output;
    inst.size = output->size;
    inst.params[0] = (stride_h << 16) | stride_w;
    inst.params[1] = (pad_h << 16) | pad_w;
    
    // Execute
    ret = npu_execute_instruction(handle, &inst);
    if (ret != NPU_SUCCESS) return ret;
    
    ret = npu_wait_completion(handle, 0);
    if (ret != NPU_SUCCESS) return ret;
    
    // Copy result back
    return copy_tensor_from_buffer(ctx, output, offset_output);
}

/**
 * Element-wise addition: C = A + B
 */
int npu_add(npu_handle_t handle, const npu_tensor_t *a, const npu_tensor_t *b, npu_tensor_t *c)
{
    npu_instruction_t inst;
    int ret;
    
    if (!handle || !a || !b || !c) {
        return NPU_ERROR_INVALID;
    }
    
    // Prepare instruction
    memset(&inst, 0, sizeof(inst));
    inst.op = NPU_OP_ADD;
    inst.size = c->size;
    
    ret = npu_execute_instruction(handle, &inst);
    if (ret != NPU_SUCCESS) return ret;
    
    return npu_wait_completion(handle, 0);
}

/**
 * Element-wise multiplication: C = A * B
 */
int npu_multiply(npu_handle_t handle, const npu_tensor_t *a, const npu_tensor_t *b, npu_tensor_t *c)
{
    npu_instruction_t inst;
    int ret;
    
    if (!handle || !a || !b || !c) {
        return NPU_ERROR_INVALID;
    }
    
    // Prepare instruction
    memset(&inst, 0, sizeof(inst));
    inst.op = NPU_OP_MUL;
    inst.size = c->size;
    
    ret = npu_execute_instruction(handle, &inst);
    if (ret != NPU_SUCCESS) return ret;
    
    return npu_wait_completion(handle, 0);
}

/**
 * Get NPU performance counters
 */
int npu_get_performance_counters(npu_handle_t handle, uint64_t *cycles, uint64_t *operations)
{
    // Placeholder implementation
    if (!handle || !cycles || !operations) {
        return NPU_ERROR_INVALID;
    }
    
    *cycles = 0;
    *operations = 0;
    
    return NPU_SUCCESS;
}

/**
 * Reset NPU performance counters
 */
int npu_reset_performance_counters(npu_handle_t handle)
{
    // Placeholder implementation
    if (!handle) {
        return NPU_ERROR_INVALID;
    }
    
    return NPU_SUCCESS;
}

// Helper functions

static int calculate_tensor_size(const npu_tensor_t *tensor)
{
    if (!tensor) {
        return 0;
    }
    return tensor->size;
}

static int copy_tensor_to_buffer(struct npu_context *ctx, const npu_tensor_t *tensor, uint32_t *offset)
{
    if (!ctx || !tensor || !offset) {
        return NPU_ERROR_INVALID;
    }
    
    if (ctx->buffer_offset + tensor->size > ctx->buffer_size) {
        return NPU_ERROR_MEMORY;
    }
    
    *offset = ctx->buffer_offset;
    memcpy((char *)ctx->buffer + ctx->buffer_offset, tensor->data, tensor->size);
    ctx->buffer_offset += tensor->size;
    
    return NPU_SUCCESS;
}

static int copy_tensor_from_buffer(struct npu_context *ctx, npu_tensor_t *tensor, uint32_t offset)
{
    if (!ctx || !tensor) {
        return NPU_ERROR_INVALID;
    }
    
    if (offset + tensor->size > ctx->buffer_size) {
        return NPU_ERROR_MEMORY;
    }
    
    memcpy(tensor->data, (char *)ctx->buffer + offset, tensor->size);
    
    return NPU_SUCCESS;
}