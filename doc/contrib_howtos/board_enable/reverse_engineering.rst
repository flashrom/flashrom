===========================================
Finding Board Enable by Reverse Engineering
===========================================

**Note the document below was migrated from old website and the content >10yrs old**
Patches to add/update documentation are very much appreciated.

Finding the board enable code inside the flash rom seems like looking for a needle in a haystack,
but using the right search method might be as easy as finding a magnetic needle using a strong magnet.
This page explains approaches to find the board enable code in different vendor BIOSes.

Useful Tools
============

For free/open source reverse engineering tools, take a look at objdump and ndisasm.

For objdump, use::

    objdump -b binary -m i386 -M i8086,intel --disassemble-all datafile.bin

(you might want to leave off the "intel" option if you prefer the AT&T assembler syntax)

But as all free tools we know of are not comparable to the commercial tool `IDA Pro <http://www.hex-rays.com/idapro/>`_
which has free-as-in-beer version for non-commercial use `IDA 4.9 Freeware <http://www.hex-rays.com/idapro/idadownfreeware.htm>`_
which has all features needed for BIOS analysis, it should be mentioned here, too.

General Hints
=============

* Get an lspci of that board before you start. Best way is to request it from the board owner,
  but you might find it on Google. Everest Reports of systems include an hexdump of config space at the end.

* Get used to the binary PCI address specification: Device/function ID is combined into one byte,
  the device ID (0..31) is multiplied by eight and added/ored with the function ID (0..7).
  Most PCI addresses in the BIOS have the resulting byte hardcoded.
  So if this byte is 0xF9 for example, it is 1f.1 in lspci syntax.

* In/Out to fixed I/O addresses above 0x400 is usually accessing GPIO pins.
  The base address can usually be found in PCI config space, but many BIOS vendors hard-code
  the address as they know what address their own BIOS configures the GPIO address to.
  Typical base values are 0x400 or 0x800 on Intel ICH and 0x4400 on nVidia MCP.
  Check the GPIO setting function for the chipset on the board early to recognize patterns
  in the assembly code (you *did* get the lspci before you started,
  so you can easily look up which chipset and which GPIO method is used, didn't you?)

Vendor-specific Hints
=====================

Award BIOS
----------

First, you need the runtime BIOS, this is the 128KB thats available at the addresses E0000-FFFFF
when the system is running. You can either obtain it by dumping from a running system,
or by running "lha x bios.bin" on a BIOS image as it gets written to the chip.

From the 128KB you only need the second half (the f-segment). In this segment, look for the text "AWDFLASH".
This signature is followed by eleven 16-bit procedure offsets (all these procedures reside in the segment F000).
The second procedure offset points to the board/chipset enable function.
The third procedure offset points to the board/chipset disable function.

Useful hints:

* PCI register manipulation is common. If you find a procedure that outputs something to port CF8,
  it accesses PCI configuration space. If a second out instruction follows, it is a PCI config write,
  if an in instruction follows, it's a PCI config read. In AWARD BIOS code,
  the Device/Function ID is passed in CH, the config space address in CL. Bus number
  (if used at all, check the procedure called) in BH or BL. Data is exchanged via AL/AX/EAX

AMI BIOS
--------

Get the runtime F-segment of the BIOS. Currently there is no automated extraction method
available than dumping from a running machine. Flashing is handled through Int 16
(entry point at F000:E82E, look for AH=E0, usually catched in the very beginning),
but can also be found in a different way. For each similar group of flash chips,
there is a call table with three 16 bit offsets for identifying, erasing and writing the chip.
In most AMI biosses, this table is quite closely followed by a pointer to the flash chip name
and a list of strings containing all flash chip names in this group. The identification procedure
adjusts the pointer to point to the right string. Finding flashing procedures can be done by searching
for magic constants like 0x2AA, 0xAAA, 0x2AAA and looking at the code. All low-level-flashing code
for one group is quite close together, so poking around to find the main entry points is feasible.

There is a table at some other place that (amongs other data) contains the offset
of the call table for each group. The offset of this table is referenced in the generic flashing code
that tends to be shortly before the group-specific code. Usually, on detection the address of one group
call table is stored in a "current group" pointer, which in the running system points to one
of the group entries, too. There will be a function that references this current group pointer
and calls the erase function [bx+2] (maybe via a looping helper function) and the write function
[bx+4] in quick succession. The function called before the erase function is the flash enable function.

If you go the forward route, trace Int 16, AX=E025 which should call the erase procedure.

Useful hints:

* PCI access in AMI commonly uses a bit fiddling function. This function takes the bus number in DH,
  the device/function number in DL, a bit mask in BH, bits to be set in BL, and the config space address in AH.
  It clears all bits set in BH and sets the bit set in BL. It returns the old value (all bits) in AL,
  and sets the zero flag if all bits in BH were already clear. This function is wrapped by a generic bit setting
  and a generic bit clearing function. These functions take a bit mask (usually only one bit set) in AL.
  The bit setting function calls the bit manipulation function with BH=0 and BL=AL,
  the bit clearing function calls the bit manipulation function with BH=AL and BL=0.

FOSDEM presentation
===================

There was a talk at FOSDEM 2010 about reverse engineering board enables.
The `slides are here <http://people.freedesktop.org/~libv/flash_enable_bios_reverse_engineering_(FOSDEM2010_-_slides).pdf>`_
