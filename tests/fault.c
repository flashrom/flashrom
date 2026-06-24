/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2026 Abdelkader Boudih <flashrom@seuros.com>
 */

#include "lifecycle.h"
#include "libflashrom.h"

#if CONFIG_FAULT == 1 && CONFIG_DUMMY == 1

/* fault wraps dummy, so we reuse the dummy io mock. */
static const struct io_mock_fallback_open_state fault_fallback_open_state = {
	.noc = 0,
	.paths = { NULL },
};

static const struct io_mock fault_io = {
	.fallback_open_state = (struct io_mock_fallback_open_state *)&fault_fallback_open_state,
};

void fault_basic_lifecycle_test_success(void **state)
{
	/* No fault type specified: fault programmer passes through to dummy unchanged. */
	run_basic_lifecycle(state, &fault_io, &programmer_fault,
			   "backend=dummy,bus=spi,emulate=W25Q128FV,seed=42");
}

void fault_init_missing_backend_test_success(void **state)
{
	run_init_error_path(state, &fault_io, &programmer_fault,
			   "seed=42", 1);
}

void fault_init_self_wrap_test_success(void **state)
{
	run_init_error_path(state, &fault_io, &programmer_fault,
			   "backend=fault,seed=42", 1);
}

void fault_init_unknown_backend_test_success(void **state)
{
	run_init_error_path(state, &fault_io, &programmer_fault,
			   "backend=nonexistent,seed=42", 1);
}

void fault_probe_lifecycle_test_success(void **state)
{
	const char *expected_matched_names[1] = {"W25Q128.V"};
	run_probe_v2_lifecycle(state, &fault_io, &programmer_fault,
			       "backend=dummy,bus=spi,emulate=W25Q128FV,seed=42",
			       "W25Q128.V", expected_matched_names, 1);
}

void fault_probe_and_read_test_success(void **state)
{
	(void) state;

	struct flashrom_programmer *flashprog = NULL;
	struct flashrom_flashctx *flashctx = NULL;
	const char **all_matched_names = NULL;
	const unsigned long chip_size = 16384 * KiB;

	assert_int_equal(0, flashrom_create_context(&flashctx));

	io_mock_register(&fault_io);

	assert_int_equal(0, flashrom_programmer_init(&flashprog, programmer_fault.name,
			 "backend=dummy,bus=spi,emulate=W25Q128FV,seed=42"));
	assert_int_equal(1, flashrom_flash_probe_v2(flashctx, &all_matched_names, flashprog, NULL));
	flashrom_data_free(all_matched_names);

	unsigned char *buf = calloc(chip_size, 1);
	assert_non_null(buf);
	assert_int_equal(0, flashrom_image_read(flashctx, buf, chip_size));
	free(buf);

	assert_int_equal(0, flashrom_programmer_shutdown(flashprog));
	io_mock_register(NULL);
	flashrom_flash_release(flashctx);
}

void fault_all_fault_params_test_success(void **state)
{
	/* Verify init succeeds with all fault parameters specified. */
	run_basic_lifecycle(state, &fault_io, &programmer_fault,
			   "backend=dummy,bus=spi,emulate=W25Q128FV,"
			   "seed=123,flip_prob=0.01,"
			   "short_read_prob=0.1,min_read_ratio=0.5,"
			   "write_fail_prob=0.05,write_lie_prob=0.05,"
			   "partial_write_prob=0.1");
}

void fault_deterministic_seed_test_success(void **state)
{
	/*
	 * Two inits with the same seed should produce the same
	 * programmer state.  We can't directly compare RNG output
	 * here, but we verify both init+shutdown cycles succeed
	 * with the same parameters.
	 */
	run_basic_lifecycle(state, &fault_io, &programmer_fault,
			   "backend=dummy,bus=spi,emulate=W25Q128FV,seed=999");
	run_basic_lifecycle(state, &fault_io, &programmer_fault,
			   "backend=dummy,bus=spi,emulate=W25Q128FV,seed=999");
}

#else
	SKIP_TEST(fault_basic_lifecycle_test_success)
	SKIP_TEST(fault_init_missing_backend_test_success)
	SKIP_TEST(fault_init_self_wrap_test_success)
	SKIP_TEST(fault_init_unknown_backend_test_success)
	SKIP_TEST(fault_probe_lifecycle_test_success)
	SKIP_TEST(fault_probe_and_read_test_success)
	SKIP_TEST(fault_all_fault_params_test_success)
	SKIP_TEST(fault_deterministic_seed_test_success)
#endif /* CONFIG_FAULT && CONFIG_DUMMY */
