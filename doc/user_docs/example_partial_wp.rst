===================================
Example of partial write-protection
===================================

This document provides demonstration of how one can protect part of a flash chip
from writing using :code:`flashrom` and its support for manipulating SPI write protection (WP).
This kind of protection requires changing connection of WP pin of the chip to prevent
any attempt of disabling the protection by software alone.

Not to be confused with protection by flash controller of your motherboard (PCH protection).

Version of flashrom
===================

Write-protect manipulation functionality is included in flashrom since release v1.3.0.
If for any reasons you need the latest code from head, you might need to build :code:`flashrom`
from scratch. The following doc describe how to do this: :doc:`/dev_guide/building_from_source`.
See also :doc:`/dev_guide/development_guide`.

Alternatively, your operating system might provide development version of :code:`flashrom` as a package.

Programmer support of WP
========================

Not all programmers support manipulating WP configuration. A suitable programmer must either
provide a dedicated API for working with WP or give sufficiently comprehensive access to the
interface of the flash chip.

In particular, *internal* programmer on Intel platforms might allow only limited access to WP
feature of chips or effectively deny it. Read "Intel chipsets" section of flashrom's manpage
for details on how you can try choosing sequencing type to possibly make WP work for you.

In some cases external flashing might be the only option and you need to unscrew your device,
find the chip, connect it to another device through a suitable adapter and finally be able
to configure it as you wish.

Chip support in flashrom
========================

There is a great variety of chips with some not supporting write protection at all and others
doing it in their own peculiar way of which :code:`flashrom` has no idea. So the first thing to do is
to make sure that :code:`flashrom` knows how WP works for your chip and chipset doesn't get in the way.
Run a command like (adjust this and similar commands below if you're not using *internal* programmer
or need to specify other options)::

   flashrom --programmer internal --wp-status:

Seeing this output line would mean that :code:`flashrom` doesn't know how to use WP feature of the chip you have::

   Failed to get WP status: WP operations are not implemented for this chip

Otherwise the output might contain something similar to this::

   Protection range: start=0x00000000 length=0x00000000 (none)
   Protection mode: disabled

If so, you can continue with the rest of the instructions.

Collecting information about the range
======================================

You need to know where the area you want to protect starts and ends. The example below assumes
you're trying to protect bootblock stored in CBFS at the end of some :code:`coreboot` firmware. In other cases
it might be a separate file which is put at the beginning of a chip. You need to have an idea of what
you're doing here or have some reliable instructions to follow.

In this case :code:`cbfstool` can be used to list information about bootblock like this::

   $ cbfstool rom print | sed -n '2p; /bootblock/p'
   Name                           Offset     Type           Size   Comp
   bootblock                      0x3ef100   bootblock       36544 none

However, the offset is relative to the start of CBFS region, so we also need to find out offset of CBFS::

   $ cbfstool rom layout | grep CBFS
   'COREBOOT' (CBFS, size 4161536, offset 12615680)

Now we can calculate:

* start offset (CBFS offset + 64 + bootblock offset)::

   12615680 + 64 + 0x3ef100 = 0xff7140
   (printf "%#x\n" $(( 12615680 + 64 + 0x3ef100 )))

* end offset (start offset + bootblock size - 1)::

   0xff7140 + 36544 - 1 = 0xffffff
   (printf "%#x\n" $(( 0xff7140 + 36544 - 1 )))

Thus we need to write-protect the smallest area that covers the range from :code:`0xff7140` to :code:`0xffffff`
(both bounds are inclusive).

“64” in the computation of start offset is offset of booblock data. Unfortunately, current tooling
doesn't provide a reliable way of determining actual offset, but 64 is the typical “extra offset” one needs
to add to account for file metadata of CBFS (otherwise it can be its multiple 128 or bigger). Bootblock
should normally end at the last byte of ROM on x86 systems, giving you a way to test the result of computations.

Finding a matching range
========================

In most chips the list of supported ranges is fixed and you can't specify an arbitrary one. Some others
allow more fine-grained control (sector/block-based), but that feature is not supported even by development
version of flashrom at the time of writing (September 2023).

