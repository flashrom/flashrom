/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2006 Giampiero Giancipoli <gianci@email.it>
 * Copyright (C) 2006 coresystems GmbH <info@coresystems.de>
 * Copyright (C) 2007 Carl-Daniel Hailfinger
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

#define MAX_REFLASH_TRIES 0x10

/* Check one byte for odd parity */
uint8_t oddparity(uint8_t val)
{
	val = (val ^ (val >> 4)) & 0xf;
	val = (val ^ (val >> 2)) & 0x3;
	return (val ^ (val >> 1)) & 0x1;
}

void toggle_ready_jedec(chipaddr dst)
{
	unsigned int i = 0;
	uint8_t tmp1, tmp2;

	tmp1 = chip_readb(dst) & 0x40;

	while (i++ < 0xFFFFFFF) {
		tmp2 = chip_readb(dst) & 0x40;
		if (tmp1 == tmp2) {
			break;
		}
		tmp1 = tmp2;
	}
}

void data_polling_jedec(chipaddr dst, uint8_t data)
{
	unsigned int i = 0;
	uint8_t tmp;

	data &= 0x80;

	while (i++ < 0xFFFFFFF) {
		tmp = chip_readb(dst) & 0x80;
		if (tmp == data) {
			break;
		}
	}
}

void unprotect_jedec(chipaddr bios)
{
	chip_writeb(0xAA, bios + 0x5555);
	chip_writeb(0x55, bios + 0x2AAA);
	chip_writeb(0x80, bios + 0x5555);
	chip_writeb(0xAA, bios + 0x5555);
	chip_writeb(0x55, bios + 0x2AAA);
	chip_writeb(0x20, bios + 0x5555);

	programmer_delay(200);
}

void protect_jedec(chipaddr bios)
{
	chip_writeb(0xAA, bios + 0x5555);
	chip_writeb(0x55, bios + 0x2AAA);
	chip_writeb(0xA0, bios + 0x5555);

	programmer_delay(200);
}

int probe_jedec(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t id1, id2;
	uint32_t largeid1, largeid2;
	uint32_t flashcontent1, flashcontent2;
	int probe_timing_enter, probe_timing_exit;

	if (flash->probe_timing > 0) 
		probe_timing_enter = probe_timing_exit = flash->probe_timing;
	else if (flash->probe_timing == TIMING_ZERO) { /* No delay. */
		probe_timing_enter = probe_timing_exit = 0;
	} else if (flash->probe_timing == TIMING_FIXME) { /* == _IGNORED */
		printf_debug("Chip lacks correct probe timing information, "
			     "using default 10mS/40uS. ");
		probe_timing_enter = 10000;
		probe_timing_exit = 40;
	} else {
		printf("Chip has negative value in probe_timing, failing "
		       "without chip access\n");
		return 0;
	}

	/* Issue JEDEC Product ID Entry command */
	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0x90, bios + 0x5555);
	/* Older chips may need up to 100 us to respond. The ATMEL 29C020
	 * needs 10 ms according to the data sheet.
	 */
	programmer_delay(probe_timing_enter);

	/* Read product ID */
	id1 = chip_readb(bios);
	id2 = chip_readb(bios + 0x01);
	largeid1 = id1;
	largeid2 = id2;

	/* Check if it is a continuation ID, this should be a while loop. */
	if (id1 == 0x7F) {
		largeid1 <<= 8;
		id1 = chip_readb(bios + 0x100);
		largeid1 |= id1;
	}
	if (id2 == 0x7F) {
		largeid2 <<= 8;
		id2 = chip_readb(bios + 0x101);
		largeid2 |= id2;
	}

	/* Issue JEDEC Product ID Exit command */
	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0xF0, bios + 0x5555);
	programmer_delay(probe_timing_exit);

	printf_debug("%s: id1 0x%02x, id2 0x%02x", __FUNCTION__, largeid1, largeid2);
	if (!oddparity(id1))
		printf_debug(", id1 parity violation");

	/* Read the product ID location again. We should now see normal flash contents. */
	flashcontent1 = chip_readb(bios);
	flashcontent2 = chip_readb(bios + 0x01);

	/* Check if it is a continuation ID, this should be a while loop. */
	if (flashcontent1 == 0x7F) {
		flashcontent1 <<= 8;
		flashcontent1 |= chip_readb(bios + 0x100);
	}
	if (flashcontent2 == 0x7F) {
		flashcontent2 <<= 8;
		flashcontent2 |= chip_readb(bios + 0x101);
	}

	if (largeid1 == flashcontent1)
		printf_debug(", id1 is normal flash content");
	if (largeid2 == flashcontent2)
		printf_debug(", id2 is normal flash content");

	printf_debug("\n");
	if (largeid1 == flash->manufacture_id && largeid2 == flash->model_id)
		return 1;

	return 0;
}

