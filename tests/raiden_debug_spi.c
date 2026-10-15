/*
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2021 Google LLC
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

struct raiden_debug_v2_mock_state {
	bool is_configured;
	bool write_only;
	unsigned int read_cnt;
	uint8_t read_payload[256];
};

static int raiden_debug_v2_libusb_get_config_descriptor(
		void *state, libusb_device *dev, uint8_t config_index, struct libusb_config_descriptor **config)
{
	int ret = raiden_debug_libusb_get_config_descriptor(state, dev, config_index, config);
	if (ret == 0 && *config && (*config)->interface &&
	    (*config)->interface->altsetting) {
		struct libusb_interface_descriptor *desc =
			(struct libusb_interface_descriptor *)(*config)->interface->altsetting;
		desc->bInterfaceProtocol = 0x02; /* GOOGLE_RAIDEN_SPI_PROTOCOL_V2 */
	}
	return ret;
}

static int raiden_debug_v2_libusb_bulk_transfer(void *state, libusb_device_handle *devh,
		unsigned char endpoint, unsigned char *data, int length, int *actual_length,
		unsigned int timeout)
{
	struct raiden_debug_v2_mock_state *s = state;

	if ((endpoint & 0x80) == 0) { /* OUT endpoint (0x00) */
		if (actual_length)
			*actual_length = length;
		return 0;
	}

	/* IN endpoint (0x80) */
	if (!s->is_configured) {
		/* Response to USB_SPI_PKT_ID_CMD_GET_USB_SPI_CONFIG */
		struct {
			uint16_t packet_id;
			uint16_t max_write_count;
			uint16_t max_read_count;
			uint16_t feature_bitmap;
		} __attribute__((packed)) rsp = {
			.packet_id = 1, /* USB_SPI_PKT_ID_RSP_USB_SPI_CONFIG */
			.max_write_count = 2040,
			.max_read_count = 2040,
			.feature_bitmap = 1,
		};
		memset(data, 0, length);
		memcpy(data, &rsp, sizeof(rsp));
		if (actual_length)
			*actual_length = sizeof(rsp);
		s->is_configured = true;
		return 0;
	}

	if (s->write_only) {
		/* Write-only response: 4-byte START packet with status 0 */
		struct {
			uint16_t packet_id;
			uint16_t status_code;
		} __attribute__((packed)) rsp_start = {
			.packet_id = 5, /* USB_SPI_PKT_ID_RSP_TRANSFER_START */
			.status_code = 0,
		};
		memset(data, 0, length);
		memcpy(data, &rsp_start, sizeof(rsp_start));
		if (actual_length)
			*actual_length = sizeof(rsp_start);
		return 0;
	}

	if (s->read_cnt > 0) {
		memset(data, 0, length);
		/* Packet 1: START packet (64 bytes) with 60 bytes payload */
		struct {
			uint16_t packet_id;
			uint16_t status_code;
			uint8_t payload[60];
		} __attribute__((packed)) *pkt1 = (void *)data;
		pkt1->packet_id = 5; /* USB_SPI_PKT_ID_RSP_TRANSFER_START */
		pkt1->status_code = 0;
		memcpy(pkt1->payload, s->read_payload, 60);

		/* Packet 2: CONTINUE packet (64 bytes) with remaining 60 bytes */
		if (s->read_cnt > 60 && length >= 128) {
			struct {
				uint16_t packet_id;
				uint16_t data_index;
				uint8_t payload[60];
			} __attribute__((packed)) *pkt2 = (void *)(data + 64);
			pkt2->packet_id = 6; /* USB_SPI_PKT_ID_RSP_TRANSFER_CONTINUE */
			pkt2->data_index = 60;
			memcpy(pkt2->payload, s->read_payload + 60, s->read_cnt - 60);
		}

		if (actual_length)
			*actual_length = (s->read_cnt > 60) ? 128 : 64;
		return 0;
	}

	return -1;
}

