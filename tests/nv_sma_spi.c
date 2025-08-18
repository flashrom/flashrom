/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2025 NVIDIA CORPORATION
 */

#include <stdlib.h>
#include <string.h>

#include "lifecycle.h"

#if CONFIG_NV_SMA_SPI == 1

/* Constants from nv_sma_spi.c */
#define NV_SMA_INTERFACE_CLASS     0xFF  /* Vendor Specific */
#define NV_SMA_INTERFACE_SUBCLASS  0x3F  /* Nvidia assigned class */
#define NV_SMA_INTERFACE_PROTOCOL  0x01  /* Protocol v1 */

static ssize_t nv_sma_spi_libusb_get_device_list(void *state, libusb_context *ctx, libusb_device ***list)
{
	*list = calloc(1, sizeof(**list));
	assert_non_null(*list);

	/*
	 * libusb_device is opaque type, it is tossed around between libusb functions but always
	 * stays opaque to the caller.
	 * Given that all libusb functions are mocked in tests, and nv_sma_spi test is mocking
	 * only one device, we don't need to initialise libusb_device.
	 */
	return 1;
}

static void nv_sma_spi_libusb_free_device_list(void *state, libusb_device **list, int unref_devices)
{
	free(list);
}

static int nv_sma_spi_libusb_get_device_descriptor(
		void *state, libusb_device *dev, struct libusb_device_descriptor *desc)
{
	desc->idVendor = 0x0955;  /* NVIDIA_VID */
	desc->idProduct = 0xcf11; /* NV_SMA_PID */
	desc->bNumConfigurations = 1;
	desc->bcdDevice = 0x0100; /* Device version 1.0.0 */

	return 0;
}

static int nv_sma_spi_libusb_get_config_descriptor(
		void *state, libusb_device *dev, uint8_t config_index, struct libusb_config_descriptor **config)
{
	*config = calloc(1, sizeof(**config));
	assert_non_null(*config);

	struct libusb_endpoint_descriptor *tmp_endpoint = calloc(2, sizeof(*tmp_endpoint));
	assert_non_null(tmp_endpoint);
	struct libusb_interface_descriptor *tmp_interface_desc = calloc(1, sizeof(*tmp_interface_desc));
	assert_non_null(tmp_interface_desc);
	struct libusb_interface *tmp_interface = calloc(1, sizeof(*tmp_interface));
	assert_non_null(tmp_interface);

	/* OUT endpoint (write) */
	tmp_endpoint[0].bEndpointAddress = 0x01;
	tmp_endpoint[0].bmAttributes = 0x02; /* Bulk transfer */
	/* IN endpoint (read) */
	tmp_endpoint[1].bEndpointAddress = 0x81;
	tmp_endpoint[1].bmAttributes = 0x02; /* Bulk transfer */

	tmp_interface_desc->bInterfaceClass = NV_SMA_INTERFACE_CLASS;
	tmp_interface_desc->bInterfaceSubClass = NV_SMA_INTERFACE_SUBCLASS;
	tmp_interface_desc->bInterfaceProtocol = NV_SMA_INTERFACE_PROTOCOL;
	tmp_interface_desc->bInterfaceNumber = 0;
	tmp_interface_desc->bNumEndpoints = 2; /* in_endpoint and out_endpoint */
	tmp_interface_desc->endpoint = tmp_endpoint;

	tmp_interface->num_altsetting = 1;
	tmp_interface->altsetting = tmp_interface_desc;

	(*config)->bConfigurationValue = 0;
	(*config)->bNumInterfaces = 1;
	(*config)->interface = tmp_interface;

	return 0;
}

static void nv_sma_spi_libusb_free_config_descriptor(void *state, struct libusb_config_descriptor *config)
{
	free((void *)config->interface->altsetting->endpoint);
	free((void *)config->interface->altsetting);
	free((void *)config->interface);
	free(config);
}

static int nv_sma_spi_libusb_bulk_transfer(void *state, libusb_device_handle *devh, unsigned char endpoint,
		unsigned char *data, int length, int *actual_length, unsigned int timeout)
{
	if (data && length == 512) {
		int all_zero = 1;
		for (int i = 0; i < 512; ++i) {
			if (data[i] != 0) {
				all_zero = 0;
				break;
			}
		}
		if (all_zero) {
			if (actual_length)
				*actual_length = 0;
			return 0;
		}
	}
	else if (actual_length)
		*actual_length = length;

	return 0;
}

void nv_sma_spi_basic_lifecycle_test_success(void **state)
{
	struct io_mock_fallback_open_state nv_sma_spi_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock nv_sma_spi_io = {
		.libusb_get_device_list = nv_sma_spi_libusb_get_device_list,
		.libusb_free_device_list = nv_sma_spi_libusb_free_device_list,
		.libusb_get_device_descriptor = nv_sma_spi_libusb_get_device_descriptor,
		.libusb_get_config_descriptor = nv_sma_spi_libusb_get_config_descriptor,
		.libusb_free_config_descriptor = nv_sma_spi_libusb_free_config_descriptor,
		.libusb_bulk_transfer = nv_sma_spi_libusb_bulk_transfer,
		.fallback_open_state = &nv_sma_spi_fallback_open_state,
	};
	run_basic_lifecycle(state, &nv_sma_spi_io, &programmer_nv_sma_spi, "");
}

#else
	SKIP_TEST(nv_sma_spi_basic_lifecycle_test_success)
#endif /* CONFIG_NV_SMA_SPI */
