#
# This file is part of the flashrom project.
#
# Copyright (C) 2005 coresystems GmbH <stepan@coresystems.de>
# Copyright (C) 2009,2010 Carl-Daniel Hailfinger
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

CC      ?= gcc
STRIP   ?= strip
INSTALL = install
DIFF    = diff
PREFIX  ?= /usr/local
MANDIR  ?= $(PREFIX)/share/man
CFLAGS  ?= -Os -Wall -Wshadow
EXPORTDIR ?= .

WARNERROR ?= yes

ifeq ($(WARNERROR), yes)
CFLAGS += -Werror
endif

# FIXME We have to differentiate between host and target arch.
OS_ARCH	?= $(shell uname)
ifneq ($(OS_ARCH), SunOS)
STRIP_ARGS = -s
endif
ifeq ($(OS_ARCH), Darwin)
CPPFLAGS += -I/opt/local/include -I/usr/local/include
LDFLAGS += -framework IOKit -framework DirectIO -L/opt/local/lib -L/usr/local/lib
endif
ifeq ($(OS_ARCH), FreeBSD)
CPPFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib
endif
ifeq ($(OS_ARCH), DOS)
CPPFLAGS += -I../libgetopt -I../libpci/include
# Bus Pirate and Serprog are not supported under DOS.
CONFIG_BUSPIRATE_SPI = no
CONFIG_SERPROG = no
endif

CHIP_OBJS = jedec.o stm50flw0x0x.o w39v040c.o w39v080fa.o w29ee011.o \
	sst28sf040.o m29f400bt.o 82802ab.o pm49fl00x.o \
	sst49lfxxxc.o sst_fwhub.o flashchips.o spi.o spi25.o

LIB_OBJS = layout.o

CLI_OBJS = flashrom.o cli_classic.o cli_output.o print.o

PROGRAMMER_OBJS = udelay.o programmer.o

all: pciutils features $(PROGRAM)

# Set the flashrom version string from the highest revision number
# of the checked out flashrom files.
# Note to packagers: Any tree exported with "make export" or "make tarball"
# will not require subversion. The downloadable snapshots are already exported.
SVNVERSION := $(shell LC_ALL=C svnversion -cn . 2>/dev/null | sed -e "s/.*://" -e "s/\([0-9]*\).*/\1/" | grep "[0-9]" || LC_ALL=C svn info . 2>/dev/null | awk '/^Revision:/ {print $$2 }' | grep "[0-9]" || LC_ALL=C git svn info . 2>/dev/null | awk '/^Revision:/ {print $$2 }' | grep "[0-9]" || echo unknown)

RELEASE := 0.9.2
VERSION := $(RELEASE)-r$(SVNVERSION)
RELEASENAME ?= $(VERSION)

SVNDEF := -D'FLASHROM_VERSION="$(VERSION)"'

# Always enable internal/onboard support for now.
CONFIG_INTERNAL ?= yes

# Always enable serprog for now. Needs to be disabled on Windows.
CONFIG_SERPROG ?= yes

# Bitbanging SPI infrastructure is not used yet.
CONFIG_BITBANG_SPI ?= no

# Always enable 3Com NICs for now.
CONFIG_NIC3COM ?= yes

# Disable NVIDIA graphics cards for now, write/erase don't work properly.
CONFIG_GFXNVIDIA ?= no

# Always enable SiI SATA controllers for now.
CONFIG_SATASII ?= yes

# Highpoint (HPT) ATA/RAID controller support.
# IMPORTANT: This code is not yet working!
CONFIG_ATAHPT ?= no

# Always enable FT2232 SPI dongles for now.
CONFIG_FT2232_SPI ?= yes

# Always enable dummy tracing for now.
CONFIG_DUMMY ?= yes

# Always enable Dr. Kaiser for now.
CONFIG_DRKAISER ?= yes

# Always enable Realtek NICs for now.
CONFIG_NICREALTEK ?= yes

# Always enable Bus Pirate SPI for now.
CONFIG_BUSPIRATE_SPI ?= yes

# Disable Dediprog SF100 until support is complete and tested.
CONFIG_DEDIPROG ?= no

# Disable wiki printing by default. It is only useful if you have wiki access.
CONFIG_PRINT_WIKI ?= no

ifeq ($(CONFIG_INTERNAL), yes)
FEATURE_CFLAGS += -D'CONFIG_INTERNAL=1'
PROGRAMMER_OBJS += processor_enable.o chipset_enable.o board_enable.o cbtable.o dmi.o internal.o
# FIXME: The PROGRAMMER_OBJS below should only be included on x86.
PROGRAMMER_OBJS += it87spi.o ichspi.o sb600spi.o wbsio_spi.o
NEED_PCI := yes
endif

