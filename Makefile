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
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
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
CPPFLAGS += -I$(LIBS_BASE)/include
LDFLAGS += -L$(LIBS_BASE)/lib -Wl,-rpath -Wl,$(LIBS_BASE)/lib
PKG_CONFIG_LIBDIR ?= $(LIBS_BASE)/lib/pkgconfig
endif

###############################################################################
# General OS-specific settings.
# 1. Prepare for later by gathering information about host and target OS
# 2. Set compiler flags and parameters according to OSes
# 3. Likewise verify user-supplied CONFIG_* variables.

# HOST_OS is only used to work around local toolchain issues.
HOST_OS ?= $(shell uname)
ifeq ($(HOST_OS), MINGW32_NT-5.1)
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
override TARGET_OS := $(strip $(shell LC_ALL=C $(CC) $(CPPFLAGS) -E os.h 2>/dev/null | grep -v '^\#' | grep '"' | cut -f 2 -d'"'))

ifeq ($(TARGET_OS), Darwin)
CPPFLAGS += -I/opt/local/include -I/usr/local/include
LDFLAGS += -L/opt/local/lib -L/usr/local/lib
endif

ifeq ($(TARGET_OS), FreeBSD)
CPPFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib
endif

ifeq ($(TARGET_OS), OpenBSD)
CPPFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib
endif

ifeq ($(TARGET_OS), NetBSD)
CPPFLAGS += -I/usr/pkg/include
LDFLAGS += -L/usr/pkg/lib
endif

ifeq ($(TARGET_OS), DragonFlyBSD)
CPPFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib
endif

ifeq ($(TARGET_OS), DOS)
EXEC_SUFFIX := .exe
# DJGPP has odd uint*_t definitions which cause lots of format string warnings.
CFLAGS += -Wno-format
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
# Dediprog, USB-Blaster, PICkit2 and FT2232 are not supported under DOS (missing USB support).
ifeq ($(CONFIG_DEDIPROG), yes)
UNSUPPORTED_FEATURES += CONFIG_DEDIPROG=yes
else
override CONFIG_DEDIPROG = no
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
endif

# FIXME: Should we check for Cygwin/MSVC as well?
ifeq ($(TARGET_OS), MinGW)
EXEC_SUFFIX := .exe
# MinGW doesn't have the ffs() function, but we can use gcc's __builtin_ffs().
FLASHROM_CFLAGS += -Dffs=__builtin_ffs
# Some functions provided by Microsoft do not work as described in C99 specifications. This macro fixes that
# for MinGW. See http://sourceforge.net/apps/trac/mingw-w64/wiki/printf%20and%20scanf%20family */
FLASHROM_CFLAGS += -D__USE_MINGW_ANSI_STDIO=1
# libusb-win32/libftdi stuff is usually installed in /usr/local.
CPPFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib
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
# Dediprog, USB-Blaster, PICkit2 and FT2232 are not supported with libpayload (missing libusb support)
ifeq ($(CONFIG_DEDIPROG), yes)
UNSUPPORTED_FEATURES += CONFIG_DEDIPROG=yes
else
override CONFIG_DEDIPROG = no
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
endif

ifneq ($(TARGET_OS), Linux)
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

###############################################################################
# General architecture-specific settings.
# Like above for the OS, below we verify user-supplied options depending on the target architecture.

# Determine the destination processor architecture.
# IMPORTANT: The following line must be placed before ARCH is ever used
# (of course), but should come after any lines setting CC because the line
# below uses CC itself.
override ARCH := $(strip $(shell LC_ALL=C $(CC) $(CPPFLAGS) -E archtest.c 2>/dev/null | grep -v '^\#' | grep '"' | cut -f 2 -d'"'))

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
ifeq ($(CONFIG_SATAMV), yes)
UNSUPPORTED_FEATURES += CONFIG_SATAMV=yes
else
override CONFIG_SATAMV = no
endif
endif

###############################################################################
# Flash chip drivers and bus support infrastructure.

