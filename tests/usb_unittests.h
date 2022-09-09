/*
 * This file is part of the flashrom project.
 *
 * Copyright 2022 Google LLC
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

/*
 * This header provides a temporary solution to unblock build system
 * work. The main goal is to remove unconditional dependency on libusb
 * for unit tests. The dependency is still present, but now it is present
 * only when it is needed and only when the header is present in the env.
 *
 * The contents of the file will be modified in a very near future.
 */

#ifndef _USB_UNITTESTS_H_
#define _USB_UNITTESTS_H_

#if CONFIG_RAIDEN_DEBUG_SPI == 1 || CONFIG_DEDIPROG == 1 || CONFIG_CH341A_SPI == 1

#include <libusb.h>

#else

struct libusb_context;
typedef struct libusb_context libusb_context;

struct libusb_device_handle;
typedef struct libusb_device_handle libusb_device_handle;

struct libusb_device_descriptor;
typedef struct libusb_device_descriptor libusb_device_descriptor;

struct libusb_device;
typedef struct libusb_device libusb_device;

struct libusb_config_descriptor;
typedef struct libusb_config_descriptor libusb_config_descriptor;

struct libusb_interface;
typedef struct libusb_interface libusb_interface;

struct libusb_interface_descriptor;
typedef struct libusb_interface_descriptor libusb_interface_descriptor;

struct libusb_endpoint_descriptor;
typedef struct libusb_endpoint_descriptor libusb_endpoint_descriptor;

struct libusb_transfer;

#endif

#endif /* _USB_UNITTESTS_H_ */
