==================
Supported chipsets
==================

To see the list of all supported chipsets, check the ``const struct penable chipset_enables[]`` in ``chipset_enable.c`` file in the source tree.
If you have a flashrom repo cloned locally, you can look at the file in your repo.

Alternatively inspect the file `on the web UI of our GitHub mirror <https://github.com/flashrom/flashrom/blob/main/chipset_enable.c#L1768>`_.

If you can run flashrom locally, the command ``flashrom -L`` prints the list of all supported chipsets
(see :doc:`/classic_cli_manpage` for more details on command line options). The output of this command is long, so you might
want to save it to file or grep.

Each chipset entry is described by ``struct penable`` in ``include/programmer.h`` which you can inspect in the same way, either in the local source tree or
`in the GitHub web UI <https://github.com/flashrom/flashrom/blob/main/include/programmer.h#L149>`_.

Note the ``enum test_state status`` of the chipset. ``OK`` means chipset is tested, ``NT`` means not tested, to see all possible
test states check the ``enum test_state`` definition in ``include/flash.h``.

Also note that macros for supported buses are defined in ``chipset_enable.c`` right before ``chipset_enables[]`` array.
For example ``B_S`` means ``BUS_SPI``, check the ``chipset_enable.c`` source code for the rest of macro definitions.
