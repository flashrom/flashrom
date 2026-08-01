===============================
Recent development (unreleased)
===============================

This document describes the major changes that are expected to be included in
the next release of flashrom and which are currently only available by source
code checkout (see :doc:`../dev_guide/building_from_source`). These changes
may be further revised before the next release.

New features
============

Multi-die flash chip support
----------------------------

Support is added for some of the multi-die flash chips.

Two new fields are added in the ``struct flashchip`` definition:

* ``.die_size`` - Size of a single die in kilobytes. Set for multi-die chips.

* ``.die_select`` - callback for die selection. Different vendors can use
  different opcodes and methods to select active dies.

This enables future support for multi-die flash chips without affecting existing single-die
chip support.

New feature flag ``FEATURE_STATUS_PER_DIE`` supports multi-die chips which have status register per die.
This flag need to be used together with new fields ``die_size`` and ``die_select``.

Feature currently enabled for models ``W77Q128NW`` and ``W77T128NW``.

2-byte addressing
-----------------

``FEATURE_ADDR_2BYTE`` marks a chip that uses a 16-bit (2-byte) address in its read and program instructions
rather than the default 24-bit (3-byte) address. This is used by small SPI EEPROMs such as
``ST M95320`` (parts up to 64 KiB in the M95xxx family).
When set, ``spi_prepare_address()`` emits only two address bytes.

Repeated reads
--------------

``--read-repeated[=<count>] [<file>]``: Read the flash chip multiple times
(default: 3, minimum: 3) and use majority voting to detect unstable
connections. Deduplicates read results to minimise memory usage. If a file
is given and a strict majority is found, saves the majority content to it.
Useful for diagnosing flaky SPI wiring or unreliable programmers.

Programmers updates
===================

New: fault injection for testing
--------------------------------

``fault`` -- A fault injection programmer that wraps an existing backend
(e.g. ``dummy``) and injects deterministic, reproducible faults. Supports
bit corruption, short reads, write failures, write lies (silent no-ops),
and partial writes. Uses a seeded PRNG for reproducibility. Intended for
testing verification logic and retry mechanisms under realistic failure
conditions.

linux_mtd: ignore_read_errors parameter
---------------------------------------

Some controllers refuse to read firmware-protected ranges (e.g. Intel PCH protected range registers) and fail the
whole read. With the optional ``ignore_read_errors=yes`` parameter unreadable blocks are replaced by the erased
value (usually ``0xff``) and a warning is printed, allowing the accessible remainder of the flash to be dumped::

        flashrom -p linux_mtd:dev=N,ignore_read_errors=yes -r dump.bin

Note that this suppresses every read error reported by the kernel, not only refusals caused by firmware-protected
ranges. Any block that cannot be read, for whatever reason, is replaced by the erased value, so the resulting dump
may differ from the actual flash contents.

dediprog: Support target 3 (socket)
-----------------------------------

Example to read W25R256JW with SF600::

	flashrom -p dediprog:voltage=1.8V,target=3 -c W25R256JW -r dump.bin

libflashrom
===========

The error code ``ERROR_FLASHROM_PREPARE_FLASH_ACCESS`` (``-3``) is now returned
for all operations in case when ``prepare_flash_access`` failed and the operation itself has not started.

``libflashrom.h`` updated to document the error code for:

``flashrom_flash_erase``

``flashrom_image_read``

``flashrom_image_write``

``flashrom_image_verify``

Added support
=============

* Nova Lake SoC

* Sophos XG 230r2 board

* BY25D20/40

* IS25LP032
* IS25LQ032

* MX25L3236D
* MX25LM51245G

* P25D32SH

* S25FL512S -> marked as tested

* W25Q01JV
* W77{Q,T}128NW -> marked tested

* XM25RH128C
* XM25RU256C -> added WP support and mark tested

Special cases:

Fudan FM25W128
--------------

Added support for the Fudan ``FM25W128`` SPI flash chip.

This also added a new decoder function ``decode_range_spi25_bp3_to_1_16`` which adds
protection range calculation for SPI chips where BP values below 3 mean no protection
and BP=3 starts at 1/16 of the chip.

ST M9532
--------

This chip has to be used in a slightly different way than most SPI flash chips,
for two reasons:

