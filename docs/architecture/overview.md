# FPGA NPU Architecture Overview

## System Architecture

The FPGA NPU (Neural Processing Unit) is designed as a high-performance accelerator for machine learning inference workloads. The system consists of several key components working together to provide efficient neural network processing capabilities.

## Top-Level Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Host System   │    │   PCIe Bridge   │    │   FPGA Board    │
│                 │    │                 │    │                 │
│  ┌───────────┐  │    │  ┌───────────┐  │    │  ┌───────────┐  │
│  │Application│  │    │  │   PCIe    │  │    │  │    NPU    │  │
│  │           │  │◄──►│  │Controller │  │◄──►│  │   Core    │  │
│  │   Library │  │    │  │           │  │    │  │           │  │
│  └───────────┘  │    │  └───────────┘  │    │  └───────────┘  │
│  ┌───────────┐  │    │                 │    │  ┌───────────┐  │
│  │   Driver  │  │    │                 │    │  │  Memory   │  │
│  └───────────┘  │    │                 │    │  │Controller │  │
└─────────────────┘    └─────────────────┘    │  └───────────┘  │
                                              │  ┌───────────┐  │
                                              │  │ DDR4 DRAM │  │
                                              │  └───────────┘  │
                                              └─────────────────┘
```

## NPU Core Architecture

The NPU core is the heart of the accelerator, designed for efficient parallel processing of neural network operations.

### Processing Element Array

```
┌─────────────────────────────────────────┐
│           NPU Core                       │
│                                         │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐       │
│  │ PE0 │ │ PE1 │ │ PE2 │ │ PE3 │  ...  │
│  └─────┘ └─────┘ └─────┘ └─────┘       │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐       │
│  │ PE4 │ │ PE5 │ │ PE6 │ │ PE7 │  ...  │
│  └─────┘ └─────┘ └─────┘ └─────┘       │
│                 ...                     │
│  ┌─────────────────────────────────┐   │
│  │        Control Unit             │   │
│  │  ┌─────────┐ ┌─────────────┐   │   │
│  │  │Decoder  │ │Dispatch Unit│   │   │
│  │  └─────────┘ └─────────────┘   │   │
│  └─────────────────────────────────┘   │
│                                         │
│  ┌─────────────────────────────────┐   │
│  │        Memory Subsystem         │   │
│  │  ┌─────┐ ┌─────┐ ┌─────────┐   │   │
│  │  │L1 $ │ │L2 $ │ │DMA Eng. │   │   │
│  │  └─────┘ └─────┘ └─────────┘   │   │
│  └─────────────────────────────────┘   │
└─────────────────────────────────────────┘
```

### Processing Element (PE) Design

Each Processing Element is optimized for multiply-accumulate (MAC) operations:

```
┌─────────────────────────────────┐
│        Processing Element       │
│                                 │
│  ┌─────────┐    ┌─────────┐    │
│  │Input A  │    │Input B  │    │
│  │Register │    │Register │    │
│  └─────────┘    └─────────┘    │
│       │              │         │
│       └──────┬───────┘         │
│              │                 │
│         ┌─────▼─────┐          │
│         │Multiplier │          │
│         └─────┬─────┘          │
│               │                │
│         ┌─────▼─────┐          │
│         │Accumulator│          │
│         └─────┬─────┘          │
│               │                │
│         ┌─────▼─────┐          │
│         │  Output   │          │
│         │ Register  │          │
│         └───────────┘          │
└─────────────────────────────────┘
```

## Memory Hierarchy

The memory subsystem is designed for high throughput and low latency:

### Memory Map

| Address Range | Component | Description |
|---------------|-----------|-------------|
| 0x0000_0000 - 0x0000_FFFF | Control Registers | NPU control and status |
| 0x0001_0000 - 0x001F_FFFF | L1 Cache | Fast local memory |
| 0x0020_0000 - 0x01FF_FFFF | L2 Cache | Shared cache memory |
| 0x1000_0000 - 0x7FFF_FFFF | DDR4 DRAM | Main system memory |

### Cache Organization

- **L1 Cache**: 32KB per PE, direct-mapped
- **L2 Cache**: 512KB shared, 8-way associative
- **Cache Line Size**: 64 bytes
- **Replacement Policy**: LRU for L2, direct for L1

## PCIe Interface

The PCIe interface provides high-bandwidth communication with the host system:

### BAR Layout

| BAR | Size | Type | Description |
|-----|------|------|-------------|
| BAR0 | 64KB | Memory | Control registers |
| BAR1 | 1MB | Memory | Data transfer buffer |
| BAR2 | - | - | Reserved |
| BAR3 | - | - | Reserved |

### Register Map (BAR0)

| Offset | Register | Access | Description |
|--------|----------|--------|-------------|
| 0x00 | CTRL | R/W | Control register |
| 0x04 | STATUS | R | Status register |
| 0x08 | DATA_ADDR | R/W | Data buffer address |
| 0x0C | DATA_SIZE | R/W | Data transfer size |
| 0x10 | INT_CTRL | R/W | Interrupt control |
| 0x14 | INT_STATUS | R/W1C | Interrupt status |

## Data Flow

### Typical Operation Sequence

1. **Host Preparation**:
   - Application loads data into host memory
   - Driver maps memory for DMA access
   - Instructions prepared in command buffer

2. **Data Transfer**:
   - DMA engine transfers data from host to FPGA memory
   - Input tensors loaded into L2 cache
   - Weights and biases pre-loaded

3. **NPU Execution**:
   - Control unit decodes instructions
   - Processing elements execute in parallel
   - Results accumulated in output buffers

4. **Result Transfer**:
   - Output data moved to host memory via DMA
   - Interrupt signals completion
   - Host retrieves results

## Performance Characteristics

### Theoretical Performance

- **Peak Throughput**: 1024 GOPS (INT8)
- **Memory Bandwidth**: 25.6 GB/s (DDR4-3200)
- **PCIe Bandwidth**: 16 GB/s (PCIe 4.0 x16)
- **Power Consumption**: ~50W @ 200MHz

### Supported Operations

| Operation | Throughput | Latency | Power |
|-----------|------------|---------|-------|
| Matrix Multiply | 800 GOPS | 10μs | 45W |
| Convolution 3x3 | 600 GOPS | 15μs | 40W |
| Element-wise Add | 200 GOPS | 2μs | 20W |
| Activation (ReLU) | 400 GOPS | 3μs | 25W |

## Design Considerations

### Scalability

- Processing element count configurable (8-64 PEs)
- Memory hierarchy scales with PE count
- PCIe lanes configurable (x4, x8, x16)

### Power Management

- Clock gating for unused PEs
- Dynamic voltage and frequency scaling
- Power islands for different subsystems

### Reliability

- ECC protection for memories
- CRC checking for PCIe transfers
- Watchdog timers for deadlock detection