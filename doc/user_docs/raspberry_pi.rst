===========
RaspberryPi
===========

`RaspberryPi <http://www.raspberrypi.org/faqs>`_ is a cheap single-board computer developed in the UK
by the Raspberry Pi Foundation with the intention of stimulating the teaching of basic computer science in schools.
It can run a fully-functional GNU/Linux distribution and exposes SPI, I2C and several GPIOs on its expansion header.

Prerequisites
=============

Use latest Raspbian (or any other distribution with a recent kernel).
Run the following commands (or make sure these kernel modules are loaded successfully)::

    modprobe spi_bcm2835 # If that fails you may wanna try the older spi_bcm2708 module instead
    modprobe spidev

Connecting the flash chip
=========================

To learn more about the RPi's expansion header refer to http://elinux.org/Rpi_Low-level_peripherals .
If you use one of the first A or B models, please make sure to not draw more than 50mA from the 3.3V pin.
Official numbers for newer models are hard to find but they use a switching regulator
that should provide a much higher margin. If the flash chip is still placed in a foreign circuit
(e.g. soldered to a PC mainboard) please refer to :doc:`/user_docs/in_system` for further details.

========== =======================
RPi header SPI flash
========== =======================
25	   GND
24	   /CS
23	   SCK
21	   DO
19	   DI
17	   VCC 3.3V (+ /HOLD, /WP)
========== =======================

.. container:: danger, admonition

   Always connect all input pins of integrated circuits (not only flash chips).

In general the other pins (usually pin 3 is /WP and pin 7 is /HOLD) should be connected to Vcc
unless they are required to be floating or connected to GND (both extremely uncommon for SPI flash chips).
Please consult the datasheet for the flash chip in question.

If your flash chip is detected but your read/write/verify operations tend to fail, try to add decoupling capacitors (one 100nF and one 4.7uF ceramic capacitor is preferred) close to the flash chip's power pin.

Running flashrom
================

Flashrom uses the Linux-native SPI driver, which is implemented by flashrom's linux_spi programmer.
To use the RaspberryPi with flashrom, you have to specify that programmer. You should always tell
it at what speed the SPI bus should run; you specify that with the spispeed parameter (given in kHz).
You also have to specify the Linux SPI device, e.g.::

    flashrom -p linux_spi:dev=/dev/spidev0.0,spispeed=1000
