# This file is part of the flashrom project.
#
# Copyright 2020 Google LLC
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

root_includes = include_directories('../subprojects')

srcs = [
  'tests.c',
  'helpers.c',
  'flashrom.c',
  'spi25.c',
  'init_shutdown.c',
]

mocks = [
  '-Wl,--wrap=physunmap',
  '-Wl,--wrap=physmap',
  '-Wl,--wrap=spi_send_command',
  '-Wl,--wrap=sio_write',
  '-Wl,--wrap=sio_read',
  '-Wl,--wrap=open',
  '-Wl,--wrap=open64',
  '-Wl,--wrap=ioctl',
  '-Wl,--wrap=fopen',
  '-Wl,--wrap=fopen64',
  '-Wl,--gc-sections',
]

flashrom_tests = executable('flashrom_unit_tests',
  srcs,
  include_directories : root_includes,
  c_args : [
    cargs,
    '-ffunction-sections',
    '-fdata-sections',
    #    '-DSTANDALONE',
    '-DCONFIG_DEFAULT_PROGRAMMER=PROGRAMMER_DUMMY',
    '-DCONFIG_DEFAULT_PROGRAMMER_ARGS=""',
  ],
  export_dynamic : true,
  link_args : mocks,
  dependencies : [cmocka_dep, flashrom_test_dep],
)
test('cmocka test flashrom', flashrom_tests)
