=================
Development Guide
=================

We welcome contributions from every human being, corporate entity or club.

This document describes the rules and recommendations about the development, contribution and review processes.

If you introduce new features (not flash chips, but stuff like partial
programming, support for new external programmers, voltage handling, etc)
please **discuss your plans** on the :ref:`mailing list` first. That way, we
can avoid duplicated work and know about how flashrom internals need to be
adjusted and you avoid frustration if there is some disagreement about the
design.

You can `look at the latest flashrom development efforts in Gerrit <https://review.coreboot.org/q/project:flashrom>`_.

Set up the git repository and dev environment
=============================================

#. Clone git repository

   * If using https: :code:`git clone "https://review.coreboot.org/flashrom"`
   * If using ssh: :code:`git clone "ssh://<gerrit_username>@review.coreboot.org:29418/flashrom"`

#. Follow the build guidelines to install dependencies :doc:`building_from_source`

#. Install Git hooks: :code:`./util/git-hooks/install.sh`

#. Add upstream as a remote:

   * If using https: :code:`git remote add -f upstream https://review.coreboot.org/flashrom`
   * If using ssh: :code:`git remote add -f upstream ssh://<gerrit_username>@review.coreboot.org:29418/flashrom`

#. Check out a new local branch that tracks :code:`upstream/main`: :code:`git checkout -b <branch_name> upstream/main`

#. Every patch is required to be signed-off (see also :ref:`sign-off`).
   Set up your ``user.name`` and ``user.email`` in git config, and don't forget
   to ``-s`` when creating a commit.

#. See also build guidelines :doc:`building_from_source` and `git docs <https://git-scm.com/doc>`_

Creating your patch
===================

In short, commit your changes with a descriptive message and remember to sign off
on the commit (``git commit -s``).

.. _commit-message:

Commit message
--------------

Commit messages shall have the following format::

    <component>: Short description (up to 72 characters)

    This is a long description. Max width of each line in the description
    is 72 characters. It is separated from the summary by a blank line. You
    may skip the long description if the short description is sufficient,
    for example "flashchips: Add FOO25Q128" to add FOO25Q128 chip support.

    You may have multiple paragraphs in the long description, but please
    do not write a novel here. For non-trivial changes you must explain
    what your patch does, why, and how it was tested.

    Finally, follow the sign-off procedure to add your sign-off!

    Signed-off-by: Your Name <your@domain>

Commit message should include:

* Commit title
* Commit description: explain what the patch is doing, or what it is fixing.
* Testing information: how did you test the patch.
* Signed-off-by line (see below :ref:`sign-off`)
* If applicable, link to the ticket in the bugtracker `<https://ticket.coreboot.org/projects/flashrom>`_
* Change-Id for Gerrit. If commit message doesn't have Change-Id, you forgot to install git hooks.

.. _sign-off:

Sign-off procedure
^^^^^^^^^^^^^^^^^^

We employ a similar sign-off procedure as the `Linux kernel developers
<http://web.archive.org/web/20070306195036/http://osdlab.org/newsroom/press_releases/2004/2004_05_24_dco.html>`_
do. Add a note such as

:code:`Signed-off-by: Random J Developer <random@developer.example.org>`

to your email/patch if you agree with the Developer's Certificate of Origin 1.1
printed below. Read `this post on the LKML
<https://lkml.org/lkml/2004/5/23/10>`_ for rationale (spoiler: SCO).

You must use your known identity in the ``Signed-off-by`` line and in any
copyright notices you add. Anonymous patches lack provenance and cannot be
committed!

Developer's Certificate of Origin 1.1
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    By making a contribution to this project, I certify that:

    (a) The contribution was created in whole or in part by me and I have
    the right to submit it under the open source license indicated in the file; or

    (b) The contribution is based upon previous work that, to the best of my
    knowledge, is covered under an appropriate open source license and I have the
    right under that license to submit that work with modifications, whether created
    in whole or in part by me, under the same open source license (unless I am
    permitted to submit under a different license), as indicated in the file; or

    (c) The contribution was provided directly to me by some other person who
    certified (a), (b) or (c) and I have not modified it; and

    (d) In the case of each of (a), (b), or (c), I understand and agree that
    this project and the contribution are public and that a record of the contribution
    (including all personal information I submit with it, including my sign-off) is
    maintained indefinitely and may be redistributed consistent with this project or the
    open source license indicated in the file.

.. note::

   The `Developer's Certificate of Origin 1.1
   <http://web.archive.org/web/20070306195036/http://osdlab.org/newsroom/press_releases/2004/2004_05_24_dco.html>`_
   is licensed under the terms of the `Creative Commons Attribution-ShareAlike
   2.5 License <http://creativecommons.org/licenses/by-sa/2.5/>`_.

Coding style
------------

Flashrom generally follows Linux kernel style:
https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/process/coding-style.rst

