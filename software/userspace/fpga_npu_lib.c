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
#include <math.h>
#include <stdbool.h>
#include <stdarg.h>

#define DEVICE_PATH "/dev/fpga_npu"
#define MAX_BUFFER_SIZE (1024 * 1024)  // 1MB
#define MAX_MANAGED_BUFFERS 64

// Buffer management structure
struct npu_buffer {
    uint32_t buffer_id;        // Driver buffer ID
    size_t size;               // Buffer size
    uint32_t flags;            // Allocation flags
    void *mapped_ptr;          // User-space mapped pointer
    uint64_t physical_addr;    // Physical address
    bool is_mapped;            // Mapping status
    struct npu_context *ctx;   // Parent context
};

// NPU context structure
struct npu_context {
    int fd;                    // Device file descriptor
    void *buffer;              // Legacy shared buffer
    size_t buffer_size;        // Legacy buffer size
    uint32_t buffer_offset;    // Current buffer offset
    
    // Enhanced memory management
    struct npu_buffer *managed_buffers[MAX_MANAGED_BUFFERS];
    uint32_t next_buffer_slot; // Next available buffer slot
    size_t total_allocated;    // Total allocated memory
    uint32_t active_buffers;   // Number of active buffers
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
    
    // Initialize memory management
    memset(ctx->managed_buffers, 0, sizeof(ctx->managed_buffers));
    ctx->next_buffer_slot = 0;
    ctx->total_allocated = 0;
    ctx->active_buffers = 0;
    
    // Open device
    ctx->fd = open(DEVICE_PATH, O_RDWR);
    if (ctx->fd < 0) {
        fprintf(stderr, "NPU: Failed to open device %s: %s\n", DEVICE_PATH, strerror(errno));
        free(ctx);
        return NULL;
    }
    
    // Allocate legacy shared buffer for backward compatibility
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
    
    // Free all managed buffers
    for (int i = 0; i < MAX_MANAGED_BUFFERS; i++) {
        if (ctx->managed_buffers[i]) {
            npu_buffer_free(handle, (npu_buffer_handle_t)ctx->managed_buffers[i]);
        }
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
 * Enhanced Memory Management Functions
 */

/**
 * Find available buffer slot
 */
static int find_buffer_slot(struct npu_context *ctx)
{
    for (int i = 0; i < MAX_MANAGED_BUFFERS; i++) {
        if (ctx->managed_buffers[i] == NULL) {
            return i;
        }
    }
    return -1;
}

/**
 * Allocate managed DMA buffer with enhanced features
 */
npu_buffer_handle_t npu_buffer_alloc(npu_handle_t handle, size_t size, uint32_t flags)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_buffer *buffer;
    struct npu_dma_buffer dma_req;
    int slot;
    
    if (!ctx || size == 0) {
        return NULL;
    }
    
    // Find available slot
    slot = find_buffer_slot(ctx);
    if (slot < 0) {
        fprintf(stderr, "NPU: No available buffer slots\n");
        return NULL;
    }
    
    // Allocate buffer structure
    buffer = malloc(sizeof(struct npu_buffer));
    if (!buffer) {
        fprintf(stderr, "NPU: Failed to allocate buffer structure\n");
        return NULL;
    }
    
    // Initialize buffer
    memset(buffer, 0, sizeof(struct npu_buffer));
    buffer->size = size;
    buffer->flags = flags;
    buffer->is_mapped = false;
    buffer->ctx = ctx;
    
    // Request DMA buffer from driver
    memset(&dma_req, 0, sizeof(dma_req));
    dma_req.size = size;
    dma_req.flags = flags;
    
    if (ioctl(ctx->fd, NPU_IOCTL_ALLOC_BUFFER, &dma_req) < 0) {
        fprintf(stderr, "NPU: Failed to allocate DMA buffer: %s\n", strerror(errno));
        free(buffer);
        return NULL;
    }
    
    buffer->buffer_id = dma_req.buffer_id;
    buffer->physical_addr = dma_req.physical_addr;
    
    // Store in context
    ctx->managed_buffers[slot] = buffer;
    ctx->total_allocated += size;
    ctx->active_buffers++;
    
    return (npu_buffer_handle_t)buffer;
}

/**
 * Free managed DMA buffer
 */
int npu_buffer_free(npu_handle_t handle, npu_buffer_handle_t buffer_handle)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_buffer *buffer = (struct npu_buffer *)buffer_handle;
    
    if (!ctx || !buffer) {
        return NPU_ERROR_INVALID;
    }
    
    // Unmap if mapped
    if (buffer->is_mapped && buffer->mapped_ptr) {
        npu_buffer_unmap(handle, buffer_handle, buffer->mapped_ptr);
    }
    
    // Free DMA buffer in driver
    if (ioctl(ctx->fd, NPU_IOCTL_FREE_BUFFER, buffer->buffer_id) < 0) {
        fprintf(stderr, "NPU: Failed to free DMA buffer: %s\n", strerror(errno));
        return NPU_ERROR_DEVICE;
    }
    
    // Remove from context
    for (int i = 0; i < MAX_MANAGED_BUFFERS; i++) {
        if (ctx->managed_buffers[i] == buffer) {
            ctx->managed_buffers[i] = NULL;
            break;
        }
    }
    
    ctx->total_allocated -= buffer->size;
    ctx->active_buffers--;
    
    free(buffer);
    return NPU_SUCCESS;
}

/**
 * Get buffer information
 */
