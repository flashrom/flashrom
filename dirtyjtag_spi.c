/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2021-2022 Jean THOMAS <virgule@jeanthomas.me>
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

/*
 * Driver for the DirtyJTAG project.
 * See https://github.com/jeanthom/dirtyjtag for more info.
 *
 *  SPI-JTAG Pin Mapping
 * |=========|==========|
 * | SPI pin | JTAG pin |
 * |=========|==========|
 * | #CS     | TMS      |
 * | #WP     | SRST     |
 * | #HOLD   | TRST     |
 * | MISO    | TDO      |
 * | MOSI    | TDI      |
 * | CLK     | TCK      |
 * |=========|==========|
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libusb.h>
#include "programmer.h"

struct dirtyjtag_spi_data {
	struct libusb_context *libusb_ctx;
	struct libusb_device_handle *libusb_handle;
};

static const struct dev_entry devs_dirtyjtag_spi[] = {
	{ 0x1209, 0xc0ca, OK, "DirtyJTAG", "JTAG probe" },
	{ 0 },
};

static const char dirtyjtag_write_endpoint = 0x01;
static const char dirtyjtag_read_endpoint = 0x82;
static const int dirtyjtag_timeout = 100 * 10; /* 100 ms */

enum dirtyjtag_command_identifier {
	CMD_STOP = 0x00,
	CMD_INFO = 0x01,
	CMD_FREQ = 0x02,
	CMD_XFER = 0x03,
	CMD_SETSIG = 0x04,
	CMD_GETSIG = 0x05,
	CMD_CLK = 0x06
};

enum dirtyjtag_signal_identifier {
	SIG_TCK = 1 << 1,
	SIG_TDI = 1 << 2,
	SIG_TDO = 1 << 3,
	SIG_TMS = 1 << 4,
	SIG_TRST = 1 << 5,
	SIG_SRST = 1 << 6
};

static int dirtyjtag_send(struct dirtyjtag_spi_data *djtag_data, uint8_t *data, size_t len)
{
	int transferred;
	int ret = libusb_bulk_transfer(djtag_data->libusb_handle,
		dirtyjtag_write_endpoint,
		data,
		len,
		&transferred,
		dirtyjtag_timeout);
	if (ret != 0) {
		msg_perr("%s: failed to send query command\n", __func__);
		return -1;
	}
	if (transferred != (int)len) {
		msg_perr("%s: failed to send whole packet\n", __func__);
		return -1;
	}

	return 0;
}

static int dirtyjtag_receive(struct dirtyjtag_spi_data *djtag_data, uint8_t *data, size_t buffer_len, int expected)
{
	int transferred;
	int ret = libusb_bulk_transfer(djtag_data->libusb_handle,
		dirtyjtag_read_endpoint,
		data,
		buffer_len,
		&transferred,
		dirtyjtag_timeout);
	if (ret != 0) {
		msg_perr("%s: Failed to read SPI commands\n", __func__);
		return -1;
	}

	if (expected != -1 && transferred != expected) {
		msg_perr("%s: failed to meet expected\n", __func__);
		return -1;
	}

	return transferred;
}

static int dirtyjtag_spi_shutdown(void *data)
{
	struct dirtyjtag_spi_data *djtag_data = (struct dirtyjtag_spi_data*)data;
	libusb_release_interface(djtag_data->libusb_handle, 0);
	libusb_attach_kernel_driver(djtag_data->libusb_handle, 0);
	libusb_close(djtag_data->libusb_handle);
	libusb_exit(djtag_data->libusb_ctx);
	free(data);
	return 0;
}

static int dirtyjtag_djtag1_spi_send_command(struct dirtyjtag_spi_data *context,
					     unsigned int writecnt, unsigned int readcnt,
					     const unsigned char *writearr, unsigned char *readarr)
{
	const size_t max_xfer_size = 30; // max transfer size in DJTAG1
	size_t len = writecnt + readcnt;
	size_t num_xfer = (len + max_xfer_size - 1 ) / max_xfer_size; // ceil(len/max_xfer_size)

	uint8_t *rxtx_buffer = malloc(max_xfer_size * num_xfer);
	if (!rxtx_buffer) {
		msg_perr("%s: Failed rxtx_buffer allocation\n", __func__);
		return -1;
	}

	memcpy(rxtx_buffer, writearr, writecnt);
	for (size_t i = 0; i < num_xfer; i++) {
		const size_t xfer_offset = i * max_xfer_size;
		size_t txn_size = max_xfer_size;
		if (i == num_xfer-1 && len % max_xfer_size != 0)
			txn_size = len % max_xfer_size;

		uint8_t transfer_buffer[32] = {
			CMD_XFER,
			txn_size * 8
		};
		memcpy(transfer_buffer + 2, rxtx_buffer + xfer_offset, txn_size);

		if (dirtyjtag_send(context, transfer_buffer, sizeof(transfer_buffer)))
			goto cleanup_fail;

		if (dirtyjtag_receive(context, transfer_buffer, sizeof(transfer_buffer), 32) < 0)
			goto cleanup_fail;

		memcpy(rxtx_buffer + xfer_offset, transfer_buffer, txn_size);
	}
	memcpy(readarr, rxtx_buffer + writecnt, readcnt);

	free(rxtx_buffer);

	uint8_t tms_reset_buffer[] = {
		CMD_SETSIG,
		SIG_TMS,
		SIG_TMS,

		CMD_STOP,
	};
	dirtyjtag_send(context, tms_reset_buffer, sizeof(tms_reset_buffer));

	return 0;

cleanup_fail:
	free(rxtx_buffer);
	return -1;
}

