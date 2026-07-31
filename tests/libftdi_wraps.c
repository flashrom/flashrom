/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2026 Kjell Peterson <kjell.peterson@shotover.com>
 */

#include "libftdi_wraps.h"

#include "include/test.h"
#include "io_mock.h"

int __wrap_ftdi_init(struct ftdi_context *ftdi)
{
	LOG_ME;
	// only the type field is accessed by flashrom, otherwise the context is opaque
	ftdi->type = TYPE_4232H;
	return 0;
}

int __wrap_ftdi_set_interface(struct ftdi_context *ftdi, enum ftdi_interface interface)
{
	LOG_ME;
	return 0;
}

int __wrap_ftdi_usb_open_desc(struct ftdi_context *ftdi, int vendor, int product,
		const char *description, const char *serial)
{
	LOG_ME;
	return 0;
}

int __wrap_ftdi_usb_find_all(struct ftdi_context *ftdi, struct ftdi_device_list **devlist,
		int vendor, int product)
{
	LOG_ME;
	if (get_io() && get_io()->ftdi_usb_find_all)
		return get_io()->ftdi_usb_find_all(get_io()->state, ftdi, devlist, vendor, product);
	if (devlist)
		*devlist = NULL;
	return 0;
}

int __wrap_ftdi_usb_open_dev(struct ftdi_context *ftdi, struct libusb_device *dev)
{
	LOG_ME;
	return 0;
}

void __wrap_ftdi_list_free(struct ftdi_device_list **devlist)
{
	LOG_ME;
}

int __wrap_ftdi_usb_reset(struct ftdi_context *ftdi)
{
	LOG_ME;
	return 0;
}

int __wrap_ftdi_set_latency_timer(struct ftdi_context *ftdi, unsigned char latency)
{
	LOG_ME;
	return 0;
}

int __wrap_ftdi_set_bitmode(struct ftdi_context *ftdi, unsigned char bitmask, unsigned char mode)
{
	LOG_ME;
	return 0;
}

int __wrap_ftdi_write_data(struct ftdi_context *ftdi, const unsigned char *buf, int size)
{
	LOG_ME;
	return size;
}

int __wrap_ftdi_read_data(struct ftdi_context *ftdi, unsigned char *buf, int size)
{
	LOG_ME;
	return size;
}

int __wrap_ftdi_usb_close(struct ftdi_context *ftdi)
{
	LOG_ME;
	return 0;
}

const char *__wrap_ftdi_get_error_string(struct ftdi_context *ftdi)
{
	LOG_ME;
	return "";
}
