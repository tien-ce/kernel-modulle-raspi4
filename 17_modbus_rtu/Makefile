# --- Kernel Source & Toolchain Config ---
KERNEL_SRC ?= ~/linux_rasp-6.12/
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-

# --- Load local menuconfig choices ---
-include .config

# --- Kbuild Descent Logic ---
# Descend into subdirectories to build them as modules
obj-m += modbus_controller/
obj-m += modbus_device/

# --- Local Build Rules ---
all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) clean

# Run menuconfig for the standalone module setup
menuconfig:
	$(KERNEL_SRC)/scripts/kconfig/mconf Kconfig

dtb:
	dtc -I dts -O dtb -o rs485_overlay.dtbo rs485_overlay.dts

install:
	push_rpi modbus_controller/modbus_controller_module.ko
	push_rpi modbus_device/modbus_device_module.ko
	push_rpi rs485_overlay.dtbo

.PHONY: all clean menuconfig dtb install