Obtain list of supported ranges from which we'll pick the best match::

	$ flashrom --programmer internal --wp-list
	...
	Available protection ranges:
        start=0x00000000 length=0x00000000 (none)
        start=0x00000000 length=0x00001000 (lower 1/4096)
        start=0x00fff000 length=0x00001000 (upper 1/4096)
        start=0x00000000 length=0x00002000 (lower 1/2048)
        start=0x00ffe000 length=0x00002000 (upper 1/2048)
        start=0x00000000 length=0x00004000 (lower 1/1024)
        start=0x00ffc000 length=0x00004000 (upper 1/1024)
        start=0x00000000 length=0x00008000 (lower 1/512)
        start=0x00ff8000 length=0x00008000 (upper 1/512)
        start=0x00000000 length=0x00040000 (lower 1/64)
        start=0x00fc0000 length=0x00040000 (upper 1/64)
        start=0x00000000 length=0x00080000 (lower 1/32)
        start=0x00f80000 length=0x00080000 (upper 1/32)
        start=0x00000000 length=0x00100000 (lower 1/16)
        start=0x00f00000 length=0x00100000 (upper 1/16)
        start=0x00000000 length=0x00200000 (lower 1/8)
        start=0x00e00000 length=0x00200000 (upper 1/8)
        start=0x00000000 length=0x00400000 (lower 1/4)
        start=0x00c00000 length=0x00400000 (upper 1/4)
        start=0x00000000 length=0x00800000 (lower 1/2)
        start=0x00800000 length=0x00800000 (upper 1/2)
        start=0x00000000 length=0x00c00000 (lower 3/4)
        start=0x00400000 length=0x00c00000 (upper 3/4)
        start=0x00000000 length=0x00e00000 (lower 7/8)
        start=0x00200000 length=0x00e00000 (upper 7/8)
        start=0x00000000 length=0x00f00000 (lower 15/16)
        start=0x00100000 length=0x00f00000 (upper 15/16)
        start=0x00000000 length=0x00f80000 (lower 31/32)
        start=0x00080000 length=0x00f80000 (upper 31/32)
        start=0x00000000 length=0x00fc0000 (lower 63/64)
        start=0x00040000 length=0x00fc0000 (upper 63/64)
        start=0x00000000 length=0x00ff8000 (lower 511/512)
        start=0x00008000 length=0x00ff8000 (upper 511/512)
        start=0x00000000 length=0x00ffc000 (lower 1023/1024)
        start=0x00004000 length=0x00ffc000 (upper 1023/1024)
        start=0x00000000 length=0x00ffe000 (lower 2047/2048)
        start=0x00002000 length=0x00ffe000 (upper 2047/2048)
        start=0x00000000 length=0x00fff000 (lower 4095/4096)
        start=0x00001000 length=0x00fff000 (upper 4095/4096)
        start=0x00000000 length=0x01000000 (all)

Pick a range by scanning the list in the top down order (because the smaller ranges come first):

  * if bootblock is at the start of a chip, look for the first lower range whose length is greater than the end offset
  * if bootblock is at the end of a chip, look for the first upper range which starts before or at the start offset
  * mind that you're unlikely to find an ideal match and will probably protect more than you need; this is fine
    if that's just an empty space, but can cause trouble with future updates if that's some data or metadata which
    changes with every release (see :doc:`fw_updates_vs_spi_wp` for more on this)

This is the first upper range starting before 0xff7140::

  start=0x00fc0000 length=0x00040000 (upper 1/64)

It covers :code:`0x00fc0000 - 0x00ffffff` which includes our bootblock. This area takes up 256 KiB, about 7 times bigger
than our bootblock, but there is no better choice in this case and output of :code:`cbfstool rom layout` shows
that we additionally include a part of 876 KiB empty space which will hopefully remain there in future firmware versions
(it's a good idea to check before a firmware update).

Protection setup
================

The following command sets the range and enables WP at the same time, the values are taken from the chosen range above::

   flashrom --programmer internal --wp-range=0x00fc0000,0x00040000 --wp-enable

You can set the range and change WP status independently as well if needed (just specify one :code:`--wp-*` option at a time).
Make sure that hardware protection is off (state of :code:`W#`/:code:`W/` pin of the chip) or you won't be able
to change WP configuration.

On success, the output of the above command will include such lines::

   Enabled hardware protection
   Activated protection range: start=0x00fc0000 length=0x00040000 (upper 1/64)

**Caveat:** :code:`flashrom` automatically tries to disable WP before any operation on a chip (read, write, erase, verify),
so double-check status of WP before changing state of WP pin on your chip!

Verifying hardware protection
=============================

Once you're happy with the configuration and changed state of WP pin, you can try disabling WP
using :code:`flashrom --wp-disable` to make sure that it fails now.
