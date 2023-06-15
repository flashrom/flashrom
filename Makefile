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
STRIP_ARGS = -s
INSTALL = install
PREFIX  ?= /usr/local
MANDIR  ?= $(PREFIX)/share/man
BASHCOMPDIR ?= $(PREFIX)/share/bash-completion/completions
CFLAGS  ?= -Os -Wall -Wextra -Wno-unused-parameter -Wshadow -Wmissing-prototypes -Wwrite-strings
EXPORTDIR ?= .
RANLIB  ?= ranlib
PKG_CONFIG ?= pkg-config
BUILD_DETAILS_FILE ?= build_details.txt
SPHINXBUILD ?= sphinx-build

# The following parameter changes the default programmer that will be used if there is no -p/--programmer
# argument given when running flashrom. The predefined setting does not enable any default so that every
# user has to declare the programmer he wants to use on every run. The rationale for this to be not set
# (to e.g. the internal programmer) is that forgetting to specify this when working with another programmer
# easily puts the system attached to the default programmer at risk (e.g. you want to flash coreboot to another
# system attached to an external programmer while the default programmer is set to the internal programmer, and
# you forget to use the -p parameter. This would (try to) overwrite the existing firmware of the computer
# running flashrom). Please do not enable this without thinking about the possible consequences. Possible
# values can be found when running 'flashrom --list-supported' under the 'Supported programmers' section.
CONFIG_DEFAULT_PROGRAMMER_NAME ?=
# The following adds a default parameter for the default programmer set above (only).
CONFIG_DEFAULT_PROGRAMMER_ARGS ?=
# Example: compiling with
#   make CONFIG_DEFAULT_PROGRAMMER_NAME=serprog CONFIG_DEFAULT_PROGRAMMER_ARGS="dev=/dev/ttyUSB0:1500000"
# would make executing './flashrom' (almost) equivialent to './flashrom -p serprog:dev=/dev/ttyUSB0:1500000'.

# The user can provide CPP, C and LDFLAGS and the Makefile will extend these
override CPPFLAGS := $(CPPFLAGS)
override CFLAGS   := $(CFLAGS)
override LDFLAGS  := $(LDFLAGS)

# If your compiler spits out excessive warnings, run make WARNERROR=no
# You shouldn't have to change this flag.
WARNERROR ?= yes

ifeq ($(WARNERROR), yes)
override CFLAGS += -Werror
endif

ifdef LIBS_BASE
PKG_CONFIG_LIBDIR ?= $(LIBS_BASE)/lib/pkgconfig
override CPPFLAGS += -I$(LIBS_BASE)/include
override LDFLAGS += -L$(LIBS_BASE)/lib -Wl,-rpath -Wl,$(LIBS_BASE)/lib
endif

ifeq ($(CONFIG_STATIC),yes)
override LDFLAGS += -static
endif

# Set LC_ALL=C to minimize influences of the locale.
# However, this won't work for the majority of relevant commands because they use the $(shell) function and
# GNU make does not relay variables exported within the makefile to their environment.
LC_ALL=C
export LC_ALL

dummy_for_make_3_80:=$(shell printf "Build started on %s\n\n" "$$(date)" >$(BUILD_DETAILS_FILE))

# Provide an easy way to execute a command, print its output to stdout and capture any error message on stderr
# in the build details file together with the original stdout output.
debug_shell = $(shell export LC_ALL=C ; { echo 'exec: export LC_ALL=C ; { $(subst ','\'',$(1)) ; }' >&2; \
    { $(1) ; } | tee -a $(BUILD_DETAILS_FILE) ; echo >&2 ; } 2>>$(BUILD_DETAILS_FILE))

include Makefile.include

###############################################################################
# Dependency handling.

DEPENDS_ON_SERIAL := \
	CONFIG_BUSPIRATE_SPI \
	CONFIG_PONY_SPI \
	CONFIG_SERPROG \

DEPENDS_ON_SOCKETS := \
	CONFIG_SERPROG \

DEPENDS_ON_BITBANG_SPI := \
	CONFIG_DEVELOPERBOX_SPI \
	CONFIG_INTERNAL_X86 \
	CONFIG_NICINTEL_SPI \
	CONFIG_OGP_SPI \
	CONFIG_PONY_SPI \
	CONFIG_RAYER_SPI \

DEPENDS_ON_RAW_MEM_ACCESS := \
	CONFIG_ATAPROMISE \
	CONFIG_DRKAISER \
	CONFIG_GFXNVIDIA \
	CONFIG_INTERNAL_X86 \
	CONFIG_IT8212 \
	CONFIG_NICINTEL \
	CONFIG_NICINTEL_EEPROM \
	CONFIG_NICINTEL_SPI \
	CONFIG_OGP_SPI \
	CONFIG_SATAMV \
	CONFIG_SATASII \

DEPENDS_ON_X86_MSR := \
	CONFIG_INTERNAL_X86 \

DEPENDS_ON_X86_PORT_IO := \
	CONFIG_ATAHPT \
	CONFIG_ATAPROMISE \
	CONFIG_INTERNAL_X86 \
	CONFIG_NIC3COM \
	CONFIG_NICNATSEMI \
	CONFIG_NICREALTEK \
	CONFIG_RAYER_SPI \
	CONFIG_SATAMV \

DEPENDS_ON_LIBPCI := \
	CONFIG_ASM106X \
	CONFIG_ATAHPT \
	CONFIG_ATAPROMISE \
	CONFIG_ATAVIA \
	CONFIG_DRKAISER \
	CONFIG_GFXNVIDIA \
	CONFIG_INTERNAL \
	CONFIG_IT8212 \
	CONFIG_NIC3COM \
	CONFIG_NICINTEL \
	CONFIG_NICINTEL_EEPROM \
	CONFIG_NICINTEL_SPI \
	CONFIG_NICNATSEMI \
	CONFIG_NICREALTEK \
	CONFIG_OGP_SPI \
	CONFIG_SATAMV \
	CONFIG_SATASII \

