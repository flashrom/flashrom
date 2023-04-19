/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2020, Google Inc. All rights reserved.
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

#include "programmer.h"
#include "spi.h"
#include "usb_device.h"

#include <assert.h>
#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Possibly extract a programmer parameter and use it to initialize the given
 * match value structure.
 */
static void usb_match_value_init(const struct programmer_cfg *cfg,
				 struct usb_match_value *match,
				 char const *parameter)
{
	char *string = extract_programmer_param_str(cfg, parameter);

	match->name = parameter;

	if (string) {
		match->set = 1;
		match->value = strtol(string, NULL, 0);
	} else {
		match->set = 0;
	}

	free(string);
}

#define USB_MATCH_VALUE_INIT(PPARAM, NAME)			\
	usb_match_value_init(PPARAM, &match->NAME, #NAME)

void usb_match_init(const struct programmer_cfg *cfg, struct usb_match *match)
{
	USB_MATCH_VALUE_INIT(cfg, vid);
	USB_MATCH_VALUE_INIT(cfg, pid);
	USB_MATCH_VALUE_INIT(cfg, bus);
	USB_MATCH_VALUE_INIT(cfg, address);
	USB_MATCH_VALUE_INIT(cfg, config);
	USB_MATCH_VALUE_INIT(cfg, interface);
	USB_MATCH_VALUE_INIT(cfg, altsetting);
	USB_MATCH_VALUE_INIT(cfg, class);
	USB_MATCH_VALUE_INIT(cfg, subclass);
	USB_MATCH_VALUE_INIT(cfg, protocol);
}

void usb_match_value_default(struct usb_match_value *value,
			     long int default_value)
{
	if (value->set)
		return;

	value->set   = 1;
	value->value = default_value;
}

/*
 * Match the value against a possible user supplied parameter.
 *
 * Return:
 *     0: The user supplied the given parameter and it did not match the value.
 *     1: Either the user didn't supply the parameter, or they did and it
 *        matches the given value.
 */
static int check_match(struct usb_match_value const *match_value, int value)
{
	int reject = match_value->set && (match_value->value != value);

	if (reject)
		msg_pdbg("USB: Rejecting device because %s = %d != %d\n",
			 match_value->name,
			 value,
			 match_value->value);

	return !reject;
}

/*
 * Allocate a copy of device and add it to the head of the devices list.
 */
static void add_device(struct usb_device *device,
		       struct usb_device **devices)
{
	struct usb_device *copy = malloc(sizeof(*copy));

	assert(copy != NULL);

	*copy = *device;

	copy->next = *devices;
	*devices = copy;

	libusb_ref_device(copy->device);
}

/*
 * Look through the interfaces of the current device config for a match. Stop
 * looking after the first valid match is found.
 *
 * Return:
 *     0: No matching interface was found.
 *     1: A complete match was found and added to the devices list.
 */
static int find_interface(struct usb_match const *match,
			  struct usb_device *current,
			  struct usb_device **devices)
{
	int i, j;

	for (i = 0; i < current->config_descriptor->bNumInterfaces; ++i) {
		struct libusb_interface const *interface;

		interface = &current->config_descriptor->interface[i];

		for (j = 0; j < interface->num_altsetting; ++j) {
			struct libusb_interface_descriptor const *descriptor;

			descriptor = &interface->altsetting[j];

			if (check_match(&match->interface,
					descriptor->bInterfaceNumber) &&
			    check_match(&match->altsetting,
					descriptor->bAlternateSetting) &&
			    check_match(&match->class,
					descriptor->bInterfaceClass) &&
			    check_match(&match->subclass,
					descriptor->bInterfaceSubClass) &&
			    check_match(&match->protocol,
					descriptor->bInterfaceProtocol)) {
				current->interface_descriptor = descriptor;
				add_device(current, devices);
				msg_pdbg("USB: Found matching device\n");
				return 1;
			}
		}
	}

	return 0;
}

/*
 * Look through the configs of the current device for a match.  Stop looking
 * after the first valid match is found.
 *
 * Return:
 *     0: All configurations successfully checked, one may have been added to
 *        the list.
 *     non-zero: There was a failure while checking for a match.
 */
static int find_config(struct usb_match const *match,
		       struct usb_device *current,
		       struct libusb_device_descriptor const *device_descriptor,
		       struct usb_device **devices)
{
	int i;

	for (i = 0; i < device_descriptor->bNumConfigurations; ++i) {
		int ret = LIBUSB(libusb_get_config_descriptor(
				current->device, i,
				&current->config_descriptor));
		if (ret != 0) {
			msg_perr("USB: Failed to get config descriptor");
			return ret;
		}

		if (check_match(&match->config,
				current->config_descriptor->
				bConfigurationValue) &&
		    find_interface(match, current, devices))
			break;

		libusb_free_config_descriptor(current->config_descriptor);
	}

	return 0;
}

int usb_device_find(struct usb_match const *match, struct usb_device **devices)
{
	libusb_device **list;
	ssize_t         count;
	ssize_t         i;

	*devices = NULL;

	int ret = LIBUSB(count = libusb_get_device_list(NULL, &list));
	if (ret != 0) {
		msg_perr("USB: Failed to get device list");
		return ret;
	}

	for (i = 0; i < count; ++i) {
		struct libusb_device_descriptor descriptor;
		struct usb_device               current = {
			.device = list[i],
			.handle = NULL,
			.next   = NULL,
		};

		uint8_t bus     = libusb_get_bus_number(list[i]);
		uint8_t address = libusb_get_device_address(list[i]);

		msg_pdbg("USB: Inspecting device (Bus %d, Address %d)\n",
			 bus,
			 address);

		ret = LIBUSB(libusb_get_device_descriptor(list[i],
							  &descriptor));
		if (ret != 0) {
			msg_perr("USB: Failed to get device descriptor");
			free(*devices);
			*devices = NULL;
			return ret;
		}

		if (check_match(&match->vid,     descriptor.idVendor) &&
		    check_match(&match->pid,     descriptor.idProduct) &&
		    check_match(&match->bus,     bus) &&
		    check_match(&match->address, address)) {
			ret = find_config(match,
					  &current,
					  &descriptor,
					  devices);
			if (ret != 0) {
				msg_perr("USB: Failed to find config");
				return ret;
			}
		}
	}

	libusb_free_device_list(list, 1);

	return (*devices == NULL);
}

/*
 * If the underlying libusb device is not open, open it.
 *
 * Return:
 *     0: The device was already open or was successfully opened.
 *     non-zero: There was a failure while opening the device.
 */
static int usb_device_open(struct usb_device *device)
{
	if (device->handle == NULL) {
		int ret = LIBUSB(libusb_open(device->device, &device->handle));
		if (ret != 0) {
			msg_perr("USB: Failed to open device\n");
			return ret;
		}
	}

	return 0;
}

int usb_device_show(char const *prefix, struct usb_device *device)
{
	struct libusb_device_descriptor descriptor;
	unsigned char                   product[256];
	int ret;

	ret = usb_device_open(device);
	if (ret != 0) {
		msg_perr("USB: Failed to open device\n");
		return ret;
	}

	ret = LIBUSB(libusb_get_device_descriptor(device->device, &descriptor));
	if (ret != 0) {
		msg_perr("USB: Failed to get device descriptor\n");
		return ret;
	}

	ret = LIBUSB(libusb_get_string_descriptor_ascii(
			     device->handle,
			     descriptor.iProduct,
			     product,
			     sizeof(product)));
	if (ret != 0) {
		msg_perr("USB: Failed to get device product string\n");
		return ret;
	}

	product[255] = '\0';

	msg_perr("%sbus=0x%02x,address=0x%02x | %s\n",
		 prefix,
		 libusb_get_bus_number(device->device),
		 libusb_get_device_address(device->device),
		 product);

	return 0;
}

int usb_device_claim(struct usb_device *device)
{
	int current_config;

	int ret = usb_device_open(device);
	if (ret != 0) {
		msg_perr("USB: Failed to open device\n");
		return ret;
	}

	ret = LIBUSB(libusb_get_configuration(device->handle,
					      &current_config));
	if (ret != 0) {
		msg_perr("USB: Failed to get current device configuration\n");
		return ret;
	}

	if (current_config != device->config_descriptor->bConfigurationValue) {
		ret = LIBUSB(libusb_set_configuration(
				     device->handle,
				     device->
				     config_descriptor->
				     bConfigurationValue));
		if (ret != 0) {
			msg_perr("USB: Failed to set new configuration from %d to %d\n",
					current_config,
					device->config_descriptor->bConfigurationValue);
			return ret;
		}
	}

	ret = libusb_detach_kernel_driver(device->handle,
		device->interface_descriptor->bInterfaceNumber);
	if (ret != 0 && ret != LIBUSB_ERROR_NOT_FOUND && ret != LIBUSB_ERROR_NOT_SUPPORTED) {
		msg_perr("Cannot detach the existing usb driver. %s\n",
				libusb_error_name(ret));
		return ret;
	}

	ret = LIBUSB(libusb_claim_interface(device->handle,
					    device->
					    interface_descriptor->
					    bInterfaceNumber));
	if (ret != 0) {
		msg_perr("USB: Could not claim device interface %d\n",
				device->interface_descriptor->bInterfaceNumber);
		libusb_attach_kernel_driver(device->handle,
			device->interface_descriptor->bInterfaceNumber);
		return ret;
	}

	if (device->interface_descriptor->bAlternateSetting != 0) {
		ret = LIBUSB(libusb_set_interface_alt_setting(
				     device->handle,
				     device->
				     interface_descriptor->
				     bInterfaceNumber,
				     device->
				     interface_descriptor->
				     bAlternateSetting));
		if (ret != 0) {
			msg_perr("USB: Failed to set alternate setting %d\n",
					device->interface_descriptor->bAlternateSetting);
			return ret;
		}
	}

	return 0;
}

struct usb_device *usb_device_free(struct usb_device *device)
{
	struct usb_device *next = device->next;

	if (device->handle != NULL) {
		libusb_release_interface(device->handle,
			device->interface_descriptor->bInterfaceNumber);
		libusb_attach_kernel_driver(device->handle,
			device->interface_descriptor->bInterfaceNumber);
		libusb_close(device->handle);
	}

	/*
	 * This unref balances the ref added in the add_device function.
	 */
	libusb_unref_device(device->device);
	libusb_free_config_descriptor(device->config_descriptor);

	free(device);

	return next;
}
