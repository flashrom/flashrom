/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2012 James Laird <jhl@mafipulation.org>
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
 * Device should be connected as per "active serial" mode:
 *
 *      +---------+------+-----------+
 *      | SPI     | Pin  |  Altera   |
 *      +---------+------+-----------+
 *      | SCLK    | 1    | DCLK      |
 *      | GND     | 2,10 | GND       |
 *      | VCC     | 4    | VCC(TRGT) |
 *      | MISO    | 7    | DATAOUT   |
 *      | /CS     | 8    | nCS       |
 *      | MOSI    | 9    | ASDI      |
 *      +---------+------+-----------+
 *
 * See also the USB-Blaster Download Cable User Guide: http://www.altera.com/literature/ug/ug_usb_blstr.pdf
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <ftdi.h>
#include "flash.h"
#include "programmer.h"
#include "spi.h"

/* Please keep sorted by vendor ID, then device ID. */
#define ALTERA_VID		0x09fb
#define ALTERA_USBBLASTER_PID	0x6001

static const struct dev_entry devs_usbblasterspi[] = {
	{ALTERA_VID, ALTERA_USBBLASTER_PID, OK, "Altera", "USB-Blaster"},

	{0}
};

struct usbblaster_spi_data {
	struct ftdi_context ftdic;
};

// command bytes
#define BIT_BYTE	(1<<7)	// byte mode (rather than bitbang)
#define BIT_READ	(1<<6)	// read request
#define BIT_LED		(1<<5)
#define BIT_CS		(1<<3)
#define BIT_TMS		(1<<1)
#define BIT_CLK		(1<<0)

#define BUF_SIZE	64

/* The programmer shifts bits in the wrong order for SPI, so we use this method to reverse the bits when needed.
 * http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith32Bits */
static uint8_t reverse(uint8_t b)
{
	return ((b * 0x0802LU & 0x22110LU) | (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

static int send_write(unsigned int writecnt, const unsigned char *writearr, struct ftdi_context ftdic)
{
	uint8_t buf[BUF_SIZE] = { 0 };

	while (writecnt) {
		unsigned int i;
		unsigned int n_write = min(writecnt, BUF_SIZE - 1);
		msg_pspew("writing %d-byte packet\n", n_write);

		buf[0] = BIT_BYTE | (uint8_t)n_write;
		for (i = 0; i < n_write; i++) {
			buf[i+1] = reverse(writearr[i]);
		}
		if (ftdi_write_data(&ftdic, buf, n_write + 1) < 0) {
			msg_perr("USB-Blaster write failed\n");
			return -1;
		}

		writearr += n_write;
		writecnt -= n_write;
	}
	return 0;
}

static int send_read(unsigned int readcnt, unsigned char *readarr, struct ftdi_context ftdic)
{
	int i;
	unsigned int n_read;
	uint8_t buf[BUF_SIZE] = { 0 };

	n_read = readcnt;
	while (n_read) {
		unsigned int payload_size = min(n_read, BUF_SIZE - 1);
		msg_pspew("reading %d-byte packet\n", payload_size);

		buf[0] = BIT_BYTE | BIT_READ | (uint8_t)payload_size;
		if (ftdi_write_data(&ftdic, buf, payload_size + 1) < 0) {
			msg_perr("USB-Blaster write failed\n");
			return -1;
		}
		n_read -= payload_size;
	}

	n_read = readcnt;
	while (n_read) {
		int ret = ftdi_read_data(&ftdic, readarr, n_read);
		if (ret < 0) {
			msg_perr("USB-Blaster read failed\n");
			return -1;
		}
		for (i = 0; i < ret; i++) {
			readarr[i] = reverse(readarr[i]);
		}
		n_read -= ret;
		readarr += ret;
	}
	return 0;
}

/* Returns 0 upon success, a negative number upon errors. */
static int usbblaster_spi_send_command(const struct flashctx *flash, unsigned int writecnt, unsigned int readcnt,
				       const unsigned char *writearr, unsigned char *readarr)
{
	struct usbblaster_spi_data *usbblaster_data = flash->mst->spi.data;
	uint8_t cmd;
	int ret = 0;

	cmd = BIT_LED; // asserts /CS
	if (ftdi_write_data(&usbblaster_data->ftdic, &cmd, 1) < 0) {
		msg_perr("USB-Blaster enable chip select failed\n");
		ret = -1;
	}

	if (!ret && writecnt)
		ret = send_write(writecnt, writearr, usbblaster_data->ftdic);

	if (!ret && readcnt)
		ret = send_read(readcnt, readarr, usbblaster_data->ftdic);

	cmd = BIT_CS;
	if (ftdi_write_data(&usbblaster_data->ftdic, &cmd, 1) < 0) {
		msg_perr("USB-Blaster disable chip select failed\n");
		ret = -1;
	}

	return ret;
}

static int usbblaster_shutdown(void *data)
{
	free(data);
	return 0;
}

static const struct spi_master spi_master_usbblaster = {
	.max_data_read	= 256,
	.max_data_write	= 256,
	.command	= usbblaster_spi_send_command,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.shutdown	= usbblaster_shutdown,
};

/* Returns 0 upon success, a negative number upon errors. */
static int usbblaster_spi_init(const struct programmer_cfg *cfg)
{
	uint8_t buf[BUF_SIZE + 1] = { 0 };
	struct ftdi_context ftdic;
	struct usbblaster_spi_data *usbblaster_data;

	if (ftdi_init(&ftdic) < 0)
		return -1;

	if (ftdi_usb_open(&ftdic, ALTERA_VID, ALTERA_USBBLASTER_PID) < 0) {
		msg_perr("Failed to open USB-Blaster: %s\n", ftdic.error_str);
		return -1;
	}

	if (ftdi_usb_reset(&ftdic) < 0) {
		msg_perr("USB-Blaster reset failed\n");
		return -1;
	}

	if (ftdi_set_latency_timer(&ftdic, 2) < 0) {
		msg_perr("USB-Blaster set latency timer failed\n");
		return -1;
	}

	if (ftdi_write_data_set_chunksize(&ftdic, 4096) < 0 ||
	    ftdi_read_data_set_chunksize(&ftdic, BUF_SIZE) < 0) {
		msg_perr("USB-Blaster set chunk size failed\n");
		return -1;
	}

	buf[sizeof(buf)-1] = BIT_LED | BIT_CS;
	if (ftdi_write_data(&ftdic, buf, sizeof(buf)) < 0) {
		msg_perr("USB-Blaster reset write failed\n");
		return -1;
	}
	if (ftdi_read_data(&ftdic, buf, sizeof(buf)) < 0) {
		msg_perr("USB-Blaster reset read failed\n");
		return -1;
	}

	usbblaster_data = calloc(1, sizeof(*usbblaster_data));
	if (!usbblaster_data) {
		msg_perr("Unable to allocate space for SPI master data\n");
		return -1;
	}
	usbblaster_data->ftdic = ftdic;

	return register_spi_master(&spi_master_usbblaster, usbblaster_data);
}

const struct programmer_entry programmer_usbblaster_spi = {
	.name			= "usbblaster_spi",
	.type			= USB,
	.devs.dev		= devs_usbblasterspi,
	.init			= usbblaster_spi_init,
};
