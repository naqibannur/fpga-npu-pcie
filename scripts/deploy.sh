#!/bin/bash

# FPGA NPU Project Deployment Script
# This script packages and deploys the FPGA NPU project for distribution

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
DIST_DIR="$PROJECT_ROOT/dist"
PACKAGE_DIR="$PROJECT_ROOT/packages"

# Version information
VERSION_FILE="$PROJECT_ROOT/VERSION"
if [[ -f "$VERSION_FILE" ]]; then
    VERSION=$(cat "$VERSION_FILE")
else
    VERSION="1.0.0"
fi

# Package information
PACKAGE_NAME="fpga-npu"
PACKAGE_VERSION="$VERSION"
PACKAGE_ARCH="$(uname -m)"
PACKAGE_OS="$(uname -s | tr '[:upper:]' '[:lower:]')"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Help function
show_help() {
    cat << EOF
FPGA NPU Deployment Script

Usage: $0 [OPTIONS] [COMMAND]

Commands:
    build       Build the project
    package     Create distribution packages
    install     Install packages
    deploy      Deploy to target systems
    clean       Clean build artifacts
    help        Show this help message

Options:
    -v, --version VERSION    Set package version (default: $VERSION)
    -t, --target TARGET      Target architecture (default: $PACKAGE_ARCH)
    -o, --output DIR         Output directory (default: $DIST_DIR)
    -j, --jobs JOBS          Number of parallel jobs (default: $(nproc))
    --debug                  Enable debug build
    --no-tests               Skip running tests
    --force                  Force overwrite existing packages
    -h, --help               Show this help message

Examples:
    $0 build                 # Build project
    $0 package               # Create packages
    $0 install               # Install locally
    $0 deploy --target x86_64 # Deploy for x86_64

EOF
}

