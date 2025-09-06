#!/bin/bash

# FPGA NPU Quick Setup Script
# This script automates the installation and initial setup of the FPGA NPU system

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default configuration
INSTALL_DIR="/usr/local"
BUILD_TYPE="Release"
SKIP_TESTS=false
FORCE_INSTALL=false
INTERACTIVE=true
ENABLE_DOCKER=true
ENABLE_SYSTEMD=true

# Logging functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_step() { echo -e "${CYAN}[STEP]${NC} $1"; }

# Progress indicator
show_progress() {
    local current=$1
    local total=$2
    local description=$3
    local percent=$((current * 100 / total))
    local filled=$((percent / 2))
    local empty=$((50 - filled))
    
    printf "\r${CYAN}[%3d%%]${NC} [" "$percent"
    printf "%*s" $filled | tr ' ' '='
    printf "%*s" $empty | tr ' ' '-'
    printf "] %s" "$description"
    
    if [[ $current -eq $total ]]; then
        echo
    fi
}

# Show welcome banner
show_banner() {
    cat << 'EOF'
┌─────────────────────────────────────────────────────────────────┐
│                     FPGA NPU Quick Setup                       │
│                                                                 │
│  This script will automatically install and configure the      │
│  FPGA NPU system on your machine.                             │
│                                                                 │
│  What will be installed:                                       │
│  • FPGA NPU kernel driver                                     │
│  • NPU runtime library and utilities                          │
│  • Development headers and examples                           │
│  • Documentation and tutorials                                │
│  • System services and configuration                          │
└─────────────────────────────────────────────────────────────────┘
EOF
}

# Show help
show_help() {
    cat << EOF
FPGA NPU Quick Setup Script

Usage: $0 [OPTIONS]

Options:
    --install-dir DIR       Installation directory (default: $INSTALL_DIR)
    --build-type TYPE       Build type: Debug|Release (default: $BUILD_TYPE)
    --skip-tests            Skip running tests during build
    --force                 Force installation even if already installed
    --non-interactive       Run without user prompts
    --no-docker             Skip Docker setup
    --no-systemd            Skip systemd service setup
    -h, --help              Show this help message

Examples:
    $0                                    # Interactive installation
    $0 --non-interactive --skip-tests    # Automated installation
    $0 --install-dir /opt --force        # Custom install location

Environment Variables:
    SUDO_PASSWORD          Password for sudo operations (non-interactive mode)

EOF
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --install-dir)
                INSTALL_DIR="$2"
                shift 2
                ;;
            --build-type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            --skip-tests)
                SKIP_TESTS=true
                shift
                ;;
            --force)
                FORCE_INSTALL=true
                shift
                ;;
            --non-interactive)
                INTERACTIVE=false
                shift
                ;;
            --no-docker)
                ENABLE_DOCKER=false
                shift
                ;;
            --no-systemd)
                ENABLE_SYSTEMD=false
                shift
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# Confirm with user
confirm_installation() {
    if [[ "$INTERACTIVE" == "true" ]]; then
        echo
        log_info "Installation Configuration:"
        echo "  Install Directory: $INSTALL_DIR"
        echo "  Build Type: $BUILD_TYPE"
        echo "  Skip Tests: $SKIP_TESTS"
        echo "  Docker Support: $ENABLE_DOCKER"
        echo "  Systemd Support: $ENABLE_SYSTEMD"
        echo
        
        read -p "Continue with installation? [Y/n] " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]] && [[ -n $REPLY ]]; then
            log_info "Installation cancelled by user"
            exit 0
        fi
    fi
}

