/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2021 Google LLC
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

void run_probe_v2_lifecycle(void **state, const struct io_mock *io,
		const struct programmer_entry *prog, const char *param,
		const char *const chip_name,
		const char **expected_matched_names, unsigned int expected_matched_count);

void run_init_error_path(void **state, const struct io_mock *io,
		const struct programmer_entry *prog, const char *param, const int error_code);
#endif /* __LIFECYCLE_H__ */
