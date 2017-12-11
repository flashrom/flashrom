/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2016 Hatim Kanchwala <hatim at hatimak.me <https://www.flashrom.org/mailman/listinfo/flashrom>>
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
#include "spi25_statusreg.h"
#include "writeprotect.h"

static uint32_t wp_range_table_id;
static struct range *wp_range_table;

// TODO(hatim): Update for chips with density greater than 128Mb
static uint16_t const sec_block_multiples[7][8] = {
					/* Chip densities */
	{ 0, 1, 1, 1, 0, 1, 1, 1 },	/* = 64 kB */
	{ 0, 2, 1, 1, 0, 2, 1, 1 },	/* = 128 kB */
	{ 0, 4, 2, 1, 0, 4, 2, 1 },	/* = 256 kB */
	{ 0, 8, 4, 2, 1, 1, 1, 1 },	/* = 512 kB */
	{ 0, 16, 8, 4, 2, 1, 1, 1 },	/* = 1024 kB */
	{ 0, 32, 16, 8, 4, 2, 1, 1 },	/* = 2 * 1024 kB */
	{ 0, 64, 32, 16, 8, 4, 2, 1 },	/* >= 4 * 1024 kB and <= 16 * 1024 kB */
};

static char const *bit_string[3][32] = {
	{ "0\t0\t0", "0\t0\t1", "0\t1\t0", "0\t1\t1", "1\t0\t0", "1\t0\t1", "1\t1\t0", "1\t1\t1" },
	{ "0\t0\t0\t0", "0\t0\t0\t1", "0\t0\t1\t0", "0\t0\t1\t1", "0\t1\t0\t0", "0\t1\t0\t1", "0\t1\t1\t0",
	  "0\t1\t1\t1", "1\t0\t0\t0", "1\t0\t0\t1", "1\t0\t1\t0", "1\t0\t1\t1", "1\t1\t0\t0", "1\t1\t0\t1",
	  "1\t1\t1\t0", "1\t1\t1\t1" },
	{ "0\t0\t0\t0\t0", "0\t0\t0\t0\t1", "0\t0\t0\t1\t0", "0\t0\t0\t1\t1", "0\t0\t1\t0\t0", "0\t0\t1\t0\t1",
	  "0\t0\t1\t1\t0", "0\t0\t1\t1\t1", "0\t1\t0\t0\t0", "0\t1\t0\t0\t1", "0\t1\t0\t1\t0", "0\t1\t0\t1\t1",
	  "0\t1\t1\t0\t0", "0\t1\t1\t0\t1", "0\t1\t1\t1\t0", "0\t1\t1\t1\t1", "1\t0\t0\t0\t0", "1\t0\t0\t0\t1",
	  "1\t0\t0\t1\t0", "1\t0\t0\t1\t1", "1\t0\t1\t0\t0", "1\t0\t1\t0\t1", "1\t0\t1\t1\t0", "1\t0\t1\t1\t1",
	  "1\t1\t0\t0\t0", "1\t1\t0\t0\t1", "1\t1\t0\t1\t0", "1\t1\t0\t1\t1", "1\t1\t1\t0\t0", "1\t1\t1\t0\t1",
	  "1\t1\t1\t1\t0", "1\t1\t1\t1\t1" },
};

/* === Generic functions === */
/* Most SPI chips (especially GigaDeice and Winbond) follow a specific
 * pattern for their block protection ranges. This pattern is a function
 * of BP and other modifier bits, and chip density. Quite a few AMIC, Eon,
 * Fudan and Macronix chips also somewhat follow this pattern. Given the status
 * register layout of a chip, the following returns the range table according
 * to the pattern relevant to the chip. The presence of a CMP bit is handled
 * automatically. Please see the note below for the standard for CMP bit.
 *
 * When adding support for newer chips that have protection ranges similar
 * to the chips already using this pattern, consider using this function
 * to generate the boilerplate range, and then modifying it appropriately. */
// TODO(hatim): Make static (?)
struct range *sec_block_range_pattern(struct flashctx *flash) {
	/* Note that a couple of lines in the code below may seem missing,
	 * but we get away with not writing them because ranges[] is static. */
	static struct range ranges[LEN_RANGES];
	uint8_t convention = 1, multiple = 0, cmp_mask_pos = 3;
	/* cmp_mask_pos points to the bit position for CMP, if the chip has one. */
	int tot_size = flash->chip->total_size;

