# FPGA NPU PCIe Project

**⚠️ IMPORTANT NOTE: This is an initial development version. The HDL code and software components have not been tested yet and are provided as a starting framework for development.**

A high-performance Neural Processing Unit (NPU) implementation on FPGA with PCIe interface for accelerated machine learning inference.

## Overview

This project implements a custom NPU (Neural Processing Unit) on FPGA hardware with a PCIe interface for seamless integration with host systems. The design focuses on efficient matrix operations, convolution processing, and memory management optimized for deep learning workloads.

## Features

- **High-Performance NPU Core**: Optimized for matrix multiplication and convolution operations
- **PCIe Interface**: PCIe 3.0/4.0 support for high-bandwidth host communication
- **Memory Management**: Efficient on-chip and external memory handling
- **Driver Support**: Linux kernel driver for seamless host integration
- **Configurable Architecture**: Scalable design supporting different NPU configurations

## Project Structure

```
fpga-npu-pcie/
├── hardware/                 # FPGA hardware design files
│   ├── rtl/                 # RTL source code (Verilog/SystemVerilog)
│   ├── constraints/         # Timing and placement constraints
│   ├── ip/                  # IP cores and custom modules
│   └── testbench/           # Hardware testbenches
├── software/                # Software components
│   ├── driver/              # Linux kernel driver
│   ├── userspace/           # User-space libraries and applications
│   └── tests/               # Software tests and benchmarks
├── docs/                    # Documentation
│   ├── architecture/        # System architecture documentation
│   ├── api/                 # API documentation
│   └── tutorials/           # Getting started guides
├── tools/                   # Build and development tools
├── examples/                # Example applications and demos
└── scripts/                 # Build and utility scripts
```

## Getting Started

### Prerequisites

- **FPGA Development Tools**: Xilinx Vivado 2022.1+ or Intel Quartus Prime
- **Linux Development Environment**: GCC, Make, Linux headers
- **Hardware**: Xilinx Zynq UltraScale+ or Intel Stratix/Arria FPGA board with PCIe

### Build Instructions

1. **Clone the repository**:
   ```bash
   git clone https://github.com/naqibannur/fpga-npu-pcie.git
   cd fpga-npu-pcie
   ```

2. **Build hardware design**:
   ```bash
   cd hardware
   make build BOARD=<your_board_name>
   ```

3. **Build software driver**:
   ```bash
   cd software/driver
   make
   sudo make install
   ```

4. **Build user-space library**:
   ```bash
   cd software/userspace
   make
   sudo make install
   ```

### Quick Test

```bash
# Load the driver
sudo modprobe fpga_npu

# Run basic functionality test
cd software/tests
./basic_test

# Run performance benchmark
./benchmark --test matrix_multiply
```

## Architecture

### NPU Core Architecture
- **Processing Elements (PEs)**: Configurable array of MAC units
- **Memory Hierarchy**: L1/L2 caches with external DDR4 interface
- **Control Unit**: Instruction decoder and execution controller
- **DMA Engine**: High-performance data movement

### PCIe Interface
- **Base Address Registers (BARs)**: Memory-mapped control and data regions
- **Interrupt Handling**: MSI/MSI-X support for efficient event notification
- **DMA Transfers**: Scatter-gather DMA for large data transfers

## Performance

Target performance metrics:
- **Throughput**: 1000+ GOPS for INT8 operations
- **Latency**: <1ms for typical inference workloads
- **Power Efficiency**: >100 GOPS/W
- **PCIe Bandwidth**: Up to 16 GB/s (PCIe 4.0 x16)

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contact

- **Author**: Naqib Annur
- **Email**: [naqibannur@example.com]
- **Project Link**: https://github.com/naqibannur/fpga-npu-pcie

## Acknowledgments

- FPGA community for open-source IP cores
- PCIe specification contributors
- Machine learning acceleration research community