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
  'layout.c',
  'chip.c',
]

mocks = [
  '-Wl,--wrap=strdup',
  '-Wl,--wrap=physunmap',
  '-Wl,--wrap=physmap',
  '-Wl,--wrap=pcidev_init',
  '-Wl,--wrap=pcidev_readbar',
  '-Wl,--wrap=spi_send_command',
  '-Wl,--wrap=sio_write',
  '-Wl,--wrap=sio_read',
  '-Wl,--wrap=open',
  '-Wl,--wrap=open64',
  '-Wl,--wrap=ioctl',
  '-Wl,--wrap=read',
  '-Wl,--wrap=write',
  '-Wl,--wrap=fopen',
  '-Wl,--wrap=fopen64',
  '-Wl,--wrap=fwrite',
  '-Wl,--wrap=fflush',
  '-Wl,--wrap=stat',
  '-Wl,--wrap=stat64',
  '-Wl,--wrap=fstat',
  '-Wl,--wrap=fstat64',
  '-Wl,--wrap=fileno',
  '-Wl,--wrap=fsync',
  '-Wl,--wrap=fread',
  '-Wl,--wrap=fgets',
  '-Wl,--wrap=fclose',
  '-Wl,--wrap=feof',
  '-Wl,--wrap=ferror',
  '-Wl,--wrap=clearerr',
  '-Wl,--wrap=setvbuf',
  '-Wl,--wrap=rget_io_perms',
  '-Wl,--wrap=test_outb',
  '-Wl,--wrap=test_inb',
  '-Wl,--wrap=test_outw',
  '-Wl,--wrap=test_inw',
  '-Wl,--wrap=test_outl',
  '-Wl,--wrap=test_inl',
  '-Wl,--wrap=usb_dev_get_by_vid_pid_number',
  '-Wl,--wrap=libusb_set_configuration',
  '-Wl,--wrap=libusb_claim_interface',
  '-Wl,--wrap=libusb_control_transfer',
  '-Wl,--wrap=libusb_release_interface',
  '-Wl,--wrap=libusb_close',
  '-Wl,--wrap=libusb_exit',
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
  ],
  export_dynamic : true,
  link_args : mocks,
  dependencies : [cmocka_dep, flashrom_test_dep],
)
test('cmocka test flashrom', flashrom_tests)
