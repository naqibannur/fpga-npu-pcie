#!/bin/bash

# FPGA NPU Project - CI/CD Setup Script
# 
# Comprehensive setup script for continuous integration and development environment
# Configures tools, dependencies, and development workflow automation

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
LOG_FILE="$PROJECT_ROOT/setup.log"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log() {
    echo -e "${GREEN}[INFO]${NC} $1" | tee -a "$LOG_FILE"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1" | tee -a "$LOG_FILE"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1" | tee -a "$LOG_FILE"
}

debug() {
    if [[ "${DEBUG:-0}" == "1" ]]; then
        echo -e "${BLUE}[DEBUG]${NC} $1" | tee -a "$LOG_FILE"
    fi
}

# Check if running as root
check_root() {
    if [[ $EUID -eq 0 ]]; then
        error "This script should not be run as root"
        exit 1
    fi
}

# Detect operating system
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [[ -f /etc/os-release ]]; then
            . /etc/os-release
            OS_NAME="$ID"
            OS_VERSION="$VERSION_ID"
        else
            OS_NAME="linux"
            OS_VERSION="unknown"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS_NAME="macos"
        OS_VERSION=$(sw_vers -productVersion)
    else
        error "Unsupported operating system: $OSTYPE"
        exit 1
    fi
    
    log "Detected OS: $OS_NAME $OS_VERSION"
}

# Install system dependencies
install_system_deps() {
    log "Installing system dependencies..."
    
    case "$OS_NAME" in
        ubuntu|debian)
            sudo apt-get update
            sudo apt-get install -y \
                build-essential \
                cmake \
                git \
                curl \
                wget \
                python3 \
                python3-pip \
                python3-venv \
                pkg-config \
                libpthread-stubs0-dev \
                libc6-dev \
                linux-headers-generic \
                dkms \
                valgrind \
                gdb \
                strace \
                ltrace \
                cppcheck \
                clang-format \
                clang-tidy \
                doxygen \
                graphviz \
                lcov \
                gcov \
                tree \
                htop \
                jq \
                xmlstarlet
            ;;
        fedora|centos|rhel)
            sudo dnf install -y \
                gcc \
                gcc-c++ \
                make \
                cmake \
                git \
                curl \
                wget \
                python3 \
                python3-pip \
                kernel-devel \
                dkms \
                valgrind \
                gdb \
                strace \
                cppcheck \
                clang-tools-extra \
                doxygen \
                graphviz \
                lcov \
                tree \
                htop \
                jq
            ;;
        macos)
            # Check if Homebrew is installed
            if ! command -v brew &> /dev/null; then
                log "Installing Homebrew..."
                /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
            fi
            
            brew install \
                cmake \
                python3 \
                cppcheck \
                clang-format \
                doxygen \
                graphviz \
                tree \
                htop \
                jq
            ;;
        *)
            error "Unsupported OS for automatic dependency installation: $OS_NAME"
            warn "Please install dependencies manually"
            ;;
    esac
    
    log "System dependencies installed successfully"
}

# Setup Python virtual environment
setup_python_env() {
    log "Setting up Python virtual environment..."
    
    VENV_DIR="$PROJECT_ROOT/venv"
    
    if [[ ! -d "$VENV_DIR" ]]; then
        python3 -m venv "$VENV_DIR"
        log "Created virtual environment at $VENV_DIR"
    fi
    
    source "$VENV_DIR/bin/activate"
    
    # Upgrade pip
    pip install --upgrade pip setuptools wheel
    
    # Install Python dependencies
    if [[ -f "$PROJECT_ROOT/requirements.txt" ]]; then
        pip install -r "$PROJECT_ROOT/requirements.txt"
    fi
    
    # Install development dependencies
    pip install \
        pre-commit \
        pytest \
        pytest-cov \
        pytest-xdist \
        black \
        isort \
        flake8 \
        mypy \
        sphinx \
        sphinx-rtd-theme \
        breathe \
        cpplint \
        lizard \
        bandit \
        safety
    
    log "Python environment setup complete"
}

# Setup pre-commit hooks
setup_pre_commit() {
    log "Setting up pre-commit hooks..."
    
    source "$PROJECT_ROOT/venv/bin/activate"
    
    cd "$PROJECT_ROOT"
    
    # Install pre-commit hooks
    pre-commit install
    pre-commit install --hook-type commit-msg
    pre-commit install --hook-type pre-push
    
    # Run pre-commit on all files to check setup
    log "Running pre-commit on all files for initial setup..."
    pre-commit run --all-files || warn "Some pre-commit checks failed - this is normal for initial setup"
    
    log "Pre-commit hooks installed successfully"
}

