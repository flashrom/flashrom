===================
Board Testing HOWTO
===================

**Note the document below was migrated from old website and the content >10yrs old**
Patches to add/update documentation are very much appreciated.

This page gives you mainly hints on how to test flashrom support on mainboards.
Testing for graphics / network / SATA cards and external programmer devices is similar but less dangerous.

.. container:: danger, admonition

   Important information

   DO NOT ATTEMPT TO FOLLOW THESE INSTRUCTIONS UNLESS YOU KNOW WHAT YOU ARE DOING!
   THIS CAN RENDER YOUR MAINBOARD TOTALLY UNUSABLE! YOU HAVE BEEN WARNED!

* If you have a laptop/notebook/netbook, please do NOT try flashrom because interactions
  with the EC on these machines might crash your machine during flashing.
  flashrom tries to detect if a machine is a laptop, but not all laptops follow the standard,
  so this is not 100% reliable.

* To check whether flashrom knows about your chipset and ROM chip, run ``flashrom -p internal``.

* If it says ``"Found chipset CHIPSETNAME..."`` and ``"CHIPNAME found at..."`` that's a good first sign.

* To check if you can read the existing BIOS image from the chip, run ``flashrom -p internal -r backup.bin``.
  Make sure that backup.bin contains a useful BIOS image (some chipsets will return 0xff for large areas
  of flash without any error messages, but there might be actually large areas of 0xff in the original image as well).

* Now the really important part, checking if *writing* an image on the chip works:

  * First, if the board has a flash socket and you have spare chips,
    make sure you have a backup chip containing the original BIOS.
    Also, you should have verified that it actually boots your system successfully.
    Put away that backup chip somewhere safe and insert a chip which you can safely
    overwrite (e.g. an empty one you bought).

  * If that is not possible make sure you have some other way for complete recovery,
    e.g. an external programmer that can do :doc:`/user_docs/in_system` on the board in question.

  * Then write an image onto the chip, which is different from what's on the chip right now:
    ``flashrom -p internal -w new.bin``. If the image is equal you will get a notice since r1680.
    If this works and flashrom reports "VERIFIED" your board is supported by flashrom.

  * If not, you might try to enable the "Enable BIOS Update" or "Write-protect BIOS"
    or similar options in your BIOS menu first, or set a jumper on your board
    (this is highly board-dependent). Also, you might have to use the flashrom --mainboard
    switch for some boards.

  * If none of the above helps (but flashrom still *does* detect your chipset and ROM chip),
    there's quite likely a board-specific initialization required in flashrom,
    which is non-trivial to add (e.g. toggling certain custom GPIO lines etc).
    In that case, contact us as we may be able to help.

  * If you can't risk a write on a given chip and if the chip is SPI, the following guidelines may help:

    * Try probing.

    * For ICH/VIA SPI, lockdown can mean probe works, but write/erase or even read doesn't.
      It can also mean that probe does not work, but write/read/erase (or any subset thereof) would work.
      For all other SPI chipsets, there is no such lockdown, so you can issue any erase/write/read command.

    * However, some SPI chips have a WP# pin which causes the block protection bits to become readonly.
      Now if flashrom has a generic block protection checker for your chip, we're able to figure out
      if write/erase is possible. Basically, you can check if you need a board enable
      by setting all block protection bits, then unsetting them. If either of the operations fail,
      you need a board enable. If they succeed, erase and write are guaranteed to work.

Please tell us about your results by sending the output of flashrom commands you ran,
the exact board manufacturer and model name, and all of your observations and test results
to the flashrom :ref:`mailing list`.
