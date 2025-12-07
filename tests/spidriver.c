/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2025 Simon Arlott
 */

#include "lifecycle.h"

#if CONFIG_SPIDRIVER == 1 && !IS_WINDOWS
#define SPIDRIVER_TEST_DEBUG 0

struct spidriver_state {
	// most recent command
	unsigned char state;

	unsigned char input[256]; // for read() responses
	size_t in_len; // available data to read
	size_t in_pos; // remaining SPI read count
	unsigned char output[256]; // incoming SPI writes
	size_t out_pos; // SPI write position in buffer
	size_t out_len; // remaining SPI write count

	// chip select
	bool cs;
	size_t cs_count;

	// probe detected
	bool probe;
	size_t cs_probe;
};

static int spidriver_read(void *state, int fd, void *buf, size_t sz)
{
	struct spidriver_state *ts = state;

	assert_int_equal(fd, MOCK_FD);
	if (SPIDRIVER_TEST_DEBUG)
		printf("read: %zu\n", sz);

	sz = min(sz, ts->in_len);

	if (sz > 0) {
		memcpy(buf, ts->input, sz);
		memmove(ts->input, &ts->input[sz], sizeof(ts->input) - sz);
		ts->in_len = 0;
		return sz;
	} else {
		return -1;
	}
}

static int spidriver_write(void *state, int fd, const void *buf, size_t sz)
{
	struct spidriver_state *ts = state;

	assert_int_equal(fd, MOCK_FD);
	if (SPIDRIVER_TEST_DEBUG)
		printf("write: %zu\n", sz);

	for (size_t i = 0; i < sz; i++) {
		unsigned char c = ((const char *)buf)[i];
		bool first = ts->state == 0;

		if (first)
			ts->state = c;

		if (SPIDRIVER_TEST_DEBUG)
			printf("c=%02X first=%d state=%02X\n", c, first, ts->state);

		switch (ts->state) {
		case '?':
			assert_int_equal(ts->in_len, 0);
			snprintf((char *)ts->input, sizeof(ts->input),
				"[spidriver2 AAAAAAAA 000000002 5.190 000"
				" 21.9 1 1 1 ffff 0                     ]");
			ts->in_len = 80;
			ts->state = 0;
			break;

		case 0:
			break;

		case 'm':
		case 'a':
		case 'b':
			if (!first)
				ts->state = 0;
			break;

		case 's':
			if (SPIDRIVER_TEST_DEBUG)
				printf("select\n");
			ts->cs = true;
			ts->cs_count++;
			ts->state = 0;
			break;

		case 'u':
			if (SPIDRIVER_TEST_DEBUG)
				printf("unselect\n");
			ts->cs = false;
			ts->state = 0;
			break;

		case 'e':
			if (!first) {
				if (SPIDRIVER_TEST_DEBUG)
					printf("echo %02X\n", c);

				assert_int_equal(ts->in_len, 0);
				snprintf((char *)ts->input, sizeof(ts->input), "%c", c);
				ts->in_len = 1;
				ts->state = 0;
			}
			break;

		case 0x80 ... 0xbf:
			if (first) {
				ts->in_pos = c - 0x80 + 1;
				if (SPIDRIVER_TEST_DEBUG)
					printf("SPI read begin %zu\n", ts->in_pos);

				if (ts->probe) {
					if (SPIDRIVER_TEST_DEBUG)
						printf("probe response\n");

					assert_int_equal(ts->in_pos, 3);
					assert_true(ts->cs);
					// Must not have lowered CS after write
					assert_int_equal(ts->cs_count, ts->cs_probe);

					assert_int_equal(ts->in_len, 0);
					ts->input[0] = 0xEF; /* WINBOND_NEX_ID */
					ts->input[1] = 0x40; /* WINBOND_NEX_W25Q128_V left byte */
					ts->input[2] = 0x18; /* WINBOND_NEX_W25Q128_V right byte */
				} else {
					assert_int_equal(ts->in_len, 0);
					memset(ts->input, 0, ts->in_pos);
				}
				continue;
			} else if (ts->in_pos > 0) {
				assert_int_equal(c, 0);
				ts->in_pos--;
				ts->in_len++;
			}

			if (ts->in_pos == 0) {
				if (SPIDRIVER_TEST_DEBUG)
					printf("SPI read finished\n");
				ts->probe = false;
				ts->state = 0;
			}
			break;

		case 0xc0 ... 0xff:
			if (first) {
				assert_int_equal(ts->out_len, 0);
				ts->out_len = c - 0xc0 + 1;
				ts->out_pos = 0;
				if (SPIDRIVER_TEST_DEBUG)
					printf("SPI write begin %zu\n", ts->out_len);
				continue;
			} else if (ts->out_len > 0) {
				ts->output[ts->out_pos++] = c;
				ts->out_len--;
			}

			if (ts->out_len == 0) {
				if (SPIDRIVER_TEST_DEBUG)
					printf("SPI write finished\n");
				assert_true(ts->cs);
				if (ts->out_pos == 1 && ts->output[0] == JEDEC_RDID) {
					if (SPIDRIVER_TEST_DEBUG)
						printf("probe detected\n");
					ts->probe = true;
					ts->cs_probe = ts->cs_count;
				}
				ts->state = 0;
			}
			break;

		default:
			fail_msg("Unsupported command 0x%02X", ts->state);
			break;
		}
	}

	return sz;
}

void spidriver_probe_lifecycle_test_success(void **state)
{
	struct spidriver_state ts = {};
	struct io_mock_fallback_open_state spidriver_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/null", NULL },
		.flags = { O_RDWR | O_NOCTTY | O_NDELAY },
	};
	const struct io_mock spidriver_io = {
		.state = &ts,
		.iom_read	= spidriver_read,
		.iom_write	= spidriver_write,
		.fallback_open_state = &spidriver_fallback_open_state,
	};

	const char *expected_matched_names[1] = {"W25Q128.V"};
	run_probe_v2_lifecycle(state, &spidriver_io, &programmer_spidriver, "dev=/dev/null", "W25Q128.V",
				expected_matched_names, 1);
}
#else
	SKIP_TEST(spidriver_probe_lifecycle_test_success)
#endif /* CONFIG_SPIDRIVER */
