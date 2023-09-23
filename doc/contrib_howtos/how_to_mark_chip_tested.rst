==========================
How to mark chip as tested
==========================

flashrom has a massive amount of flashchips definitions, not all of them are fully tested. The reason for this is:
in some situations chip definition can be introduced based on the values that the datasheet claims,
but without testing on hardware. In this case chip definition is marked as not tested, **TEST_UNTESTED**. If later
someone uses the chip for real, and it works, the chip definition can be (and should be) updated, and operations
which run successfully should be marked as tested.

To mark the chip as tested, someone (one of the maintainers, or one of the contributors) need to send a patch to update
the list of flashchips. It can be you, and *yes it can be your first patch to flashrom!* If this is your first patch,
make sure you read the :doc:`/dev_guide/development_guide` and set up dev environment locally. If this is not possible, you
are welcome to send test results to the mailing list, see :ref:`mailing list`.

Instructions below assume you went through Development guide, and set up environment and repository locally.

To begin with, make sure you have full logs from all the operations that you have run successfully and want to
mark as tested. **Providing full logs which indicate successful run is required to mark chip as tested.**

Information about tested status of the chip is stored in ``flashchips.c``, specifically in the ``.tested`` member
of ``struct flashchip``.

Relevant definitions and available options are in ``include/flash.h``, specifically see the definition of:

* ``enum test_state``
* ``struct tested`` (inside ``struct flashchip``)
* pre-defined macros for ``struct tested``, all of them follow the pattern ``#define TEST_xxx_yyy``

Choose the correct value depending on the operations that you have tested successfully. And then:

#. Open ``flashchips.c`` and find the definition of your chip.
#. Check the tested status of the chip. If you tested *not* on the latest HEAD (perhaps on the latest released version,
   or on flashrom built a month ago, etc), it is possible that tested status has been updated already. If the status
   has not been updated,
#. Update ``.tested`` value to the one which reflects current test status. Note that not all the operations have to be
   tested at once: maybe you tested only probe and read, then mark just that.
#. Make sure flashrom builds successfully, and all the unit tests pass.
#. For commit title, use the string ``flashchips: Mark <chip name> as tested for <operations>``.
#. Provide the logs in commit message, you can use flashrom paste service at `paste.flashrom.org <https://paste.flashrom.org>`_
   or any other paste service. As a plan B, you can post message on the :ref:`mailing list`, attach the logs to the post,
   and then add the link to the post to the commit message.
#. Follow :doc:`/dev_guide/development_guide` and send your patch for review.
#. Go through review process until your patch gets approved (see Development guide for more details on this).

To see the examples of such patches, have a look at commit history of ``flashchips.c``.
