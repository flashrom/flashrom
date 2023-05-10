Building with make
==================

TLDR
----

::

	make
	make install

Build instructions
------------------

**To build flashrom you need to install the following software:**

 * C compiler (GCC / clang)
 * pkg-config

 * pciutils+libpci (if you want support for mainboard or PCI device flashing)
 * libusb (if you want FT2232, Dediprog or USB-Blaster support)
 * libftdi (if you want FT2232 or USB-Blaster support)
 * libjaylink (if you want support for SEGGER J-Link and compatible devices)

**Linux et al:**

 * pciutils / libpci
 * pciutils-devel / pciutils-dev / libpci-dev
 * zlib-devel / zlib1g-dev (needed if libpci was compiled with libz support)

**On FreeBSD, you need the following ports:**

 * devel/gmake
 * devel/libpci

**On OpenBSD, you need the following ports:**

 * devel/gmake
 * sysutils/pciutils

**To compile on Linux, use**::

	make

**To compile on FreeBSD, OpenBSD or DragonFly BSD, use**::

	gmake

**To compile on Nexenta, use**::

	make

**To compile on Solaris, use**::

	gmake LDFLAGS="-L$pathtolibpci" CC="gcc -I$pathtopciheaders" CFLAGS=-O2

**To compile on NetBSD (with pciutils, libftdi, libusb installed in /usr/pkg/), use**::

	gmake

**To compile and run on Darwin/Mac OS X:**

Install DirectHW from coresystems GmbH.
DirectHW is available at https://www.coreboot.org/DirectHW .

**To cross-compile on Linux for DOS:**

Get packages of the DJGPP cross compiler and install them:

 * djgpp-filesystem djgpp-gcc djgpp-cpp djgpp-runtime djgpp-binutils

As an alternative, the DJGPP web site offers packages for download as well:

 * djcross-binutils-2.29.1-1ap.x86_64.rpm
 * djcross-gcc-7.2.0-1ap.x86_64.rpm
 * djcrx-2.05-5.x86_64.rpm

The cross toolchain packages for your distribution may have slightly different
names (look for packages named *djgpp*).

Alternatively, you could use a script to build it from scratch:
https://github.com/andrewwutw/build-djgpp

You will need the libpci and libgetopt library source trees and
their compiled static libraries and header files installed in some
directory say libpci-libgetopt/, which will be later specified with
LIBS_BASE parameter during flashrom compilation. Easiest way to
handle it is to put pciutils, libgetopt and flashrom directories
in one subdirectory. There will be an extra subdirectory libpci-libgetopt
created, which will contain compiled libpci and libgetopt.

Download pciutils 3.5.6 and apply https://flashrom.org/File:Pciutils-3.5.6.patch.gz
Compile pciutils, using following command line::

	make ZLIB=no DNS=no HOST=i386-djgpp-djgpp CROSS_COMPILE=i586-pc-msdosdjgpp- \
		PREFIX=/ DESTDIR=$PWD/../libpci-libgetopt  \
		STRIP="--strip-program=i586-pc-msdosdjgpp-strip -s" install install-lib

Download and compile with 'make' https://flashrom.org/File:Libgetopt.tar.gz

Copy the libgetopt.a to ../libpci-libgetopt/lib and
getopt.h to ../libpci-libgetopt/include

Enter the flashrom directory::

	make CC=i586-pc-msdosdjgpp-gcc STRIP=i586-pc-msdosdjgpp-strip LIBS_BASE=../libpci-libgetopt/ strip

If you like, you can compress the resulting executable with UPX::

	upx -9 flashrom.exe

To run flashrom.exe, download https://flashrom.org/File:Csdpmi7b.zip and
unpack CWSDPMI.EXE into the current directory or one in PATH.

**To cross-compile on Linux for Windows:**

Get packages of the MinGW cross compiler and install them::

	mingw32-filesystem mingw32-cross-cpp mingw32-cross-binutils mingw32-cross-gcc
	mingw32-runtime mingw32-headers

The cross toolchain packages for your distribution may have slightly different
names (look for packages named *mingw*).
PCI-based programmers (internal etc.) are not supported on Windows.
Run (change CC= and STRIP= settings where appropriate)::

	make CC=i686-w64-mingw32-gcc STRIP=i686-w64-mingw32-strip

**Processor architecture dependent features:**

On non-x86 architectures a few programmers don't work (yet) because they
use port-based I/O which is not directly available on non-x86. Those
programmers will be disabled automatically if you run "make".

**Compiler quirks:**

If you are using clang and if you want to enable only one driver, you may hit an
overzealous compiler warning from clang. Compile with "make WARNERROR=no" to
force it to continue and enjoy.

**Bindings:**

Foreign function interface bindings for the rust language are included in the
bindings folder. These are not compiled as part of the normal build process.
See the readme under bindings/rust for more information.


Installation
------------

In order to install flashrom and the manpage into /usr/local, type::

	make install

For installation in a different directory use DESTDIR, e.g. like this::

	make DESTDIR=/usr install

If you have insufficient permissions for the destination directory, use sudo
by adding sudo in front of the commands above.


Packaging
---------

To package flashrom and remove dependencies on Git, either use::

	make export

or::

	make tarball

``make export`` will export all flashrom files from the Git repository at
revision HEAD into a directory named ``$EXPORTDIR/flashrom-$RELEASENAME``
and will additionally add a ``versioninfo.inc`` file in that directory to
contain the Git revision of the exported tree and a date for the manual
page.

``make tarball`` will simply tar up the result of make export and compress
it with bzip2.

The snapshot tarballs are the result of ``make tarball`` and require no
further processing. Some git files (for example the rust bindings) are omitted
from the tarball, as controlled by the .gitattributes files.
