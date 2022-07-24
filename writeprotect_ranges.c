/*
 * This file is part of the flashrom project.
 *
 * Copyright 2021 Google LLC
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

#include "writeprotect.h"
#include "chipdrivers.h"

static void decode_range_generic(size_t *start, size_t *len, const struct wp_bits *bits, size_t chip_len,
				 bool fixed_block_len, bool apply_cmp_to_bp, int coeff_offset)
{
	const bool cmp = bits->cmp_bit_present && bits->cmp == 1;

	/* Interpret BP bits as an integer */
	size_t bp = 0;
	size_t bp_max = 0;

	for (size_t i = 0; i < bits->bp_bit_count; i++) {
		bp |= bits->bp[i] << i;
		bp_max |= 1 << i;
	}

	/*
	 * Most chips: the CMP bit only negates the range.
	 *
	 * Some MX chips: the CMP bit negates the BP bits and the range.
	 * (CMP bit is often the MSB BP bit in such chips.)
	 */
	if (cmp && apply_cmp_to_bp)
		bp ^= bp_max;

	if (bp == 0) {
		/* Special case: all BP bits are 0 => no write protection */
		*len = 0;
	} else if (bp == bp_max) {
		/* Special case: all BP bits are 1 => full write protection */
		*len = chip_len;
	} else {
		/*
		 * Usual case: the BP bits encode a coefficient in the form
		 * `coeff = 2 ** (bp - offset)` where `offset == 1`.
		 *
		 * The range's length is given by multiplying the coefficient
		 * by a base unit, usually a 4K sector or a 64K block.
		 */

		size_t coeff     = 1 << (bp - coeff_offset);
		size_t max_coeff = 1 << (bp_max - coeff_offset - 1);

		size_t sector_len        = 4  * KiB;
		size_t default_block_len = 64 * KiB;

		if (bits->sec_bit_present && bits->sec == 1) {
			/*
			 * SEC=1, protect 4K sectors. Flash chips clamp the
			 * protection length at 32K, probably to avoid overlap
			 * with the SEC=0 case.
			 */
			*len = min(sector_len * coeff, default_block_len / 2);
		} else {
			/*
			 * SEC=0 or is not present, protect blocks.
			 */
			size_t block_len = default_block_len;

			/*
			 * With very large chips, the 'block' size can be
			 * larger than 64K. This occurs when a larger block
			 * size is needed so that half the chip can be
			 * protected by the maximum possible coefficient.
			 */
			if (!fixed_block_len) {
				size_t min_block_len = chip_len / 2 / max_coeff;
				block_len = max(min_block_len, default_block_len);
			}

			*len = min(block_len * coeff, chip_len);
		}
	}

	/* Apply TB bit */
	bool protect_top = bits->tb_bit_present ? (bits->tb == 0) : 1;

	/* Apply CMP bit */
	if (cmp) {
		*len = chip_len - *len;
		protect_top = !protect_top;
	}

	/* Calculate start address, ensuring that empty ranges start at 0 */
	if (protect_top && *len > 0)
		*start = chip_len - *len;
	else
		*start = 0;
}

/*
 * Protection range calculation that works with many common SPI flash chips.
 */
void decode_range_spi25(size_t *start, size_t *len, const struct wp_bits *bits, size_t chip_len)
{
	decode_range_generic(start, len, bits, chip_len,
			     /*fixed_block_len=*/false, /*apply_cmp_to_bp=*/false, /*coeff_offset=*/1);
}

/*
 * Do not adjust block size to be able to fill half of the chip.
 */
void decode_range_spi25_64k_block(size_t *start, size_t *len, const struct wp_bits *bits, size_t chip_len)
{
	decode_range_generic(start, len, bits, chip_len,
			     /*fixed_block_len=*/true, /*apply_cmp_to_bp=*/false, /*coeff_offset=*/1);
}

/*
 * Inverts BP bits when CMP is set and treats all ones in BP bits as a request to protect whole chip regardless
 * of the CMP bit.
 */
void decode_range_spi25_bit_cmp(size_t *start, size_t *len, const struct wp_bits *bits, size_t chip_len)
{
	decode_range_generic(start, len, bits, chip_len,
			     /*fixed_block_len=*/false, /*apply_cmp_to_bp=*/true, /*coeff_offset=*/1);
}

/*
 * This multiplies coefficient by 2.  To be used with chips which have more BP bits than needed, such that the
 * most significant BP bit effectively acts as "protect whole chip" flag.
 */
void decode_range_spi25_2x_block(size_t *start, size_t *len, const struct wp_bits *bits, size_t chip_len)
{
	decode_range_generic(start, len, bits, chip_len,
			     /*fixed_block_len=*/false, /*apply_cmp_to_bp=*/false, /*coeff_offset=*/0);
}
