========================
Miscellaneous Intel info
========================

BBAR on ICH8
============

There is no sign of BBAR (BIOS Base Address Configuration Register) in the
public datasheet (or specification update) of the ICH8. Also, the offset of
that register has changed between ICH7 (SPIBAR + 50h) and ICH9 (SPIBAR +
A0h), so we have no clue if or where it is on ICH8. Out current policy is to
not touch it at all and assume/hope it is 0.

Software Sequencing vs. Hardware Sequencing and the "Opaque flash chip"
=======================================================================

Software sequencing and hardware sequencing are two methods used to interface
with the SPI controller on Intel platforms. They can be selected using either
ich_spi_mode=swseq or ich_spi_mode=hwseq programmer parameters. Flashrom will
attempt to automatically detect which mode to use.

Software sequencing is the traditional method whereby software running on the
CPU handles most of the logic needed to interact with the flash chip. This
offers good flexibility since the user can utilize any opcode available in the
OPMENU registers, and OPMENU can be left unlocked or on coreboot-supported
platforms the owner of the system may program it for their needs before locking
it. Advanced or non-standard features of a chip such as write protection and
OTP may therefore be directly utilized by software.

Hardware sequencing is a newer method (since around 2011) whereby most of the
logic for interacting with the SPI flash chip is contained within the SPI
controller itself and software such as flashrom may only select a few operations
chosen by Intel via the Flash Cycle (FCYCLE) field. The chip must conform to
specifications from Intel for each chipset/PCH. The specs are given in the
"SPI Programming Guide" application note. See [SPI_PROG] cited at the bottom of
this document for an example.

Hardware sequencing simplifies things from a software perspective since the
software is guaranteed some minimal level of support and doesn't even need to
know the chip's ID or opcodes; it just needs to tell the SPI controller to
perform a type of transaction such as "read", "4k block erase", etc. Hence when
using hardware sequencing one will see "Opaque flash chip" as the chip's
description since software might not be able to identify the chip. The SPI
controller can combine multiple physical flash chips to logically appear as a
single large flash device, and in such cases it would not make sense for
flashrom to try to identify the chip.

In many non-Intel systems the software has full control of a generic SPI
controller where the software controls the SPI signals and also constructs the
data payload including pre-op (e.g. write enable latch), opcode, address, and
data. Intel SPI flash controllers are purpose-built for flash chip access and
the software does not control the hardware directly. This makes Intel SPI
controllers less flexible from a software standpoint, however there are some
benefits such as guaranteed atomicity and multi-master arbitration needed for
modern Intel platforms where the CPU and various microprocessors can share the
same flash chip.

SMM BIOS Write Protection
=========================

Sometimes a hardware vendor will enable "SMM BIOS Write Protect" (SMM_BWP)
in the firmware during boot time. The bits that control SMM_BWP are in the
BIOS_CNTL register in the LPC interface.

When enabled, the SPI flash can only be written when the system is operating in
in System Management Mode (SMM). In other words, only certain code that was
installed by the BIOS can write to the flash chip. Programs that run in OS
context such as flashrom can still read the flash chip, but cannot write to the
flash chip.

Flashrom will attempt to detect this and print a warning such as the following:
"Warning: BIOS region SMM protection is enabled!"

Many vendor-supplied firmware update utilities do not actually write to the ROM;
instead they transfer data to/from memory which is read/written by a routine
running in SMM and is responsible for writing to the firmware ROM. This causes
severe system performance degradataion since all processors must be in SMM
context (ring -2) instead of OS context (ring 0) while the firmware ROM is being
written.

Accesses beyond region bounds in descriptor mode
================================================

Intel's flash image tool will always expand the last region so that it covers
the whole flash chip, but some boards ship with a different configuration.
It seems that in descriptor mode all addresses outside the used regions can not
be accessed whatsoever. This is not specified anywhere publicly as far as we
could tell. flashrom does not handle this explicitly yet. It will just fail
when trying to touch an address outside of any region.
See also http://www.flashrom.org/pipermail/flashrom/2011-August/007606.html

(Un)locking the ME region
=========================

If the ME region is locked by the FRAP register in descriptor mode, the host
software is not allowed to read or write any address inside that region.
Although the chipset datasheets specify that "[t]he contents of this register
are that of the Flash Descriptor" [PANTHER], this is not entirely true.
The firmware has to fill at least some of the registers involved. It is not
known when they become read-only or any other details, but there is at least
one HM67-based board, that provides an user-changeable setting in the firmware
user interface to enable ME region updates that lead to a FRAP content that is
not equal to the descriptor region bits [NC9B].

