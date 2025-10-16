/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2025 Google LLC
 */

#include "client_tests.h"

int main(int argc, char *argv[])
{
	int ret = 0;

	if (argc > 1)
		cmocka_set_test_filter(argv[1]);

	cmocka_set_message_output(CM_OUTPUT_STDOUT);

	const struct CMUnitTest external_client_tests[] = {
		cmocka_unit_test(flashrom_init_probe_erase_shutdown),
	};
	ret |= cmocka_run_group_tests_name("external_client.c tests",
			external_client_tests, NULL, NULL);

	return ret;
}