CHIP_OBJS = jedec.o stm50.o w39.o w29ee011.o \
	sst28sf040.o 82802ab.o \
	sst49lfxxxc.o sst_fwhub.o flashchips.o spi.o spi25.o spi25_statusreg.o \
	opaque.o sfdp.o en29lv640b.o at45db.o

###############################################################################
# Library code.

LIB_OBJS = layout.o flashrom.o udelay.o programmer.o helpers.o

###############################################################################
# Frontend related stuff.

CLI_OBJS = cli_classic.o cli_output.o cli_common.o print.o

# Set the flashrom version string from the highest revision number of the checked out flashrom files.
# Note to packagers: Any tree exported with "make export" or "make tarball"
# will not require subversion. The downloadable snapshots are already exported.
SVNVERSION := $(shell ./util/getrevision.sh -u 2>/dev/null )

RELEASE := 0.9.8
VERSION := $(RELEASE)-$(SVNVERSION)
RELEASENAME ?= $(VERSION)

SVNDEF := -D'FLASHROM_VERSION="$(VERSION)"'

# Inform user if there is no meaningful version string. If there is version information from a VCS print
# something anyway because $(info...) will print a line break in any case which would look suspicious.
# The && between the echos is a workaround for old versions of GNU make that issue the error "unterminated
# variable reference" if a semicolon is used instead.
$(info $(shell ./util/getrevision.sh -c 2>/dev/null || echo "Files don't seem to be under version control." && \
	echo "Replacing all version templates with $(VERSION)." ))

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

# Disable Dediprog SF100 until support is complete and tested.
CONFIG_DEDIPROG ?= no

# Always enable Marvell SATA controllers for now.
CONFIG_SATAMV ?= yes

# Enable Linux spidev interface by default. We disable it on non-Linux targets.
CONFIG_LINUX_SPI ?= yes

# Always enable ITE IT8212F PATA controllers for now.
CONFIG_IT8212 ?= yes

# Disable wiki printing by default. It is only useful if you have wiki access.
CONFIG_PRINT_WIKI ?= no

# Enable all features if CONFIG_EVERYTHING=yes is given
ifeq ($(CONFIG_EVERYTHING), yes)
$(foreach var, $(filter CONFIG_%, $(.VARIABLES)),\
	$(if $(filter no, $($(var))),\
		$(eval $(var)=yes)))
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
PROGRAMMER_OBJS += ichspi.o ich_descriptors.o dmi.o
ifeq ($(CONFIG_INTERNAL_DMI), yes)
FEATURE_CFLAGS += -D'CONFIG_INTERNAL_DMI=1'
endif
else
endif
NEED_PCI := yes
endif

ifeq ($(CONFIG_SERPROG), yes)
FEATURE_CFLAGS += -D'CONFIG_SERPROG=1'
PROGRAMMER_OBJS += serprog.o
NEED_SERIAL := yes
NEED_NET := yes
endif

ifeq ($(CONFIG_RAYER_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_RAYER_SPI=1'
PROGRAMMER_OBJS += rayer_spi.o
# Actually, NEED_PCI is wrong. NEED_IOPORT_ACCESS would be more correct.
NEED_PCI := yes
endif

ifeq ($(CONFIG_PONY_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_PONY_SPI=1'
PROGRAMMER_OBJS += pony_spi.o
NEED_SERIAL := yes
endif

ifeq ($(CONFIG_BITBANG_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_BITBANG_SPI=1'
PROGRAMMER_OBJS += bitbang_spi.o
endif

ifeq ($(CONFIG_NIC3COM), yes)
FEATURE_CFLAGS += -D'CONFIG_NIC3COM=1'
PROGRAMMER_OBJS += nic3com.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_GFXNVIDIA), yes)
FEATURE_CFLAGS += -D'CONFIG_GFXNVIDIA=1'
PROGRAMMER_OBJS += gfxnvidia.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_SATASII), yes)
FEATURE_CFLAGS += -D'CONFIG_SATASII=1'
PROGRAMMER_OBJS += satasii.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_ATAHPT), yes)
FEATURE_CFLAGS += -D'CONFIG_ATAHPT=1'
PROGRAMMER_OBJS += atahpt.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_ATAVIA), yes)
FEATURE_CFLAGS += -D'CONFIG_ATAVIA=1'
PROGRAMMER_OBJS += atavia.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_IT8212), yes)
FEATURE_CFLAGS += -D'CONFIG_IT8212=1'
PROGRAMMER_OBJS += it8212.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_FT2232_SPI), yes)
# This is a totally ugly hack.
FEATURE_CFLAGS += $(shell LC_ALL=C grep -q "FTDISUPPORT := yes" .features && printf "%s" "-D'CONFIG_FT2232_SPI=1'")
NEED_FTDI := yes
PROGRAMMER_OBJS += ft2232_spi.o
endif

