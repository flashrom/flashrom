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

#define NOT_NULL ((void *)0xf000baaa)

static void run_lifecycle(void **state, const struct programmer_entry *prog, const char *param)
{
	(void) state; /* unused */

	char *param_dup = strdup(param);

	printf("Testing programmer_init for programmer=%s ...\n", prog->name);
	assert_int_equal(0, programmer_init(prog, param_dup));
	printf("... programmer_init for programmer=%s successful\n", prog->name);

	printf("Testing programmer_shutdown for programmer=%s ...\n", prog->name);
	assert_int_equal(0, programmer_shutdown());
	printf("... programmer_shutdown for programmer=%s successful\n", prog->name);

	free(param_dup);
}

void dummy_init_and_shutdown_test_success(void **state)
{
#if CONFIG_DUMMY == 1
	run_lifecycle(state, &programmer_dummy, "bus=parallel+lpc+fwh+spi");
#else
	skip();
#endif
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
#if CONFIG_MEC1308 == 1
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
#else
	skip();
#endif
}

void nicrealtek_init_and_shutdown_test_success(void **state)
{
#if CONFIG_NICREALTEK == 1
	run_lifecycle(state, &programmer_nicrealtek, "");
#else
	skip();
#endif
}

int dediprog_libusb_control_transfer(void *state,
					libusb_device_handle *devh,
					uint8_t bmRequestType,
					uint8_t bRequest,
					uint16_t wValue,
					uint16_t wIndex,
					unsigned char *data,
					uint16_t wLength,
					unsigned int timeout)
{
	if (bRequest == 0x08 /* dediprog_cmds CMD_READ_PROG_INFO */) {
		/* Provide dediprog Device String into data buffer */
		memcpy(data, "SF600 V:7.2.2   ", wLength);
	}
	return wLength;
}

void dediprog_init_and_shutdown_test_success(void **state)
{
#if CONFIG_DEDIPROG == 1
	const struct io_mock dediprog_io = {
		.libusb_control_transfer = dediprog_libusb_control_transfer,
	};

	io_mock_register(&dediprog_io);

	run_lifecycle(state, &programmer_dediprog, "voltage=3.5V");

	io_mock_register(NULL);
#else
	skip();
#endif
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
#if CONFIG_ENE_LPC == 1
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
#else
	skip();
#endif
}

struct linux_mtd_io_state {
	char *fopen_path;
};

FILE *linux_mtd_fopen(void *state, const char *pathname, const char *mode)
{
	struct linux_mtd_io_state *io_state = state;

	io_state->fopen_path = strdup(pathname);

	return NOT_NULL;
}

size_t linux_mtd_fread(void *state, void *buf, size_t size, size_t len, FILE *fp)
{
	struct linux_mtd_fread_mock_entry {
		const char *path;
		const char *data;
	};
	const struct linux_mtd_fread_mock_entry fread_mock_map[] = {
		{ "/sys/class/mtd/mtd0//type",            "nor"    },
		{ "/sys/class/mtd/mtd0//name",            "Device" },
		{ "/sys/class/mtd/mtd0//flags",           ""       },
		{ "/sys/class/mtd/mtd0//size",            "1024"   },
		{ "/sys/class/mtd/mtd0//erasesize",       "512"    },
		{ "/sys/class/mtd/mtd0//numeraseregions", "0"      },
	};

	struct linux_mtd_io_state *io_state = state;
	unsigned int i;

	if (!io_state->fopen_path)
		return 0;

	for (i = 0; i < ARRAY_SIZE(fread_mock_map); i++) {
		const struct linux_mtd_fread_mock_entry *entry = &fread_mock_map[i];

		if (!strcmp(io_state->fopen_path, entry->path)) {
			size_t data_len = min(size * len, strlen(entry->data));
			memcpy(buf, entry->data, data_len);
			return data_len;
		}
	}

	return 0;
}

int linux_mtd_fclose(void *state, FILE *fp)
{
	struct linux_mtd_io_state *io_state = state;

	free(io_state->fopen_path);

	return 0;
}

void linux_mtd_init_and_shutdown_test_success(void **state)
{
#if CONFIG_LINUX_MTD == 1
	struct linux_mtd_io_state linux_mtd_io_state = { NULL };
	const struct io_mock linux_mtd_io = {
		.state	= &linux_mtd_io_state,
		.fopen	= linux_mtd_fopen,
		.fread	= linux_mtd_fread,
		.fclose = linux_mtd_fclose,
	};

	io_mock_register(&linux_mtd_io);

	run_lifecycle(state, &programmer_linux_mtd, "");

	io_mock_register(NULL);
#else
	skip();
#endif
}

char *linux_spi_fgets(void *state, char *buf, int len, FILE *fp)
{
	/* Emulate reading max buffer size from sysfs. */
	const char *max_buf_size = "1048576";

	return memcpy(buf, max_buf_size, min(len, strlen(max_buf_size) + 1));
}

void linux_spi_init_and_shutdown_test_success(void **state)
{
	/*
	 * Current implementation tests a particular path of the init procedure.
	 * Specifically, it is reading the buffer size from sysfs.
	 */
#if CONFIG_LINUX_SPI == 1
	const struct io_mock linux_spi_io = {
		.fgets	= linux_spi_fgets,
	};

	io_mock_register(&linux_spi_io);

	run_lifecycle(state, &programmer_linux_spi, "dev=/dev/null");

	io_mock_register(NULL);
#else
	skip();
#endif
}

#define REALTEK_MST_MOCK_FD 0x10ec

static int realtek_mst_open(void *state, const char *pathname, int flags)
{
	assert_string_equal(pathname, "/dev/i2c-254");
	assert_int_equal(flags & O_RDWR, O_RDWR);
	return REALTEK_MST_MOCK_FD;
}

static int realtek_mst_ioctl(void *state, int fd, unsigned long request, va_list args)
{
	assert_int_equal(fd, REALTEK_MST_MOCK_FD);
	assert_int_equal(request, I2C_SLAVE);
	/* Only access to I2C address 0x4a is expected */
	unsigned long addr = va_arg(args, unsigned long);
	assert_int_equal(addr, 0x4a);

	return 0;
}

static int realtek_mst_read(void *state, int fd, void *buf, size_t sz)
{
	assert_int_equal(fd, REALTEK_MST_MOCK_FD);
	assert_int_equal(sz, 1);
	return sz;
}

static int realtek_mst_write(void *state, int fd, const void *buf, size_t sz)
{
	assert_int_equal(fd, REALTEK_MST_MOCK_FD);
	const LargestIntegralType accepted_sizes[] = {1, 2};
	assert_in_set(sz, accepted_sizes, ARRAY_SIZE(accepted_sizes));
	return sz;
}

void realtek_mst_init_and_shutdown_test_success(void **state)
{
#if CONFIG_REALTEK_MST_I2C_SPI == 1
	const struct io_mock realtek_mst_io = {
		.open = realtek_mst_open,
		.ioctl = realtek_mst_ioctl,
		.read = realtek_mst_read,
		.write = realtek_mst_write,
	};
	io_mock_register(&realtek_mst_io);

	run_lifecycle(state, &programmer_realtek_mst_i2c_spi, "bus=254,enter-isp=0");

	io_mock_register(NULL);
#else
	skip();
#endif /* CONFIG_REALTEK_I2C_SPI */
}
