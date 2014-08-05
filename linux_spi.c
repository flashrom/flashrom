/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2011 Sven Schnelle <svens@stackframe.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#if CONFIG_LINUX_SPI == 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

static int fd = -1;

static int linux_spi_shutdown(void *data);
static int linux_spi_send_command(struct flashctx *flash, unsigned int writecnt,
				  unsigned int readcnt,
				  const unsigned char *txbuf,
				  unsigned char *rxbuf);
static int linux_spi_read(struct flashctx *flash, uint8_t *buf,
			  unsigned int start, unsigned int len);
static int linux_spi_write_256(struct flashctx *flash, const uint8_t *buf,
			       unsigned int start, unsigned int len);

static const struct spi_master spi_master_linux = {
	.type		= SPI_CONTROLLER_LINUX,
	.max_data_read	= MAX_DATA_UNSPECIFIED, /* TODO? */
	.max_data_write	= MAX_DATA_UNSPECIFIED, /* TODO? */
	.command	= linux_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= linux_spi_read,
	.write_256	= linux_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

int linux_spi_init(void)
{
	char *p, *endp, *dev;
	uint32_t speed_hz = 0;
	/* FIXME: make the following configurable by CLI options. */
	/* SPI mode 0 (beware this also includes: MSB first, CS active low and others */
	const uint8_t mode = SPI_MODE_0;
	const uint8_t bits = 8;

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
			 "linux_spi:dev=/dev/spidevX.Y\n");
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

	if (register_shutdown(linux_spi_shutdown, NULL))
		return 1;
	/* We rely on the shutdown function for cleanup from here on. */

	if (speed_hz > 0) {
		if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) == -1) {
			msg_perr("%s: failed to set speed to %d Hz: %s\n",
				 __func__, speed_hz, strerror(errno));
			return 1;
		}

		msg_pdbg("Using %d kHz clock\n", speed_hz/1000);
	}

	if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) {
		msg_perr("%s: failed to set SPI mode to 0x%02x: %s\n",
			 __func__, mode, strerror(errno));
		return 1;
	}

	if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
		msg_perr("%s: failed to set the number of bits per SPI word to %u: %s\n",
			 __func__, bits == 0 ? 8 : bits, strerror(errno));
		return 1;
	}

	register_spi_master(&spi_master_linux);

	return 0;
}

static int linux_spi_shutdown(void *data)
{
	if (fd != -1) {
		close(fd);
		fd = -1;
	}
	return 0;
}

static int linux_spi_send_command(struct flashctx *flash, unsigned int writecnt,
				  unsigned int readcnt,
				  const unsigned char *txbuf,
				  unsigned char *rxbuf)
{
	int iocontrol_code;
	struct spi_ioc_transfer msg[2] = {
		{
			.tx_buf = (uint64_t)(uintptr_t)txbuf,
			.len = writecnt,
		},
		{
			.rx_buf = (uint64_t)(uintptr_t)rxbuf,
			.len = readcnt,
		},
	};

	if (fd == -1)
		return -1;
	/* The implementation currently does not support requests that
	   don't start with sending a command. */
	if (writecnt == 0)
		return SPI_INVALID_LENGTH;

	/* Just submit the first (write) request in case there is nothing
	   to read. Otherwise submit both requests. */
	if (readcnt == 0)
		iocontrol_code = SPI_IOC_MESSAGE(1);
	else
		iocontrol_code = SPI_IOC_MESSAGE(2);

	if (ioctl(fd, iocontrol_code, msg) == -1) {
		msg_cerr("%s: ioctl: %s\n", __func__, strerror(errno));
		return -1;
	}
	return 0;
}

static int linux_spi_read(struct flashctx *flash, uint8_t *buf,
			  unsigned int start, unsigned int len)
{
	return spi_read_chunked(flash, buf, start, len,
				(unsigned int)getpagesize());
}

static int linux_spi_write_256(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	return spi_write_chunked(flash, buf, start, len,
				((unsigned int)getpagesize()) - 4);
}

#endif // CONFIG_LINUX_SPI == 1
