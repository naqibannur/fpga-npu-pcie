# Getting Started with FPGA NPU

This tutorial will guide you through setting up and using the FPGA NPU system for the first time.

## Prerequisites

Before starting, ensure you have:

### Hardware Requirements
- Xilinx Zynq UltraScale+ development board (recommended: ZCU102)
- PCIe-capable host system
- USB cable for JTAG programming
- Adequate power supply (12V, 5A minimum)

### Software Requirements
- Linux development system (Ubuntu 18.04+ or CentOS 7+)
- Xilinx Vivado 2022.1 or later
- GCC 7.0 or later
- Linux kernel headers
- Git

## Step 1: Clone and Setup

1. Clone the repository:
```bash
git clone https://github.com/naqibannur/fpga-npu-pcie.git
cd fpga-npu-pcie
```

2. Run the setup script:
```bash
chmod +x scripts/setup.sh
./scripts/setup.sh
```

This script will check dependencies and configure the build environment.

## Step 2: Configure the Project

1. Review the configuration file:
```bash
cat config.mk
```

2. Modify settings for your target board:
```bash
# Edit config.mk
DEFAULT_BOARD = "zcu102"  # Change to your board
NPU_PE_COUNT = 16         # Adjust PE count as needed
```

## Step 3: Build the Hardware Design

1. Build the FPGA bitstream:
```bash
make hardware BOARD=zcu102
```

This process takes 30-60 minutes depending on your system.

2. Program the FPGA:
```bash
cd hardware
make program
```

## Step 4: Build the Software

1. Build the kernel driver:
```bash
make driver
```

2. Build the user-space library:
```bash
make userspace
```

3. Install both components:
```bash
sudo make install
```

## Step 5: Load the Driver

1. Load the kernel module:
```bash
sudo modprobe fpga_npu
```

2. Verify the device is detected:
```bash
lspci | grep -i xilinx
dmesg | tail -20
```

3. Check device node creation:
```bash
ls -l /dev/fpga_npu
```

## Step 6: Run Your First Test

1. Compile the example program:
```bash
cd examples
gcc -o matrix_test matrix_test.c -lfpga_npu
```

2. Run the test:
```bash
./matrix_test
```

Expected output:
```
NPU: Initialized successfully
Matrix multiplication completed successfully
Results verified: PASS
NPU: Cleanup completed
```

## Step 7: Verify Performance

1. Run the benchmark suite:
```bash
cd software/tests
make
./benchmark --test all
```

Expected performance metrics:
- Matrix Multiply (64x64): ~500 GOPS
- Convolution 3x3: ~400 GOPS
- Memory Bandwidth: ~20 GB/s

## Common Issues and Solutions

### Issue: "No such device" error

**Symptoms**: `/dev/fpga_npu` not found

**Solution**:
```bash
# Check if driver loaded
lsmod | grep fpga_npu

# Manually create device node if needed
sudo mknod /dev/fpga_npu c $(grep fpga_npu /proc/devices | cut -d' ' -f1) 0
sudo chmod 666 /dev/fpga_npu
```

### Issue: PCIe device not detected

**Symptoms**: `lspci` doesn't show Xilinx device

**Solutions**:
1. Check PCIe connection
2. Verify FPGA is programmed
3. Check power supply
4. Rescan PCIe bus:
```bash
echo 1 | sudo tee /sys/bus/pci/rescan
```

### Issue: Driver compilation fails

**Symptoms**: "No such file or directory" errors

**Solution**:
```bash
# Install kernel headers
sudo apt-get install linux-headers-$(uname -r)  # Debian/Ubuntu
sudo yum install kernel-devel                   # RedHat/CentOS
```

### Issue: Vivado not found

**Symptoms**: "vivado: command not found"

**Solution**:
```bash
# Source Vivado settings
source /tools/Xilinx/Vivado/2022.1/settings64.sh
# Add to ~/.bashrc for permanent setup
```

## Example Application

Here's a simple example to get you started:

```c
#include <fpga_npu_lib.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Initialize NPU
    npu_handle_t npu = npu_init();
    if (!npu) {
        printf("Failed to initialize NPU\n");
        return 1;
    }
    
    // Allocate small matrices for testing
    const int M = 32, K = 16, N = 32;
    
    float *a = npu_alloc(npu, M * K * sizeof(float));
    float *b = npu_alloc(npu, K * N * sizeof(float));
    float *c = npu_alloc(npu, M * N * sizeof(float));
    
    // Initialize with simple pattern
    for (int i = 0; i < M * K; i++) a[i] = 1.0f;
    for (int i = 0; i < K * N; i++) b[i] = 2.0f;
    
    // Create tensor descriptors
    npu_tensor_t tensor_a = npu_create_tensor(a, 1, 1, M, K, NPU_DTYPE_FLOAT32);
    npu_tensor_t tensor_b = npu_create_tensor(b, 1, 1, K, N, NPU_DTYPE_FLOAT32);
    npu_tensor_t tensor_c = npu_create_tensor(c, 1, 1, M, N, NPU_DTYPE_FLOAT32);
    
    // Perform matrix multiplication
    printf("Computing %dx%d × %dx%d matrix multiplication...\n", M, K, K, N);
    
    int ret = npu_matrix_multiply(npu, &tensor_a, &tensor_b, &tensor_c);
    if (ret == NPU_SUCCESS) {
        printf("Success! Result[0][0] = %.2f (expected: %.2f)\n", 
               c[0], (float)(K * 1.0f * 2.0f));
    } else {
        printf("Matrix multiplication failed with error %d\n", ret);
    }
    
    // Cleanup
    npu_free(npu, a);
    npu_free(npu, b);
    npu_free(npu, c);
    npu_cleanup(npu);
    
    return ret == NPU_SUCCESS ? 0 : 1;
}
```

Compile and run:
```bash
gcc -o simple_test simple_test.c -lfpga_npu
./simple_test
```

## Next Steps

1. **Explore Examples**: Check the `examples/` directory for more complex applications
2. **Read Documentation**: Review the API documentation in `docs/api/`
3. **Performance Tuning**: Learn about optimization techniques in `docs/tutorials/optimization.md`
4. **Custom Applications**: Start building your own neural network accelerator applications

## Getting Help

- **Documentation**: Browse the `docs/` directory
- **Examples**: Check `examples/` for sample code
- **Issues**: Report bugs on the GitHub issue tracker
- **Community**: Join the FPGA acceleration community forums

## Summary

You now have a working FPGA NPU system! The key points covered:

- ✅ Hardware design built and programmed
- ✅ Software stack compiled and installed  
- ✅ Driver loaded and device detected
- ✅ First application successfully executed
- ✅ Performance verified

You're ready to start accelerating your machine learning workloads with the FPGA NPU!