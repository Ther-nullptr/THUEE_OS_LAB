obj-m += mypipe.o
KERNELBUILD := /lib/modules/$(shell uname -r)/build
default:
	make -C $(KERNELBUILD) M=$(shell pwd) modules
clean:
	make -C $(KERNELBUILD) M=$(shell pwd) clean