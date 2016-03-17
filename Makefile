#
# This file is part of the flashrom project.
#
# Copyright (C) 2005 coresystems GmbH <stepan@coresystems.de>
# Copyright (C) 2009,2010,2012 Carl-Daniel Hailfinger
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

PROGRAM = flashrom

###############################################################################
# Defaults for the toolchain.

# If you want to cross-compile, just run e.g.
# make CC=i586-pc-msdosdjgpp-gcc
# You may have to specify STRIP/AR/RANLIB as well.
#
# Note for anyone editing this Makefile: gnumake will happily ignore any
# changes in this Makefile to variables set on the command line.
STRIP   ?= strip
INSTALL = install
DIFF    = diff
PREFIX  ?= /usr/local
MANDIR  ?= $(PREFIX)/share/man
CFLAGS  ?= -Os -Wall -Wshadow
EXPORTDIR ?= .
RANLIB  ?= ranlib
PKG_CONFIG ?= pkg-config
BUILD_DETAILS_FILE ?= build_details.txt

# The following parameter changes the default programmer that will be used if there is no -p/--programmer
# argument given when running flashrom. The predefined setting does not enable any default so that every
# user has to declare the programmer he wants to use on every run. The rationale for this to be not set
# (to e.g. the internal programmer) is that forgetting to specify this when working with another programmer
# easily puts the system attached to the default programmer at risk (e.g. you want to flash coreboot to another
# system attached to an external programmer while the default programmer is set to the internal programmer, and
# you forget to use the -p parameter. This would (try to) overwrite the existing firmware of the computer
# running flashrom). Please do not enable this without thinking about the possible consequences. Possible
# values are those specified in enum programmer in programmer.h (which depend on other CONFIG_* options
# evaluated below, namely those that enable/disable the various programmers).
# Compilation will fail for unspecified values.
CONFIG_DEFAULT_PROGRAMMER ?= PROGRAMMER_INVALID
# The following adds a default parameter for the default programmer set above (only).
CONFIG_DEFAULT_PROGRAMMER_ARGS ?= ''
# Example: compiling with
#   make CONFIG_DEFAULT_PROGRAMMER=PROGRAMMER_SERPROG CONFIG_DEFAULT_PROGRAMMER_ARGS="dev=/dev/ttyUSB0:1500000"
# would make executing './flashrom' (almost) equivialent to './flashrom -p serprog:dev=/dev/ttyUSB0:1500000'.

# If your compiler spits out excessive warnings, run make WARNERROR=no
# You shouldn't have to change this flag.
WARNERROR ?= yes

ifeq ($(WARNERROR), yes)
CFLAGS += -Werror
endif

ifdef LIBS_BASE
PKG_CONFIG_LIBDIR ?= $(LIBS_BASE)/lib/pkgconfig
override CPPFLAGS += -I$(LIBS_BASE)/include
override LDFLAGS += -L$(LIBS_BASE)/lib -Wl,-rpath -Wl,$(LIBS_BASE)/lib
endif

ifeq ($(CONFIG_STATIC),yes)
override PKG_CONFIG += --static
override LDFLAGS += -static
endif

# Set LC_ALL=C to minimize influences of the locale.
# However, this won't work for the majority of relevant commands because they use the $(shell) function and
# GNU make does not relay variables exported within the makefile to their evironment.
LC_ALL=C
export LC_ALL

dummy_for_make_3_80:=$(shell printf "Build started on %s\n\n" "$$(date)" >$(BUILD_DETAILS_FILE))

# Provide an easy way to execute a command, print its output to stdout and capture any error message on stderr
# in the build details file together with the original stdout output.
debug_shell = $(shell export LC_ALL=C ; { echo 'exec: export LC_ALL=C ; { $(1) ; }' >&2;  { $(1) ; } | tee -a $(BUILD_DETAILS_FILE) ; echo >&2 ; } 2>>$(BUILD_DETAILS_FILE))

###############################################################################
# General OS-specific settings.
# 1. Prepare for later by gathering information about host and target OS
# 2. Set compiler flags and parameters according to OSes
# 3. Likewise verify user-supplied CONFIG_* variables.

# HOST_OS is only used to work around local toolchain issues.
HOST_OS ?= $(shell uname)
ifeq ($(findstring MINGW, $(HOST_OS)), MINGW)
# Explicitly set CC = gcc on MinGW, otherwise: "cc: command not found".
CC = gcc
endif

ifneq ($(HOST_OS), SunOS)
STRIP_ARGS = -s
endif

# Determine the destination OS.
# IMPORTANT: The following line must be placed before TARGET_OS is ever used
# (of course), but should come after any lines setting CC because the line
# below uses CC itself.
override TARGET_OS := $(strip $(call debug_shell,$(CC) $(CPPFLAGS) -E os.h 2>/dev/null | grep -v '^\#' | grep '"' | cut -f 2 -d'"'))

ifeq ($(TARGET_OS), Darwin)
override CPPFLAGS += -I/opt/local/include -I/usr/local/include
override LDFLAGS += -L/opt/local/lib -L/usr/local/lib
endif

ifeq ($(TARGET_OS), FreeBSD)
override CPPFLAGS += -I/usr/local/include
override LDFLAGS += -L/usr/local/lib
endif

ifeq ($(TARGET_OS), OpenBSD)
override CPPFLAGS += -I/usr/local/include
override LDFLAGS += -L/usr/local/lib
endif

ifeq ($(TARGET_OS), NetBSD)
override CPPFLAGS += -I/usr/pkg/include
override LDFLAGS += -L/usr/pkg/lib
endif

ifeq ($(TARGET_OS), DragonFlyBSD)
override CPPFLAGS += -I/usr/local/include
override LDFLAGS += -L/usr/local/lib
endif