ifeq ($(CONFIG_SERPROG), yes)
FEATURE_CFLAGS += -D'CONFIG_SERPROG=1'
PROGRAMMER_OBJS += serprog.o
ifeq ($(OS_ARCH), SunOS)
LIBS += -lsocket
endif
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

ifeq ($(CONFIG_FT2232_SPI), yes)
FTDILIBS := $(shell pkg-config --libs libftdi 2>/dev/null || printf "%s" "-lftdi -lusb")
# This is a totally ugly hack.
FEATURE_CFLAGS += $(shell LC_ALL=C grep -q "FTDISUPPORT := yes" .features && printf "%s" "-D'CONFIG_FT2232_SPI=1'")
FEATURE_LIBS += $(shell LC_ALL=C grep -q "FTDISUPPORT := yes" .features && printf "%s" "$(FTDILIBS)")
PROGRAMMER_OBJS += ft2232_spi.o
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

ifeq ($(CONFIG_BUSPIRATE_SPI), yes)
FEATURE_CFLAGS += -D'CONFIG_BUSPIRATE_SPI=1'
PROGRAMMER_OBJS += buspirate_spi.o
endif

ifeq ($(CONFIG_DEDIPROG), yes)
FEATURE_CFLAGS += -D'CONFIG_DEDIPROG=1'
FEATURE_LIBS += -lusb
PROGRAMMER_OBJS += dediprog.o
endif

# Ugly, but there's no elif/elseif.
ifeq ($(CONFIG_SERPROG), yes)
LIB_OBJS += serial.o
else
ifeq ($(CONFIG_BUSPIRATE_SPI), yes)
LIB_OBJS += serial.o
endif
endif

ifeq ($(NEED_PCI), yes)
CHECK_LIBPCI = yes
endif

ifeq ($(NEED_PCI), yes)
FEATURE_CFLAGS += -D'NEED_PCI=1'
PROGRAMMER_OBJS += pcidev.o physmap.o hwaccess.o
ifeq ($(OS_ARCH), NetBSD)
# The libpci we want is called libpciutils on NetBSD and needs NetBSD libpci.
LIBS += -lpciutils -lpci
# For (i386|x86_64)_iopl(2).
LIBS += -l$(shell uname -p)
else
ifeq ($(OS_ARCH), DOS)
# FIXME There needs to be a better way to do this
LIBS += ../libpci/lib/libpci.a ../libgetopt/libgetopt.a
else
LIBS += -lpci
endif
endif
endif

ifeq ($(CONFIG_PRINT_WIKI), yes)
FEATURE_CFLAGS += -D'CONFIG_PRINT_WIKI=1'
CLI_OBJS += print_wiki.o
endif

FEATURE_CFLAGS += $(shell LC_ALL=C grep -q "UTSNAME := yes" .features && printf "%s" "-D'HAVE_UTSNAME=1'")

# We could use PULLED_IN_LIBS, but that would be ugly.
FEATURE_LIBS += $(shell LC_ALL=C grep -q "NEEDLIBZ := yes" .libdeps && printf "%s" "-lz")

OBJS = $(CHIP_OBJS) $(CLI_OBJS) $(PROGRAMMER_OBJS) $(LIB_OBJS)

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROGRAM) $(OBJS) $(FEATURE_LIBS) $(LIBS)

# TAROPTIONS reduces information leakage from the packager's system.
# If other tar programs support command line arguments for setting uid/gid of
# stored files, they can be handled here as well.
TAROPTIONS = $(shell LC_ALL=C tar --version|grep -q GNU && echo "--owner=root --group=root")

%.o: %.c .features
	$(CC) -MMD $(CFLAGS) $(CPPFLAGS) $(FEATURE_CFLAGS) $(SVNDEF) -o $@ -c $<

# Make sure to add all names of generated binaries here.
# This includes all frontends and libflashrom.
clean:
	rm -f $(PROGRAM) $(PROGRAM).exe *.o *.d

distclean: clean
	rm -f .features .libdeps

strip: $(PROGRAM)
	$(STRIP) $(STRIP_ARGS) $(PROGRAM)

compiler:
	@printf "Checking for a C compiler... "
	@$(shell ( echo "int main(int argc, char **argv)"; \
		   echo "{ return 0; }"; ) > .test.c )
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .test.c -o .test >/dev/null &&	\
		echo "found." || ( echo "not found."; \
		rm -f .test.c .test; exit 1)
	@rm -f .test.c .test

