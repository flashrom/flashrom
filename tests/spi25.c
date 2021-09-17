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
#include "flashchips.h"
#include "chipdrivers.h"
#include "spi.h"

struct flashchip mock_chip = {
	.vendor		= "Generic",
	.name		= "unknown SPI chip (RDID)",
	.bustype	= BUS_SPI,
	.manufacture_id	= GENERIC_MANUF_ID,
	.model_id	= GENERIC_DEVICE_ID,
	.total_size	= 0,
	.page_size	= 256,
	.tested		= TEST_BAD_PREW,
	.probe		= probe_spi_rdid,
	.write		= NULL,
};

/*
 * This declaration is needed for visibility, so that wrap below could
 * redirect to real function.
 */
int __real_spi_send_command(const struct flashctx *flash,
		unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr);

int __wrap_spi_send_command(const struct flashctx *flash,
		unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr)
{
	if (flash->chip != &mock_chip)
		/*
		 * Caller is some other test, redirecting to real function.
		 * This test is the only one which uses wrap of spi_send_command,
		 * all other tests use real function.
		*/
		return __real_spi_send_command(flash, writecnt, readcnt, writearr, readarr);

	check_expected_ptr(flash);
	assert_int_equal(writecnt,    mock_type(int));
	assert_int_equal(writearr[0], mock_type(int));

	int rcnt = mock_type(int);
	assert_int_equal(readcnt, rcnt);
	for (int i = 0; i < rcnt; i++)
		readarr[i] = i;

	return 0;
}

void spi_write_enable_test_success(void **state)
{
	(void) state; /* unused */

	/* setup initial test state. */
	struct flashctx flashctx = { .chip = &mock_chip };
	expect_memory(__wrap_spi_send_command, flash,
			&flashctx, sizeof(flashctx));

	will_return(__wrap_spi_send_command, JEDEC_WREN_OUTSIZE);
	will_return(__wrap_spi_send_command, JEDEC_WREN);
	will_return(__wrap_spi_send_command, JEDEC_WREN_INSIZE);
	assert_int_equal(0, spi_write_enable(&flashctx));
}

void spi_write_disable_test_success(void **state)
{
	(void) state; /* unused */

	/* setup initial test state. */
	struct flashctx flashctx = { .chip = &mock_chip };
	expect_memory(__wrap_spi_send_command, flash,
			&flashctx, sizeof(flashctx));

	will_return(__wrap_spi_send_command, JEDEC_WRDI_OUTSIZE);
	will_return(__wrap_spi_send_command, JEDEC_WRDI);
	will_return(__wrap_spi_send_command, JEDEC_WRDI_INSIZE);
	assert_int_equal(0, spi_write_disable(&flashctx));
}

void probe_spi_rdid_test_success(void **state)
{
	(void) state; /* unused */

	/* setup initial test state. */
	struct flashctx flashctx = { .chip = &mock_chip };
	expect_memory(__wrap_spi_send_command, flash,
			&flashctx, sizeof(flashctx));

	will_return(__wrap_spi_send_command, JEDEC_RDID_OUTSIZE);
	will_return(__wrap_spi_send_command, JEDEC_RDID);
	will_return(__wrap_spi_send_command, JEDEC_RDID_INSIZE);
	assert_int_equal(0, probe_spi_rdid(&flashctx));
}

void probe_spi_rdid4_test_success(void **state)
{
	(void) state; /* unused */

	/* setup initial test state. */
	struct flashctx flashctx = { .chip = &mock_chip };
	expect_memory(__wrap_spi_send_command, flash,
			&flashctx, sizeof(flashctx));

	will_return(__wrap_spi_send_command, JEDEC_RDID_OUTSIZE);
	will_return(__wrap_spi_send_command, JEDEC_RDID);
	will_return(__wrap_spi_send_command, JEDEC_RDID_INSIZE + 1);
	assert_int_equal(0, probe_spi_rdid4(&flashctx));
}

void probe_spi_rems_test_success(void **state)
{
	(void) state; /* unused */

	/* setup initial test state. */
	struct flashctx flashctx = { .chip = &mock_chip };
	expect_memory(__wrap_spi_send_command, flash,
			&flashctx, sizeof(flashctx));

	will_return(__wrap_spi_send_command, JEDEC_REMS_OUTSIZE);
	will_return(__wrap_spi_send_command, JEDEC_REMS);
	will_return(__wrap_spi_send_command, JEDEC_REMS_INSIZE);
	assert_int_equal(0, probe_spi_rems(&flashctx));
}

void probe_spi_res1_test_success(void **state)
{
	(void) state; /* unused */

	/* setup initial test state. */
	struct flashctx flashctx = { .chip = &mock_chip };
	expect_memory(__wrap_spi_send_command, flash,
			&flashctx, sizeof(flashctx));

	will_return(__wrap_spi_send_command, JEDEC_RES_OUTSIZE);
	will_return(__wrap_spi_send_command, JEDEC_RES);
	will_return(__wrap_spi_send_command, JEDEC_RES_INSIZE + 1);
	assert_int_equal(0, probe_spi_res2(&flashctx));
}

void probe_spi_res2_test_success(void **state)
{
	(void) state; /* unused */

	/* setup initial test state. */
	clear_spi_id_cache();
	struct flashctx flashctx = { .chip = &mock_chip };
	expect_memory(__wrap_spi_send_command, flash,
			&flashctx, sizeof(flashctx));

	will_return(__wrap_spi_send_command, JEDEC_RES_OUTSIZE);
	will_return(__wrap_spi_send_command, JEDEC_RES);
	will_return(__wrap_spi_send_command, JEDEC_RES_INSIZE + 1);
	assert_int_equal(0, probe_spi_res2(&flashctx));
}

void probe_spi_res3_test_success(void **state)
{
	(void) state; /* unused */

	/* setup initial test state. */
	struct flashctx flashctx = { .chip = &mock_chip };
	expect_memory(__wrap_spi_send_command, flash,
			&flashctx, sizeof(flashctx));

	will_return(__wrap_spi_send_command, JEDEC_RES_OUTSIZE);
	will_return(__wrap_spi_send_command, JEDEC_RES);
	will_return(__wrap_spi_send_command, JEDEC_RES_INSIZE + 2);
	assert_int_equal(0, probe_spi_res3(&flashctx));
}

void probe_spi_at25f_test_success(void **state)
{
	(void) state; /* unused */

	/* setup initial test state. */
	struct flashctx flashctx = { .chip = &mock_chip };
	expect_memory(__wrap_spi_send_command, flash,
			&flashctx, sizeof(flashctx));

	will_return(__wrap_spi_send_command, AT25F_RDID_OUTSIZE);
	will_return(__wrap_spi_send_command, AT25F_RDID);
	will_return(__wrap_spi_send_command, AT25F_RDID_INSIZE);
	assert_int_equal(0, probe_spi_at25f(&flashctx));
}

/* spi95.c */
void probe_spi_st95_test_success(void **state)
{
	(void) state; /* unused */

	/* setup initial test state. */
	struct flashctx flashctx = { .chip = &mock_chip };
	expect_memory(__wrap_spi_send_command, flash,
			&flashctx, sizeof(flashctx));

	/* chip total size < 64K. */
	uint32_t rdid_outsize = ST_M95_RDID_2BA_OUTSIZE; // 16 bit address

	will_return(__wrap_spi_send_command, rdid_outsize);
	will_return(__wrap_spi_send_command, ST_M95_RDID);
	will_return(__wrap_spi_send_command, ST_M95_RDID_INSIZE);
	assert_int_equal(0, probe_spi_st95(&flashctx));
}
