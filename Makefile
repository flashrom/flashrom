#
# Makefile for flashrom utility
# 
# redone by Stefan Reinauer <stepan@openbios.org>
#

PROGRAM = flashrom

CC     ?= gcc
STRIP	= strip
INSTALL = install
PREFIX  ?= /usr/local
CFLAGS  ?= -Os -Wall -Werror

prefix = $(DESTDIR)$(PREFIX)
man8dir = $(prefix)/share/man/man8
sbindir = $(prefix)/sbin

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
	sst28sf040.o am29f040b.o mx29f002.o m29f400bt.o \
	w49f002u.o 82802ab.o pm49fl00x.o sst49lf040.o en29f002a.o \
	sst49lfxxxc.o sst_fwhub.o layout.o cbtable.o flashchips.o physmap.o \
	flashrom.o w39v080fa.o sharplhf00l04.o w29ee011.o spi.o it87spi.o \
	ichspi.o w39v040c.o sb600spi.o wbsio_spi.o m29f002.o internal.o \
	dummyflasher.o nic3com.o

all: pciutils dep $(PROGRAM)

# Set the flashrom version string from the highest revision number
# of the checked out flashrom files.
SVNDEF := -D'FLASHROM_VERSION="0.9.0-r$(shell svnversion -cn . \
          | sed -e "s/.*://" -e "s/\([0-9]*\).*/\1/")"'

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROGRAM) $(OBJS) $(LIBS)

flashrom.o: flashrom.c
	$(CC) -c $(CFLAGS) $(SVNDEF) $(CPPFLAGS) $< -o $@

clean:
	rm -f $(PROGRAM) *.o

distclean: clean
	rm -f .dependencies

dep:
	@$(CC) $(CPPFLAGS) $(SVNDEF) -MM *.c > .dependencies

strip: $(PROGRAM)
	$(STRIP) $(STRIP_ARGS) $(PROGRAM)

pciutils:
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
	mkdir -p $(sbindir) $(man8dir)
	$(INSTALL) -m 0755 $(PROGRAM) $(sbindir)
	$(INSTALL) -m 0644 $(PROGRAM).8 $(man8dir)

.PHONY: all clean distclean dep pciutils

-include .dependencies
