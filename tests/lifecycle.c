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

static void probe_chip(const struct programmer_entry *prog,
			struct flashrom_programmer *flashprog,
			const char *const chip_name)
{
	struct flashrom_flashctx *flashctx;

	printf("Testing flashrom_flash_probe for programmer=%s, chip=%s ... \n", prog->name, chip_name);
	assert_int_equal(0, flashrom_flash_probe(&flashctx, flashprog, chip_name));
	printf("... flashrom_flash_probe for programmer=%s successful\n", prog->name);

	flashrom_flash_release(flashctx); /* cleanup */
}

static void run_lifecycle(void **state, const struct io_mock *io, const struct programmer_entry *prog,
				const char *param, const char *const chip_name,
				void (*action)(const struct programmer_entry *prog,
						struct flashrom_programmer *flashprog,
						const char *const chip_name))
{
	(void) state; /* unused */

	io_mock_register(io);

	struct flashrom_programmer *flashprog;

	printf("Testing flashrom_programmer_init for programmer=%s ...\n", prog->name);
	assert_int_equal(0, flashrom_programmer_init(&flashprog, prog->name, param));
	printf("... flashrom_programmer_init for programmer=%s successful\n", prog->name);

	if (action)
		action(prog, flashprog, chip_name);

	printf("Testing flashrom_programmer_shutdown for programmer=%s ...\n", prog->name);
	assert_int_equal(0, flashrom_programmer_shutdown(flashprog));
	printf("... flashrom_programmer_shutdown for programmer=%s successful\n", prog->name);

	io_mock_register(NULL);
}

void run_basic_lifecycle(void **state, const struct io_mock *io,
		const struct programmer_entry *prog, const char *param)
{
	/* Basic lifecycle only does init and shutdown,
	 * so neither chip name nor action is needed. */
	run_lifecycle(state, io, prog, param, NULL /* chip_name */, NULL /* action */);
}

void run_probe_lifecycle(void **state, const struct io_mock *io,
		const struct programmer_entry *prog, const char *param, const char *const chip_name)
{
	/* Each probe lifecycle should run independently, without cache. */
	clear_spi_id_cache();
	run_lifecycle(state, io, prog, param, chip_name, &probe_chip);
}

void run_init_error_path(void **state, const struct io_mock *io, const struct programmer_entry *prog,
				const char *param, const int error_code)
{
	(void) state; /* unused */

	io_mock_register(io);

	struct flashrom_programmer *flashprog;

	printf("Testing init error path for programmer=%s with params: %s ...\n", prog->name, param);
	assert_int_equal(error_code, flashrom_programmer_init(&flashprog, prog->name, param));
	printf("... init failed with error code %i as expected\n", error_code);

	/*
	 * `flashrom_programmer_shutdown` runs only registered shutdown functions, which means
	 * if nothing has been registered then nothing runs.
	 * Since this is testing error path on initialisation and error can happen at different
	 * phases of init, we don't know whether shutdown function has already been registered
	 * or not yet. Running `flashrom_programmer_shutdown` covers both situations.
	 */
	printf("Running programmer shutdown in case anything got registered...\n");
	assert_int_equal(0, flashrom_programmer_shutdown(flashprog));
	printf("... completed\n");

	io_mock_register(NULL);
}