DEPENDS_ON_LIBUSB1 := \
	CONFIG_CH341A_SPI \
	CONFIG_CH347_SPI \
	CONFIG_DEDIPROG \
	CONFIG_DEVELOPERBOX_SPI \
	CONFIG_DIGILENT_SPI \
	CONFIG_PICKIT2_SPI \
	CONFIG_RAIDEN_DEBUG_SPI \
	CONFIG_STLINKV3_SPI \
	CONFIG_DIRTYJTAG_SPI \

DEPENDS_ON_LIBFTDI1 := \
	CONFIG_FT2232_SPI \
	CONFIG_USBBLASTER_SPI \

DEPENDS_ON_LIBJAYLINK := \
	CONFIG_JLINK_SPI \

DEPENDS_ON_LIB_NI845X := \
	CONFIG_NI845X_SPI \

DEPENDS_ON_LINUX_I2C := \
	CONFIG_MSTARDDC_SPI \
	CONFIG_PARADE_LSPCON \
	CONFIG_REALTEK_MST_I2C_SPI \
	CONFIG_MEDIATEK_I2C_SPI \

ifeq ($(CONFIG_ENABLE_LIBUSB1_PROGRAMMERS), no)
$(call disable_all,$(DEPENDS_ON_LIBUSB1))
endif

ifeq ($(CONFIG_ENABLE_LIBPCI_PROGRAMMERS), no)
$(call disable_all,$(DEPENDS_ON_LIBPCI))
endif

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

CC_WORKING := $(call c_compile_test, Makefile.d/cc_test.c)

# Configs for dependencies. Can be overwritten by commandline
CONFIG_LIBFTDI1_VERSION    := $(call dependency_version, libftdi1)
CONFIG_LIBFTDI1_CFLAGS     := $(call dependency_cflags, libftdi1)
CONFIG_LIBFTDI1_LDFLAGS    := $(call dependency_ldflags, libftdi1)

# Hack to keep legacy auto detection of Program Files (x86), Only active if none of the CONFIG_ variables for ni845x are set.
ifeq ($(CONFIG_NI845X_LIBRARY_PATH)$(CONFIG_LIB_NI845X_CFLAGS)$(CONFIG_LIB_NI845X_LDFLAGS),)
PROGRAMFILES_X86 = $(shell env | sed -n "s/^PROGRAMFILES(X86)=//p")
ifneq ($(PROGRAMFILES_X86DIR),)
ifneq ($(PROGRAMFILES_X86DIR), ${PROGRAMFILES})
NI854_X86_LIBRARY_PATH := '${PROGRAMFILES_X86}\National Instruments\NI-845x\MS Visual C'
endif
endif
endif

CONFIG_NI845X_LIBRARY_PATH := '${PROGRAMFILES}\National Instruments\NI-845x\MS Visual C'
CONFIG_LIB_NI845X_CFLAGS   := -I$(CONFIG_NI845X_LIBRARY_PATH) $(if NI854_X86_LIBRARY_PATH, -I${NI854_X86_LIBRARY_PATH})
CONFIG_LIB_NI845X_LDFLAGS  := -L$(CONFIG_NI845X_LIBRARY_PATH) $(if NI854_X86_LIBRARY_PATH, -L${NI854_X86_LIBRARY_PATH}) -lni845x

CONFIG_LIBJAYLINK_VERSION  := $(call dependency_version, libjaylink)
CONFIG_LIBJAYLINK_CFLAGS   := $(call dependency_cflags, libjaylink)
CONFIG_LIBJAYLINK_LDFLAGS  := $(call dependency_ldflags, libjaylink)

CONFIG_LIBUSB1_VERSION     := $(call dependency_version, libusb-1.0)
CONFIG_LIBUSB1_CFLAGS      := $(call dependency_cflags, libusb-1.0)
CONFIG_LIBUSB1_LDFLAGS     := $(call dependency_ldflags, libusb-1.0)

CONFIG_LIBPCI_VERSION      := $(call dependency_version, libpci)
CONFIG_LIBPCI_CFLAGS       := $(call dependency_cflags, libpci)
CONFIG_LIBPCI_LDFLAGS      := $(call dependency_ldflags, libpci)

# Determine the destination OS, architecture and endian
# IMPORTANT: The following lines must be placed before TARGET_OS, ARCH or ENDIAN
# is ever used (of course), but should come after any lines setting CC because
# the lines below use CC itself.
override TARGET_OS  := $(call c_macro_test, Makefile.d/os_test.h)
override ARCH       := $(call c_macro_test, Makefile.d/arch_test.h)
override ENDIAN     := $(call c_macro_test, Makefile.d/endian_test.h)


HAS_LIBFTDI1        := $(call find_dependency, libftdi1)
HAS_LIB_NI845X      := no
HAS_LIBJAYLINK      := $(call find_dependency, libjaylink)
HAS_LIBUSB1         := $(call find_dependency, libusb-1.0)
HAS_LIBPCI          := $(call find_dependency, libpci)

HAS_FT232H          := $(call c_compile_test, Makefile.d/ft232h_test.c, $(CONFIG_LIBFTDI1_CFLAGS))
HAS_UTSNAME         := $(call c_compile_test, Makefile.d/utsname_test.c)
HAS_CLOCK_GETTIME   := $(call c_compile_test, Makefile.d/clock_gettime_test.c)
HAS_EXTERN_LIBRT    := $(call c_link_test, Makefile.d/clock_gettime_test.c, , -lrt)
HAS_LINUX_MTD       := $(call c_compile_test, Makefile.d/linux_mtd_test.c)
HAS_LINUX_SPI       := $(call c_compile_test, Makefile.d/linux_spi_test.c)
HAS_LINUX_I2C       := $(call c_compile_test, Makefile.d/linux_i2c_test.c)
HAS_SERIAL          := $(strip $(if $(filter $(TARGET_OS), DOS libpayload), no, yes))
HAS_SPHINXBUILD     := $(shell command -v $(SPHINXBUILD) &>/dev/null && echo yes || echo no)
EXEC_SUFFIX         := $(strip $(if $(filter $(TARGET_OS), DOS MinGW), .exe))

