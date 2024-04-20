====================
Arduino flasher 3.3v
====================

Introduction
============

This explains how to:

* Easily lower the voltage of an arduino
* Use that arduino to flash a coreboot image on a GM45 Thinkpad with a SOIC16 chip

It requires:

* An AVR Arduino at 5v
* An USB<->Serial adapter capable of providing enough current to power up:

  * The arduino
  * The flash chip
  * The circuits around the flash chip

It was tested with:

* An Arduino.org "nano version 3.3"
* A Sparkfun "FTDI Basic 3v3" (Uses an FTDI FT232R)

Caveats and warnings:

* It requires you to to never connect an USB cable between the Arduino USB port and the computer while the flasher is connected to a flash chip.
  This would result in the I/O voltage being 5V instead of 3.3V. If you think you may accidentally connect it this way, this flasher isn't the right solution for you.
* You need to read the tutorial, if you don't understand it, it's probably not for you. In doubt you could ask for some help in flashrom real-time channels
  or on the mailing list, see :doc:`/contact`.
* For now it requires to patch frser-duino

Theory
========

In the `Atmega328/P datasheet <https://web.archive.org/web/20181004225154/ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42735-8-bit-AVR-Microcontroller-ATmega328-328P_Datasheet.pdf>`_,
the "32.3. Speed Grades" chapter describes (pages 368 and 369) the link between maximum frequency of the microcontroller and the voltage. At 3.3v, the maximum frequency is 12Mhz.

HOWTO
========

Build the code installing it on the arduino
-------------------------------------------

Building frser-duino
^^^^^^^^^^^^^^^^^^^^

First download frser-duino::

  $ git clone --recursive https://github.com/urjaman/frser-duino.git
  $ cd frser-duino

Then modify the ``F_CPU`` value in ``main.h`` to be 12Mhz instead of 16Mhz. ``F_CPU`` will look like that::

  #define F_CPU 16000000UL

Change it to::

  #define F_CPU 12000000UL

You can then build the code::

  $ make ftdi

Installing the code on the arduino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Before installing the code on arduino we want to make sure that we are talking to the right device.
To do that make sure that the arduino is not connected to your computer and run::

  $ ls -l /dev/ttyUSB0

If everything went fine it will output something like that::

  ls: cannot access '/dev/ttyUSB0': No such file or directory

If instead it looks like that::

  crw-rw---- 1 root uucp 188, 0 27 févr. 14:30 /dev/ttyUSB0

then something else is connected to your computer at ``/dev/ttyUSB0``. If you can't figure what's going on by yourself,
it's better to try get help on the flashrom channels to fix the issue, see :doc:`/contact`.

Then connect your arduino to the computer and run the same command::

  $ ls -l /dev/ttyUSB0

This time it's supposed to output a line that looks more or less like that::

  crw-rw---- 1 root uucp 188, 0 27 févr. 14:30 /dev/ttyUSB0

At this point we're pretty confident that ``/dev/ttyUSB0`` corresponds to the arduino, so we can install the code with::

  $ make flash-ftdi

Once it is installed we can now test that everything went fine with::

  $ flashrom -p serprog:dev=/dev/ttyUSB0:2000000

This will make flashrom talk to the arduino to verify if everything is fine up to this point.

If everything went fine it will look more or less like that::

  $ flashrom -p serprog:dev=/dev/ttyUSB0:2000000
  /sys/firmware/dmi/tables/smbios_entry_point: Permission denied
  /dev/mem: Permission denied
  /sys/firmware/dmi/tables/smbios_entry_point: Permission denied
  /dev/mem: Permission denied
  flashrom v1.0 on Linux 4.15.2-gnu-1 (i686)
  flashrom is free software, get the source code at https://flashrom.org

  Using clock_gettime for delay loops (clk_id: 1, resolution: 1ns).
  serprog: Programmer name is "frser-duino"
  serprog: requested mapping AT45CS1282 is incompatible: 0x1080000 bytes at 0xfef80000.
  No EEPROM/flash device found.
  Note: flashrom can never write if the flash chip isn't found automatically.

This is the important line::

  serprog: Programmer name is "frser-duino"

