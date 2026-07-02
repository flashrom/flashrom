/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2026 Abdelkader Boudih <coreboot@seuros.com>
 */

#include <stdlib.h>
#include <string.h>

#include "lifecycle.h"

#if CONFIG_CH347_SPI == 1

/* Constants mirrored from ch347_spi.c */
#define WRITE_EP		0x06
#define READ_EP			0x86
#define CH347_CMD_SPI_OUT	0xC4
#define CH347_CMD_SPI_IN	0xC3

/*
 * The CH347 speaks a small packet protocol over two bulk endpoints:
 *   OUT: [cmd][len_lo][len_hi][payload...]
 *   IN : [cmd][len_lo][len_hi][data...]
 * The driver only needs config to be acknowledged for init, and an RDID
 * response framed correctly for probe. We track the last SPI opcode and read
 * count seen on WRITE_EP so the following READ_EP returns a matching frame.
 */
struct ch347_spi_io_state {
	uint8_t last_spi_cmd;
	unsigned int readcnt;
	int expect_in;
};

static int ch347_spi_libusb_bulk_transfer(void *state, libusb_device_handle *devh, unsigned char endpoint,
		unsigned char *data, int length, int *actual_length, unsigned int timeout)
{
	struct ch347_spi_io_state *s = state;

	if (endpoint == WRITE_EP) {
		if (length >= 4 && data[0] == CH347_CMD_SPI_OUT)
			s->last_spi_cmd = data[3];
		if (length >= 7 && data[0] == CH347_CMD_SPI_IN) {
			s->expect_in = 1;
			s->readcnt = data[3] | (data[4] << 8) | (data[5] << 16) | (data[6] << 24);
		}
		if (actual_length)
			*actual_length = length;
		return 0;
	}

	/* READ_EP */
	if (s->expect_in) {
		unsigned int n = s->readcnt;
		s->expect_in = 0;
		if (n > (unsigned int)(length - 3))
			n = length - 3;
		memset(data, 0, length);
		data[0] = CH347_CMD_SPI_IN;
		data[1] = n & 0xFF;
		data[2] = (n >> 8) & 0xFF;
		/* Answer RDID (0x9F) with the W25Q128.V id; other reads read as 0. */
		if (s->last_spi_cmd == 0x9F && n >= 3) {
			data[3] = 0xEF; /* WINBOND_NEX_ID */
			data[4] = 0x40; /* W25Q128_V high */
			data[5] = 0x18; /* W25Q128_V low */
		}
		if (actual_length)
			*actual_length = 3 + n;
		return 0;
	}

	/* Benign write-response read. */
	if (actual_length)
		*actual_length = length;
	return 0;
}

#define CH347_SPI_IO(st) {						\
	.state = (st),							\
	.libusb_bulk_transfer = &ch347_spi_libusb_bulk_transfer,	\
	.fallback_open_state = &ch347_fallback_open_state,		\
}

static struct io_mock_fallback_open_state ch347_fallback_open_state = {
	.noc = 0,
	.paths = { NULL },
};

void ch347_spi_basic_lifecycle_test_success(void **state)
{
	struct ch347_spi_io_state st = { 0 };
	const struct io_mock io = CH347_SPI_IO(&st);
	run_basic_lifecycle(state, &io, &programmer_ch347_spi, "");
}

void ch347_spi_probe_lifecycle_test_success(void **state)
{
	struct ch347_spi_io_state st = { 0 };
	const struct io_mock io = CH347_SPI_IO(&st);
	const char *expected_matched_names[1] = {"W25Q128.V"};
	run_probe_v2_lifecycle(state, &io, &programmer_ch347_spi, "", "W25Q128.V",
				expected_matched_names, 1);
}

#else
	SKIP_TEST(ch347_spi_basic_lifecycle_test_success)
	SKIP_TEST(ch347_spi_probe_lifecycle_test_success)
#endif /* CONFIG_CH347_SPI */
