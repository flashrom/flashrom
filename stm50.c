/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2008 Claus Gindhart <claus.gindhart@kontron.com>
 * Copyright (C) 2009 Sean Nelson <audiohacked@gmail.com>
 * Copyright (C) 2013 Stefan Tauner
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

/*
 * All ST M50 chips are locked on startup. Most of them have a uniform 64 kB block layout, but some have
 * a non-uniform block/sector segmentation which has to be handled with more care. Some of the non-uniform
 * chips support erasing of the 4 kB sectors with another command.
 */

#include "flash.h"
#include "flashchips.h"
#include "chipdrivers.h"

static int stm50_unlock_address(struct flashctx *flash, int offset)
{
	chipaddr wrprotect = flash->virtual_registers + 2;
	static const uint8_t unlock_sector = 0x00;
	msg_cdbg("unlocking at 0x%x\n", offset);
	chip_writeb(flash, unlock_sector, wrprotect + offset);
	if (chip_readb(flash, wrprotect + offset) != unlock_sector) {
		msg_cerr("Cannot unlock address 0x%x\n", offset);
		return -1;
	}
	return 0;
}

/* Chips known to use a non-uniform block and sector layout for locking (as well as for erasing):
 * Name		Size	Address range of lock registers
 * M50FLW080A	1MB 	FFB00002 - FFBFF002
 * M50FLW080B	1MB	FFB00002 - FFBFF002
 * M50FW002	256k	FFBC0002 - FFBFC002
 * M50LPW116	2MB	FFA00002 - FFBFC002
 */
int unlock_stm50_nonuniform(struct flashctx *flash)
{
	int i;
	struct eraseblock *eraseblocks = flash->chip->block_erasers[0].eraseblocks;
	unsigned int done = 0;
	for (i = 0; i < NUM_ERASEREGIONS && eraseblocks[i].count != 0; i++) {
		unsigned int block_size = eraseblocks[i].size;
		unsigned int block_count = eraseblocks[i].count;

		int j;
		for (j = 0; j < block_count; j++) {
			if (stm50_unlock_address(flash, done)) {
				msg_cerr("UNLOCK FAILED!\n");
				return -1;
			}
			done += block_count * block_size;
		}
	}
	return 0;
}

/* Unlocking for uniform 64 kB blocks starting at offset 2 of the feature registers. */
int unlock_stm50_uniform(struct flashctx *flash)
{
	int i;
	for (i = 0; i < flash->chip->total_size * 1024; i+= 64 * 1024) {
		if (stm50_unlock_address(flash, i)) {
			msg_cerr("UNLOCK FAILED!\n");
			return -1;
		}
	}
	return 0;
}

/* This function is unused. */
int erase_sector_stm50(struct flashctx *flash, unsigned int sector, unsigned int sectorsize)
{
	chipaddr bios = flash->virtual_memory + sector;

	// clear status register
	chip_writeb(flash, 0x50, bios);
	// now start it
	chip_writeb(flash, 0x32, bios);
	chip_writeb(flash, 0xd0, bios);
	programmer_delay(10);

	wait_82802ab(flash);

	/* FIXME: Check the status register for errors. */
	return 0;
}
