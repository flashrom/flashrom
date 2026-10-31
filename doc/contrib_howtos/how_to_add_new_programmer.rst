===========================
How to add a new programmer
===========================

If you plan to add a new programmer, firstly double-check that
none of the existing programmers works for you.

If in doubt, you are welcome to start a thread on the mailing list,
or join the real-time channel :doc:`/contact`.

To add a new programmer, you will need to create a commit, test it, and send it for review.

If this is the first time you contribute to flashrom, please read carefully the :doc:`/dev_guide/development_guide`,
and set up your local dev environment.

There are several places to update, all these need to be included in the patch which adds the programmer:

#. Source code in ``programmers/`` directory

#. Test for it in ``tests/`` directory

   #. we aim to have tests for all new programmers that have been added
      since the time flashrom has tests (old programmers might not have one)
   #. this involves 3 files in ``tests/`` directory:
      source code of new test, and registering it in ``tests/tests.c``, ``tests/tests.h``
   #. check that your new test runs together with other tests

#. Build system

   #. ``meson.build``
   #. ``meson_options.txt``
   #. check that new programmer builds and enabled when the option enabled, and does not build when it's disabled

#. ``test_build.sh`` - jenkins runs this script

#. Header ``include/programmer.h``

#. Table ``programmer_table.c``

#. Documentation

   #. manpage :doc:`/classic_cli_manpage`
   #. recent development doc :doc:`/release_notes/devel`

#. Testing information

   #. test all operations with new programmer
   #. add testing info to the commit message

#. Will you take the responsibility to be a maintainer for your new programmer?
   The responsibility is only your programmer, not everything that is happening on flashrom.

   #. the preference is, of course, for the authors to be the maintainers of their programmer
   #. you will care that the programmer is in good working condition, especially testing before the release
   #. relevant document: :doc:`/about_flashrom/team`
   #. this means adding yourself to ``MAINTAINERS`` file
