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
 *
 * This file contains tests for operations on flash chip.
 *
 * Two flash chip test variants are used:
 *
 * 1) Mock chip state backed by `g_chip_state`.
 * Example of test: erase_chip_test_success.
 *
 * 2) Mock chip operations backed by `dummyflasher` emulation.
 * Dummyflasher controls chip state and emulates read/write/erase.
 * `g_chip_state` is NOT used for this type of tests.
 * Example of test: erase_chip_with_dummyflasher_test_success.
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

#define MOCK_CHIP_SIZE (8*MiB)
#define MOCK_CHIP_CONTENT 0xCC /* 0x00 is a zeroed heap and 0xFF is an erased chip. */

static struct {
	uint8_t buf[MOCK_CHIP_SIZE]; /* buffer of total size of chip, to emulate a chip */
} g_chip_state = {
	.buf = { 0 },
};

static int read_chip(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	printf("Read chip called with start=0x%x, len=0x%x\n", start, len);

	assert_in_range(start + len, 0, MOCK_CHIP_SIZE);

	memcpy(buf, &g_chip_state.buf[start], len);
	return 0;
}

static int write_chip(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	printf("Write chip called with start=0x%x, len=0x%x\n", start, len);

	assert_in_range(start + len, 0, MOCK_CHIP_SIZE);

	memcpy(&g_chip_state.buf[start], buf, len);
	return 0;
}

static int block_erase_chip(struct flashctx *flash, unsigned int blockaddr, unsigned int blocklen)
{
	printf("Block erase called with blockaddr=0x%x, blocklen=0x%x\n", blockaddr, blocklen);

	assert_in_range(blockaddr + blocklen, 0, MOCK_CHIP_SIZE);

	memset(&g_chip_state.buf[blockaddr], 0xff, blocklen);
	return 0;
}

static void setup_chip(struct flashrom_flashctx *flashctx, struct flashrom_layout **layout,
		struct flashchip *chip, const char *programmer_param, const struct io_mock *io)
{
	io_mock_register(io);

	flashctx->chip = chip;

	memset(g_chip_state.buf, MOCK_CHIP_CONTENT, sizeof(g_chip_state.buf));

	printf("Creating layout with one included region... ");
	assert_int_equal(0, flashrom_layout_new(layout));
	/* One region which covers total size of chip. */
	assert_int_equal(0, flashrom_layout_add_region(*layout, 0, chip->total_size * KiB - 1, "region"));
	assert_int_equal(0, flashrom_layout_include_region(*layout, "region"));

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
}

static void teardown(struct flashrom_layout **layout)
{
	printf("Dummyflasher shutdown... ");
	assert_int_equal(0, programmer_shutdown());
	printf("done\n");

	printf("Releasing layout... ");
	flashrom_layout_release(*layout);
	printf("done\n");

	io_mock_register(NULL);
}

static const struct flashchip chip_8MiB = {
	.vendor		= "aklm",
	.total_size	= MOCK_CHIP_SIZE / KiB,
	.tested		= TEST_OK_PREW,
	.read		= TEST_READ_INJECTOR,
	.write		= TEST_WRITE_INJECTOR,
	.block_erasers	=
	{{
		 /* All blocks within total size of the chip. */
		.eraseblocks = { {2 * MiB, 4} },
		.block_erase = TEST_ERASE_INJECTOR_1,
	 }},
};

/* Chip expected to be processed with dummyflasher, so using real op functions. */
static const struct flashchip chip_no_erase = {
	.vendor		= "aklm&dummyflasher",
	.total_size	= 16 * 1024,
	.tested		= TEST_OK_PREW,
	.read		= SPI_CHIP_READ,
	.write		= SPI_CHIP_WRITE256,
	.page_size	= 256,
	.feature_bits   = FEATURE_NO_ERASE | FEATURE_ERASED_ZERO,
	.block_erasers  =
	{
		{
			.eraseblocks = { {16 * 1024 * 1024, 1} },
			/* Special erase fn for chips without erase capability. */
			.block_erase = SPI_BLOCK_ERASE_EMULATION,
		}
	},
};

/* Setup the struct for W25Q128.V, all values come from flashchips.c */
static const struct flashchip chip_W25Q128_V = {
	.vendor		= "aklm&dummyflasher",
	.total_size	= 16 * 1024,
	.tested		= TEST_OK_PREW,
	.read		= SPI_CHIP_READ,
	.write		= SPI_CHIP_WRITE256,
	.page_size	= 256,
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
};

