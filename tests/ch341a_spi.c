/*
 * This file is part of the flashrom project.
 *
 * Copyright 2022 Alexander Goncharov <chat@joursoir.net>
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

#if CONFIG_CH341A_SPI == 1

/* Same macro as in ch341a_spi.c programmer. */
#define WRITE_EP 0x02
#define READ_EP 0x82

struct ch341a_spi_io_state {
	struct libusb_transfer *transfer_out;
	/*
	 * Since the test transfers a data that fits in one CH341 packet, we
	 * don't need an array of these transfers (as is done in the driver code).
	 */
	struct libusb_transfer *transfer_in;
};

static struct libusb_transfer *ch341a_libusb_alloc_transfer(void *state, int iso_packets)
{
	return calloc(1, sizeof(struct libusb_transfer));
}

/*
 * The libusb code stores submitted transfers in their own context. But this
 * function doesn't require a context pointer because libusb stores context
 * pointers in libusb_transfer instances. Since our ch341 driver is using
 * the default context, we store the transfer in our own.
 */
static int ch341a_libusb_submit_transfer(void *state, struct libusb_transfer *transfer)
{
	struct ch341a_spi_io_state *io_state = state;

	assert_true(transfer->endpoint == WRITE_EP || transfer->endpoint == READ_EP);

	if (transfer->endpoint == WRITE_EP) {
		assert_null(io_state->transfer_out);
		io_state->transfer_out = transfer;
	} else if (transfer->endpoint == READ_EP) {
		assert_null(io_state->transfer_in);
		io_state->transfer_in = transfer;
	}

	return 0;
}

static void ch341a_libusb_free_transfer(void *state, struct libusb_transfer *transfer)
{
	free(transfer);
}

/*
 * Handle submitted transfers by pretending that a transfer is completed and
 * invoking its callback (that is the flashrom code).
 */
static int ch341a_libusb_handle_events_timeout(void *state, libusb_context *ctx, struct timeval *tv)
{
	struct ch341a_spi_io_state *io_state = state;

	if (io_state->transfer_out) {
		io_state->transfer_out->status = LIBUSB_TRANSFER_COMPLETED;
		io_state->transfer_out->actual_length = io_state->transfer_out->length;
		io_state->transfer_out->callback(io_state->transfer_out);
		io_state->transfer_out = NULL;
	}

	if (io_state->transfer_in) {
		io_state->transfer_in->buffer[1] = reverse_byte(0xEF); /* WINBOND_NEX_ID */
		io_state->transfer_in->buffer[2] = reverse_byte(0x40); /* WINBOND_NEX_W25Q128_V left byte */
		io_state->transfer_in->buffer[3] = reverse_byte(0x18); /* WINBOND_NEX_W25Q128_V right byte */

		io_state->transfer_in->status = LIBUSB_TRANSFER_COMPLETED;
		io_state->transfer_in->actual_length = io_state->transfer_in->length;
		io_state->transfer_in->callback(io_state->transfer_in);
		io_state->transfer_in = NULL;
	}

	return 0;
}

void ch341a_spi_basic_lifecycle_test_success(void **state)
{
	struct ch341a_spi_io_state ch341a_spi_io_state = { 0 };
	struct io_mock_fallback_open_state ch341a_spi_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock ch341a_spi_io = {
		.state = &ch341a_spi_io_state,
		.libusb_alloc_transfer = &ch341a_libusb_alloc_transfer,
		.libusb_submit_transfer = &ch341a_libusb_submit_transfer,
		.libusb_free_transfer = &ch341a_libusb_free_transfer,
		.libusb_handle_events_timeout = &ch341a_libusb_handle_events_timeout,
		.fallback_open_state = &ch341a_spi_fallback_open_state,
	};

	run_basic_lifecycle(state, &ch341a_spi_io, &programmer_ch341a_spi, "");
}

void ch341a_spi_probe_lifecycle_test_success(void **state)
{
	struct ch341a_spi_io_state ch341a_spi_io_state = { 0 };
	struct io_mock_fallback_open_state ch341a_spi_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock ch341a_spi_io = {
		.state = &ch341a_spi_io_state,
		.libusb_alloc_transfer = &ch341a_libusb_alloc_transfer,
		.libusb_submit_transfer = &ch341a_libusb_submit_transfer,
		.libusb_free_transfer = &ch341a_libusb_free_transfer,
		.libusb_handle_events_timeout = &ch341a_libusb_handle_events_timeout,
		.fallback_open_state = &ch341a_spi_fallback_open_state,
	};

	run_probe_lifecycle(state, &ch341a_spi_io, &programmer_ch341a_spi, "", "W25Q128.V");
}

#else
	SKIP_TEST(ch341a_spi_basic_lifecycle_test_success)
	SKIP_TEST(ch341a_spi_probe_lifecycle_test_success)
#endif /* CONFIG_CH341A_SPI */
