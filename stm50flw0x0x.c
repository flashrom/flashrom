/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2008 Claus Gindhart <claus.gindhart@kontron.com>
 * Copyright (C) 2009 Sean Nelson <audiohacked@gmail.com>
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
 * This module is designed for supporting the devices
 * ST M50FLW040A (not yet tested)
 * ST M50FLW040B (not yet tested)
 * ST M50FLW080A
 * ST M50FLW080B (not yet tested)
 */

#include "flash.h"
#include "flashchips.h"
#include "chipdrivers.h"

/*
 * claus.gindhart@kontron.com
 * The ST M50FLW080B and STM50FLW080B chips have to be unlocked,
 * before you can erase them or write to them.
 */
static int unlock_block_stm50flw0x0x(struct flashchip *flash, int offset)
{
	chipaddr wrprotect = flash->virtual_registers + 2;
	static const uint8_t unlock_sector = 0x00;
	int j;

	/*
	 * These chips have to be unlocked before you can erase them or write
	 * to them. The size of the locking sectors depends on the type
	 * of chip.
	 *
	 * Sometimes, the BIOS does this for you; so you probably
	 * don't need to worry about that.
	 */

	/* Check, if it's is a top/bottom-block with 4k-sectors. */
	/* TODO: What about the other types? */
	if ((offset == 0) ||
	    (offset == (flash->model_id == ST_M50FLW080A ? 0xE0000 : 0x10000))
	    || (offset == 0xF0000)) {

		// unlock each 4k-sector
		for (j = 0; j < 0x10000; j += 0x1000) {
			msg_cdbg("unlocking at 0x%x\n", offset + j);
			chip_writeb(unlock_sector, wrprotect + offset + j);
			if (chip_readb(wrprotect + offset + j) != unlock_sector) {
				msg_cerr("Cannot unlock sector @ 0x%x\n",
				       offset + j);
				return -1;
			}
		}
	} else {
		msg_cdbg("unlocking at 0x%x\n", offset);
		chip_writeb(unlock_sector, wrprotect + offset);
		if (chip_readb(wrprotect + offset) != unlock_sector) {
			msg_cerr("Cannot unlock sector @ 0x%x\n", offset);
			return -1;
		}
	}

	return 0;
}

int unlock_stm50flw0x0x(struct flashchip *flash)
{
	int i;

	for (i = 0; i < flash->total_size * 1024; i+= flash->page_size) {
		if(unlock_block_stm50flw0x0x(flash, i)) {
			msg_cerr("UNLOCK FAILED!\n");
			return -1;
		}
	}

	return 0;
}

/* This function is unused. */
int erase_sector_stm50flw0x0x(struct flashchip *flash, unsigned int sector, unsigned int sectorsize)
{
	chipaddr bios = flash->virtual_memory + sector;

	// clear status register
	chip_writeb(0x50, bios);
	// now start it
	chip_writeb(0x32, bios);
	chip_writeb(0xd0, bios);
	programmer_delay(10);

	wait_82802ab(flash);

	/* FIXME: Check the status register for errors. */
	return 0;
}
