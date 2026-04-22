# Modbus Device Module Makefile
obj-m += modbus_device_module.o

modbus_device_module-objs	 := modbusdevice.o \
								modbusdevice_syscalls.o

# Add includes to access the controller's shared header and Kconfig headers
ccflags-y += -I$(src)/../modbus_controller \
             -I$(src)/.. -include $(src)/../include/generated/autoconf.h
