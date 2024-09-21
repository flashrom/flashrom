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

#include "lifecycle.h"

#if CONFIG_RAIDEN_DEBUG_SPI == 1
static ssize_t raiden_debug_libusb_get_device_list(void *state, libusb_context *ctx, libusb_device ***list)
{
	*list = calloc(1, sizeof(**list));
	assert_non_null(*list);

	/*
	 * libusb_device is opaque type, it is tossed around between libusb functions but always
	 * stays opaque to the caller.
	 * Given that all libusb functions are mocked in tests, and raiden_debug test is mocking
	 * only one device, we don't need to initialise libusb_device.
	 */
	return 1;
}

static void raiden_debug_libusb_free_device_list(void *state, libusb_device **list, int unref_devices)
{
	free(list);
}

static int raiden_debug_libusb_get_device_descriptor(
		void *state, libusb_device *dev, struct libusb_device_descriptor *desc)
{
	desc->idVendor = 0x18D1; /* GOOGLE_VID */
	desc->idProduct = 0;
	desc->bNumConfigurations = 1;

	return 0;
}

static int raiden_debug_libusb_get_config_descriptor(
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

	/* in endpoint */
	tmp_endpoint[0].bEndpointAddress = 0x80;
	tmp_endpoint[0].bmAttributes = 0x2;
	/* out endpoint */
	tmp_endpoint[1].bEndpointAddress = 0x0;
	tmp_endpoint[1].bmAttributes = 0x2;

	tmp_interface_desc->bInterfaceClass = 0xff; /* LIBUSB_CLASS_VENDOR_SPEC */
	tmp_interface_desc->bInterfaceSubClass = 0x51; /* GOOGLE_RAIDEN_SPI_SUBCLASS */
	tmp_interface_desc->bInterfaceProtocol = 0x01; /* GOOGLE_RAIDEN_SPI_PROTOCOL_V1 */
	tmp_interface_desc->bNumEndpoints = 2; /* in_endpoint and out_endpoint */
	tmp_interface_desc->endpoint = tmp_endpoint;

	tmp_interface->num_altsetting = 1;
	tmp_interface->altsetting = tmp_interface_desc;

	(*config)->bConfigurationValue = 0;
	(*config)->bNumInterfaces = 1;
	(*config)->interface = tmp_interface;

	return 0;
}

static void raiden_debug_libusb_free_config_descriptor(void *state, struct libusb_config_descriptor *config)
{
	free((void *)config->interface->altsetting->endpoint);
	free((void *)config->interface->altsetting);
	free((void *)config->interface);
	free(config);
}

void raiden_debug_basic_lifecycle_test_success(void **state)
{
	struct io_mock_fallback_open_state raiden_debug_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock raiden_debug_io = {
		.libusb_get_device_list = raiden_debug_libusb_get_device_list,
		.libusb_free_device_list = raiden_debug_libusb_free_device_list,
		.libusb_get_device_descriptor = raiden_debug_libusb_get_device_descriptor,
		.libusb_get_config_descriptor = raiden_debug_libusb_get_config_descriptor,
		.libusb_free_config_descriptor = raiden_debug_libusb_free_config_descriptor,
		.fallback_open_state = &raiden_debug_fallback_open_state,
	};

	char raiden_debug_param[32];

	snprintf(raiden_debug_param, sizeof(raiden_debug_param),
                 "address=%d", USB_DEVICE_ADDRESS);
	run_basic_lifecycle(state, &raiden_debug_io, &programmer_raiden_debug_spi, raiden_debug_param);
}

