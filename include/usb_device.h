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

#ifndef USB_DEVICE_H
#define USB_DEVICE_H

/*
 * USB device matching framework
 *
 * This can be used to match a USB device by a number of different parameters.
 * The parameters can be passed on the command line and defaults can be set
 * by the programmer.
 */

#include <libusb.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * The LIBUSB_ERROR macro converts a libusb failure code into an error code that
 * flashrom recognizes. It does so without displaying an error code allowing us
 * to compare error codes against the library enumeration values.
 */
#define LIBUSB_ERROR(error_code)	(0x20000 | -(error_code))

/*
 * The LIBUSB macro converts a libusb failure code into an error code that
 * flashrom recognizes. It also displays additional libusb specific
 * information about the failure.
 */
#define LIBUSB(expression)						\
	({								\
		int libusb_error__ = (expression);			\
									\
		if (libusb_error__ < 0) {				\
			msg_perr("libusb error: %s:%d %s\n",		\
				 __FILE__,				\
				 __LINE__,				\
				 libusb_error_name(libusb_error__));	\
			libusb_error__ = LIBUSB_ERROR(libusb_error__);	\
		} else {						\
			libusb_error__ = 0;				\
		}							\
									\
		libusb_error__;						\
	})

/*
 * Returns true if the error code falls within the range of valid libusb
 * error codes.
 */
static inline bool usb_device_is_libusb_error(int error_code)
{
	return (0x20000 <= error_code && error_code < 0x20064);
}

/*
 * A USB match and associated value struct are used to encode the information
 * about a device against which we wish to match.  If the value of a
 * usb_match_value has been set then a device must match that value.  The name
 * of the usb_match_value is used to fetch the programmer parameter from the
 * flashrom command line and is the same as the name of the corresponding
 * field in usb_match.
 */
struct usb_match_value {
	char const *name;
	int         value;
	int         set;
};

struct usb_match {
	struct usb_match_value bus;
	struct usb_match_value address;
	struct usb_match_value vid;
	struct usb_match_value pid;
	struct usb_match_value serial;
	struct usb_match_value config;
	struct usb_match_value interface;
	struct usb_match_value altsetting;
	struct usb_match_value class;
	struct usb_match_value subclass;
	struct usb_match_value protocol;
};

/*
 * Initialize a usb_match structure so that each value's name matches the
 * values name in the usb_match structure (so bus.name == "bus"...), and
 * look for each value in the flashrom command line via
 * extract_programmer_param_str.  If the value is found convert it to an integer
 * using strtol, accepting hex, decimal and octal encoding.
 */
void usb_match_init(const struct programmer_cfg *cfg, struct usb_match *match);

/*
 * Add a default value to a usb_match_value.  This must be done after calling
 * usb_match_init.  If usb_match_init already set the value of a usb_match_value
 * we do nothing, otherwise set the value to default_value.  This ensures that
 * parameters passed on the command line override defaults.
 */
void usb_match_value_default(struct usb_match_value *match,
			     long int default_value);

/*
 * The usb_device structure is an entry in a linked list of devices that were
 * matched by usb_device_find.
 */
struct usb_device {
	struct libusb_device                     *device;
	struct libusb_config_descriptor          *config_descriptor;
	struct libusb_interface_descriptor const *interface_descriptor;

	/*
	 * Initially NULL, the libusb_device_handle is only valid once the
	 * usb_device has been successfully passed to usb_device_show or
	 * usb_device_claim.
	 */
	struct libusb_device_handle *handle;

	/*
	 * Link to next device, or NULL
	 */
	struct usb_device *next;
};

/*
 * Find and return a list of all compatible devices.  Each device is added to
 * the list with its first valid configuration and interface.  If an alternate
 * configuration (config, interface, altsetting...) is desired the specifics
 * can be supplied as programmer parameters.
 *
 * Return:
 *     0: At least one matching device was found.
 *     1: No matching devices were found.
 */
int usb_device_find(struct usb_match const *match, struct usb_device **devices);

/*
 * Display the devices bus and address as well as its product string.  The
 * underlying libusb device is opened if it is not already open.
 *
 * Return:
 *     0: The device information was displayed.
 *     non-zero: There was a failure while displaying the device information.
 */
int usb_device_show(char const *prefix, struct usb_device *device);

/*
 * Open the underlying libusb device, set its config, claim the interface and
 * select the correct alternate interface.
 *
 * Return:
 *     0: The device was successfully claimed.
 *     non-zero: There was a failure while trying to claim the device.
 */
int usb_device_claim(struct usb_device *device);

/*
 * Free a usb_device structure.
 *
 * This ensures that the libusb device is closed and that all allocated
 * handles and descriptors are freed.
 *
 * Return:
 *     The next device in the device list.
 */
struct usb_device *usb_device_free(struct usb_device *device);

#endif /* USB_DEVICE_H */
