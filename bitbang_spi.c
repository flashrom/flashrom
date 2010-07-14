/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009, 2010 Carl-Daniel Hailfinger
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "flash.h"
#include "chipdrivers.h"
#include "spi.h"

/* Length of half a clock period in usecs */
int bitbang_spi_half_period = 0;

enum bitbang_spi_master bitbang_spi_master = BITBANG_SPI_INVALID;

const struct bitbang_spi_master_entry bitbang_spi_master_table[] = {
	{}, /* This entry corresponds to BITBANG_SPI_INVALID. */
};

const int bitbang_spi_master_count = ARRAY_SIZE(bitbang_spi_master_table);

void bitbang_spi_set_cs(int val)
{
	bitbang_spi_master_table[bitbang_spi_master].set_cs(val);
}

void bitbang_spi_set_sck(int val)
{
	bitbang_spi_master_table[bitbang_spi_master].set_sck(val);
}

void bitbang_spi_set_mosi(int val)
{
	bitbang_spi_master_table[bitbang_spi_master].set_mosi(val);
}

int bitbang_spi_get_miso(void)
{
	return bitbang_spi_master_table[bitbang_spi_master].get_miso();
}

int bitbang_spi_init(void)
{
	bitbang_spi_set_cs(1);
	bitbang_spi_set_sck(0);
	buses_supported = CHIP_BUSTYPE_SPI;
	return 0;
}

uint8_t bitbang_spi_readwrite_byte(uint8_t val)
{
	uint8_t ret = 0;
	int i;

	for (i = 7; i >= 0; i--) {
		bitbang_spi_set_mosi((val >> i) & 1);
		programmer_delay(bitbang_spi_half_period);
		bitbang_spi_set_sck(1);
		ret <<= 1;
		ret |= bitbang_spi_get_miso();
		programmer_delay(bitbang_spi_half_period);
		bitbang_spi_set_sck(0);
	}
	return ret;
}

int bitbang_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr)
{
	static unsigned char *bufout = NULL;
	static unsigned char *bufin = NULL;
	static int oldbufsize = 0;
	int bufsize;
	int i;

	/* Arbitrary size limitation here. We're only constrained by memory. */
	if (writecnt > 65536 || readcnt > 65536)
		return SPI_INVALID_LENGTH;

	bufsize = max(writecnt + readcnt, 260);
	/* Never shrink. realloc() calls are expensive. */
	if (bufsize > oldbufsize) {
		bufout = realloc(bufout, bufsize);
		if (!bufout) {
			msg_perr("Out of memory!\n");
			if (bufin)
				free(bufin);
			exit(1);
		}
		bufin = realloc(bufout, bufsize);
		if (!bufin) {
			msg_perr("Out of memory!\n");
			if (bufout)
				free(bufout);
			exit(1);
		}
		oldbufsize = bufsize;
	}
		
	memcpy(bufout, writearr, writecnt);
	/* Shift out 0x00 while reading data. */
	memset(bufout + writecnt, 0x00, readcnt);
	/* Make sure any non-read data is 0xff. */
	memset(bufin + writecnt, 0xff, readcnt);

	bitbang_spi_set_cs(0);
	for (i = 0; i < readcnt + writecnt; i++) {
		bufin[i] = bitbang_spi_readwrite_byte(bufout[i]);
	}
	programmer_delay(bitbang_spi_half_period);
	bitbang_spi_set_cs(1);
	programmer_delay(bitbang_spi_half_period);
	memcpy(readarr, bufin + writecnt, readcnt);

	return 0;
}

int bitbang_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	/* Maximum read length is unlimited, use 64k bytes. */
	return spi_read_chunked(flash, buf, start, len, 64 * 1024);
}

int bitbang_spi_write_256(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	spi_disable_blockprotect();
	return spi_write_chunked(flash, buf, start, len, 256);
}
