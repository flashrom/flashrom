/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000 Silicon Integrated System Corporation
 * SPDX-FileCopyrightText: 2009 Kontron Modular Computers
 * SPDX-FileCopyrightText: 2009 Sean Nelson <audiohacked@gmail.com>
 */

/* Adapted from the Intel FW hub stuff for 82802ax parts. */

#include "flash.h"
#include "parallel.h"
#include "chipdrivers.h"

static int check_sst_fwhub_block_lock(struct flashctx *flash, unsigned int offset)
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

static int clear_sst_fwhub_block_lock(struct flashctx *flash, unsigned int offset)
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
	unsigned int i;

	for (i = 0; i < flash->chip->total_size * 1024; i += flash->chip->page_size)
		check_sst_fwhub_block_lock(flash, i);

	return 0;
}

int unlock_sst_fwhub(struct flashctx *flash)
{
	unsigned int i;
	int ret = 0;

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
