===============================
Recent development (unreleased)
===============================

This document describes the major changes that are expected to be included in
the next release of flashrom and which are currently only available by source
code checkout (see :doc:`../dev_guide/building_from_source`). These changes
may be further revised before the next release.

Fudan FM25W128 flash chip support
---------------------------------

Added support for the Fudan FM25W128 SPI flash chip, including JEDEC
identification, erase geometry, OTP and QPI capability metadata, and
write-protect range decoding for the chip's BP table layout.

Multi-die flash chip support preparation
-----------------------------------------
Feature is currently under development. Supporting multi-die flash chips.
Added two new fields in the ``struct flashchip`` definition:

* ``.die_size`` - Size of a single die in kilobytes. Set for multi-die chips.

* ``.die_select`` - callback for die selection. Different vendors can use
  different opcodes and methods to select active dies.

This enables future support for multi-die flash chips without affecting existing single-die
chip support. Chip definitions can now be prepared with die information ahead of time,
and custom read/write functions can leverage this information for die-aware operations.

New chip support
----------------

Added support for the ST M95320, a small 32 Kbit (4 kB) SPI EEPROM from the
M95xxx family.

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

New feature flags
-----------------

* ``FEATURE_ADDR_2BYTE`` -- Marks a chip that uses a 16-bit (2-byte) address in
  its read and program instructions rather than the default 24-bit (3-byte)
  address. This is used by small SPI EEPROMs such as the ST M95320 (parts up to
  64 KiB in the M95xxx family). When set, ``spi_prepare_address()`` emits only
  two address bytes.

New programmers
---------------

* ``fault`` -- A fault injection programmer that wraps an existing backend
  (e.g. ``dummy``) and injects deterministic, reproducible faults. Supports
  bit corruption, short reads, write failures, write lies (silent no-ops),
  and partial writes. Uses a seeded PRNG for reproducibility. Intended for
  testing verification logic and retry mechanisms under realistic failure
  conditions.

New options
-----------

* ``--read-repeated[=<count>] [<file>]``: Read the flash chip multiple times
  (default: 3, minimum: 3) and use majority voting to detect unstable
  connections. Deduplicates read results to minimise memory usage. If a file
  is given and a strict majority is found, saves the majority content to it.
  Useful for diagnosing flaky SPI wiring or unreliable programmers.
