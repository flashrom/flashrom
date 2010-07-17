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

/* Length of half a clock period in usecs. */
static int bitbang_spi_half_period;

static enum bitbang_spi_master bitbang_spi_master = BITBANG_SPI_INVALID;

static const struct bitbang_spi_master_entry bitbang_spi_master_table[] = {
	{}, /* This entry corresponds to BITBANG_SPI_INVALID. */
};

const int bitbang_spi_master_count = ARRAY_SIZE(bitbang_spi_master_table);

/* Note that CS# is active low, so val=0 means the chip is active. */
static void bitbang_spi_set_cs(int val)
{
	bitbang_spi_master_table[bitbang_spi_master].set_cs(val);
}

static void bitbang_spi_set_sck(int val)
{
	bitbang_spi_master_table[bitbang_spi_master].set_sck(val);
}

static void bitbang_spi_set_mosi(int val)
{
	bitbang_spi_master_table[bitbang_spi_master].set_mosi(val);
}

static int bitbang_spi_get_miso(void)
{
	return bitbang_spi_master_table[bitbang_spi_master].get_miso();
}

int bitbang_spi_init(enum bitbang_spi_master master, int halfperiod)
{
	bitbang_spi_master = master;
	bitbang_spi_half_period = halfperiod;

	if (bitbang_spi_master == BITBANG_SPI_INVALID) {
		msg_perr("Invalid bitbang SPI master. \n"
			 "Please report a bug at flashrom@flashrom.org\n");
		return 1;
	}
	bitbang_spi_set_cs(1);
	bitbang_spi_set_sck(0);
	bitbang_spi_set_mosi(0);
	return 0;
}

static uint8_t bitbang_spi_readwrite_byte(uint8_t val)
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
	int i;

	bitbang_spi_set_cs(0);
	for (i = 0; i < writecnt; i++)
		bitbang_spi_readwrite_byte(writearr[i]);
	for (i = 0; i < readcnt; i++)
		readarr[i] = bitbang_spi_readwrite_byte(0);

	programmer_delay(bitbang_spi_half_period);
	bitbang_spi_set_cs(1);
	programmer_delay(bitbang_spi_half_period);

	return 0;
}

int bitbang_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	/* Maximum read length is unlimited, use 64k bytes. */
	return spi_read_chunked(flash, buf, start, len, 64 * 1024);
}

int bitbang_spi_write_256(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	return spi_write_chunked(flash, buf, start, len, 256);
}
