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

Build only supported with Meson
===============================

As documented in the :doc:`v1.4 release notes <v_1_4>`, support for building
flashrom with make has been removed; all Makefiles have been deleted. Meson is
now the only supported tool for building flashrom from source.

New Feature
===========
Libpci 3.13.0 and onwards support ECAM to access pci registers. Flashrom will
be moved to ECAM from IO port 0xcf8/0xcfc if the libpci version is >= 3.13.0.
The ECAM has been supported for a very long time, most platforms should support
it. For those platforms don't support ECAM, libpci will terminate the process by
exit.

Chipset support
===============
Added Raptor Point PCH support.
