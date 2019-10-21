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

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <libusb.h>
#include "programmer.h"

/*
 * Check whether we should filter the current device.
 *
 * The main code filters by VID/PID then calls out to the filter function for extra filtering.
 * The filter function is called twice for each device. Once with handle == NULL to allow the
 * filter to cull devices without opening them and, assuming the first filter does not trigger,
 * also with a real handle to allow the filter to query the device further.
 *
 * Returns true if the device should be skipped.
 */
typedef bool (*filter_func)(struct libusb_device_descriptor *desc, struct libusb_device_handle *handle, void*ctx);

static struct libusb_device_handle *get_by_vid_pid_filter(struct libusb_context *usb_ctx,
		uint16_t vid, uint16_t pid, filter_func filter_fn, void *filter_ctx)
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

		if ((desc.idVendor != vid) || (desc.idProduct != pid))
			continue;

		msg_pdbg("Found USB device %04"PRIx16":%04"PRIx16" at address %d-%d.\n",
			 desc.idVendor, desc.idProduct,
			 libusb_get_bus_number(dev), libusb_get_device_address(dev));

		/* allow filters to trigger before the device is opened */
		if (filter_fn(&desc, NULL, filter_ctx))
			continue;

		res = libusb_open(dev, &handle);
		if (res != 0) {
			msg_perr("Opening the USB device at address %d-%d failed (%s)!\n",
				 libusb_get_bus_number(dev), libusb_get_device_address(dev),
				 libusb_error_name(res));
			break;
		}

		/* filter can also trigger after a device is opened */
		if (filter_fn(&desc, handle, filter_ctx)) {
			libusb_close(handle);
			continue;
		}

		libusb_free_device_list(list, 1);
		return handle;
	}

	libusb_free_device_list(list, 1);
	return NULL;

}

static bool filter_by_serial(struct libusb_device_descriptor *desc, struct libusb_device_handle *handle,
			     void *serialno)
{
	/* Never filter if device is not yet open or when user did not provide a serial number */
	if (!handle || !serialno)
		return false;

	unsigned char myserial[64];
	int res = libusb_get_string_descriptor_ascii(handle, desc->iSerialNumber, myserial, sizeof(myserial));
	if (res < 0) {
		msg_perr("Reading the USB serialno failed (%s)!\n", libusb_error_name(res));
		return true;
	}
	msg_pdbg("Serial number is %s\n", myserial);

	/* Filter out any serial number that does not commence with serialno */
	return 0 != strncmp(serialno, (char *)myserial, strlen(serialno));
}

struct libusb_device_handle *usb_dev_get_by_vid_pid_serial(
		struct libusb_context *usb_ctx, uint16_t vid, uint16_t pid, const char *serialno)
{
	return get_by_vid_pid_filter(usb_ctx, vid, pid, filter_by_serial, (void *)serialno);
}

static bool filter_by_number(struct libusb_device_descriptor *desc, struct libusb_device_handle *handle,
			     void *ctx)
{
	/* This filter never triggers once it has allowed the device to be opened */
	if (handle != NULL)
		return false;

	unsigned int *nump = ctx;
	if (*nump) {
		(*nump)--;
		return true;
	}

	return false;
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
	return get_by_vid_pid_filter(usb_ctx, vid, pid, filter_by_number, &num);
}