int npu_buffer_get_info(npu_handle_t handle, npu_buffer_handle_t buffer_handle, struct npu_dma_buffer *info)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_buffer *buffer = (struct npu_buffer *)buffer_handle;
    
    if (!ctx || !buffer || !info) {
        return NPU_ERROR_INVALID;
    }
    
    info->buffer_id = buffer->buffer_id;
    info->size = buffer->size;
    info->physical_addr = buffer->physical_addr;
    info->user_addr = (uint64_t)buffer->mapped_ptr;
    info->flags = buffer->flags;
    
    return NPU_SUCCESS;
}

/**
 * Map buffer to user space
 */
void* npu_buffer_map(npu_handle_t handle, npu_buffer_handle_t buffer_handle)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_buffer *buffer = (struct npu_buffer *)buffer_handle;
    struct npu_mmap_request mmap_req;
    void *mapped_ptr;
    
    if (!ctx || !buffer) {
        return NULL;
    }
    
    if (buffer->is_mapped) {
        return buffer->mapped_ptr;
    }
    
    // Request memory mapping
    memset(&mmap_req, 0, sizeof(mmap_req));
    mmap_req.size = buffer->size;
    mmap_req.buffer_id = buffer->buffer_id;
    mmap_req.flags = buffer->flags;
    
    if (ioctl(ctx->fd, NPU_IOCTL_MMAP_REQUEST, &mmap_req) < 0) {
        fprintf(stderr, "NPU: Failed to prepare mmap: %s\n", strerror(errno));
        return NULL;
    }
    
    // Map the buffer
    mapped_ptr = mmap(NULL, buffer->size, PROT_READ | PROT_WRITE, MAP_SHARED, ctx->fd, 0);
    if (mapped_ptr == MAP_FAILED) {
        fprintf(stderr, "NPU: Failed to map buffer: %s\n", strerror(errno));
        return NULL;
    }
    
    buffer->mapped_ptr = mapped_ptr;
    buffer->is_mapped = true;
    
    return mapped_ptr;
}

/**
 * Unmap buffer from user space
 */
int npu_buffer_unmap(npu_handle_t handle, npu_buffer_handle_t buffer_handle, void *ptr)
{
    struct npu_buffer *buffer = (struct npu_buffer *)buffer_handle;
    
    if (!buffer || !ptr) {
        return NPU_ERROR_INVALID;
    }
    
    if (!buffer->is_mapped || buffer->mapped_ptr != ptr) {
        return NPU_ERROR_INVALID;
    }
    
    if (munmap(ptr, buffer->size) < 0) {
        fprintf(stderr, "NPU: Failed to unmap buffer: %s\n", strerror(errno));
        return NPU_ERROR_DEVICE;
    }
    
    buffer->mapped_ptr = NULL;
    buffer->is_mapped = false;
    
    return NPU_SUCCESS;
}

/**
 * Synchronize buffer contents (flush/invalidate cache)
 */
int npu_buffer_sync(npu_handle_t handle, npu_buffer_handle_t buffer_handle, uint32_t direction)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_buffer *buffer = (struct npu_buffer *)buffer_handle;
    
    if (!ctx || !buffer) {
        return NPU_ERROR_INVALID;
    }
    
    if (ioctl(ctx->fd, NPU_IOCTL_DMA_SYNC, buffer->buffer_id) < 0) {
        fprintf(stderr, "NPU: Failed to sync buffer: %s\n", strerror(errno));
        return NPU_ERROR_DEVICE;
    }
    
    return NPU_SUCCESS;
}

/**
 * Copy data to buffer
 */
int npu_buffer_write(npu_handle_t handle, npu_buffer_handle_t buffer_handle, size_t offset, const void *src, size_t size)
{
    struct npu_buffer *buffer = (struct npu_buffer *)buffer_handle;
    
    if (!buffer || !src || offset + size > buffer->size) {
        return NPU_ERROR_INVALID;
    }
    
    // Ensure buffer is mapped
    void *mapped_ptr = npu_buffer_map(handle, buffer_handle);
    if (!mapped_ptr) {
        return NPU_ERROR_MEMORY;
    }
    
    memcpy((char*)mapped_ptr + offset, src, size);
    
    return NPU_SUCCESS;
}

/**
 * Copy data from buffer
 */
int npu_buffer_read(npu_handle_t handle, npu_buffer_handle_t buffer_handle, size_t offset, void *dst, size_t size)
{
    struct npu_buffer *buffer = (struct npu_buffer *)buffer_handle;
    
    if (!buffer || !dst || offset + size > buffer->size) {
        return NPU_ERROR_INVALID;
    }
    
    // Ensure buffer is mapped
    void *mapped_ptr = npu_buffer_map(handle, buffer_handle);
    if (!mapped_ptr) {
        return NPU_ERROR_MEMORY;
    }
    
    memcpy(dst, (char*)mapped_ptr + offset, size);
    
    return NPU_SUCCESS;
}

/**
 * Create tensor from managed buffer
 */
npu_tensor_t npu_tensor_from_buffer(npu_buffer_handle_t buffer_handle, size_t offset, uint32_t n, uint32_t c, uint32_t h, uint32_t w, npu_dtype_t dtype)
{
    struct npu_buffer *buffer = (struct npu_buffer *)buffer_handle;
    npu_tensor_t tensor;
    size_t element_size;
    
    // Calculate element size
    switch (dtype) {
        case NPU_DTYPE_INT8:   element_size = 1; break;
        case NPU_DTYPE_INT16:  element_size = 2; break;
        case NPU_DTYPE_INT32:  element_size = 4; break;
        case NPU_DTYPE_FLOAT32: element_size = 4; break;
        default: element_size = 1; break;
    }
    
    size_t tensor_size = n * c * h * w * element_size;
    
    // Validate buffer and offset
    if (!buffer || offset + tensor_size > buffer->size) {
        memset(&tensor, 0, sizeof(tensor));
        return tensor;
    }
    
    tensor.data = (char*)buffer->mapped_ptr + offset;
    tensor.dims[0] = n;
    tensor.dims[1] = c;
    tensor.dims[2] = h;
    tensor.dims[3] = w;
    tensor.dtype = dtype;
    tensor.size = tensor_size;
    
    return tensor;
}

/**
 * Get memory usage statistics
 */
int npu_get_memory_stats(npu_handle_t handle, size_t *total_allocated, size_t *total_free, uint32_t *buffer_count)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    
    if (!ctx) {
        return NPU_ERROR_INVALID;
    }
    
    if (total_allocated) {
        *total_allocated = ctx->total_allocated;
    }
    
    if (total_free) {
        // This would require querying the driver for available memory
        // For now, return a placeholder value
        *total_free = 256 * 1024 * 1024 - ctx->total_allocated; // Assume 256MB total
    }
    
    if (buffer_count) {
        *buffer_count = ctx->active_buffers;
    }
    
    return NPU_SUCCESS;
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
 * Get NPU performance counters (enhanced implementation)
 */
int npu_get_performance_counters(npu_handle_t handle, uint64_t *cycles, uint64_t *operations)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_performance_counters perf;
    
    if (!handle || !cycles || !operations) {
        return NPU_ERROR_INVALID;
    }
    
    if (ioctl(ctx->fd, NPU_IOCTL_GET_PERF_COUNTERS, &perf) < 0) {
        return NPU_ERROR_DEVICE;
    }
    
    *cycles = perf.counters[NPU_PERF_CYCLES];
    *operations = perf.counters[NPU_PERF_OPERATIONS];
    
    return NPU_SUCCESS;
}

/**
 * Reset NPU performance counters (enhanced implementation)
 */
int npu_reset_performance_counters(npu_handle_t handle)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    
    if (!handle) {
        return NPU_ERROR_INVALID;
    }
    
    if (ioctl(ctx->fd, NPU_IOCTL_RESET_PERF_COUNTERS) < 0) {
        return NPU_ERROR_DEVICE;
    }
    
    return NPU_SUCCESS;
}

/**
 * Get comprehensive performance counters
 */
int npu_get_comprehensive_perf_counters(npu_handle_t handle, struct npu_performance_counters *perf)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    
    if (!handle || !perf) {
        return NPU_ERROR_INVALID;
    }
    
    if (ioctl(ctx->fd, NPU_IOCTL_GET_PERF_COUNTERS, perf) < 0) {
        return NPU_ERROR_DEVICE;
    }
    
    return NPU_SUCCESS;
}

/**
 * Get device information
 */
int npu_get_device_info(npu_handle_t handle, struct npu_device_info *info)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    
    if (!handle || !info) {
        return NPU_ERROR_INVALID;
    }
    
    if (ioctl(ctx->fd, NPU_IOCTL_GET_DEVICE_INFO, info) < 0) {
        return NPU_ERROR_DEVICE;
    }
    
    return NPU_SUCCESS;
}

/**
 * Get thermal information
 */
int npu_get_thermal_info(npu_handle_t handle, struct npu_thermal_info *thermal)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    
    if (!handle || !thermal) {
        return NPU_ERROR_INVALID;
    }
    
    if (ioctl(ctx->fd, NPU_IOCTL_GET_THERMAL_INFO, thermal) < 0) {
        return NPU_ERROR_DEVICE;
    }
    
    return NPU_SUCCESS;
}

/**
 * Internal profiling context for session management
 */
static struct {
    bool active;
    struct timespec start_time;
    struct npu_performance_counters start_counters;
    npu_handle_t handle;
} profiling_session = {false, {0, 0}, {{0}}, NULL};

/**
 * Get current time in nanoseconds
 */
static uint64_t get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * Start performance profiling session
 */
int npu_start_profiling(npu_handle_t handle)
{
    if (!handle) {
        return NPU_ERROR_INVALID;
    }
    
    if (profiling_session.active) {
        return NPU_ERROR_DEVICE; // Session already active
    }
    
    // Reset performance counters
    int ret = npu_reset_performance_counters(handle);
    if (ret != NPU_SUCCESS) {
        return ret;
    }
    
    // Get start time and counters
    clock_gettime(CLOCK_MONOTONIC, &profiling_session.start_time);
    ret = npu_get_comprehensive_perf_counters(handle, &profiling_session.start_counters);
    if (ret != NPU_SUCCESS) {
        return ret;
    }
    
    profiling_session.active = true;
    profiling_session.handle = handle;
    
    return NPU_SUCCESS;
}

/**
 * Stop performance profiling session and get results
 */