# Parse command line arguments
parse_args() {
    local COMMAND=""
    local DEBUG=false
    local NO_TESTS=false
    local FORCE=false
    local JOBS=$(nproc)
    local TARGET="$PACKAGE_ARCH"
    local OUTPUT="$DIST_DIR"

    while [[ $# -gt 0 ]]; do
        case $1 in
            build|package|install|deploy|clean)
                COMMAND="$1"
                shift
                ;;
            -v|--version)
                PACKAGE_VERSION="$2"
                shift 2
                ;;
            -t|--target)
                TARGET="$2"
                shift 2
                ;;
            -o|--output)
                OUTPUT="$2"
                shift 2
                ;;
            -j|--jobs)
                JOBS="$2"
                shift 2
                ;;
            --debug)
                DEBUG=true
                shift
                ;;
            --no-tests)
                NO_TESTS=true
                shift
                ;;
            --force)
                FORCE=true
                shift
                ;;
            -h|--help|help)
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

    # Export variables for use in functions
    export COMMAND DEBUG NO_TESTS FORCE JOBS TARGET OUTPUT
    export PACKAGE_VERSION
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    local required_tools=("cmake" "make" "gcc" "git")
    local missing_tools=()

    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            missing_tools+=("$tool")
        fi
    done

    if [[ ${#missing_tools[@]} -gt 0 ]]; then
        log_error "Missing required tools: ${missing_tools[*]}"
        log_error "Please install the missing tools and try again"
        exit 1
    fi

    # Check for kernel headers
    if [[ ! -d "/lib/modules/$(uname -r)/build" ]]; then
        log_warn "Kernel headers not found at /lib/modules/$(uname -r)/build"
        log_warn "Driver compilation may fail"
    fi

    log_success "Prerequisites check passed"
}

# Build the project
build_project() {
    log_info "Building FPGA NPU project..."

    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    # Configure build
    local build_type="Release"
    if [[ "$DEBUG" == "true" ]]; then
        build_type="Debug"
    fi

    log_info "Configuring build (type: $build_type)..."
    cmake .. \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DCMAKE_INSTALL_PREFIX="/usr/local" \
        -DENABLE_TESTING=ON \
        -DENABLE_DOCUMENTATION=ON \
        -DPACKAGE_VERSION="$PACKAGE_VERSION"

    # Build
    log_info "Building project with $JOBS parallel jobs..."
    make -j"$JOBS"

    # Run tests if not disabled
    if [[ "$NO_TESTS" != "true" ]]; then
        log_info "Running tests..."
        make test
    fi

    # Build documentation
    log_info "Building documentation..."
    make docs || log_warn "Documentation build failed (optional)"

    cd "$PROJECT_ROOT"
    log_success "Project build completed"
}

# Create distribution packages
create_packages() {
    log_info "Creating distribution packages..."

    # Create directories
    mkdir -p "$DIST_DIR" "$PACKAGE_DIR"

    # Package names
    local binary_package="${PACKAGE_NAME}_${PACKAGE_VERSION}_${TARGET}"
    local source_package="${PACKAGE_NAME}_${PACKAGE_VERSION}_src"
    local devel_package="${PACKAGE_NAME}-devel_${PACKAGE_VERSION}_${TARGET}"

    # Create binary package
    log_info "Creating binary package: $binary_package"
    create_binary_package "$binary_package"

    # Create development package
    log_info "Creating development package: $devel_package"
    create_devel_package "$devel_package"

    # Create source package
    log_info "Creating source package: $source_package"
    create_source_package "$source_package"

    # Create installer script
    log_info "Creating installer script..."
    create_installer_script

    log_success "Package creation completed"
    log_info "Packages created in: $DIST_DIR"
}

# Create binary package
create_binary_package() {
    local package_name="$1"
    local package_dir="$PACKAGE_DIR/$package_name"

    rm -rf "$package_dir"
    mkdir -p "$package_dir"

    # Install to temporary directory
    cd "$BUILD_DIR"
    make install DESTDIR="$package_dir"

    # Copy additional files
    cp "$PROJECT_ROOT/README.md" "$package_dir/"
    cp "$PROJECT_ROOT/LICENSE" "$package_dir/" 2>/dev/null || true
    cp "$PROJECT_ROOT/CHANGELOG.md" "$package_dir/" 2>/dev/null || true

    # Create package metadata
    cat > "$package_dir/PACKAGE_INFO" << EOF
Package: $PACKAGE_NAME
Version: $PACKAGE_VERSION
Architecture: $TARGET
OS: $PACKAGE_OS
Type: binary
Build-Date: $(date -u +"%Y-%m-%dT%H:%M:%SZ")
Description: FPGA NPU runtime and utilities
EOF

    # Create tarball
    cd "$PACKAGE_DIR"
    tar -czf "$DIST_DIR/${package_name}.tar.gz" "$package_name"
    rm -rf "$package_name"

    log_success "Binary package created: ${package_name}.tar.gz"
}

# Create development package
create_devel_package() {
    local package_name="$1"
    local package_dir="$PACKAGE_DIR/$package_name"

    rm -rf "$package_dir"
    mkdir -p "$package_dir"

    # Copy headers
    mkdir -p "$package_dir/usr/local/include"
    cp -r "$PROJECT_ROOT/software/userspace/"*.h "$package_dir/usr/local/include/" 2>/dev/null || true

    # Copy static libraries
    mkdir -p "$package_dir/usr/local/lib"
    cp "$BUILD_DIR/software/userspace/"*.a "$package_dir/usr/local/lib/" 2>/dev/null || true

    # Copy CMake config files
    mkdir -p "$package_dir/usr/local/lib/cmake/$PACKAGE_NAME"
    cp "$BUILD_DIR/"*Config.cmake "$package_dir/usr/local/lib/cmake/$PACKAGE_NAME/" 2>/dev/null || true

    # Copy documentation
    mkdir -p "$package_dir/usr/local/share/doc/$PACKAGE_NAME"
    cp -r "$PROJECT_ROOT/docs/"* "$package_dir/usr/local/share/doc/$PACKAGE_NAME/" 2>/dev/null || true

    # Copy examples
    mkdir -p "$package_dir/usr/local/share/$PACKAGE_NAME/examples"
    cp -r "$PROJECT_ROOT/examples/"* "$package_dir/usr/local/share/$PACKAGE_NAME/examples/" 2>/dev/null || true

    # Create package metadata
    cat > "$package_dir/PACKAGE_INFO" << EOF
Package: $PACKAGE_NAME-devel
Version: $PACKAGE_VERSION
Architecture: $TARGET
OS: $PACKAGE_OS
Type: development
Build-Date: $(date -u +"%Y-%m-%dT%H:%M:%SZ")
Description: FPGA NPU development headers and libraries
EOF

    # Create tarball
    cd "$PACKAGE_DIR"
    tar -czf "$DIST_DIR/${package_name}.tar.gz" "$package_name"
    rm -rf "$package_name"

    log_success "Development package created: ${package_name}.tar.gz"
}

# Create source package
create_source_package() {
    local package_name="$1"
    local package_dir="$PACKAGE_DIR/$package_name"

    rm -rf "$package_dir"
    mkdir -p "$package_dir"

    # Copy source files (excluding build artifacts)
    rsync -av \
        --exclude='.git' \
        --exclude='build' \
        --exclude='dist' \
        --exclude='packages' \
        --exclude='*.o' \
        --exclude='*.so' \
        --exclude='*.a' \
        --exclude='*.ko' \
        "$PROJECT_ROOT/" "$package_dir/"

    # Create package metadata
    cat > "$package_dir/PACKAGE_INFO" << EOF
Package: $PACKAGE_NAME-src
Version: $PACKAGE_VERSION
Architecture: any
OS: any
Type: source
Build-Date: $(date -u +"%Y-%m-%dT%H:%M:%SZ")
Description: FPGA NPU source code
EOF

    # Create tarball
    cd "$PACKAGE_DIR"
    tar -czf "$DIST_DIR/${package_name}.tar.gz" "$package_name"
    rm -rf "$package_name"

    log_success "Source package created: ${package_name}.tar.gz"
}

# Create installer script
create_installer_script() {
    cat > "$DIST_DIR/install.sh" << 'EOF'
#!/bin/bash

# FPGA NPU Installation Script

set -euo pipefail

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }

show_help() {
    cat << 'HELP'
FPGA NPU Installation Script

Usage: ./install.sh [OPTIONS]

Options:
    --binary-only       Install only binary package
    --devel-only        Install only development package
    --prefix PREFIX     Installation prefix (default: /usr/local)
    --user              Install to user directory (~/.local)
    --uninstall         Uninstall FPGA NPU
    -h, --help          Show this help message

Examples:
    ./install.sh                    # Install everything
    ./install.sh --user             # User installation
    ./install.sh --prefix /opt      # Custom prefix
    ./install.sh --uninstall        # Uninstall

HELP
}

install_package() {
    local package_file="$1"
    local install_prefix="$2"

    if [[ ! -f "$package_file" ]]; then
        log_error "Package file not found: $package_file"
        return 1
    fi

    log_info "Installing package: $(basename "$package_file")"
    
    # Extract package
    tar -xzf "$package_file" -C /tmp/
    local package_dir="/tmp/$(basename "$package_file" .tar.gz)"
    
    # Copy files
    if [[ -d "$package_dir/usr/local" ]]; then
        sudo cp -r "$package_dir/usr/local/"* "$install_prefix/"
    fi
    
    # Set permissions
    sudo chmod -R 755 "$install_prefix/bin" 2>/dev/null || true
    sudo chmod -R 644 "$install_prefix/lib/"*.so* 2>/dev/null || true
    sudo chmod -R 644 "$install_prefix/include/"* 2>/dev/null || true
    
    # Update library cache
    if [[ "$install_prefix" == "/usr/local" ]]; then
        sudo ldconfig
    fi
    
    # Cleanup
    rm -rf "$package_dir"
    
    log_success "Package installed successfully"
}

main() {
    local BINARY_ONLY=false
    local DEVEL_ONLY=false
    local INSTALL_PREFIX="/usr/local"
    local USER_INSTALL=false
    local UNINSTALL=false

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --binary-only)
                BINARY_ONLY=true
                shift
                ;;
            --devel-only)
                DEVEL_ONLY=true
                shift
                ;;
            --prefix)
                INSTALL_PREFIX="$2"
                shift 2
                ;;
            --user)
                USER_INSTALL=true
                INSTALL_PREFIX="$HOME/.local"
                shift
                ;;
            --uninstall)
                UNINSTALL=true
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

    # Handle uninstall
    if [[ "$UNINSTALL" == "true" ]]; then
        log_info "Uninstalling FPGA NPU..."
        # Add uninstall logic here
        log_success "FPGA NPU uninstalled"
        exit 0
    fi

    # Check for packages
    local script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    local binary_pkg=$(ls "$script_dir"/fpga-npu_*_*.tar.gz 2>/dev/null | head -1)
    local devel_pkg=$(ls "$script_dir"/fpga-npu-devel_*_*.tar.gz 2>/dev/null | head -1)

    # Install packages
    if [[ "$DEVEL_ONLY" != "true" && -n "$binary_pkg" ]]; then
        install_package "$binary_pkg" "$INSTALL_PREFIX"
    fi

    if [[ "$BINARY_ONLY" != "true" && -n "$devel_pkg" ]]; then
        install_package "$devel_pkg" "$INSTALL_PREFIX"
    fi

    log_success "FPGA NPU installation completed!"
    log_info "Installation prefix: $INSTALL_PREFIX"
}

