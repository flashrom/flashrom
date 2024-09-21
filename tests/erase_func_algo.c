/*
 * This file is part of the flashrom project.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tests.h"
#include "chipdrivers.h"
#include "flash.h"
#include "io_mock.h"
#include "libflashrom.h"
#include "programmer.h"

#define LOG_ERASE_FUNC		printf("Eraser called with blockaddr=0x%x, blocklen=0x%x, erase_func=%d\n", blockaddr, blocklen, erase_func - TEST_ERASE_INJECTOR_1 + 1)
#define LOG_READ_WRITE_FUNC printf("%s called with start=0x%x, len=0x%x\n", __func__, start, len)

#define ERASE_VALUE		0xff
#define MOCK_CHIP_SIZE		16
#define MIN_BUF_SIZE		1024 /* Minimum buffer size flashrom operates for chip operations. */
#define MIN_REAL_CHIP_SIZE	1024 /* Minimum chip size that can be defined for real chip in flashchips */

struct test_region {
	const size_t start;
	const size_t end;
	const char *name;
};

struct erase_invoke {
	unsigned int blockaddr;
	unsigned int blocklen;
	enum block_erase_func erase_func;
};

struct test_case {
	struct flashchip *chip; /* Chip definition. */
	struct test_region regions[MOCK_CHIP_SIZE]; /* Layout regions. */
	uint8_t initial_buf[MOCK_CHIP_SIZE]; /* Initial state of chip memory. */
	uint8_t erased_buf[MOCK_CHIP_SIZE]; /* Expected content after erase. */
	uint8_t written_buf[MOCK_CHIP_SIZE]; /* Expected content after write. */
	uint8_t written_protected_buf[MOCK_CHIP_SIZE]; /* Expected content after write with protected region. */
	struct erase_invoke eraseblocks_expected[MOCK_CHIP_SIZE]; /* Expected order of eraseblocks invocations. */
	unsigned int eraseblocks_expected_ind; /* Expected number of eraseblocks invocations. */
	char erase_test_name[40]; /* Test case display name for testing erase operation. */
	char write_test_name[40]; /* Test case display name for testing write operation. */
};

struct all_state {
	uint8_t buf[MIN_REAL_CHIP_SIZE]; /* Buffer emulating the memory of the mock chip. */
	bool was_modified[MIN_REAL_CHIP_SIZE]; /* Which bytes were modified, 0x1 if byte was modified. */
	bool was_verified[MIN_REAL_CHIP_SIZE]; /* Which bytes were verified, 0x1 if byte was verified. */
	struct erase_invoke eraseblocks_actual[MOCK_CHIP_SIZE]; /* The actual order of eraseblocks invocations. */
	unsigned int eraseblocks_actual_ind; /* Actual number of eraseblocks invocations. */
	const struct test_case* current_test_case; /* Currently executed test case. */
} g_state;

static int read_chip(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	if (start < MOCK_CHIP_SIZE)
		LOG_READ_WRITE_FUNC;

	assert_in_range(start + len, 0, MIN_REAL_CHIP_SIZE);

	memcpy(buf, &g_state.buf[start], len);

	/* If these bytes were modified before => current read op is verify op, track it */
	bool bytes_modified = false;
	for (unsigned int i = start; i < start + len; i++)
		if (g_state.was_modified[i]) {
			bytes_modified = true;
			break;
		}
	if (bytes_modified)
		memset(&g_state.was_verified[start], true, len);

	return 0;
}

static int write_chip(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	if (start < MOCK_CHIP_SIZE)
		LOG_READ_WRITE_FUNC;

	assert_in_range(start + len, 0, MIN_REAL_CHIP_SIZE);

	memcpy(&g_state.buf[start], buf, len);

	/* Track the bytes were written */
	memset(&g_state.was_modified[start], true, len);
	/* Clear the records of previous verification, if there were any */
	memset(&g_state.was_verified[start], false, len);

	return 0;
}

static int block_erase_chip_tagged(struct flashctx *flash, enum block_erase_func erase_func, unsigned int blockaddr, unsigned int blocklen)
{
	if (blockaddr < MOCK_CHIP_SIZE) {
		LOG_ERASE_FUNC;

		/* Register eraseblock invocation. */
		g_state.eraseblocks_actual[g_state.eraseblocks_actual_ind] = (struct erase_invoke){
			.blocklen = blocklen,
			.blockaddr = blockaddr,
			.erase_func = erase_func,
		};
		g_state.eraseblocks_actual_ind++;
	}

	assert_in_range(blockaddr + blocklen, 0, MIN_REAL_CHIP_SIZE);

	memset(&g_state.buf[blockaddr], ERASE_VALUE, blocklen);

	/* Track the bytes were erased */
	memset(&g_state.was_modified[blockaddr], true, blocklen);
	/* Clear the records of previous verification, if there were any */
	memset(&g_state.was_verified[blockaddr], false, blocklen);

	return 0;
}

static struct flashchip chip_1_2_4_8_16 = {
	.vendor		= "aklm",
	/*
	 * total_size is supposed to be in Kilobytes and this number is multiplied
	 * by 1024 everywhere in flashrom code. With this, we can't have the chip of
	 * size MOCK_CHIP_SIZE anyway, because MOCK_CHIP_SIZE is much smaller than
	 * 1024.
	 *
	 * So setting 1 here as this is the smallest possible value of unsigned int.
	 * Why aim for the smallest value? Because various places in flashrom code
	 * are allocating buffers of size total_size * 1024, however in these unit
	 * tests only MOCK_CHIP_SIZE bytes are tracked/logged/asserted.
	 */
	.total_size	= 1,
	.tested		= TEST_OK_PREW,
	.gran		= WRITE_GRAN_1BYTE,
	.read		= TEST_READ_INJECTOR,
	.write		= TEST_WRITE_INJECTOR,
	.block_erasers	=
	{
		{
			.eraseblocks = { {1, MIN_REAL_CHIP_SIZE} },
			.block_erase = TEST_ERASE_INJECTOR_1,
		}, {
			.eraseblocks = { {2, MIN_REAL_CHIP_SIZE / 2} },
			.block_erase = TEST_ERASE_INJECTOR_2,
		}, {
			.eraseblocks = { {4, MIN_REAL_CHIP_SIZE / 4} },
			.block_erase = TEST_ERASE_INJECTOR_3,
		}, {
			.eraseblocks = { {8, MIN_REAL_CHIP_SIZE / 8} },
			.block_erase = TEST_ERASE_INJECTOR_4,
		}, {
			.eraseblocks = { {16, MIN_REAL_CHIP_SIZE / 16} },
			.block_erase = TEST_ERASE_INJECTOR_5,
		}
	},
};

static struct flashchip chip_1_8_16 = {
	.vendor		= "aklm",
	/* See comment on previous chip. */
	.total_size	= 1,
	.tested		= TEST_OK_PREW,
	.gran		= WRITE_GRAN_1BYTE,
	.read		= TEST_READ_INJECTOR,
	.write		= TEST_WRITE_INJECTOR,
	.block_erasers	=
	{
		{
			.eraseblocks = { {1, MIN_REAL_CHIP_SIZE} },
			.block_erase = TEST_ERASE_INJECTOR_1,
		}, {
			.eraseblocks = { {8, MIN_REAL_CHIP_SIZE / 8} },
			.block_erase = TEST_ERASE_INJECTOR_4,
		}, {
			.eraseblocks = { {16, MIN_REAL_CHIP_SIZE / 16} },
			.block_erase = TEST_ERASE_INJECTOR_5,
		}
	},
};

static struct flashchip chip_8_16 = {
	.vendor		= "aklm",
	/* See comment on previous chip. */
	.total_size	= 1,
	.tested		= TEST_OK_PREW,
	.gran		= WRITE_GRAN_1BYTE,
	.read		= TEST_READ_INJECTOR,
	.write		= TEST_WRITE_INJECTOR,
	.block_erasers	=
	{
		{
			.eraseblocks = { {8, MIN_REAL_CHIP_SIZE / 8} },
			.block_erase = TEST_ERASE_INJECTOR_4,
		}, {
			.eraseblocks = { {16, MIN_REAL_CHIP_SIZE / 16} },
			.block_erase = TEST_ERASE_INJECTOR_5,
		}
	},
};

