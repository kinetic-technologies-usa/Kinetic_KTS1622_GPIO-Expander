# How to build

## Download the Linux Kenel

Download the Linux Kernel source of the corresponding version.

The corresponding version is here.

https://github.com/raspberrypi/linux/archive/refs/tags/raspberrypi-kernel_1.20200819-1.zip

 

## Extract the source

$ unzip raspberrypi-kernel_1.20200819-1.zip

$ git clone https://github.com/kinetic-technologies-usa/Kinetic_KTS1622_GPIO-Expander

## Build the Linux kernel

Please refer to the Raspberry Pi Official Document and set up the build environment.

https://www.raspberrypi.com/documentation/computers/linux_kernel.html

 
```
$ cd ~/linux-raspberrypi-kernel_1.20200819-1
$ make ARCH=arm64 KERNEL=kernel8 CROSS_COMPILE=aarch64-linux-gnu- bcm2712_defconfig
$ make -j4 ARCH=arm64 KERNEL=kernel8 CROSS_COMPILE=aarch64-linux-gnu- Image modules dtbs
```
 

## Build the Linux driver

```
$ cd Kinetic_KTS1622_GPIO-Expander/src/drivers
$ nano Makefile
```

In the Makefile, edit the line that starts with KDIR := to point to the directory of the Linux Kernel source.

Then, you can build the driver.

 
```
$ make
```
 

You can get the ‘gpio-kts1622.ko’ kernel driver module and ‘gpio-kts1622.dtbo’ device tree blob.

NOTE: The dtc (device tree compiler) may output several warnings for the current version of the dts file.

 
# How to enable the driver

## Copy to the Raspberry Pi

Please copy the ‘gpio-kts1622.ko’ and the ‘gpio-kts1622.dtbo’ to your Raspberry Pi 3 that is running Linux Kernel 5.4.51.

Also please ensure the KTS1662 is connected via I2C interface.

## Enable I2C

Please enable I2C in your Raspberry Pi. It is disabled by default.

```
$ sudo raspi-config
```

Enter the ‘Interfacing Options’ --> ‘I2C’


## Load and Enable the driver

On the Raspberry Pi shell, please run the following commands.

```
$ sudo insmod gpio-kts1622.ko      # Load the kernel module
$ sudo dtoverlay gpio-kt1622  # Load the device tree overlay
```


Now, you can use the KTS1622 from Linux.
Check the kernel log with `dmesg` for any errors.

 

# Test

Modern Linux distributions provide `libgpiod` for accessing GPIOs. However, `sysfs` can still be used for compatibility.

Here are the instructions to access via sysfs.

## via sysfs

If the driver is successfully loaded, you can see the device as a special file ‘/sys/class/gpio/gpiochip489’ (the number may vary depends on the environment).

The KTS1622 GPIO number is automatically mapped to the empty space in the driver.
The "gc->base = -1;" statement in the kts1622_setup_gpio() determines the GPIO starting number and "-1" indicates the number is allocated by Linux kernel automatically.
 
At first, enable a pin with this command.

```
$ echo 489 > /sys/class/gpio/export
```

This enables GPIO489, which is mapped to the KTS1622 GPIO PIN0.

The special file ‘/sys/class/gpio/gpio489’ will appear.

You can see the GPIO direction like this. The default is input pin.

```
$ cat /sys/class/gpio/gpio489/direction
in
```

You can read the current input value.

```
$ cat /sys/class/gpio/gpio489/value
0
```

You can change the direction of the pin.

```
$ echo out > /sys/class/gpio/gpio489/direction
```

You can output HIGH or LOW.

```
$ echo 1 > /sys/class/gpio/gpio489/value
$ echo 0 > /sys/class/gpio/gpio489/value
```


## via gpiod

The libgpiod provides interface to the user land application as well as some useful command line tools.
You can install libgpiod using following commands.

```
$ sudo apt install gpiod libgpiod-dev
```


You can see the device name (allocated automatically) using the gpiodetect command.

```
$ gpiodetect
gpiochip0 [pinctrl-bcm2835] (54 lines)
gpiochip1 [brcmvirt-gpio] (2 lines)
gpiochip2 [raspberrypi-exp-gpio] (8 lines)
gpiochip3 [1-0020] (16 lines)
```

The last device (16 lines GPIO) is the KT1622, so the device name is `gpiochip3`.

Now, you can control each lines using "gpioset <gpioname> <pin>=<val>" command.
For example,

```
$ gpioset gpiochip3 0=1
```

turns on the PIN 0, and

```
$ gpioset gpiochip3 11=0
```

turns off the PIN 11.

You can also control from libgpiod APIs directly. Please refer to the test_output.c.

Pin# of gpioset | Device pin#
---|---
0|A0
1|A1
2|A2
3|A3
4|A4
5|A5
6|A6
7|A7
8|B0
9|B1
10|B2
11|B3
12|B4
13|B5
14|B6
15|B7

### Set to open drain mode

To set to open drain mode, you can call libgpiod API.
Please refer to the "examples/test_opendrain.c" file to set open drain mode.

You might also be possible to set open-drain mode with the command below.

```
$ gpioset --drive=open-drain 0=0
```

However, in my environment, it doesn't work. It may because gpioset command of my version (v1.6.3) does not support the option.
It will work in more recent kernel and libgpiod.


### Use of pull-up/down

It seems that Linux Kernel 5.4 does not support configuring pull-up/down resisters through libgpiod,
according to the following post:

- https://github.com/dotnet/iot/pull/968 

It supports from Linux Kernel v5.5 and libgpiod v1.5.

If the kernel and libgpiod supports the function, you can enable pull-up/down resisters with this command.

```
$ gpioset --drive=push-pull 1=0
```

The "examples/test_pullup.c" will also work in recent versions.

### Use of interrupts

If you want to use falling/rising edge interrupts, you can refer to the sample program "examples/test_irq.c".

The KTS1622 requires a parent GPIO pin, which is set in the device tree.
You can see the Raspberry Pi 4 sample in "gpio-kts1622.dts".

```
/dts-v1/;
/plugin/;

&i2c1 {
    #address-cells = <1>;
    #size-cells = <0>;

    kts1622@20 {
        #address-cells = <1>;
        #size-cells = <0>;
        #gpio-cells = <2>;
        gpio-controller;

        compatible = "kinetic_technologies,kts1622";
        reg = <0x20>;
        interrupt-parent = <&gpio>;
        interrupts = <17 2>; // Raspberry Pi GPIO17 with falling edge trigger
    };
};
```

This sets the GPIO17 of Raspberry Pi 4 as the interrupt pin connected to the KTS1622.