ifeq ($(CONFIG_USBBLASTER_SPI), yes)
# This is a totally ugly hack.
FEATURE_CFLAGS += $(shell LC_ALL=C grep -q "FTDISUPPORT := yes" .features && printf "%s" "-D'CONFIG_USBBLASTER_SPI=1'")
NEED_FTDI := yes
PROGRAMMER_OBJS += usbblaster_spi.o
endif

ifeq ($(CONFIG_PICKIT2_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_PICKIT2_SPI=1'
PROGRAMMER_OBJS += pickit2_spi.o
NEED_USB := yes
endif

ifeq ($(NEED_FTDI), yes)
FTDILIBS := $(shell ([ -n "$(PKG_CONFIG_LIBDIR)" ] && export PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)" ); pkg-config --libs libftdi || printf "%s" "-lftdi -lusb")
FEATURE_CFLAGS += $(shell LC_ALL=C grep -q "FT232H := yes" .features && printf "%s" "-D'HAVE_FT232H=1'")
FEATURE_LIBS += $(shell LC_ALL=C grep -q "FTDISUPPORT := yes" .features && printf "%s" "$(FTDILIBS)")
# We can't set NEED_USB here because that would transform libftdi auto-enabling
# into a hard requirement for libusb, defeating the purpose of auto-enabling.
endif

ifeq ($(CONFIG_DUMMY), yes)
FEATURE_CFLAGS += -D'CONFIG_DUMMY=1'
PROGRAMMER_OBJS += dummyflasher.o
endif

ifeq ($(CONFIG_DRKAISER), yes)
FEATURE_CFLAGS += -D'CONFIG_DRKAISER=1'
PROGRAMMER_OBJS += drkaiser.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_NICREALTEK), yes)
FEATURE_CFLAGS += -D'CONFIG_NICREALTEK=1'
PROGRAMMER_OBJS += nicrealtek.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_NICNATSEMI), yes)
FEATURE_CFLAGS += -D'CONFIG_NICNATSEMI=1'
PROGRAMMER_OBJS += nicnatsemi.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_NICINTEL), yes)
FEATURE_CFLAGS += -D'CONFIG_NICINTEL=1'
PROGRAMMER_OBJS += nicintel.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_NICINTEL_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_NICINTEL_SPI=1'
PROGRAMMER_OBJS += nicintel_spi.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_NICINTEL_EEPROM), yes)
FEATURE_CFLAGS += -D'CONFIG_NICINTEL_EEPROM=1'
PROGRAMMER_OBJS += nicintel_eeprom.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_OGP_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_OGP_SPI=1'
PROGRAMMER_OBJS += ogp_spi.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_BUSPIRATE_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_BUSPIRATE_SPI=1'
PROGRAMMER_OBJS += buspirate_spi.o
NEED_SERIAL := yes
endif

ifeq ($(CONFIG_DEDIPROG), yes)
FEATURE_CFLAGS += -D'CONFIG_DEDIPROG=1'
PROGRAMMER_OBJS += dediprog.o
NEED_USB := yes
endif

