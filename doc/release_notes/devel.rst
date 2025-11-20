===============================
Recent development (unreleased)
===============================

This document describes the major changes that are expected to be included in
the next release of flashrom and which are currently only available by source
code checkout (see :doc:`../dev_guide/building_from_source`). These changes
may be further revised before the next release.

Bugs fixed
==========

* AMD-based PCs with FCH were unable to read flash contents for internal (BIOS
  flash) chips larger than 16 MB, and attempting to do so could crash the
  system.

  https://ticket.coreboot.org/issues/370

* Erasing generic SFDP chips was unreliable or could crash if the SFDP
  erase definitions were not in ascending order of sector sizes.

  See https://review.coreboot.org/c/flashrom/+/90131

Added support
=============

* Intel Wildcat Lake chipset
* EN25QX128A
* MT25QU02G, MT25QL02G marked as tested
* MT35XU02G
* MX25U12873F
* P25D80H
* W35T02NW

New features
============

Fail immediately when trying to write/erase wp regions
------------------------------------------------------

One more check has been added before performing erase/write operations.
It aborts writing to flash if any of the requested regions are
write-protected by chip, dynamically by a chipset, or are defined as
read-only.

A new message is now printed for the user to explain what happened,
example::

  can_change_target_regions: cannot fully update part1 region
  (00000000..0x04ffff) due to chip's write-protection.
  At least one target region is not fully writable. Aborting.
  Error: some of the required checks to prepare flash access failed.
  Earlier messages should give more details.
  Erase operation has not started.

For write, the last line would be ``Write operation has not started.``

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

Erase now respects --noverify option
-------------------------------------

The option ``--noverify`` is now respected during erase operation,
and verification is not performed if the option is set.

Corresponding libflashrom flag is ``FLASHROM_FLAG_VERIFY_AFTER_WRITE``.

Default behaviour stays the same, by default verification is performed.

Previously, erase operation ignored ``--noverify`` option and always
performed verification.

For more details, see https://ticket.coreboot.org/issues/520

Note there are still some remaining cases for non-spi chips,
when ``--noverify`` is ignored, more details and disussion
is here: https://ticket.coreboot.org/issues/605

New programmers
===============

* Nvidia System Management Agent

  System Management Agent (SMA) programmer is a SoC which is working as a side band management
  on Nvidia server board. One of its functions is to flash firmware to other components.

libflashrom
===========

The real life usage of ``flashrom_flash_probe_v2`` discovered the issue, the error messages as below::

    error: variable 'flashctx' has initializer but incomplete type
    error: storage size of 'flashctx' isn't known

New API method ``flashrom_create_context`` is created to fix this.
``flashrom_create_context`` does create and all initialisations needed for flash context.
It need to be called prior to any other API calls that require flash context.

The issue was a good motivation to write a new test which is built and runs as an external
client of libflashrom. The test runs in a separate test executable to achieve this.
As a bonus, test code in ``tests/external_client.c`` is an example how to use libflashrom API.

SPDX tags for license and copyright
===================================

From now on, flashrom is using SPDX tags in source files to indicate license and copyright information.

Specifically ``SPDX-License-Identifier`` tag is used for license info, and
``SPDX-FileCopyrightText`` tag is used for copyright info.

Existing source files in the tree have been converted to use SPDX tags. New files should be
created with SPDX tags. For examples, look at any source file in the tree.

Note that source files in directory ``subprojects/packagefiles/cmocka-1.1.5`` haven't been changed,
and won't be. Those files are unpacked from cmocka wrap, so that's the exact copy from another project,
and should stay as is.
