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

/* WARNING! 
   This chip uses the standard JEDEC Addresses in 16-bit mode as word
   addresses. In byte mode, 0xAAA has to be used instead of 0x555 and
   0x555 instead of 0x2AA. Do *not* blindly replace with standard JEDEC
   functions. */

void write_page_m29f400bt(chipaddr bios, uint8_t *src,
			  chipaddr dst, int page_size)
{
	int i;

	for (i = 0; i < page_size; i++) {
		chip_writeb(0xAA, bios + 0xAAA);
		chip_writeb(0x55, bios + 0x555);
		chip_writeb(0xA0, bios + 0xAAA);

		/* transfer data from source to destination */
		chip_writeb(*src, dst);
		//chip_writeb(0xF0, bios);
		//programmer_delay(5);
		toggle_ready_jedec(dst);
		printf
		    ("Value in the flash at address 0x%lx = %#x, want %#x\n",
		     (dst - bios), chip_readb(dst), *src);
		dst++;
		src++;
	}
}

int probe_m29f400bt(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t id1, id2;

	chip_writeb(0xAA, bios + 0xAAA);
	chip_writeb(0x55, bios + 0x555);
	chip_writeb(0x90, bios + 0xAAA);

	programmer_delay(10);

	id1 = chip_readb(bios);
	/* The data sheet says id2 is at (bios + 0x01) and id2 listed in
	 * flash.h does not match. It should be possible to use JEDEC probe.
	 */
	id2 = chip_readb(bios + 0x02);

	chip_writeb(0xAA, bios + 0xAAA);
	chip_writeb(0x55, bios + 0x555);
	chip_writeb(0xF0, bios + 0xAAA);

	programmer_delay(10);

	printf_debug("%s: id1 0x%02x, id2 0x%02x\n", __func__, id1, id2);

	if (id1 == flash->manufacture_id && id2 == flash->model_id)
		return 1;

	return 0;
}

