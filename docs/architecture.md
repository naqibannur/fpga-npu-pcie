# FPGA NPU Architecture Documentation

This document provides technical details of the FPGA NPU architecture, covering hardware design, software stack, and system integration.

## Table of Contents

- [System Overview](#system-overview)
- [Hardware Architecture](#hardware-architecture)
- [Software Architecture](#software-architecture)
- [Interface Specifications](#interface-specifications)
- [Performance Characteristics](#performance-characteristics)

## System Overview

### Design Philosophy

The FPGA NPU is a heterogeneous computing platform optimized for neural network acceleration, prioritizing scalability, efficiency, flexibility, and integration.

### Key Specifications

| Parameter | Specification |
|-----------|---------------|
| **Peak Performance** | 1000+ GOPS (INT8), 500+ GOPS (FP16) |
| **Memory Bandwidth** | 512 GB/s (on-chip), 32 GB/s (DDR4) |
| **Host Interface** | PCIe Gen3 x8 (8 GB/s) |
| **Power Consumption** | <75W TDP |
| **Operating Frequency** | 250-400 MHz |

### System Block Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                        Host System                         │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │Application  │ │NPU Library  │ │ Kernel Driver       │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    FPGA NPU Device                         │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              NPU Core                              │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────────────────┐  │   │
│  │  │Command  │ │Scheduler│ │Performance Monitor  │  │   │
│  │  │Processor│ │         │ │                     │  │   │
│  │  └─────────┘ └─────────┘ └─────────────────────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │         Processing Element Array (12 PEs)          │   │
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐  │   │
│  │  │ PE0 │ │ PE1 │ │ PE2 │ │ PE3 │ │ PE4 │ │ PE5 │  │   │
│  │  └─────┘ └─────┘ └─────┘ └─────┘ └─────┘ └─────┘  │   │
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐  │   │
│  │  │ PE6 │ │ PE7 │ │ PE8 │ │ PE9 │ │PE10 │ │PE11 │  │   │
│  │  └─────┘ └─────┘ └─────┘ └─────┘ └─────┘ └─────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │           Memory Subsystem                          │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────────────────┐  │   │
│  │  │L1 Cache │ │L2 Cache │ │     DDR4 Memory     │  │   │
│  │  │ (256KB) │ │  (8MB)  │ │      (16GB)         │  │   │
│  │  └─────────┘ └─────────┘ └─────────────────────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              PCIe Controller                        │   │
│  │  ┌─────────┐ ┌─────────────┐ ┌─────────────────┐  │   │
│  │  │DMA      │ │Interrupt    │ │Config Registers │  │   │
│  │  │Engine   │ │Controller   │ │                 │  │   │
│  │  └─────────┘ └─────────────┘ └─────────────────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

## Hardware Architecture

### NPU Core Design

#### Command Processor
- 64-entry instruction queue
- Out-of-order execution capability
- Dynamic resource allocation
- Multi-stage operation support

**Instruction Format (128-bit)**:
```
┌────────┬────────┬────────┬────────┬──────────┬────────┬──────────┐
│ Opcode │ Dest   │ Src1   │ Src2   │Immediate │ Flags  │ Reserved │
│(8-bit) │(8-bit) │(8-bit) │(8-bit) │(64-bit)  │(8-bit) │(24-bit)  │
└────────┴────────┴────────┴────────┴──────────┴────────┴──────────┘
```

**Supported Operations**:
- `MATMUL`: Matrix multiplication
- `CONV2D`: 2D convolution  
- `POOL`: Pooling operations
- `ACTIVATION`: Activation functions
- `BATCH_NORM`: Batch normalization
- `ELTWISE`: Element-wise operations

#### Scheduler
- Modified round-robin with priority levels
- Load balancing across processing elements
- Resource allocation management

### Processing Element (PE) Architecture

Each PE implements a 4-stage pipeline:

```
┌─────────────────────────────────────────────────────────────┐
│                Processing Element (PE)                     │
├─────────────────────────────────────────────────────────────┤
│  Input FIFOs      4-Stage Pipeline         Output FIFO     │
│  ┌────────────┐   ┌─────────────────┐     ┌──────────┐     │
│  │Data FIFO   │→  │Stage 1: Fetch   │  →  │Result    │     │
│  │(64 entries)│   │Stage 2: Decode  │     │FIFO      │     │
│  └────────────┘   │Stage 3: Execute │     │(32 entry)│     │
│  ┌────────────┐   │Stage 4: Write   │     └──────────┘     │
│  │Weight FIFO │→  └─────────────────┘                      │
│  │(32 entries)│                                            │
│  └────────────┘   ┌─────────────────┐                      │
│                   │  Local Memory   │                      │
│                   │   (32KB BRAM)   │                      │
│                   └─────────────────┘                      │
└─────────────────────────────────────────────────────────────┘
```

#### Arithmetic Units

**Floating Point Unit (FPU)**:
- IEEE 754 compliant FP32 operations
- Fused multiply-add (FMA) support
- 4-cycle latency, 1 operation/cycle throughput

**Fixed Point Unit (FixPU)**:
- INT8/INT16 arithmetic with configurable scaling
- 2-cycle latency, 2 operations/cycle throughput

**Vector ALU**:
- SIMD operations on 16 elements (FP16/INT8)
- Specialized activation function instructions
- 1-cycle latency for most operations

### Memory Hierarchy

**L1 Cache (Per PE)**:
- Size: 16KB instruction + 16KB data
- 4-way set associative, 64-byte line size
- LRU replacement policy

**L2 Cache (Shared)**:
- Size: 8MB total (4MB data + 4MB weights)
- 16-way set associative, 128-byte line size
- Modified LRU with frequency hints

**External Memory**:
- DDR4-2400 with 256-bit interface
- 16GB capacity, 32 GB/s bandwidth
- ECC support for reliability

### Interconnect Architecture

**Network-on-Chip**:
- 2D mesh topology (4x3 layout)
- XY routing with adaptive fallback
- Virtual channel flow control
- 1024 bits/cycle per link bandwidth

### PCIe Interface

**PCIe Controller Features**:
- PCIe Gen3 x8 interface (8 GB/s)
- 8 independent DMA channels
- IOMMU support for address translation
- Advanced Error Reporting (AER)

**DMA Engine**:
- Scatter-gather operation support
- Descriptor-based command processing
- Interrupt-driven completion notification

## Software Architecture

### Kernel Driver Design

#### Driver Architecture
```c
struct npu_device {
    struct pci_dev *pdev;           // PCI device structure
    void __iomem *bar0;             // Control registers
    void __iomem *bar2;             // Memory window
    
    struct npu_memory_manager mm;   // Memory management
    struct npu_dma_manager dma;     // DMA management
    struct npu_interrupt_manager irq; // Interrupt management
    
    struct mutex device_lock;       // Serialization
    atomic_t ref_count;             // Reference counting
    u32 capabilities;               // Hardware capabilities
    u32 status;                     // Device status
};
```

#### Memory Management
- DMA coherent memory allocation
- User space to kernel space mapping
- IOMMU address translation
- Memory protection and isolation

**DMA Buffer Structure**:
```c
struct npu_dma_buffer {
    void *cpu_addr;                 // CPU virtual address
    dma_addr_t dma_addr;           // DMA address
    size_t size;                   // Buffer size
    enum dma_data_direction direction; // DMA direction
    struct list_head list;         // Buffer list
    atomic_t ref_count;            // Reference count
    u32 flags;                     // Buffer flags
};
```

### User Library Architecture

#### API Layer Design
```c
typedef struct {
    struct npu_device_handle *device;   // Device handle
    struct npu_memory_pool *mem_pool;   // Memory pool
    struct npu_command_queue *cmd_queue; // Command queue
    struct npu_performance_monitor *perf; // Performance monitor
} npu_context_t;
```

#### Memory Pool Management
- Best fit, first fit, buddy system, and slab allocation strategies
- Thread-safe allocation and deallocation
- Memory usage tracking and optimization

#### Command Queue Management
- Asynchronous command submission
- Priority-based scheduling
- Completion notification mechanisms

## Interface Specifications

### Programming Interface

#### High-Level API
```c
// Initialization and cleanup
npu_result_t npu_init(npu_handle_t *handle);
void npu_cleanup(npu_handle_t handle);

// Memory management
void* npu_malloc(npu_handle_t handle, size_t size);
void npu_free(npu_handle_t handle, void *ptr);

// Tensor operations
npu_result_t npu_matrix_multiply(npu_handle_t handle,
                               const float *a, const float *b, float *c,
                               size_t m, size_t k, size_t n);

npu_result_t npu_conv2d(npu_handle_t handle,
                       const float *input, const float *kernel, float *output,
                       size_t in_h, size_t in_w, size_t in_c,
                       size_t out_c, size_t k_h, size_t k_w,
                       size_t stride_h, size_t stride_w);

// Performance monitoring
npu_result_t npu_get_performance_counters(npu_handle_t handle,
                                         npu_performance_counters_t *counters);
```

#### Register Interface
```c
// Register map (PCIe BAR0)
#define NPU_REG_CONTROL              0x0000
#define NPU_REG_STATUS               0x0004
#define NPU_REG_INTERRUPT_ENABLE     0x0008
#define NPU_REG_INTERRUPT_STATUS     0x000C
#define NPU_REG_COMMAND_QUEUE_BASE   0x0010
#define NPU_REG_COMMAND_QUEUE_SIZE   0x0014
#define NPU_REG_MEMORY_BASE          0x0020
#define NPU_REG_MEMORY_SIZE          0x0024
#define NPU_REG_PERF_COUNTER_BASE    0x0100

// Control register bits
#define NPU_CONTROL_ENABLE           (1 << 0)
#define NPU_CONTROL_RESET            (1 << 1)
#define NPU_CONTROL_DEBUG_ENABLE     (1 << 2)

// Status register bits
#define NPU_STATUS_READY             (1 << 0)
#define NPU_STATUS_BUSY              (1 << 1)
#define NPU_STATUS_ERROR             (1 << 2)
#define NPU_STATUS_THERMAL_WARNING   (1 << 3)
```

### IOCTL Interface
```c
// IOCTL command definitions
#define NPU_IOC_MAGIC               'N'
#define NPU_IOC_GET_DEVICE_INFO     _IOR(NPU_IOC_MAGIC, 1, struct npu_device_info)
#define NPU_IOC_ALLOC_MEMORY        _IOWR(NPU_IOC_MAGIC, 2, struct npu_memory_alloc)
#define NPU_IOC_FREE_MEMORY         _IOW(NPU_IOC_MAGIC, 3, struct npu_memory_free)
#define NPU_IOC_EXECUTE_COMMAND     _IOWR(NPU_IOC_MAGIC, 4, struct npu_command)
#define NPU_IOC_GET_PERFORMANCE     _IOR(NPU_IOC_MAGIC, 5, struct npu_performance_counters)
```

## Performance Characteristics

### Computational Performance

**Matrix Multiplication (FP32)**:
- Small matrices (64x64): 50 GFLOPS
- Medium matrices (256x256): 200 GFLOPS  
- Large matrices (1024x1024): 500 GFLOPS

**Convolution Operations**:
- 3x3 kernels: 300-400 GOPS (INT8)
- 5x5 kernels: 250-350 GOPS (INT8)
- Depthwise convolution: 800+ GOPS (INT8)

### Memory Performance

**Bandwidth Utilization**:
- L1 Cache: >90% hit rate for typical workloads
- L2 Cache: >80% hit rate for neural networks
- External Memory: 75% bandwidth utilization

**Latency Characteristics**:
- L1 Cache access: 1 cycle
- L2 Cache access: 4-8 cycles
- External memory access: 100-200 cycles

### Power Efficiency

**Performance per Watt**:
- Peak efficiency: 15 GOPS/Watt (INT8)
- Typical workload: 10-12 GOPS/Watt
- Idle power: <5W

**Thermal Management**:
- Operating temperature: 0-85°C
- Thermal throttling at 85°C
- Automatic frequency scaling

### Scalability

**Multi-device Support**:
- Up to 8 NPU devices per host
- Inter-device communication via host memory
- Coordinated power management

**Workload Distribution**:
- Automatic load balancing across PEs
- Dynamic resource allocation
- Priority-based scheduling

This architecture provides a robust foundation for neural network acceleration while maintaining flexibility for diverse workloads and future enhancements.