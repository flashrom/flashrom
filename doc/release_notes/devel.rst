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
