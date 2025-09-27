/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2021 Google LLC
 */

#include "lifecycle.h"

#if CONFIG_REALTEK_MST_I2C_SPI == 1
static int realtek_mst_ioctl(void *state, int fd, unsigned long request, va_list args)
{
	assert_int_equal(fd, MOCK_FD);
	assert_int_equal(request, I2C_SLAVE);
	/* Only access to I2C address 0x4a is expected */
	unsigned long addr = va_arg(args, unsigned long);
	assert_int_equal(addr, 0x4a);

	return 0;
}

static int realtek_mst_read(void *state, int fd, void *buf, size_t sz)
{
	assert_int_equal(fd, MOCK_FD);
	assert_int_equal(sz, 1);
	return sz;
}

static int realtek_mst_write(void *state, int fd, const void *buf, size_t sz)
{
	assert_int_equal(fd, MOCK_FD);
	const LargestIntegralType accepted_sizes[] = {1, 2};
	assert_in_set(sz, accepted_sizes, ARRAY_SIZE(accepted_sizes));
	return sz;
}

void realtek_mst_basic_lifecycle_test_success(void **state)
{
	struct io_mock_fallback_open_state realtek_mst_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/i2c-254", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock realtek_mst_io = {
		.iom_ioctl = realtek_mst_ioctl,
		.iom_read = realtek_mst_read,
		.iom_write = realtek_mst_write,
		.fallback_open_state = &realtek_mst_fallback_open_state,
	};

	run_basic_lifecycle(state, &realtek_mst_io, &programmer_realtek_mst_i2c_spi, "bus=254,enter_isp=0,allow_brick=yes");
}

void realtek_mst_no_allow_brick_test_success(void **state)
{
	struct io_mock_fallback_open_state realtek_mst_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/i2c-254", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock realtek_mst_io = {
		.fallback_open_state = &realtek_mst_fallback_open_state,
	};

	run_init_error_path(state, &realtek_mst_io, &programmer_realtek_mst_i2c_spi,
				"bus=254,enter_isp=0", SPI_GENERIC_ERROR);
}
#else
	SKIP_TEST(realtek_mst_basic_lifecycle_test_success)
	SKIP_TEST(realtek_mst_no_allow_brick_test_success)
#endif /* CONFIG_REALTEK_MST_I2C_SPI */
