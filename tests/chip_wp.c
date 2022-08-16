/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2021 3mdeb Embedded Systems Consulting
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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
#include "libflashrom.h"
#include "programmer.h"
#include "tests.h"

/*
 * Tests in this file do not use any mocking, because using write-protect
 * emulation in dummyflasher programmer is sufficient
 */

#define LAYOUT_TAIL_REGION_START 0x1000

static void setup_chip(struct flashrom_flashctx *flash, struct flashrom_layout **layout,
		       struct flashchip *chip, const char *programmer_param)
{
	flash->chip = chip;

	if (layout) {
		const size_t tail_start = LAYOUT_TAIL_REGION_START;
		const size_t tail_len = chip->total_size * KiB - 1;

		assert_int_equal(0, flashrom_layout_new(layout));
		assert_int_equal(0, flashrom_layout_add_region(*layout, 0, tail_start - 1, "head"));
		assert_int_equal(0, flashrom_layout_add_region(*layout, tail_start, tail_len, "tail"));

		flashrom_layout_set(flash, *layout);
	}

	assert_int_equal(0, programmer_init(&programmer_dummy, programmer_param));
	/* Assignment below normally happens while probing, but this test is not probing. */
	flash->mst = &registered_masters[0];
}

static void teardown(struct flashrom_layout **layout)
{
	assert_int_equal(0, programmer_shutdown());
	if (layout)
		flashrom_layout_release(*layout);
}

/* Setup the struct for W25Q128.V, all values come from flashchips.c */
static const struct flashchip chip_W25Q128_V = {
	.vendor		= "aklm&dummyflasher",
	.total_size	= 16 * 1024,
	.tested		= TEST_OK_PREW,
	.read		= spi_chip_read,
	.write		= SPI_CHIP_WRITE256,
	.unlock         = spi_disable_blockprotect,
	.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR_EXT2 | FEATURE_WRSR2 | FEATURE_WRSR3,
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
	.reg_bits	=
	{
		.srp    = {STATUS1, 7, RW},
		.srl    = {STATUS2, 0, RW},
		.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}},
		.tb     = {STATUS1, 5, RW},
		.sec    = {STATUS1, 6, RW},
		.cmp    = {STATUS2, 6, RW},
		.wps    = {STATUS3, 2, RW},
	},
	.decode_range	= DECODE_RANGE_SPI25,
};

/* Trying to set an unsupported WP range fails */
void invalid_wp_range_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	char *param_dup = strdup("bus=spi,emulate=W25Q128FV,hwwp=no");

	struct flashrom_flashctx flash = { 0 };
	struct flashchip mock_chip = chip_W25Q128_V;
	struct flashrom_wp_cfg *wp_cfg;

	setup_chip(&flash, NULL, &mock_chip, param_dup);

	assert_int_equal(0, flashrom_wp_cfg_new(&wp_cfg));
	flashrom_wp_set_mode(wp_cfg, FLASHROM_WP_MODE_HARDWARE);
	flashrom_wp_set_range(wp_cfg, 0x1000, 0x1000);

	assert_int_equal(FLASHROM_WP_ERR_RANGE_UNSUPPORTED, flashrom_wp_write_cfg(&flash, wp_cfg));

	teardown(NULL);

	flashrom_wp_cfg_release(wp_cfg);
	free(param_dup);
}

/* Enabling hardware WP with a valid range succeeds */
void set_wp_range_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	char *param_dup = strdup("bus=spi,emulate=W25Q128FV,hwwp=no");

	struct flashrom_flashctx flash = { 0 };
	struct flashchip mock_chip = chip_W25Q128_V;
	struct flashrom_wp_cfg *wp_cfg;

	size_t start;
	size_t len;

	setup_chip(&flash, NULL, &mock_chip, param_dup);

	/* Use last 4 KiB for a range. */
	assert_int_equal(0, flashrom_wp_cfg_new(&wp_cfg));
	flashrom_wp_set_mode(wp_cfg, FLASHROM_WP_MODE_HARDWARE);
	flashrom_wp_set_range(wp_cfg, mock_chip.total_size * KiB - 4 * KiB, 4 * KiB);

	assert_int_equal(0, flashrom_wp_write_cfg(&flash, wp_cfg));

	/* Check that range was set correctly. */
	assert_int_equal(0, flashrom_wp_read_cfg(wp_cfg, &flash));
	flashrom_wp_get_range(&start, &len, wp_cfg);
	assert_int_equal(16 * MiB - 4 * KiB, start);
	assert_int_equal(4 * KiB, len);

	teardown(NULL);

	flashrom_wp_cfg_release(wp_cfg);
	free(param_dup);
}

/* Enable hardware WP and verify that it can not be unset */
void switch_wp_mode_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	char *param_dup = strdup("bus=spi,emulate=W25Q128FV,hwwp=yes");

	struct flashrom_flashctx flash = { 0 };
	struct flashchip mock_chip = chip_W25Q128_V;
	struct flashrom_wp_cfg *wp_cfg;

	setup_chip(&flash, NULL, &mock_chip, param_dup);

	assert_int_equal(0, flashrom_wp_cfg_new(&wp_cfg));

	/* Check initial mode. */
	assert_int_equal(0, flashrom_wp_read_cfg(wp_cfg, &flash));
	assert_int_equal(FLASHROM_WP_MODE_DISABLED, flashrom_wp_get_mode(wp_cfg));

	/* Enable hardware protection, which can't be unset because simulated
	   HW WP pin is in active state. */
	flashrom_wp_set_mode(wp_cfg, FLASHROM_WP_MODE_HARDWARE);
	assert_int_equal(0, flashrom_wp_write_cfg(&flash, wp_cfg));
	assert_int_equal(0, flashrom_wp_read_cfg(wp_cfg, &flash));
	assert_int_equal(FLASHROM_WP_MODE_HARDWARE, flashrom_wp_get_mode(wp_cfg));

	/* Check that write-protection mode can't be unset. */
	flashrom_wp_set_mode(wp_cfg, FLASHROM_WP_MODE_DISABLED);
	assert_int_equal(FLASHROM_WP_ERR_VERIFY_FAILED, flashrom_wp_write_cfg(&flash, wp_cfg));

	/* Final mode should be "hardware". */
	assert_int_equal(0, flashrom_wp_read_cfg(wp_cfg, &flash));
	assert_int_equal(FLASHROM_WP_MODE_HARDWARE, flashrom_wp_get_mode(wp_cfg));

	teardown(NULL);

	flashrom_wp_cfg_release(wp_cfg);
	free(param_dup);
}

