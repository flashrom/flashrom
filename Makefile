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
	dummyflasher.o pcidev.o nic3com.o satasii.o

all: pciutils dep $(PROGRAM)

# Set the flashrom version string from the highest revision number
# of the checked out flashrom files.
# Note to packagers: Any tree exported with "make export" or "make tarball"
# will not require subversion. The downloadable snapshots are already exported.
SVNVERSION := $(shell LANG=C svnversion -cn . | sed -e "s/.*://" -e "s/\([0-9]*\).*/\1/" | grep "[0-9]" || echo unknown)

VERSION := 0.9.0-r$(SVNVERSION)

SVNDEF := -D'FLASHROM_VERSION="$(VERSION)"'

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROGRAM) $(OBJS) $(LIBS)

flashrom.o: flashrom.c
	$(CC) $(CFLAGS) $(CPPFLAGS)  -c -o $@ $< $(SVNDEF)

clean:
	rm -f $(PROGRAM) *.o

distclean: clean
	rm -f .dependencies

dep:
	@$(CC) $(CPPFLAGS) $(SVNDEF) -MM *.c > .dependencies

strip: $(PROGRAM)
	$(STRIP) $(STRIP_ARGS) $(PROGRAM)

compiler:
	@echo; printf "Checking for a C compiler... "
	@$(shell ( echo "int main(int argc, char **argv)"; \
		   echo "{ return 0; }"; ) > .test.c )
	@$(CC) $(CFLAGS) $(LDFLAGS) .test.c -o .test >/dev/null &&	\
		echo "found." || ( echo "not found."; \
		rm -f .test.c .test; exit 1)
	@rm -f .test.c .test

pciutils: compiler
	@echo; printf "Checking for pciutils and zlib... "
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

install: $(PROGRAM)
	mkdir -p $(DESTDIR)$(PREFIX)/sbin
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	$(INSTALL) -m 0755 $(PROGRAM) $(DESTDIR)$(PREFIX)/sbin
	$(INSTALL) -m 0644 $(PROGRAM).8 $(DESTDIR)$(MANDIR)/man8

export:
	@rm -rf $(EXPORTDIR)/flashrom-$(VERSION)
	@svn export -r BASE . $(EXPORTDIR)/flashrom-$(VERSION)
	@sed "s/^SVNVERSION.*/SVNVERSION := $(SVNVERSION)/" Makefile >$(EXPORTDIR)/flashrom-$(VERSION)/Makefile
	@echo Exported $(EXPORTDIR)/flashrom-$(VERSION)/

tarball: export
	@tar cfz $(EXPORTDIR)/flashrom-$(VERSION).tar.gz -C $(EXPORTDIR)/ flashrom-$(VERSION)/
	@rm -rf $(EXPORTDIR)/flashrom-$(VERSION)
	@echo Created $(EXPORTDIR)/flashrom-$(VERSION).tar.gz

.PHONY: all clean distclean dep compiler pciutils export tarball

-include .dependencies
