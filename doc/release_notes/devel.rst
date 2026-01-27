===============================
Recent development (unreleased)
===============================

This document describes the major changes that are expected to be included in
the next release of flashrom and which are currently only available by source
code checkout (see :doc:`../dev_guide/building_from_source`). These changes
may be further revised before the next release.

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
