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

test_srcs = files(
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
  'selfcheck.c',
  'io_real.c',
  'erase_func_algo.c',
  'udelay.c',
)

if not programmer.get('dummy').get('active')
  test_srcs += programmer.get('dummy').get('srcs')
endif

foreach p_name, p_data : programmer
  test_srcs += p_data.get('test_srcs')
endforeach

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
  '-Wl,--wrap=__fstat50',
  '-Wl,--wrap=__fxstat',
  '-Wl,--wrap=__fxstat64',
  '-Wl,--wrap=fileno',
  '-Wl,--wrap=fsync',
  '-Wl,--wrap=fread',
  '-Wl,--wrap=fgets',
  '-Wl,--wrap=fprintf',
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
  '-Wl,--wrap=libusb_set_debug',
  '-Wl,--wrap=libusb_set_option',
  '-Wl,--wrap=libusb_open',
  '-Wl,--wrap=libusb_set_auto_detach_kernel_driver',
  '-Wl,--wrap=libusb_detach_kernel_driver',
  '-Wl,--wrap=libusb_attach_kernel_driver',
  '-Wl,--wrap=libusb_open_device_with_vid_pid',
  '-Wl,--wrap=libusb_get_device',
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
  '-Wl,--wrap=libusb_alloc_transfer',
  '-Wl,--wrap=libusb_submit_transfer',
  '-Wl,--wrap=libusb_free_transfer',
  '-Wl,--wrap=libusb_handle_events_timeout',
  '-Wl,--wrap=libusb_exit',
  '-Wl,--gc-sections',
]

threads_dep = dependency('threads')

flashrom_tests = executable('flashrom_unit_tests',
  test_srcs,
  c_args : [
    cargs,
    '-ffunction-sections',
    '-fdata-sections',
    '-U_FORTIFY_SOURCE',
    '-DFLASHROM_TEST',
  ],
  export_dynamic : true,
  link_args : mocks + link_args,
  dependencies : [cmocka_dep, flashrom_test_dep, threads_dep],
)
test('cmocka test flashrom', flashrom_tests)

if get_option('llvm_cov').enabled()
  run_target('llvm-cov-tests', command : ['../scripts/llvm-cov', flashrom_tests])
endif
