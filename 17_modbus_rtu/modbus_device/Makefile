# Modbus Device Module Makefile
obj-m += modbus_device_module.o
modbus_device_module-y	 :=		modbusdevice.o \
								modbusdevice_syscalls.o

ccflags-y += -I$(src)/../modbus_controller
