============
Dummyflasher
============

Dummyflasher programmer is software-only implementation of a flashrom programmer. In other words,
it is an emulator which operates on in-memory arrays of bytes instead of a real chip. Dummyflasher does not interact with any hardware.

This programmer is actively used in unit tests.

Also, since dummyflasher implements all of the programmers APIs, it can be used as an example or as a starting point for implementing a new programmer.

Related documents:

* :doc:`/contrib_howtos/how_to_add_unit_test`
