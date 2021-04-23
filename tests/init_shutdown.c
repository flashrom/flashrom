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

#include <include/test.h>
#include <string.h>

#include "programmer.h"

static void run_lifecycle(void **state, enum programmer prog, const char *param)
{
	(void) state; /* unused */

	printf("Testing programmer_init for programmer=%u ...\n", prog);
	assert_int_equal(0, programmer_init(prog, strdup(param)));
	printf("... programmer_init for programmer=%u successful\n", prog);

	printf("Testing programmer_shutdown for programmer=%u ...\n", prog);
	assert_int_equal(0, programmer_shutdown());
	printf("... programmer_shutdown for programmer=%u successful\n", prog);
}

void dummy_init_and_shutdown_test_success(void **state)
{
	run_lifecycle(state, PROGRAMMER_DUMMY, "bus=parallel+lpc+fwh+spi");
}

void linux_spi_init_and_shutdown_test_success(void **state)
{
	/*
	 * Current implementation tests a particular path of the init procedure.
	 * There are two ways for it to succeed: reading the buffer size from sysfs
	 * and the fallback to getpagesize(). This test does the latter (fallback to
	 * getpagesize).
	 */
	run_lifecycle(state, PROGRAMMER_LINUX_SPI, "dev=/dev/null");
}
