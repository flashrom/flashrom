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
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "flash.h"
#include "programmer.h"
#include "spi.h"

/* Note that CS# is active low, so val=0 means the chip is active. */
static void bitbang_spi_set_cs(const struct bitbang_spi_master * const master, int val, void *spi_data)
{
	master->set_cs(val, spi_data);
}

static void bitbang_spi_set_sck(const struct bitbang_spi_master * const master, int val, void *spi_data)
{
	master->set_sck(val, spi_data);
}

static void bitbang_spi_request_bus(const struct bitbang_spi_master * const master, void *spi_data)
{
	if (master->request_bus)
		master->request_bus(spi_data);
}

static void bitbang_spi_release_bus(const struct bitbang_spi_master * const master, void *spi_data)
{
	if (master->release_bus)
		master->release_bus(spi_data);
}

static void bitbang_spi_set_sck_set_mosi(const struct bitbang_spi_master * const master, int sck, int mosi,
					void *spi_data)
{
	if (master->set_sck_set_mosi) {
		master->set_sck_set_mosi(sck, mosi, spi_data);
		return;
	}

	master->set_sck(sck, spi_data);
	master->set_mosi(mosi, spi_data);
}

static int bitbang_spi_set_sck_get_miso(const struct bitbang_spi_master * const master, int sck,
					void *spi_data)
{
	if (master->set_sck_get_miso)
		return master->set_sck_get_miso(sck, spi_data);

	master->set_sck(sck, spi_data);
	return master->get_miso(spi_data);
}

static uint8_t bitbang_spi_read_byte(const struct bitbang_spi_master *master, void *spi_data)
{
	uint8_t ret = 0;
	int i;

	for (i = 7; i >= 0; i--) {
		if (i == 0)
			bitbang_spi_set_sck_set_mosi(master, 0, 0, spi_data);
		else
			bitbang_spi_set_sck(master, 0, spi_data);
		default_delay(master->half_period);
		ret <<= 1;
		ret |= bitbang_spi_set_sck_get_miso(master, 1, spi_data);
		default_delay(master->half_period);
	}
	return ret;
}

static void bitbang_spi_write_byte(const struct bitbang_spi_master *master, uint8_t val, void *spi_data)
{
	int i;

	for (i = 7; i >= 0; i--) {
		bitbang_spi_set_sck_set_mosi(master, 0, (val >> i) & 1, spi_data);
		default_delay(master->half_period);
		bitbang_spi_set_sck(master, 1, spi_data);
		default_delay(master->half_period);
	}
}

struct bitbang_spi_master_data {
	const struct bitbang_spi_master *master;
	void *spi_data;
};

static int bitbang_spi_send_command(const struct flashctx *flash,
				    unsigned int writecnt, unsigned int readcnt,
				    const unsigned char *writearr,
				    unsigned char *readarr)
{
	unsigned int i;
	const struct bitbang_spi_master_data *data = flash->mst->spi.data;
	const struct bitbang_spi_master *master = data->master;

	/* FIXME: Run bitbang_spi_request_bus here or in programmer init?
	 * Requesting and releasing the SPI bus is handled in here to allow the
	 * programmer to use its own SPI engine for native accesses.
	 */
	bitbang_spi_request_bus(master, data->spi_data);
	bitbang_spi_set_cs(master, 0, data->spi_data);
	for (i = 0; i < writecnt; i++)
		bitbang_spi_write_byte(master, writearr[i], data->spi_data);
	for (i = 0; i < readcnt; i++)
		readarr[i] = bitbang_spi_read_byte(master, data->spi_data);

	bitbang_spi_set_sck(master, 0, data->spi_data);
	default_delay(master->half_period);
	bitbang_spi_set_cs(master, 1, data->spi_data);
	default_delay(master->half_period);
	/* FIXME: Run bitbang_spi_release_bus here or in programmer init? */
	bitbang_spi_release_bus(master, data->spi_data);

	return 0;
}

static int bitbang_spi_shutdown(void *data)
{
	/* FIXME: Run bitbang_spi_release_bus here or per command? */
	free(data);
	return 0;
}

static const struct spi_master spi_master_bitbang = {
	.features	= SPI_MASTER_4BA,
	.max_data_read	= MAX_DATA_READ_UNLIMITED,
	.max_data_write	= MAX_DATA_WRITE_UNLIMITED,
	.command	= bitbang_spi_send_command,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.shutdown	= bitbang_spi_shutdown,
};

int register_spi_bitbang_master(const struct bitbang_spi_master *master, void *spi_data)
{
	struct spi_master mst = spi_master_bitbang;
	/* If someone forgot to initialize a bitbang function, we catch it here. */
	if (!master || !master->set_cs ||
	    !master->set_sck || !master->set_mosi || !master->get_miso ||
	    (master->request_bus && !master->release_bus) ||
	    (!master->request_bus && master->release_bus)) {
		msg_perr("Incomplete SPI bitbang master setting!\n"
			 "Please report a bug at flashrom@flashrom.org\n");
		return ERROR_FLASHROM_BUG;
	}

	struct bitbang_spi_master_data *data = calloc(1, sizeof(*data));
	if (!data)
		return ERROR_FLASHROM_FATAL;

	data->master = master;
	if (spi_data)
		data->spi_data = spi_data;

	register_spi_master(&mst, data);

	/* Only mess with the bus if we're sure nobody else uses it. */
	bitbang_spi_request_bus(master, spi_data);
	bitbang_spi_set_cs(master, 1, spi_data);
	bitbang_spi_set_sck_set_mosi(master, 0, 0, spi_data);
	/* FIXME: Release SPI bus here and request it again for each command or
	 * don't release it now and only release it on programmer shutdown?
	 */
	bitbang_spi_release_bus(master, spi_data);
	return 0;
}
