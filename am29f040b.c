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

/* FIMXE: Use erase_sector_jedec if not? */
int erase_sector_29f040b(struct flashchip *flash, unsigned int address, unsigned int blocklen)
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

/* erase chip with block_erase() prototype */
int erase_chip_29f040b(struct flashchip *flash, unsigned int addr, unsigned int blocklen)
{
	if ((addr != 0) || (blocklen != flash->total_size * 1024)) {
		fprintf(stderr, "%s called with incorrect arguments\n",
			__func__);
		return -1;
	}
	return erase_29f040b(flash);
}

/* FIXME: use write_sector_jedec? */
static int write_sector_29f040b(chipaddr bios, uint8_t *src, chipaddr dst,
				unsigned int page_size)
{
	int i;

	for (i = 0; i < page_size; i++) {
		if ((i & 0xfff) == 0xfff)
			printf("0x%08lx", dst - bios);

		chip_writeb(0xAA, bios + 0x555);
		chip_writeb(0x55, bios + 0x2AA);
		chip_writeb(0xA0, bios + 0x555);
		chip_writeb(*src++, dst++);

		/* wait for Toggle bit ready */
		toggle_ready_jedec(bios);

		if ((i & 0xfff) == 0xfff)
			printf("\b\b\b\b\b\b\b\b\b\b");
	}

	return 0;
}

int probe_29f040b(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t id1, id2;

	chip_writeb(0xAA, bios + 0x555);
	chip_writeb(0x55, bios + 0x2AA);
	chip_writeb(0x90, bios + 0x555);

	id1 = chip_readb(bios);
	id2 = chip_readb(bios + 0x01);

	chip_writeb(0xF0, bios);

	programmer_delay(10);

	printf_debug("%s: id1 0x%02x, id2 0x%02x\n", __func__, id1, id2);
	if (id1 == flash->manufacture_id && id2 == flash->model_id)
		return 1;

	return 0;
}

/* FIXME: use erase_chip_jedec? */
int erase_29f040b(struct flashchip *flash)
{
	int total_size = flash->total_size * 1024;
	chipaddr bios = flash->virtual_memory;

	chip_writeb(0xAA, bios + 0x555);
	chip_writeb(0x55, bios + 0x2AA);
	chip_writeb(0x80, bios + 0x555);
	chip_writeb(0xAA, bios + 0x555);
	chip_writeb(0x55, bios + 0x2AA);
	chip_writeb(0x10, bios + 0x555);

	programmer_delay(10);
	toggle_ready_jedec(bios);

	if (check_erased_range(flash, 0, total_size)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

int write_29f040b(struct flashchip *flash, uint8_t *buf)
{
	int i;
	int total_size = flash->total_size * 1024;
	int page_size = flash->page_size;
	chipaddr bios = flash->virtual_memory;

	printf("Programming page ");
	for (i = 0; i < total_size / page_size; i++) {
		/* erase the page before programming */
		if (erase_sector_29f040b(flash, i * page_size, page_size)) {
			fprintf(stderr, "ERASE FAILED!\n");
			return -1;
		}

		/* write to the sector */
		printf("%04d at address: ", i);
		write_sector_29f040b(bios, buf + i * page_size,
				     bios + i * page_size, page_size);
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	}
	printf("\n");

	return 0;
}