	// TODO(hatim): Add support for chips with density greater than 128Mb
	switch (tot_size) {
	case 64:
		multiple = 0;
		break;
	case 128:
		multiple = 1;
		break;
	case 256:
		multiple = 2;
		break;
	case 512:
		multiple = 3;
		break;
	case 1024:
		multiple = 4;
		convention = 0;
		break;
	case 2 * 1024:
		multiple = 5;
		convention = 0;
		break;
	case 4 * 1024:
	case 8 * 1024:
	case 16 * 1024:
		multiple = 6;
		break;
	}
	/* It has been observed that roles of SEC and TB bits are identical
	 * to that of BP4 and BP3 bits respectively.
	 * (BP2=0, BP1=0, BP1=0) and (BP2=1, BP1=1, BP1=1) almost always
	 * correpsond to NONE and ALL protection range respectively. */
	/* (SEC=0, TB=0) Uppper */
	for (int i = 0; i < 8; i++) {
		if (sec_block_multiples[multiple][i]) {
			ranges[i].start = (tot_size - tot_size / sec_block_multiples[multiple][i]) * 1024;
			ranges[i].len	= tot_size / sec_block_multiples[multiple][i];
		}
	}
	/* No point in continuing if chip does NOT have TB (or BP3) bit. */
	if (!(pos_bit(flash, TB) != -1 || pos_bit(flash, BP3) != -1))
		goto compute_cmp_ranges;

	/* (SEC=0, TB=1) Lower */
	for (int i = 0; i < 8; i++) {
		if (sec_block_multiples[multiple][i]) {
			ranges[0x01 << 3 | i].len = tot_size / sec_block_multiples[multiple][i];
		}
	}
	cmp_mask_pos++;
	/* No point in continuing if chip does NOT have SEC (or BP4) bit. */
	if (!(pos_bit(flash, SEC) != -1 || pos_bit(flash, BP4) != -1))
		goto compute_cmp_ranges;

	/* (SEC=1, TB=0) Uppper */
	ranges[0x02 << 3 | 0x01].start	= (tot_size - 4) * 1024;
	ranges[0x02 << 3 | 0x01].len	= 4;
	ranges[0x02 << 3 | 0x02].start	= (tot_size - 8) * 1024;
	ranges[0x02 << 3 | 0x02].len	= 8;
	ranges[0x02 << 3 | 0x03].start	= (tot_size - 16) * 1024;
	ranges[0x02 << 3 | 0x03].len	= 16;
	ranges[0x02 << 3 | 0x04].start	= (tot_size - 32) * 1024;
	ranges[0x02 << 3 | 0x04].len	= 32;
	ranges[0x02 << 3 | 0x05].start	= (tot_size - 32) * 1024;
	ranges[0x02 << 3 | 0x05].len	= 32;
	if (convention) {
		ranges[0x02 << 3 | 0x06].start	= (tot_size - 32) * 1024;
		ranges[0x02 << 3 | 0x06].len	= 32;
	} else
		ranges[0x02 << 3 | 0x06].len	= tot_size;
	ranges[0x02 << 3 | 0x07].len	= tot_size;

	/* SEC, TB = (1, 1) Lower */
	ranges[0x03 << 3 | 0x01].len	= 4;
	ranges[0x03 << 3 | 0x02].len	= 8;
	ranges[0x03 << 3 | 0x03].len	= 16;
	ranges[0x03 << 3 | 0x04].len	= 32;
	ranges[0x03 << 3 | 0x05].len	= 32;
	if (convention)
		ranges[0x02 << 3 | 0x06].len = 32;
	else
		ranges[0x02 << 3 | 0x06].len = tot_size;
	ranges[0x03 << 3 | 0x07].len	= tot_size;
	cmp_mask_pos++;

compute_cmp_ranges:
	/* For chips with a CMP bit, the first unused bit position (or the position just higher
	 * than the most significant bit position in the bp_bitmask) is for the CMP bit. So for
	 * instance a chip has BP[2..0] and CMP bits then, bits 0, 1 and 2 represent BP0, BP1
	 * and BP2 respectively, and bit 3 represents CMP. We use bp_bitmask to index into
	 * ranges[]. So, for CMP=1 and BP[2..0]=0x03, the valid range is given by ranges[0x0f]. */
	if (pos_bit(flash, CMP) != -1) {
		for (int i = 0, j = 1 << cmp_mask_pos; i < (1 << cmp_mask_pos); i++) {
			ranges[j | i].len = tot_size - ranges[i].len;
			if (ranges[i].len != tot_size && ranges[i].start == 0x000000)
				ranges[j | i].start = ranges[i].len * 1024;
		}
	}

	return ranges;
}

// TODO(hatim): Make static (?)
char get_cmp(struct flashctx *flash) {
	int pos_cmp = pos_bit(flash, CMP);
	if (pos_cmp == -1)
		return -1;
	else {
		enum status_register_num SRn = pos_cmp / 8;
		pos_cmp -= SRn * 8;
		uint8_t cmp_mask = 1 << pos_cmp;
		uint8_t status = flash->chip->status_register->read(flash, SRn);
		return status & cmp_mask;
	}
}

// TODO(hatim): Make static (?)
// TODO(hatim): Change type of cmp to enum bit_state
int set_cmp(struct flashctx *flash, uint8_t cmp) {
	int pos_cmp = pos_bit(flash, CMP);
	if (pos_cmp == -1) {
		msg_cdbg("Chip does not have CMP bit!\n");
		msg_cerr("%s failed\n", __func__);
		return -1;
	} else {
		enum status_register_num SRn = pos_cmp / 8;
		pos_cmp -= SRn * 8;
		uint8_t cmp_mask = 1 << pos_cmp;
		uint8_t status = flash->chip->status_register->read(flash, SRn);
		status = ((status & ~cmp_mask) | ((cmp ? 1 : 0) << pos_cmp)) & 0xff;
		/* FIXME: Verify whether CMP was indeed written. */
		return flash->chip->status_register->write(flash, SRn, status);
	}
}

/* Wrapper for flash->chip->wp->range_table(). Returns the WP range table member of wp,
 * or, generates (then stores for later use) and returns one using range_table member
 * of wp. */
struct range *range_table_global(struct flashctx *flash) {
	if (flash->chip->wp->ranges) {
		return flash->chip->wp->ranges;
	} else {
		if (wp_range_table_id != flash->chip->model_id) {
			wp_range_table = flash->chip->wp->range_table(flash);
			wp_range_table_id = flash->chip->model_id;
		}
		return wp_range_table;
	}
}

/* Return BP bit mask. */
uint32_t bp_bitmask_generic(struct flashctx *flash) {
	/* The following code relies on the assumption that BP bits are present
	 * in continuity (back-to-back) in the first status register. Correct
	 * bitmask will NOT be returned if, say TB bit, is separately located
	 * from the rest of the BP bits. */
	unsigned char bitmask = 0x00;
	enum status_register_bit bits[] = { BP0, BP1, BP2, BP3, BP4, SEC, TB };

	for (int i = 0; i < ARRAY_SIZE(bits); i++) {
		if (pos_bit(flash, bits[i]) != -1)
			bitmask |= 1 << pos_bit(flash, bits[i]);
	}
	return bitmask;
}

/* Given BP bits' configuration, return the corresponding range.
 * For chips with a CMP bit, read the CMP value and automatically adjust. */
// TODO(hatim): Change type of bp_config to uint8_t
struct range *bp_to_range(struct flashctx *flash, unsigned char bp_config) {
	if (pos_bit(flash, CMP) == -1) {
		return &(range_table_global(flash)[bp_config]);
	} else {
		uint32_t cmp_mask = (flash->chip->wp->bp_bitmask(flash) >> pos_bit(flash, BP0)) + 1;
		cmp_mask = (get_cmp(flash) ? 0xffff : 0x0000) & cmp_mask;
		return &(range_table_global(flash)[cmp_mask | bp_config]);
	}
}

/* Given WP range, return BP bits' configuration (or -1, if range is invalid).
 * FIXME(hatim): This does NOT handle CMP bit automatically (yet) */
int range_to_bp_bitfield(struct flashctx *flash, uint32_t start, uint32_t len) {
	struct range *table = range_table_global(flash);
	int limit = 0, bitmask = flash->chip->wp->bp_bitmask(flash);

	for (int i = 7; i >= 0; i--)
		if (bitmask & (1 << i))
			limit++;
	limit = 1 << limit;
	for (int i = 0; i < limit * ((pos_bit(flash, CMP) == -1) ? 1 : 2); i++) {
		if (table[i].start == start && table[i].len == len)
			return i;
	}
	return -1;
}

int print_range_generic(struct flashctx *flash) {
	uint8_t status = flash->chip->status_register->read(flash, SR1);
	uint8_t bp_config = (status & flash->chip->wp->bp_bitmask(flash)) >> pos_bit(flash, BP0);

	struct range *range = bp_to_range(flash, bp_config);

	if (range->len == flash->chip->total_size)
		msg_cdbg("Entire memory range is protected against erases and writes\n");
	else if (range->len != 0)
		msg_cdbg("%d kB, starting from address 0x%06x, is protected against erases and writes\n",
			range->len, range->start);
	else
		msg_cdbg("Entire memory range is open to erases and writes\n");
	return 0;
}

int print_table_generic(struct flashctx *flash) {
	uint32_t bitmask = flash->chip->wp->bp_bitmask(flash);
	int limit = 0, pos_cmp = pos_bit(flash, CMP);
	/* FIXME: The following relies on the assumption that the entire
	 * BP bitmask (BP0..4, SEC, TB) is spread across only the first
	 * status register, hence 8 iterations for the following loop. */
	if (pos_cmp != -1)
		msg_cdbg("%s\t", statreg_bit_desc[CMP][0]);
	for (int i = 7; i >= 0; i--) {
		if (bitmask & (1 << i)) {
			limit++;
			msg_cdbg("%s\t", statreg_bit_desc[flash->chip->status_register->layout[SR1][i]][0]);
		}
	}
	msg_cdbg("START ADDR\tLENGTH (kB)\n");

	limit = 1 << limit;
	uint8_t j = (limit == 32) ? 2 : (limit == 16) ? 1 : 0;
	struct range *table = range_table_global(flash);

	for (int i = 0; i < limit * ((pos_cmp == -1) ? 1 : 2); i++)
		msg_cdbg("%s%s\t0x%06x\t%6d\n",
			(pos_cmp == -1) ? "" : (i >= limit) ? "1\t" : "0\t",
			bit_string[j][(i >= limit) ? i - limit : i],
			table[i].start, table[i].len);
	return 0;
}

/* Given a range, set the corresponding BP and CMP bit (if present) in the status
 * register. If range is invalid, return -1 and abort writing to status register. */
int set_range_generic(struct flashctx *flash, uint32_t start, uint32_t len) {
	int bitfield = range_to_bp_bitfield(flash, start, len);
	if (bitfield == -1) {
		msg_cdbg("Invalid block protection range start=0x%06x, len=%d kB\n", start, len);
		msg_cerr("%s failed\n", __func__);
		return -1;
	}

	int status = flash->chip->status_register->read(flash, SR1);
	int bitmask = flash->chip->wp->bp_bitmask(flash), result;

	status = (((bitfield << pos_bit(flash, BP0)) & bitmask) | (status & ~bitmask)) & 0xff;
	result = flash->chip->status_register->write(flash, SR1, (uint8_t)status);
	if (pos_bit(flash, CMP) != -1) {
		int cmp_mask = (flash->chip->wp->bp_bitmask(flash) >> pos_bit(flash, BP0)) + 1;
		result &= set_cmp(flash, bitfield & cmp_mask);
	}
	if (result)
		msg_cerr("%s failed\n", __func__);
	// FIXME(hatim): Perform a read to verify BP bitfield
	return result;
}

/* TODO: This should eventually replace functionality of (almost all) variants of blockprotect
 * from spi25_statusreg.c */
int disable_generic(struct flashctx *flash) {
	/* The following code assumes that all of the BP and modifier bits (SEC, TB) are present
	 * only in the first status register, and consequently the BP bitmask only masks bits
	 * from the first status register. */
	uint32_t bitmask = flash->chip->wp->bp_bitmask(flash);
	uint8_t status_sr1 = flash->chip->status_register->read(flash, SR1);
	int result;
	if (!(status_sr1 & bitmask)) {
		msg_cdbg("No block protection is in effect\n");
		return 0;
	}

	msg_cdbg("Some block protection is in effect, trying to disable...\n");
	switch (flash->chip->status_register->get_wp_mode(flash)) {
	case WP_MODE_HARDWARE_PROTECTED:
	case WP_MODE_POWER_CYCLE:
	case WP_MODE_PERMANENT:
		msg_cdbg("Write protection mode for status register(s) disallows writing to it, disabling failed\n");
		msg_cerr("%s failed\n", __func__);
		return 1;
	case WP_MODE_INVALID:
		msg_cdbg("Invalid write protection mode for status register(s), disabling failed\n");
		msg_cerr("%s failed\n", __func__);
		return 1;
	default:
		break;
	}
	/* It is observed that usually unsetting BP[0..2] corresponds to NO protection. We could use
	 * set_range with range (start=0, len=0) but use of the aformentioned observation avoids
	 * any overheads. */
	status_sr1 = (status_sr1 & ~bitmask) & 0xff;
	result = flash->chip->status_register->write(flash, SR1, (uint8_t)status_sr1);
	if (result) {
		msg_cerr("%s failed\n", __func__);
		return result;
	}
	status_sr1 = flash->chip->status_register->read(flash, SR1);
	if (status_sr1 & bitmask) {
		msg_cdbg("Could not write configuration to status register to disable block protection\n");
		msg_cerr("%s failed\n", __func__);
		return 1;
	}
	msg_cdbg("Disabled any block protection in effect\n");
	return result;
}

/* === Chip specific range_table generators === */
/* === AMIC === */
/* A25L032 */
struct range *a25l032_range_table(struct flashctx *flash) {
	struct range *table = sec_block_range_pattern(flash);
	/* CMP=0 */
	table[0x16].start = 0x3f0000;
	table[0x16].len = 64;
	table[0x1e].start = 0x000000;
	table[0x1e].len = 64;
	/* CMP=1 */
	table[1 << 5 | 0x16].start = 0x000000;
	table[1 << 5 | 0x16].len = 4032;
	table[1 << 5 | 0x1e].start = 0x010000;
	table[1 << 5 | 0x1e].len = 4032;

	return table;
}
