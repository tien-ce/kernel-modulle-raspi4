#obj-m := modbus_controller_module.o
obj-m := modbus_controller_module.o
obj-m += modbus_device_module.o
dt_source := rs485_overlay

# 1.Pull in user choices from menuconfig
-include $(PWD)/.config

# 2. Tell the C Compiler to automatically include autoconf.h in all .c files
# This replaces all your manual -DCONFIG_... flags
ccflags-y += -I$(PWD) -include $(PWD)/include/generated/autoconf.h

modbus_controller_module-objs := modbuscontroller.o \
								 modbuscontroller_timer.o \
								 modbus_rtu/mbrtu.o \
								 modbus_rtu/port_event.o \
								 modbus_rtu/port_timer.o \
								 modbus_rtu/modbus.o \
								 modbus_rtu/mbcrc.o

modbus_device_module-objs	 := modbusdevice.o \
								modbusdevice_syscalls.o \

ARCH = arm64
CROSS_COMPILE=aarch64-linux-gnu-
KERNEL_SRC = ~/linux_rasp-6.12/

HOST_KERN_DIR = /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules
menuconfig:
	$(KERNEL_SRC)/scripts/kconfig/mconf Kconfig
prepare:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) O=$(KERNEL_OUT) modules_prepare
dtb:
	dtc -I dts -O dtb -o ${dt_source}.dtbo ${dt_source}.dts
clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) clean 
install:
	push_rpi modbus_controller_module.ko
	push_rpi modbus_device_module.ko
	push_rpi rs485_overlay.dtbo
host:
	make -C $(HOST_KERN_DIR) M=$(PWD) modules
