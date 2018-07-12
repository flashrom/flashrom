/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2016 secunet Security Networks AG
 * Copyright (C) 2018 Linaro Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <string.h>
#include <libusb.h>
#include "programmer.h"

struct libusb_device_handle *usb_dev_get_by_vid_pid_serial(
		struct libusb_context *usb_ctx, uint16_t vid, uint16_t pid, const char *serialno)
{
	struct libusb_device **list;
	ssize_t count = libusb_get_device_list(usb_ctx, &list);
	if (count < 0) {
		msg_perr("Getting the USB device list failed (%s)!\n", libusb_error_name(count));
		return NULL;
	}

	ssize_t i = 0;
	for (i = 0; i < count; i++) {
		struct libusb_device *dev = list[i];
		struct libusb_device_descriptor desc;
		struct libusb_device_handle *handle;

		int res = libusb_get_device_descriptor(dev, &desc);
		if (res != 0) {
			msg_perr("Reading the USB device descriptor failed (%s)!\n", libusb_error_name(res));
			continue;
		}

		if ((desc.idVendor != vid) && (desc.idProduct != pid))
			continue;

		msg_pdbg("Found USB device %04"PRIx16":%04"PRIx16" at address %d-%d.\n",
			 desc.idVendor, desc.idProduct,
			 libusb_get_bus_number(dev), libusb_get_device_address(dev));

		res = libusb_open(dev, &handle);
		if (res != 0) {
			msg_perr("Opening the USB device failed (%s)!\n", libusb_error_name(res));
			continue;
		}

		if (serialno) {
			unsigned char myserial[64];
			res = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, myserial,
					sizeof(myserial));
			if (res < 0) {
				msg_perr("Reading the USB serialno failed (%s)!\n", libusb_error_name(res));
				libusb_close(handle);
				continue;
			}
			msg_pdbg("Serial number is %s\n", myserial);

			/* Reject any serial number that does not commence with serialno */
			if (0 != strncmp(serialno, (char *)myserial, strlen(serialno))) {
				libusb_close(handle);
				continue;
			}
		}

		libusb_free_device_list(list, 1);
		return handle;
	}

	libusb_free_device_list(list, 1);
	return NULL;
}

/*
 * This function allows different devices to be targeted based on enumeration order. Different
 * hotplug sequencing (or simply a reboot) may change the enumeration order. This function should
 * only be used if a programmers does not provide an alternative way to identify itself uniquely
 * (such as a unique serial number).
 */
struct libusb_device_handle *usb_dev_get_by_vid_pid_number(
		struct libusb_context *usb_ctx, uint16_t vid, uint16_t pid, unsigned int num)
{
	struct libusb_device **list;
	ssize_t count = libusb_get_device_list(usb_ctx, &list);
	if (count < 0) {
		msg_perr("Getting the USB device list failed (%s)!\n", libusb_error_name(count));
		return NULL;
	}

	struct libusb_device_handle *handle = NULL;
	ssize_t i = 0;
	for (i = 0; i < count; i++) {
		struct libusb_device *dev = list[i];
		struct libusb_device_descriptor desc;
		int err = libusb_get_device_descriptor(dev, &desc);
		if (err != 0) {
			msg_perr("Reading the USB device descriptor failed (%s)!\n", libusb_error_name(err));
			libusb_free_device_list(list, 1);
			return NULL;
		}
		if ((desc.idVendor == vid) && (desc.idProduct == pid)) {
			msg_pdbg("Found USB device %04"PRIx16":%04"PRIx16" at address %d-%d.\n",
				 desc.idVendor, desc.idProduct,
				 libusb_get_bus_number(dev), libusb_get_device_address(dev));
			if (num == 0) {
				err = libusb_open(dev, &handle);
				if (err != 0) {
					msg_perr("Opening the USB device failed (%s)!\n",
						 libusb_error_name(err));
					libusb_free_device_list(list, 1);
					return NULL;
				}
				break;
			}
			num--;
		}
	}
	libusb_free_device_list(list, 1);

	return handle;
}
