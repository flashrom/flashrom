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

#include <stdlib.h>

#include <include/test.h>
#include "io_mock.h"
#include "libusb_wraps.h"

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

void __wrap_libusb_set_debug(libusb_context *ctx, int level)
{
	LOG_ME;
}

int __wrap_libusb_set_option(libusb_context *ctx, int option, ...)
{
	LOG_ME;
	return 0;
}

int __wrap_libusb_open(libusb_device *dev, libusb_device_handle **devh)
{
	LOG_ME;
	return 0;
}

int __wrap_libusb_set_auto_detach_kernel_driver(libusb_device_handle *devh, int enable)
{
	LOG_ME;
	return 0;
}

int __wrap_libusb_detach_kernel_driver(libusb_device_handle *dev_handle, int interface_number)
{
	LOG_ME;
	return 0;
}

int __wrap_libusb_attach_kernel_driver(libusb_device_handle *dev_handle, int interface_number)
{
	LOG_ME;
	return 0;
}

struct libusb_device_handle *__wrap_libusb_open_device_with_vid_pid(
		libusb_context *ctx, uint16_t vendor_id, uint16_t product_id)
{
	LOG_ME;
	return not_null();
}

libusb_device *__wrap_libusb_get_device(libusb_device_handle *dev_handle)
{
	LOG_ME;
	return not_null();
}

ssize_t __wrap_libusb_get_device_list(libusb_context *ctx, libusb_device ***list)
{
	LOG_ME;
	if (get_io() && get_io()->libusb_get_device_list)
		return get_io()->libusb_get_device_list(get_io()->state, ctx, list);
	return 0;
}

void __wrap_libusb_free_device_list(libusb_device **list, int unref_devices)
{
	LOG_ME;
	if (get_io() && get_io()->libusb_free_device_list)
		get_io()->libusb_free_device_list(get_io()->state, list, unref_devices);
}

uint8_t __wrap_libusb_get_bus_number(libusb_device *dev)
{
	LOG_ME;
	return 0;
}

uint8_t __wrap_libusb_get_device_address(libusb_device *dev)
{
	LOG_ME;
	return USB_DEVICE_ADDRESS;
}

int __wrap_libusb_get_device_descriptor(libusb_device *dev, struct libusb_device_descriptor *desc)
{
	LOG_ME;
	if (get_io() && get_io()->libusb_get_device_descriptor)
		return get_io()->libusb_get_device_descriptor(get_io()->state, dev, desc);
	return 0;
}

int __wrap_libusb_get_config_descriptor(
		libusb_device *dev, uint8_t config_index, struct libusb_config_descriptor **config)
{
	LOG_ME;
	if (get_io() && get_io()->libusb_get_config_descriptor)
		return get_io()->libusb_get_config_descriptor(get_io()->state, dev, config_index, config);
	return 0;
}

void __wrap_libusb_free_config_descriptor(struct libusb_config_descriptor *config)
{
	LOG_ME;
	if (get_io() && get_io()->libusb_free_config_descriptor)
		return get_io()->libusb_free_config_descriptor(get_io()->state, config);
	return;
}

int __wrap_libusb_get_configuration(libusb_device_handle *devh, int *config)
{
	LOG_ME;
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

libusb_device *__wrap_libusb_ref_device(libusb_device *dev)
{
	LOG_ME;
	return NULL;
}

void __wrap_libusb_unref_device(libusb_device *dev)
{
	LOG_ME;
}

struct libusb_transfer *__wrap_libusb_alloc_transfer(int iso_packets)
{
	LOG_ME;
	if (get_io() && get_io()->libusb_alloc_transfer)
		return get_io()->libusb_alloc_transfer(get_io()->state, iso_packets);
	return not_null();
}

int __wrap_libusb_submit_transfer(struct libusb_transfer *transfer)
{
	LOG_ME;
	if (get_io() && get_io()->libusb_submit_transfer)
		return get_io()->libusb_submit_transfer(get_io()->state, transfer);
	return 0;
}

void __wrap_libusb_free_transfer(struct libusb_transfer *transfer)
{
	LOG_ME;
	if (get_io() && get_io()->libusb_free_transfer)
		get_io()->libusb_free_transfer(get_io()->state, transfer);
}

int __wrap_libusb_handle_events_timeout(libusb_context *ctx, struct timeval *tv)
{
	LOG_ME;
	if (get_io() && get_io()->libusb_handle_events_timeout)
		get_io()->libusb_handle_events_timeout(get_io()->state, ctx, tv);
	return 0;
}

void __wrap_libusb_exit(libusb_context *ctx)
{
	LOG_ME;
}
