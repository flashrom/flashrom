/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2020 The Chromium OS Authors
 */

#ifndef I2C_HELPER_H
#define I2C_HELPER_H

#include <inttypes.h>

struct programmer_cfg; /* defined in programmer.h */

/**
 * i2c_open_from_programmer_params: open an I2C device from programmer params
 *
 * This function is a wrapper for i2c_open and i2c_open_path that obtains the
 * I2C device to use from programmer parameters. It is meant to be called
 * from I2C-based programmers to avoid repeating parameter parsing code.
 */
int i2c_open_from_programmer_params(const struct programmer_cfg *cfg, uint16_t addr, int force);

#endif /* !I2C_HELPER_H */
