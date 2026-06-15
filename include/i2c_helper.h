/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2020 The Chromium OS Authors
 */

#ifndef I2C_HELPER_H
#define I2C_HELPER_H

#include <inttypes.h>
#include <stdbool.h>

struct programmer_cfg; /* defined in programmer.h */

/**
 * i2c_open_from_programmer_params: open an I2C device from programmer params
 *
 * This function is a wrapper for i2c_open and i2c_open_path that obtains the
 * I2C device to use from programmer parameters. It is meant to be called
 * from I2C-based programmers to avoid repeating parameter parsing code.
 */
int i2c_open_from_programmer_params(const struct programmer_cfg *cfg, uint16_t addr, int force);

/**
 * i2c_write_buffer: write len bytes from buf to the I2C device at addr
 *
 * Wraps i2c_buffer_t_fill and i2c_write for the common case of writing a
 * plain byte buffer. Returns 0 when the full buffer was written, or -1 on
 * failure (including a short write).
 */
int i2c_write_buffer(int fd, uint16_t addr, void *buf, uint16_t len);

/**
 * i2c_read_buffer: read len bytes into buf from the I2C device at addr
 *
 * Wraps i2c_buffer_t_fill and i2c_read for the common case of reading into a
 * plain byte buffer. Returns 0 when the full buffer was read, or -1 on
 * failure (including a short read).
 */
int i2c_read_buffer(int fd, uint16_t addr, void *buf, uint16_t len);

/**
 * i2c_require_allow_brick: enforce the shared allow_brick programmer parameter
 *
 * I2C programmers can brick devices because the I2C address space can be
 * overloaded, so they require the user to opt in with allow_brick=yes. This
 * parses that parameter and, when it is absent or not "yes", prints the
 * standard explanation. Returns 0 when allow_brick=yes was given, -1 otherwise
 * (missing or malformed). I2C-based init functions should bail when it fails.
 */
int i2c_require_allow_brick(const struct programmer_cfg *cfg);

#endif /* !I2C_HELPER_H */
