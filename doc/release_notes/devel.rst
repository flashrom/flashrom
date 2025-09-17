===============================
Recent development (unreleased)
===============================

This document describes the major changes that are expected to be included in
the next release of flashrom and which are currently only available by source
code checkout (see :doc:`../dev_guide/building_from_source`). These changes
may be further revised before the next release.

Known issues
============

AMD-based PCs with FCH are unable to read flash contents for internal (BIOS
flash) chips larger than 16 MB, and attempting to do so may crash the system.
Systems with AMD "Promontory" IO extenders (mostly "Zen" desktop platforms) are
not currently supported.

https://ticket.coreboot.org/issues/370

Added support
=============

* Intel Wildcat Lake chipset
* Eon EN25QX128A
* PUYA P25D80H

New features
============

Fail immediately when trying to write/erase wp regions
------------------------------------------------------

This change is done so it's harder for user to brick his own platform.
Information about read-only regions can easily be missed as flashrom
can output a lot of information on screen. Even if you notice you might
not know if one of the regions you requested falls inside read-only
range, especially if using different names for those regions.
If you are flashing multiple regions or ones that partially overlap with
read-only parts then that could result in flashrom failing in the
middle, leaving you in unknown state.
