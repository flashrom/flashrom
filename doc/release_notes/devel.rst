===============================
Recent development (unreleased)
===============================

This document describes the major changes that are expected to be included in
the next release of flashrom and which are currently only available by source
code checkout (see :doc:`../dev_guide/building_from_source`). These changes
may be further revised before the next release.

Bugs fixed
==========

AMD-based PCs with FCH were unable to read flash contents for internal (BIOS
flash) chips larger than 16 MB, and attempting to do so could crash the
system.

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

FMAP verification support
--------------------------

A new ``--fmap-verify`` option has been added that allows verification of FMAP
(Flash Map) layout compatibility before performing write operations. This option
reads the FMAP layout from both the flash ROM and the file to be written, then
compares them to ensure they match before proceeding with the write.

This is particularly useful when updating specific regions (e.g., COREBOOT) to
prevent accidentally writing a ROM image with an incompatible layout that could
result in a bricked system. The ``--fmap-verify`` option is mutually exclusive
with ``--fmap``, ``--fmap-file``, ``--layout``, and ``--ifd`` options, and can
only be used with write operations (``-w``).

Example usage::

    flashrom -p programmer --fmap-verify -w newimage.rom --image COREBOOT

If the FMAP layouts don't match, the operation will abort with a detailed error
message showing which regions differ.

New programmers
===============

* Nvidia System Management Agent
