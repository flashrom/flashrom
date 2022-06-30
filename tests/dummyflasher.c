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

#include "lifecycle.h"

#if CONFIG_DUMMY == 1
void dummy_basic_lifecycle_test_success(void **state)
{
	struct io_mock_fallback_open_state dummy_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dummy_io = {
		.fallback_open_state = &dummy_fallback_open_state,
	};

	run_basic_lifecycle(state, &dummy_io, &programmer_dummy, "bus=parallel+lpc+fwh+spi+prog");
}

void dummy_probe_lifecycle_test_success(void **state)
{
	struct io_mock_fallback_open_state dummy_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dummy_io = {
		.fallback_open_state = &dummy_fallback_open_state,
	};

	run_probe_lifecycle(state, &dummy_io, &programmer_dummy, "bus=spi,emulate=W25Q128FV", "W25Q128.V");
}

void dummy_probe_variable_size_test_success(void **state)
{
	struct io_mock_fallback_open_state dummy_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dummy_io = {
		.fallback_open_state = &dummy_fallback_open_state,
	};

	run_probe_lifecycle(state, &dummy_io, &programmer_dummy, "size=8388608,emulate=VARIABLE_SIZE", "Opaque flash chip");
}

#else
	SKIP_TEST(dummy_basic_lifecycle_test_success)
	SKIP_TEST(dummy_probe_lifecycle_test_success)
	SKIP_TEST(dummy_probe_variable_size_test_success)
#endif /* CONFIG_DUMMY */