int erase_sector_jedec(struct flashchip *flash, unsigned int page, int pagesize)
{
	chipaddr bios = flash->virtual_memory;

	/*  Issue the Sector Erase command   */
	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0x80, bios + 0x5555);
	programmer_delay(10);

	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0x30, bios + page);
	programmer_delay(10);

	/* wait for Toggle bit ready         */
	toggle_ready_jedec(bios);

	if (check_erased_range(flash, page, pagesize)) {
		fprintf(stderr,"ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

int erase_block_jedec(struct flashchip *flash, unsigned int block, int blocksize)
{
	chipaddr bios = flash->virtual_memory;

	/*  Issue the Sector Erase command   */
	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0x80, bios + 0x5555);
	programmer_delay(10);

	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0x50, bios + block);
	programmer_delay(10);

	/* wait for Toggle bit ready         */
	toggle_ready_jedec(bios);

	if (check_erased_range(flash, block, blocksize)) {
		fprintf(stderr,"ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

int erase_chip_jedec(struct flashchip *flash)
{
	int total_size = flash->total_size * 1024;
	chipaddr bios = flash->virtual_memory;

	/*  Issue the JEDEC Chip Erase command   */
	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0x80, bios + 0x5555);
	programmer_delay(10);

	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0x10, bios + 0x5555);
	programmer_delay(10);

	toggle_ready_jedec(bios);

	if (check_erased_range(flash, 0, total_size)) {
		fprintf(stderr,"ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

int write_page_write_jedec(struct flashchip *flash, uint8_t *src,
			   int start, int page_size)
{
	int i, tried = 0, start_index = 0, ok;
	uint8_t *s = src;
	chipaddr bios = flash->virtual_memory;
	chipaddr dst = bios + start;
	chipaddr d = dst;

retry:
	/* Issue JEDEC Data Unprotect comand */
	chip_writeb(0xAA, bios + 0x5555);
	chip_writeb(0x55, bios + 0x2AAA);
	chip_writeb(0xA0, bios + 0x5555);

	/* transfer data from source to destination */
	for (i = start_index; i < page_size; i++) {
		/* If the data is 0xFF, don't program it */
		if (*src != 0xFF)
			chip_writeb(*src, dst);
		dst++;
		src++;
	}

	toggle_ready_jedec(dst - 1);

	dst = d;
	src = s;
	ok = !verify_range(flash, src, start, page_size, NULL);

	if (!ok && tried++ < MAX_REFLASH_TRIES) {
		start_index = i;
		goto retry;
	}
	if (!ok) {
		fprintf(stderr, " page 0x%lx failed!\n",
			(d - bios) / page_size);
	}
	return !ok;
}

int write_byte_program_jedec(chipaddr bios, uint8_t *src,
			     chipaddr dst)
{
	int tried = 0, ok = 1;

	/* If the data is 0xFF, don't program it */
	if (*src == 0xFF) {
		return -1;
	}

retry:
	/* Issue JEDEC Byte Program command */
	chip_writeb(0xAA, bios + 0x5555);
	chip_writeb(0x55, bios + 0x2AAA);
	chip_writeb(0xA0, bios + 0x5555);

	/* transfer data from source to destination */
	chip_writeb(*src, dst);
	toggle_ready_jedec(bios);

	if (chip_readb(dst) != *src && tried++ < MAX_REFLASH_TRIES) {
		goto retry;
	}

	if (tried >= MAX_REFLASH_TRIES)
		ok = 0;

	return !ok;
}

int write_sector_jedec(chipaddr bios, uint8_t *src,
		       chipaddr dst, unsigned int page_size)
{
	int i;

	for (i = 0; i < page_size; i++) {
		write_byte_program_jedec(bios, src, dst);
		dst++, src++;
	}

	return 0;
}

int write_jedec(struct flashchip *flash, uint8_t *buf)
{
	int i;
	int total_size = flash->total_size * 1024;
	int page_size = flash->page_size;
	chipaddr bios = flash->virtual_memory;

	if (erase_chip_jedec(flash)) {
		fprintf(stderr,"ERASE FAILED!\n");
		return -1;
	}
	
	printf("Programming page: ");
	for (i = 0; i < total_size / page_size; i++) {
		printf("%04d at address: 0x%08x", i, i * page_size);
		write_page_write_jedec(flash, buf + i * page_size,
				       i * page_size, page_size);
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	}
	printf("\n");
	protect_jedec(bios);

	return 0;
}
