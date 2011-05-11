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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

/* Length of half a clock period in usecs. */
static int bitbang_spi_half_period;

static const struct bitbang_spi_master *bitbang_spi_master = NULL;

/* Note that CS# is active low, so val=0 means the chip is active. */
static void bitbang_spi_set_cs(int val)
{
	bitbang_spi_master->set_cs(val);
}

static void bitbang_spi_set_sck(int val)
{
	bitbang_spi_master->set_sck(val);
}

static void bitbang_spi_set_mosi(int val)
{
	bitbang_spi_master->set_mosi(val);
}

static int bitbang_spi_get_miso(void)
{
	return bitbang_spi_master->get_miso();
}

static void bitbang_spi_request_bus(void)
{
	if (bitbang_spi_master->request_bus)
		bitbang_spi_master->request_bus();
}

static void bitbang_spi_release_bus(void)
{
	if (bitbang_spi_master->release_bus)
		bitbang_spi_master->release_bus();
}

int bitbang_spi_init(const struct bitbang_spi_master *master, int halfperiod)
{
	/* BITBANG_SPI_INVALID is 0, so if someone forgot to initialize ->type,
	 * we catch it here. Same goes for missing initialization of bitbanging
	 * functions.
	 */
	if (!master || master->type == BITBANG_SPI_INVALID || !master->set_cs ||
	    !master->set_sck || !master->set_mosi || !master->get_miso) {
		msg_perr("Incomplete SPI bitbang master setting!\n"
			 "Please report a bug at flashrom@flashrom.org\n");
		return 1;
	}
	if (bitbang_spi_master) {
		msg_perr("SPI bitbang master already initialized!\n"
			 "Please report a bug at flashrom@flashrom.org\n");
		return 1;
	}

	bitbang_spi_master = master;
	bitbang_spi_half_period = halfperiod;

	/* FIXME: Run bitbang_spi_request_bus here or in programmer init? */
	bitbang_spi_set_cs(1);
	bitbang_spi_set_sck(0);
	bitbang_spi_set_mosi(0);
	return 0;
}

int bitbang_spi_shutdown(const struct bitbang_spi_master *master)
{
	if (!bitbang_spi_master) {
		msg_perr("Shutting down an uninitialized SPI bitbang master!\n"
			 "Please report a bug at flashrom@flashrom.org\n");
		return 1;
	}
	if (master != bitbang_spi_master) {
		msg_perr("Shutting down a mismatched SPI bitbang master!\n"
			 "Please report a bug at flashrom@flashrom.org\n");
		return 1;
	}

	/* FIXME: Run bitbang_spi_release_bus here or per command? */
	bitbang_spi_master = NULL;
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

	/* FIXME: Run bitbang_spi_request_bus here or in programmer init?
	 * Requesting and releasing the SPI bus is handled in here to allow the
	 * programmer to use its own SPI engine for native accesses.
	 */
	bitbang_spi_request_bus();
	bitbang_spi_set_cs(0);
	for (i = 0; i < writecnt; i++)
		bitbang_spi_readwrite_byte(writearr[i]);
	for (i = 0; i < readcnt; i++)
		readarr[i] = bitbang_spi_readwrite_byte(0);

	programmer_delay(bitbang_spi_half_period);
	bitbang_spi_set_cs(1);
	programmer_delay(bitbang_spi_half_period);
	/* FIXME: Run bitbang_spi_release_bus here or in programmer init? */
	bitbang_spi_release_bus();

	return 0;
}