It means that flashrom is able to talk to the flasher, which reports itself as "frser-duino" We also have the following line::

  No EEPROM/flash device found.

which tells that it didn't find any flash. This is what's supposed to happen since we didn't connect any yet.

Build the programmer
--------------------

Connect the programmer to the USB<->Serial adapter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To do that:

* Connect the FTDI adapter RX to the TX of the arduino
* Connect the FTDI adapter TX to the RX of the arduino
* Connect the 3V3 of the FTDI adapter to the 5V pin of the Arduino
* Connect the GND of the FTDI adapter to the GDN of the arduino.

Here's a summary of the above:

======== =========================
Arduino	 USB<->Serial port adapter
======== =========================
RX	 TX
TX	 RX
5v	 3.3v
GND	 GND
======== =========================

You can now check that the programmer is responding with::

  flashrom -p serprog:dev=/dev/ttyUSB0:2000000

Since you didn't connect yet a flash chip, it will says it found no flash chips::

  $ flashrom -p serprog:dev=/dev/ttyUSB0:2000000
  /sys/firmware/dmi/tables/smbios_entry_point: Permission denied
  /dev/mem: Permission denied
  /sys/firmware/dmi/tables/smbios_entry_point: Permission denied
  /dev/mem: Permission denied
  flashrom v1.0 on Linux 4.15.2-gnu-1 (i686)
  flashrom is free software, get the source code at https://flashrom.org

  Using clock_gettime for delay loops (clk_id: 1, resolution: 1ns).
  serprog: Programmer name is "frser-duino"
  serprog: requested mapping AT45CS1282 is incompatible: 0x1080000 bytes at 0xfef80000.
  No EEPROM/flash device found.
  Note: flashrom can never write if the flash chip isn't found automatically.

Again like before the important parts are::

  serprog: Programmer name is "frser-duino"

And::

  No EEPROM/flash device found.

If you made it up to this point, you successfully built the flasher.

Using the programmer
--------------------

Connect the programmer to a flash chip
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Since the flasher has been built, you probably want to use it.

If you use a clip (Like a SOIC-8 or SOIC16 Pomona clip), connect it to the Arduino
Connect the chip to the clip, or if you don't use a clip, the chip to the Arduino
Here's how to connect the flash chips to the programmer:

===========	==========================	===================	=====================
Arduino pin	Function			Flash chip pin name	SOIC16 Flash chip pin
===========	==========================	===================	=====================
D13		CLK (Clock)			CLK			16
D12		MISO (Master In Slave Out)	MISO or SO		8
D11		MOSI (Master Out Slave In)	MOSI or SI		15
D10		CS# (Chip Select)		CS# or CS OR SS		7
GND		GND (Ground)			GND			10
5V		3.3V				VCC			2
5V		3.3V				WP# (Write Protect)	9
5V		3.3V				HOLD#			1
===========	==========================	===================	=====================

Then connect an USB cable between the USB<->Serial adapter and the computer.
Never connect the cable between the Arduino USB port and the computer while the flasher is connected to a flash chip.
That would result in the I/O voltage being 5V instead of 3.3V.

Flashing
^^^^^^^^

Run flashrom like that::

  flashrom -p serprog:dev=/dev/ttyUSB0:2000000

With some models of Macronix flash chip (that are present in the Thinkpad X200) you might need to add ``spispeed=100k`` like that::

  flashrom -p serprog:dev=/dev/ttyUSB0:2000000,spispeed=100k

Thanks
========

Thanks a lot to SwiftGeek on IRC (#libreboot on Libera) for finding the first workaround to make it detect flash chips at lower voltage.
Thanks also for telling me about the Macronix issues on the Thinkpad X200. This project would not have been possible without that.

Page license
============

This page is available under the following licenses:

* `CC-BY-SA 3.0 <https://creativecommons.org/licenses/by-sa/3.0/legalcode>`_
* `CC-BY-SA 4.0 <https://creativecommons.org/licenses/by-sa/4.0/legalcode>`_ or later
* `GFDL 1.3 <https://www.gnu.org/licenses/fdl-1.3.txt>`_ or later