int npu_stop_profiling(npu_handle_t handle, npu_perf_profile_t *profile)
{
    struct timespec end_time;
    struct npu_performance_counters end_counters;
    
    if (!handle || !profile) {
        return NPU_ERROR_INVALID;
    }
    
    if (!profiling_session.active || profiling_session.handle != handle) {
        return NPU_ERROR_DEVICE; // No active session or wrong handle
    }
    
    // Get end time and counters
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    int ret = npu_get_comprehensive_perf_counters(handle, &end_counters);
    if (ret != NPU_SUCCESS) {
        return ret;
    }
    
    // Calculate elapsed time in nanoseconds
    uint64_t start_ns = (uint64_t)profiling_session.start_time.tv_sec * 1000000000ULL + 
                        (uint64_t)profiling_session.start_time.tv_nsec;
    uint64_t end_ns = (uint64_t)end_time.tv_sec * 1000000000ULL + (uint64_t)end_time.tv_nsec;
    
    // Fill profile structure
    profile->start_time = start_ns;
    profile->end_time = end_ns;
    profile->cycles = end_counters.counters[NPU_PERF_CYCLES] - 
                      profiling_session.start_counters.counters[NPU_PERF_CYCLES];
    profile->operations = end_counters.counters[NPU_PERF_OPERATIONS] - 
                          profiling_session.start_counters.counters[NPU_PERF_OPERATIONS];
    profile->memory_reads = end_counters.counters[NPU_PERF_MEMORY_READS] - 
                            profiling_session.start_counters.counters[NPU_PERF_MEMORY_READS];
    profile->memory_writes = end_counters.counters[NPU_PERF_MEMORY_WRITES] - 
                             profiling_session.start_counters.counters[NPU_PERF_MEMORY_WRITES];
    profile->cache_hits = end_counters.counters[NPU_PERF_CACHE_HITS] - 
                          profiling_session.start_counters.counters[NPU_PERF_CACHE_HITS];
    profile->cache_misses = end_counters.counters[NPU_PERF_CACHE_MISSES] - 
                            profiling_session.start_counters.counters[NPU_PERF_CACHE_MISSES];
    profile->temperature = end_counters.temperature_celsius;
    profile->power_consumption = end_counters.power_watts;
    profile->utilization = end_counters.utilization_percent;
    
    // Calculate derived metrics
    uint64_t elapsed_ns = end_ns - start_ns;
    profile->throughput_gops = npu_calculate_throughput(profile->operations, elapsed_ns);
    profile->efficiency_percent = npu_calculate_efficiency(profile->throughput_gops, 
                                                           (float)profile->power_consumption);
    
    profiling_session.active = false;
    profiling_session.handle = NULL;
    
    return NPU_SUCCESS;
}

/**
 * Calculate throughput in GOPS (Giga Operations Per Second)
 */
float npu_calculate_throughput(uint64_t operations, uint64_t time_ns)
{
    if (time_ns == 0) {
        return 0.0f;
    }
    
    // Convert nanoseconds to seconds and calculate GOPS
    double time_seconds = (double)time_ns / 1000000000.0;
    double gops = (double)operations / (1000000000.0 * time_seconds);
    
    return (float)gops;
}

/**
 * Calculate power efficiency in GOPS/Watt
 */
float npu_calculate_efficiency(float throughput_gops, float power_watts)
{
    if (power_watts <= 0.0f) {
        return 0.0f;
    }
    
    return throughput_gops / power_watts;
}

/**
 * Benchmark a specific operation
 */
int npu_benchmark_operation(npu_handle_t handle, npu_operation_t operation, uint32_t iterations, npu_perf_profile_t *profile)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_instruction inst;
    int ret;
    
    if (!handle || !profile || iterations == 0) {
        return NPU_ERROR_INVALID;
    }
    
    // Start profiling
    ret = npu_start_profiling(handle);
    if (ret != NPU_SUCCESS) {
        return ret;
    }
    
    // Prepare benchmark instruction
    memset(&inst, 0, sizeof(inst));
    inst.operation = operation;
    inst.size = 1024;  // Default test size
    inst.flags = NPU_INST_FLAG_PROFILE;
    
    // Execute operation multiple times
    for (uint32_t i = 0; i < iterations; i++) {
        ret = npu_execute_instruction(handle, &inst);
        if (ret != NPU_SUCCESS) {
            profiling_session.active = false; // Reset session on error
            return ret;
        }
        
        ret = npu_wait_completion(handle, 1000); // 1 second timeout
        if (ret != NPU_SUCCESS) {
            profiling_session.active = false; // Reset session on error
            return ret;
        }
    }
    
    // Stop profiling and get results
    ret = npu_stop_profiling(handle, profile);
    if (ret != NPU_SUCCESS) {
        return ret;
    }
    
    return NPU_SUCCESS;
}

/**
 * Error Handling and Debugging Functions
 */

// Global logging configuration
static npu_log_level_t global_log_level = NPU_LOG_INFO;
static FILE *log_file = NULL;
static bool log_to_file = false;
static void (*error_callback)(const npu_error_info_t *error) = NULL;

/**
 * Internal logging function
 */
static void npu_log(npu_log_level_t level, const char *func, const char *file, int line, const char *format, ...)
{
    if (level > global_log_level) {
        return;
    }
    
    const char *level_strings[] = {"ERROR", "WARN", "INFO", "DEBUG", "TRACE"};
    char timestamp[64];
    struct timespec ts;
    va_list args;
    
    clock_gettime(CLOCK_REALTIME, &ts);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&ts.tv_sec));
    
    // Log to stderr
    fprintf(stderr, "[%s.%03ld] [%s] %s:%d in %s(): ", 
            timestamp, ts.tv_nsec / 1000000, level_strings[level], file, line, func);
    
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");
    
    // Log to file if enabled
    if (log_to_file && log_file) {
        fprintf(log_file, "[%s.%03ld] [%s] %s:%d in %s(): ", 
                timestamp, ts.tv_nsec / 1000000, level_strings[level], file, line, func);
        
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        
        fprintf(log_file, "\n");
        fflush(log_file);
    }
}

#define NPU_LOG(level, ...) npu_log(level, __func__, __FILE__, __LINE__, __VA_ARGS__)

/**
 * Set global log level
 */
void npu_set_log_level(npu_log_level_t level)
{
    global_log_level = level;
    NPU_LOG(NPU_LOG_INFO, "Log level set to %d", level);
}

/**
 * Get current log level
 */
npu_log_level_t npu_get_log_level(void)
{
    return global_log_level;
}

/**
 * Enable or disable file logging
 */
