/**
 * Unit Tests for NPU Memory Management Functions
 * 
 * Tests buffer allocation, mapping, and memory management operations.
 */

#include "test_framework.h"
#include "../../software/userspace/fpga_npu_lib.h"
#include <sys/mman.h>

// Mock implementation of mmap() for testing
void* __wrap_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    (void)addr; (void)prot; (void)flags; (void)fd; (void)offset;
    
    if (mock_device.mmap_should_fail) {
        return MAP_FAILED;
    }
    
    // Return a valid pointer for testing
    static char mock_memory[65536]; // 64KB mock memory
    if (length <= sizeof(mock_memory)) {
        return mock_memory;
    }
    return MAP_FAILED;
}

// Mock implementation of munmap() for testing
int __wrap_munmap(void *addr, size_t length)
{
    (void)addr; (void)length;
    return 0; // Always succeed
}

/**
 * Test legacy memory allocation
 */
bool test_legacy_memory_alloc(void)
{
    TEST_CASE("legacy memory allocation");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Test successful allocation
    void* ptr = npu_alloc(handle, 1024);
    ASSERT_NOT_NULL(ptr);
    
    // Test allocation with zero size
    ptr = npu_alloc(handle, 0);
    ASSERT_NULL(ptr);
    
    // Test allocation with NULL handle
    ptr = npu_alloc(NULL, 1024);
    ASSERT_NULL(ptr);
    
    // Test free (should not crash)
    npu_free(handle, ptr);
    npu_free(NULL, ptr);
    npu_free(handle, NULL);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test managed buffer allocation
 */
bool test_buffer_allocation(void)
{
    TEST_CASE("managed buffer allocation");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Test successful buffer allocation
    npu_buffer_handle_t buffer = npu_buffer_alloc(handle, 4096, NPU_ALLOC_COHERENT);
    ASSERT_NOT_NULL(buffer);
    
    // Test allocation with zero size
    npu_buffer_handle_t invalid_buffer = npu_buffer_alloc(handle, 0, NPU_ALLOC_COHERENT);
    ASSERT_NULL(invalid_buffer);
    
    // Test allocation with NULL handle
    invalid_buffer = npu_buffer_alloc(NULL, 4096, NPU_ALLOC_COHERENT);
    ASSERT_NULL(invalid_buffer);
    
    // Test buffer deallocation
    int ret = npu_buffer_free(handle, buffer);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test deallocation with NULL parameters
    ret = npu_buffer_free(handle, NULL);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_buffer_free(NULL, buffer);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test buffer mapping and unmapping
 */
bool test_buffer_mapping(void)
{
    TEST_CASE("buffer mapping and unmapping");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Allocate buffer
    npu_buffer_handle_t buffer = npu_buffer_alloc(handle, 4096, NPU_ALLOC_COHERENT);
    ASSERT_NOT_NULL(buffer);
    
    // Test buffer mapping
    void* mapped_ptr = npu_buffer_map(handle, buffer);
    ASSERT_NOT_NULL(mapped_ptr);
    
    // Test mapping again (should return same pointer)
    void* mapped_ptr2 = npu_buffer_map(handle, buffer);
    ASSERT_EQ(mapped_ptr, mapped_ptr2);
    
    // Test mapping with NULL parameters
    void* invalid_ptr = npu_buffer_map(handle, NULL);
    ASSERT_NULL(invalid_ptr);
    
    invalid_ptr = npu_buffer_map(NULL, buffer);
    ASSERT_NULL(invalid_ptr);
    
    // Test buffer unmapping
    int ret = npu_buffer_unmap(handle, buffer, mapped_ptr);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test unmapping with NULL parameters
    ret = npu_buffer_unmap(handle, NULL, mapped_ptr);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_buffer_unmap(handle, buffer, NULL);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_buffer_free(handle, buffer);
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test buffer read/write operations
 */
bool test_buffer_readwrite(void)
{
    TEST_CASE("buffer read/write operations");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Allocate and map buffer
    npu_buffer_handle_t buffer = npu_buffer_alloc(handle, 4096, NPU_ALLOC_COHERENT);
    ASSERT_NOT_NULL(buffer);
    
    // Test data
    uint32_t test_data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint32_t read_data[10] = {0};
    
    // Test buffer write
    int ret = npu_buffer_write(handle, buffer, 0, test_data, sizeof(test_data));
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test buffer read
    ret = npu_buffer_read(handle, buffer, 0, read_data, sizeof(read_data));
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Verify data integrity
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(test_data[i], read_data[i]);
    }
    
    // Test write with NULL parameters
    ret = npu_buffer_write(handle, NULL, 0, test_data, sizeof(test_data));
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_buffer_write(handle, buffer, 0, NULL, sizeof(test_data));
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    // Test read with NULL parameters
    ret = npu_buffer_read(handle, NULL, 0, read_data, sizeof(read_data));
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_buffer_read(handle, buffer, 0, NULL, sizeof(read_data));
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    // Test out-of-bounds access
    ret = npu_buffer_write(handle, buffer, 4000, test_data, sizeof(test_data));
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_buffer_read(handle, buffer, 4000, read_data, sizeof(read_data));
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_buffer_free(handle, buffer);
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test buffer synchronization
 */
bool test_buffer_sync(void)
{
    TEST_CASE("buffer synchronization");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Allocate buffer
    npu_buffer_handle_t buffer = npu_buffer_alloc(handle, 4096, NPU_ALLOC_COHERENT);
    ASSERT_NOT_NULL(buffer);
    
    // Test buffer sync (to device)
    int ret = npu_buffer_sync(handle, buffer, 0);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test buffer sync (from device)
    ret = npu_buffer_sync(handle, buffer, 1);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    // Test sync with NULL parameters
    ret = npu_buffer_sync(handle, NULL, 0);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_buffer_sync(NULL, buffer, 0);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_buffer_free(handle, buffer);
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test buffer information retrieval
 */
bool test_buffer_info(void)
{
    TEST_CASE("buffer information retrieval");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Allocate buffer
    npu_buffer_handle_t buffer = npu_buffer_alloc(handle, 8192, NPU_ALLOC_STREAMING);
    ASSERT_NOT_NULL(buffer);
    
    // Test get buffer info
    struct npu_dma_buffer info;
    int ret = npu_buffer_get_info(handle, buffer, &info);
    ASSERT_EQ(NPU_SUCCESS, ret);
    ASSERT_EQ(info.size, 8192);
    ASSERT_EQ(info.flags, NPU_ALLOC_STREAMING);
    
    // Test with NULL parameters
    ret = npu_buffer_get_info(handle, NULL, &info);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_buffer_get_info(handle, buffer, NULL);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    ret = npu_buffer_get_info(NULL, buffer, &info);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    npu_buffer_free(handle, buffer);
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test tensor creation from buffer
 */
bool test_tensor_from_buffer(void)
{
    TEST_CASE("tensor creation from buffer");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Allocate buffer large enough for tensor
    npu_buffer_handle_t buffer = npu_buffer_alloc(handle, 4096, NPU_ALLOC_COHERENT);
    ASSERT_NOT_NULL(buffer);
    
    // Map buffer
    void* mapped_ptr = npu_buffer_map(handle, buffer);
    ASSERT_NOT_NULL(mapped_ptr);
    
    // Create tensor from buffer
    npu_tensor_t tensor = npu_tensor_from_buffer(buffer, 0, 1, 3, 4, 4, NPU_DTYPE_FLOAT32);
    
    // Verify tensor properties
    ASSERT_EQ(tensor.dims[0], 1);
    ASSERT_EQ(tensor.dims[1], 3);
    ASSERT_EQ(tensor.dims[2], 4);
    ASSERT_EQ(tensor.dims[3], 4);
    ASSERT_EQ(tensor.dtype, NPU_DTYPE_FLOAT32);
    ASSERT_EQ(tensor.size, 1 * 3 * 4 * 4 * sizeof(float));
    
    // Test with offset
    npu_tensor_t tensor_offset = npu_tensor_from_buffer(buffer, 256, 1, 1, 2, 2, NPU_DTYPE_INT32);
    ASSERT_EQ(tensor_offset.size, 1 * 1 * 2 * 2 * sizeof(int32_t));
    
    // Test with invalid buffer
    npu_tensor_t invalid_tensor = npu_tensor_from_buffer(NULL, 0, 1, 1, 1, 1, NPU_DTYPE_FLOAT32);
    ASSERT_EQ(invalid_tensor.size, 0);
    
    // Test with out-of-bounds tensor
    npu_tensor_t oob_tensor = npu_tensor_from_buffer(buffer, 0, 10, 10, 10, 10, NPU_DTYPE_FLOAT32);
    ASSERT_EQ(oob_tensor.size, 0);
    
    npu_buffer_free(handle, buffer);
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Test memory statistics
 */
bool test_memory_stats(void)
{
    TEST_CASE("memory statistics");
    
    mock_reset();
    npu_handle_t handle = npu_init();
    ASSERT_NOT_NULL(handle);
    
    // Get initial stats
    size_t total_allocated, total_free;
    uint32_t buffer_count;
    int ret = npu_get_memory_stats(handle, &total_allocated, &total_free, &buffer_count);
    ASSERT_EQ(NPU_SUCCESS, ret);
    ASSERT_EQ(total_allocated, 0);
    ASSERT_EQ(buffer_count, 0);
    
    // Allocate some buffers
    npu_buffer_handle_t buffer1 = npu_buffer_alloc(handle, 4096, NPU_ALLOC_COHERENT);
    npu_buffer_handle_t buffer2 = npu_buffer_alloc(handle, 8192, NPU_ALLOC_STREAMING);
    ASSERT_NOT_NULL(buffer1);
    ASSERT_NOT_NULL(buffer2);
    
    // Check updated stats
    ret = npu_get_memory_stats(handle, &total_allocated, &total_free, &buffer_count);
    ASSERT_EQ(NPU_SUCCESS, ret);
    ASSERT_EQ(total_allocated, 4096 + 8192);
    ASSERT_EQ(buffer_count, 2);
    
    // Test with NULL parameters (should not crash)
    ret = npu_get_memory_stats(handle, NULL, NULL, NULL);
    ASSERT_EQ(NPU_SUCCESS, ret);
    
    ret = npu_get_memory_stats(NULL, &total_allocated, &total_free, &buffer_count);
    ASSERT_EQ(NPU_ERROR_INVALID, ret);
    
    // Free buffers
    npu_buffer_free(handle, buffer1);
    npu_buffer_free(handle, buffer2);
    
    // Check final stats
    ret = npu_get_memory_stats(handle, &total_allocated, &total_free, &buffer_count);
    ASSERT_EQ(NPU_SUCCESS, ret);
    ASSERT_EQ(total_allocated, 0);
    ASSERT_EQ(buffer_count, 0);
    
    npu_cleanup(handle);
    TEST_PASS();
}

/**
 * Run all memory management tests
 */
void run_memory_tests(void)
{
    TEST_SUITE("Memory Management");
    
    RUN_TEST(test_legacy_memory_alloc);
    RUN_TEST(test_buffer_allocation);
    RUN_TEST(test_buffer_mapping);
    RUN_TEST(test_buffer_readwrite);
    RUN_TEST(test_buffer_sync);
    RUN_TEST(test_buffer_info);
    RUN_TEST(test_tensor_from_buffer);
    RUN_TEST(test_memory_stats);
}