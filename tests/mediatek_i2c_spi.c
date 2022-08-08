/*
 * This file is part of the flashrom project.
 *
 * Copyright 2022 Google LLC
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