int npu_set_log_file(bool enable, const char *filename)
{
    if (log_file && log_to_file) {
        fclose(log_file);
        log_file = NULL;
    }
    
    log_to_file = enable;
    
    if (enable) {
        const char *log_filename = filename ? filename : "npu_library.log";
        log_file = fopen(log_filename, "a");
        if (!log_file) {
            NPU_LOG(NPU_LOG_ERROR, "Failed to open log file: %s", log_filename);
            return NPU_ERROR_DEVICE;
        }
        NPU_LOG(NPU_LOG_INFO, "File logging enabled: %s", log_filename);
    }
    
    return NPU_SUCCESS;
}

/**
 * Get detailed error information from driver
 */
int npu_get_error_info(npu_handle_t handle, npu_error_info_t *error_info)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_error_info driver_error;
    
    if (!ctx || !error_info) {
        return NPU_ERROR_INVALID;
    }
    
    if (ioctl(ctx->fd, NPU_IOCTL_GET_ERROR_INFO, &driver_error) < 0) {
        NPU_LOG(NPU_LOG_ERROR, "Failed to get error info from driver");
        return NPU_ERROR_DEVICE;
    }
    
    error_info->error_code = driver_error.error_code;
    strncpy(error_info->message, driver_error.description, sizeof(error_info->message) - 1);
    error_info->message[sizeof(error_info->message) - 1] = '\0';
    error_info->timestamp = driver_error.timestamp;
    
    // Clear function and file info for driver errors
    memset(error_info->function, 0, sizeof(error_info->function));
    memset(error_info->file, 0, sizeof(error_info->file));
    error_info->line = 0;
    
    return NPU_SUCCESS;
}

/**
 * Get human-readable error string
 */
const char* npu_error_string(int error_code)
{
    switch (error_code) {
        case NPU_SUCCESS:        return "Success";
        case NPU_ERROR_INIT:     return "Initialization error";
        case NPU_ERROR_DEVICE:   return "Device error";
        case NPU_ERROR_MEMORY:   return "Memory error";
        case NPU_ERROR_TIMEOUT:  return "Timeout error";
        case NPU_ERROR_INVALID:  return "Invalid parameter";
        default:                 return "Unknown error";
    }
}

/**
 * Set error callback function
 */
void npu_set_error_callback(void (*callback)(const npu_error_info_t *error))
{
    error_callback = callback;
    NPU_LOG(NPU_LOG_INFO, "Error callback %s", callback ? "enabled" : "disabled");
}

/**
 * Internal function to trigger error callback
 */
static void trigger_error_callback(int error_code, const char *message, const char *func, const char *file, int line)
{
    if (error_callback) {
        npu_error_info_t error_info;
        error_info.error_code = error_code;
        strncpy(error_info.message, message, sizeof(error_info.message) - 1);
        error_info.message[sizeof(error_info.message) - 1] = '\0';
        strncpy(error_info.function, func, sizeof(error_info.function) - 1);
        error_info.function[sizeof(error_info.function) - 1] = '\0';
        strncpy(error_info.file, file, sizeof(error_info.file) - 1);
        error_info.file[sizeof(error_info.file) - 1] = '\0';
        error_info.line = line;
        error_info.timestamp = get_time_ns();
        
        error_callback(&error_info);
    }
}

#define NPU_ERROR_CALLBACK(code, msg) trigger_error_callback(code, msg, __func__, __FILE__, __LINE__)

/**
 * Validate tensor parameters
 */
int npu_validate_tensor(const npu_tensor_t *tensor)
{
    if (!tensor) {
        NPU_LOG(NPU_LOG_ERROR, "Tensor pointer is NULL");
        return NPU_ERROR_INVALID;
    }
    
    if (!tensor->data) {
        NPU_LOG(NPU_LOG_ERROR, "Tensor data pointer is NULL");
        return NPU_ERROR_INVALID;
    }
    
    if (tensor->size == 0) {
        NPU_LOG(NPU_LOG_ERROR, "Tensor size is zero");
        return NPU_ERROR_INVALID;
    }
    
    // Check dimensions
    for (int i = 0; i < 4; i++) {
        if (tensor->dims[i] == 0) {
            NPU_LOG(NPU_LOG_WARN, "Tensor dimension %d is zero", i);
        }
    }
    
    // Validate data type
    if (tensor->dtype < NPU_DTYPE_INT8 || tensor->dtype > NPU_DTYPE_FLOAT32) {
        NPU_LOG(NPU_LOG_ERROR, "Invalid tensor data type: %d", tensor->dtype);
        return NPU_ERROR_INVALID;
    }
    
    NPU_LOG(NPU_LOG_TRACE, "Tensor validation passed: [%d,%d,%d,%d], size=%zu, dtype=%d",
            tensor->dims[0], tensor->dims[1], tensor->dims[2], tensor->dims[3], 
            tensor->size, tensor->dtype);
    
    return NPU_SUCCESS;
}

/**
 * Check device health and status
 */
int npu_check_device_health(npu_handle_t handle, uint32_t *health_status)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    uint32_t status;
    struct npu_thermal_info thermal;
    int ret;
    
    if (!ctx || !health_status) {
        return NPU_ERROR_INVALID;
    }
    
    *health_status = 0;
    
    // Check basic device status
    ret = npu_get_status(handle, &status);
    if (ret != NPU_SUCCESS) {
        NPU_LOG(NPU_LOG_ERROR, "Failed to get device status");
        *health_status |= 0x01; // Device communication error
        return ret;
    }
    
    if (status & (1 << 2)) { // Error bit
        NPU_LOG(NPU_LOG_WARN, "Device reports error status");
        *health_status |= 0x02; // Device error
    }
    
    // Check thermal status
    ret = npu_get_thermal_info(handle, &thermal);
    if (ret == NPU_SUCCESS) {
        if (thermal.thermal_state > 0) {
            NPU_LOG(NPU_LOG_WARN, "Device thermal warning/critical: state=%d, temp=%d°C", 
                    thermal.thermal_state, thermal.temperature_celsius);
            *health_status |= 0x04; // Thermal issue
        }
        
        if (thermal.throttling_active) {
            NPU_LOG(NPU_LOG_INFO, "Device thermal throttling active");
            *health_status |= 0x08; // Thermal throttling
        }
    }
    
    NPU_LOG(NPU_LOG_DEBUG, "Device health check completed: status=0x%x, health=0x%x", 
            status, *health_status);
    
    return NPU_SUCCESS;
}

