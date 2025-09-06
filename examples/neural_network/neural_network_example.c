/**
 * Neural Network Training Example Application
 * 
 * Demonstrates neural network training using the NPU with backpropagation
 * Implements a simple multi-layer perceptron for binary classification
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "fpga_npu_lib.h"

// Network configuration
#define INPUT_SIZE 2      // 2D input for XOR problem
#define HIDDEN_SIZE 4     // Hidden layer size
#define OUTPUT_SIZE 1     // Single output for binary classification
#define TRAINING_SAMPLES 4 // XOR training data size
#define MAX_EPOCHS 1000
#define LEARNING_RATE 0.1f
#define TARGET_ACCURACY 0.95f

// Training data structure
typedef struct {
    float inputs[TRAINING_SAMPLES][INPUT_SIZE];
    float targets[TRAINING_SAMPLES][OUTPUT_SIZE];
    int num_samples;
} training_data_t;

// Neural network layer structure
typedef struct {
    float *weights;
    float *biases;
    float *weight_gradients;
    float *bias_gradients;
    float *activations;
    float *deltas;
    int input_size;
    int output_size;
} nn_layer_t;

// Neural network structure
typedef struct {
    nn_layer_t hidden_layer;
    nn_layer_t output_layer;
    float learning_rate;
    int epoch;
    float loss;
    float accuracy;
} neural_network_t;

// Activation functions
float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

float sigmoid_derivative(float x) {
    float s = sigmoid(x);
    return s * (1.0f - s);
}

float relu(float x) {
    return fmaxf(0.0f, x);
}

float relu_derivative(float x) {
    return (x > 0.0f) ? 1.0f : 0.0f;
}

// Initialize layer
void init_layer(nn_layer_t *layer, int input_size, int output_size, npu_handle_t npu) {
    layer->input_size = input_size;
    layer->output_size = output_size;
    
    // Allocate memory on NPU
    layer->weights = npu_malloc(npu, input_size * output_size * sizeof(float));
    layer->biases = npu_malloc(npu, output_size * sizeof(float));
    layer->weight_gradients = npu_malloc(npu, input_size * output_size * sizeof(float));
    layer->bias_gradients = npu_malloc(npu, output_size * sizeof(float));
    layer->activations = npu_malloc(npu, output_size * sizeof(float));
    layer->deltas = npu_malloc(npu, output_size * sizeof(float));
    
    if (!layer->weights || !layer->biases || !layer->weight_gradients ||
        !layer->bias_gradients || !layer->activations || !layer->deltas) {
        printf("Failed to allocate memory for neural network layer\n");
        return;
    }
    
    // Initialize weights with Xavier initialization
    float weight_scale = sqrtf(2.0f / (input_size + output_size));
    srand(time(NULL));
    
    for (int i = 0; i < input_size * output_size; i++) {
        layer->weights[i] = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * weight_scale;
    }
    
    // Initialize biases to zero
    memset(layer->biases, 0, output_size * sizeof(float));
    
    // Clear gradients
    memset(layer->weight_gradients, 0, input_size * output_size * sizeof(float));
    memset(layer->bias_gradients, 0, output_size * sizeof(float));
}

// Cleanup layer
void cleanup_layer(nn_layer_t *layer, npu_handle_t npu) {
    if (layer->weights) npu_free(npu, layer->weights);
    if (layer->biases) npu_free(npu, layer->biases);
    if (layer->weight_gradients) npu_free(npu, layer->weight_gradients);
    if (layer->bias_gradients) npu_free(npu, layer->bias_gradients);
    if (layer->activations) npu_free(npu, layer->activations);
    if (layer->deltas) npu_free(npu, layer->deltas);
}

// Initialize neural network
neural_network_t* create_neural_network(npu_handle_t npu, float learning_rate) {
    neural_network_t *nn = malloc(sizeof(neural_network_t));
    if (!nn) {
        printf("Failed to allocate neural network\n");
        return NULL;
    }
    
    init_layer(&nn->hidden_layer, INPUT_SIZE, HIDDEN_SIZE, npu);
    init_layer(&nn->output_layer, HIDDEN_SIZE, OUTPUT_SIZE, npu);
    
    nn->learning_rate = learning_rate;
    nn->epoch = 0;
    nn->loss = 0.0f;
    nn->accuracy = 0.0f;
    
    printf("Neural network created:\n");
    printf("  Input size: %d\n", INPUT_SIZE);
    printf("  Hidden size: %d\n", HIDDEN_SIZE);
    printf("  Output size: %d\n", OUTPUT_SIZE);
    printf("  Learning rate: %.3f\n", learning_rate);
    
    return nn;
}

// Cleanup neural network
void destroy_neural_network(neural_network_t *nn, npu_handle_t npu) {
    if (!nn) return;
    
    cleanup_layer(&nn->hidden_layer, npu);
    cleanup_layer(&nn->output_layer, npu);
    free(nn);
}

// Create XOR training data
training_data_t* create_xor_data(void) {
    training_data_t *data = malloc(sizeof(training_data_t));
    if (!data) return NULL;
    
    // XOR truth table
    float inputs[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    float targets[4][1] = {{0}, {1}, {1}, {0}};
    
    data->num_samples = TRAINING_SAMPLES;
    
    for (int i = 0; i < TRAINING_SAMPLES; i++) {
        for (int j = 0; j < INPUT_SIZE; j++) {
            data->inputs[i][j] = inputs[i][j];
        }
        for (int j = 0; j < OUTPUT_SIZE; j++) {
            data->targets[i][j] = targets[i][j];
        }
    }
    
    printf("XOR training data created:\n");
    for (int i = 0; i < TRAINING_SAMPLES; i++) {
        printf("  Input: [%.0f, %.0f] -> Target: %.0f\n", 
               data->inputs[i][0], data->inputs[i][1], data->targets[i][0]);
    }
    
    return data;
}

// Forward pass through the network
npu_result_t forward_pass(npu_handle_t npu, neural_network_t *nn, const float *input) {
    npu_result_t result;
    
    // Hidden layer forward pass
    result = npu_fully_connected(npu, input, nn->hidden_layer.weights,
                                nn->hidden_layer.biases, nn->hidden_layer.activations,
                                nn->hidden_layer.input_size, nn->hidden_layer.output_size);
    if (result != NPU_SUCCESS) {
        printf("Hidden layer forward pass failed: %d\n", result);
        return result;
    }
    
    // Apply sigmoid activation to hidden layer
    result = npu_sigmoid(npu, nn->hidden_layer.activations, nn->hidden_layer.activations,
                        nn->hidden_layer.output_size);
    if (result != NPU_SUCCESS) {
        printf("Hidden layer activation failed: %d\n", result);
        return result;
    }
    
    // Output layer forward pass
    result = npu_fully_connected(npu, nn->hidden_layer.activations, nn->output_layer.weights,
                                nn->output_layer.biases, nn->output_layer.activations,
                                nn->output_layer.input_size, nn->output_layer.output_size);
    if (result != NPU_SUCCESS) {
        printf("Output layer forward pass failed: %d\n", result);
        return result;
    }
    
    // Apply sigmoid activation to output layer
    result = npu_sigmoid(npu, nn->output_layer.activations, nn->output_layer.activations,
                        nn->output_layer.output_size);
    if (result != NPU_SUCCESS) {
        printf("Output layer activation failed: %d\n", result);
        return result;
    }
    
    return NPU_SUCCESS;
}

// Backward pass (backpropagation)
npu_result_t backward_pass(npu_handle_t npu, neural_network_t *nn, 
                          const float *input, const float *target) {
    npu_result_t result;
    
    // Calculate output layer deltas (error * derivative)
    for (int i = 0; i < nn->output_layer.output_size; i++) {
        float output = nn->output_layer.activations[i];
        float error = target[i] - output;
        nn->output_layer.deltas[i] = error * sigmoid_derivative(output);
    }
    
    // Calculate output layer gradients
    result = npu_compute_gradients(npu, nn->hidden_layer.activations, nn->output_layer.deltas,
                                  nn->output_layer.weight_gradients, nn->output_layer.bias_gradients,
                                  nn->output_layer.input_size, nn->output_layer.output_size);
    if (result != NPU_SUCCESS) {
        printf("Output layer gradient computation failed: %d\n", result);
        return result;
    }
    
    // Calculate hidden layer deltas (backpropagate error)
    result = npu_backpropagate_error(npu, nn->output_layer.weights, nn->output_layer.deltas,
                                    nn->hidden_layer.deltas, nn->output_layer.input_size,
                                    nn->output_layer.output_size);
    if (result != NPU_SUCCESS) {
        printf("Error backpropagation failed: %d\n", result);
        return result;
    }
    
    // Apply activation derivative to hidden layer deltas
    for (int i = 0; i < nn->hidden_layer.output_size; i++) {
        nn->hidden_layer.deltas[i] *= sigmoid_derivative(nn->hidden_layer.activations[i]);
    }
    
    // Calculate hidden layer gradients
    result = npu_compute_gradients(npu, input, nn->hidden_layer.deltas,
                                  nn->hidden_layer.weight_gradients, nn->hidden_layer.bias_gradients,
                                  nn->hidden_layer.input_size, nn->hidden_layer.output_size);
    if (result != NPU_SUCCESS) {
        printf("Hidden layer gradient computation failed: %d\n", result);
        return result;
    }
    
    return NPU_SUCCESS;
}

// Update weights using gradients
npu_result_t update_weights(npu_handle_t npu, neural_network_t *nn) {
    npu_result_t result;
    
    // Update hidden layer weights and biases
    result = npu_update_weights(npu, nn->hidden_layer.weights, nn->hidden_layer.weight_gradients,
                               nn->learning_rate, nn->hidden_layer.input_size * nn->hidden_layer.output_size);
    if (result != NPU_SUCCESS) {
        printf("Hidden layer weight update failed: %d\n", result);
        return result;
    }
    
    result = npu_update_weights(npu, nn->hidden_layer.biases, nn->hidden_layer.bias_gradients,
                               nn->learning_rate, nn->hidden_layer.output_size);
    if (result != NPU_SUCCESS) {
        printf("Hidden layer bias update failed: %d\n", result);
        return result;
    }
    
    // Update output layer weights and biases
    result = npu_update_weights(npu, nn->output_layer.weights, nn->output_layer.weight_gradients,
                               nn->learning_rate, nn->output_layer.input_size * nn->output_layer.output_size);
    if (result != NPU_SUCCESS) {
        printf("Output layer weight update failed: %d\n", result);
        return result;
    }
    
    result = npu_update_weights(npu, nn->output_layer.biases, nn->output_layer.bias_gradients,
                               nn->learning_rate, nn->output_layer.output_size);
    if (result != NPU_SUCCESS) {
        printf("Output layer bias update failed: %d\n", result);
        return result;
    }
    
    return NPU_SUCCESS;
}

// Calculate loss and accuracy
void calculate_metrics(neural_network_t *nn, training_data_t *data, npu_handle_t npu) {
    float total_loss = 0.0f;
    int correct_predictions = 0;
    
    for (int i = 0; i < data->num_samples; i++) {
        // Forward pass
        npu_result_t result = forward_pass(npu, nn, data->inputs[i]);
        if (result != NPU_SUCCESS) {
            printf("Forward pass failed during evaluation\n");
            return;
        }
        
        // Calculate loss (mean squared error)
        for (int j = 0; j < OUTPUT_SIZE; j++) {
            float error = data->targets[i][j] - nn->output_layer.activations[j];
            total_loss += error * error;
        }
        
        // Calculate accuracy (binary classification threshold at 0.5)
        float prediction = nn->output_layer.activations[0];
        float target = data->targets[i][0];
        
        if ((prediction > 0.5f && target > 0.5f) || (prediction <= 0.5f && target <= 0.5f)) {
            correct_predictions++;
        }
    }
    
    nn->loss = total_loss / (data->num_samples * OUTPUT_SIZE);
    nn->accuracy = (float)correct_predictions / data->num_samples;
}

// Train the neural network
int train_neural_network(npu_handle_t npu, neural_network_t *nn, training_data_t *data,
                        int max_epochs, float target_accuracy, bool verbose) {
    printf("\n=== Training Neural Network ===\n");
    printf("Max epochs: %d\n", max_epochs);
    printf("Target accuracy: %.2f\n", target_accuracy);
    printf("Learning rate: %.3f\n", nn->learning_rate);
    printf("\n");
    
    if (verbose) {
        printf("Epoch | Loss     | Accuracy | Status\n");
        printf("------|----------|----------|---------\n");
    }
    
    for (int epoch = 0; epoch < max_epochs; epoch++) {
        nn->epoch = epoch;
        
        // Train on all samples
        for (int sample = 0; sample < data->num_samples; sample++) {
            // Forward pass
            npu_result_t result = forward_pass(npu, nn, data->inputs[sample]);
            if (result != NPU_SUCCESS) {
                printf("Training failed at epoch %d, sample %d\n", epoch, sample);
                return -1;
            }
            
            // Backward pass
            result = backward_pass(npu, nn, data->inputs[sample], data->targets[sample]);
            if (result != NPU_SUCCESS) {
                printf("Backpropagation failed at epoch %d, sample %d\n", epoch, sample);
                return -1;
            }
            
            // Update weights
            result = update_weights(npu, nn);
            if (result != NPU_SUCCESS) {
                printf("Weight update failed at epoch %d, sample %d\n", epoch, sample);
                return -1;
            }
        }
        
        // Calculate metrics every 10 epochs or at the end
        if (epoch % 10 == 0 || epoch == max_epochs - 1) {
            calculate_metrics(nn, data, npu);
            
            if (verbose) {
                printf("%5d | %8.6f | %8.2f%% | ", epoch, nn->loss, nn->accuracy * 100.0f);
                if (nn->accuracy >= target_accuracy) {
                    printf("Target reached!");
                } else {
                    printf("Training...");
                }
                printf("\n");
            }
            
            // Check if target accuracy is reached
            if (nn->accuracy >= target_accuracy) {
                printf("\nTarget accuracy reached at epoch %d!\n", epoch);
                return epoch;
            }
        }
    }
    
    printf("\nTraining completed. Final accuracy: %.2f%%\n", nn->accuracy * 100.0f);
    return max_epochs;
}

// Test the trained network
void test_neural_network(npu_handle_t npu, neural_network_t *nn, training_data_t *data) {
    printf("\n=== Testing Trained Network ===\n");
    printf("Input     | Target | Output   | Prediction | Correct\n");
    printf("----------|--------|----------|------------|--------\n");
    
    for (int i = 0; i < data->num_samples; i++) {
        // Forward pass
        npu_result_t result = forward_pass(npu, nn, data->inputs[i]);
        if (result != NPU_SUCCESS) {
            printf("Test failed for sample %d\n", i);
            continue;
        }
        
        float output = nn->output_layer.activations[0];
        float target = data->targets[i][0];
        int prediction = (output > 0.5f) ? 1 : 0;
        int target_int = (target > 0.5f) ? 1 : 0;
        bool correct = (prediction == target_int);
        
        printf("[%.0f, %.0f]  |   %d    | %8.4f |     %d      | %s\n",
               data->inputs[i][0], data->inputs[i][1], target_int, output, 
               prediction, correct ? "✓" : "✗");
    }
    
    printf("\nFinal network accuracy: %.2f%%\n", nn->accuracy * 100.0f);
}

// Benchmark training performance
void benchmark_training(npu_handle_t npu, int num_runs) {
    printf("\n=== Training Performance Benchmark ===\n");
    printf("Running %d training runs...\n", num_runs);
    
    double total_time = 0.0;
    int successful_runs = 0;
    
    for (int run = 0; run < num_runs; run++) {
        // Create new network and data for each run
        neural_network_t *nn = create_neural_network(npu, LEARNING_RATE);
        training_data_t *data = create_xor_data();
        
        if (!nn || !data) {
            printf("Failed to create network/data for run %d\n", run);
            continue;
        }
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        int epochs = train_neural_network(npu, nn, data, MAX_EPOCHS, TARGET_ACCURACY, false);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        if (epochs >= 0) {
            double duration = (end.tv_sec - start.tv_sec) + 
                             (end.tv_nsec - start.tv_nsec) / 1e9;
            total_time += duration;
            successful_runs++;
            
            printf("Run %d: %d epochs, %.3f seconds, %.2f%% accuracy\n",
                   run + 1, epochs, duration, nn->accuracy * 100.0f);
        }
        
        // Cleanup
        destroy_neural_network(nn, npu);
        free(data);
    }
    
    if (successful_runs > 0) {
        printf("\nBenchmark Results:\n");
        printf("  Successful runs: %d/%d\n", successful_runs, num_runs);
        printf("  Average time: %.3f seconds\n", total_time / successful_runs);
        printf("  Total time: %.3f seconds\n", total_time);
    }
}

// Main neural network demo
int run_neural_network_demo(bool verbose, bool enable_benchmark) {
    npu_handle_t npu;
    npu_result_t result;
    
    printf("=== NPU Neural Network Training Example ===\n");
    printf("Problem: XOR function learning\n");
    printf("Architecture: %d -> %d -> %d (fully connected)\n", 
           INPUT_SIZE, HIDDEN_SIZE, OUTPUT_SIZE);
    printf("Activation: Sigmoid\n");
    printf("Loss: Mean Squared Error\n");
    printf("\n");
    
    // Initialize NPU
    printf("Initializing NPU...\n");
    result = npu_init(&npu);
    if (result != NPU_SUCCESS) {
        printf("Failed to initialize NPU: %d\n", result);
        return -1;
    }
    
    // Create neural network
    neural_network_t *nn = create_neural_network(npu, LEARNING_RATE);
    if (!nn) {
        printf("Failed to create neural network\n");
        npu_cleanup(npu);
        return -1;
    }
    
    // Create training data
    training_data_t *data = create_xor_data();
    if (!data) {
        printf("Failed to create training data\n");
        destroy_neural_network(nn, npu);
        npu_cleanup(npu);
        return -1;
    }
    
    printf("\n");
    
    // Train the network
    int epochs = train_neural_network(npu, nn, data, MAX_EPOCHS, TARGET_ACCURACY, verbose);
    if (epochs < 0) {
        printf("Training failed\n");
        free(data);
        destroy_neural_network(nn, npu);
        npu_cleanup(npu);
        return -1;
    }
    
    // Test the trained network
    test_neural_network(npu, nn, data);
    
    // Benchmark if requested
    if (enable_benchmark) {
        benchmark_training(npu, 10);
    }
    
    printf("\n✅ Neural network training example completed successfully!\n");
    
    // Cleanup
    free(data);
    destroy_neural_network(nn, npu);
    npu_cleanup(npu);
    
    return 0;
}

// Main function
int main(int argc, char *argv[]) {
    bool verbose = false;
    bool enable_benchmark = false;
    bool show_help = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "--benchmark") == 0 || strcmp(argv[i], "-b") == 0) {
            enable_benchmark = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            show_help = true;
        } else {
            printf("Unknown argument: %s\n", argv[i]);
            show_help = true;
        }
    }
    
    if (show_help) {
        printf("NPU Neural Network Training Example\n\n");
        printf("Usage: %s [OPTIONS]\n\n", argv[0]);
        printf("Options:\n");
        printf("  -v, --verbose        Enable verbose training output\n");
        printf("  -b, --benchmark      Enable performance benchmarking\n");
        printf("  -h, --help           Show this help message\n\n");
        printf("Examples:\n");
        printf("  %s                   # Train XOR network\n", argv[0]);
        printf("  %s --verbose         # Train with detailed progress\n", argv[0]);
        printf("  %s --benchmark       # Train with performance testing\n", argv[0]);
        return 0;
    }
    
    return run_neural_network_demo(verbose, enable_benchmark);
}