**flashrom** is a utility for identifying, reading, writing, verifying and erasing flash
chips. It is designed to flash BIOS/EFI/coreboot/firmware/optionROM images on mainboards,
network/graphics/storage controller cards, and various other programmer devices.

* Supports more than 476 flash chips, 291 chipsets, 500 mainboards, 79 PCI devices,
  17 USB devices and various parallel/serial port-based programmers.
* Supports parallel, LPC, FWH and SPI flash interfaces and various chip packages (DIP32,
  PLCC32, DIP8, SO8/SOIC8, TSOP32, TSOP40, TSOP48, BGA and more).
* No physical access needed, root access is sufficient (not needed for some programmers).
* No bootable floppy disk, bootable CD-ROM or other media needed.
* No keyboard or monitor needed. Simply reflash remotely via SSH.
* No instant reboot needed. Reflash your chip in a running system, verify it, be happy.
  The new firmware will be present next time you boot.
* Crossflashing and hotflashing is possible as long as the flash chips are electrically
  and logically compatible (same protocol). Great for recovery.
* Scriptability. Reflash a whole pool of identical machines at the same time from the
  command line. It is recommended to check flashrom output and error codes.
* Speed. flashrom is often much faster than most vendor flash tools.
* Portability. Supports DOS, Linux, FreeBSD (including Debian/kFreeBSD), NetBSD, OpenBSD,
  DragonFlyBSD, anything Solaris-like, Mac OS X, and other Unix-like OSes as well as GNU Hurd.
  Partial Windows support is available (no internal programmer support at the moment, hence
  no "BIOS flashing").

.. todo:: Convert Technology page and add links here