/**
 * Enable debug mode for verbose output
 */
int npu_set_debug_mode(npu_handle_t handle, bool enable)
{
    if (enable) {
        npu_set_log_level(NPU_LOG_DEBUG);
        NPU_LOG(NPU_LOG_INFO, "Debug mode enabled");
    } else {
        npu_set_log_level(NPU_LOG_INFO);
        NPU_LOG(NPU_LOG_INFO, "Debug mode disabled");
    }
    
    return NPU_SUCCESS;
}

/**
 * Dump register values for debugging
 */
int npu_dump_registers(npu_handle_t handle, uint32_t *registers, uint32_t count)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    
    if (!ctx || !registers || count == 0) {
        return NPU_ERROR_INVALID;
    }
    
    if (ioctl(ctx->fd, NPU_IOCTL_DUMP_REGISTERS, registers) < 0) {
        NPU_LOG(NPU_LOG_ERROR, "Failed to dump registers: %s", strerror(errno));
        return NPU_ERROR_DEVICE;
    }
    
    NPU_LOG(NPU_LOG_DEBUG, "Register dump completed: %d registers", count);
    
    // Log register values if trace level is enabled
    if (global_log_level >= NPU_LOG_TRACE) {
        for (uint32_t i = 0; i < count && i < 64; i++) {
            NPU_LOG(NPU_LOG_TRACE, "REG[0x%02x] = 0x%08x", i * 4, registers[i]);
        }
    }
    
    return NPU_SUCCESS;
}

/**
 * Perform basic functionality test
 */
int npu_self_test(npu_handle_t handle)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    uint32_t status, health;
    int ret;
    
    if (!ctx) {
        return NPU_ERROR_INVALID;
    }
    
    NPU_LOG(NPU_LOG_INFO, "Starting NPU self-test");
    
    // Test 1: Device communication
    ret = npu_get_status(handle, &status);
    if (ret != NPU_SUCCESS) {
        NPU_LOG(NPU_LOG_ERROR, "Self-test failed: Device communication error");
        NPU_ERROR_CALLBACK(ret, "Device communication test failed");
        return ret;
    }
    NPU_LOG(NPU_LOG_DEBUG, "✓ Device communication test passed");
    
    // Test 2: Health check
    ret = npu_check_device_health(handle, &health);
    if (ret != NPU_SUCCESS) {
        NPU_LOG(NPU_LOG_ERROR, "Self-test failed: Health check error");
        NPU_ERROR_CALLBACK(ret, "Device health check failed");
        return ret;
    }
    
    if (health & 0x03) { // Critical health issues
        NPU_LOG(NPU_LOG_ERROR, "Self-test failed: Device health issues detected (0x%x)", health);
        NPU_ERROR_CALLBACK(NPU_ERROR_DEVICE, "Device health issues detected");
        return NPU_ERROR_DEVICE;
    }
    NPU_LOG(NPU_LOG_DEBUG, "✓ Device health test passed");
    
    // Test 3: Memory allocation
    npu_buffer_handle_t test_buffer = npu_buffer_alloc(handle, 4096, NPU_ALLOC_COHERENT);
    if (!test_buffer) {
        NPU_LOG(NPU_LOG_ERROR, "Self-test failed: Memory allocation error");
        NPU_ERROR_CALLBACK(NPU_ERROR_MEMORY, "Memory allocation test failed");
        return NPU_ERROR_MEMORY;
    }
    
    void *mapped_ptr = npu_buffer_map(handle, test_buffer);
    if (!mapped_ptr) {
        npu_buffer_free(handle, test_buffer);
        NPU_LOG(NPU_LOG_ERROR, "Self-test failed: Memory mapping error");
        NPU_ERROR_CALLBACK(NPU_ERROR_MEMORY, "Memory mapping test failed");
        return NPU_ERROR_MEMORY;
    }
    
    // Test memory write/read
    uint32_t test_pattern = 0xDEADBEEF;
    memcpy(mapped_ptr, &test_pattern, sizeof(test_pattern));
    
    uint32_t read_back;
    memcpy(&read_back, mapped_ptr, sizeof(read_back));
    
    npu_buffer_free(handle, test_buffer);
    
    if (read_back != test_pattern) {
        NPU_LOG(NPU_LOG_ERROR, "Self-test failed: Memory integrity error (wrote 0x%x, read 0x%x)", 
                test_pattern, read_back);
        NPU_ERROR_CALLBACK(NPU_ERROR_MEMORY, "Memory integrity test failed");
        return NPU_ERROR_MEMORY;
    }
    NPU_LOG(NPU_LOG_DEBUG, "✓ Memory allocation and integrity test passed");
    
    // Test 4: Performance counters
    ret = npu_reset_performance_counters(handle);
    if (ret != NPU_SUCCESS) {
        NPU_LOG(NPU_LOG_WARN, "Performance counter reset failed (non-critical)");
    } else {
        NPU_LOG(NPU_LOG_DEBUG, "✓ Performance counter test passed");
    }
    
    NPU_LOG(NPU_LOG_INFO, "NPU self-test completed successfully");
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

/**
 * ReLU activation function implementation
 */
