How to add or update docs
=========================

Documentation files live in ``/doc`` directory in the source tree, so
adding or updating documentation follows the same process as changing
the code. If you've never done it before, start by carefully
reading the :doc:`/dev_guide/development_guide`.

To add or update a documentation page, you need to create or modify
an ``.rst`` file in the ``/doc`` directory and send a patch for
review.

People who are registered in MAINTAINERS file for doc/ directory will
be automatically added to the patch as reviewers. However, you are
very welcome to add more reviewers who know the subject. In fact, it
is always a good idea to add someone who has knowledge of the specific
area you are documenting.

We are using Sphinx doc engine for documentation (see
https://www.sphinx-doc.org/) and reStructured Text format for content.
reStructuredText Primer page has more details
https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#restructuredtext-primer

Brand new page needs to be added to the appropriate ``index.rst`` file
under ``/doc`` directory (that could be a root index file or nested one).

To test your changes, build flashrom with documentation and open
generated ``.html`` file in the browser. Generated ``.html`` files are
in meson ``builddir/doc/html`` directory.

Misc questions
--------------

* We use CC-BY-4.0 license for documentation.
* Writing style can be formal or informal, it's mostly up to you, the
  important thing is to make the text clear, readable and unambiguous. You
  can insert images if this really helps the readers to understand the
  instructions.
* Documentation should be relevant to either flashrom usage or flashrom
  development
