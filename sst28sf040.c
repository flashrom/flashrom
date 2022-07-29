/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2005 coresystems GmbH <stepan@openbios.org>
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
 */

#include "flash.h"
#include "chipdrivers.h"

#define AUTO_PG_ERASE1		0x20
#define AUTO_PG_ERASE2		0xD0
#define AUTO_PGRM		0x10
#define CHIP_ERASE		0x30
#define RESET			0xFF
#define READ_ID			0x90

int protect_28sf040(struct flashctx *flash)
{
	chipaddr bios = flash->virtual_memory;

	chip_readb(flash, bios + 0x1823);
	chip_readb(flash, bios + 0x1820);
	chip_readb(flash, bios + 0x1822);
	chip_readb(flash, bios + 0x0418);
	chip_readb(flash, bios + 0x041B);
	chip_readb(flash, bios + 0x0419);
	chip_readb(flash, bios + 0x040A);

	return 0;
}

int unprotect_28sf040(struct flashctx *flash)
{
	chipaddr bios = flash->virtual_memory;

	chip_readb(flash, bios + 0x1823);
	chip_readb(flash, bios + 0x1820);
	chip_readb(flash, bios + 0x1822);
	chip_readb(flash, bios + 0x0418);
	chip_readb(flash, bios + 0x041B);
	chip_readb(flash, bios + 0x0419);
	chip_readb(flash, bios + 0x041A);

	return 0;
}

int erase_sector_28sf040(struct flashctx *flash, unsigned int address,
			 unsigned int sector_size)
{
	chipaddr bios = flash->virtual_memory;

	/* This command sequence is very similar to erase_block_82802ab. */
	chip_writeb(flash, AUTO_PG_ERASE1, bios);
	chip_writeb(flash, AUTO_PG_ERASE2, bios + address);

	/* wait for Toggle bit ready */
	toggle_ready_jedec(flash, bios);

	/* FIXME: Check the status register for errors. */
	return 0;
}

/* chunksize is 1 */
int write_28sf040(struct flashctx *flash, const uint8_t *src, unsigned int start, unsigned int len)
{
	unsigned int i;
	chipaddr bios = flash->virtual_memory;
	chipaddr dst = flash->virtual_memory + start;

	for (i = 0; i < len; i++) {
		/* transfer data from source to destination */
		if (*src == 0xFF) {
			dst++, src++;
			/* If the data is 0xFF, don't program it */
			continue;
		}
		/*issue AUTO PROGRAM command */
		chip_writeb(flash, AUTO_PGRM, dst);
		chip_writeb(flash, *src++, dst++);

		/* wait for Toggle bit ready */
		toggle_ready_jedec(flash, bios);
		update_progress(flash, FLASHROM_PROGRESS_WRITE, i + 1, len);
	}

	return 0;
}

static int erase_28sf040(struct flashctx *flash)
{
	chipaddr bios = flash->virtual_memory;

	chip_writeb(flash, CHIP_ERASE, bios);
	chip_writeb(flash, CHIP_ERASE, bios);

	programmer_delay(flash, 10);
	toggle_ready_jedec(flash, bios);

	/* FIXME: Check the status register for errors. */
	return 0;
}

int erase_chip_28sf040(struct flashctx *flash, unsigned int addr,
		       unsigned int blocklen)
{
	if ((addr != 0) || (blocklen != flash->chip->total_size * 1024)) {
		msg_cerr("%s called with incorrect arguments\n",
			__func__);
		return -1;
	}
	return erase_28sf040(flash);
}
