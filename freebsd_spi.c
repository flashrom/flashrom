/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2011 Sven Schnelle <svens@stackframe.org>
 * Copyright (C) 2018 unrelentingtech <hello@unrelenting.technology>
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

#if CONFIG_FREEBSD_SPI == 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ioccom.h>
#include <sys/spigenio.h>
#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

/* Tested on:
 * Xunlong Orange Pi PC (Allwinner H3) */

/* Same as in spi(8) */
#define	DEFAULT_BUFFER_SIZE	8192

static int fd = -1;

static int freebsd_spi_shutdown(void *data);
static int freebsd_spi_send_command(struct flashctx *flash, unsigned int writecnt,
					unsigned int readcnt,
					const unsigned char *txbuf,
					unsigned char *rxbuf);
static int freebsd_spi_read(struct flashctx *flash, uint8_t *buf,
				unsigned int start, unsigned int len);
static int freebsd_spi_write_256(struct flashctx *flash, const uint8_t *buf,
						 unsigned int start, unsigned int len);

static const struct spi_master spi_master_freebsd = {
	.type		= SPI_CONTROLLER_FREEBSD,
	.features	= SPI_MASTER_4BA,
	.max_data_read	= MAX_DATA_UNSPECIFIED, /* TODO? */
	.max_data_write	= MAX_DATA_UNSPECIFIED, /* TODO? */
	.command	= freebsd_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= freebsd_spi_read,
	.write_256	= freebsd_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

int freebsd_spi_init(void)
{
	char *p, *endp, *dev;
	uint32_t speed_hz = 0;
	/* FIXME: make the following configurable by CLI options. */
	/* SPI mode 0 (beware this also includes: MSB first, CS active low and others */
	const uint8_t mode = 0;

	p = extract_programmer_param("spispeed");
	if (p && strlen(p)) {
		speed_hz = (uint32_t)strtoul(p, &endp, 10) * 1000;
		if (p == endp) {
			msg_perr("%s: invalid clock: %s kHz\n", __func__, p);
			free(p);
			return 1;
		}
	}
	free(p);

	dev = extract_programmer_param("dev");
	if (!dev || !strlen(dev)) {
		msg_perr("No SPI device given. Use flashrom -p "
			 "freebsd_spi:dev=/dev/spigenX.Y\n");
		free(dev);
		return 1;
	}

	msg_pdbg("Using device %s\n", dev);
	if ((fd = open(dev, O_RDWR)) == -1) {
		msg_perr("%s: failed to open %s: %s\n", __func__,
			 dev, strerror(errno));
		free(dev);
		return 1;
	}
	free(dev);

	if (register_shutdown(freebsd_spi_shutdown, NULL))
		return 1;
	/* We rely on the shutdown function for cleanup from here on. */

	if (speed_hz > 0) {
		if (ioctl(fd, SPIGENIOC_SET_CLOCK_SPEED, &speed_hz) == -1) {
			msg_perr("%s: failed to set speed to %d Hz: %s\n",
				 __func__, speed_hz, strerror(errno));
			return 1;
		}

		msg_pdbg("Using %d kHz clock\n", speed_hz/1000);
	}

	if (ioctl(fd, SPIGENIOC_SET_SPI_MODE, &mode) == -1) {
		msg_perr("%s: failed to set SPI mode to 0x%02x: %s\n",
			 __func__, mode, strerror(errno));
		return 1;
	}

	register_spi_master(&spi_master_freebsd);
	return 0;
}

static int freebsd_spi_shutdown(void *data)
{
	if (fd != -1) {
		close(fd);
		fd = -1;
	}
	return 0;
}

static int freebsd_spi_send_command(struct flashctx *flash, unsigned int writecnt,
					unsigned int readcnt,
					const unsigned char *txbuf,
					unsigned char *rxbuf)
{
	if (fd == -1)
		return -1;
	/* The implementation currently does not support requests that
		 don't start with sending a command. */
	if (writecnt == 0)
		return SPI_INVALID_LENGTH;

	/* FreeBSD uses a single buffer for rx and tx. Allocate a temporary one to avoid overwriting anything. */
	size_t tmpcnt = readcnt + writecnt;
	unsigned char *tmpbuf = alloca(tmpcnt);

	bzero(tmpbuf, tmpcnt);
	memcpy(tmpbuf, txbuf, writecnt);

	/* Command/data separation is pretty useless, spi(8) only uses the command. */
	struct spigen_transfer msg = {
		.st_command = {
			.iov_base = (void*)tmpbuf,
			.iov_len = tmpcnt,
		},
		.st_data = {
			.iov_base = NULL,
			.iov_len = 0,
		},
	};

	if (ioctl(fd, SPIGENIOC_TRANSFER, &msg) == -1) {
		msg_cerr("%s: ioctl: %s\n", __func__, strerror(errno));
		return -1;
	}

	if (rxbuf)
		memcpy(rxbuf, tmpbuf + writecnt, readcnt);

	return 0;
}

static int freebsd_spi_read(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	return spi_read_chunked(flash, buf, start, len, DEFAULT_BUFFER_SIZE);
}

static int freebsd_spi_write_256(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	return spi_write_chunked(flash, buf, start, len, DEFAULT_BUFFER_SIZE);
}

#endif // CONFIG_FREEBSD_SPI == 1
