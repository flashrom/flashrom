======================
ME (Management Engine)
======================

ME stands for Management Engine (or Manageability Engine) and refers to an Embedded Controller found in Intel chipsets. It uses different versions
of an `ARC <http://en.wikipedia.org/wiki/ARC_International>`_ 32-bit microcontroller that runs its own operating system independently from the user's.
The ME has access to all kinds of buses which allows for out-of-band processing which is used for features
like `Active Management Technology <http://en.wikipedia.org/wiki/Intel_Active_Management_Technology>`_, but it makes it also a very interesting target for black hats.
The firmware it runs is secured by certificates stored in ROM, but it is a complex beast and it is very unlikely that there is
no `way around its security measures <http://invisiblethingslab.com/resources/misc09/Quest%20To%20The%20Core%20(public).pdf>`_ (intentional backdoors included).
For further details about the ME please see these excellent `slides by Igor Skochinsky <http://2012.ruxconbreakpoint.com/assets/Uploads/bpx/Breakpoint%202012%20Skochinsky.pdf>`_
and the `Security Evaluation of AMT by Vassilios Ververis <http://web.it.kth.se/~maguire/DEGREE-PROJECT-REPORTS/100402-Vassilios_Ververis-with-cover.pdf>`_.

Effects on flashrom
===================

The firmware of the ME usually shares the flash memory with the firmware of the host PC (BIOS/UEFI/coreboot).
The address space is separated into regions (similar to partitions on a harddisk). The first one (*Descriptor region*)
contains configuration data which contains something similar to a partition table and access rights for the different devices that can access the flash
(host CPU, ME, GbE controller). These restrictions are enforced by the chipset's SPI controller which is the main interface for flashrom
to access the flash chip(s) attached to the chipset. Intel recommends to set the descriptor region read-only and to forbid reads and writes to the ME region by the host CPU.
Writes by the host could interfere with the code running on the ME. This means that flashrom which runs on the host PC can not access
the ME firmware region of the flash at all in this configuration. flashrom detects that, warns the user and disables write access for safety reasons in that case.

Unlocking the ME region
=======================

There are a few ways to enable full access to the ME region, but they are not user friendly at all in general. Also, the Descriptor region is not affected by these actions,
so it is still not possible to access the complete flash memory even when the ME region is unlocked. For the different possibilities please see
the document :doc:`misc_intel`.

Suggested workarounds
=====================

   * If you just want to update the proprietary firmware of the board use the vendor tool(s).
   * If you need full access to the flash chip get an external programmer (see :doc:`/supported_hw/supported_prog/index`) and try in-circuit programming.
   * If you only need to update the BIOS region, then you may use the options ``--ifd -i bios --noverify-all`` to write (and verify) only the BIOS region as described in the Intel flash descriptor.

.. todo:: Migrate page for in-circuit programming (ISP)

See also
========

   * The respective `coreboot page on the management engine <http://www.coreboot.org/Intel_Management_Engine>`_
   * :doc:`misc_intel`