# Setup Git configuration
setup_git_config() {
    log "Setting up Git configuration..."
    
    # Set up Git hooks directory
    mkdir -p "$PROJECT_ROOT/.git/hooks"
    
    # Configure Git settings for the project
    git config core.autocrlf false
    git config core.filemode true
    git config pull.rebase false
    
    # Set up commit message template
    cat > "$PROJECT_ROOT/.gitmessage" << 'EOF'
# <type>(<scope>): <subject>
#
# <body>
#
# <footer>
#
# Type can be:
#   feat     - new feature
#   fix      - bug fix
#   docs     - documentation
#   style    - formatting, missing semi colons, etc
#   refactor - code change that neither fixes a bug nor adds a feature
#   test     - adding missing tests
#   chore    - maintain, build, CI, etc
#
# Scope can be:
#   hardware - RTL, constraints, synthesis
#   driver   - kernel driver
#   library  - user library
#   tests    - test suites
#   ci       - CI/CD pipeline
#   docs     - documentation
#
# Subject line should be 50 characters or less
# Body should wrap at 72 characters
# Footer should contain issue references (Fixes #123, Closes #456)
EOF
    
    git config commit.template .gitmessage
    
    log "Git configuration complete"
}

# Setup development tools
setup_dev_tools() {
    log "Setting up development tools..."
    
    # Create development scripts directory
    mkdir -p "$PROJECT_ROOT/scripts"
    
    # Create useful development scripts
    cat > "$PROJECT_ROOT/scripts/dev-setup.sh" << 'EOF'
#!/bin/bash
# Quick development environment setup
source venv/bin/activate
export PROJECT_ROOT="$(pwd)"
export PATH="$PROJECT_ROOT/scripts:$PATH"
echo "Development environment activated"
EOF
    chmod +x "$PROJECT_ROOT/scripts/dev-setup.sh"
    
    # Create build wrapper script
    cat > "$PROJECT_ROOT/scripts/build-all.sh" << 'EOF'
#!/bin/bash
# Build all components
set -e

echo "Building driver..."
cd software/driver && make clean && make

echo "Building user library..."
cd ../userspace && make clean && make

echo "Building tests..."
cd ../../tests/unit && make clean && make
cd ../integration && make clean && make
cd ../benchmarks && make clean && make

echo "All components built successfully"
EOF
    chmod +x "$PROJECT_ROOT/scripts/build-all.sh"
    
    # Create test runner script
    cat > "$PROJECT_ROOT/scripts/run-tests.sh" << 'EOF'
#!/bin/bash
# Run all tests
set -e

echo "Running unit tests..."
cd tests/unit && make test

echo "Running integration tests..."
cd ../integration && make test-software-only

echo "Running quick benchmarks..."
cd ../benchmarks && make run-quick

echo "All tests completed"
EOF
    chmod +x "$PROJECT_ROOT/scripts/run-tests.sh"
    
    log "Development tools setup complete"
}

# Setup IDE configuration
setup_ide_config() {
    log "Setting up IDE configuration..."
    
    # VS Code settings
    mkdir -p "$PROJECT_ROOT/.vscode"
    
    cat > "$PROJECT_ROOT/.vscode/settings.json" << 'EOF'
{
    "C_Cpp.default.cStandard": "c11",
    "C_Cpp.default.cppStandard": "c++17",
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/software/userspace",
        "${workspaceFolder}/software/driver",
        "${workspaceFolder}/tests/unit",
        "${workspaceFolder}/tests/integration",
        "${workspaceFolder}/tests/benchmarks"
    ],
    "files.associations": {
        "*.sv": "systemverilog",
        "*.v": "verilog"
    },
    "python.defaultInterpreterPath": "./venv/bin/python",
    "python.formatting.provider": "black",
    "python.linting.enabled": true,
    "python.linting.flake8Enabled": true,
    "editor.formatOnSave": true,
    "editor.codeActionsOnSave": {
        "source.organizeImports": true
    },
    "clang-format.executable": "clang-format",
    "clang-format.style": "file"
}
EOF
    
    cat > "$PROJECT_ROOT/.vscode/extensions.json" << 'EOF'
{
    "recommendations": [
        "ms-vscode.cpptools",
        "ms-python.python",
        "ms-python.black-formatter",
        "ms-python.flake8",
        "ms-vscode.cmake-tools",
        "yzhang.markdown-all-in-one",
        "streetsidesoftware.code-spell-checker",
        "eamodio.gitlens",
        "ms-vscode.vscode-json",
        "redhat.vscode-yaml"
    ]
}
EOF
    
    cat > "$PROJECT_ROOT/.vscode/tasks.json" << 'EOF'
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build All",
            "type": "shell",
            "command": "./scripts/build-all.sh",
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "presentation": {
                "echo": true,
                "reveal": "always",
                "focus": false,
                "panel": "shared"
            }
        },
        {
            "label": "Run Tests",
            "type": "shell",
            "command": "./scripts/run-tests.sh",
            "group": "test",
            "presentation": {
                "echo": true,
                "reveal": "always",
                "focus": false,
                "panel": "shared"
            }
        },
        {
            "label": "Clean All",
            "type": "shell",
            "command": "find . -name 'build' -type d -exec rm -rf {} + && find . -name '*.o' -delete",
            "group": "build"
        }
    ]
}
EOF
    
    log "IDE configuration complete"
}

