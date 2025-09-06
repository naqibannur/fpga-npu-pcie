/**
 * CNN Inference Example Application
 * 
 * Demonstrates convolutional neural network inference using the NPU
 * Implements a simple LeNet-5 style network for digit classification
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "fpga_npu_lib.h"

// Network architecture constants (LeNet-5 style)
#define INPUT_HEIGHT 28
#define INPUT_WIDTH 28
#define INPUT_CHANNELS 1

#define CONV1_FILTERS 6
#define CONV1_KERNEL_SIZE 5
#define CONV1_OUTPUT_HEIGHT (INPUT_HEIGHT - CONV1_KERNEL_SIZE + 1) // 24
#define CONV1_OUTPUT_WIDTH (INPUT_WIDTH - CONV1_KERNEL_SIZE + 1)   // 24

#define POOL1_OUTPUT_HEIGHT (CONV1_OUTPUT_HEIGHT / 2) // 12
#define POOL1_OUTPUT_WIDTH (CONV1_OUTPUT_WIDTH / 2)   // 12

#define CONV2_FILTERS 16
#define CONV2_KERNEL_SIZE 5
#define CONV2_OUTPUT_HEIGHT (POOL1_OUTPUT_HEIGHT - CONV2_KERNEL_SIZE + 1) // 8
#define CONV2_OUTPUT_WIDTH (POOL1_OUTPUT_WIDTH - CONV2_KERNEL_SIZE + 1)   // 8

#define POOL2_OUTPUT_HEIGHT (CONV2_OUTPUT_HEIGHT / 2) // 4
#define POOL2_OUTPUT_WIDTH (CONV2_OUTPUT_WIDTH / 2)   // 4

#define FC1_INPUT_SIZE (CONV2_FILTERS * POOL2_OUTPUT_HEIGHT * POOL2_OUTPUT_WIDTH) // 256
#define FC1_OUTPUT_SIZE 120

#define FC2_INPUT_SIZE FC1_OUTPUT_SIZE
#define FC2_OUTPUT_SIZE 84

#define OUTPUT_SIZE 10 // 10 digit classes

// Network layer structure
typedef struct {
    float *weights;
    float *biases;
    size_t weight_size;
    size_t bias_size;
} layer_t;

// CNN model structure
typedef struct {
    layer_t conv1;
    layer_t conv2;
    layer_t fc1;
    layer_t fc2;
    layer_t output;
    
    // Intermediate buffers
    float *conv1_output;
    float *pool1_output;
    float *conv2_output;
    float *pool2_output;
    float *fc1_output;
    float *fc2_output;
    float *final_output;
} cnn_model_t;

// Initialize layer with random weights (for demonstration)
void initialize_layer_weights(layer_t *layer, int input_size, int output_size, 
                             int kernel_size, int num_filters) {
    if (kernel_size > 0) {
        // Convolutional layer
        layer->weight_size = num_filters * input_size * kernel_size * kernel_size * sizeof(float);
        layer->bias_size = num_filters * sizeof(float);
    } else {
        // Fully connected layer
        layer->weight_size = input_size * output_size * sizeof(float);
        layer->bias_size = output_size * sizeof(float);
    }
    
    layer->weights = malloc(layer->weight_size);
    layer->biases = malloc(layer->bias_size);
    
    if (!layer->weights || !layer->biases) {
        printf("Failed to allocate memory for layer weights\n");
        return;
    }
    
    // Initialize with Xavier/Glorot initialization
    float weight_scale = sqrt(2.0f / input_size);
    int num_weights = layer->weight_size / sizeof(float);
    int num_biases = layer->bias_size / sizeof(float);
    
    srand(time(NULL));
    
    for (int i = 0; i < num_weights; i++) {
        layer->weights[i] = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * weight_scale;
    }
    
    for (int i = 0; i < num_biases; i++) {
        layer->biases[i] = 0.0f; // Initialize biases to zero
    }
}

// Create and initialize CNN model
cnn_model_t* create_cnn_model(npu_handle_t npu) {
    cnn_model_t *model = malloc(sizeof(cnn_model_t));
    if (!model) {
        printf("Failed to allocate CNN model\n");
        return NULL;
    }
    
    printf("Initializing CNN model layers...\n");
    
    // Initialize layers
    initialize_layer_weights(&model->conv1, INPUT_CHANNELS, CONV1_FILTERS, 
                           CONV1_KERNEL_SIZE, CONV1_FILTERS);
    initialize_layer_weights(&model->conv2, CONV1_FILTERS, CONV2_FILTERS, 
                           CONV2_KERNEL_SIZE, CONV2_FILTERS);
    initialize_layer_weights(&model->fc1, FC1_INPUT_SIZE, FC1_OUTPUT_SIZE, 0, 0);
    initialize_layer_weights(&model->fc2, FC2_INPUT_SIZE, FC2_OUTPUT_SIZE, 0, 0);
    initialize_layer_weights(&model->output, FC2_OUTPUT_SIZE, OUTPUT_SIZE, 0, 0);
    
    // Allocate intermediate buffers on NPU
    model->conv1_output = npu_malloc(npu, CONV1_FILTERS * CONV1_OUTPUT_HEIGHT * 
                                         CONV1_OUTPUT_WIDTH * sizeof(float));
    model->pool1_output = npu_malloc(npu, CONV1_FILTERS * POOL1_OUTPUT_HEIGHT * 
                                         POOL1_OUTPUT_WIDTH * sizeof(float));
    model->conv2_output = npu_malloc(npu, CONV2_FILTERS * CONV2_OUTPUT_HEIGHT * 
                                         CONV2_OUTPUT_WIDTH * sizeof(float));
    model->pool2_output = npu_malloc(npu, CONV2_FILTERS * POOL2_OUTPUT_HEIGHT * 
                                         POOL2_OUTPUT_WIDTH * sizeof(float));
    model->fc1_output = npu_malloc(npu, FC1_OUTPUT_SIZE * sizeof(float));
    model->fc2_output = npu_malloc(npu, FC2_OUTPUT_SIZE * sizeof(float));
    model->final_output = npu_malloc(npu, OUTPUT_SIZE * sizeof(float));
    
    if (!model->conv1_output || !model->pool1_output || !model->conv2_output ||
        !model->pool2_output || !model->fc1_output || !model->fc2_output || 
        !model->final_output) {
        printf("Failed to allocate NPU memory for intermediate buffers\n");
        // Cleanup would go here
        return NULL;
    }
    
    printf("CNN model initialized successfully\n");
    return model;
}

// Cleanup CNN model
void destroy_cnn_model(cnn_model_t *model, npu_handle_t npu) {
    if (!model) return;
    
    // Free layer weights and biases
    free(model->conv1.weights);
    free(model->conv1.biases);
    free(model->conv2.weights);
    free(model->conv2.biases);
    free(model->fc1.weights);
    free(model->fc1.biases);
    free(model->fc2.weights);
    free(model->fc2.biases);
    free(model->output.weights);
    free(model->output.biases);
    
    // Free NPU buffers
    npu_free(npu, model->conv1_output);
    npu_free(npu, model->pool1_output);
    npu_free(npu, model->conv2_output);
    npu_free(npu, model->pool2_output);
    npu_free(npu, model->fc1_output);
    npu_free(npu, model->fc2_output);
    npu_free(npu, model->final_output);
    
    free(model);
}

// Generate synthetic input data (simulated digit image)
void generate_input_image(float *input, int digit_class) {
    // Create a simple synthetic digit pattern
    memset(input, 0, INPUT_HEIGHT * INPUT_WIDTH * sizeof(float));
    
    int center_x = INPUT_WIDTH / 2;
    int center_y = INPUT_HEIGHT / 2;
    int radius = 8;
    
    for (int y = 0; y < INPUT_HEIGHT; y++) {
        for (int x = 0; x < INPUT_WIDTH; x++) {
            int dx = x - center_x;
            int dy = y - center_y;
            float distance = sqrt(dx * dx + dy * dy);
            
            // Create different patterns for different digits
            float value = 0.0f;
            switch (digit_class) {
                case 0: // Circle
                    value = (distance > radius - 2 && distance < radius + 2) ? 1.0f : 0.0f;
                    break;
                case 1: // Vertical line
                    value = (abs(dx) < 2) ? 1.0f : 0.0f;
                    break;
                case 2: // Horizontal line
                    value = (abs(dy) < 2) ? 1.0f : 0.0f;
                    break;
                default:
                    // Random pattern for other digits
                    value = (sin(x * 0.5f) * cos(y * 0.5f) > 0.3f) ? 1.0f : 0.0f;
                    break;
            }
            
            input[y * INPUT_WIDTH + x] = value;
        }
    }
}

// Print input image (for debugging)
void print_input_image(const float *input) {
    printf("Input image:\n");
    for (int y = 0; y < INPUT_HEIGHT; y++) {
        for (int x = 0; x < INPUT_WIDTH; x++) {
            printf("%c", input[y * INPUT_WIDTH + x] > 0.5f ? '#' : '.');
        }
        printf("\n");
    }
    printf("\n");
}

// Perform CNN inference
npu_result_t run_cnn_inference(npu_handle_t npu, cnn_model_t *model, 
                              const float *input, float *output) {
    npu_result_t result;
    
    // Layer 1: Convolution
    result = npu_conv2d(npu, input, model->conv1.weights, model->conv1_output,
                       INPUT_HEIGHT, INPUT_WIDTH, INPUT_CHANNELS,
                       CONV1_FILTERS, CONV1_KERNEL_SIZE, CONV1_KERNEL_SIZE,
                       1, 1, 0, 0); // stride=1, padding=0
    if (result != NPU_SUCCESS) {
        printf("Conv1 failed: %d\n", result);
        return result;
    }
    
    // Add bias and apply ReLU activation
    result = npu_add_bias(npu, model->conv1_output, model->conv1.biases, 
                         model->conv1_output,
                         CONV1_FILTERS * CONV1_OUTPUT_HEIGHT * CONV1_OUTPUT_WIDTH);
    if (result != NPU_SUCCESS) {
        printf("Conv1 bias addition failed: %d\n", result);
        return result;
    }
    
    result = npu_relu(npu, model->conv1_output, model->conv1_output,
                     CONV1_FILTERS * CONV1_OUTPUT_HEIGHT * CONV1_OUTPUT_WIDTH);
    if (result != NPU_SUCCESS) {
        printf("Conv1 ReLU failed: %d\n", result);
        return result;
    }
    
    // Layer 2: Max Pooling
    result = npu_maxpool2d(npu, model->conv1_output, model->pool1_output,
                          CONV1_OUTPUT_HEIGHT, CONV1_OUTPUT_WIDTH, CONV1_FILTERS,
                          2, 2, 2, 2); // 2x2 pool, stride=2
    if (result != NPU_SUCCESS) {
        printf("Pool1 failed: %d\n", result);
        return result;
    }
    
    // Layer 3: Convolution
    result = npu_conv2d(npu, model->pool1_output, model->conv2.weights, model->conv2_output,
                       POOL1_OUTPUT_HEIGHT, POOL1_OUTPUT_WIDTH, CONV1_FILTERS,
                       CONV2_FILTERS, CONV2_KERNEL_SIZE, CONV2_KERNEL_SIZE,
                       1, 1, 0, 0); // stride=1, padding=0
    if (result != NPU_SUCCESS) {
        printf("Conv2 failed: %d\n", result);
        return result;
    }
    
    // Add bias and apply ReLU activation
    result = npu_add_bias(npu, model->conv2_output, model->conv2.biases,
                         model->conv2_output,
                         CONV2_FILTERS * CONV2_OUTPUT_HEIGHT * CONV2_OUTPUT_WIDTH);
    if (result != NPU_SUCCESS) {
        printf("Conv2 bias addition failed: %d\n", result);
        return result;
    }
    
    result = npu_relu(npu, model->conv2_output, model->conv2_output,
                     CONV2_FILTERS * CONV2_OUTPUT_HEIGHT * CONV2_OUTPUT_WIDTH);
    if (result != NPU_SUCCESS) {
        printf("Conv2 ReLU failed: %d\n", result);
        return result;
    }
    
    // Layer 4: Max Pooling
    result = npu_maxpool2d(npu, model->conv2_output, model->pool2_output,
                          CONV2_OUTPUT_HEIGHT, CONV2_OUTPUT_WIDTH, CONV2_FILTERS,
                          2, 2, 2, 2); // 2x2 pool, stride=2
    if (result != NPU_SUCCESS) {
        printf("Pool2 failed: %d\n", result);
        return result;
    }
    
    // Layer 5: Fully Connected 1
    result = npu_fully_connected(npu, model->pool2_output, model->fc1.weights,
                                model->fc1.biases, model->fc1_output,
                                FC1_INPUT_SIZE, FC1_OUTPUT_SIZE);
    if (result != NPU_SUCCESS) {
        printf("FC1 failed: %d\n", result);
        return result;
    }
    
    result = npu_relu(npu, model->fc1_output, model->fc1_output, FC1_OUTPUT_SIZE);
    if (result != NPU_SUCCESS) {
        printf("FC1 ReLU failed: %d\n", result);
        return result;
    }
    
    // Layer 6: Fully Connected 2
    result = npu_fully_connected(npu, model->fc1_output, model->fc2.weights,
                                model->fc2.biases, model->fc2_output,
                                FC2_INPUT_SIZE, FC2_OUTPUT_SIZE);
    if (result != NPU_SUCCESS) {
        printf("FC2 failed: %d\n", result);
        return result;
    }
    
    result = npu_relu(npu, model->fc2_output, model->fc2_output, FC2_OUTPUT_SIZE);
    if (result != NPU_SUCCESS) {
        printf("FC2 ReLU failed: %d\n", result);
        return result;
    }
    
    // Output layer: Fully Connected + Softmax
    result = npu_fully_connected(npu, model->fc2_output, model->output.weights,
                                model->output.biases, model->final_output,
                                FC2_OUTPUT_SIZE, OUTPUT_SIZE);
    if (result != NPU_SUCCESS) {
        printf("Output layer failed: %d\n", result);
        return result;
    }
    
    result = npu_softmax(npu, model->final_output, output, OUTPUT_SIZE);
    if (result != NPU_SUCCESS) {
        printf("Softmax failed: %d\n", result);
        return result;
    }
    
    return NPU_SUCCESS;
}

// Find the predicted class (highest probability)
int get_predicted_class(const float *output) {
    int predicted_class = 0;
    float max_prob = output[0];
    
    for (int i = 1; i < OUTPUT_SIZE; i++) {
        if (output[i] > max_prob) {
            max_prob = output[i];
            predicted_class = i;
        }
    }
    
    return predicted_class;
}

// Print inference results
void print_inference_results(const float *output, int true_class) {
    printf("Inference Results:\n");
    printf("Class | Probability\n");
    printf("------|------------\n");
    
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        printf("  %d   | %8.4f", i, output[i]);
        if (i == true_class) {
            printf(" (true)");
        }
        printf("\n");
    }
    
    int predicted_class = get_predicted_class(output);
    printf("\nPredicted class: %d\n", predicted_class);
    printf("True class: %d\n", true_class);
    printf("Prediction: %s\n", (predicted_class == true_class) ? "CORRECT" : "INCORRECT");
}

// Benchmark inference performance
void benchmark_inference(npu_handle_t npu, cnn_model_t *model, int num_iterations) {
    printf("\n=== CNN Inference Benchmark ===\n");
    printf("Running %d inference iterations...\n", num_iterations);
    
    float *input = npu_malloc(npu, INPUT_HEIGHT * INPUT_WIDTH * sizeof(float));
    float *output = malloc(OUTPUT_SIZE * sizeof(float));
    
    if (!input || !output) {
        printf("Failed to allocate benchmark memory\n");
        return;
    }
    
    // Generate test input
    generate_input_image(input, 0);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < num_iterations; i++) {
        npu_result_t result = run_cnn_inference(npu, model, input, output);
        if (result != NPU_SUCCESS) {
            printf("Inference failed at iteration %d: %d\n", i, result);
            break;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double duration = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    
    double avg_latency = (duration * 1000.0) / num_iterations; // ms
    double throughput = num_iterations / duration; // inferences per second
    
    printf("Benchmark Results:\n");
    printf("  Total duration: %.3f seconds\n", duration);
    printf("  Average latency: %.3f ms\n", avg_latency);
    printf("  Throughput: %.2f inferences/sec\n", throughput);
    
    npu_free(npu, input);
    free(output);
}

// Main CNN demo function
int run_cnn_demo(bool show_input, bool enable_benchmark, bool verbose) {
    npu_handle_t npu;
    npu_result_t result;
    
    printf("=== NPU CNN Inference Example ===\n");
    printf("Network: LeNet-5 style CNN for digit classification\n");
    printf("Input: %dx%d grayscale image\n", INPUT_HEIGHT, INPUT_WIDTH);
    printf("Output: %d class probabilities\n", OUTPUT_SIZE);
    printf("\n");
    
    // Initialize NPU
    printf("Initializing NPU...\n");
    result = npu_init(&npu);
    if (result != NPU_SUCCESS) {
        printf("Failed to initialize NPU: %d\n", result);
        return -1;
    }
    
    // Create CNN model
    cnn_model_t *model = create_cnn_model(npu);
    if (!model) {
        printf("Failed to create CNN model\n");
        npu_cleanup(npu);
        return -1;
    }
    
    // Allocate input/output buffers
    float *input = npu_malloc(npu, INPUT_HEIGHT * INPUT_WIDTH * sizeof(float));
    float *output = malloc(OUTPUT_SIZE * sizeof(float));
    
    if (!input || !output) {
        printf("Failed to allocate input/output buffers\n");
        destroy_cnn_model(model, npu);
        npu_cleanup(npu);
        return -1;
    }
    
    // Test inference with different digit classes
    printf("Running inference tests on synthetic digit images...\n\n");
    
    for (int digit = 0; digit < 3; digit++) {
        printf("--- Testing digit class %d ---\n", digit);
        
        // Generate input image
        generate_input_image(input, digit);
        
        if (show_input) {
            print_input_image(input);
        }
        
        // Run inference
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        result = run_cnn_inference(npu, model, input, output);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        if (result != NPU_SUCCESS) {
            printf("Inference failed for digit %d: %d\n", digit, result);
            continue;
        }
        
        double inference_time = (end.tv_sec - start.tv_sec) + 
                               (end.tv_nsec - start.tv_nsec) / 1e9;
        
        printf("Inference completed in %.3f ms\n", inference_time * 1000.0);
        
        if (verbose) {
            print_inference_results(output, digit);
        } else {
            int predicted = get_predicted_class(output);
            printf("Predicted: %d, True: %d (%s)\n", predicted, digit,
                   (predicted == digit) ? "CORRECT" : "INCORRECT");
        }
        
        printf("\n");
    }
    
    // Performance benchmark
    if (enable_benchmark) {
        benchmark_inference(npu, model, 100);
    }
    
    printf("✅ CNN inference example completed successfully!\n");
    
    // Cleanup
    npu_free(npu, input);
    free(output);
    destroy_cnn_model(model, npu);
    npu_cleanup(npu);
    
    return 0;
}

// Main function
int main(int argc, char *argv[]) {
    bool show_input = false;
    bool enable_benchmark = false;
    bool verbose = false;
    bool show_help = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--show-input") == 0) {
            show_input = true;
        } else if (strcmp(argv[i], "--benchmark") == 0 || strcmp(argv[i], "-b") == 0) {
            enable_benchmark = true;
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
        printf("NPU CNN Inference Example\n\n");
        printf("Usage: %s [OPTIONS]\n\n", argv[0]);
        printf("Options:\n");
        printf("  --show-input         Display input images as ASCII art\n");
        printf("  -b, --benchmark      Enable performance benchmarking\n");
        printf("  -v, --verbose        Enable verbose output\n");
        printf("  -h, --help           Show this help message\n\n");
        printf("Examples:\n");
        printf("  %s                   # Run basic CNN inference test\n", argv[0]);
        printf("  %s --verbose         # Run with detailed output\n", argv[0]);
        printf("  %s --benchmark       # Run with performance testing\n", argv[0]);
        return 0;
    }
    
    return run_cnn_demo(show_input, enable_benchmark, verbose);
}