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

#if CONFIG_PARADE_LSPCON == 1

void parade_lspcon_basic_lifecycle_test_success(void **state)
{
	struct io_mock_fallback_open_state parade_lspcon_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/i2c-254", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock parade_lspcon_io = {
		.fallback_open_state = &parade_lspcon_fallback_open_state,
	};

	run_basic_lifecycle(state, &parade_lspcon_io, &programmer_parade_lspcon, "bus=254,allow_brick=yes");
}

void parade_lspcon_no_allow_brick_test_success(void **state)
{
	struct io_mock_fallback_open_state parade_lspcon_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/i2c-254", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock parade_lspcon_io = {
		.fallback_open_state = &parade_lspcon_fallback_open_state,
	};

	run_init_error_path(state, &parade_lspcon_io, &programmer_parade_lspcon,
				"bus=254", SPI_GENERIC_ERROR);
}

#else
	SKIP_TEST(parade_lspcon_basic_lifecycle_test_success)
	SKIP_TEST(parade_lspcon_no_allow_brick_test_success)
#endif /* CONFIG_PARADE_LSPCON */
