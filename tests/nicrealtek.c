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

#if CONFIG_NICREALTEK == 1
void nicrealtek_basic_lifecycle_test_success(void **state)
{
	struct io_mock_fallback_open_state nicrealtek_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock nicrealtek_io = {
		.fallback_open_state = &nicrealtek_fallback_open_state,
	};

	run_basic_lifecycle(state, &nicrealtek_io, &programmer_nicrealtek, "");
}
#else
	SKIP_TEST(nicrealtek_basic_lifecycle_test_success)
#endif /* CONFIG_NICREALTEK */
