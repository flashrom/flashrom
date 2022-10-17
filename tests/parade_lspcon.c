/*
 * This file is part of the flashrom project.
 *
 * Copyright 2022 Google LLC
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

#include "lifecycle.h"

#if CONFIG_PARADE_LSPCON == 1

/* Same macros as in parade_lspcon.c programmer. */
/* FIXME(aklm): should driver register maps be defined in `include/drivers/` for sharing with tests? */
#define REGISTER_ADDRESS			0x4a
#define SPISTATUS				0x9e
#define SPISTATUS_SECTOR_ERASE_FINISHED		0
#define SWSPICTL				0x93
#define SWSPICTL_ENABLE_READBACK		0x8
#define SWSPI_RDATA				0x91
/* Macros for test run. */
#define DATA_TO_READ				0
#define MAX_REG_BUF_LEN				2

struct parade_lspcon_io_state {
	unsigned long addr;			/* Address to read and write */
	uint8_t reg_buf[MAX_REG_BUF_LEN];	/* Last value written to the register address */
};

static int parade_lspcon_ioctl(void *state, int fd, unsigned long request, va_list args)
{
	struct parade_lspcon_io_state *io_state = state;
	if (request == I2C_SLAVE)
		/* Addr is the next (and the only) argument in the parameters list for this ioctl call. */
		io_state->addr = va_arg(args, unsigned long);

	return 0;
}

static int parade_lspcon_read(void *state, int fd, void *buf, size_t sz)
{
	struct parade_lspcon_io_state *io_state = state;

	/*
	 * Parade_lspcon programmer has operations over register address and
	 * page address. In the current emulation for basic lifecycle we need
	 * to emulate operations over register address. Page address can do
	 * nothing for now, and just return successful return value.
	 *
	 * For future, if this unit test is upgraded to run probing lifecycle,
	 * page address operations might need to be fully emulated.
	 */
	if (io_state->addr != REGISTER_ADDRESS)
		return sz;

	assert_int_equal(sz, 1);

	switch (io_state->reg_buf[0]) {
	case SPISTATUS:
		memset(buf, SPISTATUS_SECTOR_ERASE_FINISHED, sz);
		break;
	case SWSPICTL:
		memset(buf, SWSPICTL_ENABLE_READBACK, sz);
		break;
	case SWSPI_RDATA:
		memset(buf, DATA_TO_READ, sz);
		break;
	default:
		memset(buf, 0, sz);
		break;
	}

	return sz;
}

static int parade_lspcon_write(void *state, int fd, const void *buf, size_t sz)
{
	struct parade_lspcon_io_state *io_state = state;

	/*
	 * Only register address operations are needed to be emulated for basic lifecycle.
	 * See also comment in `parade_lspcon_read`.
	 */
	if (io_state->addr != REGISTER_ADDRESS)
		return sz;

	assert_true(sz <= MAX_REG_BUF_LEN);

	memcpy(io_state->reg_buf, buf, sz);

	return sz;
}

void parade_lspcon_basic_lifecycle_test_success(void **state)
{
	struct parade_lspcon_io_state parade_lspcon_io_state = { 0 };
	struct io_mock_fallback_open_state parade_lspcon_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/i2c-254", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock parade_lspcon_io = {
		.state = &parade_lspcon_io_state,
		.iom_ioctl = parade_lspcon_ioctl,
		.iom_read = parade_lspcon_read,
		.iom_write = parade_lspcon_write,
		.fallback_open_state = &parade_lspcon_fallback_open_state,
	};

	run_basic_lifecycle(state, &parade_lspcon_io, &programmer_parade_lspcon, "bus=254,allow_brick=yes");
}

void parade_lspcon_no_allow_brick_test_success(void **state)
{
	struct io_mock_fallback_open_state parade_lspcon_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/i2c-254", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock parade_lspcon_io = {
		.fallback_open_state = &parade_lspcon_fallback_open_state,
	};

	run_init_error_path(state, &parade_lspcon_io, &programmer_parade_lspcon,
				"bus=254", SPI_GENERIC_ERROR);
}

#else
	SKIP_TEST(parade_lspcon_basic_lifecycle_test_success)
	SKIP_TEST(parade_lspcon_no_allow_brick_test_success)
#endif /* CONFIG_PARADE_LSPCON */
