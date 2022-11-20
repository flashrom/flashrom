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

#ifndef LIBUSB_WRAPS_H
#define LIBUSB_WRAPS_H

#include "usb_unittests.h"

void *__wrap_usb_dev_get_by_vid_pid_number(
		libusb_context *usb_ctx, uint16_t vid, uint16_t pid, unsigned int num);
int __wrap_libusb_init(libusb_context **ctx);
void __wrap_libusb_set_debug(libusb_context *ctx, int level);
int __wrap_libusb_set_option(libusb_context *ctx, int option, ...);
int __wrap_libusb_open(libusb_device *dev, libusb_device_handle **devh);
int __wrap_libusb_set_auto_detach_kernel_driver(libusb_device_handle *devh, int enable);
int __wrap_libusb_detach_kernel_driver(libusb_device_handle *dev_handle, int interface_number);
int __wrap_libusb_attach_kernel_driver(libusb_device_handle *dev_handle, int interface_number);
struct libusb_device_handle *__wrap_libusb_open_device_with_vid_pid(
		libusb_context *ctx, uint16_t vendor_id, uint16_t product_id);
libusb_device *__wrap_libusb_get_device(libusb_device_handle *dev_handle);
ssize_t __wrap_libusb_get_device_list(libusb_context *ctx, libusb_device ***list);
void __wrap_libusb_free_device_list(libusb_device **list, int unref_devices);
uint8_t __wrap_libusb_get_bus_number(libusb_device *dev);
uint8_t __wrap_libusb_get_device_address(libusb_device *dev);
int __wrap_libusb_get_device_descriptor(libusb_device *dev, struct libusb_device_descriptor *desc);
int __wrap_libusb_get_config_descriptor(
		libusb_device *dev, uint8_t config_index, struct libusb_config_descriptor **config);
void __wrap_libusb_free_config_descriptor(struct libusb_config_descriptor *config);
int __wrap_libusb_get_configuration(libusb_device_handle *devh, int *config);
int __wrap_libusb_set_configuration(libusb_device_handle *devh, int config);
int __wrap_libusb_claim_interface(libusb_device_handle *devh, int interface_number);
int __wrap_libusb_control_transfer(libusb_device_handle *devh, uint8_t bmRequestType,
		uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data,
		uint16_t wLength, unsigned int timeout);
int __wrap_libusb_release_interface(libusb_device_handle *devh, int interface_number);
void __wrap_libusb_close(libusb_device_handle *devh);
libusb_device *__wrap_libusb_ref_device(libusb_device *dev);
void __wrap_libusb_unref_device(libusb_device *dev);
struct libusb_transfer *__wrap_libusb_alloc_transfer(int iso_packets);
int __wrap_libusb_submit_transfer(struct libusb_transfer *transfer);
void __wrap_libusb_free_transfer(struct libusb_transfer *transfer);
int __wrap_libusb_handle_events_timeout(libusb_context *ctx, struct timeval *tv);
void __wrap_libusb_exit(libusb_context *ctx);

#endif /* LIBUSB_WRAPS_H */
