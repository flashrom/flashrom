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

/*
 * Protection range calculation that works with many common SPI flash chips.
 */
void decode_range_spi25(size_t *start, size_t *len, const struct wp_bits *bits, size_t chip_len)
{
	/* Interpret BP bits as an integer */
	size_t bp = 0;
	size_t bp_max = 0;

	for (size_t i = 0; i < bits->bp_bit_count; i++) {
		bp |= bits->bp[i] << i;
		bp_max |= 1 << i;
	}

	if (bp == 0) {
		/* Special case: all BP bits are 0 => no write protection */
		*len = 0;
	} else if (bp == bp_max) {
		/* Special case: all BP bits are 1 => full write protection */
		*len = chip_len;
	} else {
		/*
		 * Usual case: the BP bits encode a coefficient in the form
		 * `coeff = 2 ** (bp - 1)`.
		 *
		 * The range's length is given by multiplying the coefficient
		 * by a base unit, usually a 4K sector or a 64K block.
		 */

		size_t coeff     = 1 << (bp - 1);
		size_t max_coeff = 1 << (bp_max - 2);

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
			 *
			 * With very large chips, the 'block' size can be
			 * larger than 64K. This occurs when a larger block
			 * size is needed so that half the chip can be
			 * protected by the maximum possible coefficient.
			 */
			size_t min_block_len = chip_len / 2 / max_coeff;
			size_t block_len = max(min_block_len, default_block_len);

			*len = min(block_len * coeff, chip_len);
		}
	}

	/* Apply TB bit */
	bool protect_top = bits->tb_bit_present ? (bits->tb == 0) : 1;

	/* Apply CMP bit */
	if (bits->cmp_bit_present && bits->cmp == 1) {
		*len = chip_len - *len;
		protect_top = !protect_top;
	}

	/* Calculate start address, ensuring that empty ranges start at 0 */
	if (protect_top && *len > 0)
		*start = chip_len - *len;
	else
		*start = 0;
}