# Check system requirements
check_requirements() {
    log_step "Checking system requirements..."
    
    local requirements_met=true
    local total_checks=8
    local current_check=0
    
    # Check OS
    ((current_check++))
    show_progress $current_check $total_checks "Checking operating system..."
    if [[ "$(uname)" != "Linux" ]]; then
        log_error "This script only supports Linux systems"
        requirements_met=false
    fi
    
    # Check architecture
    ((current_check++))
    show_progress $current_check $total_checks "Checking architecture..."
    if [[ "$(uname -m)" != "x86_64" ]]; then
        log_warn "Only x86_64 architecture is fully supported"
    fi
    
    # Check kernel version
    ((current_check++))
    show_progress $current_check $total_checks "Checking kernel version..."
    local kernel_version
    kernel_version=$(uname -r | cut -d. -f1-2)
    local min_kernel="4.19"
    if [[ "$(printf '%s\n' "$min_kernel" "$kernel_version" | sort -V | head -n1)" != "$min_kernel" ]]; then
        log_error "Kernel version $kernel_version is too old. Minimum required: $min_kernel"
        requirements_met=false
    fi
    
    # Check available disk space (need at least 2GB)
    ((current_check++))
    show_progress $current_check $total_checks "Checking disk space..."
    local available_space
    available_space=$(df / | awk 'NR==2 {print $4}')
    if [[ $available_space -lt 2097152 ]]; then # 2GB in KB
        log_error "Insufficient disk space. Need at least 2GB available"
        requirements_met=false
    fi
    
    # Check memory (need at least 4GB)
    ((current_check++))
    show_progress $current_check $total_checks "Checking memory..."
    local total_memory
    total_memory=$(grep MemTotal /proc/meminfo | awk '{print $2}')
    if [[ $total_memory -lt 4194304 ]]; then # 4GB in KB
        log_warn "Low memory detected. 4GB+ recommended for optimal performance"
    fi
    
    # Check if running as root or with sudo access
    ((current_check++))
    show_progress $current_check $total_checks "Checking privileges..."
    if [[ $EUID -eq 0 ]]; then
        log_warn "Running as root. This is not recommended for security reasons"
    elif ! sudo -n true 2>/dev/null; then
        if [[ "$INTERACTIVE" == "false" && -z "${SUDO_PASSWORD:-}" ]]; then
            log_error "Sudo access required. Set SUDO_PASSWORD environment variable or run interactively"
            requirements_met=false
        fi
    fi
    
    # Check for package manager
    ((current_check++))
    show_progress $current_check $total_checks "Checking package manager..."
    if command -v apt-get &> /dev/null; then
        PACKAGE_MANAGER="apt"
    elif command -v yum &> /dev/null; then
        PACKAGE_MANAGER="yum"
    elif command -v dnf &> /dev/null; then
        PACKAGE_MANAGER="dnf"
    else
        log_error "No supported package manager found (apt/yum/dnf)"
        requirements_met=false
    fi
    
    # Check internet connectivity
    ((current_check++))
    show_progress $current_check $total_checks "Checking internet connectivity..."
    if ! ping -c 1 google.com &> /dev/null; then
        log_warn "No internet connectivity detected. Some features may not work"
    fi
    
    show_progress $total_checks $total_checks "Requirements check complete"
    
    if [[ "$requirements_met" == "false" ]]; then
        log_error "System requirements not met. Please fix the issues above and try again"
        exit 1
    fi
    
    log_success "System requirements check passed"
}

# Install system dependencies
install_dependencies() {
    log_step "Installing system dependencies..."
    
    local packages=()
    
    case "$PACKAGE_MANAGER" in
        apt)
            packages=(
                "build-essential"
                "cmake"
                "git"
                "linux-headers-$(uname -r)"
                "pkg-config"
                "libsystemd-dev"
                "dkms"
                "python3"
                "python3-pip"
                "curl"
                "wget"
            )
            
            # Update package lists
            log_info "Updating package lists..."
            sudo apt-get update -qq
            
            # Install packages
            log_info "Installing packages: ${packages[*]}"
            sudo apt-get install -y "${packages[@]}"
            ;;
            
        yum|dnf)
            packages=(
                "gcc"
                "gcc-c++"
                "cmake"
                "git"
                "kernel-devel"
                "pkg-config"
                "systemd-devel"
                "dkms"
                "python3"
                "python3-pip"
                "curl"
                "wget"
            )
            
            log_info "Installing packages: ${packages[*]}"
            sudo $PACKAGE_MANAGER install -y "${packages[@]}"
            ;;
    esac
    
    # Install Python packages
    log_info "Installing Python packages..."
    pip3 install --user numpy matplotlib pytest
    
    log_success "Dependencies installed successfully"
}

# Download or use existing source code
setup_source() {
    log_step "Setting up source code..."
    
    if [[ -f "$PROJECT_ROOT/CMakeLists.txt" ]]; then
        log_info "Using existing source code in $PROJECT_ROOT"
        SOURCE_DIR="$PROJECT_ROOT"
    else
        log_info "Downloading FPGA NPU source code..."
        SOURCE_DIR="/tmp/fpga-npu-setup"
        
        # Clone repository (replace with actual repository URL)
        git clone https://github.com/example/fpga-npu.git "$SOURCE_DIR" || {
            log_error "Failed to download source code"
            exit 1
        }
    fi
    
    log_success "Source code ready at $SOURCE_DIR"
}

