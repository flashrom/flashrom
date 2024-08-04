=======================
How to support flashrom
=======================

Intro
=========

This document is for people and companies who want to help flashrom, and it explains
various ways how to do this.

There are lots of ways to help (as you can see below), whether you have little time or a lot,
whether you are a developer, a user or a company.
flashrom is a free open source software project, and all the contributions are publicly visible -
which means you get all the glory for your work, and you help the whole open source ecosystem.
Thank you!

flashrom supports a huge variety of environments, platforms and hardware, and there is
no one person or one company on earth which has setup to test and maintain all of these
(and realistically, never will be). The only way we can maintain all of these,
is together as a community.

To be aware of what's going on, subscribe to our :ref:`mailing list` and/or join our :ref:`real time channels`.

Development
===========

Development, or in other words, sending patches to flashrom, is what probably comes to mind first
when you think about helping and contributing. And indeed, this is a great help,
and patches are always welcome.

If your idea fits into one patch, you can just send it. If you have bigger plans in mind,
a large amount of work over a longer time frame, the best way is to start a topic on the mailing list.
This helps with planning, and also people in the community, whoever is interested,
will be able to support your effort.

If you are new to flashrom development process, please start by reading :doc:`/dev_guide/development_guide`.
It's also useful to check `existing patches <https://review.coreboot.org/q/status:open+project:flashrom>`_
for examples on how the dev process works.

For some types of contributions we have more detailed guidelines, check the list :doc:`/contrib_howtos/index`.

Code reviews
============

Did you know: code reviews are equally important as writing the code!

For each patch, we need at least one reviewer, and often more than one.
Doing code reviews is highly appreciated and highly respected!
All reviewers' names are immortalised in git history together with authors names,
as "Reviewed-by" and "Signed-off-by" tags in a commit message (see `example <https://review.coreboot.org/c/flashrom/+/80729>`_).

Code review involves carefully inspecting the patch in Gerrit, and adding comments if anything
is unclear/potential errors/issues/something missing etc. If you think the patch is ready,
you can vote on the patch. Every Gerrit user with a registered account can add comments to patches
and vote +1/-1. Note that if you vote -1, you need to add a comment explaining why you gave a negative vote,
and what specific big issues that you see with the patch. Negative vote is a stronger opinion,
and in most cases just adding comments is enough.

The group of people who can fully approve the patch (i.e. vote +2, see :doc:`/about_flashrom/team`)
is limited, however every Gerrit user can do code reviews. This increases overall confidence
in the reviewed patch. Approving the patch is much easier when the code reviews are done well.

You can check pending patches under review `in Gerrit <https://review.coreboot.org/q/status:open+project:flashrom>`_
and help with code review if a patch looks useful, you understand what it is about, and want to have it submitted.

.. _building-and-testing:

Building and testing
====================

Given the large variety of environments and hardware that flashrom supports,
the question of building and testing flashrom is always relevant.
Try to build flashrom at head on your environment and if it doesn't build,
send a patch to fix (see :doc:`/dev_guide/development_guide`) or file a ticket in :ref:`bugtracker`
with as many details as possible.

A special case of this is, the time when flashrom team is preparing the release.
The release candidate tag is announced on the mailing list, and it is a great help to try and build and test
a release candidate in whatever your environment is. Both positive and negative results are important,
and you are welcome to share the results, just don't forget to include environment details.
In case of issues, as always: patches are very very welcome.

Documentation
=============

You don't have to be a flashrom developer to add, update or review documentation. In fact,
lots of users' docs can greatly benefit from reviews by the users, who are supposed to read and use the docs.
Doc how to update the docs is here: :doc:`/how_to_add_docs`

As for specific ideas:

#. If there is an announcement on the mailing list about new doc under review,
   have a look and you can join the review

#. Help migrate information from `old website <https://wiki.flashrom.org/>`_ to `new website <https://www.flashrom.org/>`_.
   The rule is, all useful docs need to be migrated but they should be reviewed. Concrete example,
   there are docs for programmers, written a while ago. If you are using the programmer regularly
   you can review the existing doc and help update it (if needed) and then the updated doc will
   go to the new website.

#. New documentation welcome.

Mailing list
============

If you are not subscribed: please subscribe (see :ref:`mailing list`) so you can see what's going on.

Oftentimes, mailing list has questions from flashrom users. If it so happens
that you maybe know what they are asking, or have ideas about it - you are welcome to respond!
This will be very helpful.

Similarly, if there is a development discussion that makes sense to you and is relevant: please join the discussion.

Mailing list is archived, and archives are public and searchable. Which means,
when you respond to the post you not only help that one person who is asking,
but you also help one hundred people in future, who have the same question and can search the answer on the list archives.

Joining the team
================

If you have experience of flashrom development, good knowledge of some of the areas of flashrom code,
some time and motivation, you can consider joining the team, more info here (:doc:`/about_flashrom/team`).
Unlike the previous ideas, this means some *regular* time commitment (the amount of time
can be small or large, but it is regular).

If you are not at this stage yet, but are considering this as a potential goal for the future,
check the :doc:`/about_flashrom/team` page for what it means.

Special appeal for companies
============================

There are lots of companies that have their own forks of flashrom, and it would be a great help
if you could contribute back to the upstream project!

Try to keep your fork as close as possible to upstream, do not diverge without a strong reason.
This makes it easier for you to downstream patches, and also makes it easier to contribute patches
from your fork upstream. As an end result, you will be exchanging code and knowledge with a large ecosystem
rather than hiding in your own corner. Working together we can achieve a higher quality bar,
which is better for the upstream project, and better for your fork.

Consider the following ideas:

#. Send upstream the bug fixes you found

#. Add unit tests for the areas you are using actively

#. Add new features or add support for new platforms/hardware, especially if you have that in your lab
   and can reliably test and maintain

#. Help with releases: if you have an automated test suite, run it on release candidates.
   Build and test flashrom for the devices you have in the lab.

#. If possible, allocate an engineer(s) to contribute to upstream project (and all their work
   you can downstream straight away). Upstream early, upstream often: anything you can upstream sooner
   will make your life easier in the future.

#. Have someone subscribed to the mailing list and respond when the topic is relevant to you,
   and you have a knowledge of questions or ideas how to help.

#. On a long term, consider joining the :doc:`/about_flashrom/team`, pick something to maintain:
   for example a programmer you are using often

Outro
========

If you read all of the above and still unsure what to do, but actually want to help,
please don't be afraid to ask flashrom project lead directly (Anastasia Klimchuk,
and you can find email in `Gerrit <https://review.coreboot.org/c/flashrom/+/80729>`_
or tag aklm on Discord (see :ref:`real time channels`).

**Every bit of help matters and you can help make flashrom a better place. Thank you!**
