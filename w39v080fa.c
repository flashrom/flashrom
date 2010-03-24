/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2008 coresystems GmbH
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

#include "flash.h"
#include "chipdrivers.h"

static int unlock_block_winbond_fwhub(struct flashchip *flash, int offset)
{
	chipaddr wrprotect = flash->virtual_registers + offset + 2;
	uint8_t locking;

	msg_cdbg("Trying to unlock block @0x%08x = 0x%02x\n", offset,
		     chip_readb(wrprotect));

	locking = chip_readb(wrprotect);
	switch (locking & 0x7) {
	case 0:
		msg_cdbg("Full Access.\n");
		return 0;
	case 1:
		msg_cdbg("Write Lock (Default State).\n");
		chip_writeb(0, wrprotect);
		return 0;
	case 2:
		msg_cdbg("Locked Open (Full Access, Lock Down).\n");
		return 0;
	case 3:
		msg_cerr("Error: Write Lock, Locked Down.\n");
		return -1;
	case 4:
		msg_cdbg("Read Lock.\n");
		chip_writeb(0, wrprotect);
		return 0;
	case 5:
		msg_cdbg("Read/Write Lock.\n");
		chip_writeb(0, wrprotect);
		return 0;
	case 6:
		msg_cerr("Error: Read Lock, Locked Down.\n");
		return -1;
	case 7:
		msg_cerr("Error: Read/Write Lock, Locked Down.\n");
		return -1;
	}

	/* We will never reach this point, but GCC doesn't know */
	return -1;
}

int unlock_winbond_fwhub(struct flashchip *flash)
{
	int i, total_size = flash->total_size * 1024;
	chipaddr bios = flash->virtual_memory;
	uint8_t locking;

	/* Are there any hardware restrictions that we can't overcome? 
	 * If flashrom fail here, someone's got to check all those GPIOs.
	 */

	/* Product Identification Entry */
	chip_writeb(0xAA, bios + 0x5555);
	chip_writeb(0x55, bios + 0x2AAA);
	chip_writeb(0x90, bios + 0x5555);
	programmer_delay(10);

	/* Read Hardware Lock Bits */
	locking = chip_readb(bios + 0xffff2);

	/* Product Identification Exit */
	chip_writeb(0xAA, bios + 0x5555);
	chip_writeb(0x55, bios + 0x2AAA);
	chip_writeb(0xF0, bios + 0x5555);
	programmer_delay(10);

	msg_cdbg("Lockout bits:\n");

	if (locking & (1 << 2))
		msg_cerr("Error: hardware bootblock locking (#TBL).\n");
	else
		msg_cdbg("No hardware bootblock locking (good!)\n");

	if (locking & (1 << 3))
		msg_cerr("Error: hardware block locking (#WP).\n");
	else
		msg_cdbg("No hardware block locking (good!)\n");

	if (locking & ((1 << 2) | (1 << 3)))
		return -1;

	/* Unlock the complete chip */
	for (i = 0; i < total_size; i += flash->page_size)
		if (unlock_block_winbond_fwhub(flash, i))
			return -1;

	return 0;
}
