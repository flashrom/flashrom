========================================
Firmware updates vs SPI write-protection
========================================

Enabling write-protection of any kind is meant to obstruct changing data, but it also limits
what you can do to the part of firmware that's still writable. This document is meant to cover
some of the origins of such limitations and situations which might arise after
part of a flash chip has been protected.

Firmware updates after locking bootblock
========================================

This section is primarily concerned with :code:`coreboot` with bootblock being protected from writing,
but similar problems can happen for any kind of firmware.

Risks of partial updates
------------------------

Partial updates can produce an unbootable image if an old bootblock doesn't work with a more recent
version of :code:`coreboot`. This can be manifested in various ways ranging from an old bootblock not being able
to find new romstage to system booting successfully but data in :code:`coreboot` tables being mangled or incomplete.

The incompatibilities might happen when switching version of firmware or even when using the same version
with a slightly different configuration.

Another thing that can potentially cause trouble is CBFS layout. When bootblock is part of CBFS,
it doesn't necessarily have a fixed address, moreover it can change location as well if it depends on file size
(when bootblock's last byte must be the last byte of the image, which is the case on x86). If newer bootblock
is smaller such that an old WP range now covers bootblock and some other file, this file won't be fully updated
due to write-protection, potentially resulting in a corrupt image. Luckily, when bootblock is the last file
it's normally preceded by a significant amount of empty space, which won't let this situation to occur.

On top of that, last 4 bytes of the image contain offset to the master header of CBFS. Depending on
the :code:`coreboot` version this offset might be crucial for the loading of romstage, in which case moving CBFS
within the image without updating the offset (when it's locked by WP) can also prevent the system from booting.

Recovering from a broken state
------------------------------

Since broken flash won't let the system to boot, the way to fix it is to flash the chip externally by connecting
it to a different device. A possible alternative could be to have a backup flash created beforehand and swapping
it for the broken one (not very applicable if swapping doesn't require soldering). There are also some mainboards
with dual flash chips one of which acts as a backup that can be restored by holding power on button long enough.

Flashing whole firmware image
=============================

The function of the hardware protection mechanism (:code:`W#` or :code:`W/` pin of flash chips) is to lock state of
software protection thus preventing it from being disabled. After the chip is physically unlocked by changing
the state of the pin, the state of the write protection doesn't change. However, in this state the protection
can be easily turned off programmatically, which is what :code:`flashrom` tries to do before performing an operation on a chip.

In other words, changing state of the WP pin might be enough to be able to flash the chip in full.
If :code:`flashrom` errors or you don't want to rely on the automatic behaviour, you can try to
explicitly disable the protection by running :code:`flashrom` like this::

   flashrom --wp-disable

If you need to pass extra parameters to flash your chip (e.g., programmer or chip name), add them to the above command
(order of such parameters shouldn't matter).
