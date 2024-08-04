===============
Release process
===============

The document describes the technical aspect of making a flashrom release,
and it assumes that the team of active core maintainers is in agreement to commence the process.

To go through the process, at least two maintainers are needed to be closely involved,
because it includes sending and approving patches in Gerrit.

Set up the timeline and announce on the mailing list
====================================================

Decide on the bug-fixing and testing window (3-4 weeks), decide exact dates of start and end of the window,
and announce it on the mailing list. Ideally make an announcement a few weeks in advance.

During the testing and bug-fixing window only bug fixes are merged, and no new features are added.
Typically it's fine to push new features for review, and reviews are fine too,
but merging new features will be delayed until the release is done.
*This should be very clearly explained in the announcement.*

Start testing and bug-fixing window
===================================

* Double-check and merge all the patches that are fully ready (see also :ref:`merge-checklist`)

* Update VERSION file to first release candidate. Check the git history of VERSION file for a version name pattern.

  * As an example at the time of writing, the version name of the first release candidate was ``1.4.0-rc1``.
  * To update the VERSION file, push a patch to Gerrit, and another maintainer should review and approve.

* After submitting the change to the VERSION file, tag this commit.
  You can check `flashrom tags in Gerrit <https://review.coreboot.org/admin/repos/flashrom,tags,25>`_
  for tag name pattern. As an example at the time of writing, the tag name was ``v1.4.0-rc1``.

* Write an announcement on the mailing list. Be very clear about:

  * start and end date of the window, and what does it mean
  * any help with :ref:`building-and-testing` is very much appreciated

**From this moment and until the release cut, the highest priority is for building and testing on various environments, and bug-fixing.**

Release candidates
==================

If any bugs are found and fixed (or reverted), then the second, or third release candidate will be needed.
The process is the same as with the first candidate:

* Update the VERSION file, and submit this
* Tag the commit which updates the VERSION file
* Post an announcement on mailing list

Release notes
=============

During the time in-between releases, ideally most updates are accumulated in the doc :doc:`/release_notes/devel`.
While this doc is helpful, it is not a replacement for a human to go through all development history
since the previous release and prepare release notes. One maintainer is preparing the release notes
and sending them for review, and at least one other maintainer needs to review that (it can be more than one reviewer).

Ideally the patch with release notes should be prepared, reviewed and approved before the release cut,
so that it can be published by the time of final release announcement.

For inspiration to write release notes, have a look at prior art :doc:`/release_notes/index`.

There is one section in release notes, Download, which is not possible to complete before the actual release cut.
Leave it as TODO, but complete the rest.

Cut the release
===============

Wait for at least a week (or two) since the last release candidate. if everything is alright:

* Submit the release notes, and in the same patch restart :doc:`/release_notes/devel` document.
  This way everyone who is syncing the repository by the release tag will have release notes in the tree.

* Update VERSION file to release version (for example, at the time of writing ``1.4.0``), and submit this

* Tag the commit which updates the VERSION file. You can check
  `flashrom tags in Gerrit <https://review.coreboot.org/admin/repos/flashrom,tags,25>`_ for tag name pattern.
  As an example at the time of writing, the tag name was ``v1.4.0``.

* Create the tarball, sign it, and upload to the server together with the signature.

* Update release notes with the link to download tarball, signature, and fingerprint. Submit this and check that final release notes are published on the website.

* Write the release announcement, don't forget to:

  * Link to download the tarball, signature and fingerprint.
  * Say thank you to everyone who is helping and supporting flashrom
  * Add link to published release notes on the website

Start the next cycle of development
===================================

* Update the VERSION file to the development version. For example, at the time of writing ``1.5.0-devel``, and submit this.

* Submit all the patches that have been ready and waiting.

* Celebrate :)
