=====================
How to add a new chip
=====================

To add a new chip definition you need to send a patch to flashrom. This can be your first patch to flashrom. However if it is
your first patch please read carefully :doc:`/dev_guide/development_guide`, and set up dev environment locally.

The expectation is that you have tested successfully at least some of the operations on the chip (not necessarily all of them).
There is an exception: when datasheet covers more than one model of the same chip, but you only have one and tested that one.
Typically, you would add all the models covered by the datasheet you have, but only mark tested the one you tested.

To begin with, make sure you have a link to the publicly available datasheet for the chip you want to add. Publicly
available datasheet is a requirement for adding a new chip.

Open the datasheet, ``flashchips.c`` and ``include/flashchips.h``, and then

Model ID and vendor ID
======================

Find model ID and vendor ID in the datasheet. This information often can be found in the sections like "Device identification",
"Device ID", "Manufacturer ID", "Table of ID definitions" or something similar to that.
These values come in combination with commands used for probing the chip (more on probing below). Most commonly used command for
probing SPI chips is ``0x9f`` which corresponds to ``PROBE_SPI_RDID``.

At this point you need to double-check if exact same chip is already defined. Search for the ``.name`` and ``.vendor`` of the chip
you have in ``flashchips.c`` and if found, compare values of ``.manufacture_id`` and ``.model_id``. If all 4 values match, it
seems like the chip already defined. In this case you can try using it. Possibly, only the testing status of the chip needs to be
updated, in this case see :doc:`how_to_mark_chip_tested` for instructions.

Vendor (manufacturer) ID
------------------------

Search for vendor ID in ``include/flashchips.h``, IDs are defined as macros. If it exists already, use existing macro in your
chip definition. If it does not exist (which means your chip is the first ever for this vendor), define macro for this ID,
add a comment. Look at existing IDs in ``include/flashchips.h`` as examples.

Model ID
--------

Search for model ID in ``include/flashchips.h``. If there is no macro defined for it, add macro for model ID in the corresponding
section for given vendor.

Model IDs should follow the pattern. They are all uppercase; first the manufacturer name (used for the manufacturer IDs macros
on top of each paragraph in flashchips.h), followed by an underscore and then the chipname. The latter should in general equal
the ``.name``, with dots (and other disallowed characters) replaced by underscores. Shared chip IDs typically use the macro name
that happened to be added first to flashrom (which is also probably the first one manufactured) and which usually matches
the other chips of that series in ``include/flashchips.h``.

If you added a new model ID, use it in new chip definition, skip the rest of instructions about Model ID
and continue with the next section "Properties".

Model ID already exists
^^^^^^^^^^^^^^^^^^^^^^^

It is possible that model ID already exists in the header. Then find the corresponding definition in ``flashchips.c`` and inspect
it carefully. Is it the same chip that you have or a different one?

The question you need to figure out is: whether existing definition can work for your chip or not. You figure that out by
reading the rest of instructions and comparing the values from the datasheet with existing definition.

Existing definition matches, but it has a different chip name
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It is possible all the values from the datasheet match existing definition, but it has a different chip name. This is fine,
and in this case you need to add a comment to the macro ID definition which looks like ``Same as ...``.
Look at existing examples in ``include/flashchips.h`` file.
You should change the ``.name`` to reflect the additional chip model (see other chips of naming examples).
You don't need to add new definition.

Existing definition does not match
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It is possible that, despite of the same model ID, existing vs new chips have significant different and they
need different definitions. Judging the significance of a difference might be not trivial and might require some understanding
of flashrom behavior, however the examples of significant differences are: different sizes of erase blocks,
or different opcodes for operations (see more on operations below), or different configuration for write-protection (if this
config is defined).

In this case you need to do both:

* Add a comment to existing model ID macro, ``Same as ...``
* Add new chip definition, re-use existing ID macro

Note that if you need to add new chip definition, it is fine to copy a similar one that already exist and correct the values.

Make sure to keep the alphabetic ordering of chip names, and place new definition into the section with other chips
by the same vendor.

Properties
==========

* From the datasheet, get ``.vendor``, ``.name``, ``.bustype``, ``.total_size``.
* ``.voltage`` defines the upper and lower bounds of the supply voltage of the chip. If there are multiple chip models
  with different allowed voltage ranges, the `intersection <https://en.wikipedia.org/wiki/Intersection_(set_theory)>`_
  should be used and an appropriate comment added.
* ``.page_size`` is really hard.
  Please read this `long explanation <https://mail.coreboot.org/pipermail/flashrom/2013-April/010817.html>`_,
  or ignore it for now and set it to 256.
* ``.tested`` is used to indicate if the code was tested to work with real hardware, its possible values are defined
  in ``include/flash.h``. Without any tests it should be set to ``TEST_UNTESTED``.
  See also another doc :doc:`how_to_mark_chip_tested`.

Feature Bits
============

We encode various features of flash chips in a bitmask named ``.feature_bits``.
Available options can be found in ``include/flash.h``, look for macros defined by the pattern ``#define FEATURE_XXX``.

Some of the feature bits have more detailed docs, see below.

Write-Status-Register (WRSR) Handling
-------------------------------------

The Write Status Register (WRSR) is used exclusively in SPI flash chips to configure various settings within the flash chip,
including write protection and other features.
The way WRSR is accessed varies between SPI flash chips, leading to the need for these feature bits.