ifeq ($(TARGET_OS), DOS)
EXEC_SUFFIX := .exe
# DJGPP has odd uint*_t definitions which cause lots of format string warnings.
override CFLAGS += -Wno-format
LIBS += -lgetopt
# Bus Pirate, Serprog and PonyProg are not supported under DOS (missing serial support).
ifeq ($(CONFIG_BUSPIRATE_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_BUSPIRATE_SPI=yes
else
override CONFIG_BUSPIRATE_SPI = no
endif
ifeq ($(CONFIG_SERPROG), yes)
UNSUPPORTED_FEATURES += CONFIG_SERPROG=yes
else
override CONFIG_SERPROG = no
endif
ifeq ($(CONFIG_PONY_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_PONY_SPI=yes
else
override CONFIG_PONY_SPI = no
endif
# Dediprog, Developerbox, USB-Blaster, PICkit2, CH341A and FT2232 are not supported under DOS (missing USB support).
ifeq ($(CONFIG_DEDIPROG), yes)
UNSUPPORTED_FEATURES += CONFIG_DEDIPROG=yes
else
override CONFIG_DEDIPROG = no
endif
ifeq ($(CONFIG_DEVELOPERBOX_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_DEVELOPERBOX_SPI=yes
else
override CONFIG_DEVELOPERBOX_SPI = no
endif
ifeq ($(CONFIG_FT2232_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_FT2232_SPI=yes
else
override CONFIG_FT2232_SPI = no
endif
ifeq ($(CONFIG_USBBLASTER_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_USBBLASTER_SPI=yes
else
override CONFIG_USBBLASTER_SPI = no
endif
ifeq ($(CONFIG_PICKIT2_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_PICKIT2_SPI=yes
else
override CONFIG_PICKIT2_SPI = no
endif
ifeq ($(CONFIG_CH341A_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_CH341A_SPI=yes
else
override CONFIG_CH341A_SPI = no
endif
endif

# FIXME: Should we check for Cygwin/MSVC as well?
ifeq ($(TARGET_OS), MinGW)
EXEC_SUFFIX := .exe
# MinGW doesn't have the ffs() function, but we can use gcc's __builtin_ffs().
FLASHROM_CFLAGS += -Dffs=__builtin_ffs
# Some functions provided by Microsoft do not work as described in C99 specifications. This macro fixes that
# for MinGW. See http://sourceforge.net/p/mingw-w64/wiki2/printf%20and%20scanf%20family/ */
FLASHROM_CFLAGS += -D__USE_MINGW_ANSI_STDIO=1
# For now we disable all PCI-based programmers on Windows/MinGW (no libpci).
ifeq ($(CONFIG_INTERNAL), yes)
UNSUPPORTED_FEATURES += CONFIG_INTERNAL=yes
else
override CONFIG_INTERNAL = no
endif
ifeq ($(CONFIG_RAYER_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_RAYER_SPI=yes
else
override CONFIG_RAYER_SPI = no
endif
ifeq ($(CONFIG_NIC3COM), yes)
UNSUPPORTED_FEATURES += CONFIG_NIC3COM=yes
else
override CONFIG_NIC3COM = no
endif
ifeq ($(CONFIG_GFXNVIDIA), yes)
UNSUPPORTED_FEATURES += CONFIG_GFXNVIDIA=yes
else
override CONFIG_GFXNVIDIA = no
endif
ifeq ($(CONFIG_SATASII), yes)
UNSUPPORTED_FEATURES += CONFIG_SATASII=yes
else
override CONFIG_SATASII = no
endif
ifeq ($(CONFIG_ATAHPT), yes)
UNSUPPORTED_FEATURES += CONFIG_ATAHPT=yes
else
override CONFIG_ATAHPT = no
endif
ifeq ($(CONFIG_ATAVIA), yes)
UNSUPPORTED_FEATURES += CONFIG_ATAVIA=yes
else
override CONFIG_ATAVIA = no
endif
ifeq ($(CONFIG_ATAPROMISE), yes)
UNSUPPORTED_FEATURES += CONFIG_ATAPROMISE=yes
else
override CONFIG_ATAPROMISE = no
endif
ifeq ($(CONFIG_IT8212), yes)
UNSUPPORTED_FEATURES += CONFIG_IT8212=yes
else
override CONFIG_IT8212 = no
endif
ifeq ($(CONFIG_DRKAISER), yes)
UNSUPPORTED_FEATURES += CONFIG_DRKAISER=yes
else
override CONFIG_DRKAISER = no
endif
ifeq ($(CONFIG_NICREALTEK), yes)
UNSUPPORTED_FEATURES += CONFIG_NICREALTEK=yes
else
override CONFIG_NICREALTEK = no
endif
ifeq ($(CONFIG_NICNATSEMI), yes)
UNSUPPORTED_FEATURES += CONFIG_NICNATSEMI=yes
else
override CONFIG_NICNATSEMI = no
endif
ifeq ($(CONFIG_NICINTEL), yes)
UNSUPPORTED_FEATURES += CONFIG_NICINTEL=yes
else
override CONFIG_NICINTEL = no
endif
ifeq ($(CONFIG_NICINTEL_EEPROM), yes)
UNSUPPORTED_FEATURES += CONFIG_NICINTEL_EEPROM=yes
else
override CONFIG_NICINTEL_EEPROM = no
endif
ifeq ($(CONFIG_NICINTEL_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_NICINTEL_SPI=yes
else
override CONFIG_NICINTEL_SPI = no
endif
ifeq ($(CONFIG_OGP_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_OGP_SPI=yes
else
override CONFIG_OGP_SPI = no
endif
ifeq ($(CONFIG_SATAMV), yes)
UNSUPPORTED_FEATURES += CONFIG_SATAMV=yes
else
override CONFIG_SATAMV = no
endif
endif

ifeq ($(TARGET_OS), libpayload)
ifeq ($(MAKECMDGOALS),)
.DEFAULT_GOAL := libflashrom.a
$(info Setting default goal to libflashrom.a)
endif
FLASHROM_CFLAGS += -DSTANDALONE
ifeq ($(CONFIG_DUMMY), yes)
UNSUPPORTED_FEATURES += CONFIG_DUMMY=yes
else
override CONFIG_DUMMY = no
endif
# libpayload does not provide the romsize field in struct pci_dev that the atapromise code requires.
ifeq ($(CONFIG_ATAPROMISE), yes)
UNSUPPORTED_FEATURES += CONFIG_ATAPROMISE=yes
else
override CONFIG_ATAPROMISE = no
endif
# Bus Pirate, Serprog and PonyProg are not supported with libpayload (missing serial support).
ifeq ($(CONFIG_BUSPIRATE_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_BUSPIRATE_SPI=yes
else
override CONFIG_BUSPIRATE_SPI = no
endif
ifeq ($(CONFIG_SERPROG), yes)
UNSUPPORTED_FEATURES += CONFIG_SERPROG=yes
else
override CONFIG_SERPROG = no
endif
ifeq ($(CONFIG_PONY_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_PONY_SPI=yes
else
override CONFIG_PONY_SPI = no
endif
# Dediprog, Developerbox, USB-Blaster, PICkit2, CH341A and FT2232 are not supported with libpayload (missing libusb support).
ifeq ($(CONFIG_DEDIPROG), yes)
UNSUPPORTED_FEATURES += CONFIG_DEDIPROG=yes
else
override CONFIG_DEDIPROG = no
endif
ifeq ($(CONFIG_DEVELOPERBOX_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_DEVELOPERBOX_SPI=yes
else
override CONFIG_DEVELOPERBOX_SPI = no
endif
ifeq ($(CONFIG_FT2232_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_FT2232_SPI=yes
else
override CONFIG_FT2232_SPI = no
endif
ifeq ($(CONFIG_USBBLASTER_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_USBBLASTER_SPI=yes
else
override CONFIG_USBBLASTER_SPI = no
endif
ifeq ($(CONFIG_PICKIT2_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_PICKIT2_SPI=yes
else
override CONFIG_PICKIT2_SPI = no
endif
ifeq ($(CONFIG_CH341A_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_CH341A_SPI=yes
else
override CONFIG_CH341A_SPI = no
endif
endif

ifneq ($(TARGET_OS), Linux)
# Android is handled internally as separate OS, but it supports CONFIG_LINUX_SPI and CONFIG_MSTARDDC_SPI
ifneq ($(TARGET_OS), Android)
ifeq ($(CONFIG_LINUX_MTD), yes)
UNSUPPORTED_FEATURES += CONFIG_LINUX_MTD=yes
else
override CONFIG_LINUX_MTD = no
endif
ifeq ($(CONFIG_LINUX_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_LINUX_SPI=yes
else
override CONFIG_LINUX_SPI = no
endif
ifeq ($(CONFIG_MSTARDDC_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_MSTARDDC_SPI=yes
else
override CONFIG_MSTARDDC_SPI = no
endif
endif
endif

ifeq ($(TARGET_OS), Android)
# Android on x86 (currently) does not provide raw PCI port I/O operations
ifeq ($(CONFIG_RAYER_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_RAYER_SPI=yes
else
override CONFIG_RAYER_SPI = no
endif
endif

###############################################################################
# General architecture-specific settings.
# Like above for the OS, below we verify user-supplied options depending on the target architecture.

# Determine the destination processor architecture.
# IMPORTANT: The following line must be placed before ARCH is ever used
# (of course), but should come after any lines setting CC because the line
# below uses CC itself.
override ARCH := $(strip $(call debug_shell,$(CC) $(CPPFLAGS) -E archtest.c 2>/dev/null | grep -v '^\#' | grep '"' | cut -f 2 -d'"'))
override ENDIAN := $(strip $(call debug_shell,$(CC) $(CPPFLAGS) -E endiantest.c 2>/dev/null | grep -v '^\#'))

# Disable the internal programmer on unsupported architectures (everything but x86 and mipsel)
ifneq ($(ARCH)-little, $(filter $(ARCH),x86 mips)-$(ENDIAN))
ifeq ($(CONFIG_INTERNAL), yes)
UNSUPPORTED_FEATURES += CONFIG_INTERNAL=yes
else
override CONFIG_INTERNAL = no
endif
endif

# PCI port I/O support is unimplemented on PPC/MIPS/SPARC and unavailable on ARM.
# Right now this means the drivers below only work on x86.
ifneq ($(ARCH), x86)
ifeq ($(CONFIG_NIC3COM), yes)
UNSUPPORTED_FEATURES += CONFIG_NIC3COM=yes
else
override CONFIG_NIC3COM = no
endif
ifeq ($(CONFIG_NICREALTEK), yes)
UNSUPPORTED_FEATURES += CONFIG_NICREALTEK=yes
else
override CONFIG_NICREALTEK = no
endif
ifeq ($(CONFIG_NICNATSEMI), yes)
UNSUPPORTED_FEATURES += CONFIG_NICNATSEMI=yes
else
override CONFIG_NICNATSEMI = no
endif
ifeq ($(CONFIG_RAYER_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_RAYER_SPI=yes
else
override CONFIG_RAYER_SPI = no
endif
ifeq ($(CONFIG_ATAHPT), yes)
UNSUPPORTED_FEATURES += CONFIG_ATAHPT=yes
else
override CONFIG_ATAHPT = no
endif
ifeq ($(CONFIG_ATAPROMISE), yes)
UNSUPPORTED_FEATURES += CONFIG_ATAPROMISE=yes
else
override CONFIG_ATAPROMISE = no
endif
ifeq ($(CONFIG_SATAMV), yes)
UNSUPPORTED_FEATURES += CONFIG_SATAMV=yes
else
override CONFIG_SATAMV = no
endif
endif

# Disable all drivers needing raw access (memory, PCI, port I/O) on
# architectures with unknown raw access properties.
# Right now those architectures are alpha hppa m68k sh s390
ifneq ($(ARCH),$(filter $(ARCH),x86 mips ppc arm sparc))
ifeq ($(CONFIG_RAYER_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_RAYER_SPI=yes
else
override CONFIG_RAYER_SPI = no
endif
ifeq ($(CONFIG_NIC3COM), yes)
UNSUPPORTED_FEATURES += CONFIG_NIC3COM=yes
else
override CONFIG_NIC3COM = no
endif
ifeq ($(CONFIG_GFXNVIDIA), yes)
UNSUPPORTED_FEATURES += CONFIG_GFXNVIDIA=yes
else
override CONFIG_GFXNVIDIA = no
endif
ifeq ($(CONFIG_SATASII), yes)
UNSUPPORTED_FEATURES += CONFIG_SATASII=yes
else
override CONFIG_SATASII = no
endif
ifeq ($(CONFIG_ATAHPT), yes)
UNSUPPORTED_FEATURES += CONFIG_ATAHPT=yes
else
override CONFIG_ATAHPT = no
endif
ifeq ($(CONFIG_ATAVIA), yes)
UNSUPPORTED_FEATURES += CONFIG_ATAVIA=yes
else
override CONFIG_ATAVIA = no
endif
ifeq ($(CONFIG_ATAPROMISE), yes)
UNSUPPORTED_FEATURES += CONFIG_ATAPROMISE=yes
else
override CONFIG_ATAPROMISE = no
endif
ifeq ($(CONFIG_DRKAISER), yes)
UNSUPPORTED_FEATURES += CONFIG_DRKAISER=yes
else
override CONFIG_DRKAISER = no
endif
ifeq ($(CONFIG_NICREALTEK), yes)
UNSUPPORTED_FEATURES += CONFIG_NICREALTEK=yes
else
override CONFIG_NICREALTEK = no
endif
ifeq ($(CONFIG_NICNATSEMI), yes)
UNSUPPORTED_FEATURES += CONFIG_NICNATSEMI=yes
else
override CONFIG_NICNATSEMI = no
endif
ifeq ($(CONFIG_NICINTEL), yes)
UNSUPPORTED_FEATURES += CONFIG_NICINTEL=yes
else
override CONFIG_NICINTEL = no
endif
ifeq ($(CONFIG_NICINTEL_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_NICINTEL_SPI=yes
else
override CONFIG_NICINTEL_SPI = no
endif
ifeq ($(CONFIG_NICINTEL_EEPROM), yes)
UNSUPPORTED_FEATURES += CONFIG_NICINTEL_EEPROM=yes
else
override CONFIG_NICINTEL_EEPROM = no
endif
ifeq ($(CONFIG_OGP_SPI), yes)
UNSUPPORTED_FEATURES += CONFIG_OGP_SPI=yes
else
override CONFIG_OGP_SPI = no
endif
ifeq ($(CONFIG_SATAMV), yes)
UNSUPPORTED_FEATURES += CONFIG_SATAMV=yes
else
override CONFIG_SATAMV = no
endif
ifeq ($(CONFIG_IT8212), yes)
UNSUPPORTED_FEATURES += CONFIG_IT8212=yes
else
override CONFIG_IT8212 = no
endif
endif

###############################################################################
# Flash chip drivers and bus support infrastructure.

CHIP_OBJS = jedec.o stm50.o w39.o w29ee011.o \
	sst28sf040.o 82802ab.o \
	sst49lfxxxc.o sst_fwhub.o edi.o flashchips.o spi.o spi25.o spi25_statusreg.o \
	opaque.o sfdp.o en29lv640b.o at45db.o

###############################################################################
# Library code.

LIB_OBJS = libflashrom.o layout.o flashrom.o udelay.o programmer.o helpers.o ich_descriptors.o fmap.o

###############################################################################
# Frontend related stuff.

CLI_OBJS = cli_classic.o cli_output.o cli_common.o print.o

# versioninfo.inc stores metadata required to build a packaged flashrom. It is generated by the export rule and
# imported below. If versioninfo.inc is not found and the variables are not defined by the user, the info will
# be obtained using util/getrevision.sh, which is the common case during development.
-include versioninfo.inc
VERSION ?= $(shell ./util/getrevision.sh --revision)
MAN_DATE ?= $(shell ./util/getrevision.sh --date $(PROGRAM).8.tmpl 2>/dev/null)

SCMDEF := -D'FLASHROM_VERSION="$(VERSION)"'

# No spaces in release names unless set explicitly
RELEASENAME ?= $(shell echo "$(VERSION)" | sed -e 's/ /_/')

# Inform user of the version string
$(info Replacing all version templates with $(VERSION).)

# If a VCS is found then try to install hooks.
$(shell ./util/getrevision.sh -c 2>/dev/null && ./util/git-hooks/install.sh)

###############################################################################
# Default settings of CONFIG_* variables.

# Always enable internal/onboard support for now.
CONFIG_INTERNAL ?= yes

# Always enable serprog for now.
CONFIG_SERPROG ?= yes

# RayeR SPIPGM hardware support
CONFIG_RAYER_SPI ?= yes

# PonyProg2000 SPI hardware support
CONFIG_PONY_SPI ?= yes

# Always enable 3Com NICs for now.
CONFIG_NIC3COM ?= yes

# Enable NVIDIA graphics cards. Note: write and erase do not work properly.
CONFIG_GFXNVIDIA ?= yes

# Always enable SiI SATA controllers for now.
CONFIG_SATASII ?= yes

# Highpoint (HPT) ATA/RAID controller support.
# IMPORTANT: This code is not yet working!
CONFIG_ATAHPT ?= no

# VIA VT6421A LPC memory support
CONFIG_ATAVIA ?= yes

# Promise ATA controller support.
CONFIG_ATAPROMISE ?= no

# Always enable FT2232 SPI dongles for now.
CONFIG_FT2232_SPI ?= yes

# Always enable Altera USB-Blaster dongles for now.
CONFIG_USBBLASTER_SPI ?= yes

# MSTAR DDC support needs more tests/reviews/cleanups.
CONFIG_MSTARDDC_SPI ?= no

# Always enable PICkit2 SPI dongles for now.
CONFIG_PICKIT2_SPI ?= yes

# Always enable dummy tracing for now.
CONFIG_DUMMY ?= yes

# Always enable Dr. Kaiser for now.
CONFIG_DRKAISER ?= yes

# Always enable Realtek NICs for now.
CONFIG_NICREALTEK ?= yes

# Disable National Semiconductor NICs until support is complete and tested.
CONFIG_NICNATSEMI ?= no

# Always enable Intel NICs for now.
CONFIG_NICINTEL ?= yes

# Always enable SPI on Intel NICs for now.
CONFIG_NICINTEL_SPI ?= yes

# Always enable EEPROM on Intel NICs for now.
CONFIG_NICINTEL_EEPROM ?= yes

# Always enable SPI on OGP cards for now.
CONFIG_OGP_SPI ?= yes

# Always enable Bus Pirate SPI for now.
CONFIG_BUSPIRATE_SPI ?= yes

# Always enable Dediprog SF100 for now.
CONFIG_DEDIPROG ?= yes

# Always enable Developerbox emergency recovery for now.
CONFIG_DEVELOPERBOX_SPI ?= yes

# Always enable Marvell SATA controllers for now.
CONFIG_SATAMV ?= yes

# Enable Linux spidev and MTD interfaces by default. We disable them on non-Linux targets.
CONFIG_LINUX_MTD ?= yes
CONFIG_LINUX_SPI ?= yes

# Always enable ITE IT8212F PATA controllers for now.
CONFIG_IT8212 ?= yes

# Winchiphead CH341A
CONFIG_CH341A_SPI ?= yes

# Digilent Development board JTAG
CONFIG_DIGILENT_SPI ?= yes

# Disable J-Link for now.
CONFIG_JLINK_SPI ?= no

# Disable wiki printing by default. It is only useful if you have wiki access.
CONFIG_PRINT_WIKI ?= no

# Disable all features if CONFIG_NOTHING=yes is given unless CONFIG_EVERYTHING was also set
ifeq ($(CONFIG_NOTHING), yes)
  ifeq ($(CONFIG_EVERYTHING), yes)
    $(error Setting CONFIG_NOTHING=yes and CONFIG_EVERYTHING=yes does not make sense)
  endif
  $(foreach var, $(filter CONFIG_%, $(.VARIABLES)),\
    $(if $(filter yes, $($(var))),\
      $(eval $(var)=no)))
endif

# Enable all features if CONFIG_EVERYTHING=yes is given
ifeq ($(CONFIG_EVERYTHING), yes)
$(foreach var, $(filter CONFIG_%, $(.VARIABLES)),\
	$(if $(filter no, $($(var))),\
		$(eval $(var)=yes)))
endif

# Disable feature groups
ifeq ($(CONFIG_ENABLE_LIBUSB0_PROGRAMMERS), no)
override CONFIG_PICKIT2_SPI = no
endif
ifeq ($(CONFIG_ENABLE_LIBUSB1_PROGRAMMERS), no)
override CONFIG_CH341A_SPI = no
override CONFIG_DEDIPROG = no
override CONFIG_DIGILENT_SPI = no
override CONFIG_DEVELOPERBOX_SPI = no
endif
ifeq ($(CONFIG_ENABLE_LIBPCI_PROGRAMMERS), no)
override CONFIG_INTERNAL = no
override CONFIG_NIC3COM = no
override CONFIG_GFXNVIDIA = no
override CONFIG_SATASII = no
override CONFIG_ATAHPT = no
override CONFIG_ATAVIA = no
override CONFIG_ATAPROMISE = no
override CONFIG_IT8212 = no
override CONFIG_DRKAISER = no
override CONFIG_NICREALTEK = no
override CONFIG_NICNATSEMI = no
override CONFIG_NICINTEL = no
override CONFIG_NICINTEL_SPI = no
override CONFIG_NICINTEL_EEPROM = no
override CONFIG_OGP_SPI = no
override CONFIG_SATAMV = no
endif

# Bitbanging SPI infrastructure, default off unless needed.
ifeq ($(CONFIG_RAYER_SPI), yes)
override CONFIG_BITBANG_SPI = yes
else
ifeq ($(CONFIG_PONY_SPI), yes)
override CONFIG_BITBANG_SPI = yes
else
ifeq ($(CONFIG_INTERNAL), yes)
override CONFIG_BITBANG_SPI = yes
else
ifeq ($(CONFIG_NICINTEL_SPI), yes)
override CONFIG_BITBANG_SPI = yes
else
ifeq ($(CONFIG_OGP_SPI), yes)
override CONFIG_BITBANG_SPI = yes
else
CONFIG_BITBANG_SPI ?= no
endif
endif
endif
endif
endif

###############################################################################
# Handle CONFIG_* variables that depend on others set (and verified) above.

# The external DMI decoder (dmidecode) does not work in libpayload. Bail out if the internal one got disabled.
ifeq ($(TARGET_OS), libpayload)
ifeq ($(CONFIG_INTERNAL), yes)
ifeq ($(CONFIG_INTERNAL_DMI), no)
UNSUPPORTED_FEATURES += CONFIG_INTERNAL_DMI=no
else
override CONFIG_INTERNAL_DMI = yes
endif
endif
endif

# Use internal DMI/SMBIOS decoder by default instead of relying on dmidecode.
CONFIG_INTERNAL_DMI ?= yes

###############################################################################
# Programmer drivers and programmer support infrastructure.
# Depending on the CONFIG_* variables set and verified above we set compiler flags and parameters below.

FEATURE_CFLAGS += -D'CONFIG_DEFAULT_PROGRAMMER=$(CONFIG_DEFAULT_PROGRAMMER)'
FEATURE_CFLAGS += -D'CONFIG_DEFAULT_PROGRAMMER_ARGS="$(CONFIG_DEFAULT_PROGRAMMER_ARGS)"'

ifeq ($(CONFIG_INTERNAL), yes)
FEATURE_CFLAGS += -D'CONFIG_INTERNAL=1'
PROGRAMMER_OBJS += processor_enable.o chipset_enable.o board_enable.o cbtable.o internal.o
ifeq ($(ARCH), x86)
PROGRAMMER_OBJS += it87spi.o it85spi.o sb600spi.o amd_imc.o wbsio_spi.o mcp6x_spi.o
PROGRAMMER_OBJS += ichspi.o dmi.o
ifeq ($(CONFIG_INTERNAL_DMI), yes)
FEATURE_CFLAGS += -D'CONFIG_INTERNAL_DMI=1'
endif
else
endif
NEED_LIBPCI += CONFIG_INTERNAL
endif

ifeq ($(CONFIG_SERPROG), yes)
FEATURE_CFLAGS += -D'CONFIG_SERPROG=1'
PROGRAMMER_OBJS += serprog.o
NEED_SERIAL += CONFIG_SERPROG
NEED_POSIX_SOCKETS += CONFIG_SERPROG
endif

ifeq ($(CONFIG_RAYER_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_RAYER_SPI=1'
PROGRAMMER_OBJS += rayer_spi.o
NEED_RAW_ACCESS += CONFIG_RAYER_SPI
endif

ifeq ($(CONFIG_PONY_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_PONY_SPI=1'
PROGRAMMER_OBJS += pony_spi.o
NEED_SERIAL += CONFIG_PONY_SPI
endif

ifeq ($(CONFIG_BITBANG_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_BITBANG_SPI=1'
PROGRAMMER_OBJS += bitbang_spi.o
endif

ifeq ($(CONFIG_NIC3COM), yes)
FEATURE_CFLAGS += -D'CONFIG_NIC3COM=1'
PROGRAMMER_OBJS += nic3com.o
NEED_LIBPCI += CONFIG_NIC3COM
endif

ifeq ($(CONFIG_GFXNVIDIA), yes)
FEATURE_CFLAGS += -D'CONFIG_GFXNVIDIA=1'
PROGRAMMER_OBJS += gfxnvidia.o
NEED_LIBPCI += CONFIG_GFXNVIDIA
endif

ifeq ($(CONFIG_SATASII), yes)
FEATURE_CFLAGS += -D'CONFIG_SATASII=1'
PROGRAMMER_OBJS += satasii.o
NEED_LIBPCI += CONFIG_SATASII
endif

ifeq ($(CONFIG_ATAHPT), yes)
FEATURE_CFLAGS += -D'CONFIG_ATAHPT=1'
PROGRAMMER_OBJS += atahpt.o
NEED_LIBPCI += CONFIG_ATAHPT
endif

ifeq ($(CONFIG_ATAVIA), yes)
FEATURE_CFLAGS += -D'CONFIG_ATAVIA=1'
PROGRAMMER_OBJS += atavia.o
NEED_LIBPCI += CONFIG_ATAVIA
endif

ifeq ($(CONFIG_ATAPROMISE), yes)
FEATURE_CFLAGS += -D'CONFIG_ATAPROMISE=1'
PROGRAMMER_OBJS += atapromise.o
NEED_LIBPCI += CONFIG_ATAPROMISE
endif

ifeq ($(CONFIG_IT8212), yes)
FEATURE_CFLAGS += -D'CONFIG_IT8212=1'
PROGRAMMER_OBJS += it8212.o
NEED_LIBPCI += CONFIG_IT8212
endif

ifeq ($(CONFIG_FT2232_SPI), yes)
# This is a totally ugly hack.
FEATURE_CFLAGS += $(call debug_shell,grep -q "FTDISUPPORT := yes" .features && printf "%s" "-D'CONFIG_FT2232_SPI=1'")
NEED_LIBFTDI += CONFIG_FT2232_SPI
PROGRAMMER_OBJS += ft2232_spi.o
endif

ifeq ($(CONFIG_USBBLASTER_SPI), yes)
# This is a totally ugly hack.
FEATURE_CFLAGS += $(call debug_shell,grep -q "FTDISUPPORT := yes" .features && printf "%s" "-D'CONFIG_USBBLASTER_SPI=1'")
NEED_LIBFTDI += CONFIG_USBBLASTER_SPI
PROGRAMMER_OBJS += usbblaster_spi.o
endif

ifeq ($(CONFIG_PICKIT2_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_PICKIT2_SPI=1'
PROGRAMMER_OBJS += pickit2_spi.o
NEED_LIBUSB0 += CONFIG_PICKIT2_SPI
endif

ifneq ($(NEED_LIBFTDI), )
FTDILIBS := $(call debug_shell,[ -n "$(PKG_CONFIG_LIBDIR)" ] && export PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)" ; $(PKG_CONFIG) --libs libftdi1 || $(PKG_CONFIG) --libs libftdi || printf "%s" "-lftdi -lusb")
FEATURE_CFLAGS += $(call debug_shell,grep -q "FT232H := yes" .features && printf "%s" "-D'HAVE_FT232H=1'")
FTDI_INCLUDES := $(call debug_shell,[ -n "$(PKG_CONFIG_LIBDIR)" ] && export PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)" ; $(PKG_CONFIG) --cflags-only-I libftdi1)
FEATURE_CFLAGS += $(FTDI_INCLUDES)
FEATURE_LIBS += $(call debug_shell,grep -q "FTDISUPPORT := yes" .features && printf "%s" "$(FTDILIBS)")
# We can't set NEED_LIBUSB0 here because that would transform libftdi auto-enabling
# into a hard requirement for libusb, defeating the purpose of auto-enabling.
endif

ifeq ($(CONFIG_DUMMY), yes)
FEATURE_CFLAGS += -D'CONFIG_DUMMY=1'
PROGRAMMER_OBJS += dummyflasher.o
endif

ifeq ($(CONFIG_DRKAISER), yes)
FEATURE_CFLAGS += -D'CONFIG_DRKAISER=1'
PROGRAMMER_OBJS += drkaiser.o
NEED_LIBPCI += CONFIG_DRKAISER
endif

ifeq ($(CONFIG_NICREALTEK), yes)
FEATURE_CFLAGS += -D'CONFIG_NICREALTEK=1'
PROGRAMMER_OBJS += nicrealtek.o
NEED_LIBPCI += CONFIG_NICREALTEK
endif

ifeq ($(CONFIG_NICNATSEMI), yes)
FEATURE_CFLAGS += -D'CONFIG_NICNATSEMI=1'
PROGRAMMER_OBJS += nicnatsemi.o
NEED_LIBPCI += CONFIG_NICNATSEMI
endif

ifeq ($(CONFIG_NICINTEL), yes)
FEATURE_CFLAGS += -D'CONFIG_NICINTEL=1'
PROGRAMMER_OBJS += nicintel.o
NEED_LIBPCI += CONFIG_NICINTEL
endif

ifeq ($(CONFIG_NICINTEL_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_NICINTEL_SPI=1'
PROGRAMMER_OBJS += nicintel_spi.o
NEED_LIBPCI += CONFIG_NICINTEL_SPI
endif

ifeq ($(CONFIG_NICINTEL_EEPROM), yes)
FEATURE_CFLAGS += -D'CONFIG_NICINTEL_EEPROM=1'
PROGRAMMER_OBJS += nicintel_eeprom.o
NEED_LIBPCI += CONFIG_NICINTEL_EEPROM
endif

ifeq ($(CONFIG_OGP_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_OGP_SPI=1'
PROGRAMMER_OBJS += ogp_spi.o
NEED_LIBPCI += CONFIG_OGP_SPI
endif

ifeq ($(CONFIG_BUSPIRATE_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_BUSPIRATE_SPI=1'
PROGRAMMER_OBJS += buspirate_spi.o
NEED_SERIAL += CONFIG_BUSPIRATE_SPI
endif

ifeq ($(CONFIG_DEDIPROG), yes)
FEATURE_CFLAGS += -D'CONFIG_DEDIPROG=1'
PROGRAMMER_OBJS += dediprog.o
NEED_LIBUSB1 += CONFIG_DEDIPROG
endif

ifeq ($(CONFIG_DEVELOPERBOX_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_DEVELOPERBOX_SPI=1'
PROGRAMMER_OBJS += developerbox_spi.o
NEED_LIBUSB1 += CONFIG_DEVELOPERBOX_SPI
endif

ifeq ($(CONFIG_SATAMV), yes)
FEATURE_CFLAGS += -D'CONFIG_SATAMV=1'
PROGRAMMER_OBJS += satamv.o
NEED_LIBPCI += CONFIG_SATAMV
endif

ifeq ($(CONFIG_LINUX_MTD), yes)
# This is a totally ugly hack.
FEATURE_CFLAGS += $(call debug_shell,grep -q "LINUX_MTD_SUPPORT := yes" .features && printf "%s" "-D'CONFIG_LINUX_MTD=1'")
PROGRAMMER_OBJS += linux_mtd.o
endif

ifeq ($(CONFIG_LINUX_SPI), yes)
# This is a totally ugly hack.
FEATURE_CFLAGS += $(call debug_shell,grep -q "LINUX_SPI_SUPPORT := yes" .features && printf "%s" "-D'CONFIG_LINUX_SPI=1'")
PROGRAMMER_OBJS += linux_spi.o
endif

ifeq ($(CONFIG_MSTARDDC_SPI), yes)
# This is a totally ugly hack.
FEATURE_CFLAGS += $(call debug_shell,grep -q "LINUX_I2C_SUPPORT := yes" .features && printf "%s" "-D'CONFIG_MSTARDDC_SPI=1'")
NEED_LINUX_I2C += CONFIG_MSTARDDC_SPI
PROGRAMMER_OBJS += mstarddc_spi.o
endif

ifeq ($(CONFIG_CH341A_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_CH341A_SPI=1'
PROGRAMMER_OBJS += ch341a_spi.o
NEED_LIBUSB1 += CONFIG_CH341A_SPI
endif

ifeq ($(CONFIG_DIGILENT_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_DIGILENT_SPI=1'
PROGRAMMER_OBJS += digilent_spi.o
NEED_LIBUSB1 += CONFIG_DIGILENT_SPI
endif

ifeq ($(CONFIG_JLINK_SPI), yes)
NEED_LIBJAYLINK += CONFIG_JLINK_SPI
FEATURE_CFLAGS += -D'CONFIG_JLINK_SPI=1'
PROGRAMMER_OBJS += jlink_spi.o
endif

ifneq ($(NEED_SERIAL), )
LIB_OBJS += serial.o custom_baud.o
endif

ifneq ($(NEED_POSIX_SOCKETS), )
ifeq ($(TARGET_OS), SunOS)
LIBS += -lsocket -lnsl
endif
endif

ifneq ($(NEED_LIBPCI), )
CHECK_LIBPCI = yes
# This is a dirty hack, but it saves us from checking all PCI drivers and all platforms manually.
# libpci may need raw memory, MSR or PCI port I/O on some platforms.
# Individual drivers might have the same needs as well.
NEED_RAW_ACCESS += $(NEED_LIBPCI)
FEATURE_CFLAGS += -D'NEED_PCI=1'
FEATURE_CFLAGS += $(call debug_shell,grep -q "OLD_PCI_GET_DEV := yes" .libdeps && printf "%s" "-D'OLD_PCI_GET_DEV=1'")

PROGRAMMER_OBJS += pcidev.o
ifeq ($(TARGET_OS), NetBSD)
# The libpci we want is called libpciutils on NetBSD and needs NetBSD libpci.
PCILIBS += -lpciutils -lpci
else
PCILIBS += -lpci
endif
endif

ifneq ($(NEED_RAW_ACCESS), )
# Raw memory, MSR or PCI port I/O access.
FEATURE_CFLAGS += -D'NEED_RAW_ACCESS=1'
PROGRAMMER_OBJS += physmap.o hwaccess.o

ifeq ($(TARGET_OS), NetBSD)
# For (i386|x86_64)_iopl(2).
PCILIBS += -l$(shell uname -p)
else
ifeq ($(TARGET_OS), OpenBSD)
# For (i386|amd64)_iopl(2).
PCILIBS += -l$(shell uname -m)
else
ifeq ($(TARGET_OS), Darwin)
# DirectHW framework can be found in the DirectHW library.
PCILIBS += -framework IOKit -framework DirectHW
endif
endif
endif

endif

ifneq ($(NEED_LIBUSB0), )
CHECK_LIBUSB0 = yes
FEATURE_CFLAGS += -D'NEED_LIBUSB0=1'
USBLIBS := $(call debug_shell,[ -n "$(PKG_CONFIG_LIBDIR)" ] && export PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)" ; $(PKG_CONFIG) --libs libusb || printf "%s" "-lusb")
endif

ifneq ($(NEED_LIBUSB1), )
CHECK_LIBUSB1 = yes
FEATURE_CFLAGS += -D'NEED_LIBUSB1=1'
PROGRAMMER_OBJS += usbdev.o
# FreeBSD and DragonflyBSD use a reimplementation of libusb-1.0 that is simply called libusb
ifeq ($(TARGET_OS),$(filter $(TARGET_OS),FreeBSD DragonFlyBSD))
USB1LIBS += -lusb
else
ifeq ($(TARGET_OS),NetBSD)
override CPPFLAGS += -I/usr/pkg/include/libusb-1.0
USB1LIBS += -lusb-1.0
else
USB1LIBS += $(call debug_shell,[ -n "$(PKG_CONFIG_LIBDIR)" ] && export PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)"; $(PKG_CONFIG) --libs libusb-1.0  || printf "%s" "-lusb-1.0")
override CPPFLAGS += $(call debug_shell,[ -n "$(PKG_CONFIG_LIBDIR)" ] && export PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)"; $(PKG_CONFIG) --cflags-only-I libusb-1.0  || printf "%s" "-I/usr/include/libusb-1.0")
endif
endif
endif

ifneq ($(NEED_LIBJAYLINK), )
CHECK_LIBJAYLINK = yes
JAYLINKLIBS += $(call debug_shell,[ -n "$(PKG_CONFIG_LIBDIR)" ] && export PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)"; $(PKG_CONFIG) --libs libjaylink)
override CPPFLAGS += $(call debug_shell,[ -n "$(PKG_CONFIG_LIBDIR)" ] && export PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)"; $(PKG_CONFIG) --cflags-only-I libjaylink)
endif

ifeq ($(CONFIG_PRINT_WIKI), yes)
FEATURE_CFLAGS += -D'CONFIG_PRINT_WIKI=1'
CLI_OBJS += print_wiki.o
endif

FEATURE_CFLAGS += $(call debug_shell,grep -q "UTSNAME := yes" .features && printf "%s" "-D'HAVE_UTSNAME=1'")

# We could use PULLED_IN_LIBS, but that would be ugly.
FEATURE_LIBS += $(call debug_shell,grep -q "NEEDLIBZ := yes" .libdeps && printf "%s" "-lz")

FEATURE_CFLAGS += $(call debug_shell,grep -q "CLOCK_GETTIME := yes" .features && printf "%s" "-D'HAVE_CLOCK_GETTIME=1'")
FEATURE_LIBS += $(call debug_shell,grep -q "CLOCK_GETTIME := yes" .features && printf "%s" "-lrt")

LIBFLASHROM_OBJS = $(CHIP_OBJS) $(PROGRAMMER_OBJS) $(LIB_OBJS)
OBJS = $(CLI_OBJS) $(LIBFLASHROM_OBJS)

all: hwlibs features $(PROGRAM)$(EXEC_SUFFIX) $(PROGRAM).8
ifeq ($(ARCH), x86)
	@+$(MAKE) -C util/ich_descriptors_tool/ TARGET_OS=$(TARGET_OS) EXEC_SUFFIX=$(EXEC_SUFFIX)
endif

$(PROGRAM)$(EXEC_SUFFIX): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROGRAM)$(EXEC_SUFFIX) $(OBJS) $(LIBS) $(PCILIBS) $(FEATURE_LIBS) $(USBLIBS) $(USB1LIBS) $(JAYLINKLIBS)

libflashrom.a: $(LIBFLASHROM_OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

# TAROPTIONS reduces information leakage from the packager's system.
# If other tar programs support command line arguments for setting uid/gid of
# stored files, they can be handled here as well.
TAROPTIONS = $(shell LC_ALL=C tar --version|grep -q GNU && echo "--owner=root --group=root")

%.o: %.c .features
	$(CC) -MMD $(CFLAGS) $(CPPFLAGS) $(FLASHROM_CFLAGS) $(FEATURE_CFLAGS) $(SCMDEF) -o $@ -c $<

# Make sure to add all names of generated binaries here.
# This includes all frontends and libflashrom.
# We don't use EXEC_SUFFIX here because we want to clean everything.
clean:
	rm -f $(PROGRAM) $(PROGRAM).exe libflashrom.a *.o *.d $(PROGRAM).8 $(PROGRAM).8.html $(BUILD_DETAILS_FILE)
	@+$(MAKE) -C util/ich_descriptors_tool/ clean

distclean: clean
	rm -f .features .libdeps

strip: $(PROGRAM)$(EXEC_SUFFIX)
	$(STRIP) $(STRIP_ARGS) $(PROGRAM)$(EXEC_SUFFIX)

# to define test programs we use verbatim variables, which get exported
# to environment variables and are referenced with $$<varname> later

define COMPILER_TEST
int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	return 0;
}
endef
export COMPILER_TEST

compiler: featuresavailable
	@printf "Checking for a C compiler... " | tee -a $(BUILD_DETAILS_FILE)
	@echo "$$COMPILER_TEST" > .test.c
	@printf "\nexec: %s\n" "$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .test.c -o .test$(EXEC_SUFFIX)" >>$(BUILD_DETAILS_FILE)
	@{ { { { { $(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .test.c -o .test$(EXEC_SUFFIX) >&2 && \
		echo "found." || { echo "not found."; \
		rm -f .test.c .test$(EXEC_SUFFIX); exit 1; }; } 2>>$(BUILD_DETAILS_FILE); echo $? >&3 ; } | tee -a $(BUILD_DETAILS_FILE) >&4; } 3>&1;} | { read rc ; exit ${rc}; } } 4>&1
	@rm -f .test.c .test$(EXEC_SUFFIX)
	@printf "Target arch is "
	@# FreeBSD wc will output extraneous whitespace.
	@echo $(ARCH)|wc -w|grep -q '^[[:blank:]]*1[[:blank:]]*$$' ||	\
		( echo "unknown. Aborting."; exit 1)
	@printf "%s\n" '$(ARCH)'
	@printf "Target OS is "
	@# FreeBSD wc will output extraneous whitespace.
	@echo $(TARGET_OS)|wc -w|grep -q '^[[:blank:]]*1[[:blank:]]*$$' ||	\
		( echo "unknown. Aborting."; exit 1)
	@printf "%s\n" '$(TARGET_OS)'
ifeq ($(TARGET_OS), libpayload)
	@$(CC) --version 2>&1 | grep -q coreboot || \
		( echo "Warning: It seems you are not using coreboot's reference compiler."; \
		  echo "This might work but usually does not, please beware." )
endif

define LIBPCI_TEST
/* Avoid a failing test due to libpci header symbol shadowing breakage */
#define index shadow_workaround_index
#if !defined __NetBSD__
#include <pci/pci.h>
#else
#include <pciutils/pci.h>
#endif
struct pci_access *pacc;
int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	pacc = pci_alloc();
	return 0;
}
endef
export LIBPCI_TEST

define PCI_GET_DEV_TEST
/* Avoid a failing test due to libpci header symbol shadowing breakage */
#define index shadow_workaround_index
#if !defined __NetBSD__
#include <pci/pci.h>
#else
#include <pciutils/pci.h>
#endif
struct pci_access *pacc;
struct pci_dev *dev = {0};
int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	pacc = pci_alloc();
	dev = pci_get_dev(pacc, dev->domain, dev->bus, dev->dev, 1);
	return 0;
}
endef
export PCI_GET_DEV_TEST

define LIBUSB0_TEST
#include "platform.h"
#if IS_WINDOWS
#include <lusb0_usb.h>
#else
#include <usb.h>
#endif
int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	usb_init();
	return 0;
}
endef
export LIBUSB0_TEST

define LIBUSB1_TEST
#include <stddef.h>
#include <libusb.h>
int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	libusb_init(NULL);
	return 0;
}
endef
export LIBUSB1_TEST

define LIBJAYLINK_TEST
#include <stddef.h>
#include <libjaylink/libjaylink.h>
int main(int argc, char **argv)
{
	struct jaylink_context *ctx;

	(void)argc;
	(void)argv;

	jaylink_init(&ctx);
	jaylink_exit(ctx);

	return 0;
}
endef
export LIBJAYLINK_TEST

hwlibs: compiler
	@printf "" > .libdeps
ifeq ($(CHECK_LIBPCI), yes)
	@printf "Checking for libpci headers... " | tee -a $(BUILD_DETAILS_FILE)
	@echo "$$LIBPCI_TEST" > .test.c
	@printf "\nexec: %s\n" "$(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o" >>$(BUILD_DETAILS_FILE)
	@{ { { { { $(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o >&2 && \
		echo "found." || { echo "not found."; echo;			\
		echo "The following features require libpci: $(NEED_LIBPCI).";	\
		echo "Please install libpci headers or disable all features"; \
		echo "mentioned above by specifying make CONFIG_ENABLE_LIBPCI_PROGRAMMERS=no"; \
		echo "See README for more information."; echo;			\
		rm -f .test.c .test.o; exit 1; }; } 2>>$(BUILD_DETAILS_FILE); echo $? >&3 ; } | tee -a $(BUILD_DETAILS_FILE) >&4; } 3>&1;} | { read rc ; exit ${rc}; } } 4>&1
	@printf "Checking version of pci_get_dev... " | tee -a $(BUILD_DETAILS_FILE)
	@echo "$$PCI_GET_DEV_TEST" > .test.c
	@printf "\nexec: %s\n" "$(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o" >>$(BUILD_DETAILS_FILE)
	@ { $(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o >&2 && \
		( echo "new version (including PCI domain parameter)."; echo "OLD_PCI_GET_DEV := no" >> .libdeps ) ||	\
		( echo "old version (without PCI domain parameter)."; echo "OLD_PCI_GET_DEV := yes" >> .libdeps ) } 2>>$(BUILD_DETAILS_FILE) | tee -a $(BUILD_DETAILS_FILE)
	@printf "Checking if libpci is present and sufficient... " | tee -a $(BUILD_DETAILS_FILE)
	@printf "\nexec: %s\n" "$(CC) $(LDFLAGS) .test.o -o .test$(EXEC_SUFFIX) $(LIBS) $(PCILIBS)" >>$(BUILD_DETAILS_FILE)
	@{ { { { $(CC) $(LDFLAGS) .test.o -o .test$(EXEC_SUFFIX) $(LIBS) $(PCILIBS) 2>>$(BUILD_DETAILS_FILE) >&2 && \
		echo "yes." || { echo "no.";							\
		printf "Checking if libz+libpci are present and sufficient..." ; \
		{ printf "\nexec: %s\n" "$(CC) $(LDFLAGS) .test.o -o .test$(EXEC_SUFFIX) $(LIBS) $(PCILIBS) -lz" >>$(BUILD_DETAILS_FILE) ; \
		$(CC) $(LDFLAGS) .test.o -o .test$(EXEC_SUFFIX) $(LIBS) $(PCILIBS) -lz >&2 && \
		echo "yes." && echo "NEEDLIBZ := yes" > .libdeps } || { echo "no."; echo;	\
		echo "The following features require libpci: $(NEED_LIBPCI).";			\
		echo "Please install libpci (package pciutils) and/or libz or disable all features"; \
		echo "mentioned above by specifying make CONFIG_ENABLE_LIBPCI_PROGRAMMERS=no"; \
		echo "See README for more information."; echo;				\
		rm -f .test.c .test.o .test$(EXEC_SUFFIX); exit 1; }; }; } 2>>$(BUILD_DETAILS_FILE); echo $? >&3 ; } | tee -a $(BUILD_DETAILS_FILE) >&4; } 3>&1;} | { read rc ; exit ${rc}; } } 4>&1
	@rm -f .test.c .test.o .test$(EXEC_SUFFIX)
endif
ifeq ($(CHECK_LIBUSB0), yes)
	@printf "Checking for libusb-0.1/libusb-compat headers... " | tee -a $(BUILD_DETAILS_FILE)
	@echo "$$LIBUSB0_TEST" > .test.c
	@printf "\nexec: %s\n" "$(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o" >>$(BUILD_DETAILS_FILE)
	@{ { { { { $(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o >&2 && \
		echo "found." || { echo "not found."; echo;				\
		echo "The following features require libusb-0.1/libusb-compat: $(NEED_LIBUSB0)."; \
		echo "Please install libusb-0.1 headers or libusb-compat headers or disable all features"; \
		echo "mentioned above by specifying make CONFIG_ENABLE_LIBUSB0_PROGRAMMERS=no"; \
		echo "See README for more information."; echo;				\
		rm -f .test.c .test.o; exit 1; }; } 2>>$(BUILD_DETAILS_FILE); echo $? >&3 ; } | tee -a $(BUILD_DETAILS_FILE) >&4; } 3>&1;} | { read rc ; exit ${rc}; } } 4>&1
	@printf "Checking if libusb-0.1 is usable... " | tee -a $(BUILD_DETAILS_FILE)
	@printf "\nexec: %s\n" "$(CC) $(LDFLAGS) .test.o -o .test$(EXEC_SUFFIX) $(LIBS) $(USBLIBS)" >>$(BUILD_DETAILS_FILE)
	@{ { { { { $(CC) $(LDFLAGS) .test.o -o .test$(EXEC_SUFFIX) $(LIBS) $(USBLIBS) >&2 && \
		echo "yes." || { echo "no.";						\
		echo "The following features require libusb-0.1/libusb-compat: $(NEED_LIBUSB0)."; \
		echo "Please install libusb-0.1 or libusb-compat or disable all features"; \
		echo "mentioned above by specifying make CONFIG_ENABLE_LIBUSB0_PROGRAMMERS=no"; \
		echo "See README for more information."; echo;				\
		rm -f .test.c .test.o .test$(EXEC_SUFFIX); exit 1; }; } 2>>$(BUILD_DETAILS_FILE); echo $? >&3 ; } | tee -a $(BUILD_DETAILS_FILE) >&4; } 3>&1;} | { read rc ; exit ${rc}; } } 4>&1
	@rm -f .test.c .test.o .test$(EXEC_SUFFIX)
endif
ifeq ($(CHECK_LIBUSB1), yes)
	@printf "Checking for libusb-1.0 headers... " | tee -a $(BUILD_DETAILS_FILE)
	@echo "$$LIBUSB1_TEST" > .test.c
	@printf "\nexec: %s\n" "$(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o" >>$(BUILD_DETAILS_FILE)
	@{ { { { { $(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o >&2 && \
		echo "found." || { echo "not found."; echo;				\
		echo "The following features require libusb-1.0: $(NEED_LIBUSB1).";	\
		echo "Please install libusb-1.0 headers or disable all features"; \
		echo "mentioned above by specifying make CONFIG_ENABLE_LIBUSB1_PROGRAMMERS=no"; \
		echo "See README for more information."; echo;				\
		rm -f .test.c .test.o; exit 1; }; } 2>>$(BUILD_DETAILS_FILE); echo $? >&3 ; } | tee -a $(BUILD_DETAILS_FILE) >&4; } 3>&1;} | { read rc ; exit ${rc}; } } 4>&1
	@printf "Checking if libusb-1.0 is usable... " | tee -a $(BUILD_DETAILS_FILE)
	@printf "\nexec: %s\n" "$(CC) $(LDFLAGS) .test.o -o .test$(EXEC_SUFFIX) $(LIBS) $(USB1LIBS)" >>$(BUILD_DETAILS_FILE)
	@{ { { { { $(CC) $(LDFLAGS) .test.o -o .test$(EXEC_SUFFIX) $(LIBS) $(USB1LIBS) >&2 && \
		echo "yes." || { echo "no.";						\
		echo "The following features require libusb-1.0: $(NEED_LIBUSB1).";	\
		echo "Please install libusb-1.0 or disable all features"; \
		echo "mentioned above by specifying make CONFIG_ENABLE_LIBUSB1_PROGRAMMERS=no"; \
		echo "See README for more information."; echo;				\
		rm -f .test.c .test.o .test$(EXEC_SUFFIX); exit 1; }; } 2>>$(BUILD_DETAILS_FILE); echo $? >&3 ; } | tee -a $(BUILD_DETAILS_FILE) >&4; } 3>&1;} | { read rc ; exit ${rc}; } } 4>&1
	@rm -f .test.c .test.o .test$(EXEC_SUFFIX)
endif
ifeq ($(CHECK_LIBJAYLINK), yes)
	@printf "Checking for libjaylink headers... " | tee -a $(BUILD_DETAILS_FILE)
	@echo "$$LIBJAYLINK_TEST" > .test.c
	@printf "\nexec: %s\n" "$(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o" >>$(BUILD_DETAILS_FILE)
	@{ { { { { $(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o >&2 && \
		echo "found." || { echo "not found."; echo;				\
		echo "The following feature requires libjaylink: $(NEED_LIBJAYLINK).";	\
		echo "Please install libjaylink headers or disable the feature"; \
		echo "mentioned above by specifying make CONFIG_JLINK_SPI=no"; \
		echo "See README for more information."; echo;				\
		rm -f .test.c .test.o; exit 1; }; } 2>>$(BUILD_DETAILS_FILE); echo $? >&3 ; } | tee -a $(BUILD_DETAILS_FILE) >&4; } 3>&1;} | { read rc ; exit ${rc}; } } 4>&1
	@printf "Checking if libjaylink is usable... " | tee -a $(BUILD_DETAILS_FILE)
	@printf "\nexec: %s\n" "$(CC) $(LDFLAGS) .test.o -o .test$(EXEC_SUFFIX) $(LIBS) $(JAYLINKLIBS)" >>$(BUILD_DETAILS_FILE)
	@{ { { { { $(CC) $(LDFLAGS) .test.o -o .test$(EXEC_SUFFIX) $(LIBS) $(JAYLINKLIBS) >&2 && \
		echo "yes." || { echo "no.";						\
		echo "The following feature requires libjaylink: $(NEED_LIBJAYLINK).";	\
		echo "Please install libjaylink or disable the feature"; \
		echo "mentioned above by specifying make CONFIG_JLINK_SPI=no"; \
		echo "See README for more information."; echo;				\
		rm -f .test.c .test.o .test$(EXEC_SUFFIX); exit 1; }; } 2>>$(BUILD_DETAILS_FILE); echo $? >&3 ; } | tee -a $(BUILD_DETAILS_FILE) >&4; } 3>&1;} | { read rc ; exit ${rc}; } } 4>&1
	@rm -f .test.c .test.o .test$(EXEC_SUFFIX)
endif

.features: features

# If a user does not explicitly request a non-working feature, we should
# silently disable it. However, if a non-working (does not compile) feature
# is explicitly requested, we should bail out with a descriptive error message.
# We also have to check that at least one programmer driver is enabled.
featuresavailable:
ifeq ($(PROGRAMMER_OBJS),)
	@echo "You have to enable at least one programmer driver!"
	@false
endif
ifneq ($(UNSUPPORTED_FEATURES), )
	@echo "The following features are unavailable on your machine: $(UNSUPPORTED_FEATURES)"
	@false
endif

define FTDI_TEST
#include <stdlib.h>
#include <ftdi.h>
struct ftdi_context *ftdic = NULL;
int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	return ftdi_init(ftdic);
}
endef
export FTDI_TEST

define FTDI_232H_TEST
#include <ftdi.h>
enum ftdi_chip_type type = TYPE_232H;
endef
export FTDI_232H_TEST

define UTSNAME_TEST
#include <sys/utsname.h>
struct utsname osinfo;
int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	uname (&osinfo);
	return 0;
}
endef
export UTSNAME_TEST

define LINUX_MTD_TEST
#include <mtd/mtd-user.h>

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	return 0;
}
endef
export LINUX_MTD_TEST

define LINUX_SPI_TEST
#include <linux/types.h>
#include <linux/spi/spidev.h>

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	return 0;
}
endef
export LINUX_SPI_TEST

define LINUX_I2C_TEST
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	return 0;
}
endef
export LINUX_I2C_TEST

define CLOCK_GETTIME_TEST
#include <time.h>

int main(int argc, char **argv)
{
	struct timespec res;
	clock_gettime(CLOCK_REALTIME, &res);
	return 0;
}
endef
export CLOCK_GETTIME_TEST

features: compiler
	@echo "FEATURES := yes" > .features.tmp
ifneq ($(NEED_LIBFTDI), )
	@printf "Checking for FTDI support... " | tee -a $(BUILD_DETAILS_FILE)
	@echo "$$FTDI_TEST" > .featuretest.c
	@printf "\nexec: %s\n" "$(CC) $(CPPFLAGS) $(CFLAGS) $(FTDI_INCLUDES) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX) $(FTDILIBS) $(LIBS)" >>$(BUILD_DETAILS_FILE)
	@ { $(CC) $(CPPFLAGS) $(CFLAGS) $(FTDI_INCLUDES) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX) $(FTDILIBS) $(LIBS) >&2 && \
	(	echo "found."; echo "FTDISUPPORT := yes" >> .features.tmp ; \
		printf "Checking for FT232H support in libftdi... " ; \
		echo "$$FTDI_232H_TEST" >> .featuretest.c ; \
		printf "\nexec: %s\n" "$(CC) $(CPPFLAGS) $(CFLAGS) $(FTDI_INCLUDES) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX) $(FTDILIBS) $(LIBS)" >>$(BUILD_DETAILS_FILE) ; \
		{ $(CC) $(CPPFLAGS) $(CFLAGS) $(FTDI_INCLUDES) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX) $(FTDILIBS) $(LIBS) >&2 && \
			( echo "found."; echo "FT232H := yes" >> .features.tmp ) ||	\
			( echo "not found."; echo "FT232H := no" >> .features.tmp ) } \
	) || \
	( echo "not found."; echo "FTDISUPPORT := no" >> .features.tmp ) } \
	2>>$(BUILD_DETAILS_FILE) | tee -a $(BUILD_DETAILS_FILE)
endif
ifeq ($(CONFIG_LINUX_MTD), yes)
	@printf "Checking if Linux MTD headers are present... " | tee -a $(BUILD_DETAILS_FILE)
	@echo "$$LINUX_MTD_TEST" > .featuretest.c
	@printf "\nexec: %s\n" "$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX)" >>$(BUILD_DETAILS_FILE)
	@ { $(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX) >&2 && \
		( echo "yes."; echo "LINUX_MTD_SUPPORT := yes" >> .features.tmp ) ||	\
		( echo "no."; echo "LINUX_MTD_SUPPORT := no" >> .features.tmp ) } \
		2>>$(BUILD_DETAILS_FILE) | tee -a $(BUILD_DETAILS_FILE)
endif
ifeq ($(CONFIG_LINUX_SPI), yes)
	@printf "Checking if Linux SPI headers are present... " | tee -a $(BUILD_DETAILS_FILE)
	@echo "$$LINUX_SPI_TEST" > .featuretest.c
	@printf "\nexec: %s\n" "$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX)" >>$(BUILD_DETAILS_FILE)
	@ { $(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX) >&2 && \
		( echo "yes."; echo "LINUX_SPI_SUPPORT := yes" >> .features.tmp ) ||	\
		( echo "no."; echo "LINUX_SPI_SUPPORT := no" >> .features.tmp ) } \
		2>>$(BUILD_DETAILS_FILE) | tee -a $(BUILD_DETAILS_FILE)
endif
ifneq ($(NEED_LINUX_I2C), )
	@printf "Checking if Linux I2C headers are present... " | tee -a $(BUILD_DETAILS_FILE)
	@echo "$$LINUX_I2C_TEST" > .featuretest.c
	@printf "\nexec: %s\n" "$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX)" >>$(BUILD_DETAILS_FILE)
	@ { $(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX) >&2 && \
		( echo "yes."; echo "LINUX_I2C_SUPPORT := yes" >> .features.tmp ) ||	\
		( echo "no."; echo "LINUX_I2C_SUPPORT := no" >> .features.tmp ) } \
		2>>$(BUILD_DETAILS_FILE) | tee -a $(BUILD_DETAILS_FILE)
endif
	@printf "Checking for utsname support... " | tee -a $(BUILD_DETAILS_FILE)
	@echo "$$UTSNAME_TEST" > .featuretest.c
	@printf "\nexec: %s\n" "$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX)" >>$(BUILD_DETAILS_FILE)
	@ { $(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX) >&2 && \
		( echo "found."; echo "UTSNAME := yes" >> .features.tmp ) ||	\
		( echo "not found."; echo "UTSNAME := no" >> .features.tmp ) } 2>>$(BUILD_DETAILS_FILE) | tee -a $(BUILD_DETAILS_FILE)
	@printf "Checking for clock_gettime support... " | tee -a $(BUILD_DETAILS_FILE)
	@echo "$$CLOCK_GETTIME_TEST" >.featuretest.c
	@printf "\nexec: %s\n" "$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -lrt .featuretest.c -o .featuretest$(EXEC_SUFFIX)" >>$(BUILD_DETAILS_FILE)
	@ { $(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -lrt .featuretest.c -o .featuretest$(EXEC_SUFFIX) >&2 && \
		( echo "found."; echo "CLOCK_GETTIME := yes" >>.features.tmp ) || \
		( echo "not found."; echo "CLOCK_GETTIME := no" >>.features.tmp ) } \
		2>>$(BUILD_DETAILS_FILE) | tee -a $(BUILD_DETAILS_FILE)
	@$(DIFF) -q .features.tmp .features >/dev/null 2>&1 && rm .features.tmp || mv .features.tmp .features
	@rm -f .featuretest.c .featuretest$(EXEC_SUFFIX)

$(PROGRAM).8.html: $(PROGRAM).8
	@groff -mandoc -Thtml $< >$@

$(PROGRAM).8: $(PROGRAM).8.tmpl
	@# Add the man page change date and version to the man page
	@sed -e 's#.TH FLASHROM 8 .*#.TH FLASHROM 8 "$(MAN_DATE)" "$(VERSION)" "$(MAN_DATE)"#' <$< >$@

install: $(PROGRAM)$(EXEC_SUFFIX) $(PROGRAM).8
	mkdir -p $(DESTDIR)$(PREFIX)/sbin
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -m 0755 $(PROGRAM)$(EXEC_SUFFIX) $(DESTDIR)$(PREFIX)/sbin
	$(INSTALL) -m 0644 $(PROGRAM).8 $(DESTDIR)$(MANDIR)/man8

libinstall: libflashrom.a libflashrom.h
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	$(INSTALL) -m 0644 libflashrom.a $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/include
	$(INSTALL) -m 0644 libflashrom.h $(DESTDIR)$(PREFIX)/include

_export: $(PROGRAM).8
	@rm -rf "$(EXPORTDIR)/flashrom-$(RELEASENAME)"
	@mkdir -p "$(EXPORTDIR)/flashrom-$(RELEASENAME)"
	@git archive HEAD | tar -x -C "$(EXPORTDIR)/flashrom-$(RELEASENAME)"
#	Generate versioninfo.inc containing metadata that would not be available in exported sources otherwise.
	@echo "VERSION = $(VERSION)" > "$(EXPORTDIR)/flashrom-$(RELEASENAME)/versioninfo.inc"
	@echo "MAN_DATE = $(MAN_DATE)" >> "$(EXPORTDIR)/flashrom-$(RELEASENAME)/versioninfo.inc"
#	Restore modification date of all tracked files not marked 'export-ignore' in .gitattributes.
#	sed is required to filter out file names having the attribute set.
#	The sed program saves the file name in the hold buffer and then checks if the respective value is 'set'.
#	If so it ignores the rest of the program, which otherwise restores the file name and prints it.
	@git ls-tree -r -z -t --full-name --name-only HEAD | \
		git check-attr -z --stdin export-ignore | \
		sed -zne 'x;n;n;{/^set$$/b;};x;p;' | \
		xargs -0 sh -c 'for f; do \
			touch -d $$(git log --pretty=format:%cI -1 HEAD -- "$$f") \
				"$(EXPORTDIR)/flashrom-$(RELEASENAME)/$$f"; \
		done' dummy_arg0

export: _export
	@echo "Exported $(EXPORTDIR)/flashrom-$(RELEASENAME)/"

tarball: _export
	@tar -cj --format=ustar -f "$(EXPORTDIR)/flashrom-$(RELEASENAME).tar.bz2" -C $(EXPORTDIR)/ \
		$(TAROPTIONS) "flashrom-$(RELEASENAME)/"
#	Delete the exported directory again because it is most likely what's expected by the user.
	@rm -rf "$(EXPORTDIR)/flashrom-$(RELEASENAME)"
	@echo Created "$(EXPORTDIR)/flashrom-$(RELEASENAME).tar.bz2"

libpayload: clean
	make CC="CC=i386-elf-gcc lpgcc" AR=i386-elf-ar RANLIB=i386-elf-ranlib

.PHONY: all install clean distclean compiler hwlibs features _export export tarball featuresavailable libpayload

# Disable implicit suffixes and built-in rules (for performance and profit)
.SUFFIXES:

-include $(OBJS:.o=.d)
