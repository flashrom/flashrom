/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2020 Google LLC
 */

#include <stdlib.h>

#include <include/test.h>
#include "tests.h"
#include "programmer.h"

#define assert_equal_and_free(text, expected)	\
	do {						\
		assert_string_equal(text, expected);	\
		free(text);				\
	} while (0)

#define assert_not_equal_and_free(text, expected)	\
	do {							\
		assert_string_not_equal(text, expected);	\
		free(text);					\
	} while (0)


void flashbuses_to_text_test_success(void **state)
{
	(void) state; /* unused */

	enum chipbustype bustype;
	char *text;

	bustype = BUS_NONSPI;
	text = flashbuses_to_text(bustype);
	assert_equal_and_free(text, "Non-SPI");

	bustype |= BUS_PARALLEL;
	text = flashbuses_to_text(bustype);
	assert_not_equal_and_free(text, "Non-SPI, Parallel");

	bustype = BUS_PARALLEL;
	bustype |= BUS_LPC;
	text = flashbuses_to_text(bustype);
	assert_equal_and_free(text, "Parallel, LPC");

	bustype |= BUS_FWH;
	//BUS_NONSPI = BUS_PARALLEL | BUS_LPC | BUS_FWH,
	text = flashbuses_to_text(bustype);
	assert_equal_and_free(text, "Non-SPI");

	bustype |= BUS_SPI;
	text = flashbuses_to_text(bustype);
	assert_equal_and_free(text, "Parallel, LPC, FWH, SPI");

	bustype |= BUS_PROG;
	text = flashbuses_to_text(bustype);
	assert_equal_and_free(text,
		"Parallel, LPC, FWH, SPI, Programmer-specific");

	bustype = BUS_NONE;
	text = flashbuses_to_text(bustype);
	assert_equal_and_free(text, "None");
}