#define BLOCK_ERASE_FUNC(n) \
	static int block_erase_chip_ ## n (struct flashctx *flash, unsigned int blockaddr, unsigned int blocklen) { \
		return block_erase_chip_tagged(flash, TEST_ERASE_INJECTOR_ ## n, blockaddr, blocklen); \
	}
BLOCK_ERASE_FUNC(1)
BLOCK_ERASE_FUNC(2)
BLOCK_ERASE_FUNC(3)
BLOCK_ERASE_FUNC(4)
BLOCK_ERASE_FUNC(5)

/*
 * Returns the offset how far we need to verify mock chip memory.
 * Which is minimum out of MOCK_CHIP_SIZE and the end of the logical layout.
 */
static chipoff_t setup_chip(struct flashrom_flashctx *flashctx, struct flashrom_layout **layout,
			const char *programmer_param, struct test_case *current_test_case)
{
	chipoff_t verify_end_boundary = MOCK_CHIP_SIZE - 1;

	g_test_write_injector = write_chip;
	g_test_read_injector = read_chip;
	/* Each erasefunc corresponds to an operation that erases a block of
	 * the chip with a particular size in bytes. */
	memcpy(g_test_erase_injector,
		(erasefunc_t *const[]){
			block_erase_chip_1,	// 1 byte
			block_erase_chip_2,	// 2 bytes
			block_erase_chip_3,	// 4 bytes
			block_erase_chip_4,	// 8 bytes
			block_erase_chip_5,	// 16 bytes
		},
		sizeof(g_test_erase_injector)
	);

	/* First MOCK_CHIP_SIZE bytes have a meaning and set with given values for this test case. */
	memcpy(g_state.buf, current_test_case->initial_buf, MOCK_CHIP_SIZE);
	/* The rest of mock chip memory does not matter. */
	memset(g_state.buf + MOCK_CHIP_SIZE, ERASE_VALUE, MIN_REAL_CHIP_SIZE - MOCK_CHIP_SIZE);

	/* Clear eraseblock invocation records. */
	memset(g_state.eraseblocks_actual, 0, MOCK_CHIP_SIZE * sizeof(struct erase_invoke));
	g_state.eraseblocks_actual_ind = 0;

	/* Clear the tracking of each byte modified. */
	memset(g_state.was_modified, false, MIN_REAL_CHIP_SIZE);
	/* Clear the tracking of each byte verified. */
	memset(g_state.was_verified, false, MIN_REAL_CHIP_SIZE);

	/* Set the flag to verify after writing on chip */
	flashrom_flag_set(flashctx, FLASHROM_FLAG_VERIFY_AFTER_WRITE, true);

	flashctx->chip = current_test_case->chip;

	printf("Creating layout ... ");
	assert_int_equal(0, flashrom_layout_new(layout));

	/* Adding regions from test case. */
	int i = 0;
	while (current_test_case->regions[i].name != NULL) {
		assert_int_equal(0, flashrom_layout_add_region(*layout,
					current_test_case->regions[i].start,
					current_test_case->regions[i].end,
					current_test_case->regions[i].name));
		assert_int_equal(0, flashrom_layout_include_region(*layout, current_test_case->regions[i].name));

		if (current_test_case->regions[i].end < MOCK_CHIP_SIZE - 1)
			verify_end_boundary = current_test_case->regions[i].end;
		else
			verify_end_boundary = MOCK_CHIP_SIZE - 1;

		i++;
	}

	flashrom_layout_set(flashctx, *layout);
	printf("done\n");

	/*
	 * We need some programmer (any), and dummy is a very good one,
	 * because it doesn't need any mocking. So no extra complexity
	 * from a programmer side, and test can focus on working with the chip.
	 */
	printf("Dummyflasher initialising with param=\"%s\"... ", programmer_param);
	assert_int_equal(0, programmer_init(&programmer_dummy, programmer_param));
	/* Assignment below normally happens while probing, but this test is not probing. */
	flashctx->mst = &registered_masters[0];
	printf("done\n");

	return verify_end_boundary;
}

static void teardown_chip(struct flashrom_layout **layout)
{
	printf("Dummyflasher shutdown... ");
	assert_int_equal(0, programmer_shutdown());
	printf("done\n");

	printf("Releasing layout... ");
	flashrom_layout_release(*layout);
	printf("done\n");
}

/*
 * Setup all test cases.
 *
 * First half of test cases is set up for a chip with erasers: 1, 2, 4, 8, 16 bytes.
 * Second half repeates the same test cases for a chip with erasers: 1, 8, 16 bytes.
 * Tests from #16 onwards use the chip with erasers: 8, 16 bytes, to test unaligned layout regions.
 */
