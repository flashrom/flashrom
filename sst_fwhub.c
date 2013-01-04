/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2009 Kontron Modular Computers
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

/* Adapted from the Intel FW hub stuff for 82802ax parts. */

#include "flash.h"

static int check_sst_fwhub_block_lock(struct flashctx *flash, int offset)
{
	chipaddr registers = flash->virtual_registers;
	uint8_t blockstatus;

	blockstatus = chip_readb(flash, registers + offset + 2);
	msg_cdbg("Lock status for 0x%06x (size 0x%06x) is %02x, ",
		     offset, flash->chip->page_size, blockstatus);
	switch (blockstatus & 0x3) {
	case 0x0:
		msg_cdbg("full access\n");
		break;
	case 0x1:
		msg_cdbg("write locked\n");
		break;
	case 0x2:
		msg_cdbg("locked open\n");
		break;
	case 0x3:
		msg_cdbg("write locked down\n");
		break;
	}
	/* Return content of the write_locked bit */
	return blockstatus & 0x1;
}

static int clear_sst_fwhub_block_lock(struct flashctx *flash, int offset)
{
	chipaddr registers = flash->virtual_registers;
	uint8_t blockstatus;

	blockstatus = check_sst_fwhub_block_lock(flash, offset);

	if (blockstatus) {
		msg_cdbg("Trying to clear lock for 0x%06x... ", offset);
		chip_writeb(flash, 0, registers + offset + 2);

		blockstatus = check_sst_fwhub_block_lock(flash, offset);
		msg_cdbg("%s\n", (blockstatus) ? "failed" : "OK");
	}

	return blockstatus;
}

int printlock_sst_fwhub(struct flashctx *flash)
{
	int i;

	for (i = 0; i < flash->chip->total_size * 1024; i += flash->chip->page_size)
		check_sst_fwhub_block_lock(flash, i);

	return 0;
}

int unlock_sst_fwhub(struct flashctx *flash)
{
	int i, ret=0;

	for (i = 0; i < flash->chip->total_size * 1024; i += flash->chip->page_size)
	{
		if (clear_sst_fwhub_block_lock(flash, i))
		{
			msg_cwarn("Warning: Unlock Failed for block 0x%06x\n", i);
			ret++;
		}
	}
	return ret;
}

