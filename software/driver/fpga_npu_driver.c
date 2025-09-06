/**
 * FPGA NPU PCIe Driver
 * 
 * Linux kernel driver for FPGA-based Neural Processing Unit with PCIe interface.
 * Provides device file interface for user-space applications.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include "fpga_npu_enhanced.h"

#define DRIVER_NAME "fpga_npu"
#define DEVICE_NAME "fpga_npu"
#define CLASS_NAME "fpga_npu_class"

// PCI vendor and device IDs (update with actual values)
#define VENDOR_ID 0x10EE  // Xilinx
#define DEVICE_ID 0x7024  // Custom device ID

// BAR definitions
#define CONTROL_BAR 0
#define DATA_BAR 1

// Enhanced register offsets
#define REG_CONTROL     0x00
#define REG_STATUS      0x04
#define REG_DATA_ADDR   0x08
#define REG_DATA_SIZE   0x0C
#define REG_INTERRUPT   0x10
#define REG_PERF_CTRL   0x14
#define REG_PERF_CYCLES 0x18
#define REG_PERF_OPS    0x1C
#define REG_TEMPERATURE 0x20
#define REG_POWER       0x24
#define REG_CONFIG      0x28
#define REG_ERROR       0x2C
#define REG_DMA_CTRL    0x30
#define REG_DMA_SRC     0x34
#define REG_DMA_DST     0x38
#define REG_DMA_SIZE    0x3C

// Control register bits
#define CTRL_ENABLE     BIT(0)
#define CTRL_RESET      BIT(1)
#define CTRL_START      BIT(2)

// Status register bits
#define STATUS_READY    BIT(0)
#define STATUS_BUSY     BIT(1)
#define STATUS_ERROR    BIT(2)
#define STATUS_DONE     BIT(3)

// DMA transfer states
typedef enum {
    DMA_STATE_IDLE,
    DMA_STATE_SETUP,
    DMA_STATE_RUNNING,
    DMA_STATE_COMPLETE,
    DMA_STATE_ERROR
} dma_state_t;

// DMA buffer structure
struct npu_dma_buf {
    void *cpu_addr;
    dma_addr_t dma_handle;
    size_t size;
    u32 flags;
    struct list_head list;
    u32 buffer_id;
    atomic_t ref_count;
};

// DMA transfer context
struct npu_dma_context {
    struct npu_dma_buf *buffer;
    dma_state_t state;
    struct completion completion;
    u32 direction;
    size_t transferred;
    int error_code;
};
struct fpga_npu_dev {
    struct pci_dev *pdev;
    struct cdev cdev;
    struct device *device;
    void __iomem *control_bar;
    void __iomem *data_bar;
    size_t control_bar_size;
    size_t data_bar_size;
    
    // Enhanced DMA buffer management
    void *dma_buffer;
    dma_addr_t dma_handle;
    size_t dma_size;
    struct list_head dma_buffers;
    spinlock_t dma_lock;
    u32 next_buffer_id;
    
    // Device state
    struct mutex dev_mutex;
    bool device_open;
    atomic_t ref_count;
    
    // Enhanced interrupt handling
    int irq;
    wait_queue_head_t wait_queue;
    bool interrupt_received;
    u32 interrupt_status;
    
    // Performance monitoring
    struct npu_performance_counters perf_counters;
    spinlock_t perf_lock;
    
    // Error tracking
    struct npu_error_info error_info;
    
    // Configuration
    struct npu_device_config config;
    
    // DMA transfer management
    struct npu_dma_context dma_ctx[NPU_MAX_DMA_BUFFERS];
    struct workqueue_struct *dma_workqueue;
    
    // Memory mapping
    struct vm_area_struct *vma;
    
    // Thermal monitoring
    struct timer_list thermal_timer;
    struct npu_thermal_info thermal_info;
};

static struct fpga_npu_dev *npu_device;
static dev_t dev_number;
static struct class *npu_class;
static int major_number;

// PCI device table
static const struct pci_device_id fpga_npu_ids[] = {
    { PCI_DEVICE(VENDOR_ID, DEVICE_ID) },
    { 0, }
};
MODULE_DEVICE_TABLE(pci, fpga_npu_ids);

// Function prototypes
static int fpga_npu_probe(struct pci_dev *pdev, const struct pci_device_id *id);
static void fpga_npu_remove(struct pci_dev *pdev);
static int fpga_npu_open(struct inode *inode, struct file *file);
static int fpga_npu_release(struct inode *inode, struct file *file);
static ssize_t fpga_npu_read(struct file *file, char __user *buffer, size_t len, loff_t *offset);
static ssize_t fpga_npu_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset);
static long fpga_npu_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int fpga_npu_mmap(struct file *file, struct vm_area_struct *vma);
static unsigned int fpga_npu_poll(struct file *file, poll_table *wait);
static irqreturn_t fpga_npu_interrupt(int irq, void *dev_id);

// Enhanced function prototypes
static int npu_alloc_dma_buffer(struct fpga_npu_dev *dev, struct npu_dma_buffer *buf_desc);
static int npu_free_dma_buffer(struct fpga_npu_dev *dev, u32 buffer_id);
static int npu_dma_transfer(struct fpga_npu_dev *dev, struct npu_dma_transfer *transfer);
static int npu_execute_instruction(struct fpga_npu_dev *dev, struct npu_instruction *inst);
static int npu_get_performance_counters(struct fpga_npu_dev *dev, struct npu_performance_counters *perf);
static void npu_thermal_monitor(struct timer_list *timer);
static void npu_dma_work_handler(struct work_struct *work);

// Enhanced file operations structure
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = fpga_npu_open,
    .release = fpga_npu_release,
    .read = fpga_npu_read,
    .write = fpga_npu_write,
    .unlocked_ioctl = fpga_npu_ioctl,
    .mmap = fpga_npu_mmap,
    .poll = fpga_npu_poll,
    .llseek = no_llseek,
};

// PCI driver structure
static struct pci_driver fpga_npu_driver = {
    .name = DRIVER_NAME,
    .id_table = fpga_npu_ids,
    .probe = fpga_npu_probe,
    .remove = fpga_npu_remove,
};

/**
 * PCI probe function - called when device is detected
 */
static int fpga_npu_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int ret;
    
    printk(KERN_INFO "FPGA NPU: Probing device %04x:%04x\n", pdev->vendor, pdev->device);
    
    // Allocate device structure
    npu_device = kzalloc(sizeof(struct fpga_npu_dev), GFP_KERNEL);
    if (!npu_device) {
        return -ENOMEM;
    }
    
    npu_device->pdev = pdev;
    mutex_init(&npu_device->dev_mutex);
    init_waitqueue_head(&npu_device->wait_queue);
    
    // Initialize enhanced features
    INIT_LIST_HEAD(&npu_device->dma_buffers);
    spin_lock_init(&npu_device->dma_lock);
    spin_lock_init(&npu_device->perf_lock);
    npu_device->next_buffer_id = 1;
    atomic_set(&npu_device->ref_count, 0);
    
    // Initialize thermal monitoring
    timer_setup(&npu_device->thermal_timer, npu_thermal_monitor, 0);
    
    // Initialize performance counters
    memset(&npu_device->perf_counters, 0, sizeof(npu_device->perf_counters));
    
    // Set default configuration
    npu_device->config.pe_enable_mask = 0xFFFF;  // All PEs enabled
    npu_device->config.clock_frequency = 300;
    npu_device->config.power_mode = 0;  // Performance mode
    npu_device->config.cache_policy = 1;  // Write-back
    
    // Enable PCI device
    ret = pci_enable_device(pdev);
    if (ret) {
        printk(KERN_ERR "FPGA NPU: Failed to enable PCI device\n");
        goto err_free_dev;
    }
    
    // Request PCI regions
    ret = pci_request_regions(pdev, DRIVER_NAME);
    if (ret) {
        printk(KERN_ERR "FPGA NPU: Failed to request PCI regions\n");
        goto err_disable_device;
    }
    
    // Map control BAR
    npu_device->control_bar_size = pci_resource_len(pdev, CONTROL_BAR);
    npu_device->control_bar = pci_iomap(pdev, CONTROL_BAR, npu_device->control_bar_size);
    if (!npu_device->control_bar) {
        printk(KERN_ERR "FPGA NPU: Failed to map control BAR\n");
        ret = -ENOMEM;
        goto err_release_regions;
    }
    
    // Map data BAR
    npu_device->data_bar_size = pci_resource_len(pdev, DATA_BAR);
    npu_device->data_bar = pci_iomap(pdev, DATA_BAR, npu_device->data_bar_size);
    if (!npu_device->data_bar) {
        printk(KERN_ERR "FPGA NPU: Failed to map data BAR\n");
        ret = -ENOMEM;
        goto err_unmap_control;
    }
    
    // Set up DMA
    ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
    if (ret) {
        ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
        if (ret) {
            printk(KERN_ERR "FPGA NPU: Failed to set DMA mask\n");
            goto err_unmap_data;
        }
    }
    
    pci_set_master(pdev);
    
    // Allocate DMA buffer
    npu_device->dma_size = PAGE_SIZE * 16; // 64KB
    npu_device->dma_buffer = dma_alloc_coherent(&pdev->dev, npu_device->dma_size,
                                               &npu_device->dma_handle, GFP_KERNEL);
    if (!npu_device->dma_buffer) {
        printk(KERN_ERR "FPGA NPU: Failed to allocate DMA buffer\n");
        ret = -ENOMEM;
        goto err_unmap_data;
    }
    
    // Request interrupt
    ret = request_irq(pdev->irq, fpga_npu_interrupt, IRQF_SHARED, DRIVER_NAME, npu_device);
    if (ret) {
        printk(KERN_ERR "FPGA NPU: Failed to request interrupt\n");
        goto err_free_dma;
    }
    npu_device->irq = pdev->irq;
    
    // Initialize device
    iowrite32(CTRL_RESET, npu_device->control_bar + REG_CONTROL);
    msleep(10);
    iowrite32(CTRL_ENABLE, npu_device->control_bar + REG_CONTROL);
    
    // Create character device
    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "FPGA NPU: Failed to allocate device number\n");
        goto err_free_irq;
    }
    major_number = MAJOR(dev_number);
    
    cdev_init(&npu_device->cdev, &fops);
    npu_device->cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&npu_device->cdev, dev_number, 1);
    if (ret) {
        printk(KERN_ERR "FPGA NPU: Failed to add character device\n");
        goto err_unregister_chrdev;
    }
    
    // Create device class
    npu_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(npu_class)) {
        printk(KERN_ERR "FPGA NPU: Failed to create device class\n");
        ret = PTR_ERR(npu_class);
        goto err_cdev_del;
    }
    
    // Create device
    npu_device->device = device_create(npu_class, NULL, dev_number, NULL, DEVICE_NAME);
    if (IS_ERR(npu_device->device)) {
        printk(KERN_ERR "FPGA NPU: Failed to create device\n");
        ret = PTR_ERR(npu_device->device);
        goto err_class_destroy;
    }
    
    pci_set_drvdata(pdev, npu_device);
    
    printk(KERN_INFO "FPGA NPU: Device initialized successfully\n");
    return 0;
    
err_class_destroy:
    class_destroy(npu_class);
err_cdev_del:
    cdev_del(&npu_device->cdev);
err_unregister_chrdev:
    unregister_chrdev_region(dev_number, 1);
err_free_irq:
    free_irq(npu_device->irq, npu_device);
err_free_dma:
    dma_free_coherent(&pdev->dev, npu_device->dma_size, npu_device->dma_buffer, npu_device->dma_handle);
err_unmap_data:
    pci_iounmap(pdev, npu_device->data_bar);
err_unmap_control:
    pci_iounmap(pdev, npu_device->control_bar);
err_release_regions:
    pci_release_regions(pdev);
err_disable_device:
    pci_disable_device(pdev);
err_free_dev:
    kfree(npu_device);
    return ret;
}

/**
 * PCI remove function - called when device is removed
 */
static void fpga_npu_remove(struct pci_dev *pdev)
{
    struct fpga_npu_dev *dev = pci_get_drvdata(pdev);
    
    printk(KERN_INFO "FPGA NPU: Removing device\n");
    
    // Disable device
    iowrite32(0, dev->control_bar + REG_CONTROL);
    
    // Clean up
    device_destroy(npu_class, dev_number);
    class_destroy(npu_class);
    cdev_del(&dev->cdev);
    unregister_chrdev_region(dev_number, 1);
    free_irq(dev->irq, dev);
    dma_free_coherent(&pdev->dev, dev->dma_size, dev->dma_buffer, dev->dma_handle);
    pci_iounmap(pdev, dev->data_bar);
    pci_iounmap(pdev, dev->control_bar);
    pci_release_regions(pdev);
    pci_disable_device(pdev);
    kfree(dev);
}

/**
 * Interrupt handler
 */
static irqreturn_t fpga_npu_interrupt(int irq, void *dev_id)
{
    struct fpga_npu_dev *dev = (struct fpga_npu_dev *)dev_id;
    u32 status;
    
    status = ioread32(dev->control_bar + REG_STATUS);
    if (!(status & STATUS_DONE)) {
        return IRQ_NONE; // Not our interrupt
    }
    
    // Clear interrupt
    iowrite32(status, dev->control_bar + REG_STATUS);
    
    dev->interrupt_received = true;
    wake_up_interruptible(&dev->wait_queue);
    
    return IRQ_HANDLED;
}

/**
 * Device open
 */
static int fpga_npu_open(struct inode *inode, struct file *file)
{
    if (mutex_lock_interruptible(&npu_device->dev_mutex)) {
        return -ERESTARTSYS;
    }
    
    if (npu_device->device_open) {
        mutex_unlock(&npu_device->dev_mutex);
        return -EBUSY;
    }
    
    npu_device->device_open = true;
    file->private_data = npu_device;
    
    mutex_unlock(&npu_device->dev_mutex);
    
    printk(KERN_INFO "FPGA NPU: Device opened\n");
    return 0;
}

/**
 * Device release
 */
static int fpga_npu_release(struct inode *inode, struct file *file)
{
    mutex_lock(&npu_device->dev_mutex);
    npu_device->device_open = false;
    mutex_unlock(&npu_device->dev_mutex);
    
    printk(KERN_INFO "FPGA NPU: Device released\n");
    return 0;
}

/**
 * Device read
 */
static ssize_t fpga_npu_read(struct file *file, char __user *buffer, size_t len, loff_t *offset)
{
    struct fpga_npu_dev *dev = file->private_data;
    ssize_t bytes_read = 0;
    
    if (len > dev->dma_size) {
        len = dev->dma_size;
    }
    
    if (copy_to_user(buffer, dev->dma_buffer, len)) {
        return -EFAULT;
    }
    
    bytes_read = len;
    return bytes_read;
}

/**
 * Device write
 */
static ssize_t fpga_npu_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset)
{
    struct fpga_npu_dev *dev = file->private_data;
    ssize_t bytes_written = 0;
    
    if (len > dev->dma_size) {
        len = dev->dma_size;
    }
    
    if (copy_from_user(dev->dma_buffer, buffer, len)) {
        return -EFAULT;
    }
    
    // Start NPU processing
    iowrite32((u32)dev->dma_handle, dev->control_bar + REG_DATA_ADDR);
    iowrite32((u32)len, dev->control_bar + REG_DATA_SIZE);
    iowrite32(CTRL_ENABLE | CTRL_START, dev->control_bar + REG_CONTROL);
    
    bytes_written = len;
    return bytes_written;
}

/**
 * Enhanced device ioctl with comprehensive command support
 */
static long fpga_npu_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct fpga_npu_dev *dev = file->private_data;
    int ret = 0;
    
    if (!dev) {
        return -ENODEV;
    }
    
    // Verify IOCTL magic number
    if (_IOC_TYPE(cmd) != FPGA_NPU_MAGIC) {
        return -ENOTTY;
    }
    
    switch (cmd) {
        case NPU_IOCTL_GET_DEVICE_INFO: {
            struct npu_device_info info = {
                .vendor_id = dev->pdev->vendor,
                .device_id = dev->pdev->device,
                .revision = dev->pdev->revision,
                .pci_bus = dev->pdev->bus->number,
                .pci_device = PCI_SLOT(dev->pdev->devfn),
                .pci_function = PCI_FUNC(dev->pdev->devfn),
                .pe_count = 16,
                .max_frequency = 300,
                .memory_size = dev->dma_size,
                .pcie_generation = 3,
                .pcie_lanes = 4
            };
            strcpy(info.board_name, "FPGA NPU Board");
            
            if (copy_to_user((void __user *)arg, &info, sizeof(info))) {
                ret = -EFAULT;
            }
            break;
        }
        
        case NPU_IOCTL_GET_STATUS: {
            u32 status = ioread32(dev->control_bar + REG_STATUS);
            if (copy_to_user((void __user *)arg, &status, sizeof(status))) {
                ret = -EFAULT;
            }
            break;
        }
        
        case NPU_IOCTL_GET_PERF_COUNTERS: {
            ret = npu_get_performance_counters(dev, (struct npu_performance_counters *)arg);
            break;
        }
        
        case NPU_IOCTL_RESET_PERF_COUNTERS: {
            spin_lock(&dev->perf_lock);
            memset(&dev->perf_counters, 0, sizeof(dev->perf_counters));
            iowrite32(1, dev->control_bar + REG_PERF_CTRL);
            spin_unlock(&dev->perf_lock);
            break;
        }
        
        case NPU_IOCTL_ALLOC_BUFFER: {
            struct npu_dma_buffer buf_desc;
            if (copy_from_user(&buf_desc, (void __user *)arg, sizeof(buf_desc))) {
                ret = -EFAULT;
                break;
            }
            ret = npu_alloc_dma_buffer(dev, &buf_desc);
            if (ret == 0) {
                if (copy_to_user((void __user *)arg, &buf_desc, sizeof(buf_desc))) {
                    ret = -EFAULT;
                }
            }
            break;
        }
        
        case NPU_IOCTL_FREE_BUFFER: {
            u32 buffer_id;
            if (copy_from_user(&buffer_id, (void __user *)arg, sizeof(buffer_id))) {
                ret = -EFAULT;
                break;
            }
            ret = npu_free_dma_buffer(dev, buffer_id);
            break;
        }
        
        case NPU_IOCTL_DMA_TRANSFER: {
            struct npu_dma_transfer transfer;
            if (copy_from_user(&transfer, (void __user *)arg, sizeof(transfer))) {
                ret = -EFAULT;
                break;
            }
            ret = npu_dma_transfer(dev, &transfer);
            break;
        }
        
        case NPU_IOCTL_EXECUTE_INSTRUCTION: {
            struct npu_instruction inst;
            if (copy_from_user(&inst, (void __user *)arg, sizeof(inst))) {
                ret = -EFAULT;
                break;
            }
            ret = npu_execute_instruction(dev, &inst);
            break;
        }
        
        case NPU_IOCTL_WAIT_COMPLETION: {
            u32 timeout_ms;
            if (copy_from_user(&timeout_ms, (void __user *)arg, sizeof(timeout_ms))) {
                ret = -EFAULT;
                break;
            }
            
            if (timeout_ms == 0) {
                wait_event_interruptible(dev->wait_queue, dev->interrupt_received);
            } else {
                ret = wait_event_interruptible_timeout(dev->wait_queue, 
                                                       dev->interrupt_received,
                                                       msecs_to_jiffies(timeout_ms));
                if (ret == 0) {
                    ret = -ETIMEDOUT;
                } else if (ret > 0) {
                    ret = 0;
                }
            }
            dev->interrupt_received = false;
            break;
        }
        
        case NPU_IOCTL_SET_CONFIG: {
            struct npu_device_config config;
            if (copy_from_user(&config, (void __user *)arg, sizeof(config))) {
                ret = -EFAULT;
                break;
            }
            
            mutex_lock(&dev->dev_mutex);
            dev->config = config;
            // Apply configuration to hardware
            iowrite32(config.pe_enable_mask, dev->control_bar + REG_CONFIG);
            mutex_unlock(&dev->dev_mutex);
            break;
        }
        
        case NPU_IOCTL_GET_CONFIG: {
            if (copy_to_user((void __user *)arg, &dev->config, sizeof(dev->config))) {
                ret = -EFAULT;
            }
            break;
        }
        
        case NPU_IOCTL_RESET_DEVICE: {
            mutex_lock(&dev->dev_mutex);
            iowrite32(CTRL_RESET, dev->control_bar + REG_CONTROL);
            msleep(10);
            iowrite32(CTRL_ENABLE, dev->control_bar + REG_CONTROL);
            mutex_unlock(&dev->dev_mutex);
            break;
        }
        
        case NPU_IOCTL_GET_ERROR_INFO: {
            if (copy_to_user((void __user *)arg, &dev->error_info, sizeof(dev->error_info))) {
                ret = -EFAULT;
            }
            break;
        }
        
        case NPU_IOCTL_GET_THERMAL_INFO: {
            if (copy_to_user((void __user *)arg, &dev->thermal_info, sizeof(dev->thermal_info))) {
                ret = -EFAULT;
            }
            break;
        }
        
        default:
            ret = -ENOTTY;
            break;
    }
    
    return ret;
}

/**
 * Enhanced memory mapping support
 */
static int fpga_npu_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct fpga_npu_dev *dev = file->private_data;
    unsigned long size = vma->vm_end - vma->vm_start;
    unsigned long pfn;
    
    if (!dev) {
        return -ENODEV;
    }
    
    // Ensure the mapping size doesn't exceed our DMA buffer
    if (size > dev->dma_size) {
        return -EINVAL;
    }
    
    // Set up the mapping properties
    vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    
    // Map the DMA buffer to user space
    pfn = virt_to_phys(dev->dma_buffer) >> PAGE_SHIFT;
    
    if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
        return -EAGAIN;
    }
    
    dev->vma = vma;
    return 0;
}

/**
 * Poll support for asynchronous operations
 */
static unsigned int fpga_npu_poll(struct file *file, poll_table *wait)
{
    struct fpga_npu_dev *dev = file->private_data;
    unsigned int mask = 0;
    
    if (!dev) {
        return POLLERR;
    }
    
    poll_wait(file, &dev->wait_queue, wait);
    
    if (dev->interrupt_received) {
        mask |= POLLIN | POLLRDNORM;
    }
    
    // Check if device is ready for new operations
    u32 status = ioread32(dev->control_bar + REG_STATUS);
    if (status & STATUS_READY) {
        mask |= POLLOUT | POLLWRNORM;
    }
    
    return mask;
}

/**
 * Allocate DMA buffer for NPU operations
 */
static int npu_alloc_dma_buffer(struct fpga_npu_dev *dev, struct npu_dma_buffer *buf_desc)
{
    struct npu_dma_buf *buf;
    unsigned long flags;
    
    if (buf_desc->size < NPU_MIN_BUFFER_SIZE || buf_desc->size > NPU_MAX_BUFFER_SIZE) {
        return -EINVAL;
    }
    
    buf = kzalloc(sizeof(*buf), GFP_KERNEL);
    if (!buf) {
        return -ENOMEM;
    }
    
    // Allocate DMA coherent memory
    if (buf_desc->flags & NPU_BUFFER_FLAG_COHERENT) {
        buf->cpu_addr = dma_alloc_coherent(&dev->pdev->dev, buf_desc->size,
                                          &buf->dma_handle, GFP_KERNEL);
    } else {
        buf->cpu_addr = dma_alloc_coherent(&dev->pdev->dev, buf_desc->size,
                                          &buf->dma_handle, GFP_KERNEL | __GFP_NOWARN);
    }
    
    if (!buf->cpu_addr) {
        kfree(buf);
        return -ENOMEM;
    }
    
    buf->size = buf_desc->size;
    buf->flags = buf_desc->flags;
    atomic_set(&buf->ref_count, 1);
    
    spin_lock_irqsave(&dev->dma_lock, flags);
    buf->buffer_id = dev->next_buffer_id++;
    list_add_tail(&buf->list, &dev->dma_buffers);
    spin_unlock_irqrestore(&dev->dma_lock, flags);
    
    // Fill in the buffer descriptor
    buf_desc->buffer_id = buf->buffer_id;
    buf_desc->physical_addr = buf->dma_handle;
    buf_desc->user_addr = (u64)buf->cpu_addr;
    
    dev_dbg(dev->device, "Allocated DMA buffer ID %u, size %llu\n",
            buf->buffer_id, buf_desc->size);
    
    return 0;
}

/**
 * Free DMA buffer
 */
static int npu_free_dma_buffer(struct fpga_npu_dev *dev, u32 buffer_id)
{
    struct npu_dma_buf *buf, *tmp;
    unsigned long flags;
    bool found = false;
    
    spin_lock_irqsave(&dev->dma_lock, flags);
    list_for_each_entry_safe(buf, tmp, &dev->dma_buffers, list) {
        if (buf->buffer_id == buffer_id) {
            list_del(&buf->list);
            found = true;
            break;
        }
    }
    spin_unlock_irqrestore(&dev->dma_lock, flags);
    
    if (!found) {
        return -ENOENT;
    }
    
    if (atomic_dec_and_test(&buf->ref_count)) {
        dma_free_coherent(&dev->pdev->dev, buf->size, buf->cpu_addr, buf->dma_handle);
        kfree(buf);
        dev_dbg(dev->device, "Freed DMA buffer ID %u\n", buffer_id);
    }
    
    return 0;
}

/**
 * Execute DMA transfer
 */
static int npu_dma_transfer(struct fpga_npu_dev *dev, struct npu_dma_transfer *transfer)
{
    struct npu_dma_buf *buf = NULL;
    struct npu_dma_buf *tmp;
    unsigned long flags;
    int ret = 0;
    
    // Find the buffer
    spin_lock_irqsave(&dev->dma_lock, flags);
    list_for_each_entry(tmp, &dev->dma_buffers, list) {
        if (tmp->buffer_id == transfer->buffer_id) {
            buf = tmp;
            atomic_inc(&buf->ref_count);
            break;
        }
    }
    spin_unlock_irqrestore(&dev->dma_lock, flags);
    
    if (!buf) {
        return -ENOENT;
    }
    
    // Validate transfer parameters
    if (transfer->offset + transfer->size > buf->size) {
        ret = -EINVAL;
        goto out;
    }
    
    // Set up DMA transfer
    iowrite32((u32)(buf->dma_handle + transfer->offset), dev->control_bar + REG_DMA_SRC);
    iowrite32((u32)transfer->user_addr, dev->control_bar + REG_DMA_DST);
    iowrite32((u32)transfer->size, dev->control_bar + REG_DMA_SIZE);
    
    // Start transfer
    u32 dma_ctrl = transfer->direction | (transfer->flags << 8);
    iowrite32(dma_ctrl, dev->control_bar + REG_DMA_CTRL);
    
    // Wait for completion if blocking
    if (transfer->flags & NPU_DMA_FLAG_BLOCKING) {
        if (transfer->timeout_ms > 0) {
            ret = wait_event_interruptible_timeout(dev->wait_queue,
                                                   dev->interrupt_received,
                                                   msecs_to_jiffies(transfer->timeout_ms));
            if (ret == 0) {
                ret = -ETIMEDOUT;
            } else if (ret > 0) {
                ret = 0;
            }
        } else {
            wait_event_interruptible(dev->wait_queue, dev->interrupt_received);
        }
        dev->interrupt_received = false;
    }
    
out:
    atomic_dec(&buf->ref_count);
    return ret;
}

/**
 * Execute NPU instruction
 */
