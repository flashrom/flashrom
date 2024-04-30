=========
Team
=========

flashrom development process is happening in Gerrit.
All contributors and users who have a Gerrit account can send patches,
add comments to patches and vote +1..-1 on patches.

All contributors and users are expected to follow Development guidelines and
:doc:`code_of_conduct`.

There are two special groups in Gerrit.

"flashrom reviewers" group
==========================

Members of the group (see `flashrom reviewers <https://review.coreboot.org/admin/groups/25cadc351dd0492fd2a2a1b1a8e5bb08c29e411f,members>`_)
can do full approval of patches (i.e. vote +2).

In general, members of the group have some area of responsibility in the
`MAINTAINERS <https://review.coreboot.org/plugins/gitiles/flashrom/+/refs/heads/main/MAINTAINERS>`_ file,
and are automatically added as reviewers to patches when the patch touches this area.

The responsibilities are the following.

* React to patches when added as a reviewer.

* Try to respond to technical questions on the mailing list if the topic is something you know about
  and can provide a useful response.

* Know development guidelines and check the patches you are reviewing align with the guidelines.

"flashrom developers" group
===========================

Members of the group (see `flashrom developers <https://review.coreboot.org/admin/groups/db95ce11b379445ac8c5806ea0b61195555b338d,members>`_)
can merge patches.
The responsibilities for the members of the group are described in more details below.

There is no expectation on how much time you spend on your duties, some non-zero amount of time,
whatever capacity you have. But in general, you stay around on flashrom.

If you disappear for some time (life happens), especially for a longer time, like several months,
especially without a warning: you implicitly agree that the others will handle the duties and make decisions if needed
(potentially without waiting for you to come back, if the decision is needed quickly).

* Merge all contributors's patches (when they are ready), not just your own.

* Be at least vaguely aware what development efforts are ongoing, this helps to make decisions
  in what order the patches should be merged, and where could be merge conflicts.

* Know development guidelines, and educate other contributors if needed (e.g. give links).

* React to patches when added as a reviewer.

* Try to respond to technical questions on the mailing list if the topic is something you know about
  and can provide a useful response.

* From time to time show up in real-time channel(s) and/or dev meetings.

* Follow the :doc:`code_of_conduct`, be a good example for others.

* Bonus point: if you work in a [software] company, educate and help contributors from your company
  with upstream culture and dev guidelines.
