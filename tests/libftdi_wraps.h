/*
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2026 Kjell Peterson <kjell.peterson@shotover.com>
 */

#ifndef LIBFTDI_WRAPS_H
#define LIBFTDI_WRAPS_H

#include "ftdi_unittests.h"
#include "usb_unittests.h"
#include <stdint.h>

int __wrap_ftdi_init(struct ftdi_context *ftdi);
int __wrap_ftdi_set_interface(struct ftdi_context *ftdi, enum ftdi_interface interface);
int __wrap_ftdi_usb_open_desc(struct ftdi_context *ftdi, int vendor, int product,
		const char *description, const char *serial);
int __wrap_ftdi_usb_find_all(struct ftdi_context *ftdi, struct ftdi_device_list **devlist,
		int vendor, int product);
int __wrap_ftdi_usb_open_dev(struct ftdi_context *ftdi, struct libusb_device *dev);
void __wrap_ftdi_list_free(struct ftdi_device_list **devlist);
int __wrap_ftdi_usb_reset(struct ftdi_context *ftdi);
int __wrap_ftdi_set_latency_timer(struct ftdi_context *ftdi, unsigned char latency);
int __wrap_ftdi_set_bitmode(struct ftdi_context *ftdi, unsigned char bitmask, unsigned char mode);
int __wrap_ftdi_write_data(struct ftdi_context *ftdi, const unsigned char *buf, int size);
int __wrap_ftdi_read_data(struct ftdi_context *ftdi, unsigned char *buf, int size);
int __wrap_ftdi_usb_close(struct ftdi_context *ftdi);
const char *__wrap_ftdi_get_error_string(struct ftdi_context *ftdi);

#endif /* LIBFTDI_WRAPS_H */