override CFLAGS += -Iinclude

ifeq ($(TARGET_OS), DOS)
# DJGPP has odd uint*_t definitions which cause lots of format string warnings.
override CFLAGS += -Wno-format
override LDFLAGS += -lgetopt
endif

ifeq ($(TARGET_OS), $(filter $(TARGET_OS), MinGW Cygwin))
$(call mark_unsupported,$(DEPENDS_ON_RAW_MEM_ACCESS))
$(call mark_unsupported,$(DEPENDS_ON_X86_PORT_IO))
$(call mark_unsupported,$(DEPENDS_ON_X86_MSR))
FEATURE_FLAGS += -D'IS_WINDOWS=1'
else
FEATURE_FLAGS += -D'IS_WINDOWS=0'
endif

# FIXME: Should we check for Cygwin/MSVC as well?
ifeq ($(TARGET_OS), MinGW)
# MinGW doesn't have the ffs() function, but we can use gcc's __builtin_ffs().
FLASHROM_CFLAGS += -Dffs=__builtin_ffs
# Some functions provided by Microsoft do not work as described in C99 specifications. This macro fixes that
# for MinGW. See http://sourceforge.net/p/mingw-w64/wiki2/printf%20and%20scanf%20family/ */
FLASHROM_CFLAGS += -D__USE_MINGW_ANSI_STDIO=1

# For now we disable all PCI-based programmers on Windows/MinGW (no libpci).
$(call mark_unsupported,$(DEPENDS_ON_LIBPCI))
# And programmers that need raw access.
$(call mark_unsupported,$(DEPENDS_ON_RAW_MEM_ACCESS))

else # No MinGW

# NI USB-845x only supported on Windows at the moment
$(call mark_unsupported,CONFIG_NI845X_SPI)

endif

ifeq ($(TARGET_OS), libpayload)
ifeq ($(MAKECMDGOALS),)
.DEFAULT_GOAL := libflashrom.a
$(info Setting default goal to libflashrom.a)
endif
$(call mark_unsupported,CONFIG_DUMMY)
# libpayload does not provide the romsize field in struct pci_dev that the atapromise code requires.
$(call mark_unsupported,CONFIG_ATAPROMISE)
# Dediprog, Developerbox, USB-Blaster, PICkit2, CH341A and FT2232 are not supported with libpayload (missing libusb support).
$(call mark_unsupported,$(DEPENDS_ON_LIBUSB1) $(DEPENDS_ON_LIBFTDI) $(DEPENDS_ON_LIBJAYLINK))
endif

ifeq ($(HAS_LINUX_MTD), no)
$(call mark_unsupported,CONFIG_LINUX_MTD)
endif

ifeq ($(HAS_LINUX_SPI), no)
$(call mark_unsupported,CONFIG_LINUX_SPI)
endif

ifeq ($(HAS_LINUX_I2C), no)
$(call mark_unsupported,DEPENDS_ON_LINUX_I2C)
endif

ifeq ($(TARGET_OS), Android)
# Android on x86 (currently) does not provide raw PCI port I/O operations.
$(call mark_unsupported,$(DEPENDS_ON_X86_PORT_IO))
endif

# Disable the internal programmer on unsupported architectures or systems
ifeq ($(or $(filter $(ARCH), x86), $(filter $(TARGET_OS), Linux)), )
$(call mark_unsupported,CONFIG_INTERNAL)
endif

ifeq ($(HAS_LIBPCI), no)
$(call mark_unsupported,$(DEPENDS_ON_LIBPCI))
endif

ifeq ($(HAS_LIBFTDI1), no)
$(call mark_unsupported,$(DEPENDS_ON_LIBFTDI1))
endif

ifeq ($(HAS_LIB_NI845X), no)
$(call mark_unsupported,$(DEPENDS_ON_LIB_NI845X))
endif

ifeq ($(HAS_LIBJAYLINK), no)
$(call mark_unsupported,$(DEPENDS_ON_LIBJAYLINK))
endif

ifeq ($(HAS_LIBUSB1), no)
$(call mark_unsupported,$(DEPENDS_ON_LIBUSB1))
endif

ifeq ($(HAS_SERIAL), no)
$(call mark_unsupported, $(DEPENDS_ON_SERIAL))
endif

ifeq ($(ENDIAN), little)
FEATURE_FLAGS += -D'__FLASHROM_LITTLE_ENDIAN__=1'
endif
ifeq ($(ENDIAN), big)
FEATURE_FLAGS += -D'__FLASHROM_BIG_ENDIAN__=1'
endif

# PCI port I/O support is unimplemented on PPC/MIPS/SPARC and unavailable on ARM.
# Right now this means the drivers below only work on x86.
ifneq ($(ARCH), x86)
$(call mark_unsupported,$(DEPENDS_ON_X86_MSR))
$(call mark_unsupported,$(DEPENDS_ON_X86_PORT_IO))
endif

# Additionally disable all drivers needing raw access (memory, PCI, port I/O)
# on architectures with unknown raw access properties.
# Right now those architectures are alpha hppa m68k sh s390
ifneq ($(ARCH), $(filter $(ARCH), x86 mips ppc arm sparc arc e2k))
$(call mark_unsupported,$(DEPENDS_ON_RAW_MEM_ACCESS))
endif

###############################################################################
# Flash chip drivers and bus support infrastructure.

