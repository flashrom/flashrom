/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2008 Claus Gindhart <claus.gindhart@kontron.com>
 * SPDX-FileCopyrightText: 2009 Sean Nelson <audiohacked@gmail.com>
 * SPDX-FileCopyrightText: 2013 Stefan Tauner
 */

/*
 * All ST M50 chips are locked on startup. Most of them have a uniform 64 kB block layout, but some have
 * a non-uniform block/sector segmentation which has to be handled with more care. Some of the non-uniform
 * chips support erasing of the 4 kB sectors with another command.
 */

#include "flash.h"
#include "parallel.h"
#include "chipdrivers.h"

static int stm50_erase_sector(struct flashctx *flash, unsigned int addr)
{
	chipaddr bios = flash->virtual_memory + addr;

	// clear status register
	chip_writeb(flash, 0x50, bios);
	// now start it
	chip_writeb(flash, 0x32, bios);
	chip_writeb(flash, 0xd0, bios);
	programmer_delay(flash, 10);

	uint8_t status = wait_82802ab(flash);
	print_status_82802ab(status);

	return status == 0x80;
}

/* Some ST M50* chips do support erasing of sectors. This function will derive the erase function to use from
 * the length of the of the block. For calls that apparently do not address a sector (but a block) we just call
 * the block erase function instead. FIXME: This duplicates the behavior of the remaining erasers for blocks and
 * might be fixed when flashrom supports multiple functions per eraser or erasers that do erase parts of the
 * chip only. */
int erase_sector_stm50(struct flashctx *flash, unsigned int addr, unsigned int len)
{
	if (len == 4096)
		return stm50_erase_sector(flash, addr);
	else
		return erase_block_82802ab(flash, addr, len);
}