static int dirtyjtag_spi_spi_send_command(const struct flashctx *flash,
					  unsigned int writecnt, unsigned int readcnt,
					  const unsigned char *writearr, unsigned char *readarr)
{
	struct dirtyjtag_spi_data *djtag_data = flash->mst->spi.data;
	return dirtyjtag_djtag1_spi_send_command(djtag_data, writecnt, readcnt, writearr, readarr);
}

static const struct spi_master spi_master_dirtyjtag_spi = {
	.features	= SPI_MASTER_4BA,
	.max_data_read	= MAX_DATA_READ_UNLIMITED,
	.max_data_write	= MAX_DATA_WRITE_UNLIMITED,
	.command	= dirtyjtag_spi_spi_send_command,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.write_aai	= default_spi_write_aai,
	.shutdown	= dirtyjtag_spi_shutdown,
};

static int dirtyjtag_spi_init(const struct programmer_cfg *cfg)
{
	struct libusb_device_handle *handle = NULL;
	struct dirtyjtag_spi_data *djtag_data = NULL;

	djtag_data = calloc(1, sizeof(struct dirtyjtag_spi_data));
	if (djtag_data == NULL) {
		msg_perr("%s: failed to allocate internal driver data structure\n", __func__);
		return -1;
	}

	int ret = libusb_init(&djtag_data->libusb_ctx);
	if (ret < 0) {
		msg_perr("%s: couldn't initialize libusb!\n", __func__);
		goto cleanup_djtag_struct;
	}

#if LIBUSB_API_VERSION < 0x01000106
	libusb_set_debug(djtag_data->libusb_ctx, 3);
#else
	libusb_set_option(djtag_data->libusb_ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
#endif

	uint16_t vid = devs_dirtyjtag_spi[0].vendor_id;
	uint16_t pid = devs_dirtyjtag_spi[0].device_id;
	handle = libusb_open_device_with_vid_pid(djtag_data->libusb_ctx, vid, pid);
	if (handle == NULL) {
		msg_perr("%s: couldn't open device %04x:%04x.\n", __func__, vid, pid);
		goto cleanup_libusb_ctx;
	}

	ret = libusb_detach_kernel_driver(handle, 0);
	if (ret != 0 && ret != LIBUSB_ERROR_NOT_FOUND) {
		msg_pwarn("Cannot detach the existing USB driver. Claiming the interface may fail. %s\n",
			libusb_error_name(ret));
	}

	ret = libusb_claim_interface(handle, 0);
	if (ret != 0) {
		msg_perr("%s: failed to claim interface 0: '%s'\n", __func__, libusb_error_name(ret));
		goto cleanup_libusb_handle;
	}

	djtag_data->libusb_handle = handle;

	unsigned long int freq = 100;
	char *tmp = extract_programmer_param_str(cfg, "spispeed");
	if (tmp) {
		char *units = tmp;

		errno = 0;
		freq = strtoul(tmp, &units, 0);
		if (errno) {
			msg_perr("Invalid frequency \"%s\", %s\n", tmp, strerror(errno));
			free(tmp);
			goto cleanup_libusb_handle;
		}

		if (!strcasecmp(units, "hz")) {
			freq /= 1000;
		} else if (!strcasecmp(units, "khz")) {
			/* Do nothing, already in the right unit */
		} else if (!strcasecmp(units, "mhz")) {
			freq *= 1000;
		} else {
			msg_perr("Invalid unit: %s, use hz, khz or mhz\n", units);
			free(tmp);
			goto cleanup_libusb_handle;
		}

		if (freq > UINT16_MAX) {
			msg_perr("%s: Frequency set above DJTAG1 limits (%d kHz)", __func__, UINT16_MAX);
			free(tmp);
			goto cleanup_libusb_handle;
		}

		msg_pinfo("%s: programmer speed set to %lu kHz\n", __func__, freq);
	}
	free(tmp);

	uint8_t commands[] = {
		CMD_SETSIG, /* Set TDI/TCK to low, SRST/TRST/TMS to high */
		SIG_TDI | SIG_TMS | SIG_TCK | SIG_SRST | SIG_TRST,
		SIG_SRST | SIG_TRST | SIG_TMS,

		CMD_FREQ, /* Set frequency */
		(freq >> 8) & 0xff,
		freq & 0xff,

		CMD_STOP,
	};
	ret = dirtyjtag_send(djtag_data, commands, sizeof(commands));
	if (ret != 0) {
		msg_perr("%s: failed to configure DirtyJTAG into initialized state\n", __func__);
		goto cleanup_libusb_handle;
	}

	return register_spi_master(&spi_master_dirtyjtag_spi, (void*)djtag_data);

cleanup_libusb_handle:
	libusb_attach_kernel_driver(handle, 0);
	libusb_close(handle);

cleanup_libusb_ctx:
	libusb_exit(djtag_data->libusb_ctx);

cleanup_djtag_struct:
	free(djtag_data);
	return -1;
}

const struct programmer_entry programmer_dirtyjtag_spi = {
	.name			= "dirtyjtag_spi",
	.type			= USB,
	.devs.dev		= devs_dirtyjtag_spi,
	.init			= dirtyjtag_spi_init,
};
