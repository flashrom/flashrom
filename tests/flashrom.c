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
