/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2025 Google LLC
 */

/*
 * The purpose of this test is to use libflashrom API as an external client.
 *
 * Therefore, the test *must not* include any flashrom-internal headers.
 * In meson files, test depends on a special `libflashrom_test_dep` and runs
 * in a separate test executable `flashrom_client_tests`.
 */

#include <stdio.h>
#include <string.h>
#include "client_tests.h"
#include "libflashrom.h"

#define DUMMYFLASHER_NAME "dummy"

void flashrom_init_probe_erase_shutdown(void **state)
{
	(void) state; /* unused */

	assert_int_equal(0, flashrom_init(1));
	printf("flashrom_init with selfcheck: OK\n");

	struct flashrom_programmer *flashprog = NULL;
	struct flashrom_flashctx *flashctx;
	const char **all_matched_names = NULL;

	assert_int_equal(0, flashrom_create_context(&flashctx));
	printf("flashrom_create_context: OK\n");

	const char **progs_names = flashrom_supported_programmers();
	assert_non_null(progs_names);

	const char **ptr = progs_names;
	bool dummyflasher_support = false;
	do {
		if (!strcmp(*ptr, DUMMYFLASHER_NAME)) {
			dummyflasher_support = true;
			break;
		}
	} while (*(++ptr));
	flashrom_data_free(progs_names);

	if (dummyflasher_support) {
		printf("dummyflasher supported: OK\n");

		assert_int_equal(0, flashrom_programmer_init(
					&flashprog,
					DUMMYFLASHER_NAME,
					"bus=spi,emulate=W25Q128FV"));
		printf("flashrom_programmer_init for dummy with params 'bus=spi,emulate=W25Q128FV': OK\n");

		assert_int_equal(1, flashrom_flash_probe_v2(
					flashctx,
					&all_matched_names,
					flashprog,
					NULL));
		printf("flashrom_flash_probe_v2 found 1 chip: OK\n");
		assert_int_equal(0, strcmp("W25Q128.V", all_matched_names[0]));
		printf("chip name matches 'W25Q128.V': OK\n");
		assert_int_equal(16777216, flashrom_flash_getsize(flashctx));
		printf("chip size 16M: OK\n");

		assert_int_equal(0, flashrom_flash_erase(flashctx));
		printf("flashrom_flash_erase: OK\n");

		assert_int_equal(0, flashrom_programmer_shutdown(flashprog));
		printf("flashrom_programmer_shutdown: OK\n");
	} else {
		printf("WARNING: dummyflasher is disabled, cannot test probe and erase\n");
	}

	flashrom_flash_release(flashctx);
	printf("flashrom_flash_release: OK\n");

	assert_int_equal(0, flashrom_shutdown());
	printf("flashrom_shutdown: OK\n");
}
