/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2026 Abdelkader Boudih <coreboot@seuros.com>
 */

#include <include/test.h>

#include "tests.h"
#include "programmer.h"
#include "i2c_helper.h"

#include <stdlib.h>
#include <string.h>

#if CONFIG_I2C_HELPER == 1

static int run_require_allow_brick(const char *params)
{
	struct programmer_cfg cfg = { .params = strdup(params) };
	assert_non_null(cfg.params);
	int ret = i2c_require_allow_brick(&cfg);
	free(cfg.params);
	return ret;
}

void i2c_require_allow_brick_yes_test_success(void **state)
{
	(void) state; /* unused */

	assert_int_equal(0, run_require_allow_brick("allow_brick=yes"));
}

void i2c_require_allow_brick_absent_test_success(void **state)
{
	(void) state; /* unused */

	/* Without the parameter the helper must refuse to proceed. */
	assert_int_equal(-1, run_require_allow_brick("bus=254"));
}

void i2c_require_allow_brick_invalid_test_success(void **state)
{
	(void) state; /* unused */

	/* Anything other than "yes" is malformed and must be rejected. */
	const char *invalid[] = { "allow_brick=no", "allow_brick=", "allow_brick=maybe" };
	const int count = sizeof(invalid) / sizeof(invalid[0]);

	for (int i = 0; i < count; i++)
		assert_int_equal(-1, run_require_allow_brick(invalid[i]));
}

#else
	SKIP_TEST(i2c_require_allow_brick_yes_test_success)
	SKIP_TEST(i2c_require_allow_brick_absent_test_success)
	SKIP_TEST(i2c_require_allow_brick_invalid_test_success)
#endif /* CONFIG_I2C_HELPER */
