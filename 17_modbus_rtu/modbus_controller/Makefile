# Modbus Controller Module Makefile
obj-m += modbus_controller_module.o

modbus_controller_module-objs := modbuscontroller.o \
								 modbuscontroller_timer.o \
								 modbus_rtu/mbrtu.o \
								 modbus_rtu/port_event.o \
								 modbus_rtu/port_timer.o \
								 modbus_rtu/modbus.o \
								 modbus_rtu/mbcrc.o

# Add includes for internal headers and the locally generated Kconfig headers
ccflags-y += -I$(src) -I$(src)/modbus_rtu -I$(src)/lightmodbus \
             -I$(src)/.. -include $(src)/../include/generated/autoconf.h
