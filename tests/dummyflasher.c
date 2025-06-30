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

	const char *expected_matched_names[1] = {"W25Q128.V"};
	run_probe_v2_lifecycle(state, &dummy_io, &programmer_dummy, "bus=spi,emulate=W25Q128FV", "W25Q128.V",
				expected_matched_names, 1);
}

void dummy_probe_v2_one_match_for_W25Q128FV(void **state)
{
	struct io_mock_fallback_open_state dummy_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dummy_io = {
		.fallback_open_state = &dummy_fallback_open_state,
	};

	const char *expected_matched_names[1] = {"W25Q128.V"};
	run_probe_v2_lifecycle(state, &dummy_io, &programmer_dummy, "bus=spi,emulate=W25Q128FV",
				NULL, /* any chip name */
				expected_matched_names, 1);
}

void dummy_probe_v2_six_matches_for_MX25L6436(void **state)
{
	struct io_mock_fallback_open_state dummy_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dummy_io = {
		.fallback_open_state = &dummy_fallback_open_state,
	};

	const char *expected_matched_names[6] = {"MX25L6405",
						 "MX25L6405D",
						 "MX25L6406E/MX25L6408E",
						 "MX25L6436E/MX25L6445E/MX25L6465E",
						 "MX25L6473E",
						 "MX25L6473F"};
	run_probe_v2_lifecycle(state, &dummy_io, &programmer_dummy, "bus=spi,emulate=MX25L6436",
				NULL, /* any chip name */
				expected_matched_names, 6);
}

void dummy_probe_v2_no_matches_found(void **state)
{
	struct io_mock_fallback_open_state dummy_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dummy_io = {
		.fallback_open_state = &dummy_fallback_open_state,
	};

	run_probe_v2_lifecycle(state, &dummy_io, &programmer_dummy, "bus=spi,emulate=MX25L6436",
				"NONEXISTENT", NULL /* no matched names */, 0);
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

	const char *expected_matched_names[1] = {"Opaque flash chip"};
	run_probe_v2_lifecycle(state, &dummy_io, &programmer_dummy, "size=8388608,emulate=VARIABLE_SIZE", "Opaque flash chip",
				expected_matched_names, 1);
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

/*
 * Initialize the programmer and probe, failing the test if either step returns an error,
 * the probed chip is not the expected one, or its size is not the expected size.
 */
static void dummy_test_init_and_probe(struct flashrom_flashctx *flashctx,
					struct flashrom_programmer *flashprog)
{
	struct io_mock_fallback_open_state dummy_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dummy_io = {
		.fallback_open_state = &dummy_fallback_open_state,
	};

	io_mock_register(&dummy_io);

	const char *param = "bus=spi,emulate=W25Q128FV";
	const unsigned long chip_size = 16384 * KiB; // emulated chip size
	const char **all_matched_names = NULL;

	printf("Testing flashrom_programmer_init for programmer=%s with param=%s ...\n",
		programmer_dummy.name, param);
	assert_int_equal(0, flashrom_programmer_init(&flashprog, programmer_dummy.name, param));
	printf("... flashrom_programmer_init for programmer=%s with param=%s successful\n",
		programmer_dummy.name, param);

	printf("Testing flashrom_flash_probe_v2 for programmer=%s ... \n", programmer_dummy.name);
	assert_int_equal(1, flashrom_flash_probe_v2(flashctx, &all_matched_names,
							flashprog, NULL));
	assert_int_equal(0, strcmp("W25Q128.V", all_matched_names[0]));

	assert_int_equal(chip_size, flashrom_flash_getsize(flashctx));
	printf("... flashrom_flash_probe_v2 for programmer=%s successful\n", programmer_dummy.name);

	flashrom_data_free(all_matched_names);
}

/*
 * Shuts down the programmer and frees memory for layout and chip in flash context.
 */
static void dummy_test_shutdown(struct flashrom_flashctx *flashctx,
				struct flashrom_programmer *flashprog)
{
	printf("Testing flashrom_programmer_shutdown for programmer=%s ...\n", programmer_dummy.name);
	assert_int_equal(0, flashrom_programmer_shutdown(flashprog));
	printf("... flashrom_programmer_shutdown for programmer=%s successful\n", programmer_dummy.name);

	io_mock_register(NULL);

	flashrom_layout_release(flashctx->default_layout);
	free(flashctx->chip);
}

void dummy_probe_and_read(void **state)
{
	(void) state; /* unused */

	struct flashrom_programmer *flashprog = NULL;
	struct flashrom_flashctx flashctx = { 0 };

	dummy_test_init_and_probe(&flashctx, flashprog);

	const unsigned long image_size = 16384 * KiB;
	unsigned char *buf = calloc(image_size, sizeof(unsigned char));
	assert_non_null(buf);

	printf("Testing flashrom_image_read ...\n");
	assert_int_equal(0, flashrom_image_read(&flashctx, buf, image_size));
	printf("... flashrom_image_read is successful.\n");

	free(buf);

	dummy_test_shutdown(&flashctx, flashprog);
}

void dummy_probe_and_write(void **state)
{
	(void) state; /* unused */

	struct flashrom_programmer *flashprog = NULL;
	struct flashrom_flashctx flashctx = { 0 };

	dummy_test_init_and_probe(&flashctx, flashprog);

	const unsigned long image_size = 16384 * KiB;
	uint8_t *const newcontents = malloc(image_size);
	assert_non_null(newcontents);

	printf("Testing flashrom_image_write ...\n");
	assert_int_equal(0, flashrom_image_write(&flashctx, newcontents, image_size, NULL));
	printf("... flashrom_image_write is successful.\n");

	free(newcontents);

	dummy_test_shutdown(&flashctx, flashprog);
}

void dummy_probe_and_erase(void **state)
{
	(void) state; /* unused */

	struct flashrom_programmer *flashprog = NULL;
	struct flashrom_flashctx flashctx = { 0 };

	dummy_test_init_and_probe(&flashctx, flashprog);

	printf("Testing flashrom_flash_erase ...\n");
	assert_int_equal(0, flashrom_flash_erase(&flashctx));
	printf("... flashrom_flash_erase is successful.\n");

	dummy_test_shutdown(&flashctx, flashprog);
}

#else
	SKIP_TEST(dummy_basic_lifecycle_test_success)
	SKIP_TEST(dummy_probe_lifecycle_test_success)
	SKIP_TEST(dummy_probe_v2_one_match_for_W25Q128FV)
	SKIP_TEST(dummy_probe_v2_six_matches_for_MX25L6436)
	SKIP_TEST(dummy_probe_v2_no_matches_found)
	SKIP_TEST(dummy_probe_variable_size_test_success)
	SKIP_TEST(dummy_init_fails_unhandled_param_test_success)
	SKIP_TEST(dummy_init_success_invalid_param_test_success)
	SKIP_TEST(dummy_init_success_unhandled_param_test_success)
	SKIP_TEST(dummy_null_prog_param_test_success)
	SKIP_TEST(dummy_all_buses_test_success)
	SKIP_TEST(dummy_freq_param_init)
	SKIP_TEST(dummy_probe_and_read)
	SKIP_TEST(dummy_probe_and_write)
	SKIP_TEST(dummy_probe_and_erase)
#endif /* CONFIG_DUMMY */
