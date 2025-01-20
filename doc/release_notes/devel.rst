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

flashchips.c split into separate files by vendor
================================================

``flashchips.c`` file was split into separate files per vendor. flashchips.c still exists in the source
code but it is much smaller and only contain "generic" chip entries.

With this, instead of one file ``flashchips.c`` we now have a ``flashchips/`` directory which contains
all the files.

There are no changes to the usage, and everything that's supported stays the same.

New features
============

-r/-w/-v argument is optional when using -i
-------------------------------------------

See :doc:`/classic_cli_manpage` for details.
