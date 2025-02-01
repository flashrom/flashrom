======================
Info on laptops and EC
======================

The page contains some info about flashing the EC on the laptops without official vendor support.

For the info how to flash system firmware on modern laptops with vendor support, please check
"Flashing coreboot" document in user docs.

**Note the document below was migrated from old website and the content >10yrs old**
(exept the links to mailing list and dev guide, those are up-to-date).

Why laptops are difficult
=========================

Laptops, notebooks and netbooks are difficult to support and we recommend to use the vendor flashing utility.
The embedded controller (EC) in these machines often interacts badly with flashing,
either by blocking all read/write access to the flash chip or by crashing
(it may power off the machine or mess with the battery or cause system instability).

ECs are particularly difficult besides blocking access and crashing:

* Almost all ECs do not have public datasheets with programming information.

* Even if the programming information for an EC is public,
  the errata (often needed to use the EC for flashing) are not.

* Even if you have the datasheets with programming information (under NDA or by convincing the EC vendor to release them),
  the following problems remain:

  * flashrom has to detect/probe the EC. For this, it writes a magic byte sequence to a magic port.
    The bad news is that a laptop vendor can customize the magic sequence and the magic port and the response to that,
    so even if you try all magic sequences mentioned in the datasheet on all magic ports mentioned in the datasheet,
    it will either not react (because the laptop vendor came up with its own secret magic sequence/port or
    because the laptop vendor decided to disable all detection support) or it will respond with a non-default ID
    which is not present in any table. If you are really extremely lucky,
    the EC will react to the probing sequence mentioned in the datasheet and respond with the ID mentioned in the datasheet.

  * Most ECs support 3-5 different interfaces to the flash chip. Depending on the software running on the EC,
    none, some or all of them can be disabled. So if you want to support a given embedded controller with reasonable reliability,
    you have to write code for all flash interfaces (usually vastly different from each other).

  * During a flash erase/read/write, flashrom competes with the EC for flash access. The EC usually executes
    its software directly from flash (no RAM involved) and starving the EC instruction decoder may crash the EC.
    If you erase the part of the flash chip where the EC fetches its instructions from, the EC will execute garbage
    and crash/misbehave. When (not if, it always happens) the flash chip is in ID mode during flash probing,
    the EC will mistake the ID data for instructions and execute them, resulting in a crash.

  * You have to find a way to let the EC software run for some time without requiring flash access.
    This can be achieved by writing a loop (not really a loop, but this is just to illustrate the point)
    to EC-internal tiny RAM and then jumping there. Only during that time
    you can access the flash chip reliably without side effects.

  * After flashing, you have to reset the EC so that it can resume running normally.
    This is not optional because the EC controls battery charging (among others) and you want that to work.

Adding support for laptops
==========================

That said, adding support for an EC is doable. Here is a cheat sheet on what to do.

* Find out which flash chip you have by physical inspection of the board
  (or by looking at the internal docs of the manufacturer or the repair manual).

* Find out the controller where the flash chip is attached to. Can be the EC (likely),
  SuperI/O (somewhat likely), Southbridge (pretty unlikely, if it were, you wouldn't be reading this).
  This is a job done by physical inspection of the board (or by looking at the internal docs
  of the manufacturer which sometimes only the ODM and not even the laptop vendor has).

* You can post on the flashrom mailing list (see :ref:`mailing list`) that you plan to work on your EC,
  mention the exact EC model (and laptop model). You can ask if someone has public docs for the EC,
  knows how to get them under NDA or has vendor contacts at the EC vendor.
  With some luck, you might even get docs (some vendors are more friendly than others).

* Get hold of a BIOS image for your laptop (flashrom dump in force mode won't work reliably here).
  Locate the EC code in the BIOS image. The EC often is 8051 compatible or has an architecture with similar limitations.
  The EC code will be uncompressed in flash. Download BIOS images for other laptops with the same EC
  and try to compare the EC code. It may be 100% vanilla code directly from the EC manufacturer
  (which makes your task rather easy) or a heavily customized variant of the original code
  (bad, because you have to analyze it).

* Cross-check the EC code with the EC docs to find out which interfaces for detecting the EC
  and which interfaces for flashing are enabled and working.

* The above two steps can be skipped if you get that information from the laptop vendor.

* Make sure your flash chip is removable in case something goes wrong. If it is not removable,
  look for an in-system-programming header on the board and check if it is supported by flashrom
  (or your hardware programmer of choice). If there is no header and the chip is soldered,
  desolder it and solder a socket in place.

* Dump the old firmware via the programming header or via reading the chip in an external programmer
  and store it safely (backup on a readonly medium like CD-ROM). It contains your laptop serial number,
  your MAC address, and sometimes your Windows preactivation keys.

* (optional) Disconnect the battery. You don't know how the EC behaves if it crashes
  and batteries are expensive and prone to unwanted reactions.

* Start implementing and testing EC detection code. We recommend to do this either
  as external programmer driver init code (and have the code return 1 to make sure flashrom aborts after init)
  or in a standalone program (this helps you keep an overview).

* Start implementing flasher support for your EC. The best way to do this is an external programmer driver.

* Note that you can push your patch for review even if it is still in Work In Progress state, if you want to
  get early feedback and/or share your effort with the people on the mailing list. See :doc:`/dev_guide/development_guide`.

Laptop enable
=============

Most (all?) laptop designs use an EC (embedded controller) to control the backlight,
watch the battery status, etcetera.

To access the flash chip (safely), the EC needs to be suspended/stopped.
The code to stop the EC typically is found at the end of a BIOS binary.
This page is about that trailing part of a binary.

PhoenixBIOS specifics
---------------------

This subsection is about the disassembly of a laptop BIOS file (Compal HTW20, ENE KB910QF, SMC LPC47B227?, Phoenix TrustedCore)
that is 1052598 bytes large, the trailer starts at 0x100000 (1024 * 1024) and is 4022 bytes in size.

To extract the trailer, use dd::

    dd if=<romfile> of=bios.bin bs=1024k skip=1

Example of a disassembly
^^^^^^^^^^^^^^^^^^^^^^^^

The disassembler that is used is IDAPro, the freeware version.

Example of how the disassembly of the trailer looks::

    TRAILER:0010 09 00 00 00 00 00 00 00 00 00 00 43 6F 6D 70 61 ..........Compa
    TRAILER:0020 6C 20 57 69 6E 50 68 6C 61 73 68 20 2D 20 46 6C l WinPhlash - Fl
    TRAILER:0030 61 73 68 69 6E 74 2E 61 73 6D 20 76 30 2E 30 33 ashint.asm v0.03
    TRAILER:0040 5A 46 4C 50 46 25 00 00 00 00 00 00 00 59 02 00 ZFLPF%

Hints and facts
^^^^^^^^^^^^^^^

Facts:

* ZFLPF is found at 0x40
* len(ZFLPH) = 5

The location of the entrypoint is stored at 0x51:

(base of ZFLPF) + len(ZFLPF) + 0xc = 0x40 + 5 + 0xc = 0x51


Let's take a look at position 0x51::

    TRAILER:0051 DE 02 dw 2DEh

Fact:

* 0x2de + 0x1b = 0x2f9

Preview of the begin of the board specific code
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    TRAILER:02F9  ; ---------------------------------------------------------------------------
    TRAILER:02F9 9C pushf
    TRAILER:02FA 60 pusha
    TRAILER:02FB E8 C2 00 call determine_lpc_pci_id