ifeq ($(CONFIG_SATAMV), yes)
FEATURE_CFLAGS += -D'CONFIG_SATAMV=1'
PROGRAMMER_OBJS += satamv.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_LINUX_SPI), yes)
# This is a totally ugly hack.
FEATURE_CFLAGS += $(shell LC_ALL=C grep -q "LINUX_SPI_SUPPORT := yes" .features && printf "%s" "-D'CONFIG_LINUX_SPI=1'")
PROGRAMMER_OBJS += linux_spi.o
endif

ifeq ($(CONFIG_MSTARDDC_SPI), yes)
# This is a totally ugly hack.
FEATURE_CFLAGS += $(shell LC_ALL=C grep -q "LINUX_I2C_SUPPORT := yes" .features && printf "%s" "-D'CONFIG_MSTARDDC_SPI=1'")
NEED_LINUX_I2C := yes
PROGRAMMER_OBJS += mstarddc_spi.o
endif

ifeq ($(NEED_SERIAL), yes)
LIB_OBJS += serial.o
endif

ifeq ($(NEED_NET), yes)
ifeq ($(TARGET_OS), SunOS)
LIBS += -lsocket
endif
endif

ifeq ($(NEED_PCI), yes)
CHECK_LIBPCI = yes
FEATURE_CFLAGS += -D'NEED_PCI=1'
FEATURE_CFLAGS += $(shell LC_ALL=C grep -q "OLD_PCI_GET_DEV := yes" .libdeps && printf "%s" "-D'OLD_PCI_GET_DEV=1'")

PROGRAMMER_OBJS += pcidev.o physmap.o hwaccess.o
ifeq ($(TARGET_OS), NetBSD)
# The libpci we want is called libpciutils on NetBSD and needs NetBSD libpci.
PCILIBS += -lpciutils -lpci
# For (i386|x86_64)_iopl(2).
PCILIBS += -l$(shell uname -p)
else

PCILIBS += -lpci

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

ifeq ($(NEED_USB), yes)
CHECK_LIBUSB0 = yes
FEATURE_CFLAGS += -D'NEED_USB=1'
USBLIBS := $(shell ([ -n "$(PKG_CONFIG_LIBDIR)" ] && export PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)" ); pkg-config --libs libusb  || printf "%s" "-lusb")
endif

ifeq ($(CONFIG_PRINT_WIKI), yes)
FEATURE_CFLAGS += -D'CONFIG_PRINT_WIKI=1'
CLI_OBJS += print_wiki.o
endif

FEATURE_CFLAGS += $(shell LC_ALL=C grep -q "UTSNAME := yes" .features && printf "%s" "-D'HAVE_UTSNAME=1'")

# We could use PULLED_IN_LIBS, but that would be ugly.
FEATURE_LIBS += $(shell LC_ALL=C grep -q "NEEDLIBZ := yes" .libdeps && printf "%s" "-lz")

LIBFLASHROM_OBJS = $(CHIP_OBJS) $(PROGRAMMER_OBJS) $(LIB_OBJS)
OBJS = $(CLI_OBJS) $(LIBFLASHROM_OBJS)

all: hwlibs features $(PROGRAM)$(EXEC_SUFFIX) $(PROGRAM).8
ifeq ($(ARCH), x86)
	@+$(MAKE) -C util/ich_descriptors_tool/ TARGET_OS=$(TARGET_OS) EXEC_SUFFIX=$(EXEC_SUFFIX)
endif

$(PROGRAM)$(EXEC_SUFFIX): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROGRAM)$(EXEC_SUFFIX) $(OBJS) $(LIBS) $(PCILIBS) $(FEATURE_LIBS) $(USBLIBS)

