========================
Supported boards/laptops
========================

To see the list of all supported boards or laptops, check either ``struct board_info boards_known[]`` or ``struct board_info laptops_known[]``
in the ``known_boards.c`` file in the source tree.

If you have a flashrom repo cloned locally, you can look at the file in your repo, alternatively inspect the file
`on the web UI of our GitHub mirror <https://github.com/flashrom/flashrom/blob/main/known_boards.c#L29>`_.

If you can run flashrom locally, the command ``flashrom -L`` prints the list of all supported boards and laptops
(see :doc:`/classic_cli_manpage` for more details on command line options). The output of this command is long, so you might
want to save it to file or grep.

Each board entry is described by the ``struct board_info`` in ``include/programmer.h`` which you can inspect in the same way, either in the local source tree or
`in the GitHub web UI <https://github.com/flashrom/flashrom/blob/main/include/programmer.h#L207>`_.

Note the ``enum test_state status`` of the board. ``OK`` means board is tested, ``NT`` means not tested, to see all possible
test states check the ``enum test_state`` definition in ``include/flash.h``.
