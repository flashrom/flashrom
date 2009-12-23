/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
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

int probe_29f002(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t id1, id2;

	chip_writeb(0xAA, bios + 0x5555);
	chip_writeb(0x55, bios + 0x2AAA);
	chip_writeb(0x90, bios + 0x5555);

	id1 = chip_readb(bios);
	id2 = chip_readb(bios + 0x01);

	chip_writeb(0xF0, bios);

	programmer_delay(10);

	printf_debug("%s: id1 0x%02x, id2 0x%02x\n", __func__, id1, id2);
	if (id1 == flash->manufacture_id && id2 == flash->model_id)
		return 1;

	return 0;
}

int erase_sector_29f002(struct flashchip *flash, unsigned int address, unsigned int blocklen)
{
	chipaddr bios = flash->virtual_memory;

	chip_writeb(0xAA, bios + 0x555);
	chip_writeb(0x55, bios + 0x2AA);
	chip_writeb(0x80, bios + 0x555);
	chip_writeb(0xAA, bios + 0x555);
	chip_writeb(0x55, bios + 0x2AA);
	chip_writeb(0x30, bios + address);

	programmer_delay(10);

	/* wait for Toggle bit ready         */
	toggle_ready_jedec(bios + address);

	if (check_erased_range(flash, address, blocklen)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

int erase_chip_29f002(struct flashchip *flash, unsigned int addr, unsigned int blocklen)
{
	if ((addr != 0) || (blocklen != flash->total_size * 1024)) {
		fprintf(stderr, "%s called with incorrect arguments\n",
			__func__);
		return -1;
	}
	return erase_29f002(flash);
}

/* FIXME: Use erase_chip_jedec?
 * erase_29f002 uses shorter addresses, sends F0 (exit ID mode) and
 * and has a bigger delay before polling the toggle bit */
int erase_29f002(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;

	chip_writeb(0xF0, bios + 0x555);
	chip_writeb(0xAA, bios + 0x555);
	chip_writeb(0x55, bios + 0x2AA);
	chip_writeb(0x80, bios + 0x555);
	chip_writeb(0xAA, bios + 0x555);
	chip_writeb(0x55, bios + 0x2AA);
	chip_writeb(0x10, bios + 0x555);

	programmer_delay(100);
	toggle_ready_jedec(bios);

	if (check_erased_range(flash, 0, flash->total_size * 1024)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	return 0;
}
