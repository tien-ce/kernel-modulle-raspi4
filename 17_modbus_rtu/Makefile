obj-m := my_modbus_module.o
ccflags-y += -I$(PWD)
my_modbus_module-objs := serdev_driver.o \
                         serdev_syscalls.o \
                         modbus_timer.o \
                         modbuscontroller.o \
                         modbus_timer.o \
                         modbus_rtu/mbrtu.o \
                         modbus_rtu/port_serial.o \
                         modbus_rtu/port_event.o \
                         modbus_rtu/port_timer.o \
                         modbus_rtu/mbcrc.o

ARCH = arm64
CROSS_COMPILE=aarch64-linux-gnu-
KERNEL_SRC = ~/linux_rasp-6.12/

HOST_KERN_DIR = /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules
prepare:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) O=$(KERNEL_OUT) modules_prepare

clean:
	rm *.order *.symvers *.mod* *.o *.ko 

host:
	make -C $(HOST_KERN_DIR) M=$(PWD) modules