libflashrom.a: $(LIBFLASHROM_OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

# TAROPTIONS reduces information leakage from the packager's system.
# If other tar programs support command line arguments for setting uid/gid of
# stored files, they can be handled here as well.
TAROPTIONS = $(shell LC_ALL=C tar --version|grep -q GNU && echo "--owner=root --group=root")

%.o: %.c .features
	$(CC) -MMD $(CFLAGS) $(CPPFLAGS) $(FLASHROM_CFLAGS) $(FEATURE_CFLAGS) $(SVNDEF) -o $@ -c $<

# Make sure to add all names of generated binaries here.
# This includes all frontends and libflashrom.
# We don't use EXEC_SUFFIX here because we want to clean everything.
clean:
	rm -f $(PROGRAM) $(PROGRAM).exe libflashrom.a *.o *.d $(PROGRAM).8
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
	@printf "Checking for a C compiler... "
	@echo "$$COMPILER_TEST" > .test.c
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .test.c -o .test$(EXEC_SUFFIX) >/dev/null &&	\
		echo "found." || ( echo "not found."; \
		rm -f .test.c .test$(EXEC_SUFFIX); exit 1)
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

hwlibs: compiler
	@printf "" > .libdeps
ifeq ($(CHECK_LIBPCI), yes)
	@printf "Checking for libpci headers... "
	@echo "$$LIBPCI_TEST" > .test.c
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o >/dev/null &&		\
		echo "found." || ( echo "not found."; echo;			\
		echo "Please install libpci headers (package pciutils-devel).";	\
		echo "See README for more information."; echo;			\
		rm -f .test.c .test.o; exit 1)
	@printf "Checking version of pci_get_dev... "
	@echo "$$PCI_GET_DEV_TEST" > .test.c
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o >/dev/null 2>&1 &&	\
		( echo "new version (including PCI domain parameter)."; echo "OLD_PCI_GET_DEV := no" >> .libdeps ) ||	\
		( echo "old version (without PCI domain parameter)."; echo "OLD_PCI_GET_DEV := yes" >> .libdeps )
	@printf "Checking if libpci is present and sufficient... "
	@$(CC) $(LDFLAGS) .test.o -o .test$(EXEC_SUFFIX) $(LIBS) $(PCILIBS) >/dev/null &&		\
		echo "yes." || ( echo "no.";							\
		printf "Checking if libz+libpci are present and sufficient...";	\
		$(CC) $(LDFLAGS) .test.o -o .test$(EXEC_SUFFIX) $(LIBS) $(PCILIBS) -lz >/dev/null &&	\
		( echo "yes."; echo "NEEDLIBZ := yes" > .libdeps ) || ( echo "no."; echo;	\
		echo "Please install libpci (package pciutils) and/or libz.";			\
		echo "See README for more information."; echo;				\
		rm -f .test.c .test.o .test$(EXEC_SUFFIX); exit 1) )
	@rm -f .test.c .test.o .test$(EXEC_SUFFIX)
endif
ifeq ($(CHECK_LIBUSB0), yes)
	@printf "Checking for libusb-0.1/libusb-compat headers... "
	@echo "$$LIBUSB0_TEST" > .test.c
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o >/dev/null &&		\
		echo "found." || ( echo "not found."; echo;				\
		echo "Please install libusb-0.1 headers or libusb-compat headers.";	\
		echo "See README for more information."; echo;				\
		rm -f .test.c .test.o; exit 1)
	@printf "Checking if libusb-0.1 is usable... "
	@$(CC) $(LDFLAGS) .test.o -o .test$(EXEC_SUFFIX) $(LIBS) $(USBLIBS) >/dev/null &&	\
		echo "yes." || ( echo "no.";						\
		echo "Please install libusb-0.1 or libusb-compat.";			\
		echo "See README for more information."; echo;				\
		rm -f .test.c .test.o .test$(EXEC_SUFFIX); exit 1)
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

features: compiler
	@echo "FEATURES := yes" > .features.tmp
ifeq ($(NEED_FTDI), yes)
	@printf "Checking for FTDI support... "
	@echo "$$FTDI_TEST" > .featuretest.c
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX) $(FTDILIBS) $(LIBS) >/dev/null 2>&1 &&	\
		( echo "found."; echo "FTDISUPPORT := yes" >> .features.tmp ) ||	\
		( echo "not found."; echo "FTDISUPPORT := no" >> .features.tmp )
	@printf "Checking for FT232H support in libftdi... "
	@echo "$$FTDI_232H_TEST" >> .featuretest.c
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX) $(FTDILIBS) $(LIBS) >/dev/null 2>&1 &&	\
		( echo "found."; echo "FT232H := yes" >> .features.tmp ) ||	\
		( echo "not found."; echo "FT232H := no" >> .features.tmp )