main "$@"
EOF

    chmod +x "$DIST_DIR/install.sh"
    log_success "Installer script created: install.sh"
}

# Install packages locally
install_local() {
    log_info "Installing FPGA NPU locally..."

    if [[ ! -d "$BUILD_DIR" ]]; then
        log_error "Build directory not found. Please run 'build' first."
        exit 1
    fi

    cd "$BUILD_DIR"
    
    # Install kernel module
    log_info "Installing kernel module..."
    make -C software/driver modules_install || log_warn "Kernel module installation failed"

    # Install user space components
    log_info "Installing user space components..."
    sudo make install

    # Update library cache
    sudo ldconfig

    # Load kernel module
    log_info "Loading kernel module..."
    sudo modprobe fpga_npu || log_warn "Failed to load kernel module"

    log_success "Local installation completed"
}

# Deploy to target systems
deploy_to_targets() {
    log_info "Deploying to target systems..."

    local targets_file="$PROJECT_ROOT/deploy_targets.txt"
    if [[ ! -f "$targets_file" ]]; then
        log_warn "No deploy targets file found: $targets_file"
        log_info "Create a file with target hostnames/IPs, one per line"
        return 0
    fi

    while IFS= read -r target; do
        [[ -z "$target" || "$target" =~ ^#.* ]] && continue
        
        log_info "Deploying to: $target"
        
        # Copy packages
        scp "$DIST_DIR"/*.tar.gz "$DIST_DIR/install.sh" "$target:/tmp/" || {
            log_error "Failed to copy files to $target"
            continue
        }
        
        # Run installer
        ssh "$target" "cd /tmp && ./install.sh" || {
            log_error "Installation failed on $target"
            continue
        }
        
        log_success "Deployment to $target completed"
    done < "$targets_file"
}

# Clean build artifacts
clean_build() {
    log_info "Cleaning build artifacts..."

    rm -rf "$BUILD_DIR"
    rm -rf "$DIST_DIR"
    rm -rf "$PACKAGE_DIR"

    # Clean kernel module build artifacts
    if [[ -d "$PROJECT_ROOT/software/driver" ]]; then
        make -C "$PROJECT_ROOT/software/driver" clean 2>/dev/null || true
    fi

    log_success "Clean completed"
}

# Main function
main() {
    log_info "FPGA NPU Deployment Script v$VERSION"
    log_info "============================================"

    parse_args "$@"

    case "$COMMAND" in
        build)
            check_prerequisites
            build_project
            ;;
        package)
            check_prerequisites
            if [[ ! -d "$BUILD_DIR" ]]; then
                build_project
            fi
            create_packages
            ;;
        install)
            check_prerequisites
            install_local
            ;;
        deploy)
            check_prerequisites
            if [[ ! -d "$DIST_DIR" ]] || [[ -z "$(ls -A "$DIST_DIR" 2>/dev/null)" ]]; then
                log_info "No packages found, creating them first..."
                if [[ ! -d "$BUILD_DIR" ]]; then
                    build_project
                fi
                create_packages
            fi
            deploy_to_targets
            ;;
        clean)
            clean_build
            ;;
        *)
            log_error "No command specified"
            show_help
            exit 1
            ;;
    esac

    log_success "Operation completed successfully!"
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi