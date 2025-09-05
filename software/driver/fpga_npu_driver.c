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

#define DRIVER_NAME "fpga_npu"
#define DEVICE_NAME "fpga_npu"
#define CLASS_NAME "fpga_npu_class"

// PCI vendor and device IDs (update with actual values)
#define VENDOR_ID 0x10EE  // Xilinx
#define DEVICE_ID 0x7024  // Custom device ID

// BAR definitions
#define CONTROL_BAR 0
#define DATA_BAR 1

// Register offsets
#define REG_CONTROL     0x00
#define REG_STATUS      0x04
#define REG_DATA_ADDR   0x08
#define REG_DATA_SIZE   0x0C
#define REG_INTERRUPT   0x10

// Control register bits
#define CTRL_ENABLE     BIT(0)
#define CTRL_RESET      BIT(1)
#define CTRL_START      BIT(2)

// Status register bits
#define STATUS_READY    BIT(0)
#define STATUS_BUSY     BIT(1)
#define STATUS_ERROR    BIT(2)
#define STATUS_DONE     BIT(3)

struct fpga_npu_dev {
    struct pci_dev *pdev;
    struct cdev cdev;
    struct device *device;
    void __iomem *control_bar;
    void __iomem *data_bar;
    size_t control_bar_size;
    size_t data_bar_size;
    
    // DMA buffer
    void *dma_buffer;
    dma_addr_t dma_handle;
    size_t dma_size;
    
    // Device state
    struct mutex dev_mutex;
    bool device_open;
    
    // Interrupt handling
    int irq;
    wait_queue_head_t wait_queue;
    bool interrupt_received;
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
static irqreturn_t fpga_npu_interrupt(int irq, void *dev_id);

// File operations structure
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = fpga_npu_open,
    .release = fpga_npu_release,
    .read = fpga_npu_read,
    .write = fpga_npu_write,
    .unlocked_ioctl = fpga_npu_ioctl,
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
 * Device ioctl
 */
static long fpga_npu_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct fpga_npu_dev *dev = file->private_data;
    u32 status;
    
    switch (cmd) {
        case 0: // Get status
            status = ioread32(dev->control_bar + REG_STATUS);
            if (copy_to_user((void __user *)arg, &status, sizeof(status))) {
                return -EFAULT;
            }
            break;
        case 1: // Wait for completion
            wait_event_interruptible(dev->wait_queue, dev->interrupt_received);
            dev->interrupt_received = false;
            break;
        default:
            return -ENOTTY;
    }
    
    return 0;
}

/**
 * Module initialization
 */
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