ifeq ($(CHECK_LIBPCI), yes)
pciutils: compiler
	@printf "Checking for libpci headers... "
	@$(shell ( echo "#include <pci/pci.h>";		   \
		   echo "struct pci_access *pacc;";	   \
		   echo "int main(int argc, char **argv)"; \
		   echo "{ pacc = pci_alloc(); return 0; }"; ) > .test.c )
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) .test.c -o .test.o >/dev/null 2>&1 &&		\
		echo "found." || ( echo "not found."; echo;			\
		echo "Please install libpci headers (package pciutils-devel).";	\
		echo "See README for more information."; echo;			\
		rm -f .test.c .test.o; exit 1)
	@printf "Checking if libpci is present and sufficient... "
	@printf "" > .libdeps
	@$(CC) $(LDFLAGS) .test.o -o .test $(LIBS) >/dev/null 2>&1 &&				\
		echo "yes." || ( echo "no.";							\
		printf "Checking if libz+libpci are present and sufficient...";	\
		$(CC) $(LDFLAGS) .test.o -o .test $(LIBS) -lz >/dev/null 2>&1 &&		\
		( echo "yes."; echo "NEEDLIBZ := yes" > .libdeps ) || ( echo "no."; echo;	\
		echo "Please install libpci (package pciutils) and/or libz.";			\
		echo "See README for more information."; echo;				\
		rm -f .test.c .test.o .test; exit 1) )
	@rm -f .test.c .test.o .test
else
pciutils: compiler
	@printf "" > .libdeps
endif

.features: features

ifeq ($(CONFIG_FT2232_SPI), yes)
features: compiler
	@echo "FEATURES := yes" > .features.tmp
	@printf "Checking for FTDI support... "
	@$(shell ( echo "#include <ftdi.h>";		   \
		   echo "struct ftdi_context *ftdic = NULL;";	   \
		   echo "int main(int argc, char **argv)"; \
		   echo "{ return ftdi_init(ftdic); }"; ) > .featuretest.c )
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest $(FTDILIBS) $(LIBS) >/dev/null 2>&1 &&	\
		( echo "found."; echo "FTDISUPPORT := yes" >> .features.tmp ) ||	\
		( echo "not found."; echo "FTDISUPPORT := no" >> .features.tmp )
	@printf "Checking for utsname support... "
	@$(shell ( echo "#include <sys/utsname.h>";		   \
		   echo "struct utsname osinfo;";	   \
		   echo "int main(int argc, char **argv)"; \
		   echo "{ uname (&osinfo); return 0; }"; ) > .featuretest.c )
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest >/dev/null 2>&1 &&	\
		( echo "found."; echo "UTSNAME := yes" >> .features.tmp ) ||	\
		( echo "not found."; echo "UTSNAME := no" >> .features.tmp )
	@$(DIFF) -q .features.tmp .features >/dev/null 2>&1 && rm .features.tmp || mv .features.tmp .features
	@rm -f .featuretest.c .featuretest
else
features: compiler
	@echo "FEATURES := yes" > .features.tmp
	@printf "Checking for utsname support... "
	@$(shell ( echo "#include <sys/utsname.h>";		   \
		   echo "struct utsname osinfo;";	   \
		   echo "int main(int argc, char **argv)"; \
		   echo "{ uname (&osinfo); return 0; }"; ) > .featuretest.c )
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest >/dev/null 2>&1 &&	\
		( echo "found."; echo "UTSNAME := yes" >> .features.tmp ) ||	\
		( echo "not found."; echo "UTSNAME := no" >> .features.tmp )
	@$(DIFF) -q .features.tmp .features >/dev/null 2>&1 && rm .features.tmp || mv .features.tmp .features
	@rm -f .featuretest.c .featuretest
endif

install: $(PROGRAM)
	mkdir -p $(DESTDIR)$(PREFIX)/sbin
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -m 0755 $(PROGRAM) $(DESTDIR)$(PREFIX)/sbin
	$(INSTALL) -m 0644 $(PROGRAM).8 $(DESTDIR)$(MANDIR)/man8

export:
	@rm -rf $(EXPORTDIR)/flashrom-$(RELEASENAME)
	@svn export -r BASE . $(EXPORTDIR)/flashrom-$(RELEASENAME)
	@sed "s/^SVNVERSION.*/SVNVERSION := $(SVNVERSION)/" Makefile >$(EXPORTDIR)/flashrom-$(RELEASENAME)/Makefile
	@LC_ALL=C svn log >$(EXPORTDIR)/flashrom-$(RELEASENAME)/ChangeLog
	@echo Exported $(EXPORTDIR)/flashrom-$(RELEASENAME)/

tarball: export
	@tar cjf $(EXPORTDIR)/flashrom-$(RELEASENAME).tar.bz2 -C $(EXPORTDIR)/ $(TAROPTIONS) flashrom-$(RELEASENAME)/
	@rm -rf $(EXPORTDIR)/flashrom-$(RELEASENAME)
	@echo Created $(EXPORTDIR)/flashrom-$(RELEASENAME).tar.bz2

djgpp-dos: clean
	make CC=i586-pc-msdosdjgpp-gcc STRIP=i586-pc-msdosdjgpp-strip WARNERROR=no OS_ARCH=DOS

.PHONY: all clean distclean compiler pciutils features export tarball dos

-include $(OBJS:.o=.d)
