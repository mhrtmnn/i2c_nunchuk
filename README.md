# i2c Nunchuk Driver



This kernel driver reads the status of a Wii Nunchuk and exports this data to the relevant kernel subsystems.
Input events are exported via [evdev] and accelerometer data via [Industrial I/O].

The driver is part of my experimental _BeagleBone Black_ [buildroot project].

It was initially based on bootlin's [Linux kernel and driver development training].


## Evdev
User input of the Nunchuk is exported via an **input_polled_dev**.

The _evdev_ driver exports the input events via a char device (e.g. `/dev/input/event0`) to user space.

The input device used by the evdev driver is called `Wii Nunchuk`.

The Nunchuk has the following inputs:

- **Button C** and **Button Z**

> Event Type `EV_KEY`

> Codes `BTN_C` and `BTN_Z`

- **Joystick-Axis X** and **Joystick-Axis Y**

> Event Type `EV_ABS`,

> Codes `ABS_X` and `ABS_Y`)


## Industrial I/O
Acceleration data of the Nunchuk is exported via an **iio_dev**.
It uses three channels of type `IIO_ACCEL`, one for each of the accelerometer's three axis.

They are exported by the _iio_ driver at `/sys/bus/iio/devices/iio\:device0/in_accel_{x,y,z}_raw`.

The iio device is called `Nunchuk Accel`.



[//]:  #  (Reference Links)
[evdev]: <https://www.kernel.org/doc/Documentation/input/input.txt>
[Industrial I/O]: <https://01.org/linuxgraphics/gfx-docs/drm/driver-api/iio/index.html>
[Linux kernel and driver development training]: <https://bootlin.com/doc/training/linux-kernel/linux-kernel-labs.pdf>

[buildroot project]: <https://bitbucket.org/MarcoHartmann/buildroot_bbb/src>
