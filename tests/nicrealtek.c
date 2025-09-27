/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2021 Google LLC
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
