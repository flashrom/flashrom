/*
 * This file is part of the flashrom project.
 *
 * Copyright 2025 Dmitry Zhadinets <dzhadinets@gmail.com>
 * Copyright 2025 Google LLC
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

#include <stdlib.h>

#include <include/test.h>
#include "tests.h"
#include "libflashrom.h"
#include "flash.h"
#include "programmer.h"

static int test_log_callback(enum flashrom_log_level level, const char *format,
			     va_list vargs)
{
	char message[3] = {0};
	vsnprintf(message, 3, format, vargs);
	assert_string_equal(message, "1\n");
	return 0x666 + (int)level;
}

static void test_log_callback_v2(enum flashrom_log_level level,
				 const char *message, void *user_data)
{
	/* check that user dta has passed */
	assert_ptr_not_equal(user_data, 0);
	/* check that user_data is correct */
	assert_int_equal(*(int *)(user_data), 100500);
	/* check that format is working correctly */
	assert_string_equal(message, "2\n");
	*(int*)user_data = 0x666 + (int)level;
}

void flashrom_set_log_callback_test_success(void **state)
{
	(void)state; /* unused */
	flashrom_set_log_callback(test_log_callback);
	/* check that callback is called */
	assert_int_equal(print(FLASHROM_MSG_INFO, "1%s", "\n"), 0x666 + (int)FLASHROM_MSG_INFO);
	flashrom_set_log_callback(NULL);
}

void flashrom_set_log_callback_v2_test_success(void **state)
{
	(void)state; /* unused */
	int user_data = 100500;
	flashrom_set_log_callback_v2(test_log_callback_v2, &user_data);
	print(FLASHROM_MSG_ERROR, "2%s", "\n");
	/* check that callback is called */
	assert_int_equal(user_data, 0x666 + (int)FLASHROM_MSG_ERROR);
	flashrom_set_log_callback_v2(NULL, NULL);
}

void flashrom_set_log_level_test_success(void **state)
{
	(void)state; /* unused */
	int user_data;

	flashrom_set_log_level(FLASHROM_MSG_WARN);

	/* v2 API check */
	user_data = 100500;
	flashrom_set_log_callback_v2(test_log_callback_v2, &user_data);
	print(FLASHROM_MSG_DEBUG, "2%s", "\n");
	/* check that callback is not called */
	assert_int_equal(user_data, 100500);
	print(FLASHROM_MSG_ERROR, "2%s", "\n");
	/* check that callback is called for less */
	assert_int_equal(user_data, 0x666 + (int)FLASHROM_MSG_ERROR);
	user_data = 100500;
	print(FLASHROM_MSG_WARN, "2%s", "\n");
	/* check that callback is called for equal */
	assert_int_equal(user_data, 0x666 + (int)FLASHROM_MSG_WARN);

	/* v1 API check */
	flashrom_set_log_callback(test_log_callback);

	/* check that callback is not called */
	assert_int_equal(print(FLASHROM_MSG_INFO, "1%s", "\n"), 0);
	/* check that callback is called for equal */
	assert_int_equal(print(FLASHROM_MSG_WARN, "1%s", "\n"), 0x666 + (int)FLASHROM_MSG_WARN);
	/* check that callback is called for less*/
	assert_int_equal(print(FLASHROM_MSG_ERROR, "1%s", "\n"), 0x666 + (int)FLASHROM_MSG_ERROR);

	flashrom_set_log_level(FLASHROM_MSG_INFO);
	/* check that callback is called after the change*/
	assert_int_equal(print(FLASHROM_MSG_INFO, "1%s", "\n"), 0x666 + (int)FLASHROM_MSG_INFO);
	flashrom_set_log_callback(NULL);
}

void flashrom_supported_programmers_test_success(void **state)
{
	(void) state; /* unused */
	const char **array = flashrom_supported_programmers();
	const char **ptr = array;

	assert_non_null(array);

	do {
		assert_non_null(*ptr);
	}while (*(++ptr));

	flashrom_data_free(array);

	assert_int_not_equal(ptr - array, 0);
}

#if CONFIG_DUMMY == 1
typedef void* (map_flash_fn)(const char *descr, uintptr_t phys_addr, size_t len);
typedef int (spi_send_fn)(const struct flashctx *flash, unsigned int writecnt,
				unsigned int readcnt,
				const unsigned char *writearr,
				unsigned char *readarr);

static void *always_fail_map(const char *descr, uintptr_t phys_addr, size_t len)
{
	return ERROR_PTR;
}

static int always_fail_spi_send_command(const struct flashctx *flash, unsigned int writecnt,
				  unsigned int readcnt,
				  const unsigned char *writearr,
				  unsigned char *readarr)
{
	return -1;
}

void probe_v2_error_code_propagation(void **state)
{
	(void) state; /* unused */

	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_programmer *flashprog;
	const char **all_matched_names = NULL;

	assert_int_equal(0, flashrom_programmer_init(&flashprog,
			programmer_dummy.name,
			"bus=spi,emulate=W25Q128FV"));

	map_flash_fn *original_map_flash = registered_masters[0].spi.map_flash_region;
	spi_send_fn *original_spi_send = registered_masters[0].spi.command;
	/* This makes sure probe_flash fails. */
	registered_masters[0].spi.map_flash_region = &always_fail_map;
	registered_masters[0].spi.command = &always_fail_spi_send_command;

	assert_int_equal(0 /* no chips found */,
			flashrom_flash_probe_v2(&flashctx, &all_matched_names,
			flashprog,
			NULL));

	/* restore programmer functions */
	registered_masters[0].spi.map_flash_region = original_map_flash;
	registered_masters[0].spi.command = original_spi_send;

	assert_int_equal(0, flashrom_programmer_shutdown(flashprog));

	flashrom_data_free(all_matched_names);
}
#else
	SKIP_TEST(probe_v2_error_code_propagation)
#endif /* CONFIG_DUMMY */
