/*
 * This file is part of the flashrom project.
 *
 * Copyright 2025 Google LLC
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

/*
 * This test does probing based on linux_spi programmer, and therefore the test
 * runs only when linux_spi programmer is enabled.
 * The test is emulation only, and not making any calls to real hardware.
*/
#if CONFIG_LINUX_SPI == 1

/*
 * These numbers represent how many times the given opcode should be sent to the chip.
 *
 * Probing has a caching mechanism, which tries to cache the ID response from the chip,
 * so that not to request the same info again. So, ideally, this number should be 1:
 * first time opcode is sent, response received and ID is cached. Then any more times
 * the ID is requested by the same opcode, ID can be taken from cache without hw access.
 *
 * SFDP opcode is sent twice to get the properties from SFDP headers.
 *
 * NOTE: at the moment of writing, caching is skipped in some cases and some opcodes are
 * sent multiple times.
 *
 * TODO: Fix the caching and update assertions for PROBE_COUNT_FOR_ALL_SPI_OPCODES
 */
#define PROBE_COUNT_JEDEC_RDID_3	1
#define PROBE_COUNT_JEDEC_RDID_4	1
#define PROBE_COUNT_AT25F_RDID		1
#define PROBE_COUNT_JEDEC_RES_1		1
#define PROBE_COUNT_JEDEC_RES_2		1
#define PROBE_COUNT_JEDEC_REMS		1
#define PROBE_COUNT_JEDEC_ST_M95_RDID	1
#define PROBE_COUNT_JEDEC_RDID_6	1
#define PROBE_COUNT_JEDEC_SFDP		2

/*
 * This number represents how many times in total a probing opcode is sent to a SPI chip,
 * during the probing operation which goes through the whole flashchips array.
 * opcodes: {9f+3bytes, 9f+4, 15+2, ab+1, ab+2, 90+2, 83+3, 9f+6, 5a+3, 5a+3}
 *
 * Probing has a caching mechanism which tries to cache the ID response from the chip,
 * so that not to request the same info again.
 *
 * NOTE: at the time of writing, in some cases caching is skipped and the number is higher than it could be.
 *
 * TODO: Fix the caching and update assertions for PROBE_COUNT_FOR_ALL_SPI_OPCODES
 */
#define PROBE_COUNT_ALL_SPI_OPCODES ( \
					PROBE_COUNT_JEDEC_RDID_3 + \
					PROBE_COUNT_JEDEC_RDID_4 + \
					PROBE_COUNT_AT25F_RDID + \
					PROBE_COUNT_JEDEC_RES_1 + \
					PROBE_COUNT_JEDEC_RES_2 + \
					PROBE_COUNT_JEDEC_REMS + \
					PROBE_COUNT_JEDEC_ST_M95_RDID + \
					PROBE_COUNT_JEDEC_RDID_6 + \
					PROBE_COUNT_JEDEC_SFDP \
					)

struct probe_io_state {
	unsigned int opcode;

	unsigned int readcount;
	unsigned int writecount;
	unsigned char vendor_id;
	unsigned char model_id_left_byte;
	unsigned char model_id_right_byte;
	unsigned int opcode_counter;
	unsigned int counter;
};

static bool is_probe_opcode(unsigned int opcode)
{
	if (opcode == AT25F_RDID
		|| opcode == JEDEC_RDID
		|| opcode == JEDEC_REMS
		|| opcode == JEDEC_RES
		|| opcode == JEDEC_SFDP
		|| opcode == ST_M95_RDID)
			return true;

	return false;
}

static int probe_handler(void *state, int fd, unsigned long request, va_list args)
{
	if (request == SPI_IOC_MESSAGE(2)) { /* ioctl code for read request */
		struct spi_ioc_transfer* msg = va_arg(args, struct spi_ioc_transfer*);

		/* First message has write array and write count */
		unsigned int writecount = msg[0].len;
		unsigned char *writearr = (unsigned char *)(uintptr_t)msg[0].tx_buf;
		/* Second message has read array and read count */
		unsigned int readcount = msg[1].len;
		unsigned char *readarr = (unsigned char *)(uintptr_t)msg[1].rx_buf;

		struct probe_io_state *probe_state = state;
		unsigned int opcode = writearr[0];

		if (is_probe_opcode(opcode)) {
			probe_state->counter++;
			/* Response for any other opcode (except the one we are emulating) is 0xff */
			memset(readarr, 0xff, readcount);
		}

		/* Detect probing for opcode and number of bytes that we need to emulate. */
		if (opcode == probe_state->opcode
			&& readcount == probe_state->readcount
			&& writecount == probe_state->writecount) {
			/* Emulate successful probing, which means we need to populate read array
			 * with chip vendor ID and model ID. */
			readarr[0] = probe_state->vendor_id;
			readarr[1] = probe_state->model_id_left_byte;
			if (readcount > 2)
				readarr[2] = probe_state->model_id_right_byte;

			probe_state->opcode_counter++;
		}
	}

	return 0;
}

