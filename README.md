# FPGA NPU PCIe Project

**⚠️ DISCLAIMER: USE AT YOUR OWN RISK**  
**This repository is provided for educational and research purposes. Users assume full responsibility for any consequences arising from the use of this code, including but not limited to hardware damage, system instability, or data loss. The authors disclaim all liability for any direct, indirect, incidental, or consequential damages.**

**🚀 COMPLETE IMPLEMENTATION: This project now includes a comprehensive, production-ready FPGA NPU implementation with extensive hardware RTL modules, kernel drivers, user-space libraries, testing frameworks, and CI/CD pipeline.**

A high-performance Neural Processing Unit (NPU) implementation on FPGA with PCIe interface for accelerated machine learning inference, featuring complete hardware-software co-design and robust testing infrastructure.

## Overview

This project implements a custom NPU (Neural Processing Unit) on FPGA hardware with a PCIe interface for seamless integration with host systems. The design focuses on efficient matrix operations, convolution processing, and memory management optimized for deep learning workloads.

## Features

### **🔧 Hardware Components**
- **Complete NPU Core**: Fully implemented with configurable processing elements for matrix multiplication and convolution operations
- **PCIe Controller**: Full PCIe 3.0/4.0 support with enhanced interrupt handling and DMA capabilities
- **Memory Management**: Advanced on-chip caches, external DDR4 interface, and optimized data paths
- **Processing Elements**: Scalable array of MAC units with configurable precision
- **Clock Domain Crossing**: Robust async FIFO implementation for multi-clock designs

### **💻 Software Stack**
- **Enhanced Kernel Driver**: Production-ready Linux driver with comprehensive interrupt handling and memory management
- **User-Space Library**: Complete API with tensor operations, memory management, and performance monitoring
- **Device Management**: Advanced IOCTL interface and scatter-gather DMA support

### **🧪 Testing & Validation**
- **Comprehensive Test Suite**: Unit tests, integration tests, simulation testbenches, and performance benchmarks
- **Hardware Validation**: Complete RTL testbenches for all major components
- **Software Testing**: End-to-end testing framework with stress tests and validation

### **🚀 DevOps & CI/CD**
- **GitHub Actions Pipeline**: Automated testing, building, and security scanning
- **CodeQL Security**: Integrated security analysis and vulnerability detection
- **Pre-commit Hooks**: Code quality enforcement and formatting

### **📦 Packaging & Deployment**
- **Docker Support**: Complete containerization with development and production environments
- **Multiple Package Formats**: DKMS, Debian, and RPM packaging support
- **System Integration**: Systemd services, udev rules, and automated deployment scripts

## Project Structure