CHIP_OBJS = jedec.o printlock.o stm50.o w39.o w29ee011.o \
	sst28sf040.o 82802ab.o \
	sst49lfxxxc.o sst_fwhub.o edi.o flashchips.o spi.o spi25.o spi25_statusreg.o \
	spi95.o opaque.o sfdp.o en29lv640b.o at45db.o s25f.o \
	writeprotect.o writeprotect_ranges.o

###############################################################################
# Library code.

LIB_OBJS = libflashrom.o layout.o erasure_layout.o flashrom.o udelay.o parallel.o programmer.o programmer_table.o \
	helpers.o helpers_fileio.o ich_descriptors.o fmap.o platform/endian_$(ENDIAN).o platform/memaccess.o


###############################################################################
# Frontend related stuff.

CLI_OBJS = cli_classic.o cli_output.o cli_common.o print.o

VERSION ?= $(shell cat ./VERSION)
VERSION_GIT ?= $(shell git describe 2>/dev/null)
ifdef VERSION_GIT
  VERSION := "$(VERSION) (git:$(VERSION_GIT))"
endif

# No spaces in release names unless set explicitly
RELEASENAME ?= $(shell echo "$(VERSION)" | sed -e 's/ /_/')

###############################################################################
# Default settings of CONFIG_* variables.

# Always enable internal/onboard support for now.
CONFIG_INTERNAL ?= yes
CONFIG_INTERNAL_X86 ?= yes

# Always enable serprog for now.
CONFIG_SERPROG ?= yes

# RayeR SPIPGM hardware support
CONFIG_RAYER_SPI ?= yes

# ChromiumOS servo DUT debug board hardware support
CONFIG_RAIDEN_DEBUG_SPI ?= yes

# PonyProg2000 SPI hardware support
CONFIG_PONY_SPI ?= yes

# Always enable 3Com NICs for now.
CONFIG_NIC3COM ?= yes

# Enable NVIDIA graphics cards. Note: write and erase do not work properly.
CONFIG_GFXNVIDIA ?= yes

# Always enable SiI SATA controllers for now.
CONFIG_SATASII ?= yes

# ASMedia ASM106x
CONFIG_ASM106X ?= yes

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

# Always enable STLink V3
CONFIG_STLINKV3_SPI ?= yes

# Disables Parade LSPCON support until the i2c helper supports multiple systems.
CONFIG_PARADE_LSPCON ?= no

# Disables MediaTek support until the i2c helper supports multiple systems.
CONFIG_MEDIATEK_I2C_SPI ?= no

# Disables REALTEK_MST support until the i2c helper supports multiple systems.
CONFIG_REALTEK_MST_I2C_SPI ?= no

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

# Winchiphead CH347
CONFIG_CH347_SPI ?= yes

# Digilent Development board JTAG
CONFIG_DIGILENT_SPI ?= yes

# DirtyJTAG
CONFIG_DIRTYJTAG_SPI ?= yes

# Disable J-Link for now.
CONFIG_JLINK_SPI ?= no

# National Instruments USB-845x is Windows only and needs a proprietary library.
CONFIG_NI845X_SPI ?= no

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

ifdef CONFIG_DEFAULT_PROGRAMMER_NAME
FEATURE_FLAGS += -D'CONFIG_DEFAULT_PROGRAMMER_NAME=&programmer_$(CONFIG_DEFAULT_PROGRAMMER_NAME)'
else
FEATURE_FLAGS += -D'CONFIG_DEFAULT_PROGRAMMER_NAME=NULL'
endif

FEATURE_FLAGS += -D'CONFIG_DEFAULT_PROGRAMMER_ARGS="$(CONFIG_DEFAULT_PROGRAMMER_ARGS)"'

################################################################################

ifeq ($(ARCH), x86)
ifeq ($(CONFIG_INTERNAL) $(CONFIG_INTERNAL_X86), yes yes)
FEATURE_FLAGS += -D'CONFIG_INTERNAL=1'
PROGRAMMER_OBJS += processor_enable.o chipset_enable.o board_enable.o cbtable.o \
	internal.o internal_par.o it87spi.o sb600spi.o superio.o amd_imc.o wbsio_spi.o mcp6x_spi.o \
	ichspi.o dmi.o known_boards.o
ACTIVE_PROGRAMMERS += internal
endif
else
ifeq ($(CONFIG_INTERNAL), yes)
FEATURE_FLAGS += -D'CONFIG_INTERNAL=1'
PROGRAMMER_OBJS += processor_enable.o chipset_enable.o board_enable.o cbtable.o internal.o internal_par.o known_boards.o
ACTIVE_PROGRAMMERS += internal
endif
endif

ifeq ($(CONFIG_INTERNAL_DMI), yes)
FEATURE_FLAGS += -D'CONFIG_INTERNAL_DMI=1'
endif

ifeq ($(CONFIG_SERPROG), yes)
FEATURE_FLAGS += -D'CONFIG_SERPROG=1'
PROGRAMMER_OBJS += serprog.o
ACTIVE_PROGRAMMERS += serprog
endif

