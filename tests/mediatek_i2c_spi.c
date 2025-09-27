/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2022 Google LLC
 */

#include "lifecycle.h"

#if CONFIG_MEDIATEK_I2C_SPI == 1

void mediatek_i2c_spi_basic_lifecycle_test_success(void **state)
{
	struct io_mock_fallback_open_state mediatek_i2c_spi_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/i2c-254", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock mediatek_i2c_spi_io = {
		.fallback_open_state = &mediatek_i2c_spi_fallback_open_state,
	};

	run_basic_lifecycle(state, &mediatek_i2c_spi_io, &programmer_mediatek_i2c_spi, "bus=254,allow_brick=yes");
}

void mediatek_i2c_no_allow_brick_test_success(void **state)
{
	struct io_mock_fallback_open_state mediatek_i2c_spi_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/i2c-254", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock mediatek_i2c_spi_io = {
		.fallback_open_state = &mediatek_i2c_spi_fallback_open_state,
	};

	run_init_error_path(state, &mediatek_i2c_spi_io, &programmer_mediatek_i2c_spi,
				"bus=254", SPI_GENERIC_ERROR);
}

#else
	SKIP_TEST(mediatek_i2c_spi_basic_lifecycle_test_success)
	SKIP_TEST(mediatek_i2c_no_allow_brick_test_success)
#endif /* CONFIG_MEDIATEK_I2C_SPI */