void erase_chip_test_success(void **state)
{
	(void) state; /* unused */

	static struct io_mock_fallback_open_state data = {
		.noc	= 0,
		.paths	= { NULL },
	};
	const struct io_mock chip_io = {
		.fallback_open_state = &data,
	};

	g_test_write_injector = write_chip;
	g_test_read_injector = read_chip;
	g_test_erase_injector[0] = block_erase_chip;
	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_8MiB;
	const char *param = ""; /* Default values for all params. */

	setup_chip(&flashctx, &layout, &mock_chip, param, &chip_io);

	printf("Erase chip operation started.\n");
	assert_int_equal(0, flashrom_flash_erase(&flashctx));
	printf("Erase chip operation done.\n");

	teardown(&layout);
}

void erase_chip_with_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	static struct io_mock_fallback_open_state data = {
		.noc	= 0,
		.paths	= { NULL },
	};
	const struct io_mock chip_io = {
		.fallback_open_state = &data,
	};

	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_W25Q128_V;
	/*
	 * Dummyflasher is capable to emulate W25Q128.V, so we ask it to do this.
	 * Nothing to mock, dummy is taking care of this already.
	 */
	const char *param_dup = "bus=spi,emulate=W25Q128FV";

	setup_chip(&flashctx, &layout, &mock_chip, param_dup, &chip_io);

	printf("Erase chip operation started.\n");
	assert_int_equal(0, flashrom_flash_erase(&flashctx));
	printf("Erase chip operation done.\n");

	teardown(&layout);
}

void read_chip_test_success(void **state)
{
	(void) state; /* unused */

	static struct io_mock_fallback_open_state data = {
		.noc	= 0,
		.paths	= { NULL },
	};
	const struct io_mock chip_io = {
		.fallback_open_state = &data,
	};

	g_test_write_injector = write_chip;
	g_test_read_injector = read_chip;
	g_test_erase_injector[0] = block_erase_chip;
	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_8MiB;
	const char *param = ""; /* Default values for all params. */

	setup_chip(&flashctx, &layout, &mock_chip, param, &chip_io);

	const char *const filename = "read_chip.test";
	unsigned long size = mock_chip.total_size * 1024;
	unsigned char *buf = calloc(size, sizeof(unsigned char));
	assert_non_null(buf);

	printf("Read chip operation started.\n");
	assert_int_equal(0, flashrom_image_read(&flashctx, buf, size));
	assert_int_equal(0, write_buf_to_file(buf, size, filename));
	printf("Read chip operation done.\n");

	teardown(&layout);

	free(buf);
}

void read_chip_with_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	static struct io_mock_fallback_open_state data = {
		.noc	= 0,
		.paths	= { NULL },
	};
	const struct io_mock chip_io = {
		.fallback_open_state = &data,
	};

	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_W25Q128_V;
	/*
	 * Dummyflasher is capable to emulate W25Q128.V, so we ask it to do this.
	 * Nothing to mock, dummy is taking care of this already.
	 */
	const char *param_dup = "bus=spi,emulate=W25Q128FV";

	setup_chip(&flashctx, &layout, &mock_chip, param_dup, &chip_io);

	const char *const filename = "read_chip.test";
	unsigned long size = mock_chip.total_size * 1024;
	unsigned char *buf = calloc(size, sizeof(unsigned char));
	assert_non_null(buf);

	printf("Read chip operation started.\n");
	assert_int_equal(0, flashrom_image_read(&flashctx, buf, size));
	assert_int_equal(0, write_buf_to_file(buf, size, filename));
	printf("Read chip operation done.\n");

	teardown(&layout);

	free(buf);
}

void write_chip_test_success(void **state)
{
	(void) state; /* unused */

	static struct io_mock_fallback_open_state data = {
		.noc	= 0,
		.paths	= { NULL },
	};
	const struct io_mock chip_io = {
		.fallback_open_state = &data,
	};

	g_test_write_injector = write_chip;
	g_test_read_injector = read_chip;
	g_test_erase_injector[0] = block_erase_chip;
	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_8MiB;
	const char *param = ""; /* Default values for all params. */

	setup_chip(&flashctx, &layout, &mock_chip, param, &chip_io);

	/*
	 * Providing filename "-" means content is taken from standard input.
	 * This doesn't change much because all file operations are mocked.
	 * However filename "-" makes a difference for
	 * flashrom.c#read_buf_from_file and allows to avoid mocking
	 * image_stat.st_size.
	 *
	 * Now this does mean test covers successful path only, but this test
	 * is designed to cover only successful write operation anyway.
	 *
	 * To cover error path of image_stat.st_size != flash size, filename
	 * needs to be provided and image_stat.st_size needs to be mocked.
	 */
	const char *const filename = "-";
	unsigned long size = mock_chip.total_size * 1024;
	uint8_t *const newcontents = malloc(size);
	assert_non_null(newcontents);

	printf("Write chip operation started.\n");
	assert_int_equal(0, read_buf_from_file(newcontents, size, filename));
	assert_int_equal(0, flashrom_image_write(&flashctx, newcontents, size, NULL));
	printf("Write chip operation done.\n");

	teardown(&layout);

	free(newcontents);
}

