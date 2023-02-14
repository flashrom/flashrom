/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2006 Giampiero Giancipoli <gianci@email.it>
 * Copyright (C) 2006 coresystems GmbH <info@coresystems.de>
 * Copyright (C) 2007-2012 Carl-Daniel Hailfinger
 * Copyright (C) 2009 Sean Nelson <audiohacked@gmail.com>
 * Copyright (C) 2014 Stefan Tauner
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

#define MAX_REFLASH_TRIES 0x10
#define MASK_FULL 0xffff
#define MASK_2AA 0x7ff
#define MASK_AAA 0xfff

/* Check one byte for odd parity */
uint8_t oddparity(uint8_t val)
{
	val = (val ^ (val >> 4)) & 0xf;
	val = (val ^ (val >> 2)) & 0x3;
	return (val ^ (val >> 1)) & 0x1;
}

static void toggle_ready_jedec_common(const struct flashctx *flash, chipaddr dst, unsigned int delay)
{
	unsigned int i = 0;
	uint8_t tmp1 = chip_readb(flash, dst) & 0x40;

	while (i++ < 0xFFFFFFF) {
		programmer_delay(flash, delay);
		uint8_t tmp2 = chip_readb(flash, dst) & 0x40;
		if (tmp1 == tmp2) {
			break;
		}
		tmp1 = tmp2;
	}
	if (i > 0x100000)
		msg_cdbg("%s: excessive loops, i=0x%x\n", __func__, i);
}

void toggle_ready_jedec(const struct flashctx *flash, chipaddr dst)
{
	toggle_ready_jedec_common(flash, dst, 0);
}

/* Some chips require a minimum delay between toggle bit reads.
 * The Winbond W39V040C wants 50 ms between reads on sector erase toggle,
 * but experiments show that 2 ms are already enough. Pick a safety factor
 * of 4 and use an 8 ms delay.
 * Given that erase is slow on all chips, it is recommended to use
 * toggle_ready_jedec_slow in erase functions.
 */
static void toggle_ready_jedec_slow(const struct flashctx *flash)
{
	const chipaddr dst = flash->virtual_memory;
	toggle_ready_jedec_common(flash, dst, 8 * 1000);
}

void data_polling_jedec(const struct flashctx *flash, chipaddr dst,
			uint8_t data)
{
	unsigned int i = 0;

	data &= 0x80;

	while (i++ < 0xFFFFFFF) {
		uint8_t tmp = chip_readb(flash, dst) & 0x80;
		if (tmp == data) {
			break;
		}
	}
	if (i > 0x100000)
		msg_cdbg("%s: excessive loops, i=0x%x\n", __func__, i);
}

static unsigned int getaddrmask(const struct flashchip *chip)
{
	switch (chip->feature_bits & FEATURE_ADDR_MASK) {
	case FEATURE_ADDR_FULL:
		return MASK_FULL;
		break;
	case FEATURE_ADDR_2AA:
		return MASK_2AA;
		break;
	case FEATURE_ADDR_AAA:
		return MASK_AAA;
		break;
	default:
		msg_cerr("%s called with unknown mask\n", __func__);
		return 0;
		break;
	}
}

