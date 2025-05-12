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

Hall of Fame added to documentation
-----------------------------------

The flashrom HTML documentation (and web site) now includes an
automatically-generated list of historical contributors, to acknowledge
everybody who has made flashrom into what it is:
:doc:`../about_flashrom/hall_of_fame`.

When building the documentation, the ``generate_authors_list`` Meson option will
cause the lists to be generated, requiring a runnable copy of Git on the system
and that the source tree being built is a Git working copy. If those
requirements are not satisfied or the option is disabled, the authors lists will
be replaced with placeholders unless the ``generate_authors_list`` option is set
to ``enabled`` in which case the build will fail if the requirements are not
satisfied.

Programmer updates
------------------

* spidriver: Add support for the Excamera Labs SPIDriver

libflashrom API updates
=======================

New API for progress reporting
------------------------------------------

The old ``flashrom_set_progress_callback`` function for requesting progress updates
during library operations is now deprecated. Users should call
``flashrom_set_progress_callback_v2`` instead, which also changes the signature
of the callback function. Specifically, new function type ``flashrom_progress_callback_v2``
should be used from now on.

This new API fixes limitations with the old one where most users would need to
define their own global state to track progress, and it was impossible to fix that
issue while maintaining binary compatibility without adding a new API.

New API for logging and setting log level
-----------------------------------------

New API ``flashrom_set_log_callback_v2`` is added which allows the users to provide
a pointer to user data, the latter is managed by the caller of the API. New API also
introduces a new signature for log callback function, ``flashrom_log_callback_v2``.

New API supports setting the log level for messages from libflashrom, ``flashrom_set_log_level``.
The log callback is invoked only for the messages with log level less or equal than the one
requested (i.e. only for the messages at least as urgent as the level requested).

Old API ``flashrom_set_log_callback`` without user data continues to work as before.

New API for probing flashchips
------------------------------

New API for probing is added ``flashrom_flash_probe_v2`` which can (if requested)
go through all known flashchips and find all matches. v2 returns the number of matches
found (or 0 if none found) and the list of names of all matched entries.

``flashrom_flash_probe_v2`` continues to support an optional parameter ``chip_name``
if the caller want to probe for only one specific chip with given name.

Old API ``flashrom_flash_probe`` which stops probing if more than one chip entry matches
continues to work as before.

New API to get list of supported programmers
--------------------------------------------

New API ``flashrom_supported_programmers`` returns the list of all programmers that are
supported on a current run environment.
