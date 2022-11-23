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

static int unlock_regspace2_uniform_64k(struct flashctx *flash)
{
	return unlock_regspace2_uniform(flash, 64 * 1024);
}

static int unlock_regspace2_uniform_32k(struct flashctx *flash)
{
	return unlock_regspace2_uniform(flash, 32 * 1024);
}

static int unlock_regspace2_block_eraser_0(struct flashctx *flash)
{
	// FIXME: this depends on the eraseblocks not to be filled up completely (i.e. to be null-terminated).
	const struct unlockblock *unlockblocks =
		(const struct unlockblock *)flash->chip->block_erasers[0].eraseblocks;
	return regspace2_walk_unlockblocks(flash, unlockblocks, &unlock_regspace2_block_generic);
}

static int unlock_regspace2_block_eraser_1(struct flashctx *flash)
{
	// FIXME: this depends on the eraseblocks not to be filled up completely (i.e. to be null-terminated).
	const struct unlockblock *unlockblocks =
		(const struct unlockblock *)flash->chip->block_erasers[1].eraseblocks;
	return regspace2_walk_unlockblocks(flash, unlockblocks, &unlock_regspace2_block_generic);
}

blockprotect_func_t *lookup_jedec_blockprotect_func_ptr(const struct flashchip *const chip)
{
	switch (chip->unlock) {
		case UNLOCK_REGSPACE2_BLOCK_ERASER_0: return unlock_regspace2_block_eraser_0;
		case UNLOCK_REGSPACE2_BLOCK_ERASER_1: return unlock_regspace2_block_eraser_1;
		case UNLOCK_REGSPACE2_UNIFORM_32K: return unlock_regspace2_uniform_32k;
		case UNLOCK_REGSPACE2_UNIFORM_64K: return unlock_regspace2_uniform_64k;
		default: return NULL; /* fallthough */
	};
}
