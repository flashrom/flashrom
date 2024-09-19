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

void dummy_init_fails_unhandled_param_test_success(void **state)
{
	struct io_mock_fallback_open_state dummy_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dummy_io = {
		.fallback_open_state = &dummy_fallback_open_state,
	};

	/*
	 * Programmer init should fail due to `dummy_init` failure caused by
	 * invalid value of `emulate` param. There is unhandled param left
	 * at the end of param string.
	 */
	run_init_error_path(state, &dummy_io, &programmer_dummy, "bus=spi,emulate=INVALID,unhandled=value", 1);
}

void dummy_init_success_invalid_param_test_success(void **state)
{
	struct io_mock_fallback_open_state dummy_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dummy_io = {
		.fallback_open_state = &dummy_fallback_open_state,
	};

	/*
	 * Programmer init should fail despite of the fact that `dummy_init`
	 * is successful, due to invalid param at the end of param string.
	 */
	run_init_error_path(state, &dummy_io, &programmer_dummy,
				"bus=spi,emulate=W25Q128FV,invalid=value", ERROR_FLASHROM_FATAL);
}

void dummy_init_success_unhandled_param_test_success(void **state)
{
	struct io_mock_fallback_open_state dummy_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dummy_io = {
		.fallback_open_state = &dummy_fallback_open_state,
	};

	/*
	 * Programmer init should fail despite of the fact that `dummy_init`
	 * is successful, due to unhandled param at the end of param string.
	 * Unhandled param `voltage` is not used for dummyflasher.
	 */
	run_init_error_path(state, &dummy_io, &programmer_dummy,
				"bus=spi,emulate=W25Q128FV,voltage=3.5V", ERROR_FLASHROM_FATAL);
}

void dummy_null_prog_param_test_success(void **state)
{
	struct io_mock_fallback_open_state dummy_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dummy_io = {
		.fallback_open_state = &dummy_fallback_open_state,
	};

	run_basic_lifecycle(state, &dummy_io, &programmer_dummy, NULL);
}

void dummy_all_buses_test_success(void **state)
{
	struct io_mock_fallback_open_state dummy_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dummy_io = {
		.fallback_open_state = &dummy_fallback_open_state,
	};

	run_basic_lifecycle(state, &dummy_io, &programmer_dummy, "bus=lpc+fwh");
	run_basic_lifecycle(state, &dummy_io, &programmer_dummy, "bus=spi");
	run_basic_lifecycle(state, &dummy_io, &programmer_dummy, "bus=prog");
	run_basic_lifecycle(state, &dummy_io, &programmer_dummy, "bus=parallel+fwh+prog");
	run_basic_lifecycle(state, &dummy_io, &programmer_dummy, "bus=spi+prog");
	run_basic_lifecycle(state, &dummy_io, &programmer_dummy, "bus=parallel+lpc+spi");
}

void dummy_freq_param_init(void **state)
{
	struct io_mock_fallback_open_state dummy_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dummy_io = {
		.fallback_open_state = &dummy_fallback_open_state,
	};

	run_basic_lifecycle(state, &dummy_io, &programmer_dummy, "bus=spi,freq=12Hz");
	run_basic_lifecycle(state, &dummy_io, &programmer_dummy, "bus=spi,freq=123KHz");
	run_basic_lifecycle(state, &dummy_io, &programmer_dummy, "bus=spi,freq=345MHz");
	run_basic_lifecycle(state, &dummy_io, &programmer_dummy, "bus=spi,freq=8000MHz");
	/* Valid values for freq param are within the range [1Hz, 8000Mhz] */
	run_init_error_path(state, &dummy_io, &programmer_dummy, "bus=spi,freq=0Hz", 0x1);
	run_init_error_path(state, &dummy_io, &programmer_dummy, "bus=spi,freq=8001Mhz", 0x1);
}

#else
	SKIP_TEST(dummy_basic_lifecycle_test_success)
	SKIP_TEST(dummy_probe_lifecycle_test_success)
	SKIP_TEST(dummy_probe_variable_size_test_success)
	SKIP_TEST(dummy_init_fails_unhandled_param_test_success)
	SKIP_TEST(dummy_init_success_invalid_param_test_success)
	SKIP_TEST(dummy_init_success_unhandled_param_test_success)
	SKIP_TEST(dummy_null_prog_param_test_success)
	SKIP_TEST(dummy_all_buses_test_success)
	SKIP_TEST(dummy_freq_param_init)
#endif /* CONFIG_DUMMY */
