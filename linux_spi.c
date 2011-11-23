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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

static int fd = -1;

static int linux_spi_shutdown(void *data);
static int linux_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			const unsigned char *txbuf, unsigned char *rxbuf);
static int linux_spi_read(struct flashchip *flash, uint8_t *buf,
			  unsigned int start, unsigned int len);
static int linux_spi_write_256(struct flashchip *flash, uint8_t *buf,
			       unsigned int start, unsigned int len);

static const struct spi_programmer spi_programmer_linux = {
	.type		= SPI_CONTROLLER_LINUX,
	.max_data_read	= MAX_DATA_UNSPECIFIED, /* TODO? */
	.max_data_write	= MAX_DATA_UNSPECIFIED, /* TODO? */
	.command	= linux_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= linux_spi_read,
	.write_256	= linux_spi_write_256,
};

int linux_spi_init(void)
{
	char *p, *endp, *dev;
	uint32_t speed = 0;

	dev = extract_programmer_param("dev");
	if (!dev || !strlen(dev)) {
		msg_perr("No SPI device given. Use flashrom -p "
			 "linux_spi:dev=/dev/spidevX.Y\n");
		return 1;
	}

	p = extract_programmer_param("speed");
	if (p && strlen(p)) {
		speed = (uint32_t)strtoul(p, &endp, 10) * 1024;
		if (p == endp) {
			msg_perr("%s: invalid clock: %s kHz\n", __func__, p);
			return 1;
		}
	}

	msg_pdbg("Using device %s\n", dev);
	if ((fd = open(dev, O_RDWR)) == -1) {
		msg_perr("%s: failed to open %s: %s\n", __func__,
			 dev, strerror(errno));
		return 1;
	}

	if (speed > 0) {
		if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
			msg_perr("%s: failed to set speed %dHz: %s\n",
				 __func__, speed, strerror(errno));
			close(fd);
			return 1;
		}

		msg_pdbg("Using %d kHz clock\n", speed);
	}

	if (register_shutdown(linux_spi_shutdown, NULL))
		return 1;

	register_spi_programmer(&spi_programmer_linux);

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

static int linux_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			const unsigned char *txbuf, unsigned char *rxbuf)
{
	struct spi_ioc_transfer msg[2] = {
		{
			.tx_buf = (uint64_t)(ptrdiff_t)txbuf,
			.len = writecnt,
		},
		{
			.rx_buf = (uint64_t)(ptrdiff_t)rxbuf,
			.len = readcnt,
		},
	};

	if (fd == -1)
		return -1;

	if (ioctl(fd, SPI_IOC_MESSAGE(2), msg) == -1) {
		msg_cerr("%s: ioctl: %s\n", __func__, strerror(errno));
		return -1;
	}
	return 0;
}

static int linux_spi_read(struct flashchip *flash, uint8_t *buf,
			  unsigned int start, unsigned int len)
{
	return spi_read_chunked(flash, buf, start, len, (unsigned)getpagesize());
}

static int linux_spi_write_256(struct flashchip *flash, uint8_t *buf,
			       unsigned int start, unsigned int len)
{
	return spi_write_chunked(flash, buf, start, len, ((unsigned)getpagesize()) - 4);
}
