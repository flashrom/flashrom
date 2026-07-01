===============================
Recent development (unreleased)
===============================

This document describes the major changes that are expected to be included in
the next release of flashrom and which are currently only available by source
code checkout (see :doc:`../dev_guide/building_from_source`). These changes
may be further revised before the next release.

Programmers updates
===================

ft2232_spi: specify device by USB port path
-------------------------------------------

It was previously not possible to specify a device in case of collision in serial/description,
for example between multiples of the same device with blank EEPROM. now it is possible to select
by USB bus & port path.

Example::

  flashrom -p ft2232_spi:type=4232H,usbpath=1-1.4.2 -r -c W25Q80BV/W25Q80DV -r dump.bin

ni845x_spi programmer is deleted
--------------------------------

The driver was gated to 32-bit Windows and linked against a proprietary
National Instruments library that ships for no other platform, so the
hardware has never been usable anywhere else.

It has been disabled by default all the time and was not built or
tested by CI scripts.

Removed programmers
-------------------
The ``rayer_spi`` programmer has been removed. It bit-banged SPI over a PC
parallel port (RayeR cable, Altera ByteBlasterMV, Atmel STK200/300, Macraigor
Wiggler, Xilinx Parallel Cable III, and SPI Tiny Tools hardware). Parallel
ports are absent from modern hardware, and the driver required raw x86 I/O port
access, restricting it to legacy x86 hosts.

The ``atapromise`` programmer has been removed. It reflashed the option ROM
on Promise PDC2026x ATA/RAID PCI cards via raw x86 port I/O and was capped at
32 kB by the tested card wiring. The hardware is obsolete and legacy hosts
already ship a flashrom that supports it.

The ``gfxnvidia`` programmer has been removed. It reflashed the parallel
flash on old NVIDIA graphics cards through a memory-mapped PCI BAR. The
hardware is obsolete and legacy hosts already ship a flashrom that supports
it.
