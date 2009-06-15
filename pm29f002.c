/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
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

int write_pm29f002(struct flashchip *flash, uint8_t *buf)
{
	int i, total_size = flash->total_size * 1024;
	chipaddr bios = flash->virtual_memory;
	chipaddr dst = bios;

	/* Pm29F002T/B use the same erase method... */
	if (erase_29f040b(flash)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}

	printf("Programming page: ");
	for (i = 0; i < total_size; i++) {
		if ((i & 0xfff) == 0)
			printf("address: 0x%08lx", (unsigned long)i);

		/* Pm29F002T/B only support byte-wise programming. */
		chip_writeb(0xAA, bios + 0x555);
		chip_writeb(0x55, bios + 0x2AA);
		chip_writeb(0xA0, bios + 0x555);
		chip_writeb(*buf++, dst++);

		/* Wait for Toggle bit ready. */
		toggle_ready_jedec(dst);

		if ((i & 0xfff) == 0)
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	}

	printf("\n");

	return 0;
}
