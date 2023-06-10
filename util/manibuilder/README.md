Manibuilder
===========

Manibuilder is a set of Dockerfiles for manic build testing, held
together by some make-foo. Most of the Dockerfiles make use of
*multiarch* images. This way we can test building on many platforms
supported by *Qemu*. The idea is to test in environments as close
as possible to those of potential users, i.e. no cross-compiling
(with some exceptions).

Make targets
------------

For each supported target OS/version/architecture exists a *tag*
target, for instance `alpine:amd64-v3.7`. These targets will
automatically check for existence of their respective *Docker*
images (sub target <tag>-check-build), and build them if necessary
(<tag>-build). Finally, flashrom revision `$(TEST_REVISION)` is
fetched and build tested.

The results will be kept by *Docker* as stopped containers and
can be accessed with the <tag>-shell target.

There are some additional targets that form sets of the *tag*
targets:

* default:  runs a preselected subset of all supported tags.
* native:   runs all tags native to the host architecture.
* all:      runs all supported tags.

For each of these show-<set> lists the included *tags*.

For preparation of *Qemu* for the *multiarch* images, there is the
`register` target. It has to be run once per boot, though as it
uses a privileged *Docker* container, that is kept as a manual step.

Usage example
-------------

The most common use case may be testing the current upstream
*main* branch which is the default for `$(TEST_REVISION)`.
You'll need roughly 20GiB for the *Docker* images. Might look
like this:

    $ # have to register Qemu first:
    $ make register
    [...]
    $ # run the default target:
    $ make -j4
    debian-debootstrap:mips-stretch: 2
    debian-debootstrap:mips-sid: 2
    debian-debootstrap:mips-buster: 2
    ubuntu-debootstrap:powerpc-xenial: 2
    djgpp:6.1.0: 2

For each *tag* that returns with a non-zero exit code, the *tag*
and actual exit code is printed. An exit code of `2` is most likely
as that is what *make* returns on failure. Other exit codes might
hint towards a problem in the setup. Failing *tags* can then be
investigated individually with the <tag>-shell target, e.g.:

    $ make debian-debootstrap:mips-sid-shell
    [...]
    mani@63536fc102a5:~/flashrom$ make
    [...]
    cc -MMD -Os -Wall -Wshadow -Werror -I/usr/include/libusb-1.0  -D'CONFIG_DEFAULT_PROGRAMMER=PROGRAMMER_INVALID' -D'CONFIG_DEFAULT_PROGRAMMER_ARGS="''"' -D'CONFIG_SERPROG=1' -D'CONFIG_PONY_SPI=1' -D'CONFIG_BITBANG_SPI=1' -D'CONFIG_GFXNVIDIA=1' -D'CONFIG_SATASII=1' -D'CONFIG_ATAVIA=1' -D'CONFIG_IT8212=1' -D'CONFIG_FT2232_SPI=1' -D'CONFIG_USBBLASTER_SPI=1' -D'CONFIG_PICKIT2_SPI=1' -D'HAVE_FT232H=1'  -D'CONFIG_DUMMY=1' -D'CONFIG_DRKAISER=1' -D'CONFIG_NICINTEL=1' -D'CONFIG_NICINTEL_SPI=1' -D'CONFIG_NICINTEL_EEPROM=1' -D'CONFIG_OGP_SPI=1' -D'CONFIG_BUSPIRATE_SPI=1' -D'CONFIG_DEDIPROG=1' -D'CONFIG_DEVELOPERBOX_SPI=1' -D'CONFIG_LINUX_MTD=1' -D'CONFIG_LINUX_SPI=1' -D'CONFIG_CH341A_SPI=1' -D'CONFIG_DIGILENT_SPI=1' -D'NEED_PCI=1'  -D'NEED_RAW_ACCESS=1' -D'NEED_LIBUSB0=1' -D'NEED_LIBUSB1=1' -D'HAVE_UTSNAME=1' -D'HAVE_CLOCK_GETTIME=1' -D'FLASHROM_VERSION="p1.0-141-g9cecc7e"' -o libflashrom.o -c libflashrom.c
    libflashrom.c:386:12: error: 'flashrom_layout_parse_fmap' defined but not used [-Werror=unused-function]
     static int flashrom_layout_parse_fmap(struct flashrom_layout **layout,
            ^~~~~~~~~~~~~~~~~~~~~~~~~~
    cc1: all warnings being treated as errors
    make: *** [Makefile:1075: libflashrom.o] Error 1
    $ # uh-huh, might be a problem with big-endian #if foo
