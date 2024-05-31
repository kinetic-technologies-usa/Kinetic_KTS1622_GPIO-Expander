#!/bin/bash
echo Mapping the port number 300 with gpiochip300-subsystem-export
sudo echo 300 > /sys/class/gpio/gpiochip300/subsystem/export
ls /sys/class/gpio/
cat /sys/class/gpio/gpio300/direction