# Build the project
build_project() {
    log_step "Building FPGA NPU..."
    
    cd "$SOURCE_DIR"
    
    # Create build directory
    local build_dir="$SOURCE_DIR/build"
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # Configure build
    log_info "Configuring build (type: $BUILD_TYPE)..."
    cmake .. \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
        -DENABLE_TESTING=ON \
        -DENABLE_DOCUMENTATION=ON
    
    # Build
    local jobs=$(nproc)
    log_info "Building with $jobs parallel jobs..."
    make -j"$jobs"
    
    # Run tests if not skipped
    if [[ "$SKIP_TESTS" == "false" ]]; then
        log_info "Running tests..."
        make test
    fi
    
    log_success "Build completed successfully"
}

# Install the project
install_project() {
    log_step "Installing FPGA NPU..."
    
    cd "$SOURCE_DIR/build"
    
    # Install
    log_info "Installing to $INSTALL_DIR..."
    sudo make install
    
    # Update library cache
    sudo ldconfig
    
    log_success "Installation completed"
}

# Setup kernel module
setup_kernel_module() {
    log_step "Setting up kernel module..."
    
    # Install DKMS package
    local dkms_dir="/usr/src/fpga-npu-1.0.0"
    sudo mkdir -p "$dkms_dir"
    sudo cp -r "$SOURCE_DIR/software/driver/"* "$dkms_dir/"
    sudo cp "$SOURCE_DIR/packaging/dkms/dkms.conf" "$dkms_dir/"
    
    # Add to DKMS
    log_info "Adding module to DKMS..."
    sudo dkms add -m fpga-npu -v 1.0.0
    
    # Build and install
    log_info "Building and installing kernel module..."
    sudo dkms build -m fpga-npu -v 1.0.0
    sudo dkms install -m fpga-npu -v 1.0.0
    
    # Install modprobe configuration
    sudo cp "$SOURCE_DIR/packaging/modprobe/fpga-npu.conf" /etc/modprobe.d/
    
    # Install udev rules
    sudo cp "$SOURCE_DIR/packaging/udev/99-fpga-npu.rules" /lib/udev/rules.d/
    sudo udevadm control --reload-rules
    
    log_success "Kernel module setup completed"
}

# Setup systemd service
setup_systemd() {
    if [[ "$ENABLE_SYSTEMD" == "false" ]]; then
        log_info "Skipping systemd setup (disabled)"
        return
    fi
    
    log_step "Setting up systemd service..."
    
    # Install service file
    sudo cp "$SOURCE_DIR/packaging/systemd/fpga-npu.service" /lib/systemd/system/
    
    # Reload systemd
    sudo systemctl daemon-reload
    
    # Enable service
    sudo systemctl enable fpga-npu.service
    
    log_success "Systemd service configured"
}

# Setup Docker (optional)
setup_docker() {
    if [[ "$ENABLE_DOCKER" == "false" ]]; then
        log_info "Skipping Docker setup (disabled)"
        return
    fi
    
    log_step "Setting up Docker environment..."
    
    # Check if Docker is installed
    if ! command -v docker &> /dev/null; then
        log_info "Installing Docker..."
        curl -fsSL https://get.docker.com -o get-docker.sh
        sudo sh get-docker.sh
        rm get-docker.sh
        
        # Add user to docker group
        sudo usermod -aG docker "$USER"
        log_warn "Please log out and back in for Docker group membership to take effect"
    fi
    
    # Build Docker images
    if [[ -f "$SOURCE_DIR/packaging/docker/Dockerfile" ]]; then
        log_info "Building Docker images..."
        cd "$SOURCE_DIR"
        docker build -f packaging/docker/Dockerfile -t fpga-npu:latest .
        
        # Build specific targets
        docker build -f packaging/docker/Dockerfile --target devel -t fpga-npu:devel .
        docker build -f packaging/docker/Dockerfile --target runtime -t fpga-npu:runtime .
    fi
    
    log_success "Docker environment configured"
}

# Create configuration
create_config() {
    log_step "Creating configuration files..."
    
    # Create config directory
    sudo mkdir -p /etc/fpga-npu
    
    # Create main configuration file
    sudo tee /etc/fpga-npu/npu.conf > /dev/null << 'EOF'
# FPGA NPU Configuration File

[general]
log_level = INFO
debug = false
max_devices = 8

[device]
enable_msi = true
dma_coherent = true
power_management = true

[performance]
auto_frequency_scaling = true
thermal_throttling = true
performance_monitoring = true

[security]
allow_all_users = false
require_authentication = false
EOF

    # Set permissions
    sudo chmod 644 /etc/fpga-npu/npu.conf
    
    log_success "Configuration files created"
}

# Verify installation
verify_installation() {
    log_step "Verifying installation..."
    
    local verification_passed=true
    
    # Check if binaries exist
    local binaries=("npu-info" "npu-test" "npu-benchmark")
    for binary in "${binaries[@]}"; do
        if [[ -x "$INSTALL_DIR/bin/$binary" ]]; then
            log_info "✓ $binary installed"
        else
            log_warn "✗ $binary missing"
            verification_passed=false
        fi
    done
    
    # Check if library exists
    if [[ -f "$INSTALL_DIR/lib/libfpga_npu.so" ]]; then
        log_info "✓ NPU library installed"
    else
        log_warn "✗ NPU library missing"
        verification_passed=false
    fi
    
    # Check if headers exist
    if [[ -f "$INSTALL_DIR/include/fpga_npu_lib.h" ]]; then
        log_info "✓ Development headers installed"
    else
        log_warn "✗ Development headers missing"
        verification_passed=false
    fi
    
    # Check kernel module
    if [[ -f "/lib/modules/$(uname -r)/extra/fpga_npu.ko" ]]; then
        log_info "✓ Kernel module installed"
    else
        log_warn "✗ Kernel module missing"
        verification_passed=false
    fi
    
    # Test basic functionality
    if command -v "$INSTALL_DIR/bin/npu-info" &> /dev/null; then
        log_info "Testing basic functionality..."
        if "$INSTALL_DIR/bin/npu-info" --version &> /dev/null; then
            log_info "✓ Basic functionality test passed"
        else
            log_warn "✗ Basic functionality test failed"
        fi
    fi
    
    if [[ "$verification_passed" == "true" ]]; then
        log_success "Installation verification passed"
    else
        log_warn "Some components may not be properly installed"
    fi
}

# Show next steps
show_next_steps() {
    cat << EOF

${GREEN}🎉 FPGA NPU Installation Complete! 🎉${NC}

${CYAN}Next Steps:${NC}

1. ${YELLOW}Load the kernel module:${NC}
   sudo modprobe fpga_npu

2. ${YELLOW}Start the NPU service:${NC}
   sudo systemctl start fpga-npu

3. ${YELLOW}Check for NPU devices:${NC}
   $INSTALL_DIR/bin/npu-info

4. ${YELLOW}Run a simple test:${NC}
   $INSTALL_DIR/bin/npu-test --quick

5. ${YELLOW}Try the examples:${NC}
   cd $INSTALL_DIR/share/fpga-npu/examples
   make && ./matrix_multiply_example

${CYAN}Documentation:${NC}
• User Guide: $INSTALL_DIR/share/doc/fpga-npu/user_guide.md
• API Reference: $INSTALL_DIR/share/doc/fpga-npu/api_reference.md
• Tutorials: $INSTALL_DIR/share/doc/fpga-npu/tutorial.md

${CYAN}Troubleshooting:${NC}
• Check logs: journalctl -u fpga-npu
• Debug mode: NPU_DEBUG=1 $INSTALL_DIR/bin/npu-info
• Get help: $INSTALL_DIR/bin/npu-info --help

${GREEN}Happy NPU computing! 🚀${NC}

EOF
}

# Main installation function
main() {
    local start_time
    start_time=$(date +%s)
    
    show_banner
    parse_args "$@"
    
    if [[ "$INTERACTIVE" == "true" ]]; then
        echo
        log_info "Welcome to the FPGA NPU Quick Setup!"
        echo
    fi
    
    confirm_installation
    
    # Installation steps
    check_requirements
    install_dependencies
    setup_source
    build_project
    install_project
    setup_kernel_module
    setup_systemd
    setup_docker
    create_config
    verify_installation
    
    local end_time
    end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    echo
    log_success "Installation completed in $duration seconds"
    show_next_steps
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi