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

#ifndef I2C_HELPER_H
#define I2C_HELPER_H

#include <inttypes.h>

struct programmer_cfg; /* defined in programmer.h */

/**
 * An convenient structure that contains the buffer size and the buffer
 * pointer. Used to wrap buffer details while doing the I2C data
 * transfer on both input and output. It is the client's responsibility
 * to use i2c_buffer_t_fill to initialize this struct instead of
 * trying to construct it directly.
 */
typedef struct {
	void *buf;
	uint16_t len;
} i2c_buffer_t;

/**
 * i2c_buffer_t_fill - fills in the i2c_buffer_t
 *
 * @i2c_buf:	pointer to the be constructed.
 * @buf:	buffer contains data to be included in i2c_buffer_t.
 * @len:	length of buffer to be included in i2c_buffer_t.
 *
 * This function takes in a pointer to an initialized i2c_buffer_t
 * object with related information, and fill in the i2c_buffer_t with
 * some validation applied. The function does allow initialization with
 * NULL buffer but will make sure len == 0 in such case.
 *
 * returns 0 on success, <0 to indicate failure
 */
static inline int i2c_buffer_t_fill(i2c_buffer_t *i2c_buf, void *buf, uint16_t len)
{
	if (!i2c_buf || (!buf && len))
		return -1;

	i2c_buf->buf = buf;
	i2c_buf->len = len;

	return 0;
}

/**
 * i2c_open - opens the target I2C device and set the I2C slave address
 *
 * @bus:	I2C bus number of the target device.
 * @addr:	I2C slave address.
 * @force:	whether to force set the I2C slave address.
 *
 * This function returns a file descriptor for the target device. It is
 * the client's responsibility to pass the return value to i2c_close to
 * clean up.
 *
 * returns fd of target device on success, <0 to indicate failure
 */
int i2c_open(int bus, uint16_t addr, int force);

/**
 * i2c_open_path: open an I2C device by device path
 *
 * This function behaves the same as i2c_open, but takes a filesystem
 * path (assumed to be an I2C device file) instead of a bus number.
 */
int i2c_open_path(const char *path, uint16_t addr, int force);

/**
 * i2c_open_from_programmer_params: open an I2C device from programmer params
 *
 * This function is a wrapper for i2c_open and i2c_open_path that obtains the
 * I2C device to use from programmer parameters. It is meant to be called
 * from I2C-based programmers to avoid repeating parameter parsing code.
 */
int i2c_open_from_programmer_params(const struct programmer_cfg *cfg, uint16_t addr, int force);

/**
 * i2c_close - closes the file descriptor returned by i2c_open
 *
 * @fd:	file descriptor to be closed.
 *
 * It is the client's responsibility to set fd = -1 when it is
 * done with it.
 *
 * returns 0 on success, <0 to indicate failure
 */
int i2c_close(int fd);

/**
 * i2c_read - reads data from the I2C device
 *
 * @fd:		file descriptor of the target device.
 * @addr:	I2C slave address of the target device.
 * @buf_read:	data struct includes reading buffer and size.
 *
 * This function does accept empty read and do nothing on such case.
 *
 * returns read length on success, <0 to indicate failure
 */
int i2c_read(int fd, uint16_t addr, i2c_buffer_t *buf_read);

/**
 * i2c_write - writes command/data into the I2C device
 *
 * @fd:		file descriptor of the target device.
 * @addr:	I2C slave address of the target device.
 * @buf_write:	data struct includes writing buffer and size.
 *
 * This function does accept empty write and do nothing on such case.
 *
 * returns wrote length on success, <0 to indicate failure.
 */
int i2c_write(int fd, uint16_t addr, const i2c_buffer_t *buf_write);

#endif /* !I2C_HELPER_H */