endif
ifeq ($(CONFIG_LINUX_SPI), yes)
	@printf "Checking if Linux SPI headers are present... "
	@echo "$$LINUX_SPI_TEST" > .featuretest.c
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX) >/dev/null 2>&1 &&	\
		( echo "yes."; echo "LINUX_SPI_SUPPORT := yes" >> .features.tmp ) ||	\
		( echo "no."; echo "LINUX_SPI_SUPPORT := no" >> .features.tmp )
endif
ifeq ($(NEED_LINUX_I2C), yes)
	@printf "Checking if Linux I2C headers are present... "
	@echo "$$LINUX_I2C_TEST" > .featuretest.c
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX) >/dev/null 2>&1 &&	\
		( echo "yes."; echo "LINUX_I2C_SUPPORT := yes" >> .features.tmp ) ||	\
		( echo "no."; echo "LINUX_I2C_SUPPORT := no" >> .features.tmp )
endif
	@printf "Checking for utsname support... "
	@echo "$$UTSNAME_TEST" > .featuretest.c
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest$(EXEC_SUFFIX) >/dev/null 2>&1 &&	\
		( echo "found."; echo "UTSNAME := yes" >> .features.tmp ) ||	\
		( echo "not found."; echo "UTSNAME := no" >> .features.tmp )
	@$(DIFF) -q .features.tmp .features >/dev/null 2>&1 && rm .features.tmp || mv .features.tmp .features
	@rm -f .featuretest.c .featuretest$(EXEC_SUFFIX)

$(PROGRAM).8: $(PROGRAM).8.tmpl
	@sed -e '1 s#".*".*#"$(shell ./util/getrevision.sh -d $(PROGRAM).8.tmpl 2>/dev/null)" "$(VERSION)"#' <$< >$@

install: $(PROGRAM)$(EXEC_SUFFIX) $(PROGRAM).8
	mkdir -p $(DESTDIR)$(PREFIX)/sbin
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -m 0755 $(PROGRAM)$(EXEC_SUFFIX) $(DESTDIR)$(PREFIX)/sbin
	$(INSTALL) -m 0644 $(PROGRAM).8 $(DESTDIR)$(MANDIR)/man8

export: $(PROGRAM).8
	@rm -rf $(EXPORTDIR)/flashrom-$(RELEASENAME)
	@svn export -r BASE . $(EXPORTDIR)/flashrom-$(RELEASENAME)
	@sed "s/^SVNVERSION.*/SVNVERSION := $(SVNVERSION)/" Makefile >$(EXPORTDIR)/flashrom-$(RELEASENAME)/Makefile
	@cp $(PROGRAM).8 "$(EXPORTDIR)/flashrom-$(RELEASENAME)/$(PROGRAM).8"
	@LC_ALL=C svn log >$(EXPORTDIR)/flashrom-$(RELEASENAME)/ChangeLog
	@echo Exported $(EXPORTDIR)/flashrom-$(RELEASENAME)/

tarball: export
	@tar cjf $(EXPORTDIR)/flashrom-$(RELEASENAME).tar.bz2 -C $(EXPORTDIR)/ $(TAROPTIONS) flashrom-$(RELEASENAME)/
	@rm -rf $(EXPORTDIR)/flashrom-$(RELEASENAME)
	@echo Created $(EXPORTDIR)/flashrom-$(RELEASENAME).tar.bz2

djgpp-dos: clean
	make CC=i586-pc-msdosdjgpp-gcc STRIP=i586-pc-msdosdjgpp-strip
libpayload: clean
	make CC="CC=i386-elf-gcc lpgcc" AR=i386-elf-ar RANLIB=i386-elf-ranlib

.PHONY: all install clean distclean compiler hwlibs features export tarball djgpp-dos featuresavailable libpayload

-include $(OBJS:.o=.d)
