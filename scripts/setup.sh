#!/bin/bash

# FPGA NPU Project Setup Script
# Configures the development environment and dependencies

set -e

echo "FPGA NPU Project Setup"
echo "======================"

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   echo "This script should not be run as root" 
   exit 1
fi

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
    if [ -f /etc/debian_version ]; then
        DISTRO="debian"
    elif [ -f /etc/redhat-release ]; then
        DISTRO="redhat"
    else
        DISTRO="unknown"
    fi
else
    echo "Unsupported operating system: $OSTYPE"
    exit 1
fi

echo "Detected OS: $OS ($DISTRO)"

# Check dependencies
check_command() {
    if ! command -v $1 &> /dev/null; then
        echo "❌ $1 is not installed"
        return 1
    else
        echo "✅ $1 is available"
        return 0
    fi
}

echo ""
echo "Checking dependencies..."

# Essential tools
check_command make
check_command gcc
check_command git

# Kernel development
if [ -d "/lib/modules/$(uname -r)/build" ]; then
    echo "✅ Kernel headers are available"
else
    echo "❌ Kernel headers are missing"
    if [ "$DISTRO" = "debian" ]; then
        echo "   Install with: sudo apt-get install linux-headers-$(uname -r)"
    elif [ "$DISTRO" = "redhat" ]; then
        echo "   Install with: sudo yum install kernel-devel"
    fi
fi

# Optional but recommended
check_command dkms || echo "   Consider installing DKMS for automatic driver rebuilding"

# FPGA tools check
echo ""
echo "Checking FPGA development tools..."
if command -v vivado &> /dev/null; then
    echo "✅ Xilinx Vivado is available"
    vivado -version | head -n 1
elif command -v quartus &> /dev/null; then
    echo "✅ Intel Quartus is available"
    quartus_sh --version | head -n 1
else
    echo "⚠️  No FPGA development tools detected"
    echo "   Please install Xilinx Vivado or Intel Quartus"
fi

# Create necessary directories
echo ""
echo "Creating project directories..."
mkdir -p build
mkdir -p logs
mkdir -p tmp

# Set up git hooks (if in git repository)
if [ -d ".git" ]; then
    echo "Setting up git hooks..."
    cp scripts/pre-commit .git/hooks/
    chmod +x .git/hooks/pre-commit
fi

# Generate build configuration
echo ""
echo "Generating build configuration..."
cat > build_config.mk << EOF
# Auto-generated build configuration
# Generated on $(date)

# System information
BUILD_HOST = $(hostname)
BUILD_USER = $(whoami)
BUILD_DATE = $(date)
KERNEL_VERSION = $(uname -r)

# Compiler information
CC_VERSION = $(gcc --version | head -n 1)

# Build directories
BUILD_DIR = build
LOG_DIR = logs
TMP_DIR = tmp
EOF

# Check PCI devices (for development boards)
echo ""
echo "Checking for FPGA development boards..."
if command -v lspci &> /dev/null; then
    if lspci | grep -i xilinx &> /dev/null; then
        echo "✅ Xilinx device detected:"
        lspci | grep -i xilinx
    elif lspci | grep -i altera &> /dev/null; then
        echo "✅ Intel/Altera device detected:"
        lspci | grep -i altera
    else
        echo "ℹ️  No FPGA development board detected"
    fi
else
    echo "⚠️  lspci not available"
fi

# Final setup
echo ""
echo "Setup complete!"
echo ""
echo "Next steps:"
echo "1. Review and modify config.mk for your target platform"
echo "2. Build hardware design: make hardware"
echo "3. Build software components: make software"
echo "4. Install driver: sudo make install"
echo ""
echo "For help, run: make help"