void raiden_debug_v2_read_response_batched_test_success(void **state)
{
	(void) state;
	struct raiden_debug_v2_mock_state mock_state = {
		.is_configured = false,
		.write_only = false,
		.read_cnt = 120,
	};
	for (int i = 0; i < 120; i++)
		mock_state.read_payload[i] = (uint8_t)(i + 0xa0);

	struct io_mock_fallback_open_state raiden_debug_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock raiden_debug_v2_io = {
		.state = &mock_state,
		.libusb_get_device_list = raiden_debug_libusb_get_device_list,
		.libusb_free_device_list = raiden_debug_libusb_free_device_list,
		.libusb_get_device_descriptor = raiden_debug_libusb_get_device_descriptor,
		.libusb_get_config_descriptor = raiden_debug_v2_libusb_get_config_descriptor,
		.libusb_free_config_descriptor = raiden_debug_libusb_free_config_descriptor,
		.libusb_bulk_transfer = raiden_debug_v2_libusb_bulk_transfer,
		.fallback_open_state = &raiden_debug_fallback_open_state,
	};

	io_mock_register(&raiden_debug_v2_io);

	struct flashrom_programmer *flashprog;
	char raiden_debug_param[32];
	snprintf(raiden_debug_param, sizeof(raiden_debug_param), "address=%d", USB_DEVICE_ADDRESS);

	assert_int_equal(0, flashrom_programmer_init(&flashprog, programmer_raiden_debug_spi.name, raiden_debug_param));

	struct flashctx flash = { .mst = &registered_masters[0] };
	unsigned char cmd = 0x03;
	unsigned char readbuf[120] = { 0 };

	assert_int_equal(0, spi_send_command(&flash, 1, 120, &cmd, readbuf));
	assert_memory_equal(readbuf, mock_state.read_payload, 120);

	assert_int_equal(0, flashrom_programmer_shutdown(flashprog));
	io_mock_register(NULL);
}

void raiden_debug_v2_write_only_test_success(void **state)
{
	(void) state;
	struct raiden_debug_v2_mock_state mock_state = {
		.is_configured = false,
		.write_only = true,
		.read_cnt = 0,
	};

	struct io_mock_fallback_open_state raiden_debug_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock raiden_debug_v2_io = {
		.state = &mock_state,
		.libusb_get_device_list = raiden_debug_libusb_get_device_list,
		.libusb_free_device_list = raiden_debug_libusb_free_device_list,
		.libusb_get_device_descriptor = raiden_debug_libusb_get_device_descriptor,
		.libusb_get_config_descriptor = raiden_debug_v2_libusb_get_config_descriptor,
		.libusb_free_config_descriptor = raiden_debug_libusb_free_config_descriptor,
		.libusb_bulk_transfer = raiden_debug_v2_libusb_bulk_transfer,
		.fallback_open_state = &raiden_debug_fallback_open_state,
	};

	io_mock_register(&raiden_debug_v2_io);

	struct flashrom_programmer *flashprog;
	char raiden_debug_param[32];
	snprintf(raiden_debug_param, sizeof(raiden_debug_param), "address=%d", USB_DEVICE_ADDRESS);

	assert_int_equal(0, flashrom_programmer_init(&flashprog, programmer_raiden_debug_spi.name, raiden_debug_param));

	struct flashctx flash = { .mst = &registered_masters[0] };
	unsigned char cmd[4] = { 0x06, 0x01, 0x02, 0x03 };

	assert_int_equal(0, spi_send_command(&flash, 4, 0, cmd, NULL));

	assert_int_equal(0, flashrom_programmer_shutdown(flashprog));
	io_mock_register(NULL);
}
#else
	SKIP_TEST(raiden_debug_basic_lifecycle_test_success)
	SKIP_TEST(raiden_debug_targetAP_basic_lifecycle_test_success)
	SKIP_TEST(raiden_debug_targetEC_basic_lifecycle_test_success)
	SKIP_TEST(raiden_debug_target0_basic_lifecycle_test_success)
	SKIP_TEST(raiden_debug_target1_basic_lifecycle_test_success)
	SKIP_TEST(raiden_debug_v2_read_response_batched_test_success)
	SKIP_TEST(raiden_debug_v2_write_only_test_success)
#endif /* CONFIG_RAIDEN_DEBUG_SPI */