static char *linux_spi_fgets(void *state, char *buf, int len, FILE *fp)
{
	/* Emulate reading max buffer size from sysfs. */
	const char *max_buf_size = "1048576";

	return memcpy(buf, max_buf_size, min(len, strlen(max_buf_size) + 1));
}

static void print_probing_results(const struct probe_io_state probe_io_state)
{
	printf("Probe opcode 0x%2x, %d bytes, sent to chip %d times\n",
		probe_io_state.opcode, probe_io_state.readcount, probe_io_state.opcode_counter);
	printf("Total count of all probe opcode sent to chip was %d times\n",
		probe_io_state.counter);
}

void probe_jedec_rdid3_fixed_chipname(void **state)
{
	struct probe_io_state probe_io_state = {
		.opcode			= JEDEC_RDID,
		.readcount		= JEDEC_RDID_INSIZE,
		.writecount		= JEDEC_RDID_OUTSIZE,
		.vendor_id		= 0xEF, /* WINBOND_NEX_ID */
		.model_id_left_byte	= 0x40, /* WINBOND_NEX_W25Q128_V left byte */
		.model_id_right_byte	= 0x18, /* WINBOND_NEX_W25Q128_V right byte */
	};
	struct io_mock_fallback_open_state linux_spi_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/null", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock linux_spi_io = {
		.state		= &probe_io_state,
		.iom_fgets	= linux_spi_fgets,
		.iom_ioctl	= probe_handler,
		.fallback_open_state = &linux_spi_fallback_open_state,
	};

	const char *expected_matched_names[1] = {"W25Q128.V"};
	run_probe_v2_lifecycle(state, &linux_spi_io, &programmer_linux_spi, "dev=/dev/null",
				"W25Q128.V", expected_matched_names, 1);

	print_probing_results(probe_io_state);

	/* Since fixed chip name was given, probing should happen only once, for that name. */
	assert_int_equal(1, probe_io_state.opcode_counter);
	assert_int_equal(1, probe_io_state.counter);
}

void probe_jedec_rdid3_try_all_flashchips(void **state)
{
	struct probe_io_state probe_io_state = {
		.opcode			= JEDEC_RDID,
		.readcount		= JEDEC_RDID_INSIZE,
		.writecount		= JEDEC_RDID_OUTSIZE,
		.vendor_id		= 0xEF, /* WINBOND_NEX_ID */
		.model_id_left_byte	= 0x40, /* WINBOND_NEX_W25Q128_V left byte */
		.model_id_right_byte	= 0x18, /* WINBOND_NEX_W25Q128_V right byte */
	};
	struct io_mock_fallback_open_state linux_spi_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/null", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock linux_spi_io = {
		.state		= &probe_io_state,
		.iom_fgets	= linux_spi_fgets,
		.iom_ioctl	= probe_handler,
		.fallback_open_state = &linux_spi_fallback_open_state,
	};

	const char *expected_matched_names[1] = {"W25Q128.V"};
	run_probe_v2_lifecycle(state, &linux_spi_io, &programmer_linux_spi, "dev=/dev/null",
				NULL, /* no fixed name, go through all flashchips */
				expected_matched_names, 1);

	print_probing_results(probe_io_state);

	// FIXME: change to assert_int_equal after caching is fully implemented.
	// At the moment the number of opcode calls are greater than, because not all
	// probing functions are using cache.
	assert_in_range(probe_io_state.opcode_counter, PROBE_COUNT_JEDEC_RDID_3, flashchips_size);
	assert_in_range(probe_io_state.counter, PROBE_COUNT_ALL_SPI_OPCODES, flashchips_size);
}

void probe_jedec_rdid3_no_matches_found(void **state)
{
	struct probe_io_state probe_io_state = {
		.opcode			= JEDEC_RDID,
		.readcount		= JEDEC_RDID_INSIZE,
		.writecount		= JEDEC_RDID_OUTSIZE,
		/* The values below represent non-existent model, we expect no matches found. */
		.vendor_id		= 0x00,
		.model_id_left_byte	= 0xFF,
		.model_id_right_byte	= 0xFF,
	};
	struct io_mock_fallback_open_state linux_spi_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/null", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock linux_spi_io = {
		.state		= &probe_io_state,
		.iom_fgets	= linux_spi_fgets,
		.iom_ioctl	= probe_handler,
		.fallback_open_state = &linux_spi_fallback_open_state,
	};

	run_probe_v2_lifecycle(state, &linux_spi_io, &programmer_linux_spi, "dev=/dev/null",
				NULL, /* no fixed name, go through all flashchips */
				NULL /* no matched names expected */, 0);

	print_probing_results(probe_io_state);

	/* No matches, but we needed to go through everything to discover that. */
	// FIXME: change to assert_int_equal after caching is fully implemented.
	// At the moment the number of opcode calls are greater than, because not all
	// probing functions are using cache.
	assert_in_range(probe_io_state.opcode_counter, PROBE_COUNT_JEDEC_RDID_3, flashchips_size);
	assert_in_range(probe_io_state.counter, PROBE_COUNT_ALL_SPI_OPCODES, flashchips_size);
}

