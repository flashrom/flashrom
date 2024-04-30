======================
How to add a unit test
======================

Unit tests help to maintain higher bar for code quality. A new unit test which adds coverage to the code is always useful,
no matter how small or large it is! Unit test is a valuable contribution, moreover it makes a good starter project, since
you don't need specific hardware (apart from you development host machine).

For more details on how to run unit tests and measure coverage, check the dev guide: :ref:`unit tests`.

To see the examples of existing unit tests, check the ``/tests`` directory in the source tree. If it helps, you can also look
at git history for examples of previous commits that add new tests.

When is a good time to add a unit test? Any time is a good time. Test can go in its own separate patch, and also it can go
together in a patch which introduces a new code.

Unit tests are using `cmocka <https://cmocka.org/>`_ as a mocking framework, but we also have flashrom mock framework
on the top of that.

Mocking
=========

Unit tests mock all interactions with hardware, interactions with filesystem, syscalls, 3rd party libraries calls
(e.g. libusb, libpci) etc. You can think of a flashrom unit test as a mini-emulator. The goal is to cover as much as possible
flashrom code, but you don't need to go outside of that.

See the list of all current mocks (which are called wraps in cmocka terminology) in ``/tests/wraps.h``. These might be enough for
your new test, or you might need to add more wraps as a part of new test.

New wrap needs to be added to ``/tests/wraps.h``, ``/tests/tests.c``, ``/tests/meson.build``. If it's fine for new wrap to
do nothing, log invokation and return success, all good.

If a wrap need to behave in a specific way for a test, and the behaviour can be different from one test to another, you need to
extend the wrap into ``/tests/io_mock.h`` framework.

Add corresponding member (a function pointer) to ``struct io_mock``
and redirect calls from a wrap function in ``/tests/tests.c`` into a member of ``io_mock``. The exact implementation
of the member function needs to be defined in your new test. At the beginning of a test scenario, define function pointers that your
test needs in your own ``struct io_mock`` and then register by calling ``io_mock_register``. At the end of a test, clean up
by calling ``io_mock_register(NULL)``.

Note that ``io_mock`` can support state (if needed). State is a piece of custom data which is carried around for the duration
of the test scenario and is available in all functions in ``io_mock``.

Adding a new test to a framework
================================

To add new test you will either add a new test function in existing .c file, or add new .c file and new function(s) there.

If you add new file, you need to add it into the list of test source files in ``/tests/meson.build``.

Each new test function needs to be added into ``/tests/tests.h`` and ``/tests/tests.c``. Follow existing entries as examples
how to do it.

Types of tests
==============

Programmers tests
-----------------

The concept of a unit test for flashrom programmer is based on a programmer lifecycle. The options supported by the flashrom
test framework are the following (but you are very welcome to try implement new ideas).

The smallest possible lifecycle is called basic and it does initialisation -> shutdown of a programmer (nothing else).
Another option is probing lifecycle, which does initialisation -> probing a chip -> shutdown.
These two expect successful scenarious, the whole scenario is expected to run until the end with no errors and
success return codes.

One more option is to test expected failure for programmer initialisation. This is useful to test known invalid
(and potentially dangerous) combination of programmer parameters, when such combination should be detected at init time.
If invalid combination of parameters is detected, initialisation expected to fail early and programmer must not continue,
and not send any opcodes to the chip.

For more details, source code is in ``/tests/lifecycle.h`` and ``/tests/lifecycle.c``.

If you want to add new test(s) for a programmer, first you look whether that programmer has any tests already, or none at all.
Test source file has the same name as a programmer itself, for example programmer ``dummyflasher.c`` has its dedicated tests in
``/tests/dummyflasher.c`` file. Either add your tests to an existing file, or create new file for a programmer that had no tests
so far. The latter is more challenging, but it is very useful and highly appreciated.

For programmers tests, the test scenario most likely won't be long: most likely it is one of the options to run lifecycle with
given combination of programmer params as an input. Most time and effort is typically spent on mocking (see above), and this
type of tests will indeed look like a mini-emulator.

Chip operations tests
---------------------

These tests are based on dummyflasher programmer and they are running operations of a chip: read, write, erase, verify.
The test defines mock chip(s) with given properties, and all the operations of the chip are redirected to mock functions.
Mock chip has its own mock memory (an array of bytes) and all operations are performed on this array of bytes.
As all the others, these tests are completely independent of hardware, and are focused on testing core flashrom logic
for read, write, erase, verify.

Examples of chip operation tests are: ``tests/chip.c``, ``/tests/chip_wp.c`` (focused on write-protection logic),
``/tests/erase_func_algo.c`` (focused on erasing and writing logic, especially the choice of erase blocks for given
layout regions).

Misc
----

All other tests. You choose a function that you want to test, call it with given arguments, assert the results are as expected.
