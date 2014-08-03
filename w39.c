/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2008 coresystems GmbH
 * Copyright (C) 2010 Carl-Daniel Hailfinger
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

static uint8_t w39_idmode_readb(struct flashctx *flash, unsigned int offset)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t val;

	/* Product Identification Entry */
	chip_writeb(flash, 0xAA, bios + 0x5555);
	chip_writeb(flash, 0x55, bios + 0x2AAA);
	chip_writeb(flash, 0x90, bios + 0x5555);
	programmer_delay(10);

	/* Read something, maybe hardware lock bits */
	val = chip_readb(flash, bios + offset);

	/* Product Identification Exit */
	chip_writeb(flash, 0xAA, bios + 0x5555);
	chip_writeb(flash, 0x55, bios + 0x2AAA);
	chip_writeb(flash, 0xF0, bios + 0x5555);
	programmer_delay(10);

	return val;
}

static int printlock_w39_tblwp(uint8_t lock)
{
	msg_cdbg("Hardware bootblock locking (#TBL) is %sactive.\n",
		 (lock & (1 << 2)) ? "" : "not ");
	msg_cdbg("Hardware remaining chip locking (#WP) is %sactive..\n",
		(lock & (1 << 3)) ? "" : "not ");
	if (lock & ((1 << 2) | (1 << 3)))
		return -1;

	return 0;
}

static int printlock_w39_single_bootblock(uint8_t lock, uint16_t kB)
{
	msg_cdbg("Software %d kB bootblock locking is %sactive.\n", kB, (lock & 0x03) ? "" : "not ");
	if (lock & 0x03)
		return -1;

	return 0;
}

static int printlock_w39_bootblock_64k16k(uint8_t lock)
{
	msg_cdbg("Software 64 kB bootblock locking is %sactive.\n",
		 (lock & (1 << 0)) ? "" : "not ");
	msg_cdbg("Software 16 kB bootblock locking is %sactive.\n",
		 (lock & (1 << 1)) ? "" : "not ");
	if (lock & ((1 << 1) | (1 << 0)))
		return -1;

	return 0;
}

static int printlock_w39_common(struct flashctx *flash, unsigned int offset)
{
	uint8_t lock;

	lock = w39_idmode_readb(flash, offset);
	msg_cdbg("Lockout bits:\n");
	return printlock_w39_tblwp(lock);
}

int printlock_w39f010(struct flashctx *flash)
{
	uint8_t lock;
	int ret;

	lock = w39_idmode_readb(flash, 0x00002);
	msg_cdbg("Bottom boot block:\n");
	ret = printlock_w39_single_bootblock(lock, 16);

	lock = w39_idmode_readb(flash, 0x1fff2);
	msg_cdbg("Top boot block:\n");
	ret |= printlock_w39_single_bootblock(lock, 16);

	return ret;
}

int printlock_w39l010(struct flashctx *flash)
{
	uint8_t lock;
	int ret;

	lock = w39_idmode_readb(flash, 0x00002);
	msg_cdbg("Bottom boot block:\n");
	ret = printlock_w39_single_bootblock(lock, 8);

	lock = w39_idmode_readb(flash, 0x1fff2);
	msg_cdbg("Top boot block:\n");
	ret |= printlock_w39_single_bootblock(lock, 8);

	return ret;
}

int printlock_w39l020(struct flashctx *flash)
{
	uint8_t lock;
	int ret;

	lock = w39_idmode_readb(flash, 0x00002);
	msg_cdbg("Bottom boot block:\n");
	ret = printlock_w39_bootblock_64k16k(lock);

	lock = w39_idmode_readb(flash, 0x3fff2);
	msg_cdbg("Top boot block:\n");
	ret |= printlock_w39_bootblock_64k16k(lock);

	return ret;
}

int printlock_w39l040(struct flashctx *flash)
{
	uint8_t lock;
	int ret;

	lock = w39_idmode_readb(flash, 0x00002);
	msg_cdbg("Bottom boot block:\n");
	ret = printlock_w39_bootblock_64k16k(lock);

	lock = w39_idmode_readb(flash, 0x7fff2);
	msg_cdbg("Top boot block:\n");
	ret |= printlock_w39_bootblock_64k16k(lock);

	return ret;
}

int printlock_w39v040a(struct flashctx *flash)
{
	uint8_t lock;
	int ret = 0;

	/* The W39V040A datasheet contradicts itself on the lock register
	 * location: 0x00002 and 0x7fff2 are both mentioned. Pick the one
	 * which is similar to the other chips of the same family.
	 */
	lock = w39_idmode_readb(flash, 0x7fff2);
	msg_cdbg("Lockout bits:\n");

	ret = printlock_w39_tblwp(lock);
	ret |= printlock_w39_bootblock_64k16k(lock);

	return ret;
}

int printlock_w39v040b(struct flashctx *flash)
{
	return printlock_w39_common(flash, 0x7fff2);
}

int printlock_w39v040c(struct flashctx *flash)
{
	/* Typo in the datasheet? The other chips use 0x7fff2. */
	return printlock_w39_common(flash, 0xfff2);
}

int printlock_w39v040fa(struct flashctx *flash)
{
	int ret = 0;

	ret = printlock_w39v040a(flash);
	ret |= printlock_regspace2_uniform_64k(flash);

	return ret;
}

int printlock_w39v040fb(struct flashctx *flash)
{
	int ret = 0;

	ret = printlock_w39v040b(flash);
	ret |= printlock_regspace2_uniform_64k(flash);

	return ret;
}

int printlock_w39v040fc(struct flashctx *flash)
{
	int ret = 0;

	/* W39V040C and W39V040FC use different WP/TBL offsets. */
	ret = printlock_w39_common(flash, 0x7fff2);
	ret |= printlock_regspace2_uniform_64k(flash);

	return ret;
}

int printlock_w39v080a(struct flashctx *flash)
{
	return printlock_w39_common(flash, 0xffff2);
}

int printlock_w39v080fa(struct flashctx *flash)
{
	int ret = 0;

	ret = printlock_w39v080a(flash);
	ret |= printlock_regspace2_uniform_64k(flash);

	return ret;
}

int printlock_w39v080fa_dual(struct flashctx *flash)
{
	msg_cinfo("Block locking for W39V080FA in dual mode is "
		  "undocumented.\n");
	/* Better safe than sorry. */
	return -1;
}

int printlock_at49f(struct flashctx *flash)
{
	uint8_t lock = w39_idmode_readb(flash, 0x00002);
	msg_cdbg("Hardware bootblock lockout is %sactive.\n",
		 (lock & 0x01) ? "" : "not ");
	return 0;
}