void raiden_debug_targetAP_basic_lifecycle_test_success(void **state)
{
	struct io_mock_fallback_open_state raiden_debug_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock raiden_debug_io = {
		.libusb_get_device_list = raiden_debug_libusb_get_device_list,
		.libusb_free_device_list = raiden_debug_libusb_free_device_list,
		.libusb_get_device_descriptor = raiden_debug_libusb_get_device_descriptor,
		.libusb_get_config_descriptor = raiden_debug_libusb_get_config_descriptor,
		.libusb_free_config_descriptor = raiden_debug_libusb_free_config_descriptor,
		.fallback_open_state = &raiden_debug_fallback_open_state,
	};

	char raiden_debug_param[32];
	snprintf(raiden_debug_param, sizeof(raiden_debug_param),
                 "address=%d,target=AP", USB_DEVICE_ADDRESS);
	run_basic_lifecycle(state, &raiden_debug_io, &programmer_raiden_debug_spi, raiden_debug_param);
}

void raiden_debug_targetEC_basic_lifecycle_test_success(void **state)
{
	struct io_mock_fallback_open_state raiden_debug_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock raiden_debug_io = {
		.libusb_get_device_list = raiden_debug_libusb_get_device_list,
		.libusb_free_device_list = raiden_debug_libusb_free_device_list,
		.libusb_get_device_descriptor = raiden_debug_libusb_get_device_descriptor,
		.libusb_get_config_descriptor = raiden_debug_libusb_get_config_descriptor,
		.libusb_free_config_descriptor = raiden_debug_libusb_free_config_descriptor,
		.fallback_open_state = &raiden_debug_fallback_open_state,
	};

	char raiden_debug_param[32];
	snprintf(raiden_debug_param, sizeof(raiden_debug_param),
                 "address=%d,target=ec", USB_DEVICE_ADDRESS);
	run_basic_lifecycle(state, &raiden_debug_io, &programmer_raiden_debug_spi, raiden_debug_param);
}

void raiden_debug_target0_basic_lifecycle_test_success(void **state)
{
	struct io_mock_fallback_open_state raiden_debug_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock raiden_debug_io = {
		.libusb_get_device_list = raiden_debug_libusb_get_device_list,
		.libusb_free_device_list = raiden_debug_libusb_free_device_list,
		.libusb_get_device_descriptor = raiden_debug_libusb_get_device_descriptor,
		.libusb_get_config_descriptor = raiden_debug_libusb_get_config_descriptor,
		.libusb_free_config_descriptor = raiden_debug_libusb_free_config_descriptor,
		.fallback_open_state = &raiden_debug_fallback_open_state,
	};

	char raiden_debug_param[32];
	snprintf(raiden_debug_param, sizeof(raiden_debug_param),
                 "address=%d,target=0", USB_DEVICE_ADDRESS);
	run_basic_lifecycle(state, &raiden_debug_io, &programmer_raiden_debug_spi, raiden_debug_param);
}

void raiden_debug_target1_basic_lifecycle_test_success(void **state)
{
	struct io_mock_fallback_open_state raiden_debug_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock raiden_debug_io = {
		.libusb_get_device_list = raiden_debug_libusb_get_device_list,
		.libusb_free_device_list = raiden_debug_libusb_free_device_list,
		.libusb_get_device_descriptor = raiden_debug_libusb_get_device_descriptor,
		.libusb_get_config_descriptor = raiden_debug_libusb_get_config_descriptor,
		.libusb_free_config_descriptor = raiden_debug_libusb_free_config_descriptor,
		.fallback_open_state = &raiden_debug_fallback_open_state,
	};

	char raiden_debug_param[32];
	snprintf(raiden_debug_param, sizeof(raiden_debug_param),
                 "address=%d,target=1", USB_DEVICE_ADDRESS);
	run_basic_lifecycle(state, &raiden_debug_io, &programmer_raiden_debug_spi, raiden_debug_param);
}
#else
	SKIP_TEST(raiden_debug_basic_lifecycle_test_success)
	SKIP_TEST(raiden_debug_targetAP_basic_lifecycle_test_success)
	SKIP_TEST(raiden_debug_targetEC_basic_lifecycle_test_success)
	SKIP_TEST(raiden_debug_target0_basic_lifecycle_test_success)
	SKIP_TEST(raiden_debug_target1_basic_lifecycle_test_success)
#endif /* CONFIG_RAIDEN_DEBUG_SPI */