void write_chip_with_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	static struct io_mock_fallback_open_state data = {
		.noc	= 0,
		.paths	= { NULL },
	};
	const struct io_mock chip_io = {
		.fallback_open_state = &data,
	};

	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_W25Q128_V;
	/*
	 * Dummyflasher is capable to emulate W25Q128.V, so we ask it to do this.
	 * Nothing to mock, dummy is taking care of this already.
	 */
	const char *param_dup = "bus=spi,emulate=W25Q128FV";

	setup_chip(&flashctx, &layout, &mock_chip, param_dup, &chip_io);

	/* See comment in write_chip_test_success */
	const char *const filename = "-";
	unsigned long size = mock_chip.total_size * 1024;
	uint8_t *const newcontents = malloc(size);
	assert_non_null(newcontents);

	printf("Write chip operation started.\n");
	assert_int_equal(0, read_buf_from_file(newcontents, size, filename));
	assert_int_equal(0, flashrom_image_write(&flashctx, newcontents, size, NULL));
	printf("Write chip operation done.\n");

	teardown(&layout);

	free(newcontents);
}

void write_chip_feature_no_erase(void **state)
{
	(void) state; /* unused */

	static struct io_mock_fallback_open_state data = {
		.noc	= 0,
		.paths	= { NULL },
	};
	const struct io_mock chip_io = {
		.fallback_open_state = &data,
	};

	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_layout *layout;

	/*
	 * Tricking the dummyflasher by asking to emulate W25Q128FV but giving to it
	 * mock chip with FEATURE_NO_ERASE.
	 * As long as chip size is the same, this is fine.
	 */
	struct flashchip mock_chip = chip_no_erase;
	const char *param_dup = "bus=spi,emulate=W25Q128FV";

	setup_chip(&flashctx, &layout, &mock_chip, param_dup, &chip_io);

	/* See comment in write_chip_test_success */
	const char *const filename = "-";
	unsigned long size = mock_chip.total_size * 1024;
	uint8_t *const newcontents = malloc(size);
	assert_non_null(newcontents);

	printf("Write chip operation started.\n");
	assert_int_equal(0, read_buf_from_file(newcontents, size, filename));
	assert_int_equal(0, flashrom_image_write(&flashctx, newcontents, size, NULL));
	assert_int_equal(0, flashrom_image_verify(&flashctx, newcontents, size));
	printf("Write chip operation done.\n");

	teardown(&layout);

	free(newcontents);
}

void write_nonaligned_region_with_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	static struct io_mock_fallback_open_state data = {
		.noc	= 0,
		.paths	= { NULL },
	};
	const struct io_mock chip_io = {
		.fallback_open_state = &data,
	};

	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_W25Q128_V;
	const uint32_t mock_chip_size = mock_chip.total_size * KiB;
	/*
	 * Dummyflasher is capable to emulate W25Q128.V, so we ask it to do this.
	 * Nothing to mock, dummy is taking care of this already.
	 */
	const char *param_dup = "bus=spi,emulate=W25Q128FV";

	/* FIXME: MOCK_CHIP_CONTENT is buggy within setup_chip, it should also
	 * not be either 0x00 or 0xFF as those are specific values related to
	 * either a erased chip or zero'ed heap thus ambigous.
	 */
