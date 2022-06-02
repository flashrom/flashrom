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
  'io_mock.c',
  'tests.c',
  'libusb_wraps.c',
  'helpers.c',
  'flashrom.c',
  'spi25.c',
  'lifecycle.c',
  'layout.c',
  'chip.c',
  'chip_wp.c',
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
  '-Wl,--wrap=__open64_2',
  '-Wl,--wrap=ioctl',
  '-Wl,--wrap=read',
  '-Wl,--wrap=write',
  '-Wl,--wrap=fopen',
  '-Wl,--wrap=fopen64',
  '-Wl,--wrap=fdopen',
  '-Wl,--wrap=fwrite',
  '-Wl,--wrap=fflush',
  '-Wl,--wrap=stat',
  '-Wl,--wrap=stat64',
  '-Wl,--wrap=__xstat',
  '-Wl,--wrap=__xstat64',
  '-Wl,--wrap=fstat',
  '-Wl,--wrap=fstat64',
  '-Wl,--wrap=__fxstat',
  '-Wl,--wrap=__fxstat64',
  '-Wl,--wrap=fileno',
  '-Wl,--wrap=fsync',
  '-Wl,--wrap=fread',
  '-Wl,--wrap=fgets',
  '-Wl,--wrap=__fgets_chk',
  '-Wl,--wrap=fprintf',
  '-Wl,--wrap=__vfprintf_chk',
  '-Wl,--wrap=fclose',
  '-Wl,--wrap=feof',
  '-Wl,--wrap=ferror',
  '-Wl,--wrap=clearerr',
  '-Wl,--wrap=setvbuf',
  '-Wl,--wrap=rget_io_perms',
  '-Wl,--wrap=OUTB',
  '-Wl,--wrap=INB',
  '-Wl,--wrap=OUTW',
  '-Wl,--wrap=INW',
  '-Wl,--wrap=OUTL',
  '-Wl,--wrap=INL',
  '-Wl,--wrap=usb_dev_get_by_vid_pid_number',
  '-Wl,--wrap=libusb_init',
  '-Wl,--wrap=libusb_open',
  '-Wl,--wrap=libusb_set_auto_detach_kernel_driver',
  '-Wl,--wrap=libusb_get_device_list',
  '-Wl,--wrap=libusb_free_device_list',
  '-Wl,--wrap=libusb_get_bus_number',
  '-Wl,--wrap=libusb_get_device_address',
  '-Wl,--wrap=libusb_get_device_descriptor',
  '-Wl,--wrap=libusb_get_config_descriptor',
  '-Wl,--wrap=libusb_free_config_descriptor',
  '-Wl,--wrap=libusb_get_configuration',
  '-Wl,--wrap=libusb_set_configuration',
  '-Wl,--wrap=libusb_claim_interface',
  '-Wl,--wrap=libusb_control_transfer',
  '-Wl,--wrap=libusb_release_interface',
  '-Wl,--wrap=libusb_ref_device',
  '-Wl,--wrap=libusb_unref_device',
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
  ],
  export_dynamic : true,
  link_args : mocks,
  dependencies : [cmocka_dep, flashrom_test_dep],
)
test('cmocka test flashrom', flashrom_tests)