/* WP state is decoded correctly from status registers */
void wp_init_from_status_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	/*
	 * CMP  (S14) = 1 (range complement)
	 * SRP1 (S8)  = 1
	 * SRP0 (S7)  = 1 (`SRP1 == 1 && SRP0 == 1` is permanent mode)
	 * SEC  (S6)  = 1 (base unit is a 4 KiB sector)
	 * TB   (S5)  = 1 (bottom up range)
	 * BP2  (S4)  = 0
	 * BP1  (S3)  = 1
	 * BP0  (S2)  = 1 (bp: BP2-0 == 0b011 == 3)
	 *
	 * Range coefficient is `2 ** (bp - 1)`, which is 4 in this case.
	 * Multiplaying that by base unit gives 16 KiB protected region at the
	 * bottom (start of the chip), which is then complemented.
	 */
	char *param_dup = strdup("bus=spi,emulate=W25Q128FV,spi_status=0x41ec");

	struct flashrom_flashctx flash = { 0 };
	struct flashchip mock_chip = chip_W25Q128_V;
	struct flashrom_wp_cfg *wp_cfg;

	size_t start;
	size_t len;

	setup_chip(&flash, NULL, &mock_chip, param_dup);

	assert_int_equal(0, flashrom_wp_cfg_new(&wp_cfg));

	/* Verify that WP mode reflects SPI status */
	assert_int_equal(0, flashrom_wp_read_cfg(wp_cfg, &flash));
	assert_int_equal(FLASHROM_WP_MODE_PERMANENT, flashrom_wp_get_mode(wp_cfg));
	flashrom_wp_get_range(&start, &len, wp_cfg);
	assert_int_equal(0x004000, start);
	assert_int_equal(0xffc000, len);

	teardown(NULL);

	flashrom_wp_cfg_release(wp_cfg);
	free(param_dup);
}

/* Enabled WP makes full chip erasure fail */
void full_chip_erase_with_wp_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	struct flashrom_flashctx flash = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_W25Q128_V;
	struct flashrom_wp_cfg *wp_cfg;

	char *param_dup = strdup("bus=spi,emulate=W25Q128FV,hwwp=yes");

	setup_chip(&flash, &layout, &mock_chip, param_dup);
	/* Layout regions are created by setup_chip(). */
	assert_int_equal(0, flashrom_layout_include_region(layout, "head"));
	assert_int_equal(0, flashrom_layout_include_region(layout, "tail"));

	assert_int_equal(0, flashrom_wp_cfg_new(&wp_cfg));

	/* Write protection takes effect only after changing SRP values, so at
	   this stage WP is not enabled and erase completes successfully. */
	assert_int_equal(0, flashrom_flash_erase(&flash));

	assert_int_equal(0, flashrom_wp_read_cfg(wp_cfg, &flash));

	/* Hardware-protect first 4 KiB. */
	flashrom_wp_set_range(wp_cfg, 0, 4 * KiB);
	flashrom_wp_set_mode(wp_cfg, FLASHROM_WP_MODE_HARDWARE);

	assert_int_equal(0, flashrom_wp_write_cfg(&flash, wp_cfg));

	/* Try erasing the chip again. Now that WP is active, the first 4 KiB is
	   protected and we're trying to erase the whole chip, erase should
	   fail. */
	assert_int_equal(1, flashrom_flash_erase(&flash));

	teardown(&layout);

	flashrom_wp_cfg_release(wp_cfg);
	free(param_dup);
}

/* Enabled WP does not block erasing unprotected parts of the chip */
void partial_chip_erase_with_wp_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	struct flashrom_flashctx flash = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_W25Q128_V;
	struct flashrom_wp_cfg *wp_cfg;

	char *param_dup = strdup("bus=spi,emulate=W25Q128FV,hwwp=yes");

	setup_chip(&flash, &layout, &mock_chip, param_dup);
	/* Layout region is created by setup_chip(). */
	assert_int_equal(0, flashrom_layout_include_region(layout, "tail"));

	assert_int_equal(0, flashrom_wp_cfg_new(&wp_cfg));

	assert_int_equal(0, flashrom_wp_read_cfg(wp_cfg, &flash));

	/* Hardware-protect head region. */
	flashrom_wp_set_mode(wp_cfg, FLASHROM_WP_MODE_HARDWARE);
	flashrom_wp_set_range(wp_cfg, 0, LAYOUT_TAIL_REGION_START);

	assert_int_equal(0, flashrom_wp_write_cfg(&flash, wp_cfg));

	/* First 4 KiB is the only protected part of the chip and the region
	   we included covers only unprotected part, so erase operation should
	   succeed. */
	assert_int_equal(0, flashrom_flash_erase(&flash));

	teardown(&layout);

	flashrom_wp_cfg_release(wp_cfg);
	free(param_dup);
}
