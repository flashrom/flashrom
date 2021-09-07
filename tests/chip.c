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
#include <stdio.h>
#include <string.h>

#include "chipdrivers.h"
#include "flash.h"
#include "programmer.h"

#define CHIP_TOTAL_SIZE 8192

static struct {
	unsigned int unlock_calls; /* how many times unlock function was called */
	uint8_t buf[CHIP_TOTAL_SIZE]; /* buffer of total size of chip, to emulate a chip */
} g_chip_state = {
	.unlock_calls = 0,
	.buf = { 0 },
};

int read_chip(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	printf("Read chip called with start=0x%x, len=0x%x\n", start, len);
	if (!g_chip_state.unlock_calls) {
		printf("Error while reading chip: unlock was not called.\n");
		return 1;
	}

	assert_in_range(start + len, 0, CHIP_TOTAL_SIZE);

	memcpy(buf, &g_chip_state.buf[start], len);
	return 0;
}

int write_chip(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	printf("Write chip called with start=0x%x, len=0x%x\n", start, len);
	if (!g_chip_state.unlock_calls) {
		printf("Error while writing chip: unlock was not called.\n");
		return 1;
	}

	assert_in_range(start + len, 0, CHIP_TOTAL_SIZE);

	memcpy(&g_chip_state.buf[start], buf, len);
	return 0;
}

int unlock_chip(struct flashctx *flash)
{
	printf("Unlock chip called\n");
	g_chip_state.unlock_calls++;

	if (g_chip_state.unlock_calls > 1) {
		printf("Error: Unlock called twice\n");
		return -1;
	}

	return 0;
}

int block_erase_chip(struct flashctx *flash, unsigned int blockaddr, unsigned int blocklen)
{
	printf("Block erase called with blockaddr=0x%x, blocklen=0x%x\n", blockaddr, blocklen);
	if (!g_chip_state.unlock_calls) {
		printf("Error while erasing chip: unlock was not called.\n");
		return 1;
	}

	assert_in_range(blockaddr + blocklen, 0, CHIP_TOTAL_SIZE);

	memset(&g_chip_state.buf[blockaddr], 0xff, blocklen);
	return 0;
}

static void setup_chip(struct flashrom_flashctx *flash, struct flashrom_layout **layout,
		struct flashchip *chip, const char *programmer_param)
{
	flash->chip = chip;

	g_chip_state.unlock_calls = 0;

	printf("Creating layout with one included region... ");
	assert_int_equal(0, flashrom_layout_new(layout));
	/* One region which covers total size of chip. */
	assert_int_equal(0, flashrom_layout_add_region(*layout, 0, chip->total_size * 1024 - 1, "region"));
	assert_int_equal(0, flashrom_layout_include_region(*layout, "region"));

	flashrom_layout_set(flash, *layout);
	printf("done\n");

	/*
	 * We need some programmer (any), and dummy is a very good one,
	 * because it doesn't need any mocking. So no extra complexity
	 * from a programmer side, and test can focus on working with the chip.
	 */
	printf("Dummyflasher initialising with param=\"%s\"... ", programmer_param);
	assert_int_equal(0, programmer_init(&programmer_dummy, programmer_param));
	/* Assignment below normally happens while probing, but this test is not probing. */
	flash->mst = &registered_masters[0];
	printf("done\n");
}

static void teardown(struct flashrom_layout **layout)
{
	printf("Dummyflasher shutdown... ");
	assert_int_equal(0, programmer_shutdown());
	printf("done\n");

	printf("Releasing layout... ");
	flashrom_layout_release(*layout);
	printf("done\n");
}

void erase_chip_test_success(void **state)
{
	(void) state; /* unused */

	struct flashchip chip = {
		.vendor		= "aklm",
		/*
		 * Total size less than 16 to skip some steps
		 * in flashrom.c#prepare_flash_access.
		 */
		.total_size	= 8,
		.tested		= TEST_OK_PREW,
		.read		= read_chip,
		.write		= write_chip,
		.unlock		= unlock_chip,
		.block_erasers	=
		{{
			 /* All blocks within total size of the chip. */
			.eraseblocks = { {2 * 1024, 4} },
			.block_erase = block_erase_chip,
		 }},
	};
	struct flashrom_flashctx flash = { 0 };
	struct flashrom_layout *layout;
	const char *param = ""; /* Default values for all params. */

	setup_chip(&flash, &layout, &chip, param);

	printf("Erase chip operation started.\n");
	assert_int_equal(0, do_erase(&flash));
	printf("Erase chip operation done.\n");

	teardown(&layout);
}

void erase_chip_with_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	struct flashchip chip = {
		.vendor		= "aklm&dummyflasher",
		/*
		 * Setup the values for W25Q128.V because we ask dummyflasher
		 * to emulate this chip. All operations: read/write/unlock/erase
		 * are real, not mocks, and they are expected to be handled by
		 * dummyflasher.
		 */
		.total_size	= 16 * 1024,
		.tested		= TEST_OK_PREW,
		.read		= spi_chip_read,
		.write		= spi_chip_write_256,
		.unlock         = spi_disable_blockprotect,
		.block_erasers  =
		{
			{
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 512} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
	};

	struct flashrom_flashctx flash = { 0 };
	struct flashrom_layout *layout;
	/*
	 * Dummyflasher is capable to emulate a chip, so we ask it to do this.
	 * Nothing to mock, dummy is taking care of this already.
	 */
	char *param_dup = strdup("bus=spi,emulate=W25Q128FV");

	setup_chip(&flash, &layout, &chip, param_dup);

	printf("Erase chip operation started.\n");
	assert_int_equal(0, do_erase(&flash));
	printf("Erase chip operation done.\n");

	teardown(&layout);

	free(param_dup);
}
