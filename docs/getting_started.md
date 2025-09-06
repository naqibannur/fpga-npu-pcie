# Getting Started with FPGA NPU

This guide provides step-by-step instructions for setting up and using the FPGA NPU system for the first time.

## Table of Contents

- [Quick Start](#quick-start)
- [System Requirements](#system-requirements)
- [Installation Methods](#installation-methods)
- [First Steps](#first-steps)
- [Basic Usage Examples](#basic-usage-examples)
- [Troubleshooting](#troubleshooting)
- [What's Next](#whats-next)

## Quick Start

### 30-Second Setup (Automated)

For the fastest setup experience, use our automated installation script:

```bash
# Download and run the setup script
curl -fsSL https://raw.githubusercontent.com/example/fpga-npu/main/scripts/setup.sh | bash

# Or if you prefer to review the script first:
curl -fsSL https://raw.githubusercontent.com/example/fpga-npu/main/scripts/setup.sh -o setup.sh
chmod +x setup.sh
./setup.sh
```

### 5-Minute Manual Setup

If you prefer manual installation or want more control:

```bash
# 1. Install dependencies
sudo apt-get update
sudo apt-get install build-essential cmake git linux-headers-$(uname -r)

# 2. Clone and build
git clone https://github.com/example/fpga-npu.git
cd fpga-npu
mkdir build && cd build
cmake .. && make -j$(nproc)

# 3. Install
sudo make install
sudo modprobe fpga_npu
```

### Verify Installation

```bash
# Check if everything is working
npu-info --version
npu-info --devices
npu-test --quick
```

## System Requirements

### Minimum Requirements

| Component | Requirement |
|-----------|-------------|
| **OS** | Linux 4.19+ (Ubuntu 18.04+, CentOS 7+, Fedora 28+) |
| **CPU** | x86_64 processor |
| **Memory** | 4GB RAM |
| **Storage** | 2GB free space |
| **Permissions** | sudo access |

### Recommended Requirements

| Component | Recommendation |
|-----------|----------------|
| **OS** | Ubuntu 20.04 LTS or newer |
| **CPU** | Multi-core x86_64 (4+ cores) |
| **Memory** | 8GB+ RAM |
| **Storage** | 10GB+ free space (for development) |
| **Hardware** | FPGA NPU device installed |

### Supported Distributions

✅ **Ubuntu**: 18.04, 20.04, 22.04  
✅ **CentOS**: 7, 8  
✅ **RHEL**: 7, 8, 9  
✅ **Fedora**: 28+  
✅ **Debian**: 10, 11  

### Hardware Compatibility

The NPU software can run in three modes:

1. **Hardware Mode**: With actual FPGA NPU hardware
2. **Simulation Mode**: Software simulation for development
3. **Emulation Mode**: Simplified emulation for testing

## Installation Methods

### Method 1: Automated Setup Script (Recommended)

Our setup script handles everything automatically:

```bash
# Interactive installation (recommended for first-time users)
curl -fsSL https://example.com/setup.sh | bash

# Non-interactive installation (for automation)
curl -fsSL https://example.com/setup.sh | bash -s -- --non-interactive

# Custom installation directory
curl -fsSL https://example.com/setup.sh | bash -s -- --install-dir /opt/fpga-npu
```

**What the script does:**
- ✅ Checks system requirements
- ✅ Installs dependencies
- ✅ Downloads and builds source code
- ✅ Installs drivers and libraries
- ✅ Configures system services
- ✅ Runs verification tests

### Method 2: Package Installation

#### Debian/Ubuntu

```bash
# Add repository
curl -fsSL https://packages.example.com/key.gpg | sudo apt-key add -
echo "deb https://packages.example.com/ubuntu $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/fpga-npu.list

# Install packages
sudo apt-get update
sudo apt-get install fpga-npu fpga-npu-devel fpga-npu-docs
```

#### CentOS/RHEL/Fedora

```bash
# Add repository
sudo tee /etc/yum.repos.d/fpga-npu.repo << EOF
[fpga-npu]
name=FPGA NPU Repository
baseurl=https://packages.example.com/rhel/\$releasever/\$basearch/
enabled=1
gpgcheck=1
gpgkey=https://packages.example.com/key.gpg
EOF

# Install packages
sudo yum install fpga-npu fpga-npu-devel fpga-npu-docs
```

### Method 3: Source Installation

For developers or custom builds:

```bash
# Prerequisites
sudo apt-get install build-essential cmake git linux-headers-$(uname -r) \
    pkg-config libsystemd-dev dkms python3 python3-pip

# Clone source
git clone https://github.com/example/fpga-npu.git
cd fpga-npu

# Build and install
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=ON
make -j$(nproc)
make test
sudo make install
sudo ldconfig

# Setup kernel module
sudo dkms add -m fpga-npu -v 1.0.0
sudo dkms build -m fpga-npu -v 1.0.0
sudo dkms install -m fpga-npu -v 1.0.0
```

### Method 4: Docker Installation

For containerized environments:

```bash
# Run development environment
docker run -it --privileged fpga-npu:devel

# Run runtime environment
docker run -d --privileged --name npu-runtime fpga-npu:runtime

# Use Docker Compose for full environment
git clone https://github.com/example/fpga-npu.git
cd fpga-npu/packaging/docker
docker-compose up -d
```

## First Steps

### 1. Verify Installation

After installation, verify everything is working:

```bash
# Check version
npu-info --version
# Expected output: FPGA NPU Utilities v1.0.0

# Check system status
npu-info --system
# Shows kernel module status, device detection, etc.

# List available devices
npu-info --devices
# Lists detected NPU devices or shows simulation mode

# Run quick test
npu-test --quick
# Runs basic functionality tests
```

### 2. Load Kernel Module

The kernel module should load automatically, but if needed:

```bash
# Load module manually
sudo modprobe fpga_npu

# Check if loaded
lsmod | grep fpga_npu

# View module information
modinfo fpga_npu
```

### 3. Check Device Access

```bash
# Check device files
ls -l /dev/fpga_npu*

# Check permissions (should be accessible to your user)
groups $USER

# If needed, add user to npuusers group
sudo usermod -a -G npuusers $USER
# Then log out and back in
```

### 4. Start System Service

```bash
# Start NPU service
sudo systemctl start fpga-npu

# Enable automatic startup
sudo systemctl enable fpga-npu

# Check service status
systemctl status fpga-npu
```

## Basic Usage Examples

### Example 1: Your First NPU Program

Create a simple program to test NPU functionality:

```bash
# Create a new directory for your project
mkdir my-npu-project && cd my-npu-project

# Create a simple test program
cat > hello_npu.c << 'EOF'
#include <stdio.h>
#include "fpga_npu_lib.h"

int main() {
    npu_handle_t npu;
    
    // Initialize NPU
    if (npu_init(&npu) != NPU_SUCCESS) {
        printf("Failed to initialize NPU\n");
        return 1;
    }
    
    printf("Hello, NPU! 🚀\n");
    
    // Get device information
    npu_device_info_t info;
    if (npu_get_device_info(npu, &info) == NPU_SUCCESS) {
        printf("Device: %s\n", info.device_name);
        printf("Memory: %zu MB\n", info.memory_size / (1024*1024));
    }
    
    // Cleanup
    npu_cleanup(npu);
    return 0;
}
EOF

# Compile and run
gcc hello_npu.c -o hello_npu -lfpga_npu
./hello_npu
```

### Example 2: Matrix Multiplication

```bash
# Copy and run the matrix multiplication example
cp -r /usr/local/share/fpga-npu/examples/matrix_multiply .
cd matrix_multiply

# Build and run
make
./matrix_multiply_example --size 256

# Expected output:
# Matrix multiplication (256x256) completed in X.XX ms
# Performance: XXX.X GFLOPS
```

### Example 3: Neural Network Inference

```bash
# Try the CNN inference example
cp -r /usr/local/share/fpga-npu/examples/cnn_inference .
cd cnn_inference

# Build and run
make
./cnn_inference_example --verbose

# This runs a simple LeNet-5 style CNN for digit classification
```

### Example 4: Benchmarking Performance

```bash
# Run comprehensive benchmarks
npu-benchmark --all

# Run specific benchmark
npu-benchmark --matrix-multiply --sizes 64,128,256,512,1024

# Run with custom parameters
npu-benchmark --matrix-multiply --iterations 100 --warmup 10
```

## Troubleshooting

### Common Issues and Solutions

#### Issue: "No NPU devices found"

**Causes and Solutions:**

1. **Hardware not connected**
   ```bash
   # Check PCI devices
   lspci | grep -i npu
   # If not found, check hardware connection
   ```

2. **Kernel module not loaded**
   ```bash
   # Load module
   sudo modprobe fpga_npu
   
   # Check if loaded
   lsmod | grep fpga_npu
   ```

3. **Driver not installed**
   ```bash
   # Reinstall driver
   sudo dkms remove fpga-npu/1.0.0 --all
   sudo dkms install fpga-npu/1.0.0
   ```

4. **Permission issues**
   ```bash
   # Check device permissions
   ls -l /dev/fpga_npu*
   
   # Fix permissions if needed
   sudo chmod 666 /dev/fpga_npu*
   ```

#### Issue: "Operation failed with error -4"

This usually indicates a hardware communication error:

```bash
# Check system logs
dmesg | grep -i npu
journalctl -u fpga-npu

# Reset the device
echo 1 | sudo tee /sys/bus/pci/devices/*/reset

# Restart the service
sudo systemctl restart fpga-npu
```

#### Issue: Low performance

**Performance optimization:**

```bash
# Check CPU frequency scaling
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Set performance mode
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Check memory bandwidth
npu-benchmark --memory-bandwidth

# Monitor thermals
npu-info --thermal
```

#### Issue: Compilation errors

**For development setup:**

```bash
# Install missing headers
sudo apt-get install linux-headers-$(uname -r)

# Update package cache
sudo apt-get update

# Reinstall build dependencies
sudo apt-get install --reinstall build-essential cmake
```

### Getting Help

1. **Check Documentation**
   - User Guide: `/usr/local/share/doc/fpga-npu/user_guide.md`
   - API Reference: `/usr/local/share/doc/fpga-npu/api_reference.md`
   - Tutorials: `/usr/local/share/doc/fpga-npu/tutorial.md`

2. **Use Built-in Help**
   ```bash
   npu-info --help
   npu-test --help
   npu-benchmark --help
   ```

3. **Enable Debug Mode**
   ```bash
   export NPU_DEBUG=1
   export NPU_LOG_LEVEL=DEBUG
   npu-info --system
   ```

4. **Check System Logs**
   ```bash
   # Service logs
   journalctl -u fpga-npu -f
   
   # Kernel logs
   dmesg | grep -i npu
   
   # Application logs
   tail -f /var/log/fpga-npu/npu.log
   ```

5. **Community Support**
   - GitHub Issues: https://github.com/example/fpga-npu/issues
   - Discussions: https://github.com/example/fpga-npu/discussions
   - Documentation: https://fpga-npu.readthedocs.io

## What's Next

### For Users

1. **Explore Examples**
   - Matrix operations: `/usr/local/share/fpga-npu/examples/matrix_multiply/`
   - Neural networks: `/usr/local/share/fpga-npu/examples/cnn_inference/`
   - Training: `/usr/local/share/fpga-npu/examples/neural_training/`

2. **Read Documentation**
   - [User Guide](user_guide.md) - Comprehensive usage guide
   - [API Reference](api_reference.md) - Complete API documentation
   - [Performance Tuning](performance_tuning.md) - Optimization techniques

3. **Join the Community**
   - Star the project on GitHub
   - Join discussions and share your use cases
   - Report issues and contribute improvements

### For Developers

1. **Development Setup**
   ```bash
   # Install development packages
   sudo apt-get install fpga-npu-devel
   
   # Or use Docker development environment
   docker run -it --privileged fpga-npu:devel
   ```

2. **Read Developer Resources**
   - [Developer Guide](developer_guide.md) - Contributing guidelines
   - [Architecture](architecture.md) - System architecture
   - [Tutorial](tutorial.md) - Programming tutorials

3. **Start Building**
   - Build your first NPU application
   - Contribute to the project
   - Create optimized neural network kernels

### Performance Optimization

Once you have the basics working:

1. **Profile Your Applications**
   ```bash
   npu-profile ./your_application
   ```

2. **Optimize Memory Usage**
   - Use memory pools for frequent allocations
   - Align data structures properly
   - Minimize host-device transfers

3. **Tune for Your Workload**
   - Experiment with different batch sizes
   - Use appropriate precision (FP32 vs FP16 vs INT8)
   - Leverage parallelism across multiple PEs

Congratulations! You're now ready to start using the FPGA NPU for accelerated computing. 🎉

---

**Need help?** Check our [troubleshooting guide](troubleshooting.md) or ask questions in our [community discussions](https://github.com/example/fpga-npu/discussions).