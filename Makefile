obj-m += GenesisZircon_HID_driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
install:
	sudo insmod GenesisZircon_HID_driver.ko
	sudo mkdir -p /lib/modules/$(uname -r)/extra/
	sudo cp GenesisZircon_HID_driver.ko /lib/modules/$(uname -r)/extra/
	sudo depmod -a
