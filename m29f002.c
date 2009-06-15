/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Peter Stuge <peter@stuge.se>
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

int erase_m29f002(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;
	chip_writeb(0xaa, bios + 0x555);
	chip_writeb(0x55, bios + 0xaaa);
	chip_writeb(0x80, bios + 0x555);
	chip_writeb(0xaa, bios + 0x555);
	chip_writeb(0x55, bios + 0xaaa);
	chip_writeb(0x10, bios + 0x555);
	programmer_delay(10);
	toggle_ready_jedec(bios);
	if (check_erased_range(flash, 0, flash->total_size * 1024)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

static int rewrite_block(struct flashchip *flash, uint8_t *src,
			  unsigned long start, int size)
{
	chipaddr bios = flash->virtual_memory;
	chipaddr dst = bios + start;

	/* erase */
	chip_writeb(0xaa, bios + 0x555);
	chip_writeb(0x55, bios + 0xaaa);
	chip_writeb(0x80, bios + 0x555);
	chip_writeb(0xaa, bios + 0x555);
	chip_writeb(0x55, bios + 0xaaa);
	chip_writeb(0x30, dst);
	programmer_delay(10);
	toggle_ready_jedec(bios);
	if (check_erased_range(flash, start, size)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}

	/* program */
	while (size--) {
		chip_writeb(0xaa, bios + 0x555);
		chip_writeb(0x55, bios + 0xaaa);
		chip_writeb(0xa0, bios + 0x555);
		chip_writeb(*src, dst);
		toggle_ready_jedec(dst);
		dst++;
		src++;
	}
	return 0;
}

static int do_block(struct flashchip *flash, uint8_t *src, int i,
		     unsigned long start, int size)
{
	int ret;
	printf("%d at address: 0x%08lx", i, start);
	ret = rewrite_block(flash, src + start, start, size);
	if (ret)
		return ret;
	printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	return 0;
}

int write_m29f002t(struct flashchip *flash, uint8_t *buf)
{
	int i, page_size = flash->page_size;

	/* M29F002(N)T has 7 blocks. From bottom to top their sizes are:
	 * 64k 64k 64k 32k 8k 8k 16k
	 * flash->page_size is set to 64k in flashchips.c
	 */

	printf("Programming block: ");
	for (i = 0; i < 3; i++)
		do_block(flash, buf, i, i * page_size, page_size);
	do_block(flash, buf, i++, 0x30000, 32 * 1024);
	do_block(flash, buf, i++, 0x38000, 8 * 1024);
	do_block(flash, buf, i++, 0x3a000, 8 * 1024);
	do_block(flash, buf, i, 0x3c000, 16 * 1024);

	printf("\n");
	return 0;
}

int write_m29f002b(struct flashchip *flash, uint8_t *buf)
{
	int i = 0, page_size = flash->page_size;

	/* M29F002B has 7 blocks. From bottom to top their sizes are:
	 * 16k 8k 8k 32k 64k 64k 64k
	 * flash->page_size is set to 64k in flashchips.c
	 */

	printf("Programming block: ");
	do_block(flash, buf, i++, 0x00000, 16 * 1024);
	do_block(flash, buf, i++, 0x04000, 8 * 1024);
	do_block(flash, buf, i++, 0x06000, 8 * 1024);
	do_block(flash, buf, i++, 0x08000, 32 * 1024);
	for (; i < 7; i++)
		do_block(flash, buf, i, (i - 3) * page_size, page_size);

	printf("\n");
	return 0;
}