* ``FEATURE_WRSR_EWSR``
  indicates that we need an **Enable-Write-Status-Register** (EWSR) instruction which opens the status register for the
  immediately-followed next WRSR instruction. Usually, the opcode is **0x50**.

* ``FEATURE_WRSR_WREN``
  indicates that we need an **Write-Enable** (WREN) instruction to set the Write Enable Latch (WEL) bit. The WEL bit
  must be set prior to every WRSR command. Usually, the opcode is **0x06**.

* ``FEATURE_WRSR_EITHER``
  indicates that either EWSR or WREN is supported in this chip.

Operations
==========

Each operation is defined as a enum value from a corresponding enum.

Probe
-----

``.probe`` indicates which function is called to fetch IDs from the chip and to compare them with the ones in
``.manufacture_id`` and ``.model_id``. For most SPI flash chips ``PROBE_SPI_RDID`` is the most commonly used if the datasheets
mentions **0x9f** as an identification/probing opcode.

To see the full list of available probing functions, check definition of ``enum probe_func`` in ``include/flash.h``.
You may need to inspect the source code of what a probing function is doing, check the mapping ``lookup_probe_func_ptr`` and
search for the function code.

``.probe_timing`` is only used for non-SPI chips. It indicates the delay after "enter/exit ID mode" commands in microseconds
(see ``include/flash.h`` for special values).

Read and write
--------------

``.read`` and ``.write`` indicate which functions are used for reading and writing on the chip. Currently flashrom
does only support a single function each. The one that is best supported by existing programmers should be used for now,
but others should be noted in a comment if available.

To see the full list of available functions, check definitions of ``enum read_func`` and ``enum write_func`` in ``include/flash.h``.
To inspect the source code, check the mappings ``lookup_write_func_ptr`` and ``lookup_read_func_ptr`` and search for
the function code.

The write granularity can be expressed by the ``.gran`` field. If you think you need something else than the default
``write_gran_256bytes`` then you should definitely ask one of the regular flashrom hackers first.
Possible values can be found in ``include/flash.h``.

Erase
-----

``block_erasers`` stores an array of pairs of erase functions (``.block_erase``) with their respective layout (``.eraseblocks``).

``.block_erase`` is similar to the probing function. You should at least check that the opcode named in the function name
is matching the respective opcode in the datasheet.

To see the full list of available functions, check definition of ``enum block_erase_func`` in ``include/flash.h``.
To inspect the source code, check the mappings ``lookup_erase_func_ptr`` and search for the function code.

Two forms of ``.eraseblocks`` can be distinguished: symmetric and asymmetric layouts.
Symmetric means that all blocks that can be erased by an opcode are sized equal. In that case a single range can define
the whole layout (e.g. ``{4 * 1024, 256}`` means 256 blocks of 4 kB each). Asymmetric layouts on the other hand contain
differently sized blocks, ordered by their base addresses (e.g. ``{{8 * 1024, 1}, {4 * 1024, 2}, {16 * 1024, 7}}`` describes
a layout that starts with a single 8 kB block, followed by two 4 kB blocks and 7 16 kB blocks at the end).

``.eraseblocks`` should be listed in order, from the smallest to the largest size.

Printlock
---------

``.printlock`` is a `misnomer to some extent <https://mail.coreboot.org/pipermail/flashrom/2011-May/006495.html>`_.

It is used not only to print (write) protected address ranges of the chip, but also to pretty print the values
of the status register(s) - especially true for SPI chips. There are a lot of existing functions for that already
and you should reuse one if possible. Comparing the description of the status register in the datasheet of an already
supported chip with that of your chip can help to determine if you can reuse a printlock function.

Check for definition of ``enum printlock_func`` and ``lookup_printlock_func_ptr`` for available options and source code.

Unlock
------

``.unlock`` is called before flashrom wants to modify the chip's contents to disable possible write protections.
It is related to the ``.printlock`` function as it tries to change some of the bits displayed by ``.printlock``.

Check for definition of ``enum blockprotect_func`` and ``lookup_blockprotect_func_ptr`` for available options and source code.

Write-protection
================

Write-protection support is optional, and if you haven't tested it on the chip, don't add it.
If you, however, used and tested it, that would be great to add to chip definition.

Registers bits
--------------

``.reg_bits`` stores information about what configuration bits the chip has and where they are found.

For example, ``.cmp = {STATUS2, 6, RW}`` indicates that the chip has a complement bit (``CMP``) and it is the 6th bit
of the 2nd status register. See ``struct reg_bit_info`` in ``include/flash.h`` for details on each of the structure's fields.

Note that some chips have configuration bits that function like ``TB/SEC/CMP`` but are called something else in the datasheet
(e.g. ``BP3/BP4/...``). These bits should be assigned to a field *according their function* and the datasheet name should be
noted in a comment, for example:

:code:`.sec = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like SEC */`

Decode range
------------

``.decode_range`` points to a function that determines what protection range will be selected by particular configuration
bit values. It is required for write-protect operations on the chip.

Check for definition of ``enum decode_range_func`` and ``lookup_decode_range_func_ptr`` for available options and source code.

Test your changes
=================

After making changes in the code, rebuild flashrom, run unit tests, and test the chip.

Add testing information to commit message.

When all of the above done, follow :doc:`/dev_guide/development_guide` to push a patch and go through review process.
Dev guide has more details on the process.
