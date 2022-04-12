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
 * Dummyflasher controls chip state and emulates read/write/unlock/erase.
 * `g_chip_state` is NOT used for this type of tests.
 * Example of test: erase_chip_with_dummyflasher_test_success.
 */

#include <include/test.h>
#include <stdio.h>
#include <string.h>

#include "chipdrivers.h"
#include "flash.h"
#include "io_mock.h"
#include "libflashrom.h"
#include "programmer.h"

#define MOCK_CHIP_SIZE (8*MiB)
#define MOCK_CHIP_CONTENT 0xff

static struct {
	unsigned int unlock_calls; /* how many times unlock function was called */
	uint8_t buf[MOCK_CHIP_SIZE]; /* buffer of total size of chip, to emulate a chip */
} g_chip_state = {
	.unlock_calls = 0,
	.buf = { 0 },
};

static int read_chip(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	printf("Read chip called with start=0x%x, len=0x%x\n", start, len);
	if (!g_chip_state.unlock_calls) {
		printf("Error while reading chip: unlock was not called.\n");
		return 1;
	}

	assert_in_range(start + len, 0, MOCK_CHIP_SIZE);

	memcpy(buf, &g_chip_state.buf[start], len);
	return 0;
}

static int write_chip(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	printf("Write chip called with start=0x%x, len=0x%x\n", start, len);
	if (!g_chip_state.unlock_calls) {
		printf("Error while writing chip: unlock was not called.\n");
		return 1;
	}

	assert_in_range(start + len, 0, MOCK_CHIP_SIZE);

	memcpy(&g_chip_state.buf[start], buf, len);
	return 0;
}

static int unlock_chip(struct flashctx *flash)
{
	printf("Unlock chip called\n");
	g_chip_state.unlock_calls++;

	if (g_chip_state.unlock_calls > 1) {
		printf("Error: Unlock called twice\n");
		return -1;
	}

	return 0;
}

static int block_erase_chip(struct flashctx *flash, unsigned int blockaddr, unsigned int blocklen)
{
	printf("Block erase called with blockaddr=0x%x, blocklen=0x%x\n", blockaddr, blocklen);
	if (!g_chip_state.unlock_calls) {
		printf("Error while erasing chip: unlock was not called.\n");
		return 1;
	}

	assert_in_range(blockaddr + blocklen, 0, MOCK_CHIP_SIZE);

	memset(&g_chip_state.buf[blockaddr], 0xff, blocklen);
	return 0;
}

static void setup_chip(struct flashrom_flashctx *flashctx, struct flashrom_layout **layout,
		struct flashchip *chip, const char *programmer_param, const struct io_mock *io)
{
	io_mock_register(io);

	flashctx->chip = chip;

	g_chip_state.unlock_calls = 0;
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
	.read		= read_chip,
	.write		= write_chip,
	.unlock		= unlock_chip,
	.block_erasers	=
	{{
		 /* All blocks within total size of the chip. */
		.eraseblocks = { {2 * MiB, 4} },
		.block_erase = block_erase_chip,
	 }},
};

/* Setup the struct for W25Q128.V, all values come from flashchips.c */
static const struct flashchip chip_W25Q128_V = {
	.vendor		= "aklm&dummyflasher",
	.total_size	= 16 * 1024,
	.tested		= TEST_OK_PREW,
	.read		= spi_chip_read,
	.write		= spi_chip_write_256,
	.unlock         = spi_disable_blockprotect,
	.page_size	= 256,
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
	char *param_dup = strdup("bus=spi,emulate=W25Q128FV");

	setup_chip(&flashctx, &layout, &mock_chip, param_dup, &chip_io);

	printf("Erase chip operation started.\n");
	assert_int_equal(0, flashrom_flash_erase(&flashctx));
	printf("Erase chip operation done.\n");

	teardown(&layout);

	free(param_dup);
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

	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_8MiB;
	const char *param = ""; /* Default values for all params. */

	setup_chip(&flashctx, &layout, &mock_chip, param, &chip_io);

	const char *const filename = "read_chip.test";
	unsigned long size = mock_chip.total_size * 1024;
	unsigned char *buf = calloc(size, sizeof(unsigned char));

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
	char *param_dup = strdup("bus=spi,emulate=W25Q128FV");

	setup_chip(&flashctx, &layout, &mock_chip, param_dup, &chip_io);

	const char *const filename = "read_chip.test";
	unsigned long size = mock_chip.total_size * 1024;
	unsigned char *buf = calloc(size, sizeof(unsigned char));

	printf("Read chip operation started.\n");
	assert_int_equal(0, flashrom_image_read(&flashctx, buf, size));
	assert_int_equal(0, write_buf_to_file(buf, size, filename));
	printf("Read chip operation done.\n");

	teardown(&layout);

	free(param_dup);
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
	char *param_dup = strdup("bus=spi,emulate=W25Q128FV");

	setup_chip(&flashctx, &layout, &mock_chip, param_dup, &chip_io);

	/* See comment in write_chip_test_success */
	const char *const filename = "-";
	unsigned long size = mock_chip.total_size * 1024;
	uint8_t *const newcontents = malloc(size);

	printf("Write chip operation started.\n");
	assert_int_equal(0, read_buf_from_file(newcontents, size, filename));
	assert_int_equal(0, flashrom_image_write(&flashctx, newcontents, size, NULL));
	printf("Write chip operation done.\n");

	teardown(&layout);

	free(param_dup);
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
		.fread = verify_chip_fread,
		.fallback_open_state = &data,
	};

	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_8MiB;
	const char *param = ""; /* Default values for all params. */

	setup_chip(&flashctx, &layout, &mock_chip, param, &verify_chip_io);

	/* See comment in write_chip_test_success */
	const char *const filename = "-";
	unsigned long size = mock_chip.total_size * 1024;
	uint8_t *const newcontents = malloc(size);

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
		.fread = verify_chip_fread,
		.fallback_open_state = &data,
	};

	struct flashrom_flashctx flashctx = { 0 };
	struct flashrom_layout *layout;
	struct flashchip mock_chip = chip_W25Q128_V;
	/*
	 * Dummyflasher is capable to emulate W25Q128.V, so we ask it to do this.
	 * Nothing to mock, dummy is taking care of this already.
	 */
	char *param_dup = strdup("bus=spi,emulate=W25Q128FV");

	setup_chip(&flashctx, &layout, &mock_chip, param_dup, &verify_chip_io);

	/* See comment in write_chip_test_success */
	const char *const filename = "-";
	unsigned long size = mock_chip.total_size * 1024;
	uint8_t *const newcontents = malloc(size);

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

	free(param_dup);
	free(newcontents);
}
