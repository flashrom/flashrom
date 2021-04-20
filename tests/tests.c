/*
 * This file is part of the flashrom project.
 *
 * Copyright 2020 Google LLC
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

#include <include/test.h>
#include "tests.h"

#include <stdio.h>

/* redefinitions/wrapping */
#define LOG_ME printf("%s is called\n", __func__)

void __wrap_physunmap(void *virt_addr, size_t len)
{
	LOG_ME;
}

void *__wrap_physmap(const char *descr, uintptr_t phys_addr, size_t len)
{
	LOG_ME;
	return NULL;
}

int main(void)
{
	int ret = 0;

	cmocka_set_message_output(CM_OUTPUT_STDOUT);

	const struct CMUnitTest helpers_tests[] = {
		cmocka_unit_test(address_to_bits_test_success),
		cmocka_unit_test(bitcount_test_success),
		cmocka_unit_test(minmax_test_success),
		cmocka_unit_test(strcat_realloc_test_success),
		cmocka_unit_test(tolower_string_test_success),
		cmocka_unit_test(reverse_byte_test_success),
		cmocka_unit_test(reverse_bytes_test_success),
	};
	ret |= cmocka_run_group_tests_name("helpers.c tests", helpers_tests, NULL, NULL);

	const struct CMUnitTest flashrom_tests[] = {
		cmocka_unit_test(flashbuses_to_text_test_success),
	};
	ret |= cmocka_run_group_tests_name("flashrom.c tests", flashrom_tests, NULL, NULL);

	const struct CMUnitTest spi25_tests[] = {
		cmocka_unit_test(spi_write_enable_test_success),
		cmocka_unit_test(spi_write_disable_test_success),
		cmocka_unit_test(probe_spi_rdid_test_success),
		cmocka_unit_test(probe_spi_rdid4_test_success),
		cmocka_unit_test(probe_spi_rems_test_success),
		cmocka_unit_test(probe_spi_res1_test_success),
		cmocka_unit_test(probe_spi_res2_test_success),
		cmocka_unit_test(probe_spi_res3_test_success),
		cmocka_unit_test(probe_spi_at25f_test_success),
		cmocka_unit_test(probe_spi_st95_test_success), /* spi95.c */
	};
	ret |= cmocka_run_group_tests_name("spi25.c tests", spi25_tests, NULL, NULL);

	const struct CMUnitTest init_shutdown_tests[] = {
		cmocka_unit_test(dummy_init_and_shutdown_test_success),
	};
	ret |= cmocka_run_group_tests_name("init_shutdown.c tests", init_shutdown_tests, NULL, NULL);

	return ret;
}