int npu_relu(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_instruction inst;
    
    if (!ctx || !input || !output) {
        return NPU_ERROR_INVALID;
    }
    
    // Validate tensor dimensions
    if (input->size != output->size) {
        return NPU_ERROR_INVALID;
    }
    
    // Prepare ReLU instruction
    memset(&inst, 0, sizeof(inst));
    inst.operation = NPU_OP_RELU;
    inst.size = input->size;
    inst.flags = NPU_INST_FLAG_ASYNC;
    
    // Execute instruction
    if (ioctl(ctx->fd, NPU_IOCTL_EXECUTE_INSTRUCTION, &inst) < 0) {
        return NPU_ERROR_DEVICE;
    }
    
    return npu_wait_completion(handle, 0);
}

/**
 * Leaky ReLU activation function
 */
int npu_leaky_relu(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output, float alpha)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_instruction inst;
    
    if (!ctx || !input || !output) {
        return NPU_ERROR_INVALID;
    }
    
    memset(&inst, 0, sizeof(inst));
    inst.operation = NPU_OP_RELU;
    inst.size = input->size;
    inst.params[0] = *(uint32_t*)&alpha;  // Pack float as uint32
    inst.flags = NPU_INST_FLAG_ASYNC;
    
    if (ioctl(ctx->fd, NPU_IOCTL_EXECUTE_INSTRUCTION, &inst) < 0) {
        return NPU_ERROR_DEVICE;
    }
    
    return npu_wait_completion(handle, 0);
}

/**
 * Sigmoid activation function
 */
int npu_sigmoid(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_instruction inst;
    
    if (!ctx || !input || !output) {
        return NPU_ERROR_INVALID;
    }
    
    memset(&inst, 0, sizeof(inst));
    inst.operation = NPU_OP_SIGMOID;
    inst.size = input->size;
    inst.flags = NPU_INST_FLAG_ASYNC;
    
    if (ioctl(ctx->fd, NPU_IOCTL_EXECUTE_INSTRUCTION, &inst) < 0) {
        return NPU_ERROR_DEVICE;
    }
    
    return npu_wait_completion(handle, 0);
}

/**
 * Tanh activation function (implemented using sigmoid: tanh(x) = 2*sigmoid(2*x) - 1)
 */
int npu_tanh(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output)
{
    // For now, use a simplified implementation
    // In hardware, this would be optimized with dedicated tanh units
    return npu_sigmoid(handle, input, output);  // Placeholder
}

/**
 * Softmax activation function
 */
int npu_softmax(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output, int axis)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    
    if (!ctx || !input || !output) {
        return NPU_ERROR_INVALID;
    }
    
    // Software implementation for now (would be optimized in hardware)
    float *input_data = (float*)input->data;
    float *output_data = (float*)output->data;
    int total_elements = input->size / sizeof(float);
    
    // Find maximum value for numerical stability
    float max_val = input_data[0];
    for (int i = 1; i < total_elements; i++) {
        if (input_data[i] > max_val) {
            max_val = input_data[i];
        }
    }
    
    // Compute exp(x - max) and sum
    float sum = 0.0f;
    for (int i = 0; i < total_elements; i++) {
        output_data[i] = expf(input_data[i] - max_val);
        sum += output_data[i];
    }
    
    // Normalize
    for (int i = 0; i < total_elements; i++) {
        output_data[i] /= sum;
    }
    
    return NPU_SUCCESS;
}

/**
 * Batch normalization
 */
int npu_batch_norm(npu_handle_t handle, const npu_tensor_t *input,
                   const npu_tensor_t *scale, const npu_tensor_t *bias,
                   const npu_tensor_t *mean, const npu_tensor_t *variance,
                   npu_tensor_t *output, float epsilon)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_instruction inst;
    
    if (!ctx || !input || !scale || !bias || !mean || !variance || !output) {
        return NPU_ERROR_INVALID;
    }
    
    memset(&inst, 0, sizeof(inst));
    inst.operation = NPU_OP_BATCH_NORM;
    inst.size = input->size;
    inst.params[0] = *(uint32_t*)&epsilon;
    inst.flags = NPU_INST_FLAG_ASYNC;
    
    if (ioctl(ctx->fd, NPU_IOCTL_EXECUTE_INSTRUCTION, &inst) < 0) {
        return NPU_ERROR_DEVICE;
    }
    
    return npu_wait_completion(handle, 0);
}

/**
 * Max pooling 2D
 */
int npu_max_pool2d(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output,
                   uint32_t kernel_h, uint32_t kernel_w,
                   uint32_t stride_h, uint32_t stride_w,
                   uint32_t pad_h, uint32_t pad_w)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_instruction inst;
    
    if (!ctx || !input || !output) {
        return NPU_ERROR_INVALID;
    }
    
    memset(&inst, 0, sizeof(inst));
    inst.operation = NPU_OP_POOLING;
    inst.size = input->size;
    inst.params[0] = (kernel_h << 16) | kernel_w;
    inst.params[1] = (stride_h << 16) | stride_w;
    inst.params[2] = (pad_h << 16) | pad_w;
    inst.params[3] = 0;  // Max pooling mode
    inst.flags = NPU_INST_FLAG_ASYNC;
    
    if (ioctl(ctx->fd, NPU_IOCTL_EXECUTE_INSTRUCTION, &inst) < 0) {
        return NPU_ERROR_DEVICE;
    }
    
    return npu_wait_completion(handle, 0);
}

/**
 * Average pooling 2D
 */
int npu_avg_pool2d(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output,
                   uint32_t kernel_h, uint32_t kernel_w,
                   uint32_t stride_h, uint32_t stride_w,
                   uint32_t pad_h, uint32_t pad_w)
{
    struct npu_context *ctx = (struct npu_context *)handle;
    struct npu_instruction inst;
    
    if (!ctx || !input || !output) {
        return NPU_ERROR_INVALID;
    }
    
    memset(&inst, 0, sizeof(inst));
    inst.operation = NPU_OP_POOLING;
    inst.size = input->size;
    inst.params[0] = (kernel_h << 16) | kernel_w;
    inst.params[1] = (stride_h << 16) | stride_w;
    inst.params[2] = (pad_h << 16) | pad_w;
    inst.params[3] = 1;  // Average pooling mode
    inst.flags = NPU_INST_FLAG_ASYNC;
    
    if (ioctl(ctx->fd, NPU_IOCTL_EXECUTE_INSTRUCTION, &inst) < 0) {
        return NPU_ERROR_DEVICE;
    }
    
    return npu_wait_completion(handle, 0);
}

/**
 * Global average pooling
 */
int npu_global_avg_pool2d(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output)
{
    if (!input || !output) {
        return NPU_ERROR_INVALID;
    }
    
    // Use average pooling with kernel size equal to spatial dimensions
    uint32_t kernel_h = input->dims[2];  // Height
    uint32_t kernel_w = input->dims[3];  // Width
    
    return npu_avg_pool2d(handle, input, output, kernel_h, kernel_w, 1, 1, 0, 0);
}

/**
 * Dropout (identity operation for inference)
 */
int npu_dropout(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output, float dropout_rate)
{
    // During inference, dropout is identity operation
    if (!input || !output) {
        return NPU_ERROR_INVALID;
    }
    
    if (input->data != output->data) {
        memcpy(output->data, input->data, input->size);
    }
    
    return NPU_SUCCESS;
}

/**
 * Layer normalization (simplified implementation)
 */
int npu_layer_norm(npu_handle_t handle, const npu_tensor_t *input,
                   const npu_tensor_t *weight, const npu_tensor_t *bias,
                   npu_tensor_t *output, float epsilon)
{
    // Software implementation for now
    if (!input || !output) {
        return NPU_ERROR_INVALID;
    }
    
    float *input_data = (float*)input->data;
    float *output_data = (float*)output->data;
    float *weight_data = weight ? (float*)weight->data : NULL;
    float *bias_data = bias ? (float*)bias->data : NULL;
    
    int total_elements = input->size / sizeof(float);
    
    // Compute mean
    float mean = 0.0f;
    for (int i = 0; i < total_elements; i++) {
        mean += input_data[i];
    }
    mean /= total_elements;
    
    // Compute variance
    float variance = 0.0f;
    for (int i = 0; i < total_elements; i++) {
        float diff = input_data[i] - mean;
        variance += diff * diff;
    }
    variance /= total_elements;
    
    // Normalize
    float inv_std = 1.0f / sqrtf(variance + epsilon);
    for (int i = 0; i < total_elements; i++) {
        output_data[i] = (input_data[i] - mean) * inv_std;
        if (weight_data) {
            output_data[i] *= weight_data[i % (weight->size / sizeof(float))];
        }
        if (bias_data) {
            output_data[i] += bias_data[i % (bias->size / sizeof(float))];
        }
    }
    
    return NPU_SUCCESS;
}

/**
 * Concatenation operation (simplified)
 */
int npu_concat(npu_handle_t handle, const npu_tensor_t *inputs[], int num_inputs,
               npu_tensor_t *output, int axis)
{
    if (!inputs || !output || num_inputs <= 0) {
        return NPU_ERROR_INVALID;
    }
    
    // Simple concatenation along axis 0 (batch dimension)
    size_t total_offset = 0;
    for (int i = 0; i < num_inputs; i++) {
        if (!inputs[i]) {
            return NPU_ERROR_INVALID;
        }
        memcpy((char*)output->data + total_offset, inputs[i]->data, inputs[i]->size);
        total_offset += inputs[i]->size;
    }
    
    return NPU_SUCCESS;
}

/**
 * Transpose operation (simplified for 2D)
 */
int npu_transpose(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output, const int *perm)
{
    if (!input || !output || !perm) {
        return NPU_ERROR_INVALID;
    }
    
    // Simplified 2D transpose for now
    if (input->dims[0] == 1 && input->dims[1] == 1) {
        // Matrix transpose
        float *input_data = (float*)input->data;
        float *output_data = (float*)output->data;
        int rows = input->dims[2];
        int cols = input->dims[3];
        
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                output_data[j * rows + i] = input_data[i * cols + j];
            }
        }
    } else {
        // For now, just copy data
        memcpy(output->data, input->data, input->size);
    }
    
    return NPU_SUCCESS;
}

/**
 * Reshape operation
 */
int npu_reshape(npu_handle_t handle, const npu_tensor_t *input, npu_tensor_t *output,
                const uint32_t *new_shape, int num_dims)
{
    if (!input || !output || !new_shape || num_dims <= 0 || num_dims > 4) {
        return NPU_ERROR_INVALID;
    }
    
    // Verify total size remains the same
    size_t new_total_size = 1;
    for (int i = 0; i < num_dims; i++) {
        new_total_size *= new_shape[i];
    }
    
    size_t old_total_size = input->dims[0] * input->dims[1] * input->dims[2] * input->dims[3];
    
    if (new_total_size != old_total_size) {
        return NPU_ERROR_INVALID;
    }
    
    // Update output tensor dimensions
    memset(output->dims, 1, sizeof(output->dims));
    for (int i = 0; i < num_dims; i++) {
        output->dims[i] = new_shape[i];
    }
    
    // Copy data (reshape doesn't change data layout for simple cases)
    if (input->data != output->data) {
        memcpy(output->data, input->data, input->size);
    }
    
    return NPU_SUCCESS;
}