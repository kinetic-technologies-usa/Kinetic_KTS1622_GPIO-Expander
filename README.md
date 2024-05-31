# gpio-kts1622 - Linux Driver
KTS1622 16-bit GPIO expander driver for Linux.

Datasheet: [https://www.kinet-ic.com/kts1622/](https://www.kinet-ic.com/kts1622/)
## Device Description
KTS1622 is a 16-bit general-purpose I/O expander via the 
I2C bus for microcontrollers when additional I/Os are 
needed while keeping interconnections to the minimum.  
KTS1622 has separate power rails (VDD_I2C and VDD_P) 
for I2C bus and I/O ports, both ranging from 1.65V to 5.5V, 
allowing mixed power system where I2C bus power is not 
compatible with I/O port power. 
KTS1622 meets the I2C Fast-mode Plus spec up to 1MHz. 
External reset input, internal power-on reset and I2C 
software reset provide flexible ways to reset the IC. Four 
adjustable I2C slave addresses allow multiple KTS1622s in 
one I2C bus system. 
KTS1622 provides multiple ways to program the 16-bit I/O 
ports. When the port works as input, it can program the 
polarity, latch, pull-up, pull-down and interrupt functions. 
The interrupt function includes the level/edge trigger, 
mask, clear, status features. For system with noisy input, 
KTS1622 also provides debounce function with 
programmable debounce time. When the port works as 
output, it can program output stage with bank/pin 
selectable push-pull or open-drain options, it can also 
program four drive strengths of the output stage to optimize 
the rise/fall times. 

## Instructions
Find [README](https://github.com/kinetic-technologies-usa/Kinetic_KTS1622_GPIO-Expander/tree/main/src) in src directory.

## Release note
### Revision 3.0.1 - 5/30/2024
Function | Tested with | Note
---|---|---
GPIO push-pull output | (1) Kernel 5.4 with sysfs, (2) Kernel 5.5 and libgpiod v1.5, (3) Kernel 5.10.92 and libgpiod v1.6.2
GPIO input read | (1) Kernel 5.4 with sysfs, (2) Kernel 5.5 and libgpiod v1.5, (3) Kernel 5.10.92 and libgpiod v1.6.2
Pull-up, Pull-down resistor | (1) Kernel 5.5 and libgpiod v1.5, (2) Kernel 5.10.92 and libgpiod v1.6.2
GPIO open-drain output | (1) Kernel 5.5 and libgpiod v1.5, (2) Kernel 5.10.92 and libgpiod v1.6.2
IRQ/Interrupt generation | (1) Kernel 5.5 and libgpiod v1.5, (2) Kernel 5.10.92 and libgpiod v1.6.2 | GPIO17 of RPi4 used as IRQ input. Defined in .dts
### Revision 1.5.0 - 3/28/2024 (not published)
* GPIO push-pull output - Tested with Kernel 5.4 on RPi4.
* GPIO input read   - Tested with Kernel 5.4 on RPi4
* Pull-up, Pull-down resistor - Implemented but not tested yet due to the environmental issue (RPi4 and the version of libgpiod)
* GPIO open-drain output- (not implemented)

## Validation platform
* Raspberry Pi 4 - 64-bit kernel 5.10.92-v8*
    * Dist image Lite version - Debian Bullseye contains Kernel 5.5.92: [2022-01-28-raspios-bullseye-arm64-lite.zip](https://downloads.raspberrypi.com/raspios_lite_arm64/images/raspios_lite_arm64-2022-01-28/)
    * Linux Kernel 5.5.92: source hash-key [b4145cfaa838049fcc1174d1311a98a854703c29](https://github.com/raspberrypi/rpi-firmware/commit/b4145cfaa838049fcc1174d1311a98a854703c29)
    ```
    Download Source with Hash value:

    $ FIRMWARE_REV=b4145cfaa838049fcc1174d1311a98a854703c29
    $ KERNEL_REV=`curl -L https://github.com/Hexxeh/rpi-firmware/raw/${FIRMWARE_REV}/git_hash`
    $ curl -L https://github.com/raspberrypi/linux/archive/${KERNEL_REV}.tar.gz > linux-kernel-src_5.10.92.tar.gz
    ```
    ```
    To get module files, execute the following command.

    $ cd (to the root of Linux source)
    $ curl -L https://github.com/Hexxeh/rpi-firmware/raw/${FIRMWARE_REV}/Module8.symvers > Module8.symvers
    $ make -j4 modules_prepare
    ```
    ```
    Package Update:
    Note:To prevent upgrade the kernel during the process, sudo apt-mark as below.

    $ sudo apt-mark hold raspberrypi-kernel
    $ sudo apt-mark hold firmware*
    $ sudo apt update && apt upgrade
    $ sudo apt install git bc bison flex libssl-dev make
    $ sudo apt install gpiod libgpiod-dev    
    ```
    ```
    Kernel configuration in the root directory of the Kernel source:

    $ KERNEL=kernel8
    $ make ARCH=arm64 bcm2712_defconfig
    ```

* Raspberry Pi 3B - 64-bit kernel 5.4.51
    * Linux Kernel 5.4.51 source:
      * https://github.com/raspberrypi/linux/archive/refs/tags/raspberrypi-kernel_1.20200819-1.zip
    * Distribution: [Raspberry Pi OS (Legacy) Lite 64bit](https://www.raspberrypi.com/software/operating-systems/)
      * Replaced to Kernel 5.4.51 with below command.
      * $ sudo rpi-update 453e49bdd87325369b462b40e809d5f3187df21d


## Quick instructions
Note: Please see README.md in the codes directory for more details.

I2C device address: 0x20(default on EVB), 0x21, 0x22, 0x23
After compiling the code in your environment, you will need to load the driver. And edit the .dts file as you prefeered.
### Compiling the driver code
Edit the 'KDIR' in the Makefile to specify the root directory of your kernel source tree.
```
obj-m += gpio-kts1622.o
KDIR := ~/top_of_kernel_source    ----> EDIT THIS LINE
PWD := $(shell pwd)

default:
	$(MAKE) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KDIR) M=$(PWD) modules
	dtc -I dts -O dtb -o gpio-kts1622.dtbo gpio-kts1622.dts

clean:
	rm -f *.ko *.o *.mod *.mod.c modules.order Module.symvers *.dtbo
```
### Editing the .dts file
Default setting has I2C device address 0x20 like below
```
  &i2c1 {
          kts1622@20 {
                  compatible = "kinetic_technologies,kts1622";
                  reg = <0x20>;
                  gpio-controller;
          };
  };
```
### Loading the driver module
After compiling the kernel source code, you will get the binary file and load two files into your system. As an example below.
```
$ sudo insmod gpio-kts1622.ko
$ sudo dtoverlay gpio-kt1622.dtbo  
```
If the driver is successfully loaded, new system class is appeared in the tree like this because the number '300' is specified in the driver. kts1622_setup_gpio().
```
/sys/class/gpio/gpiochip300
```
To make the device accessible through sysfs, here is an example in case of using bit#-0.
```
$ sudo echo 300 > /sys/class/gpio/gpiochip300/subsystem/export

 then, this class will be appered.

/sys/class/gpio/gpio300
```

If you like to setup another bit of the GPIO pin, find the example in 12_map_all_ports.sh. 
```
$ sudo echo 301 > /sys/class/gpio/gpiochip300/subsystem/export  --->> for bit#-1
$ sudo echo 302 > /sys/class/gpio/gpiochip300/subsystem/export  --->> for bit#-2

  then, two classes will be appered.

/sys/class/gpio/gpio301
/sys/class/gpio/gpio302
```
### Reading the GPIO direction
```
$ cat /sys/class/gpio/gpio300/direction

  then, you will get:
in
  or
out
```

### GPIO push-pull output
To set the specific GPIO pin in the device as the output pin, use this command as an example for the bit#-0 defined as the 'gpio300' above.
```
$ sudo echo out > /sys/class/gpio/gpio300/direction
```
To make the pin high/low, set the value 1/0.
```
$ echo 1 > /sys/class/gpio/gpio300/value
or
$ echo 0 > /sys/class/gpio/gpio300/value
```

### Reading GPIO input value
To set the specific GPIO pin in the device as the input pin, use this command as an example for the bit#-0 defined as the 'gpio300' above.
```
$ cat /sys/class/gpio/gpio300/value
  then, you will get:
1 --> input high
  or
0 --> input low
```

### Setting the pull-up or pull-down resistor
This procedure requires 'libgpiod'. You can install libgpiod using following commands.
```
$ sudo apt install gpiod libgpiod-dev
```
The libgpiod have a command interface "gpioset". You can obtain the device name with this command.
```
$ sudo gpiodetect
```

Then you can find the device name like "gpiochip3". You can enable the pull-up and pull-down with these commands. It sets the bit#-0 pin as pull-up.
```
$ sudo gpioset gpiochip3 --bias=pull-up 0=0
```
for more details about gpiolibd, refer this document.
[Debian gpiod](https://manpages.debian.org/experimental/gpiod/gpioset.1.en.html)

### GPIO open-drain output
It’s possible to set open-drain mode using libgpiod. As an example, the “gpioset” command for setting pin0 as the open-drain output pin.
```
$ gpioset --drive=open-drain 0=0
```
If change the bit#1-pin to the push-pull,
```
$ gpioset --drive=push-pull 1=0
```
