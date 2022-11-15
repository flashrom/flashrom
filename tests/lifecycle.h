/*
 * This file is part of the flashrom project.
 *
 * Copyright 2021 Google LLC
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

#ifndef __LIFECYCLE_H__
#define __LIFECYCLE_H__

#include <include/test.h>
#include <string.h>
#if defined(__linux__) && !defined(__ANDROID__)
#include <linux/spi/spidev.h>
#endif

#include "tests.h"
#include "libflashrom.h"
#include "io_mock.h"
#include "programmer.h"
#include "spi.h"

void run_basic_lifecycle(void **state, const struct io_mock *io,
		const struct programmer_entry *prog, const char *param);

void run_probe_lifecycle(void **state, const struct io_mock *io,
		const struct programmer_entry *prog, const char *param, const char *const chip_name);

void run_init_error_path(void **state, const struct io_mock *io,
		const struct programmer_entry *prog, const char *param, const int error_code);
#endif /* __LIFECYCLE_H__ */