static struct test_case test_cases[] = {
	{
		/*
		 * Test case #0
		 *
		 * Initial vs written: all 16 bytes are different.
		 * One layout region for the whole chip.
		 * Chip with eraseblocks 1, 2, 4, 8, 16.
		 */
		.chip =		&chip_1_2_4_8_16,
		.regions =	{{0, MIN_REAL_CHIP_SIZE - 1, "whole chip"}},
		.initial_buf =	{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
				 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
				 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f},
		.eraseblocks_expected = {{0x0, 0x10, TEST_ERASE_INJECTOR_5}},
		.eraseblocks_expected_ind = 1,
		.erase_test_name = "Erase test case #0",
		.write_test_name = "Write test case #0",
	}, {
		/*
		 * Test case #1
		 *
		 * Initial vs written: 9 bytes the same, 7 bytes different.
		 * Two layout regions each one 8 bytes, which is 1/2 size of chip.
		 * Chip with eraseblocks 1, 2, 4, 8, 16.
		 */
		.chip =		&chip_1_2_4_8_16,
		.regions =	{{0, MOCK_CHIP_SIZE/2 - 1, "part1"}, {MOCK_CHIP_SIZE/2, MIN_REAL_CHIP_SIZE - 1, "part2"}},
		.initial_buf =	{0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
				 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
				 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f},
		.eraseblocks_expected = {{0x8, 0x8, TEST_ERASE_INJECTOR_4}, {0x0, 0x8, TEST_ERASE_INJECTOR_4}},
		.eraseblocks_expected_ind = 2,
		.erase_test_name = "Erase test case #1",
		.write_test_name = "Write test case #1",
	}, {
		/*
		 * Test case #2
		 *
		 * Initial vs written: 6 bytes the same, 4 bytes different, 4 bytes the same, 2 bytes different.
		 * Two layout regions 11 and 5 bytes each.
		 * Chip with eraseblocks 1, 2, 4, 8, 16.
		 */
		.chip =		&chip_1_2_4_8_16,
		.regions =	{{0, 10, "odd1"}, {11, 15, "odd2"}, {MOCK_CHIP_SIZE, MIN_REAL_CHIP_SIZE - 1, "longtail"}},
		.initial_buf =	{0xff, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff,
				 0x0, 0xff, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0xff, 0xff, 0x0, 0xff, 0x0, 0xff, 0x20, 0x2f,
				 0x20, 0x2f, 0x0, 0xff, 0xff, 0xff, 0x2f, 0x2f},
		.eraseblocks_expected = {
			{0xb, 0x1, TEST_ERASE_INJECTOR_1},
			{0xc, 0x4, TEST_ERASE_INJECTOR_3},
			{0xa, 0x1, TEST_ERASE_INJECTOR_1},
			{0x8, 0x2, TEST_ERASE_INJECTOR_2},
			{0x0, 0x8, TEST_ERASE_INJECTOR_4}
		},
		.eraseblocks_expected_ind = 5,
		.erase_test_name = "Erase test case #2",
		.write_test_name = "Write test case #2",
	}, {
		/*
		 * Test case #3
		 *
		 * Initial vs written: 4 bytes the same, 4 bytes different, 8 bytes the same.
		 * One layout region covering the whole chip.
		 * Chip with eraseblocks 1, 2, 4, 8, 16.
		 */
		.chip =		&chip_1_2_4_8_16,
		.regions =	{{0, MIN_REAL_CHIP_SIZE - 1, "whole chip"}},
		.initial_buf =	{0xff, 0xff, 0xff, 0xff, 0x11, 0x22, 0x33, 0x44,
				 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0xff, 0xff, 0xff, 0xff, 0x1, 0x2, 0x3, 0x4,
				 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.eraseblocks_expected = {{0x0, 0x10, TEST_ERASE_INJECTOR_5}},
		.eraseblocks_expected_ind = 1,
		.erase_test_name = "Erase test case #3",
		.write_test_name = "Write test case #3",
	}, {
		/*
		 * Test case #4
		 *
		 * Initial vs written: 4 bytes different, 4 bytes the same, 8 bytes different.
		 * One layout region covering the whole chip.
		 * Chip with eraseblocks 1, 2, 4, 8, 16.
		 */
		.chip =		&chip_1_2_4_8_16,
		.regions =	{{0, MIN_REAL_CHIP_SIZE - 1, "whole chip"}},
		.initial_buf =	{0x1, 0x2, 0x3, 0x4, 0xff, 0xff, 0xff, 0xff,
				 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0x11, 0x22, 0x33, 0x44, 0xff, 0xff, 0xff, 0xff,
				 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8},
		.eraseblocks_expected = {{0x0, 0x10, TEST_ERASE_INJECTOR_5}},
		.eraseblocks_expected_ind = 1,
		.erase_test_name = "Erase test case #4",
		.write_test_name = "Write test case #4",
	}, {
		/*
		 * Test case #5
		 *
		 * Initial vs written: 7 bytes different, 1 bytes the same, 8 bytes different.
		 * One layout region covering the whole chip.
		 * Chip with eraseblocks 1, 2, 4, 8, 16.
		 */
		.chip =		&chip_1_2_4_8_16,
		.regions =	{{0, MIN_REAL_CHIP_SIZE - 1, "whole chip"}},
		.initial_buf =	{0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0xff,
				 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xff,
				 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8},
		.eraseblocks_expected = {{0x0, 0x10, TEST_ERASE_INJECTOR_5}},
		.eraseblocks_expected_ind = 1,
		.erase_test_name = "Erase test case #5",
		.write_test_name = "Write test case #5",
	}, {
		/*
		 * Test case #6
		 *
		 * Initial vs written: 7 bytes the same, 1 bytes different, 8 bytes the same.
		 * One layout region covering the whole chip.
		 * Chip with eraseblocks 1, 2, 4, 8, 16.
		 */
		.chip =		&chip_1_2_4_8_16,
		.regions =	{{0, MIN_REAL_CHIP_SIZE - 1, "whole chip"}},
		.initial_buf =	{0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x1d,
				 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0xdd,
				 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8},
		.eraseblocks_expected = {{0x0, 0x10, TEST_ERASE_INJECTOR_5}},
		.eraseblocks_expected_ind = 1,
		.erase_test_name = "Erase test case #6",
		.write_test_name = "Write test case #6",
	}, {
		/*
		 * Test case #7
		 *
		 * Initial vs written: all 16 bytes are different.
		 * Layout with irregular regions unaligned with eraseblocks.
		 * Chip with eraseblocks 1, 2, 4, 8, 16.
		 */
		.chip =		&chip_1_2_4_8_16,
		.regions =	{{0, 2, "reg3"}, {3, 7, "reg5"}, {8, 14, "reg7"}, {15, MIN_REAL_CHIP_SIZE - 1, "reg1"}},
		.initial_buf =	{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
				 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
				 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f},
		.eraseblocks_expected = {
			{0xf, 0x1, TEST_ERASE_INJECTOR_1},
			{0xe, 0x1, TEST_ERASE_INJECTOR_1},
			{0xc, 0x2, TEST_ERASE_INJECTOR_2},
			{0x8, 0x4, TEST_ERASE_INJECTOR_3},
			{0x3, 0x1, TEST_ERASE_INJECTOR_1},
			{0x4, 0x4, TEST_ERASE_INJECTOR_3},
			{0x2, 0x1, TEST_ERASE_INJECTOR_1},
			{0x0, 0x2, TEST_ERASE_INJECTOR_2}
		},
		.eraseblocks_expected_ind = 8,
		.erase_test_name = "Erase test case #7",
		.write_test_name = "Write test case #7",
	}, {
		/*
		 * Test case #8
		 *
		 * Initial vs written: all 16 bytes are different.
		 * One layout region for the whole chip.
		 * Chip with eraseblocks 1, 8, 16.
		 */
		.chip =		&chip_1_8_16,
		.regions =	{{0, MIN_REAL_CHIP_SIZE - 1, "whole chip"}},
		.initial_buf =	{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
				 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
				 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f},
		.eraseblocks_expected = {{0x0, 0x10, TEST_ERASE_INJECTOR_5}},
		.eraseblocks_expected_ind = 1,
		.erase_test_name = "Erase test case #8",
		.write_test_name = "Write test case #8",
	}, {
		/*
		 * Test case #9
		 *
		 * Initial vs written: 9 bytes the same, 7 bytes different.
		 * Two layout regions each one 8 bytes, which is 1/2 size of chip.
		 * Chip with eraseblocks 1, 8, 16.
		 */
		.chip =		&chip_1_8_16,
		.regions =	{{0, MOCK_CHIP_SIZE/2 - 1, "part1"}, {MOCK_CHIP_SIZE/2, MOCK_CHIP_SIZE - 1, "part2"},
					{MOCK_CHIP_SIZE, MIN_REAL_CHIP_SIZE - 1, "longtail"}},
		.initial_buf =	{0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
				 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
				 0xf8, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f},
		.eraseblocks_expected = {{0x8, 0x8, TEST_ERASE_INJECTOR_4}, {0x0, 0x8, TEST_ERASE_INJECTOR_4}},
		.eraseblocks_expected_ind = 2,
		.erase_test_name = "Erase test case #9",
		.write_test_name = "Write test case #9",
	}, {
		/*
		 * Test case #10
		 *
		 * Initial vs written: 6 bytes the same, 4 bytes different, 4 bytes the same, 2 bytes different.
		 * Two layout regions 11 and 5 bytes each.
		 * Chip with eraseblocks 1, 8, 16.
		 */
		.chip =		&chip_1_8_16,
		.regions =	{{0, 10, "odd1"}, {11, 15, "odd2"}, {MOCK_CHIP_SIZE, MIN_REAL_CHIP_SIZE - 1, "longtail"}},
		.initial_buf =	{0xff, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff,
				 0x0, 0xff, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0xff, 0xff, 0x0, 0xff, 0x0, 0xff, 0x20, 0x2f,
				 0x20, 0x2f, 0x0, 0xff, 0xff, 0xff, 0x2f, 0x2f},
		.eraseblocks_expected = {
			{0xb, 0x1, TEST_ERASE_INJECTOR_1},
			{0xc, 0x1, TEST_ERASE_INJECTOR_1},
			{0xd, 0x1, TEST_ERASE_INJECTOR_1},
			{0xe, 0x1, TEST_ERASE_INJECTOR_1},
			{0xf, 0x1, TEST_ERASE_INJECTOR_1},
			{0x8, 0x1, TEST_ERASE_INJECTOR_1},
			{0x9, 0x1, TEST_ERASE_INJECTOR_1},
			{0xa, 0x1, TEST_ERASE_INJECTOR_1},
			{0x0, 0x8, TEST_ERASE_INJECTOR_4}
		},
		.eraseblocks_expected_ind = 9,
		.erase_test_name = "Erase test case #10",
		.write_test_name = "Write test case #10",
	}, {
		/*
		 * Test case #11
		 *
		 * Initial vs written: 4 bytes the same, 4 bytes different, 8 bytes the same.
		 * One layout region covering the whole chip.
		 * Chip with eraseblocks 1, 8, 16.
		 */
		.chip =		&chip_1_8_16,
		.regions =	{{0, MIN_REAL_CHIP_SIZE - 1, "whole chip"}},
		.initial_buf =	{0xff, 0xff, 0xff, 0xff, 0x11, 0x22, 0x33, 0x44,
				 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0xff, 0xff, 0xff, 0xff, 0x1, 0x2, 0x3, 0x4,
				 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.eraseblocks_expected = {{0x0, 0x10, TEST_ERASE_INJECTOR_5}},
		.eraseblocks_expected_ind = 1,
		.erase_test_name = "Erase test case #11",
		.write_test_name = "Write test case #11",
	}, {
		/*
		 * Test case #12
		 *
		 * Initial vs written: 4 bytes different, 4 bytes the same, 8 bytes different.
		 * One layout region covering the whole chip.
		 * Chip with eraseblocks 1, 8, 16.
		 */
		.chip =		&chip_1_8_16,
		.regions =	{{0, MIN_REAL_CHIP_SIZE - 1, "whole chip"}},
		.initial_buf =	{0x1, 0x2, 0x3, 0x4, 0xff, 0xff, 0xff, 0xff,
				 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0x11, 0x22, 0x33, 0x44, 0xff, 0xff, 0xff, 0xff,
				 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8},
		.eraseblocks_expected = {{0x0, 0x10, TEST_ERASE_INJECTOR_5}},
		.eraseblocks_expected_ind = 1,
		.erase_test_name = "Erase test case #12",
		.write_test_name = "Write test case #12",
	}, {
		/*
		 * Test case #13
		 *
		 * Initial vs written: 7 bytes different, 1 bytes the same, 8 bytes different.
		 * One layout region covering the whole chip.
		 * Chip with eraseblocks 1, 8, 16.
		 */
		.chip =		&chip_1_8_16,
		.regions =	{{0, MIN_REAL_CHIP_SIZE - 1, "whole chip"}},
		.initial_buf =	{0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0xff,
				 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xff,
				 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8},
		.eraseblocks_expected = {{0x0, 0x10, TEST_ERASE_INJECTOR_5}},
		.eraseblocks_expected_ind = 1,
		.erase_test_name = "Erase test case #13",
		.write_test_name = "Write test case #13",
	}, {
		/*
		 * Test case #14
		 *
		 * Initial vs written: 7 bytes the same, 1 bytes different, 8 bytes the same.
		 * One layout region covering the whole chip.
		 * Chip with eraseblocks 1, 8, 16.
		 */
		.chip =		&chip_1_8_16,
		.regions =	{{0, MIN_REAL_CHIP_SIZE - 1, "whole chip"}},
		.initial_buf =	{0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x1d,
				 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0xdd,
				 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8},
		.eraseblocks_expected = {{0x0, 0x10, TEST_ERASE_INJECTOR_5}},
		.eraseblocks_expected_ind = 1,
		.erase_test_name = "Erase test case #14",
		.write_test_name = "Write test case #14",
	}, {
		/*
		 * Test case #15
		 *
		 * Initial vs written: all 16 bytes are different.
		 * Layout with irregular regions unaligned with eraseblocks.
		 * Chip with eraseblocks 1, 8, 16.
		 */
		.chip =		&chip_1_8_16,
		.regions =	{{0, 2, "reg3"}, {3, 7, "reg5"}, {8, 14, "reg7"},
				 {15, MIN_REAL_CHIP_SIZE - 1, "reg1"}},
		.initial_buf =	{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
				 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
				 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f},
		.eraseblocks_expected = {
			{0xf, 0x1, TEST_ERASE_INJECTOR_1},
			{0x8, 0x1, TEST_ERASE_INJECTOR_1},
			{0x9, 0x1, TEST_ERASE_INJECTOR_1},
			{0xa, 0x1, TEST_ERASE_INJECTOR_1},
			{0xb, 0x1, TEST_ERASE_INJECTOR_1},
			{0xc, 0x1, TEST_ERASE_INJECTOR_1},
			{0xd, 0x1, TEST_ERASE_INJECTOR_1},
			{0xe, 0x1, TEST_ERASE_INJECTOR_1},
			{0x3, 0x1, TEST_ERASE_INJECTOR_1},
			{0x4, 0x1, TEST_ERASE_INJECTOR_1},
			{0x5, 0x1, TEST_ERASE_INJECTOR_1},
			{0x6, 0x1, TEST_ERASE_INJECTOR_1},
			{0x7, 0x1, TEST_ERASE_INJECTOR_1},
			{0x0, 0x1, TEST_ERASE_INJECTOR_1},
			{0x1, 0x1, TEST_ERASE_INJECTOR_1},
			{0x2, 0x1, TEST_ERASE_INJECTOR_1},
		},
		.eraseblocks_expected_ind = 16,
		.erase_test_name = "Erase test case #15",
		.write_test_name = "Write test case #15",
	}, {
		/*
		 * Test case #16
		 *
		 * Initial vs written: all 16 bytes are different.
		 * Layout with unaligned regions 2+4+9+1b which are smaller than the smallest eraseblock.
		 * Chip with eraseblocks 8, 16.
		 */
		.chip =		&chip_8_16,
		.regions =	{{0, 1, "reg2"}, {2, 5, "reg4"}, {6, 14, "reg9"},
				 {15, MIN_REAL_CHIP_SIZE - 1, "reg1"}},
		.initial_buf =	{0x4, 0x4, 0x5, 0x5, 0x5, 0x5, 0x6, 0x6,
				 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x7},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16,
				 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x17},
		.eraseblocks_expected = {
			{0x8, 0x8, TEST_ERASE_INJECTOR_4},
			{0x0, 0x10, TEST_ERASE_INJECTOR_5},
			{0x0, 0x8, TEST_ERASE_INJECTOR_4},
			{0x0, 0x8, TEST_ERASE_INJECTOR_4},
		},
		.eraseblocks_expected_ind = 4,
		.erase_test_name = "Erase test case #16",
		.write_test_name = "Write test case #16",
	}, {
		/*
		 * Test case #17
		 *
		 * Initial vs written: all 16 bytes are different.
		 * Layout with unaligned region 3+13b which are smaller than the smallest eraseblock.
		 * Chip with eraseblocks 8, 16.
		 */
		.chip =		&chip_8_16,
		.regions =	{{0, 2, "reg3"}, {3, MIN_REAL_CHIP_SIZE - 1, "tail"}},
		.initial_buf =	{0x4, 0x4, 0x4, 0x6, 0x6, 0x6, 0x6, 0x6,
				 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0x14, 0x14, 0x14, 0x16, 0x16, 0x16, 0x16, 0x16,
				 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16},
		.eraseblocks_expected = {{0x0, 0x10, TEST_ERASE_INJECTOR_5}, {0x0, 0x8, TEST_ERASE_INJECTOR_4}},
		.eraseblocks_expected_ind = 2,
		.erase_test_name = "Erase test case #17",
		.write_test_name = "Write test case #17",
	}, {
		/*
		 * Test case #18
		 *
		 * Initial vs written: all 16 bytes are different.
		 * Layout with unaligned region 9+7b.
		 * Chip with eraseblocks 8, 16.
		 */
		.chip =		&chip_8_16,
		.regions =	{{0, 8, "reg9"}, {9, MIN_REAL_CHIP_SIZE - 1, "tail"}},
		.initial_buf =	{0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4,
				 0x4, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
				 0x14, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16},
		.eraseblocks_expected = {{0x8, 0x8, TEST_ERASE_INJECTOR_4}, {0x0, 0x10, TEST_ERASE_INJECTOR_5}},
		.eraseblocks_expected_ind = 2,
		.erase_test_name = "Erase test case #18",
		.write_test_name = "Write test case #18",
	}, {
		/*
		 * Test case #19
		 *
		 * Initial vs written: 3 bytes of the logical layout are different, rest is the same.
		 * Layout with unaligned region 3 bytes. Layout does not cover the whole chip memory.
		 * Chip memory outside of logical layout is skipped by both erase and write ops.
		 * Chip with eraseblocks 8, 16.
		 */
		.chip =		&chip_8_16,
		.regions =	{{0, 2, "reg3"}},
		.initial_buf =	{0x4, 0x4, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0,
				 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, 0x0,
					0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
		.written_buf =  {0x14, 0x14, 0x14, 0x0, 0x0, 0x0, 0x0, 0x0,
				 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
		.eraseblocks_expected = {{0x0, 0x8, TEST_ERASE_INJECTOR_4}},
		.eraseblocks_expected_ind = 1,
		.erase_test_name = "Erase test case #19",
		.write_test_name = "Write test case #19",
	},
};


#define START_PROTECTED_REGION 6
#define END_PROTECTED_REGION 13

/*
 * Setup all test cases with protected region.
 * Protected region is the same for all test cases, between bytes START_PROTECTED_REGION and up to END_PROTECTED_REGION.
 */
static struct test_case test_cases_protected_region[] = {
	{
		/*
		 * Test case #0
		 *
		 * Initial vs written: all 16 bytes are different.
		 * One layout region for the whole chip.
		 * Chip with eraseblocks 1, 2, 4, 8, 16.
		 */
		.chip =		&chip_1_2_4_8_16,
		.regions =	{{0, MIN_REAL_CHIP_SIZE - 1, "whole chip"}},
		.initial_buf =	{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
				 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, 0x6, 0x7,
					0x8, 0x9, 0xa, 0xb, 0xc, 0xd, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
				 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff},
		.written_protected_buf =  {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0x6, 0x7,
						0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xfe, 0xff},
		.eraseblocks_expected = {{0x4, 0x2, TEST_ERASE_INJECTOR_2}, {0x0, 0x4, TEST_ERASE_INJECTOR_3},
					{0xe, 0x2, TEST_ERASE_INJECTOR_2}},
		.eraseblocks_expected_ind = 3,
		.erase_test_name = "Erase protected region test case #0",
		.write_test_name = "Write protected region test case #0",
	}, {
		/*
		 * Test case #1
		 *
		 * Initial vs written: all 16 bytes are different.
		 * Two layout regions each one 8 bytes, which is 1/2 size of chip.
		 * Chip with eraseblocks 1, 2, 4, 8, 16.
		 */
		.chip =		&chip_1_2_4_8_16,
		.regions =	{{0, MOCK_CHIP_SIZE/2 - 1, "part1"}, {MOCK_CHIP_SIZE/2, MIN_REAL_CHIP_SIZE - 1, "part2"}},
		.initial_buf =	{0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
				 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, 0xf6, 0xf7,
					0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
				 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf},
		.written_protected_buf =  {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xf6, 0xf7,
						0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xae, 0xaf},
		.eraseblocks_expected = {{0xe, 0x2, TEST_ERASE_INJECTOR_2}, {0x4, 0x2, TEST_ERASE_INJECTOR_2},
					{0x0, 0x4, TEST_ERASE_INJECTOR_3}},
		.eraseblocks_expected_ind = 3,
		.erase_test_name = "Erase protected region test case #1",
		.write_test_name = "Write protected region test case #1",
	}, {
		/*
		 * Test case #2
		 *
		 * Initial vs written: all 16 bytes are different.
		 * Three layout regions 8+4+4b
		 * Chip with eraseblocks 1, 2, 4, 8, 16.
		 */
		.chip =		&chip_1_2_4_8_16,
		.regions =	{{0, 7, "odd1"}, {8, 11, "odd2"}, {12, 15, "odd3"},
				{MOCK_CHIP_SIZE, MIN_REAL_CHIP_SIZE - 1, "longtail"}},
		.initial_buf =	{0xff, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff,
				 0x0, 0xff, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, 0x0, 0xff,
					0x0, 0xff, 0x0, 0xff, 0xff, 0xff, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
				 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf},
		.written_protected_buf =  {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0x0, 0xff,
						0x0, 0xff, 0x0, 0xff, 0xff, 0xff, 0xae, 0xaf},
		.eraseblocks_expected = {{0xe, 0x2, TEST_ERASE_INJECTOR_2}, {0x4, 0x2, TEST_ERASE_INJECTOR_2},
					{0x0, 0x4, TEST_ERASE_INJECTOR_3}},
		.eraseblocks_expected_ind = 3,
		.erase_test_name = "Erase protected region test case #2",
		.write_test_name = "Write protected region test case #2",
	},  {
		/*
		 * Test case #3
		 *
		 * Initial vs written: all 16 bytes are different.
		 * Layout with unaligned regions 2+4+9+1b which require use of the 1-byte erase block.
		 * Chip with eraseblocks 1, 2, 4, 8, 16.
		 */
		.chip =		&chip_1_2_4_8_16,
		.regions =	{{0, 1, "reg2"}, {2, 5, "reg4"}, {6, 14, "reg9"},
				 {15, MIN_REAL_CHIP_SIZE - 1, "reg1"}},
		.initial_buf =	{0x4, 0x4, 0x5, 0x5, 0x5, 0x5, 0x6, 0x6,
				 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x7},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, 0x6, 0x6,
					0x6, 0x6, 0x6, 0x6, 0x6, 0x6, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
				 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf},
		.written_protected_buf =  {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0x6, 0x6,
						0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0xae, 0xaf},
		.eraseblocks_expected = {{0xf, 0x1, TEST_ERASE_INJECTOR_1}, {0xe, 0x1, TEST_ERASE_INJECTOR_1},
					{0x2, 0x2, TEST_ERASE_INJECTOR_2}, {0x4, 0x2, TEST_ERASE_INJECTOR_2},
					{0x0, 0x2, TEST_ERASE_INJECTOR_2}},
		.eraseblocks_expected_ind = 5,
		.erase_test_name = "Erase protected region test case #3",
		.write_test_name = "Write protected region test case #3",
	}, {
		/*
		 * Test case #4
		 *
		 * Initial vs written: all 16 bytes are different.
		 * Layout with unaligned region 3+13b which require use of the 1-byte erase block.
		 * Chip with eraseblocks 1, 8, 16.
		 */
		.chip =		&chip_1_8_16,
		.regions =	{{0, 2, "reg3"}, {3, MIN_REAL_CHIP_SIZE - 1, "tail"}},
		.initial_buf =	{0x4, 0x4, 0x4, 0x6, 0x6, 0x6, 0x6, 0x6,
				 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, 0x6, 0x6,
					0x6, 0x6, 0x6, 0x6, 0x6, 0x6, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
				 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf},
		.written_protected_buf =  {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0x6, 0x6,
						0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0xae, 0xaf},
		.eraseblocks_expected = {
			{0x3, 0x1, TEST_ERASE_INJECTOR_1},
			{0x4, 0x1, TEST_ERASE_INJECTOR_1},
			{0x5, 0x1, TEST_ERASE_INJECTOR_1},
			{0xe, 0x1, TEST_ERASE_INJECTOR_1},
			{0xf, 0x1, TEST_ERASE_INJECTOR_1},
			{0x0, 0x1, TEST_ERASE_INJECTOR_1},
			{0x1, 0x1, TEST_ERASE_INJECTOR_1},
			{0x2, 0x1, TEST_ERASE_INJECTOR_1},
		},
		.eraseblocks_expected_ind = 8,
		.erase_test_name = "Erase protected region test case #4",
		.write_test_name = "Write protected region test case #4",
	}, {
		/*
		 * Test case #5
		 *
		 * Initial vs written: all 16 bytes are different.
		 * Layout with unaligned region 9+7b.
		 * Chip with eraseblocks 1, 8, 16.
		 */
		.chip =		&chip_1_8_16,
		.regions =	{{0, 8, "reg9"}, {9, MIN_REAL_CHIP_SIZE - 1, "tail"}},
		.initial_buf =	{0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4,
				 0x4, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6},
		.erased_buf =	{ERASE_VALUE, ERASE_VALUE, ERASE_VALUE, ERASE_VALUE,
					ERASE_VALUE, ERASE_VALUE, 0x4, 0x4,
					0x4, 0x6, 0x6, 0x6, 0x6, 0x6, ERASE_VALUE, ERASE_VALUE},
		.written_buf =  {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
				 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf},
		.written_protected_buf =  {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0x4, 0x4,
						0x4, 0x6, 0x6, 0x6, 0x6, 0x6, 0xae, 0xaf},
		.eraseblocks_expected = {
			{0xe, 0x1, TEST_ERASE_INJECTOR_1},
			{0xf, 0x1, TEST_ERASE_INJECTOR_1},
			{0x0, 0x1, TEST_ERASE_INJECTOR_1},
			{0x1, 0x1, TEST_ERASE_INJECTOR_1},
			{0x2, 0x1, TEST_ERASE_INJECTOR_1},
			{0x3, 0x1, TEST_ERASE_INJECTOR_1},
			{0x4, 0x1, TEST_ERASE_INJECTOR_1},
			{0x5, 0x1, TEST_ERASE_INJECTOR_1},
		},
		.eraseblocks_expected_ind = 8,
		.erase_test_name = "Erase protected region test case #5",
		.write_test_name = "Write protected region test case #5",
	},
};

static int setup(void **state) {
	struct test_case *current_test_case = *state;
	g_state.current_test_case = current_test_case;
	return 0;
}

static int teardown(void **state) {
	return 0;
}

/*
 * Creates the array of tests for each test case in test_cases[].
 * Each test_case produces two tests: one for erase and one for write operation.
 * The caller needs to free the allocated memory.
 */
struct CMUnitTest *get_erase_func_algo_tests(size_t *num_tests) {
	const size_t test_cases_num = ARRAY_SIZE(test_cases);

	// Twice the number of test cases, because each test case is run twice: for erase and write.
	struct CMUnitTest *all_cases = calloc(test_cases_num * 2, sizeof(struct CMUnitTest));
	*num_tests = test_cases_num * 2;

	for (size_t i = 0; i < test_cases_num; i++) {
		all_cases[i] = (struct CMUnitTest) {
				.name		= test_cases[i].erase_test_name,
				.setup_func	= setup,
				.teardown_func	= teardown,
				.initial_state	= &test_cases[i],
				.test_func	= erase_function_algo_test_success,
		};
		all_cases[i + test_cases_num] = (struct CMUnitTest) {
				.name		= test_cases[i].write_test_name,
				.setup_func	= setup,
				.teardown_func	= teardown,
				.initial_state	= &test_cases[i],
				.test_func	= write_function_algo_test_success,
		};
	}

	return all_cases;
}

static void test_erase_fails_for_unwritable_region(void **);
static void erase_unwritable_regions_skipflag_on_test_success(void **);
static void write_unwritable_regions_skipflag_on_test_success(void **);

/*
 * Creates the array of tests for each test case in test_cases_protected_region[].
 * The caller needs to free the allocated memory.
 */
struct CMUnitTest *get_erase_protected_region_algo_tests(size_t *num_tests) {
	const size_t num_parameterized = ARRAY_SIZE(test_cases_protected_region);
	const size_t num_unparameterized = 1;
	// Twice the number of parameterized test cases, because each test case is run twice:
	// for erase and write.
	const size_t num_cases = num_parameterized * 2 + num_unparameterized;

	struct CMUnitTest *all_cases = calloc(num_cases, sizeof(struct CMUnitTest));
	*num_tests = num_cases;

	for (size_t i = 0; i < num_parameterized; i++) {
		all_cases[i] = (struct CMUnitTest) {
				.name		= test_cases_protected_region[i].erase_test_name,
				.setup_func	= setup,
				.teardown_func	= teardown,
				.initial_state	= &test_cases_protected_region[i],
				.test_func	= erase_unwritable_regions_skipflag_on_test_success,
		};
		all_cases[i + num_parameterized] = (struct CMUnitTest) {
				.name		= test_cases_protected_region[i].write_test_name,
				.setup_func	= setup,
				.teardown_func	= teardown,
				.initial_state	= &test_cases_protected_region[i],
				.test_func	= write_unwritable_regions_skipflag_on_test_success,
		};
	}

	memcpy(
		&all_cases[num_parameterized * 2],
		(const struct CMUnitTest[]){
			(const struct CMUnitTest) {
				.name = "erase failure for unskipped unwritable regions",
				.test_func = test_erase_fails_for_unwritable_region,
			}
		},
		sizeof(*all_cases) * num_unparameterized
	);

	return all_cases;
}

/*
 * This function is invoked for every test case in test_cases[],
 * current test case is passed as an argument.
 */
void erase_function_algo_test_success(void **state)
{
	struct test_case* current_test_case = *state;

	int all_erase_tests_result = 0;
	struct flashrom_flashctx flashctx = { 0 };
	const char *param = ""; /* Default values for all params. */

	struct flashrom_layout *layout;

	const chipoff_t verify_end_boundary = setup_chip(&flashctx, &layout, param, current_test_case);

	printf("%s started.\n", current_test_case->erase_test_name);
	int ret = flashrom_flash_erase(&flashctx);
	printf("%s returned %d.\n", current_test_case->erase_test_name, ret);

	int chip_erased = !memcmp(g_state.buf, current_test_case->erased_buf, MOCK_CHIP_SIZE);

	int eraseblocks_in_order = !memcmp(g_state.eraseblocks_actual,
					current_test_case->eraseblocks_expected,
					current_test_case->eraseblocks_expected_ind * sizeof(struct erase_invoke));

	int eraseblocks_invocations = (g_state.eraseblocks_actual_ind ==
					current_test_case->eraseblocks_expected_ind);

	int chip_verified = 1;
	for (unsigned int i = 0; i <= verify_end_boundary; i++)
		if (g_state.was_modified[i] && !g_state.was_verified[i]) {
			chip_verified = 0; /* byte was modified, but not verified after */
			printf("Error: byte 0x%x, modified: %d, verified: %d\n", i, g_state.was_modified[i], g_state.was_verified[i]);
		}

	if (chip_erased)
		printf("Erased chip memory state for %s is CORRECT\n",
			current_test_case->erase_test_name);
	else
		printf("Erased chip memory state for %s is WRONG\n",
			current_test_case->erase_test_name);

	if (eraseblocks_in_order)
		printf("Eraseblocks order of invocation for %s is CORRECT\n",
			current_test_case->erase_test_name);
	else
		printf("Eraseblocks order of invocation for %s is WRONG\n",
			current_test_case->erase_test_name);

	if (eraseblocks_invocations)
		printf("Eraseblocks number of invocations for %s is CORRECT\n",
			current_test_case->erase_test_name);
	else
		printf("Eraseblocks number of invocations for %s is WRONG, expected %d actual %d\n",
			current_test_case->erase_test_name,
			current_test_case->eraseblocks_expected_ind,
			g_state.eraseblocks_actual_ind);

	if (chip_verified)
		printf("Erased chip memory state for %s was verified successfully\n",
			current_test_case->erase_test_name);
	else
		printf("Erased chip memory state for %s was NOT verified completely\n",
			current_test_case->erase_test_name);

	all_erase_tests_result |= ret;
	all_erase_tests_result |= !chip_erased;
	all_erase_tests_result |= !eraseblocks_in_order;
	all_erase_tests_result |= !eraseblocks_invocations;
	all_erase_tests_result |= !chip_verified;

	teardown_chip(&layout);

	assert_int_equal(0, all_erase_tests_result);
}

/*
 * This function is invoked for every test case in test_cases[],
 * current test case is passed as an argument.
 */
void write_function_algo_test_success(void **state)
{
	struct test_case* current_test_case = *state;

	int all_write_test_result = 0;
	struct flashrom_flashctx flashctx = { 0 };
	uint8_t newcontents[MIN_BUF_SIZE];
	const char *param = ""; /* Default values for all params. */

	struct flashrom_layout *layout;

	const chipoff_t verify_end_boundary = setup_chip(&flashctx, &layout, param, current_test_case);
	memcpy(&newcontents, current_test_case->written_buf, MOCK_CHIP_SIZE);

	printf("%s started.\n", current_test_case->write_test_name);
	int ret = flashrom_image_write(&flashctx, &newcontents, MIN_BUF_SIZE, NULL);
	printf("%s returned %d.\n", current_test_case->write_test_name, ret);

	int chip_written = !memcmp(g_state.buf, current_test_case->written_buf, MOCK_CHIP_SIZE);

	int chip_verified = 1;
	for (unsigned int i = 0; i <= verify_end_boundary; i++)
		if (g_state.was_modified[i] && !g_state.was_verified[i]) {
			chip_verified = 0; /* the byte was modified, but not verified after */
			printf("Error: byte 0x%x, modified: %d, verified: %d\n", i, g_state.was_modified[i], g_state.was_verified[i]);
		}

	if (chip_written)
		printf("Written chip memory state for %s is CORRECT\n",
			current_test_case->write_test_name);
	else
		printf("Written chip memory state for %s is WRONG\n",
			current_test_case->write_test_name);

	if (chip_verified)
		printf("Written chip memory state for %s was verified successfully\n",
			current_test_case->write_test_name);
	else
		printf("Written chip memory state for %s was NOT verified completely\n",
			current_test_case->write_test_name);

	all_write_test_result |= ret;
	all_write_test_result |= !chip_written;
	all_write_test_result |= !chip_verified;

	teardown_chip(&layout);

	assert_int_equal(0, all_write_test_result);
}

static void get_protected_region(const struct flashctx *flash, unsigned int addr, struct flash_region *region)
{
	if (addr < 20)
		printf("Inside test get_protected_region for addr=0x%x\n", addr);

	if (addr < START_PROTECTED_REGION) {
		region->name		= strdup("not protected");
		region->start		= 0;
		region->end		= START_PROTECTED_REGION - 1;
		region->read_prot	= false;
		region->write_prot	= false;
	} else if (addr <= END_PROTECTED_REGION) {
		region->name		= strdup("protected");
		region->start		= START_PROTECTED_REGION;
		region->end		= END_PROTECTED_REGION;
		region->read_prot	= false;
		region->write_prot	= true;
	} else {
		region->name		= strdup("tail");
		region->start		= END_PROTECTED_REGION + 1;
		region->end		= flashrom_flash_getsize(flash) - 1;
		region->read_prot	= false;
		region->write_prot	= false;
	}
}

static int block_erase_chip_with_protected_region(struct flashctx *flash, enum block_erase_func erase_func, unsigned int blockaddr, unsigned int blocklen)
{
	if (blockaddr + blocklen <= MOCK_CHIP_SIZE) {
		LOG_ERASE_FUNC;

		/* Register eraseblock invocation. */
		g_state.eraseblocks_actual[g_state.eraseblocks_actual_ind] = (struct erase_invoke){
			.erase_func = erase_func,
			.blocklen = blocklen,
			.blockaddr = blockaddr,
		};
		g_state.eraseblocks_actual_ind++;
	}

	assert_in_range(blockaddr + blocklen, 0, MIN_REAL_CHIP_SIZE);

	// Check we are not trying to erase protected region. This should not happen,
	// because the logic should handle protected regions and never invoke erasefn
	// for them. If this happens, means there is a bug in erasure logic, and test fails.
	//
	// Note: returning 1 instead of assert, so that the flow goes back to erasure code
	// to clean up the memory after failed erase. Memory leaks are also tested by unit tests.
	const unsigned int erase_op_size = 1 << (erase_func - TEST_ERASE_INJECTOR_1);
	if (blocklen < erase_op_size) {
		printf("Error: block length %d is smaller than erase_func length %d\n", blocklen, erase_op_size);
		return 1;
	}

	if ((blockaddr >= START_PROTECTED_REGION && blockaddr <= END_PROTECTED_REGION)
		|| (blockaddr + blocklen - 1 >= START_PROTECTED_REGION
			&& blockaddr + blocklen - 1 <= END_PROTECTED_REGION)
		|| (blockaddr < START_PROTECTED_REGION
		    && blockaddr + blocklen + 1 > END_PROTECTED_REGION)) {
		printf("Error: block with start=%d, len=%d overlaps protected region %d-%d\n",
			   blockaddr, blocklen, START_PROTECTED_REGION, END_PROTECTED_REGION);
		return 1;
	}

	memset(&g_state.buf[blockaddr], ERASE_VALUE, blocklen);

	/* Track the bytes were erased */
	memset(&g_state.was_modified[blockaddr], true, blocklen);
	/* Clear the records of previous verification, if there were any */
	memset(&g_state.was_verified[blockaddr], false, blocklen);

	return 0;
}

#define BLOCK_ERASE_PROTECTED_FUNC(i) static int block_erase_chip_with_protected_region_ ## i \
	(struct flashctx *flash, unsigned int blockaddr, unsigned int blocklen) { \
	    return block_erase_chip_with_protected_region(flash, TEST_ERASE_INJECTOR_ ## i, blockaddr, blocklen); \
	}
BLOCK_ERASE_PROTECTED_FUNC(1)
BLOCK_ERASE_PROTECTED_FUNC(2)
BLOCK_ERASE_PROTECTED_FUNC(3)
BLOCK_ERASE_PROTECTED_FUNC(4)
BLOCK_ERASE_PROTECTED_FUNC(5)

/*
 * Runs the test cases that use protected flash regions (regions returned from
 * get_flash_region() where write_prot is true) when the runtime flag to avoid
 * writing to those regions is enabled.
 *
 * These tests verify that no protected region is erased, and that the erase
 * commands used match the expected erase size (ensuring for example that a
 * command erasing 16 bytes is not used when only 8 should be erased).
 */
static void erase_unwritable_regions_skipflag_on_test_success(void **state)
{
	struct test_case* current_test_case = *state;

	int all_erase_tests_result = 0;
	struct flashrom_flashctx flashctx = { 0 };
	const char *param = ""; /* Default values for all params. */

	struct flashrom_layout *layout;

	const chipoff_t verify_end_boundary = setup_chip(&flashctx, &layout, param, current_test_case);

	// This test needs special block erase to emulate protected regions.
	memcpy(g_test_erase_injector,
		(erasefunc_t *const[]){
			block_erase_chip_with_protected_region_1,
			block_erase_chip_with_protected_region_2,
			block_erase_chip_with_protected_region_3,
			block_erase_chip_with_protected_region_4,
			block_erase_chip_with_protected_region_5,
		},
		sizeof(g_test_erase_injector)
	);

	flashrom_flag_set(&flashctx, FLASHROM_FLAG_SKIP_UNWRITABLE_REGIONS, true);

	// We use dummyflasher programmer in tests, but for this test we need to
	// replace dummyflasher's default get_region fn with test one.
	// The rest of master struct is fine for this test.
	// Note dummyflasher registers multiple masters by default, so replace
	// get_region for each of them.
	flashctx.mst->spi.get_region = &get_protected_region;
	flashctx.mst->opaque.get_region = &get_protected_region;

	printf("%s started.\n", current_test_case->erase_test_name);
	int ret = flashrom_flash_erase(&flashctx);
	printf("%s returned %d.\n", current_test_case->erase_test_name, ret);

	int chip_erased = !memcmp(g_state.buf, current_test_case->erased_buf, MOCK_CHIP_SIZE);

	int eraseblocks_in_order = !memcmp(g_state.eraseblocks_actual,
					current_test_case->eraseblocks_expected,
					current_test_case->eraseblocks_expected_ind * sizeof(struct erase_invoke));

	int eraseblocks_invocations = (g_state.eraseblocks_actual_ind ==
					current_test_case->eraseblocks_expected_ind);

	int chip_verified = 1;
	for (unsigned int i = 0; i <= verify_end_boundary; i++)
		if (g_state.was_modified[i] && !g_state.was_verified[i]) {
			chip_verified = 0; /* byte was modified, but not verified after */
			printf("Error: byte 0x%x, modified: %d, verified: %d\n", i, g_state.was_modified[i], g_state.was_verified[i]);
		}

	if (chip_erased)
		printf("Erased chip memory state for %s is CORRECT\n",
			current_test_case->erase_test_name);
	else
		printf("Erased chip memory state for %s is WRONG\n",
			current_test_case->erase_test_name);

	if (eraseblocks_in_order)
		printf("Eraseblocks order of invocation for %s is CORRECT\n",
			current_test_case->erase_test_name);
	else
		printf("Eraseblocks order of invocation for %s is WRONG\n",
			current_test_case->erase_test_name);

	if (eraseblocks_invocations)
		printf("Eraseblocks number of invocations for %s is CORRECT\n",
			current_test_case->erase_test_name);
	else
		printf("Eraseblocks number of invocations for %s is WRONG, expected %d actual %d\n",
			current_test_case->erase_test_name,
			current_test_case->eraseblocks_expected_ind,
			g_state.eraseblocks_actual_ind);

	if (chip_verified)
		printf("Erased chip memory state for %s was verified successfully\n",
			current_test_case->erase_test_name);
	else
		printf("Erased chip memory state for %s was NOT verified completely\n",
			current_test_case->erase_test_name);

	all_erase_tests_result |= ret;
	all_erase_tests_result |= !chip_erased;
	all_erase_tests_result |= !eraseblocks_in_order;
	all_erase_tests_result |= !eraseblocks_invocations;
	all_erase_tests_result |= !chip_verified;

	teardown_chip(&layout);

	assert_int_equal(0, all_erase_tests_result);
}

/*
 * Runs the test cases that use protected flash regions (regions returned from
 * get_flash_region() where write_prot is true) when the runtime flag to avoid
 * writing to those regions is enabled.
 *
 * These tests verify that no protected region is written, i.e. protected region
 * memory state stays untouched.
 */
static void write_unwritable_regions_skipflag_on_test_success(void **state) {
	struct test_case* current_test_case = *state;

	int all_write_tests_result = 0;
	struct flashrom_flashctx flashctx = { 0 };
	uint8_t newcontents[MIN_BUF_SIZE];
	uint8_t newcontents_protected[MIN_BUF_SIZE];
	const char *param = ""; /* Default values for all params. */

	struct flashrom_layout *layout;

	const chipoff_t verify_end_boundary = setup_chip(&flashctx, &layout, param, current_test_case);
	memcpy(&newcontents, current_test_case->written_buf, MOCK_CHIP_SIZE);

	// This test needs special block erase to emulate protected regions.
	memcpy(g_test_erase_injector,
		(erasefunc_t *const[]){
			block_erase_chip_with_protected_region_1,
			block_erase_chip_with_protected_region_2,
			block_erase_chip_with_protected_region_3,
			block_erase_chip_with_protected_region_4,
			block_erase_chip_with_protected_region_5,
		},
		sizeof(g_test_erase_injector)
	);

	flashrom_flag_set(&flashctx, FLASHROM_FLAG_SKIP_UNWRITABLE_REGIONS, true);
	flashrom_flag_set(&flashctx, FLASHROM_FLAG_SKIP_UNREADABLE_REGIONS, true);
	flashrom_flag_set(&flashctx, FLASHROM_FLAG_VERIFY_WHOLE_CHIP, false);
	/* We need to manually trigger verify op after write, because of protected regions */
	flashrom_flag_set(&flashctx, FLASHROM_FLAG_VERIFY_AFTER_WRITE, false);

	// We use dummyflasher programmer in tests, but for this test we need to
	// replace dummyflasher's default get_region fn with test one.
	// The rest of master struct is fine for this test.
	// Note dummyflasher registers multiple masters by default, so replace
	// get_region for each of them.
	flashctx.mst->spi.get_region = &get_protected_region;
	flashctx.mst->opaque.get_region = &get_protected_region;

	printf("%s started.\n", current_test_case->write_test_name);
	int ret = flashrom_image_write(&flashctx, &newcontents, MIN_BUF_SIZE, NULL);
	printf("%s returned %d.\n", current_test_case->write_test_name, ret);

	/* Expected end result leaves protected region untouched */
	memcpy(&newcontents_protected, current_test_case->written_protected_buf, MOCK_CHIP_SIZE);
	/* Outside of MOCK_CHIP_SIZE newcontents are not initialised in test cases, so just copy */
	memcpy(&newcontents_protected[MOCK_CHIP_SIZE],
		&newcontents[MOCK_CHIP_SIZE],
		MIN_BUF_SIZE - MOCK_CHIP_SIZE);
	printf("%s verification started.\n", current_test_case->write_test_name);
	ret = flashrom_image_verify(&flashctx, &newcontents_protected, MIN_BUF_SIZE);
	printf("%s verification returned %d.\n", current_test_case->write_test_name, ret);

	int chip_written = !memcmp(g_state.buf, current_test_case->written_protected_buf, MOCK_CHIP_SIZE);

	int chip_verified = 1;
	for (unsigned int i = 0; i <= verify_end_boundary; i++)
		if (g_state.was_modified[i] && !g_state.was_verified[i]) {
			chip_verified = 0; /* byte was modified, but not verified after */
			printf("Error: byte 0x%x, modified: %d, verified: %d\n", i, g_state.was_modified[i], g_state.was_verified[i]);
		}

	if (chip_written)
		printf("Written chip memory state for %s is CORRECT\n",
			current_test_case->write_test_name);
	else
		printf("Written chip memory state for %s is WRONG\n",
			current_test_case->write_test_name);

	if (chip_verified)
		printf("Written chip memory state for %s was verified successfully\n",
			current_test_case->write_test_name);
	else
		printf("Written chip memory state for %s was NOT verified completely\n",
			current_test_case->write_test_name);

	all_write_tests_result |= ret;
	all_write_tests_result |= !chip_written;
	all_write_tests_result |= !chip_verified;

	teardown_chip(&layout);

	assert_int_equal(0, all_write_tests_result);
}

static void test_erase_fails_for_unwritable_region(void **state) {
	struct flashrom_flashctx flashctx = {
		.chip = &chip_1_2_4_8_16,
	};
	assert_int_equal(0, programmer_init(&programmer_dummy, ""));
	flashctx.mst = &registered_masters[0];

	flashctx.mst->spi.get_region = &get_protected_region;
	flashctx.mst->opaque.get_region = &get_protected_region;
	flashrom_flag_set(&flashctx, FLASHROM_FLAG_SKIP_UNWRITABLE_REGIONS, false);

	/* Ask to erase one byte at the end of the unprotected region and one byte
	 * at the beginning of the protected one. If the check for unwritable regions
	 * wrongly treats the upper bound as exclusive, it will incorrectly try
	 * to erase inside the protected region. */
	struct flashrom_layout *layout;
	flashrom_layout_new(&layout);
	flashrom_layout_add_region(layout, 7, 8, "protected");
	flashrom_layout_include_region(layout, "protected");
	flashrom_layout_set(&flashctx, layout);

	int ret = flashrom_flash_erase(&flashctx);

	assert_int_equal(0, programmer_shutdown());
	flashrom_layout_release(layout);

	assert_int_not_equal(ret, 0);
}
