/**
 * Enhanced FPGA NPU Driver Header
 * 
 * Advanced IOCTL definitions, structures, and constants for enhanced driver functionality
 */

#ifndef FPGA_NPU_DRIVER_H
#define FPGA_NPU_DRIVER_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define FPGA_NPU_MAGIC 'N'

/* Device capabilities */
#define NPU_MAX_DEVICES         4
#define NPU_MAX_DMA_BUFFERS     16
#define NPU_MAX_BUFFER_SIZE     (16 * 1024 * 1024)  /* 16MB */
#define NPU_MIN_BUFFER_SIZE     4096                 /* 4KB */

/* Performance counter types */
typedef enum {
    NPU_PERF_CYCLES = 0,
    NPU_PERF_OPERATIONS,
    NPU_PERF_MEMORY_READS,
    NPU_PERF_MEMORY_WRITES,
    NPU_PERF_CACHE_HITS,
    NPU_PERF_CACHE_MISSES,
    NPU_PERF_PIPELINE_STALLS,
    NPU_PERF_POWER_CONSUMPTION,
    NPU_PERF_COUNTER_MAX
} npu_perf_counter_t;

/* NPU operation types */
typedef enum {
    NPU_OP_ADD = 1,
    NPU_OP_SUB = 2,
    NPU_OP_MUL = 3,
    NPU_OP_MAC = 4,
    NPU_OP_CONV = 5,
    NPU_OP_MATMUL = 6,
    NPU_OP_RELU = 7,
    NPU_OP_SIGMOID = 8,
    NPU_OP_POOLING = 9,
    NPU_OP_BATCH_NORM = 10
} npu_operation_t;

/* Data types */
typedef enum {
    NPU_DTYPE_INT8,
    NPU_DTYPE_INT16,
    NPU_DTYPE_INT32,
    NPU_DTYPE_FLOAT16,
    NPU_DTYPE_FLOAT32
} npu_dtype_t;

/* Device information structure */
struct npu_device_info {
    __u32 vendor_id;
    __u32 device_id;
    __u32 revision;
    __u32 pci_bus;
    __u32 pci_device;
    __u32 pci_function;
    char board_name[64];
    __u32 fpga_part;
    __u32 pe_count;
    __u32 max_frequency;
    __u64 memory_size;
    __u32 pcie_generation;
    __u32 pcie_lanes;
};

/* Performance counters structure */
struct npu_performance_counters {
    __u64 counters[NPU_PERF_COUNTER_MAX];
    __u64 timestamp;
    __u32 frequency_mhz;
    __u32 temperature_celsius;
    __u32 power_watts;
    __u32 utilization_percent;
};

/* DMA buffer descriptor */
struct npu_dma_buffer {
    __u32 buffer_id;
    __u64 size;
    __u64 physical_addr;
    __u64 user_addr;
    __u32 flags;
    __u32 reserved[3];
};

/* DMA transfer descriptor */
struct npu_dma_transfer {
    __u32 buffer_id;
    __u64 offset;
    __u64 size;
    __u32 direction;  /* 0: to device, 1: from device */
    __u32 flags;
    __u64 user_addr;
    __u32 timeout_ms;
    __u32 reserved[2];
};

/* NPU instruction structure */
struct npu_instruction {
    npu_operation_t operation;
    __u32 src1_addr;
    __u32 src2_addr;
    __u32 dst_addr;
    __u32 size;
    __u32 params[8];  /* Operation-specific parameters */
    __u32 flags;
    __u32 reserved[3];
};

/* Batch instruction execution */
struct npu_instruction_batch {
    struct npu_instruction *instructions;
    __u32 count;
    __u32 flags;
    __u32 timeout_ms;
    __u32 reserved[5];
};

/* Memory mapping request */
struct npu_mmap_request {
    __u64 size;
    __u32 flags;
    __u32 buffer_id;
    __u64 reserved[2];
};

/* Device configuration */
struct npu_device_config {
    __u32 pe_enable_mask;    /* Bitmask of enabled processing elements */
    __u32 clock_frequency;   /* Target clock frequency in MHz */
    __u32 power_mode;        /* 0: performance, 1: balanced, 2: power_save */
    __u32 cache_policy;      /* 0: write-through, 1: write-back */
    __u32 debug_level;       /* Debug output level */
    __u32 reserved[3];
};

/* Error information */
struct npu_error_info {
    __u32 error_code;
    __u32 error_count;
    __u64 timestamp;
    char description[128];
    __u32 register_dump[16];
};

/* Temperature and power monitoring */
struct npu_thermal_info {
    __u32 temperature_celsius;
    __u32 power_consumption_mw;
    __u32 thermal_state;     /* 0: normal, 1: warning, 2: critical */
    __u32 throttling_active;
    __u32 fan_speed_rpm;
    __u32 reserved[3];
};

/* IOCTL command definitions */

/* Device management */
#define NPU_IOCTL_GET_DEVICE_INFO    _IOR(FPGA_NPU_MAGIC, 0x01, struct npu_device_info)
#define NPU_IOCTL_SET_CONFIG         _IOW(FPGA_NPU_MAGIC, 0x02, struct npu_device_config)
#define NPU_IOCTL_GET_CONFIG         _IOR(FPGA_NPU_MAGIC, 0x03, struct npu_device_config)
#define NPU_IOCTL_RESET_DEVICE       _IO(FPGA_NPU_MAGIC, 0x04)

