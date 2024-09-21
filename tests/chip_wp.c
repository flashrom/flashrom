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
#include <stdlib.h>
#include <string.h>

#include "chipdrivers.h"
#include "flash.h"
#include "libflashrom.h"
#include "programmer.h"
#include "tests.h"

static int unittest_print_cb(enum flashrom_log_level level, const char *fmt, va_list ap)
{
	if (level > FLASHROM_MSG_INFO) return 0;
	return vfprintf(stderr, fmt, ap);
}

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

	flashrom_set_log_callback((flashrom_log_callback *)&unittest_print_cb);

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
	.page_size	= 1024,
	.tested		= TEST_OK_PREW,
	.read		= SPI_CHIP_READ,
	.write		= SPI_CHIP_WRITE256,
	.unlock         = SPI_DISABLE_BLOCKPROTECT,
	.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR_EXT2 | FEATURE_WRSR2 | FEATURE_WRSR3,
	.block_erasers  =
	{
		{
			.eraseblocks = { {4 * 1024, 4096} },
			.block_erase = SPI_BLOCK_ERASE_20,
		}, {
			.eraseblocks = { {32 * 1024, 512} },
			.block_erase = SPI_BLOCK_ERASE_52,
		}, {
			.eraseblocks = { {64 * 1024, 256} },
			.block_erase = SPI_BLOCK_ERASE_D8,
		}, {
			.eraseblocks = { {16 * 1024 * 1024, 1} },
			.block_erase = SPI_BLOCK_ERASE_60,
		}, {
			.eraseblocks = { {16 * 1024 * 1024, 1} },
			.block_erase = SPI_BLOCK_ERASE_C7,
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

	const char *param_dup = "bus=spi,emulate=W25Q128FV,hwwp=no";

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
}

/* Enabling hardware WP with a valid range succeeds */
void set_wp_range_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	const char *param_dup = "bus=spi,emulate=W25Q128FV,hwwp=no";

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
}

/* Enable hardware WP and verify that it can not be unset */
void switch_wp_mode_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	const char *param_dup = "bus=spi,emulate=W25Q128FV,hwwp=yes";

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
	const char *param_dup = "bus=spi,emulate=W25Q128FV,spi_status=0x41ec";

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
}

/* Enabled WP makes full chip erasure fail */
void full_chip_erase_with_wp_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	struct flashrom_flashctx flash = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_W25Q128_V;
	struct flashrom_wp_cfg *wp_cfg;

	const char *param_dup = "bus=spi,emulate=W25Q128FV,hwwp=yes";

	setup_chip(&flash, &layout, &mock_chip, param_dup);
	/* Layout regions are created by setup_chip(). */
	assert_int_equal(0, flashrom_layout_include_region(layout, "head"));
	assert_int_equal(0, flashrom_layout_include_region(layout, "tail"));

	assert_int_equal(0, flashrom_wp_cfg_new(&wp_cfg));

	/* Write protection takes effect only after changing SRP values, so at
	   this stage WP is not enabled and erase completes successfully. */
	assert_int_equal(0, flashrom_flash_erase(&flash));

	/* Write non-erased value to entire chip so that erase operations cannot
	 * be optimized away. */
	unsigned long size = flashrom_flash_getsize(&flash);
	uint8_t *const contents = malloc(size);
	assert_non_null(contents);
	memset(contents, UNERASED_VALUE(&flash), size);
	assert_int_equal(0, flashrom_image_write(&flash, contents, size, NULL));
	free(contents);

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
}

/* Enabled WP does not block erasing unprotected parts of the chip */
void partial_chip_erase_with_wp_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	struct flashrom_flashctx flash = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_W25Q128_V;
	struct flashrom_wp_cfg *wp_cfg;

	const char *param_dup = "bus=spi,emulate=W25Q128FV,hwwp=yes";

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
}

/* Chip register values & masks are calculated correctly by WP */
void wp_get_register_values_and_masks(void **state)
{
	(void) state; /* unused */

	/*
	 * Test with range: start = 0x004000, lengh = 0xffc000
	 *
	 * WP should use these bit values:
	 * WPS  (S17) = 0 (write protect scheme)
	 * CMP  (S14) = 1 (range complement)
	 * SRP1 (S8)  = 0
	 * SRP0 (S7)  = 1 (`SRP1 == 1 && SRP0 == 1` is permanent mode)
	 * SEC  (S6)  = 1 (base unit is a 4 KiB sector)
	 * TB   (S5)  = 1 (bottom up range)
	 * BP2  (S4)  = 0
	 * BP1  (S3)  = 1
	 * BP0  (S2)  = 1 (bp: BP2-0 == 0b011 == 3)
	 *
	 * Register values:
	 * SR1 = 0b11101100 = 0xec
	 * SR2 = 0b01000000 = 0x40
	 * SR3 = 0b00000000 = 0x00
	 *
	 * Masks for WP bits in registers:
	 * SR1: 0b11111100 = 0xfc
	 * SR2: 0b01000000 = 0x41
	 * SR3: 0b00000100 = 0x04
	 *
	 * All WP bits are RW so write masks should be the same as the bit masks.
	 *
	 */
	struct flashrom_flashctx flash = { 0 };
	struct flashchip mock_chip = chip_W25Q128_V;
	struct flashrom_wp_cfg *wp_cfg;

	uint8_t reg_values[MAX_REGISTERS];
	uint8_t bit_masks[MAX_REGISTERS];
	uint8_t write_masks[MAX_REGISTERS];

	setup_chip(&flash, NULL, &mock_chip, "bus=spi,emulate=W25Q128FV");

	assert_int_equal(0, flashrom_wp_cfg_new(&wp_cfg));
	flashrom_wp_set_mode(wp_cfg, FLASHROM_WP_MODE_HARDWARE);
	flashrom_wp_set_range(wp_cfg, 0x004000, 0xffc000);

	assert_int_equal(0, wp_cfg_to_reg_values(reg_values, bit_masks, write_masks, &flash, wp_cfg));

	assert_int_equal(0xec, reg_values[STATUS1]);
	assert_int_equal(0x40, reg_values[STATUS2]);
	assert_int_equal(0x00, reg_values[STATUS3]);

	assert_int_equal(0xfc, bit_masks[STATUS1]);
	assert_int_equal(0x41, bit_masks[STATUS2]);
	assert_int_equal(0x04, bit_masks[STATUS3]);

	assert_int_equal(0xfc, write_masks[STATUS1]);
	assert_int_equal(0x41, write_masks[STATUS2]);
	assert_int_equal(0x04, write_masks[STATUS3]);

	teardown(NULL);

	flashrom_wp_cfg_release(wp_cfg);
}