The notable exception is line length limit. Our guidelines are:

* 80-columns soft limit for most code and comments. This is to encourage simple design and concise naming.
* 112-columns hard limit. Use this to reduce line breaks in cases where they
  harm grep-ability or overall readability, such as print statements and
  function signatures. Don't abuse this for long variable/function names or
  deep nesting.
* Tables are the only exception to the hard limit and may be as long as needed
  for practical purposes.

Our guidelines borrow heavily from `coreboot coding style
<https://doc.coreboot.org/contributing/coding_style.html>`_ and `coreboot Gerrit
guidelines <https://doc.coreboot.org/contributing/gerrit_guidelines.html>`_,
and most of them apply to flashrom as well. The really important part is about
the :ref:`sign-off procedure <sign-off>`.

We try to **reuse as much code as possible** and create new files only if
absolutely needed, so if you find a function somewhere in the tree which
already does what you want, please use it.

Testing a patch
---------------

We expect the patch to be appropriately tested by the patch owner.
Please add the testing information in commit message, for example that could be some of these:
programmer you were using, programmer params, chip, OS, operations you were running
(read/write/erase/verify), and anything else that is relevant.

.. _working-with-gerrit:

Working with Gerrit
===================

All of the patches and code reviews need to go via
`Gerrit on review.coreboot.org <https://review.coreboot.org/#/q/project:flashrom>`_.
While it is technically possible to send a patch to the mailing list, that patch
still needs to be pushed to Gerrit by someone. We treat patches on the mailing list as a very
exceptional situation. Normal process is to push a patch to Gerrit.
Please read below for instructions and check `official Gerrit documentation <https://gerrit-review.googlesource.com/Documentation/>`_.

Creating an account
---------------------

#. Go to https://review.coreboot.org/login and sign in using the credentials of
   your choice.
#. Edit your settings by clicking on the gear icon in the upper right corner.
#. Set your Gerrit username (this may be the different from the username of an
   external account you log in with).
#. Add an e-mail address so that Gerrit can send notifications to you about
   your patch.
#. Upload an SSH public key, or click the button to generate an HTTPS password.
#. After account created, set either "Full name" or "Display name", it is used by Gerrit
   for code review emails.

.. _pushing-a-patch:

Pushing a patch
---------------

Before pushing a patch, make sure it builds on your environment and all unit tests pass (see :doc:`building_from_source`).

To push patch to Gerrit, use the follow command: :code:`git push upstream HEAD:refs/for/main`.

* If using HTTPS you will be prompted for the username and password you
  set in the Gerrit UI.
* If successful, the Gerrit URL for your patch will be shown in the output.

There is an option to add a topic to the patch. For one-off standalone patches this
is not necessary. However if your patch is a part of a larger effort, especially if the
work involves multiple contributors, it can be useful to mark that the patch belongs
to a certain topic.

Adding a topic makes it easy to search "all the patches by the topic", even if the patches
have been authored by multiple people.

To add a topic, push with the command: :code:`git push upstream HEAD:refs/for/main%topic=example_topic`.
Alternatively, you can add a topic from a Gerrit UI after the patch in pushed
(on the top-left section) of patch UI.

Checking the CI
---------------

Every patch needs to get a ``Verified +1`` label, typically from Jenkins. Once the patch is pushed
to Gerrit, Jenkins is added automatically and runs its build script. The script builds the patch with
various config options, and runs unit tests (for more details see source code of ``test_build.sh``).
Then, Jenkins gives the patch ``+1`` or ``-1`` vote, indicating success or fail.

In case of failure, follow Jenkins link (which it adds as a comment to the patch), open Console output,
find the error and try to fix it.

In addition to building and running unit tests, Jenkins also runs a scan-build over the patch. Ideally
you should check that your patch does not introduce new warnings. To see scan-build report, follow
Jenkins link -> Build artifacts -> scan build link for the given run.

Adding reviewers to the patch
-----------------------------

After pushing the patch, ideally try to make sure there are some reviewers added to your patch.

flashrom has MAINTAINERS file with people registered for some areas of the code. People who
are in MAINTAINERS file will be automatically added as reviewers if the patch touches that
area. However, not all areas are covered in the file, and it is possible that for the patch you
sent no one is added automatically.

If you know someone in the dev community who can help with patch review, add the person(s) you know.

In general, it's a good idea to add someone who has a knowledge of whatever the patch is doing,
even if the person has not been added automatically.

If you are new, and don't know anyone, and no one has been added automatically: you can add
Anastasia Klimchuk (aklm) as a reviewer.

Going through code reviews
--------------------------

You will likely get some comments on your patch, and you will need to fix the comments.
After doing the work locally, amend your commit ``git commit --amend -s`` and push to Gerrit again.
Check that Change-Id in commit message stays the same. This way Gerrit knows your change belongs
to the same patch, and will upload new change as new patchset for the same patch.

