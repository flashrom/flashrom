/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2019 Konstantin Grudnev
 * Copyright (C) 2019 Nikolay Nikolaev
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Contains SPI chip driver functions related to ST95XXX series (SPI EEPROM)
 */
#include <string.h>
#include <stdlib.h>
#include "flashchips.h"
#include "chipdrivers.h"
#include "spi.h"

/* For ST95XXX chips which have RDID */
int probe_spi_st95(struct flashctx *flash)
{
	/*
	 * ST_M95_RDID_OUTSIZE depends on size of the flash and
	 * not all ST_M95XXX have RDID.
	 */
	static const unsigned char cmd[ST_M95_RDID_OUTSIZE_MAX] = { ST_M95_RDID };
	unsigned char readarr[ST_M95_RDID_INSIZE];
	uint32_t id1, id2;
	int ret;

	uint32_t rdid_outsize = ST_M95_RDID_2BA_OUTSIZE; // 16 bit address
	if (flash->chip->total_size * KiB > 64 * KiB)
		rdid_outsize = ST_M95_RDID_3BA_OUTSIZE; // 24 bit address

	ret = spi_send_command(flash, rdid_outsize, sizeof(readarr), cmd, readarr);
	if (ret)
		return ret;

	id1 = readarr[0]; // manufacture id
	id2 = (readarr[1] << 8) | readarr[2]; // SPI family code + model id

	msg_cdbg("%s: id1 0x%02"PRIx32", id2 0x%02"PRIx32"\n", __func__, id1, id2);

	if (id1 == flash->chip->manufacture_id && id2 == flash->chip->model_id)
		return 1;

	return 0;
}

/* ST95XXX chips don't have erase operation and erase is made as part of write command */
int spi_block_erase_emulation(struct flashctx *flash, unsigned int addr, unsigned int blocklen)
{
	uint8_t *erased_contents = NULL;
	int result = 0;

	erased_contents = (uint8_t *)malloc(blocklen * sizeof(uint8_t));
	if (!erased_contents) {
		msg_cerr("Out of memory!\n");
		return 1;
	}
	memset(erased_contents, ERASED_VALUE(flash), blocklen * sizeof(uint8_t));
	result = spi_write_chunked(flash, erased_contents, 0, blocklen, flash->chip->page_size);
	free(erased_contents);
	return result;
}
