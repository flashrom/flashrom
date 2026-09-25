/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2026 Abdelkader Boudih <coreboot@seuros.com>
 */

#include <include/test.h>

#include "tests.h"
#include "cli_output.h"

void cli_output_color_wanted_test(void **state)
{
	(void) state; /* unused */

	/* Color follows the terminal. */
	assert_true(cli_output_color_wanted(true, NULL, "xterm"));
	assert_true(cli_output_color_wanted(true, NULL, NULL));
	assert_false(cli_output_color_wanted(false, NULL, "xterm"));

	/* NO_COLOR disables color only when it is non-empty. */
	assert_false(cli_output_color_wanted(true, "1", "xterm"));
	assert_true(cli_output_color_wanted(true, "", "xterm"));

	/* A dumb terminal never gets color. */
	assert_false(cli_output_color_wanted(true, NULL, "dumb"));
}
