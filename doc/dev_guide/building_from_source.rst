Building from source
====================

You're going to need the following tools to get started:

* gcc or clang
* meson
* ninja
* pkg-config
* sphinx-build*

| \* optional, to build man-pages and html documentation

And the following dependencies:

* cmocka*
* linux-headers**
* libpci**
* libusb1**
* libftdi1**
* libjaylink**

| \*  optional, for building unit testing
| \** optional, depending on the selected programmer

If you are cross compiling, install the dependencies for your target.

TL;DR
-----
::

    meson setup builddir
    meson compile -C builddir
    meson install -C builddir


Installing dependencies
-----------------------

.. todo:: Move the bullet points to `tabs <https://www.w3schools.com/howto/howto_js_tabs.asp>`_

    * No external dependencies (documentation should be build without fetching all of pypi)
    * No Javascript?

* Linux
    * Debian / Ubuntu
        ::

            apt-get install -y \
            gcc meson ninja-build pkg-config python3-sphinx \
            libcmocka-dev libpci-dev libusb-1.0-0-dev libftdi1-dev libjaylink-dev

    * ArchLinux / Manjaro
        ::

            pacman -S --noconfirm \
            gcc meson ninja pkg-config python-sphinx cmocka \
            pciutils libusb libftdi libjaylink

    * openSUSE / SUSE
        ::

            zypper install -y \
            gcc meson ninja pkg-config python3-Sphinx \
            libcmocka-devel pciutils-devel libusb-1_0-devel libftdi1-devel libjaylink-devel

    * NixOS / nixpkgs
        * There is a ``shell.nix`` under ``scripts/``

        ::

            nix-shell -p \
            gcc meson ninja pkg-config sphinx \
            cmocka pciutils libusb1 libftdi1 libjaylink

    * Alpine Linux
        ::

            apk add \
            build-base meson ninja pkgconf py3-sphinx \
            cmocka-dev pciutils-dev libusb-dev libjaylink-dev

* Windows
    .. todo:: Add build instructions to build under MSYS2, CYGWIN or cross compiling from Linux

* MacOS
    * Homebrew
        * ``libpci`` is not available through the package manager
        * ``libjaylink`` is not available through the package manager

        ::

            brew install \
            meson ninja pkg-config sphinx-doc \
            libusb libftdi

* BSD
    * FreeBSD / DragonFlyBSD
        * ``libusb1`` is part of the system
        * ``libjaylink`` is not available through the package manager

        ::

            pkg install \
            meson ninja pkgconf py39-sphinx \
            cmocka libpci libftdi1

    * OpenBSD
        * ``libjaylink`` is not available through the package manager

        ::

            pkg_add \
            meson ninja pkg-config py39-sphinx\
            cmocka pciutils libusb1 libftdi1

    * NetBSD
        * ``libjaylink`` is not available through the package manager
        * note: https://www.cambus.net/installing-ca-certificates-on-netbsd/

        ::

            pkgin install \
            meson ninja pkg-config py39-sphinx \
            cmocka pciutils libusb1 libftdi1

* OpenIndiana (Illumos, Solaris, SunOS)
    * ``libpci`` missing, pciutils is build without it
    * ``libftdi1`` & ``libjaylink`` are not available through the package manager
    * TODO: replace ``build-essential`` with the default compiler

    ::

        pkg install build-essential meson ninja cmocka libusb-1

* DJGPP-DOS
    * Get `DJGPP <https://www.delorie.com/djgpp/>`_
        * A great build script can be found `here <https://github.com/andrewwutw/build-djgpp>`_
    * Download the `pciutils <https://mj.ucw.cz/sw/pciutils/>`_ sources

    | Run the following commands in the the pciutils directory to build libpci for DOS.
    | Replace ``<DOS_INSTALL_ROOT>`` with your cross-compile install root.

    ::

        make install-lib \
            ZLIB=no \
            DNS=no \
            HOST=i386-djgpp-djgpp \
            CROSS_COMPILE=i586-pc-msdosdjgpp- \
            STRIP="--strip-program=i586-pc-msdosdjgpp-strip -s" \
            PREFIX=<DOS_INSTALL_ROOT>

    Point pkg-config to the ``<DOS_INSTALL_ROOT>`` ::

        export PKG_CONFIG_SYSROOT=<DOS_INSTALL_ROOT>

    * To compile flashrom use the ``meson_cross/i586_djgpp_dos.txt`` cross-file
    * You will need `CWSDPMI.EXE <https://sandmann.dotster.com/cwsdpmi/>`_ to run flashrom

* libpayload
    .. todo:: Add building instructions for libpayload


Configuration
-------------
In the flashrom repository run::

    meson setup [builtin options] [flashrom options] <builddir>

Mesons ``[builtin options]`` can be displayed with ``meson setup --help``.
The flashrom specific options can be found in ``meson_options.txt`` in the top-level
directory of flashrom and are used like in cmake with ``-Doption=value``
Run ``meson configure`` to display all configuration options.

.. todo:: Write a sphinx extension to render ``meson_options.txt`` here


Configuration for Crossbuilds
-----------------------------
Flashrom specific cross-files can be found in the ``meson_cross`` folder.
To use them run::

    meson setup --cross-file <path/to/crossfile> [builtin options] [flashrom options] <builddir>

The options are the same as the normal configuration options. For more information see
https://mesonbuild.com/Cross-compilation.html


Compiling
---------
Run::

    meson compile -C <builddir>


Update configuration
--------------------
If you want to change your initial configuration for some reason
(for example you discovered that a programmer is missing), run::

    meson configure [updated builtin options] [updated flashrom options] <builddir>


Unit Tests
----------
To execute the unit tests run::

    meson test -C <builddir>

You will get a summary of the unit test results at the end.


Code coverage
"""""""""""""
gcov
    Due to a bug in lcov, the html file will only be correct if lcov is not
    installed and gcovr is installed. See
    https://github.com/linux-test-project/lcov/issues/168 and
    https://github.com/mesonbuild/meson/issues/6747

    To create the coverage target add ``-Db_coverage=true`` to your build configuration.
    After executing the tests, you can run ::

        ninja -C <builddir> coverage

    to generate the coverage report.

lcov / llvm
    https://clang.llvm.org/docs/SourceBasedCodeCoverage.html
    Make sure that you are using `clang` as compiler, e.g. by setting `CC=clang` during configuration.
    Beside that you need to add ``-Dllvm_cov=enabled`` to your build configuration ::

        CC=clang meson setup -Dllvm_cov=enable <builddir>
        meson test -C <builddir>
        ninja -C <builddir> llvm-cov-tests

For additional information see `the meson documentation <https://mesonbuild.com/Unit-tests.html#coverage>`_


Installing
----------
To install flashrom and documentation, run::

    meson install -C <builddir>

This will install flashrom under the PREFIX selected in the configuration phase. Default is ``/usr/local``.

To install into a different directory use DESTDIR, like this::

	DESTDIR=/your/destination/directory meson install -C <your_build_dir>

You can also set the prefix during configuration with::

	meson setup --prefix <DESTDIR> <your_build_dir>

Create distribution package
---------------------------
To create a distribution tarball from your ``builddir``, run::

    meson dist -C <builddir>

This will collect all git tracked files and pack them into an archive.

Current flashrom version is in the VERSION file. To release a new flashrom
version you need to change VERSION file and tag the changing commit.