ifeq ($(CONFIG_RAYER_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_RAYER_SPI=1'
PROGRAMMER_OBJS += rayer_spi.o
ACTIVE_PROGRAMMERS += rayer_spi
endif

ifeq ($(CONFIG_RAIDEN_DEBUG_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_RAIDEN_DEBUG_SPI=1'
PROGRAMMER_OBJS += raiden_debug_spi.o
ACTIVE_PROGRAMMERS += raiden_debug_spi
endif

ifeq ($(CONFIG_PONY_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_PONY_SPI=1'
PROGRAMMER_OBJS += pony_spi.o
ACTIVE_PROGRAMMERS += pony_spi
endif

ifeq ($(CONFIG_NIC3COM), yes)
FEATURE_FLAGS += -D'CONFIG_NIC3COM=1'
PROGRAMMER_OBJS += nic3com.o
ACTIVE_PROGRAMMERS += nic3com
endif

ifeq ($(CONFIG_GFXNVIDIA), yes)
FEATURE_FLAGS += -D'CONFIG_GFXNVIDIA=1'
PROGRAMMER_OBJS += gfxnvidia.o
ACTIVE_PROGRAMMERS += gfxnvidia
endif

ifeq ($(CONFIG_SATASII), yes)
FEATURE_FLAGS += -D'CONFIG_SATASII=1'
PROGRAMMER_OBJS += satasii.o
ACTIVE_PROGRAMMERS += satasii
endif

ifeq ($(CONFIG_ASM106X), yes)
FEATURE_FLAGS += -D'CONFIG_ASM106X=1'
PROGRAMMER_OBJS += asm106x.o
ACTIVE_PROGRAMMERS += asm106x
endif

ifeq ($(CONFIG_ATAHPT), yes)
FEATURE_FLAGS += -D'CONFIG_ATAHPT=1'
PROGRAMMER_OBJS += atahpt.o
ACTIVE_PROGRAMMERS += atahpt
endif

ifeq ($(CONFIG_ATAVIA), yes)
FEATURE_FLAGS += -D'CONFIG_ATAVIA=1'
PROGRAMMER_OBJS += atavia.o
ACTIVE_PROGRAMMERS += atavia
endif

ifeq ($(CONFIG_ATAPROMISE), yes)
FEATURE_FLAGS += -D'CONFIG_ATAPROMISE=1'
PROGRAMMER_OBJS += atapromise.o
ACTIVE_PROGRAMMERS += atapromise
endif

ifeq ($(CONFIG_IT8212), yes)
FEATURE_FLAGS += -D'CONFIG_IT8212=1'
PROGRAMMER_OBJS += it8212.o
ACTIVE_PROGRAMMERS += it8212
endif

ifeq ($(CONFIG_FT2232_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_FT2232_SPI=1'
PROGRAMMER_OBJS += ft2232_spi.o
ACTIVE_PROGRAMMERS += ft2232_spi
endif

ifeq ($(CONFIG_USBBLASTER_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_USBBLASTER_SPI=1'
PROGRAMMER_OBJS += usbblaster_spi.o
ACTIVE_PROGRAMMERS += usbblaster_spi
endif

ifeq ($(CONFIG_PICKIT2_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_PICKIT2_SPI=1'
PROGRAMMER_OBJS += pickit2_spi.o
ACTIVE_PROGRAMMERS += pickit2_spi
endif

ifeq ($(CONFIG_STLINKV3_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_STLINKV3_SPI=1'
PROGRAMMER_OBJS += stlinkv3_spi.o
ACTIVE_PROGRAMMERS += stlinkv3_spi
endif

ifeq ($(CONFIG_PARADE_LSPCON), yes)
FEATURE_FLAGS += -D'CONFIG_PARADE_LSPCON=1'
PROGRAMMER_OBJS += parade_lspcon.o
ACTIVE_PROGRAMMERS += parade_lspcon
endif

ifeq ($(CONFIG_MEDIATEK_I2C_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_MEDIATEK_I2C_SPI=1'
PROGRAMMER_OBJS += mediatek_i2c_spi.o
ACTIVE_PROGRAMMERS += mediatek_i2c_spi
endif

ifeq ($(CONFIG_REALTEK_MST_I2C_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_REALTEK_MST_I2C_SPI=1'
PROGRAMMER_OBJS += realtek_mst_i2c_spi.o
ACTIVE_PROGRAMMERS += realtek_mst_i2c_spi
endif

ifeq ($(CONFIG_DUMMY), yes)
FEATURE_FLAGS += -D'CONFIG_DUMMY=1'
PROGRAMMER_OBJS += dummyflasher.o
ACTIVE_PROGRAMMERS += dummyflasher
endif

ifeq ($(CONFIG_DRKAISER), yes)
FEATURE_FLAGS += -D'CONFIG_DRKAISER=1'
PROGRAMMER_OBJS += drkaiser.o
ACTIVE_PROGRAMMERS += drkaiser
endif

ifeq ($(CONFIG_NICREALTEK), yes)
FEATURE_FLAGS += -D'CONFIG_NICREALTEK=1'
PROGRAMMER_OBJS += nicrealtek.o
ACTIVE_PROGRAMMERS += nicrealtek
endif

ifeq ($(CONFIG_NICNATSEMI), yes)
FEATURE_FLAGS += -D'CONFIG_NICNATSEMI=1'
PROGRAMMER_OBJS += nicnatsemi.o
ACTIVE_PROGRAMMERS += nicnatsemi
endif

ifeq ($(CONFIG_NICINTEL), yes)
FEATURE_FLAGS += -D'CONFIG_NICINTEL=1'
PROGRAMMER_OBJS += nicintel.o
ACTIVE_PROGRAMMERS += nicintel
endif

ifeq ($(CONFIG_NICINTEL_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_NICINTEL_SPI=1'
PROGRAMMER_OBJS += nicintel_spi.o
ACTIVE_PROGRAMMERS += nicintel_spi
endif

ifeq ($(CONFIG_NICINTEL_EEPROM), yes)
FEATURE_FLAGS += -D'CONFIG_NICINTEL_EEPROM=1'
PROGRAMMER_OBJS += nicintel_eeprom.o
ACTIVE_PROGRAMMERS += nicintel_eeprom
endif

ifeq ($(CONFIG_OGP_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_OGP_SPI=1'
PROGRAMMER_OBJS += ogp_spi.o
ACTIVE_PROGRAMMERS += ogp_spi
endif

ifeq ($(CONFIG_BUSPIRATE_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_BUSPIRATE_SPI=1'
PROGRAMMER_OBJS += buspirate_spi.o
ACTIVE_PROGRAMMERS += buspirate_spi
endif

ifeq ($(CONFIG_DEDIPROG), yes)
FEATURE_FLAGS += -D'CONFIG_DEDIPROG=1'
PROGRAMMER_OBJS += dediprog.o
ACTIVE_PROGRAMMERS += dediprog
endif

ifeq ($(CONFIG_DEVELOPERBOX_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_DEVELOPERBOX_SPI=1'
PROGRAMMER_OBJS += developerbox_spi.o
ACTIVE_PROGRAMMERS += developerbox_spi
endif

ifeq ($(CONFIG_SATAMV), yes)
FEATURE_FLAGS += -D'CONFIG_SATAMV=1'
PROGRAMMER_OBJS += satamv.o
ACTIVE_PROGRAMMERS += satamv
endif

ifeq ($(CONFIG_LINUX_MTD), yes)
FEATURE_FLAGS += -D'CONFIG_LINUX_MTD=1'
PROGRAMMER_OBJS += linux_mtd.o
ACTIVE_PROGRAMMERS += linux_mtd
endif

ifeq ($(CONFIG_LINUX_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_LINUX_SPI=1'
PROGRAMMER_OBJS += linux_spi.o
ACTIVE_PROGRAMMERS += linux_spi
endif

ifeq ($(CONFIG_MSTARDDC_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_MSTARDDC_SPI=1'
PROGRAMMER_OBJS += mstarddc_spi.o
ACTIVE_PROGRAMMERS += mstarddc_spi
endif

ifeq ($(CONFIG_CH341A_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_CH341A_SPI=1'
PROGRAMMER_OBJS += ch341a_spi.o
ACTIVE_PROGRAMMERS += ch341a_spi
endif

ifeq ($(CONFIG_CH347_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_CH347_SPI=1'
PROGRAMMER_OBJS += ch347_spi.o
endif

ifeq ($(CONFIG_DIGILENT_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_DIGILENT_SPI=1'
PROGRAMMER_OBJS += digilent_spi.o
ACTIVE_PROGRAMMERS += digilent_spi
endif

ifeq ($(CONFIG_DIRTYJTAG_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_DIRTYJTAG_SPI=1'
PROGRAMMER_OBJS += dirtyjtag_spi.o
ACTIVE_PROGRAMMERS += dirtyjtag_spi
endif

ifeq ($(CONFIG_JLINK_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_JLINK_SPI=1'
PROGRAMMER_OBJS += jlink_spi.o
ACTIVE_PROGRAMMERS += jlink_spi
endif

ifeq ($(CONFIG_NI845X_SPI), yes)
FEATURE_FLAGS += -D'CONFIG_NI845X_SPI=1'
PROGRAMMER_OBJS += ni845x_spi.o
ACTIVE_PROGRAMMERS += ni845x_spi
endif

USE_BITBANG_SPI := $(if $(call filter_deps,$(DEPENDS_ON_BITBANG_SPI)),yes,no)
ifeq ($(USE_BITBANG_SPI), yes)
LIB_OBJS += bitbang_spi.o
endif

USE_LINUX_I2C := $(if $(call filter_deps,$(DEPENDS_ON_LINUX_I2C)),yes,no)
ifeq ($(USE_LINUX_I2C), yes)
LIB_OBJS += i2c_helper_linux.o
endif

USE_SERIAL := $(if $(call filter_deps,$(DEPENDS_ON_SERIAL)),yes,no)
ifeq ($(USE_SERIAL), yes)
LIB_OBJS += serial.o
ifeq ($(TARGET_OS), Linux)
LIB_OBJS += custom_baud_linux.o
else
ifeq ($(TARGET_OS), Darwin)
LIB_OBJS += custom_baud_darwin.o
else
LIB_OBJS += custom_baud.o
endif
endif
endif

USE_SOCKETS := $(if $(call filter_deps,$(DEPENDS_ON_SOCKETS)),yes,no)
ifeq ($(USE_SOCKETS), yes)
ifeq ($(TARGET_OS), SunOS)
override LDFLAGS += -lsocket -lnsl
endif
endif

USE_X86_MSR := $(if $(call filter_deps,$(DEPENDS_ON_X86_MSR)),yes,no)
ifeq ($(USE_X86_MSR), yes)
PROGRAMMER_OBJS += hwaccess_x86_msr.o
endif

USE_X86_PORT_IO := $(if $(call filter_deps,$(DEPENDS_ON_X86_PORT_IO)),yes,no)
ifeq ($(USE_X86_PORT_IO), yes)
FEATURE_FLAGS += -D'__FLASHROM_HAVE_OUTB__=1'
PROGRAMMER_OBJS += hwaccess_x86_io.o
endif

USE_RAW_MEM_ACCESS := $(if $(call filter_deps,$(DEPENDS_ON_RAW_MEM_ACCESS)),yes,no)
ifeq ($(USE_RAW_MEM_ACCESS), yes)
PROGRAMMER_OBJS += hwaccess_physmap.o
endif

ifeq (Darwin yes, $(TARGET_OS) $(filter $(USE_X86_MSR) $(USE_X86_PORT_IO) $(USE_RAW_MEM_ACCESS), yes))
override LDFLAGS += -framework IOKit -framework DirectHW
endif

ifeq (NetBSD yes, $(TARGET_OS) $(filter $(USE_X86_MSR) $(USE_X86_PORT_IO), yes))
override LDFLAGS += -l$(shell uname -p)
endif

ifeq (OpenBSD yes, $(TARGET_OS) $(filter $(USE_X86_MSR) $(USE_X86_PORT_IO), yes))
override LDFLAGS += -l$(shell uname -m)
endif

USE_LIBPCI := $(if $(call filter_deps,$(DEPENDS_ON_LIBPCI)),yes,no)
ifeq ($(USE_LIBPCI), yes)
PROGRAMMER_OBJS += pcidev.o
override CFLAGS  += $(CONFIG_LIBPCI_CFLAGS)
override LDFLAGS += $(CONFIG_LIBPCI_LDFLAGS)
endif

USE_LIBUSB1 := $(if $(call filter_deps,$(DEPENDS_ON_LIBUSB1)),yes,no)
ifeq ($(USE_LIBUSB1), yes)
override CFLAGS  += $(CONFIG_LIBUSB1_CFLAGS)
override LDFLAGS += $(CONFIG_LIBUSB1_LDFLAGS)
PROGRAMMER_OBJS +=usbdev.o usb_device.o
endif

USE_LIBFTDI1 := $(if $(call filter_deps,$(DEPENDS_ON_LIBFTDI1)),yes,no)
ifeq ($(USE_LIBFTDI1), yes)
override CFLAGS  += $(CONFIG_LIBFTDI1_CFLAGS)
override LDFLAGS += $(CONFIG_LIBFTDI1_LDFLAGS)
ifeq ($(HAS_FT232H), yes)
FEATURE_FLAGS += -D'HAVE_FT232H=1'
endif
endif

USE_LIB_NI845X := $(if $(call filter_deps,$(DEPENDS_ON_LIB_NI845X)),yes,no)
ifeq ($(USE_LIB_NI845X), yes)
override CFLAGS += $(CONFIG_LIB_NI845X_CFLAGS)
override LDFLAGS += $(CONFIG_LIB_NI845X_LDFLAGS)
endif

USE_LIBJAYLINK := $(if $(call filter_deps,$(DEPENDS_ON_LIBJAYLINK)),yes,no)
ifeq ($(USE_LIBJAYLINK), yes)
override CFLAGS  += $(CONFIG_LIBJAYLINK_CFLAGS)
override LDFLAGS += $(CONFIG_LIBJAYLINK_LDFLAGS)
endif

ifeq ($(CONFIG_PRINT_WIKI), yes)
FEATURE_FLAGS += -D'CONFIG_PRINT_WIKI=1'
CLI_OBJS += print_wiki.o
endif

ifeq ($(HAS_UTSNAME), yes)
FEATURE_FLAGS += -D'HAVE_UTSNAME=1'
endif

ifeq ($(HAS_CLOCK_GETTIME), yes)
FEATURE_FLAGS += -D'HAVE_CLOCK_GETTIME=1'
ifeq ($(HAS_EXTERN_LIBRT), yes)
override LDFLAGS += -lrt
endif
endif

OBJS = $(CHIP_OBJS) $(PROGRAMMER_OBJS) $(LIB_OBJS)


all: $(PROGRAM)$(EXEC_SUFFIX) $(call has_dependency, $(HAS_SPHINXBUILD), man8/$(PROGRAM).8)
ifeq ($(ARCH), x86)
	@+$(MAKE) -C util/ich_descriptors_tool/ HOST_OS=$(HOST_OS) TARGET_OS=$(TARGET_OS)
endif

config:
	@echo Building flashrom version $(VERSION)
	@printf "C compiler found: "
	@if [ $(CC_WORKING) = yes ]; \
		then $(CC) --version 2>/dev/null | head -1; \
		else echo no; echo Aborting.; exit 1; fi
	@echo "Target arch: $(ARCH)"
	@if [ $(ARCH) = unknown ]; then echo Aborting.; exit 1; fi
	@echo "Target OS: $(TARGET_OS)"
	@if [ $(TARGET_OS) = unknown ]; then echo Aborting.; exit 1; fi
	@if [ $(TARGET_OS) = libpayload ] && ! $(CC) --version 2>&1 | grep -q coreboot; then \
		echo "  Warning: It seems you are not using coreboot's reference compiler."; \
		echo "  This might work but usually does not, please beware."; fi
	@echo "Target endian: $(ENDIAN)"
	@if [ $(ENDIAN) = unknown ]; then echo Aborting.; exit 1; fi
	@echo Dependency libpci found: $(HAS_LIBPCI) $(CONFIG_LIBPCI_VERSION)
	@if [ $(HAS_LIBPCI) = yes ]; then			\
		echo "  CFLAGS: $(CONFIG_LIBPCI_CFLAGS)";	\
		echo "  LDFLAGS: $(CONFIG_LIBPCI_LDFLAGS)";	\
	fi
	@echo Dependency libusb1 found: $(HAS_LIBUSB1) $(CONFIG_LIBUSB1_VERSION)
	@if [ $(HAS_LIBUSB1) = yes ]; then			\
		echo "  CFLAGS: $(CONFIG_LIBUSB1_CFLAGS)";	\
		echo "  LDFLAGS: $(CONFIG_LIBUSB1_LDFLAGS)";	\
	fi
	@echo Dependency libjaylink found: $(HAS_LIBJAYLINK) $(CONFIG_LIBJAYLINK_VERSION)
	@if [ $(HAS_LIBJAYLINK) = yes ]; then			\
		echo "  CFLAGS: $(CONFIG_LIBJAYLINK_CFLAGS)";	\
		echo "  LDFLAGS: $(CONFIG_LIBJAYLINK_LDFLAGS)";	\
	fi
	@echo Dependency NI-845x found: $(HAS_LIB_NI845X)
	@if [ $(HAS_LIB_NI845X) = yes ]; then			\
		echo "  CFLAGS: $(CONFIG_LIB_NI845X_CFLAGS)";	\
		echo "  LDFLAGS: $(CONFIG_LIB_NI845X_LDFLAGS)";	\
	fi
	@echo Dependency libftdi1 found: $(HAS_LIBFTDI1) $(CONFIG_LIBFTDI1_VERSION)
	@if [ $(HAS_LIBFTDI1) = yes ]; then 			\
		echo "  Checking for \"TYPE_232H\" in \"enum ftdi_chip_type\": $(HAS_FT232H)"; \
		echo "  CFLAGS: $(CONFIG_LIBFTDI1_CFLAGS)";	\
		echo "  LDFLAGS: $(CONFIG_LIBFTDI1_LDFLAGS)";	\
	fi
	@echo "Checking for header \"mtd/mtd-user.h\": $(HAS_LINUX_MTD)"
	@echo "Checking for header \"linux/spi/spidev.h\": $(HAS_LINUX_SPI)"
	@echo "Checking for header \"linux/i2c-dev.h\": $(HAS_LINUX_I2C)"
	@echo "Checking for header \"linux/i2c.h\": $(HAS_LINUX_I2C)"
	@echo "Checking for header \"sys/utsname.h\": $(HAS_UTSNAME)"
	@echo "Checking for function \"clock_gettime\": $(HAS_CLOCK_GETTIME)"
	@echo "Checking for external \"librt\": $(HAS_EXTERN_LIBRT)"
	@if ! [ "$(PROGRAMMER_OBJS)" ]; then					\
		echo "You have to enable at least one programmer driver!";	\
		exit 1;								\
	fi
	@if [ "$(UNSUPPORTED_FEATURES)" ]; then					\
		echo "The following features are unavailable on your machine: $(UNSUPPORTED_FEATURES)" \
		exit 1;								\
	fi
	@echo "Checking for program \"sphinx-build\": $(HAS_SPHINXBUILD)"

%.o: %.c | config
	$(CC) -MMD $(CFLAGS) $(CPPFLAGS) $(FLASHROM_CFLAGS) $(FEATURE_FLAGS) -D'FLASHROM_VERSION=$(VERSION)'  -o $@ -c $<

$(PROGRAM)$(EXEC_SUFFIX): $(CLI_OBJS) libflashrom.a
	$(CC) -o $@ $^ $(LDFLAGS)

libflashrom.a: $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

man8/$(PROGRAM).8: doc/*
	@if [ "$(HAS_SPHINXBUILD)" = "yes" ]; then			\
		$(SPHINXBUILD) -Drelease=$(VERSION) -b man doc .;	\
	else								\
		echo "$(SPHINXBUILD) not found. Can't build man-page";	\
		exit 1;							\
	fi

$(PROGRAM).bash: util/$(PROGRAM).bash-completion.tmpl
	@# Add to the bash completion file a list of enabled programmers.
	sed -e 's/@PROGRAMMERS@/$(ACTIVE_PROGRAMMERS)/g' <$< >$@

strip: $(PROGRAM)$(EXEC_SUFFIX)
	$(STRIP) $(STRIP_ARGS) $(PROGRAM)$(EXEC_SUFFIX)

# Make sure to add all names of generated binaries here.
# This includes all frontends and libflashrom.
# We don't use EXEC_SUFFIX here because we want to clean everything.
clean:
	rm -rf $(PROGRAM) $(PROGRAM).exe libflashrom.a $(filter-out Makefile.d, $(wildcard *.d *.o platform/*.d platform/*.o)) \
		man8 .doctrees $(PROGRAM).bash $(BUILD_DETAILS_FILE)
	@+$(MAKE) -C util/ich_descriptors_tool/ clean

install: $(PROGRAM)$(EXEC_SUFFIX) $(call has_dependency, $(HAS_SPHINXBUILD), man8/$(PROGRAM).8) $(PROGRAM).bash
	mkdir -p $(DESTDIR)$(PREFIX)/sbin
	$(INSTALL) -m 0755 $(PROGRAM)$(EXEC_SUFFIX) $(DESTDIR)$(PREFIX)/sbin
	mkdir -p $(DESTDIR)$(BASHCOMPDIR)
	$(INSTALL) -m 0644 $(PROGRAM).bash $(DESTDIR)$(BASHCOMPDIR)
ifeq ($(HAS_SPHINXBUILD), yes)
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -m 0644 man8/$(PROGRAM).8 $(DESTDIR)$(MANDIR)/man8
endif

libinstall: libflashrom.a include/libflashrom.h
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	$(INSTALL) -m 0644 libflashrom.a $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/include
	$(INSTALL) -m 0644 include/libflashrom.h $(DESTDIR)$(PREFIX)/include

_export: man8/$(PROGRAM).8
	@rm -rf "$(EXPORTDIR)/flashrom-$(RELEASENAME)"
	@mkdir -p "$(EXPORTDIR)/flashrom-$(RELEASENAME)"
	@git archive HEAD | tar -x -C "$(EXPORTDIR)/flashrom-$(RELEASENAME)"
#	Generate versioninfo.inc containing metadata that would not be available in exported sources otherwise.
	@echo "VERSION = $(VERSION)" > "$(EXPORTDIR)/flashrom-$(RELEASENAME)/versioninfo.inc"
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


# TAROPTIONS reduces information leakage from the packager's system.
# If other tar programs support command line arguments for setting uid/gid of
# stored files, they can be handled here as well.
TAROPTIONS = $(shell LC_ALL=C tar --version|grep -q GNU && echo "--owner=root --group=root")

tarball: _export
	@tar -cj --format=ustar -f "$(EXPORTDIR)/flashrom-$(RELEASENAME).tar.bz2" -C $(EXPORTDIR)/ \
		$(TAROPTIONS) "flashrom-$(RELEASENAME)/"
#	Delete the exported directory again because it is most likely what's expected by the user.
	@rm -rf "$(EXPORTDIR)/flashrom-$(RELEASENAME)"
	@echo Created "$(EXPORTDIR)/flashrom-$(RELEASENAME).tar.bz2"

libpayload: clean
	make CC="CC=i386-elf-gcc lpgcc" AR=i386-elf-ar RANLIB=i386-elf-ranlib

.PHONY: all install clean distclean config _export export tarball libpayload

# Disable implicit suffixes and built-in rules (for performance and profit)
.SUFFIXES:

-include $(OBJS:.o=.d)
