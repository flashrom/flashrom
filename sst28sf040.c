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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "flash.h"
#include "chipdrivers.h"

#define AUTO_PG_ERASE1		0x20
#define AUTO_PG_ERASE2		0xD0
#define AUTO_PGRM		0x10
#define CHIP_ERASE		0x30
#define RESET			0xFF
#define READ_ID			0x90

static void protect_28sf040(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;

	chip_readb(bios + 0x1823);
	chip_readb(bios + 0x1820);
	chip_readb(bios + 0x1822);
	chip_readb(bios + 0x0418);
	chip_readb(bios + 0x041B);
	chip_readb(bios + 0x0419);
	chip_readb(bios + 0x040A);
}

static void unprotect_28sf040(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;

	chip_readb(bios + 0x1823);
	chip_readb(bios + 0x1820);
	chip_readb(bios + 0x1822);
	chip_readb(bios + 0x0418);
	chip_readb(bios + 0x041B);
	chip_readb(bios + 0x0419);
	chip_readb(bios + 0x041A);
}

int erase_sector_28sf040(struct flashchip *flash, unsigned int address, unsigned int sector_size)
{
	chipaddr bios = flash->virtual_memory;

	chip_writeb(AUTO_PG_ERASE1, bios);
	chip_writeb(AUTO_PG_ERASE2, bios + address);

	/* wait for Toggle bit ready */
	toggle_ready_jedec(bios);

	if (check_erased_range(flash, address, sector_size)) {
		msg_cerr("ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

int write_sector_28sf040(struct flashchip *flash, uint8_t *src, int start, int len)
{
	int i;
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
		chip_writeb(AUTO_PGRM, dst);
		chip_writeb(*src++, dst++);

		/* wait for Toggle bit ready */
		toggle_ready_jedec(bios);
	}

	return 0;
}

static int erase_28sf040(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;

	unprotect_28sf040(flash);
	chip_writeb(CHIP_ERASE, bios);
	chip_writeb(CHIP_ERASE, bios);
	protect_28sf040(flash);

	programmer_delay(10);
	toggle_ready_jedec(bios);

	if (check_erased_range(flash, 0, flash->total_size * 1024)) {
		msg_cerr("ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

int write_28sf040(struct flashchip *flash, uint8_t *buf)
{
	unprotect_28sf040(flash);

	write_sector_28sf040(flash, buf, 0, flash->total_size * 1024);

	protect_28sf040(flash);

	return 0;
}

int erase_chip_28sf040(struct flashchip *flash, unsigned int addr, unsigned int blocklen)
{
	if ((addr != 0) || (blocklen != flash->total_size * 1024)) {
		msg_cerr("%s called with incorrect arguments\n",
			__func__);
		return -1;
	}
	return erase_28sf040(flash);
}
