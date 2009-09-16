#
# This file is part of the flashrom project.
#
# Copyright (C) 2005 coresystems GmbH <stepan@coresystems.de>
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
STRIP   = strip
INSTALL = install
DIFF    = diff
PREFIX  ?= /usr/local
MANDIR  ?= $(PREFIX)/share/man
CFLAGS  ?= -Os -Wall -Werror
EXPORTDIR ?= .

OS_ARCH	= $(shell uname)
ifneq ($(OS_ARCH), SunOS)
STRIP_ARGS = -s
endif
ifeq ($(OS_ARCH), Darwin)
CFLAGS += -I/usr/local/include
LDFLAGS += -framework IOKit -framework DirectIO -L/usr/local/lib
endif
ifeq ($(OS_ARCH), FreeBSD)
CFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib
endif

LIBS += -lpci -lz

OBJS = chipset_enable.o board_enable.o udelay.o jedec.o stm50flw0x0x.o \
	sst28sf040.o am29f040b.o mx29f002.o m29f400bt.o pm29f002.o \
	w49f002u.o 82802ab.o pm49fl00x.o sst49lf040.o en29f002a.o \
	sst49lfxxxc.o sst_fwhub.o layout.o cbtable.o flashchips.o physmap.o \
	flashrom.o w39v080fa.o sharplhf00l04.o w29ee011.o spi.o it87spi.o \
	ichspi.o w39v040c.o sb600spi.o wbsio_spi.o m29f002.o internal.o \
	pcidev.o print.o

all: pciutils features dep $(PROGRAM)

# Set the flashrom version string from the highest revision number
# of the checked out flashrom files.
# Note to packagers: Any tree exported with "make export" or "make tarball"
# will not require subversion. The downloadable snapshots are already exported.
SVNVERSION := $(shell LC_ALL=C svnversion -cn . | sed -e "s/.*://" -e "s/\([0-9]*\).*/\1/" | grep "[0-9]" || LC_ALL=C svn info . | grep ^Revision | sed "s/.*[[:blank:]]\+\([0-9]*\)[^0-9]*/\1/" | grep "[0-9]" || echo unknown)

RELEASE := 0.9.1
VERSION := $(RELEASE)-r$(SVNVERSION)
RELEASENAME ?= $(VERSION)

SVNDEF := -D'FLASHROM_VERSION="$(VERSION)"'

# Always enable serprog for now. Needs to be disabled on Windows.
CONFIG_SERPROG ?= yes

# Always enable 3Com NICs for now.
CONFIG_NIC3COM ?= yes

# Always enable SiI SATA controllers for now.
CONFIG_SATASII ?= yes

# Always enable FT2232 SPI dongles for now.
CONFIG_FT2232SPI ?= yes

# Always enable dummy tracing for now.
CONFIG_DUMMY ?= yes

# Always enable Dr. Kaiser for now.
CONFIG_DRKAISER ?= yes

# Always enable wiki printing for now.
CONFIG_PRINT_WIKI ?= no

ifeq ($(CONFIG_SERPROG), yes)
FEATURE_CFLAGS += -D'SERPROG_SUPPORT=1'
OBJS += serprog.o
ifeq ($(OS_ARCH), SunOS)
LIBS += -lsocket
endif
endif

ifeq ($(CONFIG_NIC3COM), yes)
FEATURE_CFLAGS += -D'NIC3COM_SUPPORT=1'
OBJS += nic3com.o
endif

ifeq ($(CONFIG_SATASII), yes)
FEATURE_CFLAGS += -D'SATASII_SUPPORT=1'
OBJS += satasii.o
endif

ifeq ($(CONFIG_FT2232SPI), yes)
# This is a totally ugly hack.
FEATURE_CFLAGS += $(shell LC_ALL=C grep -q "FTDISUPPORT := yes" .features && printf "%s" "-D'FT2232_SPI_SUPPORT=1'")
FEATURE_LIBS += $(shell LC_ALL=C grep -q "FTDISUPPORT := yes" .features && printf "%s" "-lftdi")
OBJS += ft2232_spi.o
endif

ifeq ($(CONFIG_DUMMY), yes)
FEATURE_CFLAGS += -D'DUMMY_SUPPORT=1'
OBJS += dummyflasher.o
endif

ifeq ($(CONFIG_DRKAISER), yes)
FEATURE_CFLAGS += -D'DRKAISER_SUPPORT=1'
OBJS += drkaiser.o
endif

ifeq ($(CONFIG_PRINT_WIKI), yes)
FEATURE_CFLAGS += -D'PRINT_WIKI_SUPPORT=1'
OBJS += print_wiki.o
endif

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROGRAM) $(OBJS) $(LIBS) $(FEATURE_LIBS)

# TAROPTIONS reduces information leakage from the packager's system.
# If other tar programs support command line arguments for setting uid/gid of
# stored files, they can be handled here as well.
TAROPTIONS = $(shell LC_ALL=C tar --version|grep -q GNU && echo "--owner=root --group=root")

%.o: %.c .features
	$(CC) $(CFLAGS) $(CPPFLAGS) $(FEATURE_CFLAGS) $(SVNDEF) -o $@ -c $<

clean:
	rm -f $(PROGRAM) *.o

distclean: clean
	rm -f .dependencies .features

dep:
	@$(CC) $(CPPFLAGS) $(SVNDEF) -MM *.c > .dependencies

strip: $(PROGRAM)
	$(STRIP) $(STRIP_ARGS) $(PROGRAM)

compiler:
	@printf "Checking for a C compiler... "
	@$(shell ( echo "int main(int argc, char **argv)"; \
		   echo "{ return 0; }"; ) > .test.c )
	@$(CC) $(CFLAGS) $(LDFLAGS) .test.c -o .test >/dev/null &&	\
		echo "found." || ( echo "not found."; \
		rm -f .test.c .test; exit 1)
	@rm -f .test.c .test

pciutils: compiler
	@printf "Checking for pciutils and zlib... "
	@$(shell ( echo "#include <pci/pci.h>";		   \
		   echo "struct pci_access *pacc;";	   \
		   echo "int main(int argc, char **argv)"; \
		   echo "{ pacc = pci_alloc(); return 0; }"; ) > .test.c )
	@$(CC) $(CFLAGS) $(LDFLAGS) .test.c -o .test $(LIBS) >/dev/null 2>&1 &&	\
		echo "found." || ( echo "not found."; echo;		\
		echo "Please install pciutils-devel and zlib-devel.";	\
		echo "See README for more information."; echo;		\
		rm -f .test.c .test; exit 1)
	@rm -f .test.c .test

.features: features

features: compiler
	@echo "FEATURES := yes" > .features.tmp
	@printf "Checking for FTDI support... "
	@$(shell ( echo "#include <ftdi.h>";		   \
		   echo "struct ftdi_context *ftdic = NULL;";	   \
		   echo "int main(int argc, char **argv)"; \
		   echo "{ return ftdi_init(ftdic); }"; ) > .featuretest.c )
	@$(CC) $(CFLAGS) $(LDFLAGS) .featuretest.c -o .featuretest $(LIBS) -lftdi >/dev/null 2>&1 &&	\
		( echo "found."; echo "FTDISUPPORT := yes" >> .features.tmp ) ||	\
		( echo "not found."; echo "FTDISUPPORT := no" >> .features.tmp )
	@$(DIFF) -q .features.tmp .features >/dev/null 2>&1 && rm .features.tmp || mv .features.tmp .features
	@rm -f .featuretest.c .featuretest

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

.PHONY: all clean distclean dep compiler pciutils features export tarball

-include .dependencies
