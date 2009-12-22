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

void toggle_ready_jedec_common(chipaddr dst, int delay)
{
	unsigned int i = 0;
	uint8_t tmp1, tmp2;

	tmp1 = chip_readb(dst) & 0x40;

	while (i++ < 0xFFFFFFF) {
		if (delay)
			programmer_delay(delay);
		tmp2 = chip_readb(dst) & 0x40;
		if (tmp1 == tmp2) {
			break;
		}
		tmp1 = tmp2;
	}
	if (i > 0x100000)
		printf_debug("%s: excessive loops, i=0x%x\n", __func__, i);
}

void toggle_ready_jedec(chipaddr dst)
{
	toggle_ready_jedec_common(dst, 0);
}

/* Some chips require a minimum delay between toggle bit reads.
 * The Winbond W39V040C wants 50 ms between reads on sector erase toggle,
 * but experiments show that 2 ms are already enough. Pick a safety factor
 * of 4 and use an 8 ms delay.
 * Given that erase is slow on all chips, it is recommended to use 
 * toggle_ready_jedec_slow in erase functions.
 */
void toggle_ready_jedec_slow(chipaddr dst)
{
	toggle_ready_jedec_common(dst, 8 * 1000);
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
	if (i > 0x100000)
		printf_debug("%s: excessive loops, i=0x%x\n", __func__, i);
}

void start_program_jedec(chipaddr bios)
{
	chip_writeb(0xAA, bios + 0x5555);
	chip_writeb(0x55, bios + 0x2AAA);
	chip_writeb(0xA0, bios + 0x5555);
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
	if (probe_timing_enter)
		programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	if (probe_timing_enter)
		programmer_delay(10);
	chip_writeb(0x90, bios + 0x5555);
	if (probe_timing_enter)
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
	if (probe_timing_exit)
		programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	if (probe_timing_exit)
		programmer_delay(10);
	chip_writeb(0xF0, bios + 0x5555);
	if (probe_timing_exit)
		programmer_delay(probe_timing_exit);

	printf_debug("%s: id1 0x%02x, id2 0x%02x", __func__, largeid1, largeid2);
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

int erase_sector_jedec(struct flashchip *flash, unsigned int page, unsigned int pagesize)
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
	toggle_ready_jedec_slow(bios);

	if (check_erased_range(flash, page, pagesize)) {
		fprintf(stderr,"ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

int erase_block_jedec(struct flashchip *flash, unsigned int block, unsigned int blocksize)
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
	toggle_ready_jedec_slow(bios);

	if (check_erased_range(flash, block, blocksize)) {
		fprintf(stderr,"ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

/* erase chip with block_erase() prototype */
int erase_chip_block_jedec(struct flashchip *flash, unsigned int addr, unsigned int blocksize)
{
	if ((addr != 0) || (blocksize != flash->total_size * 1024)) {
		fprintf(stderr, "%s called with incorrect arguments\n",
			__func__);
		return -1;
	}
	return erase_chip_jedec(flash);
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

	toggle_ready_jedec_slow(bios);

	if (check_erased_range(flash, 0, total_size)) {
		fprintf(stderr,"ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

int write_page_write_jedec(struct flashchip *flash, uint8_t *src,
			   int start, int page_size)
{
	int i, tried = 0, failed;
	uint8_t *s = src;
	chipaddr bios = flash->virtual_memory;
	chipaddr dst = bios + start;
	chipaddr d = dst;

retry:
	/* Issue JEDEC Data Unprotect comand */
	start_program_jedec(bios);

	/* transfer data from source to destination */
	for (i = 0; i < page_size; i++) {
		/* If the data is 0xFF, don't program it */
		if (*src != 0xFF)
			chip_writeb(*src, dst);
		dst++;
		src++;
	}

	toggle_ready_jedec(dst - 1);

	dst = d;
	src = s;
	failed = verify_range(flash, src, start, page_size, NULL);

	if (failed && tried++ < MAX_REFLASH_TRIES) {
		fprintf(stderr, "retrying.\n");
		goto retry;
	}
	if (failed) {
		fprintf(stderr, " page 0x%lx failed!\n",
			(d - bios) / page_size);
	}
	return failed;
}

int write_byte_program_jedec(chipaddr bios, uint8_t *src,
			     chipaddr dst)
{
	int tried = 0, failed = 0;

	/* If the data is 0xFF, don't program it and don't complain. */
	if (*src == 0xFF) {
		return 0;
	}

retry:
	/* Issue JEDEC Byte Program command */
	start_program_jedec(bios);

	/* transfer data from source to destination */
	chip_writeb(*src, dst);
	toggle_ready_jedec(bios);

	if (chip_readb(dst) != *src && tried++ < MAX_REFLASH_TRIES) {
		goto retry;
	}

	if (tried >= MAX_REFLASH_TRIES)
		failed = 1;

	return failed;
}

int write_sector_jedec(chipaddr bios, uint8_t *src,
		       chipaddr dst, unsigned int page_size)
{
	int i, failed = 0;
	chipaddr olddst;

	olddst = dst;
	for (i = 0; i < page_size; i++) {
		if (write_byte_program_jedec(bios, src, dst))
			failed = 1;
		dst++, src++;
	}
	if (failed)
		fprintf(stderr, " writing sector at 0x%lx failed!\n", olddst);

	return failed;
}

int write_jedec(struct flashchip *flash, uint8_t *buf)
{
	int i, failed = 0;
	int total_size = flash->total_size * 1024;
	int page_size = flash->page_size;

	if (erase_chip_jedec(flash)) {
		fprintf(stderr,"ERASE FAILED!\n");
		return -1;
	}
	
	printf("Programming page: ");
	for (i = 0; i < total_size / page_size; i++) {
		printf("%04d at address: 0x%08x", i, i * page_size);
		if (write_page_write_jedec(flash, buf + i * page_size,
					   i * page_size, page_size))
			failed = 1;
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	}
	printf("\n");

	return failed;
}

int write_jedec_1(struct flashchip *flash, uint8_t * buf)
{
	int i;
	chipaddr bios = flash->virtual_memory;
	chipaddr dst = bios;

	programmer_delay(10);
	if (erase_flash(flash)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}

	printf("Programming page: ");
	for (i = 0; i < flash->total_size; i++) {
		if ((i & 0x3) == 0)
			printf("address: 0x%08lx", (unsigned long)i * 1024);

                write_sector_jedec(bios, buf + i * 1024, dst + i * 1024, 1024);

		if ((i & 0x3) == 0)
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	}

	printf("\n");
	return 0;
}
