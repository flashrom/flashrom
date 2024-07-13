=====================
Misc notes and advice
=====================

This document contains miscellaneous and unstructured (and mostly, legacy) notes and advice about using flashrom.

Command set tricks for parallel and LPC chips
=============================================

This is only mentioned in very few datasheets, but it applies to some parallel (and some LPC) chips.

Upper address bits of commands are ignored if they are not mentioned explicitly. If a datasheet specifies the following sequence::

   chip_writeb(0xAA, bios + 0x555);
   chip_writeb(0x55, bios + 0x2AA);
   chip_writeb(0x90, bios + 0x555);

then it is quite likely the following sequence will work as well::

   chip_writeb(0xAA, bios + 0x5555);
   chip_writeb(0x55, bios + 0x2AAA);
   chip_writeb(0x90, bios + 0x5555);

However, if the chip datasheet specifies addresses like ``0x5555``, you can't shorten them to ``0x555``.

To summarize, replacing short addresses with long addresses usually works, but the other way round usually fails.

flashrom doesn't work on my board, what can I do?
=================================================

* First of all, check if your chipset, ROM chip, and mainboard are supported
  (see :doc:`/supported_hw/index`).
* If your board has a jumper for BIOS flash protection (check the manual), disable it.
* Should your BIOS menu have a BIOS flash protection option, disable it.
* If you run flashrom on Linux and see messages about ``/dev/mem``, see next section.
* If you run flashrom on OpenBSD, you might need to obtain raw access permission by setting
  ``securelevel = -1`` in ``/etc/rc.securelevel`` and rebooting, or rebooting into single user mode.

What can I do about /dev/mem errors?
====================================

* If flashrom tells you ``/dev/mem mmap failed: Operation not permitted``:

  * Most common at the time of writing is a Linux kernel option, ``CONFIG_IO_STRICT_DEVMEM``,
    that prevents even the root user from accessing hardware from user-space if the resource is unknown
    to the kernel or a conflicting kernel driver reserved it. On Intel systems, this is most often ``lpc_ich``,
    so ``modprobe -r lpc_ich`` can help. A more invasive solution is to try again after rebooting
    with ``iomem=relaxed`` in the kernel command line.

  * Some systems with incorrect memory reservations (e.g. E820 map) may have the same problem
    even with ``CONFIG_STRICT_DEVMEM``. In that case ``iomem=relaxed`` in the kernel command line may help too.

* If it tells you ``/dev/mem mmap failed: Resource temporarily unavailable``:

  * This may be an issue with PAT (e.g. if the memory flashrom tries to map is already mapped
    in an incompatible mode). Try again after rebooting with nopat in the kernel command line.

* If you see this message ``Can't mmap memory using /dev/mem: Invalid argument``:

  * Your flashrom is very old, better update it. If the issue persists, try the kernel options mentioned above.

* Generally, if your version of flashrom is very old, an update might help.
  Flashrom has less strict requirements now and works on more systems without having to change the kernel.

Connections
===========

Using In-System programming requires some means to connect the external programmer to the flash chip.

Note that some external flashers (like the Openmoko debug board) lack a connector,
so they do requires some soldering to be used. Some other don't. For instance the buspirate has a pin connector on it.

Programmer <-> Removable chip connection
----------------------------------------

A breadboard can be used to connect Dual in-line 8 pins chips to the programmer, as they they fit well into it.

Programmer <-> Clip connection
------------------------------

If your programmer has a pin connector, and that you want to avoid soldering, you can use
**Short** `Jump Wires <https://en.wikipedia.org/wiki/Jump_wire>`_ to connect it to a clip.
They usually can be found on some electronic shops.

Other issues
-------------

* Wires length and connection quality: Long wires, and bad connection can create some issues, so avoid them.

  * The maximum wires length is very dependent on your setup, so try to have the shortest wires possible.
  * If you can't avoid long wires and if you're flash chip is SPI, then lowering the SPI clock could make
    it work in some cases. Many programmers do support such option (Called spispeed with most of them, or divisor with ft2232_spi).

* When soldering wires, the wire tend to break near the soldering point. To avoid such issue,
  you have to prevent the wires from bending near the soldering point.
  To do that `Heat-shrink_tubing <https://en.wikipedia.org/wiki/Heat-shrink_tubing>`_ or similar methods can be used.

Common problems
===============

The following describes problems commonly found when trying to access flash chips in systems
that are not designed properly for this job, e.g. ad-hoc setups to flash in-system
(TODO add a doc for in-system-specific problems).

Symptoms indicating you may have at least one of these are for example inconsistent reads or probing results.
This happens basically because the analog electrical waveforms representing the digital information
get too distorted to by interpreted correctly all the time. Depending on the cause different steps can be tried.

* Not all input pins are connected to the correct voltage level/output pin of the programmer.
  Always connect all input pins of ICs!

* The easiest thing to try is lowering the (SPI) clock frequency if your programmer supports it.
  That way the waveforms have more time to settle before being sampled by the receiver which might be enough.
  Depending on the design of the driver and receiver as well as the actual communication path
  this might not change anything as well.

* Wires are too long. Shortening them to a few cm (i.e. < 20, the lesser the better) might help.

* The impedances of the wires/traces do not match the impedances of the input pins
  (of either the circuit/chip on the mainboard or the external programmer).
  Try using shorter wires, adding small (<100 Ohm) series resistors or parallel capacitors (<20pF)
  as near as possible to the input pins (this includes also the MISO line which ends near the programmer)\
  and/or ask someone who has experience with high frequency electronics.

* The supply voltage of the flash chip is not stable enough. Try adding a 0.1 µF - 1 µF (ceramic) capacitor
  between the flash chip's VCC and GND pins as near as possible to the chip.

Live CD
=========

A Live CD containing flashrom provides a user with a stable work environment to read, write and verify a flash device on any supported hardware.

It can help avoid Linux installation issues, which can be a hassle for some users.

flashrom is already shipped in some of the Live CDs, see below. *Please note, some of these ship very old versions of flashrom*.

* `SystemRescueCd <http://www.sysresccd.org/>`_ has been including flashrom since about version 2.5.1.

* `grml <http://grml.org/>`_

  * Note: You need the full grml ISO, "small" (and "medium") ISOs do not contain flashrom.
  * Note: Some releases (e.g. 2011.12) did not contain flashrom.

* `Parted Magic <http://partedmagic.com/>`_

* `Hiren's BootCD <http://www.hirensbootcd.org/>`_

   * When you select "Linux based rescue environment (Parted Magic 6.7)" and then "Live with default settings",
     you have access to a system which has flashrom.