/* Status and monitoring */
#define NPU_IOCTL_GET_STATUS         _IOR(FPGA_NPU_MAGIC, 0x10, __u32)
#define NPU_IOCTL_GET_PERF_COUNTERS  _IOR(FPGA_NPU_MAGIC, 0x11, struct npu_performance_counters)
#define NPU_IOCTL_RESET_PERF_COUNTERS _IO(FPGA_NPU_MAGIC, 0x12)
#define NPU_IOCTL_GET_ERROR_INFO     _IOR(FPGA_NPU_MAGIC, 0x13, struct npu_error_info)
#define NPU_IOCTL_GET_THERMAL_INFO   _IOR(FPGA_NPU_MAGIC, 0x14, struct npu_thermal_info)

/* Memory management */
#define NPU_IOCTL_ALLOC_BUFFER       _IOWR(FPGA_NPU_MAGIC, 0x20, struct npu_dma_buffer)
#define NPU_IOCTL_FREE_BUFFER        _IOW(FPGA_NPU_MAGIC, 0x21, __u32)
#define NPU_IOCTL_GET_BUFFER_INFO    _IOWR(FPGA_NPU_MAGIC, 0x22, struct npu_dma_buffer)
#define NPU_IOCTL_MMAP_REQUEST       _IOWR(FPGA_NPU_MAGIC, 0x23, struct npu_mmap_request)

/* DMA operations */
#define NPU_IOCTL_DMA_TRANSFER       _IOW(FPGA_NPU_MAGIC, 0x30, struct npu_dma_transfer)
#define NPU_IOCTL_DMA_SYNC           _IOW(FPGA_NPU_MAGIC, 0x31, __u32)
#define NPU_IOCTL_DMA_ABORT          _IOW(FPGA_NPU_MAGIC, 0x32, __u32)

/* Instruction execution */
#define NPU_IOCTL_EXECUTE_INSTRUCTION _IOW(FPGA_NPU_MAGIC, 0x40, struct npu_instruction)
#define NPU_IOCTL_EXECUTE_BATCH      _IOW(FPGA_NPU_MAGIC, 0x41, struct npu_instruction_batch)
#define NPU_IOCTL_WAIT_COMPLETION    _IOW(FPGA_NPU_MAGIC, 0x42, __u32)

/* Debug and development */
#define NPU_IOCTL_READ_REGISTER      _IOWR(FPGA_NPU_MAGIC, 0x50, struct { __u32 offset; __u32 value; })
#define NPU_IOCTL_WRITE_REGISTER     _IOW(FPGA_NPU_MAGIC, 0x51, struct { __u32 offset; __u32 value; })
#define NPU_IOCTL_DUMP_REGISTERS     _IOR(FPGA_NPU_MAGIC, 0x52, __u32[64])

/* Buffer flags */
#define NPU_BUFFER_FLAG_COHERENT     BIT(0)  /* CPU coherent buffer */
#define NPU_BUFFER_FLAG_STREAMING    BIT(1)  /* Streaming DMA buffer */
#define NPU_BUFFER_FLAG_READONLY     BIT(2)  /* Read-only buffer */
#define NPU_BUFFER_FLAG_WRITEONLY    BIT(3)  /* Write-only buffer */

/* DMA transfer flags */
#define NPU_DMA_FLAG_BLOCKING        BIT(0)  /* Blocking transfer */
#define NPU_DMA_FLAG_INTERRUPT       BIT(1)  /* Generate interrupt on completion */
#define NPU_DMA_FLAG_COHERENT        BIT(2)  /* Maintain cache coherency */

/* Instruction flags */
#define NPU_INST_FLAG_ASYNC          BIT(0)  /* Asynchronous execution */
#define NPU_INST_FLAG_HIGH_PRIORITY  BIT(1)  /* High priority execution */
#define NPU_INST_FLAG_PROFILE        BIT(2)  /* Enable profiling */

/* Status register bits */
#define NPU_STATUS_READY             BIT(0)
#define NPU_STATUS_BUSY              BIT(1)
#define NPU_STATUS_ERROR             BIT(2)
#define NPU_STATUS_DONE              BIT(3)
#define NPU_STATUS_THERMAL_WARNING   BIT(4)
#define NPU_STATUS_POWER_WARNING     BIT(5)

/* Error codes */
#define NPU_ERROR_SUCCESS            0
#define NPU_ERROR_INVALID_PARAM      1
#define NPU_ERROR_NO_MEMORY          2
#define NPU_ERROR_TIMEOUT            3
#define NPU_ERROR_DEVICE_BUSY        4
#define NPU_ERROR_DEVICE_ERROR       5
#define NPU_ERROR_DMA_ERROR          6
#define NPU_ERROR_THERMAL_LIMIT      7
#define NPU_ERROR_POWER_LIMIT        8

#endif /* FPGA_NPU_DRIVER_H */