int erase_m29f400bt(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;

	chip_writeb(0xAA, bios + 0xAAA);
	chip_writeb(0x55, bios + 0x555);
	chip_writeb(0x80, bios + 0xAAA);

	chip_writeb(0xAA, bios + 0xAAA);
	chip_writeb(0x55, bios + 0x555);
	chip_writeb(0x10, bios + 0xAAA);

	programmer_delay(10);
	toggle_ready_jedec(bios);

	if (check_erased_range(flash, 0, flash->total_size * 1024)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

int block_erase_m29f400bt(struct flashchip *flash, unsigned int start, unsigned int len)
{
	chipaddr bios = flash->virtual_memory;
	chipaddr dst = bios + start;

	chip_writeb(0xAA, bios + 0xAAA);
	chip_writeb(0x55, bios + 0x555);
	chip_writeb(0x80, bios + 0xAAA);

	chip_writeb(0xAA, bios + 0xAAA);
	chip_writeb(0x55, bios + 0x555);
	//chip_writeb(0x10, bios + 0xAAA);
	chip_writeb(0x30, dst);

	programmer_delay(10);
	toggle_ready_jedec(bios);

	if (check_erased_range(flash, start, len)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

int block_erase_chip_m29f400bt(struct flashchip *flash, unsigned int address, unsigned int blocklen)
{
	if ((address != 0) || (blocklen != flash->total_size * 1024)) {
		fprintf(stderr, "%s called with incorrect arguments\n",
			__func__);
		return -1;
	}
	return erase_m29f400bt(flash);
}

int write_m29f400bt(struct flashchip *flash, uint8_t *buf)
{
	int i;
	int total_size = flash->total_size * 1024;
	int page_size = flash->page_size;
	chipaddr bios = flash->virtual_memory;

	//erase_m29f400bt (flash);
	printf("Programming page:\n ");
	/*********************************
	*Pages for M29F400BT:
	* 16	0x7c000		0x7ffff		TOP
	*  8 	0x7a000		0x7bfff
	*  8 	0x78000		0x79fff
	* 32	0x70000		0x77fff
	* 64	0x60000		0x6ffff
	* 64	0x50000		0x5ffff
	* 64	0x40000		0x4ffff
	*---------------------------------
	* 64	0x30000		0x3ffff
	* 64	0x20000		0x2ffff
	* 64	0x10000		0x1ffff
	* 64	0x00000		0x0ffff		BOTTOM
	*********************************/
	printf("total_size/page_size = %d\n", total_size / page_size);
	for (i = 0; i < (total_size / page_size) - 1; i++) {
		printf("%04d at address: 0x%08x\n", i, i * page_size);
		if (block_erase_m29f400bt(flash, i * page_size, page_size)) {
			fprintf(stderr, "ERASE FAILED!\n");
			return -1;
		}
		write_page_m29f400bt(bios, buf + i * page_size,
				     bios + i * page_size, page_size);
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	}

	printf("%04d at address: 0x%08x\n", 7, 0x70000);
	if (block_erase_m29f400bt(flash, 0x70000, 32 * 1024)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	write_page_m29f400bt(bios, buf + 0x70000, bios + 0x70000, 32 * 1024);

	printf("%04d at address: 0x%08x\n", 8, 0x78000);
	if (block_erase_m29f400bt(flash, 0x78000, 8 * 1024)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	write_page_m29f400bt(bios, buf + 0x78000, bios + 0x78000, 8 * 1024);

	printf("%04d at address: 0x%08x\n", 9, 0x7a000);
	if (block_erase_m29f400bt(flash, 0x7a000, 8 * 1024)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	write_page_m29f400bt(bios, buf + 0x7a000, bios + 0x7a000, 8 * 1024);

	printf("%04d at address: 0x%08x\n", 10, 0x7c000);
	if (block_erase_m29f400bt(flash, 0x7c000, 16 * 1024)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	write_page_m29f400bt(bios, buf + 0x7c000, bios + 0x7c000, 16 * 1024);

	printf("\n");

	return 0;
}

int write_coreboot_m29f400bt(struct flashchip *flash, uint8_t *buf)
{
	chipaddr bios = flash->virtual_memory;

	printf("Programming page:\n ");
	/*********************************
	*Pages for M29F400BT:
	* 16	0x7c000		0x7ffff		TOP
	*  8 	0x7a000		0x7bfff
	*  8 	0x78000		0x79fff
	* 32	0x70000		0x77fff
	* 64	0x60000		0x6ffff
	* 64	0x50000		0x5ffff
	* 64	0x40000		0x4ffff
	*---------------------------------
	* 64	0x30000		0x3ffff
	* 64	0x20000		0x2ffff
	* 64	0x10000		0x1ffff
	* 64	0x00000		0x0ffff		BOTTOM
	*********************************/
	printf("%04d at address: 0x%08x\n", 7, 0x00000);
	if (block_erase_m29f400bt(flash, 0x00000, 64 * 1024)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	write_page_m29f400bt(bios, buf + 0x00000, bios + 0x00000, 64 * 1024);

	printf("%04d at address: 0x%08x\n", 7, 0x10000);
	if (block_erase_m29f400bt(flash, 0x10000, 64 * 1024)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	write_page_m29f400bt(bios, buf + 0x10000, bios + 0x10000, 64 * 1024);

	printf("%04d at address: 0x%08x\n", 7, 0x20000);
	if (block_erase_m29f400bt(flash, 0x20000, 64 * 1024)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	write_page_m29f400bt(bios, buf + 0x20000, bios + 0x20000, 64 * 1024);

	printf("%04d at address: 0x%08x\n", 7, 0x30000);
	if (block_erase_m29f400bt(flash, 0x30000, 64 * 1024)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	write_page_m29f400bt(bios, buf + 0x30000, bios + 0x30000, 64 * 1024);

	printf("\n");

	return 0;
}