```
fpga-npu-pcie/
├── hardware/                     # **Complete FPGA hardware design**
│   ├── rtl/                     # RTL modules (NPU core, PCIe controller, processing elements)
│   ├── configs/                 # Board-specific configurations (VCU118, ZCU102)
│   ├── constraints/             # Comprehensive timing and placement constraints
│   ├── scripts/                 # FPGA build and programming automation
│   ├── testbench/               # Complete RTL testbenches with validation framework
│   └── Makefile                 # Hardware build system
├── software/                     # **Production-ready software stack**
│   ├── driver/                  # Enhanced Linux kernel driver with interrupt handling
│   └── userspace/               # Complete user-space library with tensor operations
├── tests/                        # **Comprehensive testing infrastructure**
│   ├── unit/                    # Unit tests for core functionality
│   ├── integration/             # End-to-end and stress testing
│   ├── simulation/              # Hardware simulation tests
│   └── benchmarks/              # Performance benchmarking suite
├── examples/                     # **Ready-to-run example applications**
│   ├── matrix_multiply/         # Matrix multiplication examples
│   ├── cnn_inference/           # CNN inference demonstrations
│   ├── neural_network/          # Neural network processing examples
│   └── Makefile                 # Example build system
├── docs/                         # **Complete documentation suite**
│   ├── api_reference.md         # Comprehensive API documentation
│   ├── architecture.md          # System architecture guide
│   ├── getting_started.md       # Quick start tutorial
│   ├── developer_guide.md       # Development best practices
│   ├── user_guide.md            # End-user documentation
│   ├── performance_tuning.md    # Optimization guidelines
│   └── tutorial.md              # Step-by-step tutorials
├── packaging/                    # **Multi-platform packaging**
│   ├── docker/                  # Docker containerization
│   ├── dkms/                    # DKMS kernel module packaging
│   ├── debian/                  # Debian package specifications
│   ├── rpm/                     # RPM package specifications
│   ├── systemd/                 # System service definitions
│   └── udev/                    # Device management rules
├── scripts/                      # **Automation and deployment**
│   ├── setup.sh                # Environment setup automation
│   ├── deploy.sh                # Production deployment
│   ├── validate.sh              # System validation
│   ├── run_tests.py             # Test execution framework
│   └── setup-ci.sh              # CI/CD environment setup
├── .github/                      # **CI/CD pipeline**
│   ├── workflows/               # GitHub Actions automation
│   └── codeql/                  # Security scanning configuration
├── IMPLEMENTATION_PLAN.md        # **Development roadmap**
├── IMPLEMENTATION_STATUS.md      # **Current status and progress**
├── PROJECT_COMPLETION_REPORT.md  # **Comprehensive completion summary**
└── Makefile                      # **Root build system**
```

## Getting Started

### Prerequisites

- **FPGA Development Tools**: Xilinx Vivado 2022.1+ or Intel Quartus Prime
- **Linux Development Environment**: GCC, Make, Linux headers
- **Hardware**: Xilinx Zynq UltraScale+ or Intel Stratix/Arria FPGA board with PCIe

### **🚀 Quick Start**

1. **Clone the repository**:
   ```bash
   git clone https://github.com/naqibannur/fpga-npu-pcie.git
   cd fpga-npu-pcie
   ```

2. **Automated setup** (recommended):
   ```bash
   # Complete environment setup
   ./scripts/setup.sh
   
   # Build all components
   make all
   ```

3. **Manual build** (alternative):
   ```bash
   # Build hardware for specific board
   make hardware BOARD=vcu118  # or zcu102
   
   # Build software components
   make software
   
   # Install driver and library
   sudo make install
   ```

4. **Docker deployment** (containerized):
   ```bash
   # Build and run in Docker
   docker-compose up --build
   ```

### **🧪 Testing & Validation**

```bash
# Comprehensive test suite
./scripts/run_tests.py --all

# Hardware simulation tests
cd tests/simulation
./run_tests.sh

# Unit tests
cd tests/unit
./run_tests.sh

# Integration and stress tests
cd tests/integration
make test

# Performance benchmarks
cd tests/benchmarks
./benchmark_main --suite all

# Example applications
cd examples
make run-examples
```

### **📊 Example Applications**

```bash
# Matrix multiplication example
cd examples/matrix_multiply
make run

# CNN inference demonstration
cd examples/cnn_inference
./cnn_inference_example

# Neural network processing
cd examples/neural_network
./neural_network_example
```

## **🏗️ Architecture**

### **Hardware Architecture**
- **NPU Top Module**: Complete system integration with clock management and reset handling
- **NPU Core**: Advanced processing engine with configurable precision and parallelism
- **Processing Elements (PEs)**: Scalable array of specialized MAC units with optimized data paths
- **PCIe Controller**: Full-featured PCIe 3.0/4.0 interface with enhanced error handling
- **Memory Subsystem**: Multi-level cache hierarchy with DDR4 controller integration
- **Clock Domain Crossing**: Robust async FIFO implementation for multi-clock operation

### **Software Architecture**
- **Kernel Driver Layer**: 
  - Advanced interrupt handling with MSI/MSI-X support
  - Sophisticated memory management with IOMMU integration
  - Comprehensive error handling and recovery mechanisms
  - Device lifecycle management and power control

