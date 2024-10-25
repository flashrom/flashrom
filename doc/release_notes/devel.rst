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

Progress display
================

Progress display feature is now working for all operations: read, erase, write.

Chipset support
===============

Added Raptor Point PCH support.

Chip model support added
========================

* FM25Q04
* FM25Q64
* FM25Q128

* GD25B128E
* GD25B256E
* GD25B512MF
* GD25F64F
* GD25F256F
* GD25R128E
* GD25R256E
* GD25R512MF
* GD25LB256F
* GD25LB512ME
* GD25LB512MF
* GD25LR256F
* GD25LR512MF
* GD25LF256F
* GD25LF512MF

* MX25U25645G
* MX77U51250F

* W25Q32JV_M

* XM25LU64C
* XM25QH32C
* XM25QH32D
* XM25QH64D
* XM25QH128D
* XM25QH256D
* XM25QH512C
* XM25QH512D
* XM25QU16C
* XM25QU32C
* XM25QU128D
* XM25QU256D
* XM25QU512C
* XM25QU512D
