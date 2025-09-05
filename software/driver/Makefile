# Kernel Driver Makefile

# Module information
MODULE_NAME = fpga_npu
obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-objs := fpga_npu_driver.o

# Kernel directory
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

# Cross-compilation support
ARCH ?= x86_64
CROSS_COMPILE ?=

# Build
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

# Clean
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f Module.symvers modules.order

# Install
install: all
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules_install
	depmod -a

# Uninstall
uninstall:
	rm -f /lib/modules/$(shell uname -r)/extra/$(MODULE_NAME).ko
	depmod -a

# Load module
load:
	sudo insmod $(MODULE_NAME).ko

# Unload module
unload:
	sudo rmmod $(MODULE_NAME)

# Device nodes
device_nodes:
	sudo mknod /dev/fpga_npu c $$(grep fpga_npu /proc/devices | cut -d' ' -f1) 0
	sudo chmod 666 /dev/fpga_npu

.PHONY: all clean install uninstall load unload device_nodes