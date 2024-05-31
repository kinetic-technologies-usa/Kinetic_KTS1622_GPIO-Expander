#!/bin/bash
ls /sys/class/gpio
sudo insmod gpio-kts1622.ko
sudo dtoverlay gpio-kts1622.dtbo
ls /sys/class/gpio
