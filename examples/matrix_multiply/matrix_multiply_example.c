/**
 * Matrix Multiplication Example Application
 * 
 * Demonstrates basic NPU usage with matrix multiplication operations
 * Shows initialization, memory management, and performance measurement
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "fpga_npu_lib.h"

// Configuration constants
#define DEFAULT_MATRIX_SIZE 256
#define MAX_MATRIX_SIZE 2048
#define PERFORMANCE_ITERATIONS 100

// Helper function to initialize matrix with random values
void initialize_matrix_random(float *matrix, int rows, int cols) {
    srand(time(NULL));
    for (int i = 0; i < rows * cols; i++) {
        matrix[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f; // Range [-1, 1]
    }
}

// Helper function to initialize matrix with specific pattern
void initialize_matrix_pattern(float *matrix, int rows, int cols, const char *pattern) {
    if (strcmp(pattern, "identity") == 0) {
        memset(matrix, 0, rows * cols * sizeof(float));
        for (int i = 0; i < (rows < cols ? rows : cols); i++) {
            matrix[i * cols + i] = 1.0f;
        }
    } else if (strcmp(pattern, "ones") == 0) {
        for (int i = 0; i < rows * cols; i++) {
            matrix[i] = 1.0f;
        }
    } else if (strcmp(pattern, "sequential") == 0) {
        for (int i = 0; i < rows * cols; i++) {
            matrix[i] = (float)(i + 1);
        }
    } else {
        initialize_matrix_random(matrix, rows, cols);
    }
}

// Helper function to print matrix (for small matrices)
void print_matrix(const float *matrix, int rows, int cols, const char *name) {
    if (rows > 8 || cols > 8) {
        printf("%s: %dx%d matrix (too large to display)\n", name, rows, cols);
        return;
    }
    
    printf("%s (%dx%d):\n", name, rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%8.3f ", matrix[i * cols + j]);
        }
        printf("\n");
    }
    printf("\n");
}

// CPU reference implementation for verification
void cpu_matrix_multiply(const float *A, const float *B, float *C, 
                        int M, int N, int K) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += A[i * K + k] * B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}

// Verify NPU result against CPU reference
bool verify_result(const float *npu_result, const float *cpu_result, 
                  int size, float tolerance) {
    for (int i = 0; i < size; i++) {
        float diff = fabs(npu_result[i] - cpu_result[i]);
        if (diff > tolerance) {
            printf("Verification failed at index %d: NPU=%.6f, CPU=%.6f, diff=%.6f\n",
                   i, npu_result[i], cpu_result[i], diff);
            return false;
        }
    }
    return true;
}

// Performance measurement function
double measure_performance(npu_handle_t npu, const float *A, const float *B, 
                          float *C, int M, int N, int K, int iterations) {
    struct timespec start, end;
    
    printf("Running %d iterations for performance measurement...\n", iterations);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < iterations; i++) {
        npu_result_t result = npu_matrix_multiply(npu, A, B, C, M, N, K);
        if (result != NPU_SUCCESS) {
            printf("NPU operation failed at iteration %d: %d\n", i, result);
            return -1.0;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double duration = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    
    // Calculate performance metrics
    uint64_t total_ops = (uint64_t)iterations * M * N * K * 2; // MAC operations
    double gops = (double)total_ops / (duration * 1e9);
    double avg_latency = (duration * 1000.0) / iterations; // ms
    
    printf("Performance Results:\n");
    printf("  Total duration: %.3f seconds\n", duration);
    printf("  Average latency: %.3f ms\n", avg_latency);
    printf("  Throughput: %.2f GOPS\n", gops);
    
    return gops;
}

// Demonstrate different matrix sizes and their performance
void performance_scaling_demo(npu_handle_t npu) {
    printf("\n=== Performance Scaling Demonstration ===\n");
    
    const int sizes[] = {64, 128, 256, 512, 1024};
    const int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    printf("Matrix Size | Throughput (GOPS) | Latency (ms)\n");
    printf("------------|-------------------|-------------\n");
    
    for (int i = 0; i < num_sizes; i++) {
        int size = sizes[i];
        size_t matrix_bytes = size * size * sizeof(float);
        
        // Allocate matrices
        float *A = npu_malloc(npu, matrix_bytes);
        float *B = npu_malloc(npu, matrix_bytes);
        float *C = npu_malloc(npu, matrix_bytes);
        
        if (!A || !B || !C) {
            printf("Failed to allocate memory for size %d\n", size);
            continue;
        }
        
        // Initialize matrices
        initialize_matrix_random(A, size, size);
        initialize_matrix_random(B, size, size);
        
        // Measure performance (fewer iterations for larger matrices)
        int iterations = (size <= 256) ? 50 : 10;
        double gops = measure_performance(npu, A, B, C, size, size, size, iterations);
        
        if (gops > 0) {
            double latency = 1000.0 / (gops * 1e9 / (2.0 * size * size * size));
            printf("%11d | %17.2f | %11.3f\n", size, gops, latency);
        } else {
            printf("%11d | %17s | %11s\n", size, "FAILED", "FAILED");
        }
        
        // Cleanup
        npu_free(npu, A);
        npu_free(npu, B);
        npu_free(npu, C);
    }
}

// Main demonstration function
int run_matrix_multiply_demo(int matrix_size, bool enable_verification, 
                           bool enable_performance, bool verbose) {
    npu_handle_t npu;
    npu_result_t result;
    
    printf("=== NPU Matrix Multiplication Example ===\n");
    printf("Matrix size: %dx%d\n", matrix_size, matrix_size);
    printf("Verification: %s\n", enable_verification ? "enabled" : "disabled");
    printf("Performance testing: %s\n", enable_performance ? "enabled" : "disabled");
    printf("\n");
    
    // Initialize NPU
    printf("Initializing NPU...\n");
    result = npu_init(&npu);
    if (result != NPU_SUCCESS) {
        printf("Failed to initialize NPU: %d\n", result);
        return -1;
    }
    
    // Get NPU information
    npu_device_info_t device_info;
    result = npu_get_device_info(npu, &device_info);
    if (result == NPU_SUCCESS) {
        printf("NPU Device Information:\n");
        printf("  Device ID: 0x%04x\n", device_info.device_id);
        printf("  Vendor ID: 0x%04x\n", device_info.vendor_id);
        printf("  Memory size: %zu MB\n", device_info.memory_size / (1024 * 1024));
        printf("  Max frequency: %u MHz\n", device_info.max_frequency_mhz);
        printf("  Processing elements: %u\n", device_info.num_processing_elements);
        printf("\n");
    }
    
    // Allocate matrices
    size_t matrix_bytes = matrix_size * matrix_size * sizeof(float);
    printf("Allocating matrices (%zu bytes each)...\n", matrix_bytes);
    
    float *matrix_A = npu_malloc(npu, matrix_bytes);
    float *matrix_B = npu_malloc(npu, matrix_bytes);
    float *matrix_C = npu_malloc(npu, matrix_bytes);
    
    if (!matrix_A || !matrix_B || !matrix_C) {
        printf("Failed to allocate NPU memory\n");
        npu_cleanup(npu);
        return -1;
    }
    
    // Initialize matrices
    printf("Initializing matrices...\n");
    initialize_matrix_random(matrix_A, matrix_size, matrix_size);
    initialize_matrix_random(matrix_B, matrix_size, matrix_size);
    
    if (verbose && matrix_size <= 8) {
        print_matrix(matrix_A, matrix_size, matrix_size, "Matrix A");
        print_matrix(matrix_B, matrix_size, matrix_size, "Matrix B");
    }
    
    // Perform NPU matrix multiplication
    printf("Performing NPU matrix multiplication...\n");
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    result = npu_matrix_multiply(npu, matrix_A, matrix_B, matrix_C, 
                                matrix_size, matrix_size, matrix_size);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    if (result != NPU_SUCCESS) {
        printf("NPU matrix multiplication failed: %d\n", result);
        goto cleanup;
    }
    
    double npu_time = (end.tv_sec - start.tv_sec) + 
                      (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("NPU computation completed in %.3f ms\n", npu_time * 1000.0);
    
    if (verbose && matrix_size <= 8) {
        print_matrix(matrix_C, matrix_size, matrix_size, "Result Matrix C");
    }
    
    // Verification against CPU implementation
    if (enable_verification) {
        printf("Running CPU verification...\n");
        
        float *cpu_result = malloc(matrix_bytes);
        if (!cpu_result) {
            printf("Failed to allocate CPU memory for verification\n");
            goto cleanup;
        }
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        cpu_matrix_multiply(matrix_A, matrix_B, cpu_result, 
                           matrix_size, matrix_size, matrix_size);
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        double cpu_time = (end.tv_sec - start.tv_sec) + 
                         (end.tv_nsec - start.tv_nsec) / 1e9;
        
        printf("CPU computation completed in %.3f ms\n", cpu_time * 1000.0);
        printf("NPU speedup: %.2fx\n", cpu_time / npu_time);
        
        // Verify results
        float tolerance = 1e-4f; // Tolerance for floating point comparison
        bool verification_passed = verify_result(matrix_C, cpu_result, 
                                                matrix_size * matrix_size, tolerance);
        
        if (verification_passed) {
            printf("✅ Verification PASSED - Results match within tolerance\n");
        } else {
            printf("❌ Verification FAILED - Results do not match\n");
        }
        
        free(cpu_result);
    }
    
    // Performance testing
    if (enable_performance) {
        printf("\n=== Performance Testing ===\n");
        measure_performance(npu, matrix_A, matrix_B, matrix_C, 
                          matrix_size, matrix_size, matrix_size, 
                          PERFORMANCE_ITERATIONS);
        
        // Run scaling demonstration
        performance_scaling_demo(npu);
    }
    
    printf("\n✅ Matrix multiplication example completed successfully!\n");

cleanup:
    // Cleanup resources
    if (matrix_A) npu_free(npu, matrix_A);
    if (matrix_B) npu_free(npu, matrix_B);
    if (matrix_C) npu_free(npu, matrix_C);
    npu_cleanup(npu);
    
    return (result == NPU_SUCCESS) ? 0 : -1;
}

// Main function with command line argument parsing
int main(int argc, char *argv[]) {
    int matrix_size = DEFAULT_MATRIX_SIZE;
    bool enable_verification = true;
    bool enable_performance = false;
    bool verbose = false;
    bool show_help = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--size") == 0 || strcmp(argv[i], "-s") == 0) {
            if (i + 1 < argc) {
                matrix_size = atoi(argv[++i]);
                if (matrix_size <= 0 || matrix_size > MAX_MATRIX_SIZE) {
                    printf("Invalid matrix size: %d (must be 1-%d)\n", 
                           matrix_size, MAX_MATRIX_SIZE);
                    return -1;
                }
            } else {
                printf("Error: --size requires a value\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--no-verify") == 0) {
            enable_verification = false;
        } else if (strcmp(argv[i], "--performance") == 0 || strcmp(argv[i], "-p") == 0) {
            enable_performance = true;
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            show_help = true;
        } else {
            printf("Unknown argument: %s\n", argv[i]);
            show_help = true;
        }
    }
    
    if (show_help) {
        printf("NPU Matrix Multiplication Example\n\n");
        printf("Usage: %s [OPTIONS]\n\n", argv[0]);
        printf("Options:\n");
        printf("  -s, --size SIZE      Matrix size (default: %d, max: %d)\n", 
               DEFAULT_MATRIX_SIZE, MAX_MATRIX_SIZE);
        printf("  --no-verify          Disable CPU verification\n");
        printf("  -p, --performance    Enable performance testing\n");
        printf("  -v, --verbose        Enable verbose output\n");
        printf("  -h, --help           Show this help message\n\n");
        printf("Examples:\n");
        printf("  %s                           # Run with default settings\n", argv[0]);
        printf("  %s --size 512 --performance  # 512x512 matrix with performance test\n", argv[0]);
        printf("  %s --size 64 --verbose       # Small matrix with detailed output\n", argv[0]);
        return 0;
    }
    
    return run_matrix_multiply_demo(matrix_size, enable_verification, 
                                   enable_performance, verbose);
}