* **No probing / auto-detection.** The M95320 has no ``RDID`` instruction and
  therefore no electronic signature to read back, so flashrom cannot identify
  it automatically. The chip must be selected explicitly on the command line
  and forced, for example::

      flashrom -p <programmer> -c M95320 --force -r backup.bin

  Because the chip cannot be probed, running a plain probe (for instance while
  probing every known chip) will report::

      Probing for ST M95320, 4 kB: failed! flashrom has no probe function for this flash chip.

  This message is expected and simply means the chip cannot be detected on its
  own; it does not indicate a hardware or connection problem. Use
  ``-c M95320 --force`` to talk to the chip.

* **2-byte addressing.** The M95320 uses a 16-bit (2-byte) address for its
  read and program instructions instead of the usual 24-bit (3-byte) address.

Note that on this chip only reading has been verified on real hardware so far while
write and erase are implemented but untested.

ZB25VQ16
--------

This added support for a model, but also a new manufacturer Zbit Semiconductor
and the corresponding new file ``flashchips/zbit.c``

util/flashrom_tester deleted
============================

This util has not been maintained in the upstream tree for 2+ years,
and its status in the upstream tree was unknown.

It is actively maintained in the chromium flashrom fork, and used by the dev team there.
There were no users of this util outside of chromium flashrom fork.

Misc updates
============

* platform: Use a single liner for flashrom endianness defines in meson
* platform: Remove intermediate variable in meson
* delay: Remove imprecise internal_sleep function
* progress bar: Improve the warning text when current exceeds total
* cli_classic: Initialize time_start and time_end to zero
* libflashrom: Fix print format specifiers
* helpers.h: Extract helper declarations to a separate header
* platform/endian: Rename+move platform.h to platform/endian.h
* Move all comments about struct flashchip from flashchips.c to flash.h
* dmi: Avoid redefinition of _POSIX_C_SOURCE
* subprojects/cmocka: Update to fix compilation in gcc 15.2.1
* platform/udelay: Extract windows implementation to a separate file
* tests: Replace deprecated assertions with backward compatible ones
* include/platform/: Unify header guards style
* platform/i2c: Move platform dependent code to the platform folder
* custom_baud.h: Unify header guards style
* serial.h: Unify header guards style
* tests: Add basic chip test for chip with FEATURE_STATUS_PER_DIE
* libflashrom: fix build on big-endian targets
* platform/udelay: Extract libpayload implementation to a separate file
* meson_cross: Add a libpayload cross compiling example
* platform/string: Move platform dependent code to a separate file
* gitignore: Ignore meson .wraplock
* ich_descriptors: Guess Wildcat Lake from descriptor content
* cli_classic: Extract alloc_flashsize_buf() helper
* MAINTAINERS: Update programmers file paths in programmers/ directory
* platform/string: Use meson to detect string functions availability
* ch347_spi: Fix misleading error when spispeed is not specified
* platform/meson: Move platform PCI detection to platform/meson.build
* doc: Add includes ordering guidelines
* Use platform/string.h instead of string.h
* serprog: change requested mapping... warning to debug only for spi
* log.h: Extract log declarations to a separate header
* parallel: Avoid performing read/write twice on the same data
* ft2232_spi: Factor out find_ft2232_dev() lookup
* raiden_debug_spi: Extract shared transfer error reporters
* helpers: Add shared parse_voltage() helper
* tests: Add initial set of tests for parse_voltage() function
* i2c_helper: Add shared i2c_write_buffer()/i2c_read_buffer()
* i2c_helper: Add shared i2c_require_allow_brick() helper
* tests: Add unit tests for i2c_require_allow_brick()
* Reorder imports to comply with coding style guidelines
* platform/i2c: Use meson declare dependency to expose platform code
* jedec: Factor out chip reset sequence in probe_jedec
* jedec: Replace magic command bytes with named JEDEC defines
* usb_device: Factor out device revision logging
* helpers_fileio: Factor out file metadata retrieval
* print: Factor out wrapped multiline name token printing
* platform/i2c: Add CONFIG_I2C_HELPER to gate i2c helper code
* programmer: Factor out master shutdown registration
* tests: Tests for invalid input for parse_voltage()
* tests: Add ch347_spi lifecycle and probe tests
* tests: Exercise ch341a and ch347 through the fault programmer
* nicintel: Rename bar fields to match other par_master drivers
* tests: Remove unnecessary mocking for dummyflasher tests
* usb_device: Factor out USB interface claim and revision logging
* par_mmio: Add shared MMIO accessors for parallel PCI drivers
* tests/chip.c: Refactor to use default_layout from flash context
* 94507: raiden: Remove accidental debug print
* linux_mtd: Fix out-of-bounds write in read_sysfs_string()
* linux_mtd: Simplify setup and drop dead code
