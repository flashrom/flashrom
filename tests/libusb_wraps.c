/*
 * This file is part of the flashrom project.
 *
 * Copyright 2021 Google LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <include/test.h>
#include "io_mock.h"

void *__wrap_usb_dev_get_by_vid_pid_number(
		libusb_context *usb_ctx, uint16_t vid, uint16_t pid, unsigned int num)
{
	LOG_ME;
	return not_null();
}

int __wrap_libusb_init(libusb_context **ctx)
{
	LOG_ME;
	if (get_io() && get_io()->libusb_init)
		return get_io()->libusb_init(get_io()->state, ctx);
	return 0;
}

int __wrap_libusb_set_configuration(libusb_device_handle *devh, int config)
{
	LOG_ME;
	return 0;
}

int __wrap_libusb_claim_interface(libusb_device_handle *devh, int interface_number)
{
	LOG_ME;
	return 0;
}

int __wrap_libusb_control_transfer(libusb_device_handle *devh, uint8_t bmRequestType,
		uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data,
		uint16_t wLength, unsigned int timeout)
{
	LOG_ME;
	if (get_io() && get_io()->libusb_control_transfer)
		return get_io()->libusb_control_transfer(get_io()->state,
				devh, bmRequestType, bRequest, wValue, wIndex, data, wLength, timeout);
	return 0;
}

int __wrap_libusb_release_interface(libusb_device_handle *devh, int interface_number)
{
	LOG_ME;
	return 0;
}

void __wrap_libusb_close(libusb_device_handle *devh)
{
	LOG_ME;
}

void __wrap_libusb_exit(libusb_context *ctx)
{
	LOG_ME;
}
