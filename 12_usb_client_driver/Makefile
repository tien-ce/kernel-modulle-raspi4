obj-m += usb_client.o
SRC := $(shell pwd)
ARCHITECTURE=arm64
CROSS := aarch64-linux-gnu-
KERNEL_OUT = /data200/yocto-build-64/tmp/work/raspberrypi4_lsmy-poky-linux/linux-raspberrypi/6.6.63+git/linux-raspberrypi4_lsmy-standard-build
KERNEL_SRC=/data200/yocto-build-64/tmp/work-shared/raspberrypi4-lsmy/kernel-source
all:
	$(MAKE) ARCH=$(ARCHITECTURE) CROSS_COMPILE=$(CROSS) -C $(KERNEL_SRC) M=$(SRC) O=$(KERNEL_OUT) modules

oldconfig:
	$(MAKE) ARCH=$(ARCHITECTURE) CROSS_COMPILE=$(CROSS) -C $(KERNEL_SRC) O=$(KERNEL_OUT) olddefconfig

prepare:
	$(MAKE) ARCH=$(ARCHITECTURE) CROSS_COMPILE=$(CROSS) -C $(KERNEL_SRC) O=$(KERNEL_OUT) modules_prepare

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
