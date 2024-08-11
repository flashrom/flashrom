Supported programmers
=====================

flashrom supports many different programmers, for the full list you can look into `programmer_table.c <https://github.com/flashrom/flashrom/blob/main/programmer_table.c>`_
in the source tree. Some of the programmers have their own documentation pages, see below.

Please check the :ref:`programmer-specific information` section of the :doc:`/classic_cli_manpage` for more details about programmers and their usage.

If you can run flashrom locally, the command ``flashrom -L`` prints all devices supported per programmer
(see :doc:`/classic_cli_manpage` for more details on command line options). The output of this command is long, so you might
want to save it to file or grep.

Patches to add/update documentation, or migrate docs from `old wiki website <https://wiki.flashrom.org/Supported_programmers>`_ are very much appreciated.

.. toctree::
    :maxdepth: 1

    buspirate
    dummyflasher
    ft2232_spi
    serprog/index
