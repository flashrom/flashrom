GitHub binary builds
====================

``build.yml`` runs on branch pushes, pull requests and manual dispatch. It
calls ``build-binaries.yml``, which builds the CLI using Meson on these runners:

.. list-table::
   :header-rows: 1

   * - Platform
     - Runner
     - Artifact
   * - Linux x86-64
     - Ubuntu 24.04
     - ``.deb`` with generated runtime dependencies
   * - macOS Apple Silicon
     - macOS 15
     - ``.tar.gz`` with bundled libraries
   * - macOS Intel
     - macOS 15 Intel
     - ``.tar.gz`` with bundled libraries
   * - Windows x86-64
     - Windows 2022, MSYS2 UCRT64
     - ``.zip`` with bundled DLLs

Download the ``flashrom-*`` artifacts from the workflow run. Install the Linux
package with ``sudo apt install ./flashrom-*.deb`` on Ubuntu 24.04 or a
compatible newer distribution. On macOS, extract the archive and run
``./flashrom`` from its directory, keeping ``lib/`` beside it. These builds
target macOS 15 or later. On Windows, extract the ZIP and run ``flashrom.exe``,
keeping its DLLs beside it; MSYS2 is not required on the target.

The macOS executable and bundled libraries are ad-hoc signed after their
library paths are rewritten, and each signature is verified before packaging.
They are not signed with an Apple Developer ID or notarized. Ad-hoc signing
allows local execution but does not establish trust for Gatekeeper: macOS may
block files quarantined by a browser download. The CI startup check does not
test this download path. Distribution without a Gatekeeper override requires
Developer ID signing and `Apple notarization
<https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution>`_.

Builds enable Meson's ``auto`` programmer selection and RPMC. Linux includes
PCI, USB, FTDI, J-Link and serial dependencies. macOS includes USB, FTDI and
serial programmers; Windows also includes J-Link. Internal/PCI programmers
are not available on macOS or Windows. USB access on Windows requires the
appropriate device driver. The Meson configuration summary lists the actual
programmers.

Linux and Windows run the unit tests using GNU linker wrapping, which is not
available with the macOS linker. Every job smoke-tests the packaged CLI using
``--version``; Windows clears the toolchain from PATH first, and macOS checks
that bundled binaries no longer reference Homebrew libraries. These checks
do not test physical programmers.

CI packages append the commit ID to their version. Packages include the source
commit ID, Meson options and license information. The macOS and Windows builds
bundle package-manager libraries and include dependency notices/provenance;
Linux relies on distribution packages. Dependency versions follow the runner's
package repositories, so rebuilding a tag later is not bit-for-bit reproducible.
