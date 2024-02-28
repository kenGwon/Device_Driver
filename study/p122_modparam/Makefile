MOD := ledkey_modparam
obj-m := $(MOD).o

CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
#KDIR := /lib/modules/$(shell uname -r)/build
KDIR := /home/ubuntu/pi_bsp/kernel/linux
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules $(CROSS)
	cp $(MOD).ko /srv/nfs
clean:
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf .*.cmd
	rm -rf *.o
	rm -rf modules.order
	rm -rf Module.symvers
	rm -rf $(MOD).mod
	rm -rf .tmp_versions
