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
#include "programmer.h"
#include "spi.h"

/* Note that CS# is active low, so val=0 means the chip is active. */
static void bitbang_spi_set_cs(const struct bitbang_spi_master * const master, int val)
{
	master->set_cs(val);
}

static void bitbang_spi_set_sck(const struct bitbang_spi_master * const master, int val)
{
	master->set_sck(val);
}

static void bitbang_spi_set_mosi(const struct bitbang_spi_master * const master, int val)
{
	master->set_mosi(val);
}

static int bitbang_spi_get_miso(const struct bitbang_spi_master * const master)
{
	return master->get_miso();
}

static void bitbang_spi_request_bus(const struct bitbang_spi_master * const master)
{
	if (master->request_bus)
		master->request_bus();
}

static void bitbang_spi_release_bus(const struct bitbang_spi_master * const master)
{
	if (master->release_bus)
		master->release_bus();
}

static int bitbang_spi_send_command(struct flashctx *flash,
				    unsigned int writecnt, unsigned int readcnt,
				    const unsigned char *writearr,
				    unsigned char *readarr);

static const struct spi_master spi_master_bitbang = {
	.type		= SPI_CONTROLLER_BITBANG,
	.max_data_read	= MAX_DATA_READ_UNLIMITED,
	.max_data_write	= MAX_DATA_WRITE_UNLIMITED,
	.command	= bitbang_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

#if 0 // until it is needed
static int bitbang_spi_shutdown(const struct bitbang_spi_master *master)
{
	/* FIXME: Run bitbang_spi_release_bus here or per command? */
	return 0;
}
#endif

int register_spi_bitbang_master(const struct bitbang_spi_master *master)
{
	struct spi_master mst = spi_master_bitbang;
	/* BITBANG_SPI_INVALID is 0, so if someone forgot to initialize ->type,
	 * we catch it here. Same goes for missing initialization of bitbanging
	 * functions.
	 */
	if (!master || master->type == BITBANG_SPI_INVALID || !master->set_cs ||
	    !master->set_sck || !master->set_mosi || !master->get_miso ||
	    (master->request_bus && !master->release_bus) ||
	    (!master->request_bus && master->release_bus)) {
		msg_perr("Incomplete SPI bitbang master setting!\n"
			 "Please report a bug at flashrom@flashrom.org\n");
		return ERROR_FLASHROM_BUG;
	}

	mst.data = master;
	register_spi_master(&mst);

	/* Only mess with the bus if we're sure nobody else uses it. */
	bitbang_spi_request_bus(master);
	bitbang_spi_set_cs(master, 1);
	bitbang_spi_set_sck(master, 0);
	bitbang_spi_set_mosi(master, 0);
	/* FIXME: Release SPI bus here and request it again for each command or
	 * don't release it now and only release it on programmer shutdown?
	 */
	bitbang_spi_release_bus(master);
	return 0;
}

static uint8_t bitbang_spi_rw_byte(const struct bitbang_spi_master *master,
				   uint8_t val)
{
	uint8_t ret = 0;
	int i;

	for (i = 7; i >= 0; i--) {
		bitbang_spi_set_mosi(master, (val >> i) & 1);
		programmer_delay(master->half_period);
		bitbang_spi_set_sck(master, 1);
		ret <<= 1;
		ret |= bitbang_spi_get_miso(master);
		programmer_delay(master->half_period);
		bitbang_spi_set_sck(master, 0);
	}
	return ret;
}

static int bitbang_spi_send_command(struct flashctx *flash,
				    unsigned int writecnt, unsigned int readcnt,
				    const unsigned char *writearr,
				    unsigned char *readarr)
{
	int i;
	const struct bitbang_spi_master *master = flash->mst->spi.data;

	/* FIXME: Run bitbang_spi_request_bus here or in programmer init?
	 * Requesting and releasing the SPI bus is handled in here to allow the
	 * programmer to use its own SPI engine for native accesses.
	 */
	bitbang_spi_request_bus(master);
	bitbang_spi_set_cs(master, 0);
	for (i = 0; i < writecnt; i++)
		bitbang_spi_rw_byte(master, writearr[i]);
	for (i = 0; i < readcnt; i++)
		readarr[i] = bitbang_spi_rw_byte(master, 0);

	programmer_delay(master->half_period);
	bitbang_spi_set_cs(master, 1);
	programmer_delay(master->half_period);
	/* FIXME: Run bitbang_spi_release_bus here or in programmer init? */
	bitbang_spi_release_bus(master);

	return 0;
}
