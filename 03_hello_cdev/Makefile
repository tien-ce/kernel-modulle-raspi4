obj-m += pcd.o
SRC := $(shell pwd)
HOST_KERN_DIR = /lib/modules/$(shell uname -r)/build
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) EXTRA_CFLAGS="-g -DDEBUG" O=$(KERNEL_OUT) modules
prepare:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) O=$(KERNEL_OUT) modules_prepare

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) O=$(KERNEL_OUT) clean
	rm -f *.ko

host:
	make -C $(HOST_KERN_DIR) M=$(PWD) modules
