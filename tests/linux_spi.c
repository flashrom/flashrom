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

#include "lifecycle.h"

#if CONFIG_LINUX_SPI == 1
static int linux_spi_ioctl(void *state, int fd, unsigned long request, va_list args) {

	if (request == SPI_IOC_MESSAGE(2)) { /* ioctl code for read request */
		struct spi_ioc_transfer* msg = va_arg(args, struct spi_ioc_transfer*);

		/* First message has write array and write count */
		unsigned int writecnt = msg[0].len;
		unsigned char *writearr = (unsigned char *)(uintptr_t)msg[0].tx_buf;
		/* Second message has read array and read count */
		unsigned int readcnt = msg[1].len;

		/* Detect probing */
		if (writecnt == 1 && writearr[0] == JEDEC_RDID && readcnt == 3) {
			/* We need to populate read array. */
			unsigned char *readarr = (unsigned char *)(uintptr_t)msg[1].rx_buf;
			readarr[0] = 0xEF; /* WINBOND_NEX_ID */
			readarr[1] = 0x40; /* WINBOND_NEX_W25Q128_V left byte */
			readarr[2] = 0x18; /* WINBOND_NEX_W25Q128_V right byte */
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

void linux_spi_probe_lifecycle_test_success(void **state)
{
	/*
	 * Current implementation tests a particular path of the init procedure.
	 * Specifically, it is reading the buffer size from sysfs.
	 */
	struct io_mock_fallback_open_state linux_spi_fallback_open_state = {
		.noc = 0,
		.paths = { "/dev/null", NULL },
		.flags = { O_RDWR },
	};
	const struct io_mock linux_spi_io = {
		.iom_fgets	= linux_spi_fgets,
		.iom_ioctl	= linux_spi_ioctl,
		.fallback_open_state = &linux_spi_fallback_open_state,
	};

	run_probe_lifecycle(state, &linux_spi_io, &programmer_linux_spi, "dev=/dev/null", "W25Q128.V");
}
#else
	SKIP_TEST(linux_spi_probe_lifecycle_test_success)
#endif /* CONFIG_LINUX_SPI */
