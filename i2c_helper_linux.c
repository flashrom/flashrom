/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2020 The Chromium OS Authors
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <stdlib.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "flash.h"
#include "i2c_helper.h"

#define I2C_DEV_PREFIX	"/dev/i2c-"
#define I2C_MAX_BUS	255

int i2c_close(int fd)
{
	return fd == -1 ? 0 : close(fd);
}

int i2c_open(int bus, uint16_t addr, int force)
{
	int ret = -1;
	int fd = -1;

	/* Maximum i2c bus number is 255(3 char), +1 for null terminated string. */
	int path_len = strlen(I2C_DEV_PREFIX) + 4;
	int request = force ? I2C_SLAVE_FORCE : I2C_SLAVE;

	if (bus < 0 || bus > I2C_MAX_BUS) {
		msg_perr("Invalid I2C bus %d.\n", bus);
		return ret;
	}

	char *dev = calloc(1, path_len);
	if (!dev) {
		msg_perr("Unable to allocate space for device name of len %d: %s.\n",
			path_len, strerror(errno));
		goto linux_i2c_open_err;
	}

	ret = snprintf(dev, path_len, "%s%d", I2C_DEV_PREFIX, bus);
	if (ret < 0) {
		msg_perr("Unable to join bus number to device name: %s.\n", strerror(errno));
		goto linux_i2c_open_err;
	}

	fd = open(dev, O_RDWR);
	if (fd < 0) {
		msg_perr("Unable to open I2C device %s: %s.\n", dev, strerror(errno));
		ret = fd;
		goto linux_i2c_open_err;
	}

	ret = ioctl(fd, request, addr);
	if (ret < 0) {
		msg_perr("Unable to set I2C slave address to 0x%02x: %s.\n", addr, strerror(errno));
		i2c_close(fd);
		goto linux_i2c_open_err;
	}

linux_i2c_open_err:
	if (dev)
		free(dev);

	return ret ? ret : fd;
}

int i2c_read(int fd, uint16_t addr, i2c_buffer_t *buf)
{
	if (buf->len == 0)
		return 0;

	int ret = ioctl(fd, I2C_SLAVE, addr);
	if (ret < 0) {
		msg_perr("Unable to set I2C slave address to 0x%02x: %s.\n", addr, strerror(errno));
		return ret;
	}

	return read(fd, buf->buf, buf->len);
}

int i2c_write(int fd, uint16_t addr, const i2c_buffer_t *buf)
{
	if (buf->len == 0)
		return 0;

	int ret = ioctl(fd, I2C_SLAVE, addr);
	if (ret < 0) {
		msg_perr("Unable to set I2C slave address to 0x%02x: %s.\n", addr, strerror(errno));
		return ret;
	}

	return write(fd, buf->buf, buf->len);
}
