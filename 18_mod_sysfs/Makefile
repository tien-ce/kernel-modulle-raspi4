obj-m := modbus_device_sysfs.o
dt_source = mod_sysfs_overlay
ccflags-y += -I$(PWD)
modbus_device_sysfs-objs	 := modbusdevice_sysfs.o \
								modbusdevice_syscalls.o \

ARCH = arm64
CROSS_COMPILE=aarch64-linux-gnu-
KERNEL_SRC = ~/linux_rasp-6.12/

HOST_KERN_DIR = /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules
prepare:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) O=$(KERNEL_OUT) modules_prepare
dtb:
	dtc -I dts -O dtb -o ${dt_source}.dtbo ${dt_source}.dts
clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) clean 
install:
	push_rpi modbus_device_sysfs.ko
host:
	make -C $(HOST_KERN_DIR) M=$(PWD) modules
