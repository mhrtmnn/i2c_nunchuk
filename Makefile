-include cfg.local

VERSION_TAG := \"$(shell git --git-dir=/home/marco/Documents/Projects/BBB/modules/i2c_nunchuk/.git rev-parse HEAD)\"
EXTRA_CFLAGS += -D MOD_VER=$(VERSION_TAG) -Wall

ARCHITECTURE := arm
TOOLCHAIN := $(MY_TOOLCHAIN)
COMPILER := $(MY_COMPILER)

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) ARCH=$(ARCHITECTURE) CROSS_COMPILE=$(TOOLCHAIN) CC=$(COMPILER) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

deploy: modules
	scp i2c_nunchuk.ko root@10.10.0.40:/root

.PHONY: modules clean

obj-m += i2c_nunchuk.o

#"make print-XXX" prints value of variable XXX
print-% : ; @echo $* = $($*)
