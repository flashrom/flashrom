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
#include "programmer.h"

/* Null characters are placeholders for bus number digits */
#define I2C_DEV_PREFIX	"/dev/i2c-\0\0\0"
#define I2C_MAX_BUS	255

int i2c_close(int fd)
{
	return fd == -1 ? 0 : close(fd);
}

int i2c_open_path(const char *path, uint16_t addr, int force)
{
	int fd = open(path, O_RDWR);
	if (fd < 0) {
		msg_perr("Unable to open I2C device %s: %s.\n", path, strerror(errno));
		return fd;
	}

	int request = force ? I2C_SLAVE_FORCE : I2C_SLAVE;
	int ret = ioctl(fd, request, addr);
	if (ret < 0) {
		msg_perr("Unable to set I2C slave address to 0x%02x: %s.\n", addr, strerror(errno));
		i2c_close(fd);
		return ret;
	}

	return fd;
}

int i2c_open(int bus, uint16_t addr, int force)
{
	char dev[sizeof(I2C_DEV_PREFIX)] = {0};

	int ret = -1;

	if (bus < 0 || bus > I2C_MAX_BUS) {
		msg_perr("Invalid I2C bus %d.\n", bus);
		return ret;
	}

	ret = snprintf(dev, sizeof(dev), "%s%d", I2C_DEV_PREFIX, bus);
	if (ret < 0) {
		msg_perr("Unable to join bus number to device name: %s.\n", strerror(errno));
		return ret;
	}

	return i2c_open_path(dev, addr, force);
}

static int get_bus_number(char *bus_str)
{
	char *bus_suffix;
	errno = 0;
	int bus = (int)strtol(bus_str, &bus_suffix, 10);
	if (errno != 0 || bus_str == bus_suffix) {
		msg_perr("%s: Could not convert 'bus'.\n", __func__);
		return -1;
	}

	if (strlen(bus_suffix) > 0) {
		msg_perr("%s: Garbage following 'bus' value.\n", __func__);
		return -1;
	}

	msg_pinfo("Using I2C bus %d.\n", bus);
	return bus;
}

int i2c_open_from_programmer_params(const struct programmer_cfg *cfg, uint16_t addr, int force)
{
	int fd = -1;

	char *bus_str = extract_programmer_param_str(cfg, "bus");
	char *device_path = extract_programmer_param_str(cfg, "devpath");

	if (device_path != NULL && bus_str != NULL) {
		msg_perr("%s: only one of bus and devpath may be specified\n", __func__);
		goto out;
	}
	if (device_path == NULL && bus_str == NULL) {
		msg_perr("%s: one of bus and devpath must be specified\n", __func__);
		goto out;
	}

	if (device_path != NULL)
		fd = i2c_open_path(device_path, addr, force);
	else
		fd = i2c_open(get_bus_number(bus_str), addr, force);

out:
	free(bus_str);
	free(device_path);
	return fd;
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
