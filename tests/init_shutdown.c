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
#include <string.h>

#include "io_mock.h"
#include "programmer.h"

static void run_lifecycle(void **state, const struct programmer_entry *prog, const char *param)
{
	(void) state; /* unused */

	printf("Testing programmer_init for programmer=%s ...\n", prog->name);
	assert_int_equal(0, programmer_init(prog, strdup(param)));
	printf("... programmer_init for programmer=%s successful\n", prog->name);

	printf("Testing programmer_shutdown for programmer=%s ...\n", prog->name);
	assert_int_equal(0, programmer_shutdown());
	printf("... programmer_shutdown for programmer=%s successful\n", prog->name);
}

void dummy_init_and_shutdown_test_success(void **state)
{
	run_lifecycle(state, &programmer_dummy, "bus=parallel+lpc+fwh+spi");
}

struct mec1308_io_state {
	unsigned char outb_val;
};

void mec1308_outb(void *state, unsigned char value, unsigned short port)
{
	struct mec1308_io_state *io_state = state;

	io_state->outb_val = value;
}

unsigned char mec1308_inb(void *state, unsigned short port)
{
	struct mec1308_io_state *io_state = state;

	return ((port == 0x2e /* MEC1308_SIO_PORT1 */
			|| port == 0x4e /* MEC1308_SIO_PORT2 */)
		? 0x20 /* MEC1308_DEVICE_ID_REG */
		: ((io_state->outb_val == 0x84 /* MEC1308_MBX_DATA_START */)
			? 0xaa /* MEC1308_CMD_PASSTHRU_SUCCESS */
			: 0));
}

void mec1308_init_and_shutdown_test_success(void **state)
{
	struct mec1308_io_state mec1308_io_state = { 0 };
	const struct io_mock mec1308_io = {
		.state		= &mec1308_io_state,
		.outb		= mec1308_outb,
		.inb		= mec1308_inb,
	};

	io_mock_register(&mec1308_io);

	will_return_always(__wrap_sio_read, 0x4d); /* MEC1308_DEVICE_ID_VAL */
	run_lifecycle(state, &programmer_mec1308, "");

	io_mock_register(NULL);
}

struct ene_lpc_io_state {
	unsigned char outb_val;
	int pause_cmd;
};

void ene_lpc_outb_kb932(void *state, unsigned char value, unsigned short port)
{
	struct ene_lpc_io_state *io_state = state;

	io_state->outb_val = value;
	if (value == 0x59 /* ENE_KB932.ec_pause_cmd */)
		io_state->pause_cmd = 1;
}

unsigned char ene_lpc_inb_kb932(void *state, unsigned short port)
{
	struct ene_lpc_io_state *io_state = state;
	unsigned char ene_hwver_offset = 0; /* REG_EC_HWVER & 0xff */
	unsigned char ene_ediid_offset = 0x24; /* REG_EC_EDIID & 0xff */
	unsigned char ec_status_buf_offset = 0x54; /* ENE_KB932.ec_status_buf & 0xff */

	if (port == 0xfd63 /* ENE_KB932.port_io_base + port_ene_data  */) {
		if (io_state->outb_val == ene_hwver_offset)
			return 0xa2; /* ENE_KB932.hwver */
		if (io_state->outb_val == ene_ediid_offset)
			return 0x02; /* ENE_KB932.ediid */
		if (io_state->outb_val == ec_status_buf_offset) {
			if (io_state->pause_cmd == 1) {
				io_state->pause_cmd = 0;
				return 0x33; /* ENE_KB932.ec_is_pausing mask */
			} else {
				return 0x00; /* ENE_KB932.ec_is_running mask */
			}
		}
	}

	return 0;
}

void ene_lpc_init_and_shutdown_test_success(void **state)
{
	/*
	 * Current implementation tests for chip ENE_KB932.
	 * Another chip which is not tested here is ENE_KB94X.
	 */
	struct ene_lpc_io_state ene_lpc_io_state = { 0, 0 };
	const struct io_mock ene_lpc_io = {
		.state		= &ene_lpc_io_state,
		.outb		= ene_lpc_outb_kb932,
		.inb		= ene_lpc_inb_kb932,
	};

	io_mock_register(&ene_lpc_io);

	run_lifecycle(state, &programmer_ene_lpc, "");

	io_mock_register(NULL);
}

void linux_spi_init_and_shutdown_test_success(void **state)
{
	/*
	 * Current implementation tests a particular path of the init procedure.
	 * There are two ways for it to succeed: reading the buffer size from sysfs
	 * and the fallback to getpagesize(). This test does the latter (fallback to
	 * getpagesize).
	 */
	run_lifecycle(state, &programmer_linux_spi, "dev=/dev/null");
}