- **User-Space Library**:
  - High-level tensor operation APIs
  - Memory pool management and optimization
  - Performance monitoring and profiling tools
  - Thread-safe operation queuing and execution

### **Data Flow Architecture**
- **Initialization**: Device discovery, memory allocation, and configuration
- **Execution**: Instruction dispatch, data movement, and parallel processing
- **Completion**: Result retrieval, status monitoring, and resource cleanup
- **Error Handling**: Comprehensive error detection, reporting, and recovery

## **📈 Performance & Capabilities**

### **Target Performance Metrics**
- **Throughput**: 1000+ GOPS for INT8 operations, 500+ GOPS for FP16
- **Latency**: <1ms for typical inference workloads, <100μs for small matrices
- **Power Efficiency**: >100 GOPS/W with dynamic power management
- **Memory Bandwidth**: Up to 16 GB/s (PCIe 4.0 x16), 100+ GB/s on-chip

### **Supported Operations**
- **Matrix Operations**: Matrix multiplication, transpose, addition, scaling
- **Convolution**: 2D convolution with configurable kernel sizes and strides
- **Element-wise**: Addition, multiplication, activation functions (ReLU, sigmoid)
- **Memory Operations**: Efficient tensor loading, storing, and reshaping

### **Benchmarking Suite**
- **Latency Benchmarks**: Operation-specific timing analysis
- **Throughput Benchmarks**: Maximum sustainable performance testing
- **Power Efficiency**: Energy consumption profiling and optimization
- **Scalability**: Multi-device and concurrent operation testing

## **🤝 Contributing**

### **Development Workflow**
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Follow coding standards and run pre-commit hooks
4. Add comprehensive tests for new functionality
5. Update documentation as needed
6. Commit your changes (`git commit -m 'Add amazing feature'`)
7. Push to the branch (`git push origin feature/amazing-feature`)
8. Open a Pull Request with detailed description

### **Code Quality**
- **Pre-commit Hooks**: Automated code formatting and linting
- **CI/CD Pipeline**: Automated testing and security scanning
- **Documentation**: All new features must include documentation
- **Testing**: Comprehensive test coverage required for all changes

### **Development Guidelines**
- Follow Linux kernel coding style for driver code
- Use consistent naming conventions across all components
- Include performance benchmarks for optimization changes
- Maintain backward compatibility where possible

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## **📚 Documentation & Resources**

### **Available Documentation**
- **[Getting Started Guide](docs/getting_started.md)**: Quick setup and first steps
- **[API Reference](docs/api_reference.md)**: Complete API documentation
- **[Architecture Guide](docs/architecture.md)**: Detailed system architecture
- **[Developer Guide](docs/developer_guide.md)**: Development best practices
- **[User Guide](docs/user_guide.md)**: End-user documentation
- **[Performance Tuning](docs/performance_tuning.md)**: Optimization guidelines
- **[Tutorial](docs/tutorial.md)**: Step-by-step learning path

### **Project Status**
- **[Implementation Plan](IMPLEMENTATION_PLAN.md)**: Development roadmap
- **[Implementation Status](IMPLEMENTATION_STATUS.md)**: Current progress
- **[Completion Report](PROJECT_COMPLETION_REPORT.md)**: Comprehensive summary

## **📞 Contact & Support**

- **Author**: Naqib Annur
- **Project Link**: https://github.com/naqibannur/fpga-npu-pcie
- **Issues**: Please use GitHub Issues for bug reports and feature requests
- **Discussions**: Use GitHub Discussions for questions and community support

## **🙏 Acknowledgments**

- **FPGA Community**: Open-source IP cores and development tools
- **Linux Kernel Community**: Driver development frameworks and best practices
- **PCIe Specification Contributors**: Industry standards and documentation
- **Machine Learning Community**: Algorithm optimization and acceleration research
- **Open Source Contributors**: Tools, libraries, and development frameworks

---

**⭐ If this project helps you, please consider giving it a star! ⭐**