There are different ways to unlock access:

 * A pin strap: Flash Descriptor Security Override Strap (as indicated by the
   Flash Descriptor Override Pin Strap Status (FDOPSS) in HSFS. That pin is
   probably not accessible to end users on consumer boards (every Intel doc i
   have seen stresses that this is for debugging in manufacturing only and
   should not be available for end users).
   The ME indicates this in bits [19:16] (Operation Mode) in the HFS register of
   the HECI/MEI PCI device by setting them to 4 (SECOVR_JMPR) [MODE_CTRL].

 * Intel Management Engine BIOS Extension (MEBx) Disable
   This option may be available to end users on some boards usually accessible
   by hitting ctrl+p after BIOS POST. Quote: "'Disabling' the Intel ME does not
   really disable it: it causes the Intel ME code to be halted at an early stage
   of the Intel ME's booting so that the system has no traffic originating from
   the Intel ME on any of the buses." [MEBX] The ME indicates this in
   bits [19:16] (Operation Mode) in the HFS register of the HECI/MEI PCI device
   by setting them to 3 (Soft Temporary Disable) [MODE_CTRL].

 * Previous to Ibex Peak/5 Series chipsets removing the DIMM from slot (or
   channel?) #0 disables the ME completely, which may give the host access to
   the ME region.

 * HMRFPO (Host ME Region Flash Protection Override) Enable MEI command
   This is the most interesting one because it allows to temporarily disable
   the ME region protection by software. The ME indicates this in bits [19:16]
   (Operation Mode) in the HFS register of the HECI/MEI PCI device by setting
   them to 5 (SECOVER_MEI_MSG) [MODE_CTRL].

MEI/HECI
========

Communication between the host software and the different services provided by
the ME is done via a packet-based protocol that uses MMIO transfers to one or
more virtual PCI devices. Upon this layer there exist various services that can
be used to read out hardware management values (e.g. temperatures, fan speeds
etc.). The lower levels of that protocol are well documented:
The locations/offsets of the PCI MMIO registers are noted in the chipset
datasheets. The actually communication is documented in a whitepaper [DCMI] and
an outdated as well as a current Linux kernel implementation (currently in
staging/ exist [KERNEL]. There exists a patch that re-implements this in user
space (as part of flashrom).

Problems
========

The problem is that only very few higher level protocols are documented publicly,
especially the bunch of messages that contain the HMRFPO commands is probably
well protected and only documented in ME-specific docs and the BIOS writer's
guides. We are aware of a few leaked documents though that give us a few hints
about it, but nothing substantial regarding its implementation.

The documents are somewhat contradicting each other in various points which
might be due to factual changes in process of time or due to the different
capabilities of the ME firmwares, example:

Intel's Flash Programming Tool (FPT) "automatically stops ME writing to SPI
ME Region, to prevent both writing at the same time, causing data corruption." [ME8]

"FPT is not HMRFPO-capable, so needs [the help of the FDOPS pin] HDA_SDO if
used to update the ME Region." [SPS]

When looking at the various ME firmware editions (and different chipsets), things
get very unclear. Some docs say that HMRFPO needs to be sent before End-of-POST
(EOP), others say that the ME region can be updated in the field or that some
vendor tools use it for updates. This needs to be investigated further before
drawing any conclusion.

[PANTHER]
   Intel 7 Series Chipset Family Platform Controller Hub (PCH) Datasheet
   Document Number: 326776, April 2012, page 857

[NC9B]
   Jetway NC9B flashrom v0.9.5.2-r1517 log with ME region unlocked.
   NB: "FRAP 0e0f" vs. "FLMSTR1 0a0b".
   http://paste.flashrom.org/view.php?id=1215

[MODE_CTRL]
   Client Platform Enabling Tour: Platform Software
   Document Number: 439167, Revision 1.2, page 52

[MEBX]
   Intel Management Engine BIOS Extension (MEBX) User's Guide
   Revision 1.2, Section 3.1 and 3.5

[DCMI]
   DCMI Host Interface Specification
   Revision 1.0

[SPI_PROG]
   Ibex Peak SPI Programming Guide
   Document Number: 403598, Revision 1.3, page 79

[ME8]
   Manufacturing with Intel Management Engine (ME) Firmware 8.X on Intel 7 Series
   Revision 2.0, page 59

[SPS]
   Manufacturing with Intel Management Engine (ME) on Intel C600 Series Chipset 1
   for Romley Server 2 Platforms using Server Platform Services (SPS) Firmware
   Revision 2.2, page 51
