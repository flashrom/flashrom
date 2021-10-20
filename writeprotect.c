/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Google Inc.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flash.h"
#include "libflashrom.h"
#include "chipdrivers.h"
#include "writeprotect.h"

/** Read and extract a single bit from the chip's registers */
static enum flashrom_wp_result read_bit(uint8_t *value, bool *present, struct flashctx *flash, struct reg_bit_info bit)
{
	*present = bit.reg != INVALID_REG;
	if (*present) {
		if (spi_read_register(flash, bit.reg, value))
			return FLASHROM_WP_ERR_READ_FAILED;
		*value = (*value >> bit.bit_index) & 1;
	} else {
		/* Zero bit, it may be used by compare_ranges(). */
		*value = 0;
	}

	return FLASHROM_WP_OK;
}

/** Read all WP configuration bits from the chip's registers. */
static enum flashrom_wp_result read_wp_bits(struct wp_bits *bits, struct flashctx *flash)
{
	/*
	 * For each WP bit that is included in the chip's register layout, read
	 * the register that contains it, extracts the bit's value, and assign
	 * it to the appropriate field in the wp_bits structure.
	 */
	const struct reg_bit_map *bit_map = &flash->chip->reg_bits;
	bool ignored;
	size_t i;
	enum flashrom_wp_result ret;

	ret = read_bit(&bits->tb,  &bits->tb_bit_present,  flash, bit_map->tb);
	if (ret != FLASHROM_WP_OK)
		return ret;

	ret = read_bit(&bits->sec, &bits->sec_bit_present, flash, bit_map->sec);
	if (ret != FLASHROM_WP_OK)
		return ret;

	ret = read_bit(&bits->cmp, &bits->cmp_bit_present, flash, bit_map->cmp);
	if (ret != FLASHROM_WP_OK)
		return ret;

	ret = read_bit(&bits->srp, &bits->srp_bit_present, flash, bit_map->srp);
	if (ret != FLASHROM_WP_OK)
		return ret;

	ret = read_bit(&bits->srl, &bits->srl_bit_present, flash, bit_map->srl);
	if (ret != FLASHROM_WP_OK)
		return ret;

	for (i = 0; i < ARRAY_SIZE(bits->bp); i++) {
		if (bit_map->bp[i].reg == INVALID_REG)
			break;

		bits->bp_bit_count = i + 1;
		ret = read_bit(&bits->bp[i], &ignored, flash, bit_map->bp[i]);
		if (ret != FLASHROM_WP_OK)
			return ret;
	}

	return ret;
}

/** Helper function for write_wp_bits(). */
static void set_reg_bit(
		uint8_t *reg_values, uint8_t *write_masks,
		struct reg_bit_info bit, uint8_t value)
{
	if (bit.reg != INVALID_REG) {
		reg_values[bit.reg] |= value << bit.bit_index;
		write_masks[bit.reg] |= 1 << bit.bit_index;
	}
}

/** Write WP configuration bits to the flash's registers. */
static enum flashrom_wp_result write_wp_bits(struct flashctx *flash, struct wp_bits bits)
{
	size_t i;
	const struct reg_bit_map *reg_bits = &flash->chip->reg_bits;

	/* Convert wp_bits to register values and write masks */
	uint8_t reg_values[MAX_REGISTERS] = {0};
	uint8_t write_masks[MAX_REGISTERS] = {0};

	for (i = 0; i < bits.bp_bit_count; i++)
		set_reg_bit(reg_values, write_masks, reg_bits->bp[i], bits.bp[i]);

	set_reg_bit(reg_values, write_masks, reg_bits->tb,  bits.tb);
	set_reg_bit(reg_values, write_masks, reg_bits->sec, bits.sec);
	set_reg_bit(reg_values, write_masks, reg_bits->cmp, bits.cmp);
	set_reg_bit(reg_values, write_masks, reg_bits->srp, bits.srp);
	set_reg_bit(reg_values, write_masks, reg_bits->srl, bits.srl);

	/* Write each register */
	for (enum flash_reg reg = STATUS1; reg < MAX_REGISTERS; reg++) {
		if (!write_masks[reg])
			continue;

		uint8_t value;
		if (spi_read_register(flash, reg, &value))
			return FLASHROM_WP_ERR_READ_FAILED;

		value = (value & ~write_masks[reg]) | (reg_values[reg] & write_masks[reg]);

		if (spi_write_register(flash, reg, value))
			return FLASHROM_WP_ERR_WRITE_FAILED;
	}

	/* Verify each register */
	for (enum flash_reg reg = STATUS1; reg < MAX_REGISTERS; reg++) {
		if (!write_masks[reg])
			continue;

		uint8_t value;
		if (spi_read_register(flash, reg, &value))
			return FLASHROM_WP_ERR_READ_FAILED;

		uint8_t actual = value & write_masks[reg];
		uint8_t expected = reg_values[reg] & write_masks[reg];

		if (actual != expected)
			return FLASHROM_WP_ERR_VERIFY_FAILED;
	}

	return FLASHROM_WP_OK;
}

static bool chip_supported(struct flashctx *flash)
{
	return false;
}

enum flashrom_wp_result wp_read_cfg(struct flashrom_wp_cfg *cfg, struct flashctx *flash)
{
	struct wp_bits bits;
	enum flashrom_wp_result ret = FLASHROM_WP_OK;

	if (!chip_supported(flash))
		ret = FLASHROM_WP_ERR_CHIP_UNSUPPORTED;

	if (ret == FLASHROM_WP_OK)
		ret = read_wp_bits(&bits, flash);

	/* TODO: implement get_wp_range() and get_wp_mode() and call them */
	/*
	if (ret == FLASHROM_WP_OK)
		ret = get_wp_range(&cfg->range, flash, &bits);

	if (ret == FLASHROM_WP_OK)
		ret = get_wp_mode(&cfg->mode, &bits);
	*/

	return ret;
}

enum flashrom_wp_result wp_write_cfg(struct flashctx *flash, const struct flashrom_wp_cfg *cfg)
{
	struct wp_bits bits;
	enum flashrom_wp_result ret = FLASHROM_WP_OK;

	if (!chip_supported(flash))
		ret = FLASHROM_WP_ERR_CHIP_UNSUPPORTED;

	if (ret == FLASHROM_WP_OK)
		ret = read_wp_bits(&bits, flash);

	/* Set protection range */
	/* TODO: implement set_wp_range() and use it */
	/*
	if (ret == FLASHROM_WP_OK)
		ret = set_wp_range(&bits, flash, cfg->range);
	if (ret == FLASHROM_WP_OK)
		ret = write_wp_bits(flash, bits);
	*/

	/* Set protection mode */
	/* TODO: implement set_wp_mode() and use it */
	/*
	if (ret == FLASHROM_WP_OK)
		ret = set_wp_mode(&bits, cfg->mode);
	*/
	if (ret == FLASHROM_WP_OK)
		ret = write_wp_bits(flash, bits);

	return ret;
}

/** @} */ /* end flashrom-wp */