void probe_jedec_res1_fixed_chipname(void **state)
{
	struct probe_io_state probe_io_state = {
		.opcode			= JEDEC_RES,
		.readcount		= JEDEC_RES_INSIZE,
		.writecount		= JEDEC_RES_OUTSIZE,
		/* readarr[0] is used as chip model ID for probe_spi_res1,
		 * unlike other probing functions which use readarr[0] as vendor ID. */
		.vendor_id		= 0x05, /* ST_M25P05_RES */
		.model_id_left_byte	= 0xff, /* Not used for M25P05 */
		.model_id_right_byte	= 0xff, /* Not used for M25P05 */
	};
	struct io_mock_fallback_open_state linux_spi_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/null", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock linux_spi_io = {
		.state		= &probe_io_state,
		.iom_fgets	= linux_spi_fgets,
		.iom_ioctl	= probe_handler,
		.fallback_open_state = &linux_spi_fallback_open_state,
	};

	const char *expected_matched_names[1] = {"M25P05"};
	run_probe_v2_lifecycle(state, &linux_spi_io, &programmer_linux_spi, "dev=/dev/null",
				"M25P05", expected_matched_names, 1);

	print_probing_results(probe_io_state);

	/* Since fixed chip name was given, probing should happen only once, for that name. */
	assert_int_equal(1, probe_io_state.opcode_counter);
	/* probe_spi_res1 tries, in order, RDID, REMS, and if none of these works, RES. */
	assert_int_equal(3, probe_io_state.counter);
}

void probe_jedec_res1_try_all_flashchips(void **state)
{
	struct probe_io_state probe_io_state = {
		.opcode			= JEDEC_RES,
		.readcount		= JEDEC_RES_INSIZE,
		.writecount		= JEDEC_RES_OUTSIZE,
		/* readarr[0] is used as chip model ID for probe_spi_res1,
		 * unlike other probing functions which use readarr[0] as vendor ID. */
		.vendor_id		= 0x05, /* ST_M25P05_RES */
		.model_id_left_byte	= 0xff, /* Not used for M25P05 */
		.model_id_right_byte	= 0xff, /* Not used for M25P05 */
	};
	struct io_mock_fallback_open_state linux_spi_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/null", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock linux_spi_io = {
		.state		= &probe_io_state,
		.iom_fgets	= linux_spi_fgets,
		.iom_ioctl	= probe_handler,
		.fallback_open_state = &linux_spi_fallback_open_state,
	};

	const char *expected_matched_names[1] = {"M25P05"};
	run_probe_v2_lifecycle(state, &linux_spi_io, &programmer_linux_spi, "dev=/dev/null",
				NULL, /* no fixed name, go through all flashchips */
				expected_matched_names, 1);

	print_probing_results(probe_io_state);

	// FIXME: change to assert_int_equal after caching is fully implemented.
	// At the moment the number of opcode calls are greater than, because not all
	// probing functions are using cache.
	assert_in_range(probe_io_state.opcode_counter, PROBE_COUNT_JEDEC_RES_1, flashchips_size);
	assert_in_range(probe_io_state.counter, PROBE_COUNT_ALL_SPI_OPCODES, flashchips_size);
}

void probe_jedec_res1_no_matches_found(void **state)
{
	struct probe_io_state probe_io_state = {
		.opcode			= JEDEC_RES,
		.readcount		= JEDEC_RES_INSIZE,
		.writecount		= JEDEC_RES_OUTSIZE,
		/* The values below represent non-existent chip. */
		.vendor_id		= 0x00,
		.model_id_left_byte	= 0xff,
		.model_id_right_byte	= 0xff,
	};
	struct io_mock_fallback_open_state linux_spi_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/null", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock linux_spi_io = {
		.state		= &probe_io_state,
		.iom_fgets	= linux_spi_fgets,
		.iom_ioctl	= probe_handler,
		.fallback_open_state = &linux_spi_fallback_open_state,
	};

	run_probe_v2_lifecycle(state, &linux_spi_io, &programmer_linux_spi, "dev=/dev/null",
				NULL, /* no fixed name, go through all flashchips */
				NULL, 0);

	print_probing_results(probe_io_state);

	/* No matches, but we need to go through everything to find that out. */
	// FIXME: change to assert_int_equal after caching is fully implemented.
	// At the moment the number of opcode calls are greater than, because not all
	// probing functions are using cache.
	assert_in_range(probe_io_state.opcode_counter, PROBE_COUNT_JEDEC_RES_1, flashchips_size);
	assert_in_range(probe_io_state.counter, PROBE_COUNT_ALL_SPI_OPCODES, flashchips_size);
}

#else
	SKIP_TEST(probe_jedec_rdid3_fixed_chipname)
	SKIP_TEST(probe_jedec_rdid3_try_all_flashchips)
	SKIP_TEST(probe_jedec_rdid3_no_matches_found)
	SKIP_TEST(probe_jedec_res1_fixed_chipname)
	SKIP_TEST(probe_jedec_res1_try_all_flashchips)
	SKIP_TEST(probe_jedec_res1_no_matches_found)
#endif /* CONFIG_LINUX_SPI */