static int npu_execute_instruction(struct fpga_npu_dev *dev, struct npu_instruction *inst)
{
    u32 instruction_word;
    
    // Pack instruction into 32-bit word
    instruction_word = ((u32)inst->operation << 24) |
                      ((inst->src1_addr & 0xFF) << 16) |
                      ((inst->src2_addr & 0xFF) << 8) |
                      (inst->dst_addr & 0xFF);
    
    // Write instruction to device
    iowrite32(instruction_word, dev->control_bar + REG_DATA_ADDR);
    iowrite32(inst->size, dev->control_bar + REG_DATA_SIZE);
    
    // Start execution
    u32 ctrl = CTRL_ENABLE | CTRL_START;
    if (inst->flags & NPU_INST_FLAG_HIGH_PRIORITY) {
        ctrl |= BIT(8);
    }
    iowrite32(ctrl, dev->control_bar + REG_CONTROL);
    
    // Update performance counters
    if (inst->flags & NPU_INST_FLAG_PROFILE) {
        spin_lock(&dev->perf_lock);
        dev->perf_counters.counters[NPU_PERF_OPERATIONS]++;
        spin_unlock(&dev->perf_lock);
    }
    
    return 0;
}

/**
 * Get performance counters
 */
static int npu_get_performance_counters(struct fpga_npu_dev *dev, struct npu_performance_counters *perf)
{
    unsigned long flags;
    
    spin_lock_irqsave(&dev->perf_lock, flags);
    
    // Read hardware performance counters
    dev->perf_counters.counters[NPU_PERF_CYCLES] = 
        ((u64)ioread32(dev->control_bar + REG_PERF_CYCLES + 4) << 32) |
        ioread32(dev->control_bar + REG_PERF_CYCLES);
    
    dev->perf_counters.timestamp = ktime_get_ns();
    dev->perf_counters.frequency_mhz = 300;  // From config
    dev->perf_counters.temperature_celsius = ioread32(dev->control_bar + REG_TEMPERATURE);
    dev->perf_counters.power_watts = ioread32(dev->control_bar + REG_POWER);
    
    memcpy(perf, &dev->perf_counters, sizeof(*perf));
    spin_unlock_irqrestore(&dev->perf_lock, flags);
    
    if (copy_to_user((void __user *)perf, &dev->perf_counters, sizeof(*perf))) {
        return -EFAULT;
    }
    
    return 0;
}

/**
 * Thermal monitoring timer
 */
static void npu_thermal_monitor(struct timer_list *timer)
{
    struct fpga_npu_dev *dev = from_timer(dev, timer, thermal_timer);
    
    dev->thermal_info.temperature_celsius = ioread32(dev->control_bar + REG_TEMPERATURE);
    dev->thermal_info.power_consumption_mw = ioread32(dev->control_bar + REG_POWER);
    
    // Check thermal limits
    if (dev->thermal_info.temperature_celsius > 85) {
        dev->thermal_info.thermal_state = 2;  // Critical
        dev_warn(dev->device, "Critical temperature: %u°C\n",
                dev->thermal_info.temperature_celsius);
    } else if (dev->thermal_info.temperature_celsius > 75) {
        dev->thermal_info.thermal_state = 1;  // Warning
    } else {
        dev->thermal_info.thermal_state = 0;  // Normal
    }
    
    // Reschedule timer
    mod_timer(&dev->thermal_timer, jiffies + msecs_to_jiffies(5000));
}
static int __init fpga_npu_init(void)
{
    int result;
    
    printk(KERN_INFO "FPGA NPU: Loading driver\n");
    
    result = pci_register_driver(&fpga_npu_driver);
    if (result) {
        printk(KERN_ERR "FPGA NPU: Failed to register PCI driver\n");
        return result;
    }
    
    printk(KERN_INFO "FPGA NPU: Driver loaded successfully\n");
    return 0;
}

/**
 * Module cleanup
 */
static void __exit fpga_npu_exit(void)
{
    printk(KERN_INFO "FPGA NPU: Unloading driver\n");
    pci_unregister_driver(&fpga_npu_driver);
    printk(KERN_INFO "FPGA NPU: Driver unloaded\n");
}

module_init(fpga_npu_init);
module_exit(fpga_npu_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Naqib Annur");
MODULE_DESCRIPTION("FPGA NPU PCIe Driver");
MODULE_VERSION("1.0");