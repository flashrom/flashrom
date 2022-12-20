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
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"
/*
 * Linux versions prior to v4.14-rc7 may need linux/ioctl.h included here due
 * to missing from linux/spi/spidev.h. This was fixed in the following commit:
 * a2b4a79b88b2 spi: uapi: spidev: add missing ioctl header
 */
#include <linux/ioctl.h>
#include <linux/spi/spidev.h>

/* Devices known to work with this module (FIXME: export as struct dev_entry):
 * Beagle Bone Black
 * Raspberry Pi
 * HummingBoard
 */

#define BUF_SIZE_FROM_SYSFS	"/sys/module/spidev/parameters/bufsiz"

struct linux_spi_data {
	int fd;
	size_t max_kernel_buf_size;
};

static int linux_spi_read(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	struct linux_spi_data *spi_data = flash->mst->spi.data;
	/* Older kernels use a single buffer for combined input and output
	   data. So account for longest possible command + address, too. */
	return spi_read_chunked(flash, buf, start, len, spi_data->max_kernel_buf_size - 5);
}

static int linux_spi_write_256(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	struct linux_spi_data *spi_data = flash->mst->spi.data;
	/* 5 bytes must be reserved for longest possible command + address. */
	return spi_write_chunked(flash, buf, start, len, spi_data->max_kernel_buf_size - 5);
}

static int linux_spi_shutdown(void *data)
{
	struct linux_spi_data *spi_data = data;
	close(spi_data->fd);
	free(spi_data);
	return 0;
}

static int linux_spi_send_command(const struct flashctx *flash, unsigned int writecnt,
				  unsigned int readcnt,
				  const unsigned char *txbuf,
				  unsigned char *rxbuf)
{
	struct linux_spi_data *spi_data = flash->mst->spi.data;
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

	if (spi_data->fd == -1)
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

	if (ioctl(spi_data->fd, iocontrol_code, msg) == -1) {
		msg_cerr("%s: ioctl: %s\n", __func__, strerror(errno));
		return -1;
	}
	return 0;
}

static const struct spi_master spi_master_linux = {
	.features	= SPI_MASTER_4BA,
	.max_data_read	= MAX_DATA_UNSPECIFIED, /* TODO? */
	.max_data_write	= MAX_DATA_UNSPECIFIED, /* TODO? */
	.command	= linux_spi_send_command,
	.read		= linux_spi_read,
	.write_256	= linux_spi_write_256,
	.shutdown	= linux_spi_shutdown,
};

/* Read max buffer size from sysfs, or use page size as fallback. */
static size_t get_max_kernel_buf_size()
{
	size_t result = 0;
	FILE *fp;
	fp = fopen(BUF_SIZE_FROM_SYSFS, "r");
	if (!fp) {
		msg_pwarn("Cannot open %s: %s.\n", BUF_SIZE_FROM_SYSFS, strerror(errno));
		goto out;
	}

	char buf[10];
	if (!fgets(buf, sizeof(buf), fp)) {
		if (feof(fp))
			msg_pwarn("Cannot read %s: file is empty.\n", BUF_SIZE_FROM_SYSFS);
		else
			msg_pwarn("Cannot read %s: %s.\n", BUF_SIZE_FROM_SYSFS, strerror(errno));
		goto out;
	}

	long int tmp;
	errno = 0;
	tmp = strtol(buf, NULL, 0);
	if ((tmp < 0) || errno) {
		msg_pwarn("Buffer size %ld from %s seems wrong.\n", tmp, BUF_SIZE_FROM_SYSFS);
	} else {
		msg_pdbg("%s: Using value from %s as max buffer size.\n", __func__, BUF_SIZE_FROM_SYSFS);
		result = (size_t)tmp;
	}

out:
	if (fp)
		fclose(fp);

	if (!result) {
		msg_pdbg("%s: Using page size as max buffer size.\n", __func__);
		result = (size_t)getpagesize();
	}
	return result;
}

static int linux_spi_init(const struct programmer_cfg *cfg)
{
	char *param_str, *endp;
	uint32_t speed_hz = 2 * 1000 * 1000;
	/* FIXME: make the following configurable by CLI options. */
	/* SPI mode 0 (beware this also includes: MSB first, CS active low and others */
	const uint8_t mode = SPI_MODE_0;
	const uint8_t bits = 8;
	int fd;
	size_t max_kernel_buf_size;
	struct linux_spi_data *spi_data;

	param_str = extract_programmer_param_str(cfg, "spispeed");
	if (param_str && strlen(param_str)) {
		speed_hz = (uint32_t)strtoul(param_str, &endp, 10) * 1000;
		if (param_str == endp || speed_hz == 0) {
			msg_perr("%s: invalid clock: %s kHz\n", __func__, param_str);
			free(param_str);
			return 1;
		}
	} else {
		msg_pinfo("Using default %"PRIu32
			  "kHz clock. Use 'spispeed' parameter to override.\n",
			  speed_hz / 1000);
	}
	free(param_str);

	param_str = extract_programmer_param_str(cfg, "dev");
	if (!param_str || !strlen(param_str)) {
		msg_perr("No SPI device given. Use flashrom -p "
			 "linux_spi:dev=/dev/spidevX.Y\n");
		free(param_str);
		return 1;
	}

	msg_pdbg("Using device %s\n", param_str);
	if ((fd = open(param_str, O_RDWR)) == -1) {
		msg_perr("%s: failed to open %s: %s\n", __func__,
			 param_str, strerror(errno));
		free(param_str);
		return 1;
	}
	free(param_str);

	if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) == -1) {
		msg_perr("%s: failed to set speed to %"PRIu32"Hz: %s\n",
			 __func__, speed_hz, strerror(errno));
		goto init_err;
	}
	msg_pdbg("Using %"PRIu32"kHz clock\n", speed_hz / 1000);

	if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) {
		msg_perr("%s: failed to set SPI mode to 0x%02x: %s\n",
			 __func__, mode, strerror(errno));
		goto init_err;
	}

	if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
		msg_perr("%s: failed to set the number of bits per SPI word to %u: %s\n",
			 __func__, bits == 0 ? 8 : bits, strerror(errno));
		goto init_err;
	}

	max_kernel_buf_size = get_max_kernel_buf_size();
	msg_pdbg("%s: max_kernel_buf_size: %zu\n", __func__, max_kernel_buf_size);

	spi_data = calloc(1, sizeof(*spi_data));
	if (!spi_data) {
		msg_perr("Unable to allocated space for SPI master data\n");
		goto init_err;
	}
	spi_data->fd = fd;
	spi_data->max_kernel_buf_size = max_kernel_buf_size;

	return register_spi_master(&spi_master_linux, spi_data);

init_err:
	close(fd);
	return 1;
}

const struct programmer_entry programmer_linux_spi = {
	.name			= "linux_spi",
	.type			= OTHER,
	.devs.note		= "Device files /dev/spidev*.*\n",
	.init			= linux_spi_init,
};
