=====================
Board Enable Overview
=====================

**Note the document below was migrated from old website and the content >10yrs old**
Patches to add/update documentation are very much appreciated.

The term "board enable" describes mainboard specific code to make flash rom
write access possible on this mainboard. Usually this means programming some
directly programmable output pin (usually called general purpose I/O or GPIO for short)
of the chipset that is connect to a write protect input of the BIOS ROM chip to release write protection.

How to find the board enable procedure on your board
====================================================

The reverse engineering method at the end is preferred because it has a bit lower risk.

Try & Error method on GPIO pins
-------------------------------

The following is a mail from Ron:

we just had the need to find a flash write enable on some servers.
These are Dell S1850s and we're tired of having a non-Linux-based Flash tool,
and, still worse, one to which we do not have source. Flashrom would be great,
save that it can't get the flash to write. We decided to see if it was the classic
GPIO-enabled FLASH write pin, which is the standard it seems in PC hardware.

In this note I am just describing a program that I wrote long ago at LANL
and have used from time to time when I could not get the info I needed on enabling FLASH write.

One thing we have found over the past 10 years: the single most common write enable control
is a GPIO attached to a southbridge. Don't know why it always seems to be this way, but there it is.

This leads to a simple strategy to test for a GPIO enable, and to find which one it is.

First, we find the southbridge, which in this case is an ICH5.
The GPIO programming on this part is little changed from earlier parts.

Then we find the pci function which has the GPIOs. It's usually the LPC bridge.

So for ICH5:

00:1f.0 ISA bridge: Intel Corporation 82801EB/ER (ICH5/ICH5R) LPC Interface Bridge (rev 02)

it's that one.

So, to make it easy, rather than look at the BAR for the GPIO, just ``cat /proc/ioports`` and find this::

    0880-08bf : 0000:00:1f.0
    0880-08bf : pnp 00:06

OK, we are about ready to go. The base address of the GPIOs is 0x880.
If you're paranoid confirm it with setpci::

    [root@tn4 ~]# setpci -s 0:1f.0 58.l
    00000881
    [root@tn4 ~]#

You need to look up the IO space mappings of SOME of the registers,
but for this simple program, not ALL. In fact all we're going to do is
read in the GPIO data level register, complement it, write it out,
then run flashrom to see if it works. But, you ask:

* what if you read inputs and write them out nothing, so don't worry. They're inputs.
* you change GPIO pins that do some other thing well, it gets harder in that case.
  For instance, some laptops use a GPIO pin to enable DRAM power.
  Don't worry, you'll find out if they do.

In that case, you'll have to do 32 boot/test cycles in the worst case,
instead of the five we do here. It actually can be instructive on a laptop
to change output GPIO levels and see what happens, so this is a fun test to do anyway.

First, though, do this: ``flashrom -r factory.img``

Then ``emacs factory.img`` , (Go into OVRWRT mode!) and look for a string like this::

    F2 = Setup

I changed it to::

    F2 = FIXup

I may have used some other F-based words, as time went on, but that's another story.

You want to make sure that if you really do rewrite it that it is easy to tell!
With this change, as soon as the BIOS splash screen comes up, you will know.

OK, some code: Just set a few things up we think we'll need.::

    #include <stdio.h>
    #include <sys/io.h>

    #define LVL 0xc

LVL is the level register for the GPIO. Now let's go to work.::

	int main(int argc, char *argv[])
	{
	       unsigned long gpioport = 0x880;
	       unsigned long gpioval;
	       iopl(3);
	       /* first simple test: read in all GPIOs, complement them,
		* output them, see if flashrom works */
	       gpioval = inl(gpioport + LVL);
	       printf("GPIO is 0x%x (default 0x1f1f0000)\n", gpioval);
	       /* invert */
	       gpioval = ~gpioval;
	       printf("GPIO will be set to 0x%x \n", gpioval);
	       outl(gpioval, gpioport + LVL);
	       gpioval = inl(gpioport + LVL);
	       printf("GPIO is 0x%x \n", gpioval);
	}

OK, call this program 'one'. At this point, you want to try a flashrom run.
As it happens this works and is sufficient to allow us to use flashrom!

How to finish the task? It's actually a fairly simple newtonian search.

First try ``gpioval ^= 0xffff0000;``

If that works, then try 0xff000000, etc. etc. Even if you get it wrong,
which I did, it still doesn't take long to find it.

Warning, though: each time you try, be sure to change the FIXup string in the rom image,
to be very very sure that you really did rewrite it. You need to be careful about this step.

Anyway, hope that is a little useful. It really is a very simple process to find a GPIO enable.
That's one reason that vendors are going to make this much, much harder on future systems.
GPIO enables are not a security feature, in spite of what you may have heard;
they are really accident protection in case some piece of software goes insane
and starts writing to random memory locations.

Reverse Engineering your BIOS
-----------------------------

Reverse engineering the BIOS is more straight-forward than just poking around all GPIO pins,
but in itself a quite advanced topic, so it is explained in detail in :doc:`reverse_engineering`.
It basically is about analyzing what the BIOS or the vendor tool does to update the flash rom.

How to add the board enable to flashrom
=======================================

First, find out whether the board enable method you found out is already used for another mainboard.
In the case that it is just raising or lowering one GPIO pin, it could be quite possible that this is the case.
As we don't like code duplication, reuse that function. If it is named something like "board_vendor_model"
but really just changes a single pin (board_epox_ep_bx3 is an example in r803), rename it to what it does,
which would be intel_piix4_gpo22_raise in this case. Add a comment to the function that it also works on your board.

If you don't find a function that does what you want, write it yourself,
also trying to reuse codes for parts of what you have to do.
Add a comment mentioning the board name and the involved chips in this case too.

After having written an enable function, you just need to add it to the board enable table
(board_pciids_enables). This table maps PCI IDs found on the board to the correct enabling functions.
Be sure to write a good match (which mostly means that you have to choose onboard chips
that have vendor-assigned subsystem IDs that are unique for each board that vendor produced).
If a unique match is not possible at all, provide vendor and model name (short form, usually lower case only)
as "coreboot IDs" for use with the "-m" option, which means that the board won't be autodetected
but can be selected by hand. The board name and vendor name following the coreboot ID strings are freeform
and should disply vendor and model name in their preferred form.
