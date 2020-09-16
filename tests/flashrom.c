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

#include "programmer.h"

void flashbuses_to_text_test_success(void **state)
{
	(void) state; /* unused */

	enum chipbustype bustype;

	bustype = BUS_NONSPI;
	assert_string_equal(flashbuses_to_text(bustype), "Non-SPI");

	bustype |= BUS_PARALLEL;
	assert_string_not_equal(flashbuses_to_text(bustype), "Non-SPI, Parallel");

	bustype = BUS_PARALLEL;
	bustype |= BUS_LPC;
	assert_string_equal(flashbuses_to_text(bustype), "Parallel, LPC");

	bustype |= BUS_FWH;
	//BUS_NONSPI = BUS_PARALLEL | BUS_LPC | BUS_FWH,
	assert_string_equal(flashbuses_to_text(bustype), "Non-SPI");

	bustype |= BUS_SPI;
	assert_string_equal(flashbuses_to_text(bustype), "Parallel, LPC, FWH, SPI");

	bustype |= BUS_PROG;
	assert_string_equal(
			flashbuses_to_text(bustype),
			"Parallel, LPC, FWH, SPI, Programmer-specific"
	);

	bustype = BUS_NONE;
	assert_string_equal(flashbuses_to_text(bustype), "None");
}