After uploading the work, go through comments and respond to them. Mark as Done the ones you done
and mark them as resolved. If there is something that is impossible to do, or maybe you have more questions,
or maybe you are not sure what you are asked about: respond to a comment **without marking it as resolved**.

It is completely fine to ask a clarifying questions if you don't understand what the comment is asking you to do.
If is also fine to explain why a comment can't be done, if you think it can't be done.

The patch reviews may take some time, but please don't get discouraged.
We have quite high standards regarding code quality.

Initial review should include a broad indication of acceptance or rejection of
the idea/rationale/motivation or the implementation

In general, reviews should focus on the architectural changes and things that
affect flashrom as a whole. This includes (but is by no means limited to)
changes in APIs and types, safety, portability, extensibility, and
maintainability. The purpose of reviews is not to create perfect patches, but
to steer development in the right direction and produce consensus within the
community. The goal of each patch should be to improve the state of the project
- it does not need to fix all problems of the respective field perfectly.

   New contributors may need more detailed advices and should be told about
   minor issues like formatting problems more precisely. The result of a review
   should either be an accepted patch or a guideline how the existing code
   should be changed to be eventually accepted.

To get an idea whether the patch is ready or not, please check :ref:`merge-checklist`.

If you sent a patch and later lost interest or no longer have time to follow up on code review,
please add a comment saying so. Then, if any of our maintainers are interested in finishing the work,
they can take over the patch.

Downloading patch from Gerrit
-----------------------------

Sometimes you may need to download a patch into your local repository. This can be needed for example:

* if you want to test someone else's patch,
* if multiple developers are collaborating on a patch,
* if you are continuing someone else's work, when original author left or unable to continue.

First prepare local repository: sync to head or to desired tag / commit.

Open patch in Gerrit, open "three dot" menu on top-right, open Download patch. Copy Cherry-pick command (pick
the relevant tab for you: anonymous http / http / ssh) and run the copied command in your local repo.

Now you have the commit locally and can do the testing or futher developing. To upload your local changes,
push patch to Gerrit again (see :ref:`pushing-a-patch`).

Make sure people involved in the patch agree that you are pushing new version of someone else's patch,
so this does not come at a surprise for an original author.

Merging patches
---------------

Merging to branches is limited to the "flashrom developers" group on Gerrit (see also :doc:`/about_flashrom/team`).

The list of requirements for the patch to be ready for merging is below, see :ref:`merge-checklist`.
Some of the requirements are enforced by Gerrit, but not all of them. In general, a person who clicks
Submit button is responsible to go through Merge checklist. Code reviewers should be aware of the checklist
as well.

Patch owners can use the checklist to detect whether the patch is ready for merging or not.

.. _merge-checklist:

Merge checklist
^^^^^^^^^^^^^^^

#. Every patch has to be reviewed and needs at least one +2 that was not given by the commit's author.
   Ideally, people who were actively reviewing the patch and adding comments, would be the ones approving it.
#. If a patch is authored by more than one person (Co-developed-by), each author may +2 the other author's changes.
#. Patch needs to get Verified +1 vote, typically from Jenkins build bot. This means the patch builds successfully
   and all unit tests pass.
#. Commit message should have Signed-off-by line, see :ref:`sign-off` and align with the rest
   of the rules for :ref:`commit-message`
#. All the comments need to be addressed, especially if there was a negative vote in the process of review (-1 or -2).
#. flashrom developers are people from literally all around the planet, and various timezones. We usually wait
   for 3 days (3 * 24hours) after the patch is fully approved just in case of last minute concerns from all timezones.
#. In the case of emergency, merging should not take place within less than 24 hours after the review
   started (i.e. the first message by a reviewer on Gerrit).

To help search for patches which are potential candidates for merging, you can try using this search in Gerrit::

   status:open project:flashrom -is:wip -label:Verified-1 label:Verified+1 -label:Code-Review<0 age:3d is:mergeable is:submittable -has:unresolved

Note the search is not a replacement for Merge checklist, but it can help find candidates for merging.

.. _bugtracker:

Bugtracker
==========

We have a bugtracker on `<https://ticket.coreboot.org/projects/flashrom>`_.
Anyone can view tickets, but to be able to create/update/assign tickets you need an account.

Mirrors
========

The only official repository is https://review.coreboot.org/flashrom ; GitHub and GitLab are just mirrors.
**Reviewers do not look at pull requests** on mirrors.
Even if pull requests were automatically transferred to Gerrit,
requirements such as :ref:`sign-off` still present a problem.

The quickest and best way to get your patch reviewed and merged is by sending
it to review.coreboot.org (see :ref:`working-with-Gerrit`). Conveniently, you can use your GitHub, GitLab or
Google account as an OAuth2 `login method <https://review.coreboot.org/login>`_.
