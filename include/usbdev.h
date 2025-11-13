/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2024 Antonio Vázquez Blanco <antoniovazquezblanco@gmail.com>
 */

#ifndef __USBDEV_H__
#define __USBDEV_H__ 1

#include <libusb.h>

struct libusb_device_handle *usb_dev_get_by_vid_pid_serial(
		struct libusb_context *usb_ctx, uint16_t vid, uint16_t pid, const char *serialno);
struct libusb_device_handle *usb_dev_get_by_vid_pid_number(
		struct libusb_context *usb_ctx, uint16_t vid, uint16_t pid, unsigned int num);

#endif /* __USBDEV_H__ */
