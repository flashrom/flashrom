/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2008 Peter Stuge <peter@stuge.se>
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

int probe_w39v040c(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;
	int result = probe_jedec(flash);
	uint8_t lock;

	if (!result)
		return result;

	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0x90, bios + 0x5555);
	programmer_delay(10);

	lock = chip_readb(bios + 0xfff2);

	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0xF0, bios + 0x5555);
	programmer_delay(40);

	printf("%s: Boot block #TBL is %slocked, rest of chip #WP is %slocked.\n",
		__func__, lock & 0x4 ? "" : "un", lock & 0x8 ? "" : "un");
	return 1;
}

int printlock_w39v040c(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t lock;

	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0x90, bios + 0x5555);
	programmer_delay(10);

	lock = chip_readb(bios + 0xfff2);

	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0xF0, bios + 0x5555);
	programmer_delay(40);

	printf("%s: Boot block #TBL is %slocked, rest of chip #WP is %slocked.\n",
		__func__, lock & 0x4 ? "" : "un", lock & 0x8 ? "" : "un");
	return 0;
}

int erase_w39v040c(struct flashchip *flash)
{
	int i;
	unsigned int total_size = flash->total_size * 1024;

	for (i = 0; i < total_size; i += flash->page_size) {
		if (erase_sector_jedec(flash, i, flash->page_size)) {
			fprintf(stderr, "ERASE FAILED!\n");
			return -1;
		}
	}

	return 0;
}

int write_w39v040c(struct flashchip *flash, uint8_t *buf)
{
	int i;
	int total_size = flash->total_size * 1024;
	int page_size = flash->page_size;
	chipaddr bios = flash->virtual_memory;

	if (erase_flash(flash)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}

	printf("Programming page: ");
	for (i = 0; i < total_size / page_size; i++) {
		printf("%04d at address: 0x%08x", i, i * page_size);
		write_sector_jedec_common(flash, buf + i * page_size,
				   bios + i * page_size, page_size, 0xffff);
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	}
	printf("\n");

	return 0;
}
