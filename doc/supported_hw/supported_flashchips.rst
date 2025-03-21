=====================
Supported flash chips
=====================

The info about all supported flash chips is in the ``/flashchips`` directory in the source tree.
If you have a flashrom repo cloned locally, you can look at this directory in your repo.

Alternatively inspect it on the `web UI of our GitHub mirror <https://github.com/flashrom/flashrom/tree/main/flashchips>`_.

All the files in the ``/flashchips`` directory are included in parent file ``flashchips.c``. You can inspect the source
`here <https://github.com/flashrom/flashrom/blob/main/flashchips.c>`_.

If you can run flashrom locally, the command ``flashrom -L`` prints the list of all supported flash chips
(see :doc:`/classic_cli_manpage` for more details on command line options). The output of this command is long, so you might
want to save it to file or grep.

If you want to check whether a flash chip is supported in the given release, you can rebase your local
repo at the release tag, alternatively select a tag/branch in GitHub web UI (dropdown on the top-left).

Each chip definition is described by a ``struct flashchip`` in ``include/flash.h``, which you can inspect in the same way,
either in local source tree or on GitHub web UI.

Note the ``.tested`` status of the chip. If the status is ``TEST_UNTESTED`` this means chip definition has been added purely based on
datasheet, but without testing on real hardware.

Related documents:

   * :doc:`/contrib_howtos/how_to_mark_chip_tested`.
   * :doc:`/contrib_howtos/how_to_add_new_chip`.
