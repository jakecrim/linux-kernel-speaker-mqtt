obj-m += kthread_test.o
obj-m += kthread_Jacob.o
obj-m += hrtimer_test.o
obj-m += Lab6_cdev_kmod.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean