/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2016 Hatim Kanchwala <hatim@hatimak.me>
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

#include "chipdrivers.h"
#include "flash.h"
#include "writeprotect.h"

/*
 * struct wp {
 *	struct range *ranges;
 *	struct range *(*range_table) (struct flashctx *flash);
 * 	uint32_t (*bp_bitmask) (struct flashctx *flash);
 *	int (*set_range) (struct flashctx *flash, uint32_t start, uint32_t len);
 *	int (*disable) (struct flashctx *flash);
 *	int (*print_table) (struct flashctx *flash);
 * };
 */

/* A25LQ032, A25LQ32A */
struct wp a25l032_32a_wp = {
	.range_table	= &a25l032_range_table,
	.bp_bitmask	= &bp_bitmask_generic,
	.print_table	= &print_table_generic,
	.set_range	= &set_range_generic,
	.disable	= &disable_generic,
};

/* A25L080, A25LQ16, GD25LQ40, GD25LQ80, GD25LQ16, GD25Q16, GD25Q16B */
struct wp gd25_a25l080_q16_32a_wp = {
	.range_table	= &sec_block_range_pattern,
	.bp_bitmask	= &bp_bitmask_generic,
	.print_table	= &print_table_generic,
	.set_range	= &set_range_generic,
	.disable	= &disable_generic,
};

/* MX25L1605D, MX25L1608D, MX25L1673E */
struct wp mx25l16xd_wp = {
	.ranges = (struct range[]){
		/* BP3 effectively acts as CMP bit,
		 * BP[0..2] function normally. */
		{ 0x000000, 0 },
		{ 0x1f0000, 64 },
		{ 0x1e0000, 128 },
		{ 0x1c0000, 256 },
		{ 0x180000, 512 },
		{ 0x100000, 1024 },
		{ 0x000000, 2048 },
		{ 0x000000, 2048 },

		{ 0x000000, 2048 },
		{ 0x000000, 2048 },
		{ 0x000000, 1024 },
		{ 0x000000, 1536 },
		{ 0x000000, 1792 },
		{ 0x000000, 1920 },
		{ 0x000000, 1984 },
		{ 0x000000, 2048 },
	},
	.bp_bitmask	= &bp_bitmask_generic,
	.print_table	= &print_table_generic,
	.set_range	= &set_range_generic,
	.disable	= &disable_generic,
};

/* MX25L6406E, MX25L6408E, MX25L6405D */
struct wp mx25l6405d_wp = {
	/* BP3 effectively acts as CMP bit,
	 * BP[0..2] function normally. */
	.ranges = (struct range[]){
		{ 0x000000, 0 },
		{ 0x7e0000, 128 },
		{ 0x7c0000, 256 },
		{ 0x780000, 512 },
		{ 0x700000, 1024 },
		{ 0x600000, 2048 },
		{ 0x400000, 4096 },
		{ 0x000000, 8192 },

		{ 0x000000, 8192 },
		{ 0x000000, 4096 },
		{ 0x000000, 6144 },
		{ 0x000000, 7168 },
		{ 0x000000, 7680 },
		{ 0x000000, 7936 },
		{ 0x000000, 8064 },
		{ 0x000000, 8192 },
	},
	.bp_bitmask	= &bp_bitmask_generic,
	.print_table	= &print_table_generic,
	.set_range	= &set_range_generic,
	.disable	= &disable_generic,
};

/* MX25L3205D, MX25L3208D */
struct wp mx25lx5d_wp = {
	.ranges = (struct range[]){
		/* BP3 effectively acts as CMP bit,
		 * BP[0..2] function normally. */
		{ 0x000000, 0 },
		{ 0x3f0000, 64 },
		{ 0x3e0000, 128 },
		{ 0x3c0000, 256 },
		{ 0x380000, 512 },
		{ 0x300000, 1024 },
		{ 0x200000, 2048 },
		{ 0x000000, 4096 },

		{ 0x000000, 4096 },
		{ 0x000000, 2048 },
		{ 0x000000, 3072 },
		{ 0x000000, 3584 },
		{ 0x000000, 3840 },
		{ 0x000000, 3968 },
		{ 0x000000, 4032 },
		{ 0x000000, 4096 },
	},
	.bp_bitmask	= &bp_bitmask_generic,
	.print_table	= &print_table_generic,
	.set_range	= &set_range_generic,
	.disable	= &disable_generic,
};

/* MX25L6436E, MX25L6445E, MX25L6465E, MX25L6473E */
struct wp mx25lx65e_wp = {
	.ranges = (struct range[]){
		{ 0x000000, 0 },
		{ 0x7e0000, 128 },
		{ 0x7c0000, 256 },
		{ 0x780000, 512 },
		{ 0x700000, 1024 },
		{ 0x600000, 2048 },
		{ 0x400000, 4096 },
		{ 0x000000, 8192 },

		{ 0x000000, 8192 },
		{ 0x000000, 8192 },
		{ 0x000000, 8192 },
		{ 0x000000, 8192 },
		{ 0x000000, 8192 },
		{ 0x000000, 8192 },
		{ 0x000000, 8192 },
		{ 0x000000, 8192 },
	},
	.bp_bitmask	= &bp_bitmask_generic,
	.print_table	= &print_table_generic,
	.set_range	= &set_range_generic,
	.disable	= &disable_generic,
};