#define MOCK_CHIP_SUBREGION_CONTENTS 0xCC
	/**
	 * Step 0 - Prepare newcontents as contiguous sample data bytes as follows:
	 * {MOCK_CHIP_SUBREGION_CONTENTS, [..]}.
	 */
	uint8_t *newcontents = calloc(1, mock_chip_size);
	assert_non_null(newcontents);
	memset(newcontents, MOCK_CHIP_SUBREGION_CONTENTS, mock_chip_size);

	setup_chip(&flashctx, &layout, &mock_chip, param_dup, &chip_io);
	/* Expect to verify only the non-aligned write operation within the region. */
	flashrom_flag_set(&flashctx, FLASHROM_FLAG_VERIFY_AFTER_WRITE, true);
	flashrom_flag_set(&flashctx, FLASHROM_FLAG_VERIFY_WHOLE_CHIP, false);

	/**
	 * Prepare mock chip content and release setup_chip() layout for our
	 * custom ones.
	 */
	assert_int_equal(0, flashrom_image_write(&flashctx, newcontents, mock_chip_size, NULL));
	flashrom_layout_release(layout);

	/**
	 * Create region smaller than erase granularity of chip.
	 */
	printf("Creating custom region layout... ");
	assert_int_equal(0, flashrom_layout_new(&layout));
	printf("Adding and including region0... ");
	assert_int_equal(0, flashrom_layout_add_region(layout, 0, (1 * KiB), "region0"));
	assert_int_equal(0, flashrom_layout_include_region(layout, "region0"));
	flashrom_layout_set(&flashctx, layout);
	printf("Subregion layout configuration done.\n");

	/**
	 * Step 1 - Modify newcontents as non-contiguous sample data bytes as follows:
	 * 0xAA 0xAA {MOCK_CHIP_SUBREGION_CONTENTS}, [..]}.
	 */
	printf("Subregion chip write op..\n");
	memset(newcontents, 0xAA, 2);
	assert_int_equal(0, flashrom_image_write(&flashctx, newcontents, mock_chip_size, NULL));
	printf("Subregion chip write op done.\n");

	/**
	 * FIXME: A 'NULL' layout should indicate a default layout however this
	 * causes a crash for a unknown reason. For now prepare a new default
	 * layout of the entire chip. flashrom_layout_set(&flashctx, NULL); // use default layout.
	 */
	flashrom_layout_release(layout);
	assert_int_equal(0, flashrom_layout_new(&layout));
	assert_int_equal(0, flashrom_layout_add_region(layout, 0, mock_chip_size - 1, "entire"));
	assert_int_equal(0, flashrom_layout_include_region(layout, "entire"));
	flashrom_layout_set(&flashctx, layout);

	/**
	 * Expect a verification pass that the previous content within the region, however
	 * outside the region write length, is untouched.
	 */
	printf("Entire chip verify op..\n");
	assert_int_equal(0, flashrom_image_verify(&flashctx, newcontents, mock_chip_size));
	printf("Entire chip verify op done.\n");

	teardown(&layout);
	free(newcontents);
}

static size_t verify_chip_fread(void *state, void *buf, size_t size, size_t len, FILE *fp)
{
	/*
	 * Verify operation compares contents of the file vs contents on the chip.
	 * To emulate successful verification we emulate file contents to be the
	 * same as what is on the chip.
	 */
	memset(buf, MOCK_CHIP_CONTENT, len);
	return len;
}

void verify_chip_test_success(void **state)
{
	(void) state; /* unused */

	static struct io_mock_fallback_open_state data = {
		.noc	= 0,
		.paths	= { NULL },
	};
	const struct io_mock verify_chip_io = {
		.iom_fread = verify_chip_fread,
		.fallback_open_state = &data,
	};

	g_test_write_injector = write_chip;
	g_test_read_injector = read_chip;
	g_test_erase_injector[0] = block_erase_chip;
	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_8MiB;
	const char *param = ""; /* Default values for all params. */

	setup_chip(&flashctx, &layout, &mock_chip, param, &verify_chip_io);

	/* See comment in write_chip_test_success */
	const char *const filename = "-";
	unsigned long size = mock_chip.total_size * 1024;
	uint8_t *const newcontents = malloc(size);
	assert_non_null(newcontents);

	printf("Verify chip operation started.\n");
	assert_int_equal(0, read_buf_from_file(newcontents, size, filename));
	assert_int_equal(0, flashrom_image_verify(&flashctx, newcontents, size));
	printf("Verify chip operation done.\n");

	teardown(&layout);

	free(newcontents);
}

void verify_chip_with_dummyflasher_test_success(void **state)
{
	(void) state; /* unused */

	static struct io_mock_fallback_open_state data = {
		.noc	= 0,
		.paths	= { NULL },
	};
	const struct io_mock verify_chip_io = {
		.iom_fread = verify_chip_fread,
		.fallback_open_state = &data,
	};

	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_W25Q128_V;
	/*
	 * Dummyflasher is capable to emulate W25Q128.V, so we ask it to do this.
	 * Nothing to mock, dummy is taking care of this already.
	 */
	const char *param_dup = "bus=spi,emulate=W25Q128FV";

	setup_chip(&flashctx, &layout, &mock_chip, param_dup, &verify_chip_io);

	/* See comment in write_chip_test_success */
	const char *const filename = "-";
	unsigned long size = mock_chip.total_size * 1024;
	uint8_t *const newcontents = malloc(size);
	assert_non_null(newcontents);

	/*
	 * Dummyflasher controls chip state and fully emulates reads and writes,
	 * so to set up initial chip state we need to write on chip. Write
	 * operation takes content from file and writes on chip. File content is
	 * emulated in verify_chip_fread mock.
	 */

	printf("Write chip operation started.\n");
	assert_int_equal(0, read_buf_from_file(newcontents, size, filename));
	assert_int_equal(0, flashrom_image_write(&flashctx, newcontents, size, NULL));
	printf("Write chip operation done.\n");

	printf("Verify chip operation started.\n");
	assert_int_equal(0, flashrom_image_verify(&flashctx, newcontents, size));
	printf("Verify chip operation done.\n");

	teardown(&layout);

	free(newcontents);
}