static void start_program_jedec_common(const struct flashctx *flash)
{
	const chipaddr bios = flash->virtual_memory;
	const bool shifted = (flash->chip->feature_bits & FEATURE_ADDR_SHIFTED);
	const unsigned int mask = getaddrmask(flash->chip);

	chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
	chip_writeb(flash, 0xA0, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
}

int probe_jedec_29gl(struct flashctx *flash)
{
	const unsigned int mask = getaddrmask(flash->chip);
	const chipaddr bios = flash->virtual_memory;
	const struct flashchip *chip = flash->chip;

	/* Reset chip to a clean slate */
	chip_writeb(flash, 0xF0, bios + (0x5555 & mask));

	/* Issue JEDEC Product ID Entry command */
	chip_writeb(flash, 0xAA, bios + (0x5555 & mask));
	chip_writeb(flash, 0x55, bios + (0x2AAA & mask));
	chip_writeb(flash, 0x90, bios + (0x5555 & mask));

	/* Read product ID */
	// FIXME: Continuation loop, second byte is at word 0x100/byte 0x200
	uint32_t man_id = chip_readb(flash, bios + 0x00);
	uint32_t dev_id = (chip_readb(flash, bios + 0x01) << 16) |
			  (chip_readb(flash, bios + 0x0E) <<  8) |
			  (chip_readb(flash, bios + 0x0F) <<  0);

	/* Issue JEDEC Product ID Exit command */
	chip_writeb(flash, 0xF0, bios + (0x5555 & mask));

	msg_cdbg("%s: man_id 0x%02"PRIx32", dev_id 0x%06"PRIx32"", __func__, man_id, dev_id);
	if (!oddparity(man_id))
		msg_cdbg(", man_id parity violation");

	/* Read the product ID location again. We should now see normal flash contents. */
	uint32_t flashcontent1 = chip_readb(flash, bios + 0x00); // FIXME: Continuation loop
	uint32_t flashcontent2 = (chip_readb(flash, bios + 0x01) << 16) |
				 (chip_readb(flash, bios + 0x0E) <<  8) |
				 (chip_readb(flash, bios + 0x0F) <<  0);

	if (man_id == flashcontent1)
		msg_cdbg(", man_id seems to be normal flash content");
	if (dev_id == flashcontent2)
		msg_cdbg(", dev_id seems to be normal flash content");

	msg_cdbg("\n");
	if (man_id != chip->manufacture_id || dev_id != chip->model_id)
		return 0;

	return 1;
}

static int probe_timings(const struct flashchip *chip, unsigned int *tenter, unsigned int *texit)
{
	if (chip->probe_timing > 0) {
		*tenter = *texit = chip->probe_timing;
	} else if (chip->probe_timing == TIMING_ZERO) { /* No delay. */
		*tenter = *texit = 0;
	} else if (chip->probe_timing == TIMING_FIXME) { /* == _IGNORED */
		msg_cdbg("Chip lacks correct probe timing information, using default 10ms/40us. ");
		*tenter = 10000;
		*texit = 40;
	} else {
		msg_cerr("Chip has negative value in probe_timing, failing without chip access\n");
		return -1;
	}

	return 0;
}

int probe_jedec(struct flashctx *flash)
{
	const chipaddr bios = flash->virtual_memory;
	const struct flashchip *chip = flash->chip;
	const bool shifted = (flash->chip->feature_bits & FEATURE_ADDR_SHIFTED);
	const unsigned int mask = getaddrmask(flash->chip);
	uint8_t id1, id2;
	uint32_t largeid1, largeid2;
	uint32_t flashcontent1, flashcontent2;
	unsigned int probe_timing_enter, probe_timing_exit;

	if (probe_timings(chip, &probe_timing_enter, &probe_timing_exit) < 0)
		return 0;

	/* Earlier probes might have been too fast for the chip to enter ID
	 * mode completely. Allow the chip to finish this before seeing a
	 * reset command.
	 */
	programmer_delay(flash, probe_timing_enter);
	/* Reset chip to a clean slate */
	if ((chip->feature_bits & FEATURE_RESET_MASK) == FEATURE_LONG_RESET) {
		chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
		if (probe_timing_exit)
			programmer_delay(flash, 10);
		chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
		if (probe_timing_exit)
			programmer_delay(flash, 10);
	}
	chip_writeb(flash, 0xF0, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(flash, probe_timing_exit);

	/* Issue JEDEC Product ID Entry command */
	chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	if (probe_timing_enter)
		programmer_delay(flash, 10);
	chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
	if (probe_timing_enter)
		programmer_delay(flash, 10);
	chip_writeb(flash, 0x90, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(flash, probe_timing_enter);

	/* Read product ID */
	id1 = chip_readb(flash, bios + (0x00 << shifted));
	id2 = chip_readb(flash, bios + (0x01 << shifted));
	largeid1 = id1;
	largeid2 = id2;

	/* Check if it is a continuation ID, this should be a while loop. */
	if (id1 == 0x7F) {
		largeid1 <<= 8;
		id1 = chip_readb(flash, bios + 0x100);
		largeid1 |= id1;
	}
	if (id2 == 0x7F) {
		largeid2 <<= 8;
		id2 = chip_readb(flash, bios + 0x101);
		largeid2 |= id2;
	}

	/* Issue JEDEC Product ID Exit command */
	if ((chip->feature_bits & FEATURE_RESET_MASK) == FEATURE_LONG_RESET) {
		chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
		if (probe_timing_exit)
			programmer_delay(flash, 10);
		chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
		if (probe_timing_exit)
			programmer_delay(flash, 10);
	}
	chip_writeb(flash, 0xF0, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(flash, probe_timing_exit);

	msg_cdbg("%s: id1 0x%02"PRIx32", id2 0x%02"PRIx32"", __func__, largeid1, largeid2);
	if (!oddparity(id1))
		msg_cdbg(", id1 parity violation");

	/* Read the product ID location again. We should now see normal flash contents. */
	flashcontent1 = chip_readb(flash, bios + (0x00 << shifted));
	flashcontent2 = chip_readb(flash, bios + (0x01 << shifted));

	/* Check if it is a continuation ID, this should be a while loop. */
	if (flashcontent1 == 0x7F) {
		flashcontent1 <<= 8;
		flashcontent1 |= chip_readb(flash, bios + 0x100);
	}
	if (flashcontent2 == 0x7F) {
		flashcontent2 <<= 8;
		flashcontent2 |= chip_readb(flash, bios + 0x101);
	}

	if (largeid1 == flashcontent1)
		msg_cdbg(", id1 is normal flash content");
	if (largeid2 == flashcontent2)
		msg_cdbg(", id2 is normal flash content");

	msg_cdbg("\n");
	if (largeid1 != chip->manufacture_id || largeid2 != chip->model_id)
		return 0;

	return 1;
}

static void issuecmd(const struct flashctx *flash, uint8_t op, unsigned int operand)
{
	const chipaddr bios = flash->virtual_memory;
	bool shifted = (flash->chip->feature_bits & FEATURE_ADDR_SHIFTED);
	const unsigned int mask = getaddrmask(flash->chip);
	unsigned int delay_us = (flash->chip->probe_timing == TIMING_ZERO) ? 0 : 10;

	if (!operand)
		operand = (shifted ? 0x2AAA : 0x5555) & mask;

	chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(flash, delay_us);
	chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
	programmer_delay(flash, delay_us);
	chip_writeb(flash, op, bios + operand);
	programmer_delay(flash, delay_us);
}

int erase_sector_jedec(struct flashctx *flash, unsigned int page, unsigned int size)
{
	/* Issue the Sector Erase command */
	issuecmd(flash, 0x80, 0);
	issuecmd(flash, 0x30, page);

	/* Wait for Toggle bit ready */
	toggle_ready_jedec_slow(flash);

	/* FIXME: Check the status register for errors. */
	return 0;
}

int erase_block_jedec(struct flashctx *flash, unsigned int block, unsigned int size)
{
	/* Issue the Block Erase command */
	issuecmd(flash, 0x80, 0);
	issuecmd(flash, 0x50, block);

	/* Wait for Toggle bit ready */
	toggle_ready_jedec_slow(flash);

	/* FIXME: Check the status register for errors. */
	return 0;
}

/* erase chip with block_erase() prototype */
int erase_chip_block_jedec(struct flashctx *flash, unsigned int addr, unsigned int blocksize)
{
	if ((addr != 0) || (blocksize != flash->chip->total_size * 1024)) {
		msg_cerr("%s called with incorrect arguments\n", __func__);
		return -1;
	}

	/* Issue the JEDEC Chip Erase command */
	issuecmd(flash, 0x80, 0);
	issuecmd(flash, 0x10, 0);

	toggle_ready_jedec_slow(flash);

	/* FIXME: Check the status register for errors. */
	return 0;
}

static int write_byte_program_jedec_common(const struct flashctx *flash, const uint8_t *src,
					   chipaddr dst)
{
	int tries = 0;

	/* If the data is 0xFF, don't program it and don't complain. */
	if (*src == 0xFF) {
		return 0;
	}

	for (; tries < MAX_REFLASH_TRIES; tries++) {
		const chipaddr bios = flash->virtual_memory;
		/* Issue JEDEC Byte Program command */
		start_program_jedec_common(flash);

		/* transfer data from source to destination */
		chip_writeb(flash, *src, dst);
		toggle_ready_jedec(flash, bios);

		if (chip_readb(flash, dst) == *src)
			break;
	}

	return (tries >= MAX_REFLASH_TRIES) ? 1 : 0;
}

/* chunksize is 1 */
int write_jedec_1(struct flashctx *flash, const uint8_t *src, unsigned int start,
		  unsigned int len)
{
	int failed = 0;
	chipaddr dst = flash->virtual_memory + start;
	const chipaddr olddst = dst;

	for (unsigned int i = 0; i < len; i++) {
		if (write_byte_program_jedec_common(flash, src, dst))
			failed = 1;
		dst++, src++;
		update_progress(flash, FLASHROM_PROGRESS_WRITE, i + 1, len);
	}
	if (failed)
		msg_cerr(" writing sector at 0x%" PRIxPTR " failed!\n", olddst);

	return failed;
}

static int jedec_write_page(struct flashctx *flash, const uint8_t *src,
					 unsigned int start, unsigned int page_size)
{
	int tries = 0, failed;
	const uint8_t *s = src;
	const chipaddr bios = flash->virtual_memory;
	chipaddr dst = bios + start;
	chipaddr d = dst;

	for (; tries < MAX_REFLASH_TRIES; tries++) {
		/* Issue JEDEC Start Program command */
		start_program_jedec_common(flash);

		/* transfer data from source to destination */
		for (unsigned int i = 0; i < page_size; i++) {
			/* If the data is 0xFF, don't program it */
			if (*src != 0xFF)
				chip_writeb(flash, *src, dst);
			dst++;
			src++;
		}

		toggle_ready_jedec(flash, dst - 1);

		dst = d;
		src = s;
		failed = verify_range(flash, src, start, page_size);
		if (!failed)
			break;

		msg_cerr("retrying.\n");
	}

	if (failed) {
		msg_cerr(" page 0x%" PRIxPTR " failed!\n", (d - bios) / page_size);
	}

	return failed;
}

/* chunksize is page_size */
/*
 * Write a part of the flash chip.
 * FIXME: Use the chunk code from Michael Karcher instead.
 * This function is a slightly modified copy of spi_write_chunked.
 * Each page is written separately in chunks with a maximum size of chunksize.
 */
int write_jedec(struct flashctx *flash, const uint8_t *buf, unsigned int start,
		int unsigned len)
{
	unsigned int starthere, lenhere;
	/* FIXME: page_size is the wrong variable. We need max_writechunk_size
	 * in struct flashctx to do this properly. All chips using
	 * write_jedec have page_size set to max_writechunk_size, so
	 * we're OK for now.
	 */
	const unsigned int page_size = flash->chip->page_size;
	const unsigned int nwrites = (start + len - 1) / page_size;

	/* Warning: This loop has a very unusual condition and body.
	 * The loop needs to go through each page with at least one affected
	 * byte. The lowest page number is (start / page_size) since that
	 * division rounds down. The highest page number we want is the page
	 * where the last byte of the range lives. That last byte has the
	 * address (start + len - 1), thus the highest page number is
	 * (start + len - 1) / page_size. Since we want to include that last
	 * page as well, the loop condition uses <=.
	 */
	for (unsigned int i = start / page_size; i <= nwrites; i++) {
		/* Byte position of the first byte in the range in this page. */
		/* starthere is an offset to the base address of the chip. */
		starthere = max(start, i * page_size);
		/* Length of bytes in the range in this page. */
		lenhere = min(start + len, (i + 1) * page_size) - starthere;

		if (jedec_write_page(flash, buf + starthere - start, starthere, lenhere))
			return 1;
		update_progress(flash, FLASHROM_PROGRESS_WRITE, i + 1, nwrites + 1);
	}

	return 0;
}
