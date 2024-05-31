obj-m += gpio-kts1622.o
KDIR := /home/koji/linux-5.10.92

PWD := $(shell pwd)

default:
	$(MAKE) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KDIR) M=$(PWD) modules
	dtc -@ -i $(KDIR)/scripts/dtc/include-prefixes -I dts -O dtb -o gpio-kts1622.dtbo gpio-kts1622.dts

clean:
	rm -f *.ko *.o *.mod *.mod.c modules.order Module.symvers *.dtbo
