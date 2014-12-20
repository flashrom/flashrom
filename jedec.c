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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
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
	uint8_t tmp1, tmp2;

	tmp1 = chip_readb(flash, dst) & 0x40;

	while (i++ < 0xFFFFFFF) {
		if (delay)
			programmer_delay(delay);
		tmp2 = chip_readb(flash, dst) & 0x40;
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
static void toggle_ready_jedec_slow(const struct flashctx *flash, chipaddr dst)
{
	toggle_ready_jedec_common(flash, dst, 8 * 1000);
}

void data_polling_jedec(const struct flashctx *flash, chipaddr dst,
			uint8_t data)
{
	unsigned int i = 0;
	uint8_t tmp;

	data &= 0x80;

	while (i++ < 0xFFFFFFF) {
		tmp = chip_readb(flash, dst) & 0x80;
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

static void start_program_jedec_common(const struct flashctx *flash, unsigned int mask)
{
	chipaddr bios = flash->virtual_memory;
	bool shifted = (flash->chip->feature_bits & FEATURE_ADDR_SHIFTED);

	chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
	chip_writeb(flash, 0xA0, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
}

int probe_jedec_29gl(struct flashctx *flash)
{
	unsigned int mask = getaddrmask(flash->chip);
	chipaddr bios = flash->virtual_memory;
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

	msg_cdbg("%s: man_id 0x%02x, dev_id 0x%06x", __func__, man_id, dev_id);
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

static int probe_jedec_common(struct flashctx *flash, unsigned int mask)
{
	chipaddr bios = flash->virtual_memory;
	const struct flashchip *chip = flash->chip;
	bool shifted = (flash->chip->feature_bits & FEATURE_ADDR_SHIFTED);
	uint8_t id1, id2;
	uint32_t largeid1, largeid2;
	uint32_t flashcontent1, flashcontent2;
	unsigned int probe_timing_enter, probe_timing_exit;

	if (chip->probe_timing > 0)
		probe_timing_enter = probe_timing_exit = chip->probe_timing;
	else if (chip->probe_timing == TIMING_ZERO) { /* No delay. */
		probe_timing_enter = probe_timing_exit = 0;
	} else if (chip->probe_timing == TIMING_FIXME) { /* == _IGNORED */
		msg_cdbg("Chip lacks correct probe timing information, "
			     "using default 10mS/40uS. ");
		probe_timing_enter = 10000;
		probe_timing_exit = 40;
	} else {
		msg_cerr("Chip has negative value in probe_timing, failing "
		       "without chip access\n");
		return 0;
	}

	/* Earlier probes might have been too fast for the chip to enter ID
	 * mode completely. Allow the chip to finish this before seeing a
	 * reset command.
	 */
	if (probe_timing_enter)
		programmer_delay(probe_timing_enter);
	/* Reset chip to a clean slate */
	if ((chip->feature_bits & FEATURE_RESET_MASK) == FEATURE_LONG_RESET)
	{
		chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
		if (probe_timing_exit)
			programmer_delay(10);
		chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
		if (probe_timing_exit)
			programmer_delay(10);
	}
	chip_writeb(flash, 0xF0, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	if (probe_timing_exit)
		programmer_delay(probe_timing_exit);

	/* Issue JEDEC Product ID Entry command */
	chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	if (probe_timing_enter)
		programmer_delay(10);
	chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
	if (probe_timing_enter)
		programmer_delay(10);
	chip_writeb(flash, 0x90, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	if (probe_timing_enter)
		programmer_delay(probe_timing_enter);

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
	if ((chip->feature_bits & FEATURE_RESET_MASK) == FEATURE_LONG_RESET)
	{
		chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
		if (probe_timing_exit)
			programmer_delay(10);
		chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
		if (probe_timing_exit)
			programmer_delay(10);
	}
	chip_writeb(flash, 0xF0, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	if (probe_timing_exit)
		programmer_delay(probe_timing_exit);

	msg_cdbg("%s: id1 0x%02x, id2 0x%02x", __func__, largeid1, largeid2);
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

static int erase_sector_jedec_common(struct flashctx *flash, unsigned int page,
				     unsigned int pagesize, unsigned int mask)
{
	chipaddr bios = flash->virtual_memory;
	bool shifted = (flash->chip->feature_bits & FEATURE_ADDR_SHIFTED);
	unsigned int delay_us = 0;

	if(flash->chip->probe_timing != TIMING_ZERO)
		delay_us = 10;

	/*  Issue the Sector Erase command   */
	chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(delay_us);
	chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
	programmer_delay(delay_us);
	chip_writeb(flash, 0x80, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(delay_us);

	chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(delay_us);
	chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
	programmer_delay(delay_us);
	chip_writeb(flash, 0x30, bios + page);
	programmer_delay(delay_us);

	/* wait for Toggle bit ready         */
	toggle_ready_jedec_slow(flash, bios);

	/* FIXME: Check the status register for errors. */
	return 0;
}

static int erase_block_jedec_common(struct flashctx *flash, unsigned int block,
				    unsigned int blocksize, unsigned int mask)
{
	chipaddr bios = flash->virtual_memory;
	bool shifted = (flash->chip->feature_bits & FEATURE_ADDR_SHIFTED);
	unsigned int delay_us = 0;

	if(flash->chip->probe_timing != TIMING_ZERO)
		delay_us = 10;

	/*  Issue the Sector Erase command   */
	chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(delay_us);
	chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
	programmer_delay(delay_us);
	chip_writeb(flash, 0x80, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(delay_us);

	chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(delay_us);
	chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
	programmer_delay(delay_us);
	chip_writeb(flash, 0x50, bios + block);
	programmer_delay(delay_us);

	/* wait for Toggle bit ready         */
	toggle_ready_jedec_slow(flash, bios);

	/* FIXME: Check the status register for errors. */
	return 0;
}

static int erase_chip_jedec_common(struct flashctx *flash, unsigned int mask)
{
	chipaddr bios = flash->virtual_memory;
	bool shifted = (flash->chip->feature_bits & FEATURE_ADDR_SHIFTED);
	unsigned int delay_us = 0;

	if(flash->chip->probe_timing != TIMING_ZERO)
		delay_us = 10;

	/*  Issue the JEDEC Chip Erase command   */
	chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(delay_us);
	chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
	programmer_delay(delay_us);
	chip_writeb(flash, 0x80, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(delay_us);

	chip_writeb(flash, 0xAA, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(delay_us);
	chip_writeb(flash, 0x55, bios + ((shifted ? 0x5555 : 0x2AAA) & mask));
	programmer_delay(delay_us);
	chip_writeb(flash, 0x10, bios + ((shifted ? 0x2AAA : 0x5555) & mask));
	programmer_delay(delay_us);

	toggle_ready_jedec_slow(flash, bios);

	/* FIXME: Check the status register for errors. */
	return 0;
}

static int write_byte_program_jedec_common(const struct flashctx *flash, const uint8_t *src,
					   chipaddr dst, unsigned int mask)
{
	int tried = 0, failed = 0;
	chipaddr bios = flash->virtual_memory;

	/* If the data is 0xFF, don't program it and don't complain. */
	if (*src == 0xFF) {
		return 0;
	}

retry:
	/* Issue JEDEC Byte Program command */
	start_program_jedec_common(flash, mask);

	/* transfer data from source to destination */
	chip_writeb(flash, *src, dst);
	toggle_ready_jedec(flash, bios);

	if (chip_readb(flash, dst) != *src && tried++ < MAX_REFLASH_TRIES) {
		goto retry;
	}

	if (tried >= MAX_REFLASH_TRIES)
		failed = 1;

	return failed;
}

/* chunksize is 1 */
int write_jedec_1(struct flashctx *flash, const uint8_t *src, unsigned int start,
		  unsigned int len)
{
	int i, failed = 0;
	chipaddr dst = flash->virtual_memory + start;
	chipaddr olddst;
	unsigned int mask;

	mask = getaddrmask(flash->chip);

	olddst = dst;
	for (i = 0; i < len; i++) {
		if (write_byte_program_jedec_common(flash, src, dst, mask))
			failed = 1;
		dst++, src++;
	}
	if (failed)
		msg_cerr(" writing sector at 0x%" PRIxPTR " failed!\n", olddst);

	return failed;
}

static int write_page_write_jedec_common(struct flashctx *flash, const uint8_t *src,
					 unsigned int start, unsigned int page_size)
{
	int i, tried = 0, failed;
	const uint8_t *s = src;
	chipaddr bios = flash->virtual_memory;
	chipaddr dst = bios + start;
	chipaddr d = dst;
	unsigned int mask;

	mask = getaddrmask(flash->chip);

retry:
	/* Issue JEDEC Start Program command */
	start_program_jedec_common(flash, mask);

	/* transfer data from source to destination */
	for (i = 0; i < page_size; i++) {
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

	if (failed && tried++ < MAX_REFLASH_TRIES) {
		msg_cerr("retrying.\n");
		goto retry;
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
	unsigned int i, starthere, lenhere;
	/* FIXME: page_size is the wrong variable. We need max_writechunk_size
	 * in struct flashctx to do this properly. All chips using
	 * write_jedec have page_size set to max_writechunk_size, so
	 * we're OK for now.
	 */
	unsigned int page_size = flash->chip->page_size;

	/* Warning: This loop has a very unusual condition and body.
	 * The loop needs to go through each page with at least one affected
	 * byte. The lowest page number is (start / page_size) since that
	 * division rounds down. The highest page number we want is the page
	 * where the last byte of the range lives. That last byte has the
	 * address (start + len - 1), thus the highest page number is
	 * (start + len - 1) / page_size. Since we want to include that last
	 * page as well, the loop condition uses <=.
	 */
	for (i = start / page_size; i <= (start + len - 1) / page_size; i++) {
		/* Byte position of the first byte in the range in this page. */
		/* starthere is an offset to the base address of the chip. */
		starthere = max(start, i * page_size);
		/* Length of bytes in the range in this page. */
		lenhere = min(start + len, (i + 1) * page_size) - starthere;

		if (write_page_write_jedec_common(flash, buf + starthere - start, starthere, lenhere))
			return 1;
	}

	return 0;
}

/* erase chip with block_erase() prototype */
int erase_chip_block_jedec(struct flashctx *flash, unsigned int addr,
			   unsigned int blocksize)
{
	unsigned int mask;

	mask = getaddrmask(flash->chip);
	if ((addr != 0) || (blocksize != flash->chip->total_size * 1024)) {
		msg_cerr("%s called with incorrect arguments\n",
			__func__);
		return -1;
	}
	return erase_chip_jedec_common(flash, mask);
}

int probe_jedec(struct flashctx *flash)
{
	unsigned int mask;

	mask = getaddrmask(flash->chip);
	return probe_jedec_common(flash, mask);
}

int erase_sector_jedec(struct flashctx *flash, unsigned int page,
		       unsigned int size)
{
	unsigned int mask;

	mask = getaddrmask(flash->chip);
	return erase_sector_jedec_common(flash, page, size, mask);
}

int erase_block_jedec(struct flashctx *flash, unsigned int page,
		      unsigned int size)
{
	unsigned int mask;

	mask = getaddrmask(flash->chip);
	return erase_block_jedec_common(flash, page, size, mask);
}

int erase_chip_jedec(struct flashctx *flash)
{
	unsigned int mask;

	mask = getaddrmask(flash->chip);
	return erase_chip_jedec_common(flash, mask);
}

struct unlockblock {
	unsigned int size;
	unsigned int count;
};

typedef int (*unlockblock_func)(const struct flashctx *flash, chipaddr offset);
static int regspace2_walk_unlockblocks(const struct flashctx *flash, const struct unlockblock *block, unlockblock_func func)
{
	chipaddr off = flash->virtual_registers + 2;
	while (block->count != 0) {
		unsigned int j;
		for (j = 0; j < block->count; j++) {
			if (func(flash, off))
				return -1;
			off += block->size;
		}
		block++;
	}
	return 0;
}

#define REG2_RWLOCK ((1 << 2) | (1 << 0))
#define REG2_LOCKDOWN (1 << 1)
#define REG2_MASK (REG2_RWLOCK | REG2_LOCKDOWN)

static int printlock_regspace2_block(const struct flashctx *flash, chipaddr lockreg)
{
	uint8_t state = chip_readb(flash, lockreg);
	msg_cdbg("Lock status of block at 0x%0*" PRIxPTR " is ", PRIxPTR_WIDTH, lockreg);
	switch (state & REG2_MASK) {
	case 0:
		msg_cdbg("Full Access.\n");
		break;
	case 1:
		msg_cdbg("Write Lock (Default State).\n");
		break;
	case 2:
		msg_cdbg("Locked Open (Full Access, Locked Down).\n");
		break;
	case 3:
		msg_cdbg("Write Lock, Locked Down.\n");
		break;
	case 4:
		msg_cdbg("Read Lock.\n");
		break;
	case 5:
		msg_cdbg("Read/Write Lock.\n");
		break;
	case 6:
		msg_cdbg("Read Lock, Locked Down.\n");
		break;
	case 7:
		msg_cdbg("Read/Write Lock, Locked Down.\n");
		break;
	}
	return 0;
}

int printlock_regspace2_blocks(const struct flashctx *flash, const struct unlockblock *blocks)
{
	return regspace2_walk_unlockblocks(flash, blocks, &printlock_regspace2_block);
}

static int printlock_regspace2_uniform(struct flashctx *flash, unsigned long block_size)
{
	const unsigned int elems = flash->chip->total_size * 1024 / block_size;
	struct unlockblock blocks[2] = {{.size = block_size, .count = elems}};
	return regspace2_walk_unlockblocks(flash, blocks, &printlock_regspace2_block);
}

int printlock_regspace2_uniform_64k(struct flashctx *flash)
{
	return printlock_regspace2_uniform(flash, 64 * 1024);
}

int printlock_regspace2_block_eraser_0(struct flashctx *flash)
{
	// FIXME: this depends on the eraseblocks not to be filled up completely (i.e. to be null-terminated).
	const struct unlockblock *unlockblocks =
		(const struct unlockblock *)flash->chip->block_erasers[0].eraseblocks;
	return regspace2_walk_unlockblocks(flash, unlockblocks, &printlock_regspace2_block);
}

int printlock_regspace2_block_eraser_1(struct flashctx *flash)
{
	// FIXME: this depends on the eraseblocks not to be filled up completely (i.e. to be null-terminated).
	const struct unlockblock *unlockblocks =
		(const struct unlockblock *)flash->chip->block_erasers[1].eraseblocks;
	return regspace2_walk_unlockblocks(flash, unlockblocks, &printlock_regspace2_block);
}

/* Try to change the lock register at address lockreg from cur to new.
 *
 * - Try to unlock the lock bit if requested and it is currently set (although this is probably futile).
 * - Try to change the read/write bits if requested.
 * - Try to set the lockdown bit if requested.
 * Return an error immediately if any of this fails. */
static int changelock_regspace2_block(const struct flashctx *flash, chipaddr lockreg, uint8_t cur, uint8_t new)
{
	/* Only allow changes to known read/write/lockdown bits */
	if (((cur ^ new) & ~REG2_MASK) != 0) {
		msg_cerr("Invalid lock change from 0x%02x to 0x%02x requested at 0x%0*" PRIxPTR "!\n"
			 "Please report a bug at flashrom@flashrom.org\n",
			 cur, new, PRIxPTR_WIDTH, lockreg);
		return -1;
	}

	/* Exit early if no change (of read/write/lockdown bits) was requested. */
	if (((cur ^ new) & REG2_MASK) == 0) {
		msg_cdbg2("Lock bits at 0x%0*" PRIxPTR " not changed.\n", PRIxPTR_WIDTH, lockreg);
		return 0;
	}

	/* Normally the lockdown bit can not be cleared. Try nevertheless if requested. */
	if ((cur & REG2_LOCKDOWN) && !(new & REG2_LOCKDOWN)) {
		chip_writeb(flash, cur & ~REG2_LOCKDOWN, lockreg);
		cur = chip_readb(flash, lockreg);
		if ((cur & REG2_LOCKDOWN) == REG2_LOCKDOWN) {
			msg_cwarn("Lockdown can't be removed at 0x%0*" PRIxPTR "! New value: 0x%02x.\n",
				  PRIxPTR_WIDTH, lockreg, cur);
			return -1;
		}
	}

	/* Change read and/or write bit */
	if ((cur ^ new) & REG2_RWLOCK) {
		/* Do not lockdown yet. */
		uint8_t wanted = (cur & ~REG2_RWLOCK) | (new & REG2_RWLOCK);
		chip_writeb(flash, wanted, lockreg);
		cur = chip_readb(flash, lockreg);
		if (cur != wanted) {
			msg_cerr("Changing lock bits failed at 0x%0*" PRIxPTR "! New value: 0x%02x.\n",
				 PRIxPTR_WIDTH, lockreg, cur);
			return -1;
		}
		msg_cdbg("Changed lock bits at 0x%0*" PRIxPTR " to 0x%02x.\n",
			 PRIxPTR_WIDTH, lockreg, cur);
	}

	/* Eventually, enable lockdown if requested. */
	if (!(cur & REG2_LOCKDOWN) && (new & REG2_LOCKDOWN)) {
		chip_writeb(flash, new, lockreg);
		cur = chip_readb(flash, lockreg);
		if (cur != new) {
			msg_cerr("Enabling lockdown FAILED at 0x%0*" PRIxPTR "! New value: 0x%02x.\n",
				 PRIxPTR_WIDTH, lockreg, cur);
			return -1;
		}
		msg_cdbg("Enabled lockdown at 0x%0*" PRIxPTR ".\n", PRIxPTR_WIDTH, lockreg);
	}

	return 0;
}

static int unlock_regspace2_block_generic(const struct flashctx *flash, chipaddr lockreg)
{
	uint8_t old = chip_readb(flash, lockreg);
	/* We don't care for the lockdown bit as long as the RW locks are 0 after we're done */
	return changelock_regspace2_block(flash, lockreg, old, old & ~REG2_RWLOCK);
}

static int unlock_regspace2_uniform(struct flashctx *flash, unsigned long block_size)
{
	const unsigned int elems = flash->chip->total_size * 1024 / block_size;
	struct unlockblock blocks[2] = {{.size = block_size, .count = elems}};
	return regspace2_walk_unlockblocks(flash, blocks, &unlock_regspace2_block_generic);
}

int unlock_regspace2_uniform_64k(struct flashctx *flash)
{
	return unlock_regspace2_uniform(flash, 64 * 1024);
}

int unlock_regspace2_uniform_32k(struct flashctx *flash)
{
	return unlock_regspace2_uniform(flash, 32 * 1024);
}

int unlock_regspace2_block_eraser_0(struct flashctx *flash)
{
	// FIXME: this depends on the eraseblocks not to be filled up completely (i.e. to be null-terminated).
	const struct unlockblock *unlockblocks =
		(const struct unlockblock *)flash->chip->block_erasers[0].eraseblocks;
	return regspace2_walk_unlockblocks(flash, unlockblocks, &unlock_regspace2_block_generic);
}

int unlock_regspace2_block_eraser_1(struct flashctx *flash)
{
	// FIXME: this depends on the eraseblocks not to be filled up completely (i.e. to be null-terminated).
	const struct unlockblock *unlockblocks =
		(const struct unlockblock *)flash->chip->block_erasers[1].eraseblocks;
	return regspace2_walk_unlockblocks(flash, unlockblocks, &unlock_regspace2_block_generic);
}