# Setup documentation
setup_documentation() {
    log "Setting up documentation framework..."
    
    mkdir -p "$PROJECT_ROOT/docs"
    
    # Create Doxygen configuration
    cat > "$PROJECT_ROOT/docs/Doxyfile" << 'EOF'
PROJECT_NAME           = "FPGA NPU"
PROJECT_VERSION        = "1.0.0"
PROJECT_BRIEF          = "FPGA-based Neural Processing Unit"
OUTPUT_DIRECTORY       = docs/doxygen
INPUT                  = software/ hardware/testbench/ tests/
RECURSIVE              = YES
EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = YES
EXTRACT_STATIC         = YES
GENERATE_HTML          = YES
GENERATE_LATEX         = NO
HTML_OUTPUT            = html
SEARCHENGINE           = YES
HAVE_DOT               = YES
CALL_GRAPH             = YES
CALLER_GRAPH           = YES
EOF
    
    # Create Sphinx configuration
    source "$PROJECT_ROOT/venv/bin/activate"
    cd "$PROJECT_ROOT/docs"
    
    if [[ ! -f "conf.py" ]]; then
        sphinx-quickstart --quiet --project "FPGA NPU" --author "Development Team" \
                         --release "1.0.0" --language "en" --extensions "breathe" .
    fi
    
    log "Documentation framework setup complete"
}

# Verify installation
verify_installation() {
    log "Verifying installation..."
    
    # Check if virtual environment works
    if source "$PROJECT_ROOT/venv/bin/activate"; then
        log "✓ Python virtual environment is working"
    else
        error "✗ Python virtual environment setup failed"
        return 1
    fi
    
    # Check pre-commit installation
    if pre-commit --version &> /dev/null; then
        log "✓ Pre-commit is installed"
    else
        error "✗ Pre-commit installation failed"
        return 1
    fi
    
    # Check if development scripts are executable
    if [[ -x "$PROJECT_ROOT/scripts/build-all.sh" ]]; then
        log "✓ Development scripts are ready"
    else
        error "✗ Development scripts setup failed"
        return 1
    fi
    
    # Try to build a simple component
    log "Testing build system..."
    if cd "$PROJECT_ROOT/software/userspace" && make clean &> /dev/null && make &> /dev/null; then
        log "✓ Build system is working"
    else
        warn "⚠ Build system test failed - some dependencies might be missing"
    fi
    
    log "Installation verification complete"
}

# Print usage information
print_usage() {
    cat << EOF
FPGA NPU CI/CD Setup Script

Usage: $0 [OPTIONS]

Options:
    -h, --help      Show this help message
    -v, --verbose   Enable verbose output
    -q, --quiet     Suppress non-error output
    --skip-deps     Skip system dependency installation
    --skip-python   Skip Python environment setup
    --skip-git      Skip Git configuration
    --dev-only      Setup development tools only

Examples:
    $0                          # Full setup
    $0 --skip-deps             # Skip system dependencies
    $0 --dev-only              # Development tools only
    $0 -v                      # Verbose output

EOF
}

# Main setup function
main() {
    local skip_deps=false
    local skip_python=false
    local skip_git=false
    local dev_only=false
    local verbose=false
    local quiet=false
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                print_usage
                exit 0
                ;;
            -v|--verbose)
                DEBUG=1
                verbose=true
                shift
                ;;
            -q|--quiet)
                quiet=true
                shift
                ;;
            --skip-deps)
                skip_deps=true
                shift
                ;;
            --skip-python)
                skip_python=true
                shift
                ;;
            --skip-git)
                skip_git=true
                shift
                ;;
            --dev-only)
                dev_only=true
                shift
                ;;
            *)
                error "Unknown option: $1"
                print_usage
                exit 1
                ;;
        esac
    done
    
    # Redirect output if quiet mode
    if [[ "$quiet" == "true" ]]; then
        exec > "$LOG_FILE" 2>&1
    fi
    
    # Initialize log file
    echo "FPGA NPU CI/CD Setup - $(date)" > "$LOG_FILE"
    
    log "Starting FPGA NPU CI/CD setup..."
    log "Project root: $PROJECT_ROOT"
    
    # Run setup steps
    check_root
    detect_os
    
    if [[ "$dev_only" == "false" ]]; then
        if [[ "$skip_deps" == "false" ]]; then
            install_system_deps
        fi
        
        if [[ "$skip_python" == "false" ]]; then
            setup_python_env
        fi
    fi
    
    setup_dev_tools
    setup_ide_config
    
    if [[ "$dev_only" == "false" ]]; then
        if [[ "$skip_git" == "false" ]]; then
            setup_git_config
        fi
        
        if [[ "$skip_python" == "false" ]]; then
            setup_pre_commit
        fi
        
        setup_documentation
        verify_installation
    fi
    
    log ""
    log "🎉 FPGA NPU CI/CD setup completed successfully!"
    log ""
    log "Next steps:"
    log "1. Source the development environment: source scripts/dev-setup.sh"
    log "2. Build all components: ./scripts/build-all.sh"
    log "3. Run tests: ./scripts/run-tests.sh"
    log "4. Check the documentation: make -C docs html"
    log ""
    log "Log file: $LOG_FILE"
}

# Run main function with all arguments
main "$@"