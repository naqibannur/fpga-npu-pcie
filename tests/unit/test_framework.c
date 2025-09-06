/**
 * Simple Unit Test Framework Implementation
 * 
 * Provides test statistics and mock device functionality.
 */

#include "test_framework.h"

// Test statistics
int test_count = 0;
int test_passed = 0;
int test_failed = 0;

// Mock device state
mock_device_t mock_device = {
    .init_should_fail = false,
    .ioctl_should_fail = false,
    .mmap_should_fail = false,
    .mock_fd = 42,
    .mock_status = 0x01, // STATUS_READY
    .mock_cycles = 1000,
    .mock_operations = 100
};

/**
 * Reset mock device to default state
 */
void mock_reset(void)
{
    mock_device.init_should_fail = false;
    mock_device.ioctl_should_fail = false;
    mock_device.mmap_should_fail = false;
    mock_device.mock_fd = 42;
    mock_device.mock_status = 0x01;
    mock_device.mock_cycles = 1000;
    mock_device.mock_operations = 100;
}

/**
 * Configure mock to fail initialization
 */
void mock_set_init_fail(bool should_fail)
{
    mock_device.init_should_fail = should_fail;
}

/**
 * Configure mock to fail ioctl calls
 */
void mock_set_ioctl_fail(bool should_fail)
{
    mock_device.ioctl_should_fail = should_fail;
}

/**
 * Set mock device status
 */
void mock_set_status(uint32_t status)
{
    mock_device.mock_status = status;
}

/**
 * Set mock performance counters
 */
void mock_set_performance_counters(uint64_t cycles, uint64_t operations)
{
    mock_device.mock_cycles = cycles;
    mock_device.mock_operations = operations;
}