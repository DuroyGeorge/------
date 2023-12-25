KVERS := $(shell uname -r)
TARGET = ch_device
obj-m += $(TARGET).o

all:kernel_modules clean

PWD := $(shell pwd)

kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(PWD) modules
	insmod $(TARGET).ko
clean:
	make -C /lib/modules/$(KVERS)/build M=$(PWD) clean