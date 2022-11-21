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

#include "spi.h"
#include "flash.h"
#include "libflashrom.h"
#include "chipdrivers.h"
#include "writeprotect.h"
#include "programmer.h"

/*
 * Allow specialisation in opaque masters, such as ichspi hwseq, to r/w to status registers.
 */
static int wp_write_register(const struct flashctx *flash, enum flash_reg reg, uint8_t value)
{
	int ret;
	if ((flash->mst->buses_supported & BUS_PROG) && flash->mst->opaque.write_register) {
		ret = flash->mst->opaque.write_register(flash, reg, value);
	} else {
		ret = spi_write_register(flash, reg, value);
	}

	/* Writing SR1 should always be supported, ignore errors for other registers. */
	if (ret == SPI_INVALID_OPCODE && reg != STATUS1) {
		msg_pdbg("%s: write to register %d not supported by programmer, ignoring.\n", __func__, reg);
		ret = 0;
	}
	return ret;
}

static int wp_read_register(const struct flashctx *flash, enum flash_reg reg, uint8_t *value)
{
	int ret;
	if ((flash->mst->buses_supported & BUS_PROG) && flash->mst->opaque.read_register) {
		ret = flash->mst->opaque.read_register(flash, reg, value);
	} else {
		ret = spi_read_register(flash, reg, value);
	}

	/* Reading SR1 should always be supported, ignore errors for other registers. */
	if (ret == SPI_INVALID_OPCODE && reg != STATUS1) {
		msg_pdbg("%s: read from register %d not is supported by programmer, "
			  "writeprotect operations will assume it contains 0x00.\n", __func__, reg);
		*value = 0;
		ret = 0;
	}
	return ret;
}

/** Read and extract a single bit from the chip's registers */
static enum flashrom_wp_result read_bit(uint8_t *value, bool *present, struct flashctx *flash, struct reg_bit_info bit)
{
	*present = bit.reg != INVALID_REG;
	if (*present) {
		if (wp_read_register(flash, bit.reg, value))
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

	/*
	 * Write protection select bit (WPS) controls kind of write protection
	 * that is used by the chip. When set, BP bits are ignored and each
	 * block/sector has its own WP bit managed by special commands. When
	 * the bit is set and we can't change it, just bail out until
	 * implementation is extended to handle this kind of WP.
	 */
	if (bit_map->wps.reg != INVALID_REG && bit_map->wps.writability != RW) {
		bool wps_bit_present;
		uint8_t wps;

		ret = read_bit(&wps, &wps_bit_present, flash, bit_map->wps);
		if (ret != FLASHROM_WP_OK)
			return ret;

		if (wps_bit_present && wps)
			return FLASHROM_WP_ERR_UNSUPPORTED_STATE;
	}

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

/** Helper function for get_wp_bits_reg_values(). */
static void set_reg_bit(
		uint8_t *reg_values, uint8_t *bit_masks, uint8_t *write_masks,
		struct reg_bit_info bit, uint8_t value)
{
	if (bit.reg != INVALID_REG) {
		reg_values[bit.reg] |= value << bit.bit_index;
		bit_masks[bit.reg] |= 1 << bit.bit_index;

		/* Avoid RO and OTP bits causing a register update */
		if (bit.writability == RW)
			write_masks[bit.reg] |= 1 << bit.bit_index;
	}
}

/** Convert wp_bits to register values and write masks */
static void get_wp_bits_reg_values(
		uint8_t *reg_values, uint8_t *bit_masks, uint8_t *write_masks,
		const struct reg_bit_map *reg_bits, struct wp_bits bits)
{
	memset(reg_values, 0, sizeof(uint8_t) * MAX_REGISTERS);
	memset(bit_masks, 0, sizeof(uint8_t) * MAX_REGISTERS);
	memset(write_masks, 0, sizeof(uint8_t) * MAX_REGISTERS);

	for (size_t i = 0; i < bits.bp_bit_count; i++)
		set_reg_bit(reg_values, bit_masks, write_masks, reg_bits->bp[i], bits.bp[i]);

	set_reg_bit(reg_values, bit_masks, write_masks, reg_bits->tb,  bits.tb);
	set_reg_bit(reg_values, bit_masks, write_masks, reg_bits->sec, bits.sec);
	set_reg_bit(reg_values, bit_masks, write_masks, reg_bits->cmp, bits.cmp);
	set_reg_bit(reg_values, bit_masks, write_masks, reg_bits->srp, bits.srp);
	set_reg_bit(reg_values, bit_masks, write_masks, reg_bits->srl, bits.srl);
	/* Note: always setting WPS bit to zero until its fully supported. */
	set_reg_bit(reg_values, bit_masks, write_masks, reg_bits->wps, 0);
}

/** Write WP configuration bits to the flash's registers. */
static enum flashrom_wp_result write_wp_bits(struct flashctx *flash, struct wp_bits bits)
{
	uint8_t reg_values[MAX_REGISTERS];
	uint8_t bit_masks[MAX_REGISTERS];	/* masks of valid bits */
	uint8_t write_masks[MAX_REGISTERS];	/* masks of written bits */
	get_wp_bits_reg_values(reg_values, bit_masks, write_masks, &flash->chip->reg_bits, bits);

	/* Write each register whose value was updated */
	for (enum flash_reg reg = STATUS1; reg < MAX_REGISTERS; reg++) {
		if (!write_masks[reg])
			continue;

		uint8_t value;
		if (wp_read_register(flash, reg, &value))
			return FLASHROM_WP_ERR_READ_FAILED;

		/* Skip unnecessary register writes */
		uint8_t actual = value & write_masks[reg];
		uint8_t expected = reg_values[reg] & write_masks[reg];
		if (actual == expected)
			continue;

		value = (value & ~write_masks[reg]) | expected;

		if (wp_write_register(flash, reg, value))
			return FLASHROM_WP_ERR_WRITE_FAILED;
	}

	enum flashrom_wp_result ret = FLASHROM_WP_OK;
	/* Verify each register even if write to it was skipped */
	for (enum flash_reg reg = STATUS1; reg < MAX_REGISTERS; reg++) {
		if (!bit_masks[reg])
			continue;

		uint8_t value;
		if (wp_read_register(flash, reg, &value))
			return FLASHROM_WP_ERR_READ_FAILED;

		msg_cdbg2("%s: wp_verify reg:%u value:0x%x\n", __func__, reg, value);
		uint8_t actual = value & bit_masks[reg];
		uint8_t expected = reg_values[reg] & bit_masks[reg];

		if (actual != expected) {
			msg_cdbg("%s: wp_verify failed: reg:%u actual:0x%x expected:0x%x\n",
				 __func__, reg, actual, expected);
			ret = FLASHROM_WP_ERR_VERIFY_FAILED;
		}
	}

	return ret;
}

static decode_range_func_t *lookup_decode_range_func_ptr(const struct flashchip *chip)
{
	switch (chip->decode_range) {
		case DECODE_RANGE_SPI25: return &decode_range_spi25;
		case DECODE_RANGE_SPI25_64K_BLOCK: return &decode_range_spi25_64k_block;
		case DECODE_RANGE_SPI25_BIT_CMP: return &decode_range_spi25_bit_cmp;
		case DECODE_RANGE_SPI25_2X_BLOCK: return &decode_range_spi25_2x_block;
	/* default: total function, 0 indicates no decode range function set. */
		case NO_DECODE_RANGE_FUNC: return NULL;
	};

	return NULL;
}


/** Get the range selected by a WP configuration. */
static enum flashrom_wp_result get_wp_range(struct wp_range *range, struct flashctx *flash, const struct wp_bits *bits)
{
	decode_range_func_t *decode_range = lookup_decode_range_func_ptr(flash->chip);
	if (decode_range == NULL)
		return FLASHROM_WP_ERR_OTHER;

	decode_range(&range->start, &range->len, bits, flashrom_flash_getsize(flash));
	return FLASHROM_WP_OK;
}

/** Write protect bit values and the range they will activate. */
struct wp_range_and_bits {
	struct wp_bits bits;
	struct wp_range range;
};

/**
 * Comparator used for sorting ranges in get_ranges_and_wp_bits().
 *
 * Ranges are ordered by these attributes, in decreasing significance:
 *   (range length, range start, cmp bit, sec bit, tb bit, bp bits)
 */
static int compare_ranges(const void *aa, const void *bb)
{
	const struct wp_range_and_bits
		*a = (const struct wp_range_and_bits *)aa,
		*b = (const struct wp_range_and_bits *)bb;

	int ord = 0;

	if (ord == 0)
		ord = a->range.len - b->range.len;

	if (ord == 0)
		ord = a->range.start - b->range.start;

	if (ord == 0)
		ord = a->bits.cmp - b->bits.cmp;

	if (ord == 0)
		ord = a->bits.sec - b->bits.sec;

	if (ord == 0)
		ord = a->bits.tb  - b->bits.tb;

	for (int i = a->bits.bp_bit_count - 1; i >= 0; i--) {
		if (ord == 0)
			ord = a->bits.bp[i] - b->bits.bp[i];
	}

	return ord;
}

static bool can_write_bit(const struct reg_bit_info bit)
{
	/*
	 * TODO: check if the programmer supports writing the register that the
	 * bit is in. For example, some chipsets may only allow SR1 to be
	 * written.
	 */

	return bit.reg != INVALID_REG && bit.writability == RW;
}

/**
 * Enumerate all protection ranges that the chip supports and that are able to
 * be activated, given limitations such as OTP bits or programmer-enforced
 * restrictions. Returns a list of deduplicated wp_range_and_bits structures.
 *
 * Allocates a buffer that must be freed by the caller with free().
 */
static enum flashrom_wp_result get_ranges_and_wp_bits(struct flashctx *flash, struct wp_bits bits, struct wp_range_and_bits **ranges, size_t *count)
{
	const struct reg_bit_map *reg_bits = &flash->chip->reg_bits;
	/*
	 * Create a list of bits that affect the chip's protection range in
	 * range_bits. Each element is a pointer to a member of the wp_bits
	 * structure that will be modified.
	 *
	 * Some chips have range bits that cannot be changed (e.g. MX25L6473E
	 * has a one-time programmable TB bit). Rather than enumerating all
	 * possible values for unwritable bits, just read their values from the
	 * chip to ensure we only enumerate ranges that are actually available.
	 */
	uint8_t *range_bits[ARRAY_SIZE(bits.bp) + 1 /* TB */ + 1 /* SEC */ + 1 /* CMP */];
	size_t bit_count = 0;

	for (size_t i = 0; i < ARRAY_SIZE(bits.bp); i++) {
		if (can_write_bit(reg_bits->bp[i]))
			range_bits[bit_count++] = &bits.bp[i];
	}

	if (can_write_bit(reg_bits->tb))
		range_bits[bit_count++] = &bits.tb;

	if (can_write_bit(reg_bits->sec))
		range_bits[bit_count++] = &bits.sec;

	if (can_write_bit(reg_bits->cmp))
		range_bits[bit_count++] = &bits.cmp;

	/* Allocate output buffer */
	*count = 1 << bit_count;
	*ranges = calloc(*count, sizeof(struct wp_range_and_bits));

	/* TODO: take WPS bit into account. */

	for (size_t range_index = 0; range_index < *count; range_index++) {
		/*
		 * Extract bits from the range index and assign them to members
		 * of the wp_bits structure. The loop bounds ensure that all
		 * bit combinations will be enumerated.
		 */
		for (size_t i = 0; i < bit_count; i++)
			*range_bits[i] = (range_index >> i) & 1;

		struct wp_range_and_bits *output = &(*ranges)[range_index];

		output->bits = bits;
		enum flashrom_wp_result ret = get_wp_range(&output->range, flash, &bits);
		if (ret != FLASHROM_WP_OK) {
			free(*ranges);
			return ret;
		}

		/* Debug: print range bits and range */
		msg_gspew("Enumerated range: ");
		if (bits.cmp_bit_present)
			msg_gspew("CMP=%u ", bits.cmp);
		if (bits.sec_bit_present)
			msg_gspew("SEC=%u ", bits.sec);
		if (bits.tb_bit_present)
			msg_gspew("TB=%u ", bits.tb);
		for (size_t i = 0; i < bits.bp_bit_count; i++) {
			size_t j = bits.bp_bit_count - i - 1;
			msg_gspew("BP%zu=%u ", j, bits.bp[j]);
		}
		msg_gspew(" start=0x%08zx length=0x%08zx\n",
			  output->range.start, output->range.len);
	}

	/* Sort ranges. Ensures consistency if there are duplicate ranges. */
	qsort(*ranges, *count, sizeof(struct wp_range_and_bits), compare_ranges);

	/* Remove duplicates */
	size_t output_index = 0;
	struct wp_range *last_range = NULL;

	for (size_t i = 0; i < *count; i++) {
		bool different_to_last =
			(last_range == NULL) ||
			((*ranges)[i].range.start != last_range->start) ||
			((*ranges)[i].range.len   != last_range->len);

		if (different_to_last) {
			/* Move range to the next free position */
			(*ranges)[output_index] = (*ranges)[i];
			output_index++;
			/* Keep track of last non-duplicate range */
			last_range = &(*ranges)[i].range;
		}
	}
	/* Reduce count to only include non-duplicate ranges */
	*count = output_index;

	return FLASHROM_WP_OK;
}

static bool ranges_equal(struct wp_range a, struct wp_range b)
{
	return (a.start == b.start) && (a.len == b.len);
}

/*
 * Modify the range-related bits in a wp_bits structure so they select a given
 * protection range. Bits that control the protection mode are not changed.
 */
static int set_wp_range(struct wp_bits *bits, struct flashctx *flash, const struct wp_range range)
{
	struct wp_range_and_bits *ranges = NULL;
	size_t count;

	enum flashrom_wp_result ret = get_ranges_and_wp_bits(flash, *bits, &ranges, &count);
	if (ret != FLASHROM_WP_OK)
		return ret;

	/* Search for matching range */
	ret = FLASHROM_WP_ERR_RANGE_UNSUPPORTED;
	for (size_t i = 0; i < count; i++) {

		if (ranges_equal(ranges[i].range, range)) {
			*bits = ranges[i].bits;
			ret = 0;
			break;
		}
	}

	free(ranges);

	return ret;
}

/** Get the mode selected by a WP configuration. */
static int get_wp_mode(enum flashrom_wp_mode *mode, const struct wp_bits *bits)
{
	const enum flashrom_wp_mode wp_modes[2][2] = {
		{
			FLASHROM_WP_MODE_DISABLED,	/* srl=0, srp=0 */
			FLASHROM_WP_MODE_HARDWARE,	/* srl=0, srp=1 */
		}, {
			FLASHROM_WP_MODE_POWER_CYCLE,	/* srl=1, srp=0 */
			FLASHROM_WP_MODE_PERMANENT,	/* srl=1, srp=1 */
		},
	};

	*mode = wp_modes[bits->srl][bits->srp];

	return FLASHROM_WP_OK;
}

/** Modify a wp_bits structure such that it will select a specified protection mode. */
static int set_wp_mode(struct wp_bits *bits, const enum flashrom_wp_mode mode)
{
	switch (mode) {
	case FLASHROM_WP_MODE_DISABLED:
		bits->srl = 0;
		bits->srp = 0;
		return FLASHROM_WP_OK;

	case FLASHROM_WP_MODE_HARDWARE:
		if (!bits->srp_bit_present)
			return FLASHROM_WP_ERR_CHIP_UNSUPPORTED;

		bits->srl = 0;
		bits->srp = 1;
		return FLASHROM_WP_OK;

	case FLASHROM_WP_MODE_POWER_CYCLE:
	case FLASHROM_WP_MODE_PERMANENT:
	default:
		/*
		 * Don't try to enable power cycle or permanent protection for
		 * now. Those modes may be possible to activate on some chips,
		 * but they are usually unavailable by default or require special
		 * commands to activate.
		 */
		return FLASHROM_WP_ERR_MODE_UNSUPPORTED;
	}
}

static bool chip_supported(struct flashctx *flash)
{
	return (flash->chip != NULL) && (flash->chip->decode_range != NO_DECODE_RANGE_FUNC);
}


bool wp_operations_available(struct flashrom_flashctx *flash)
{
	return (flash->mst->buses_supported & BUS_SPI) ||
		((flash->mst->buses_supported & BUS_PROG) &&
			flash->mst->opaque.read_register &&
			flash->mst->opaque.write_register);
}

enum flashrom_wp_result wp_read_cfg(struct flashrom_wp_cfg *cfg, struct flashctx *flash)
{
	struct wp_bits bits;
	enum flashrom_wp_result ret = FLASHROM_WP_OK;

	if (!chip_supported(flash))
		ret = FLASHROM_WP_ERR_CHIP_UNSUPPORTED;

	if (ret == FLASHROM_WP_OK)
		ret = read_wp_bits(&bits, flash);

	if (ret == FLASHROM_WP_OK)
		ret = get_wp_range(&cfg->range, flash, &bits);

	if (ret == FLASHROM_WP_OK)
		ret = get_wp_mode(&cfg->mode, &bits);

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
	if (ret == FLASHROM_WP_OK)
		ret = set_wp_range(&bits, flash, cfg->range);
	if (ret == FLASHROM_WP_OK)
		ret = write_wp_bits(flash, bits);

	/* Set protection mode */
	if (ret == FLASHROM_WP_OK)
		ret = set_wp_mode(&bits, cfg->mode);
	if (ret == FLASHROM_WP_OK)
		ret = write_wp_bits(flash, bits);

	return ret;
}

enum flashrom_wp_result wp_get_available_ranges(struct flashrom_wp_ranges **list, struct flashrom_flashctx *flash)
{
	struct wp_bits bits;
	struct wp_range_and_bits *range_pairs = NULL;
	size_t count;

	if (!chip_supported(flash))
		return FLASHROM_WP_ERR_CHIP_UNSUPPORTED;

	enum flashrom_wp_result ret = read_wp_bits(&bits, flash);
	if (ret != FLASHROM_WP_OK)
		return ret;

	ret = get_ranges_and_wp_bits(flash, bits, &range_pairs, &count);
	if (ret != FLASHROM_WP_OK)
		return ret;

	*list = calloc(1, sizeof(struct flashrom_wp_ranges));
	struct wp_range *ranges = calloc(count, sizeof(struct wp_range));

	if (!(*list) || !ranges) {
		free(*list);
		free(ranges);
		ret = FLASHROM_WP_ERR_OTHER;
		goto out;
	}
	(*list)->count = count;
	(*list)->ranges = ranges;

	for (size_t i = 0; i < count; i++)
		ranges[i] = range_pairs[i].range;

out:
	free(range_pairs);
	return ret;
}

enum flashrom_wp_result wp_cfg_to_reg_values(
		uint8_t *reg_values, uint8_t *bit_masks, uint8_t *write_masks,
		struct flashctx *flash, const struct flashrom_wp_cfg *cfg)
{
	struct wp_bits bits;

	if (!chip_supported(flash))
		return FLASHROM_WP_ERR_CHIP_UNSUPPORTED;

	enum flashrom_wp_result ret = read_wp_bits(&bits, flash);
	if (ret != FLASHROM_WP_OK)
		return ret;

	/* Set protection range */
	ret = set_wp_range(&bits, flash, cfg->range);
	if (ret != FLASHROM_WP_OK)
		return ret;

	/* Set protection mode */
	ret = set_wp_mode(&bits, cfg->mode);
	if (ret != FLASHROM_WP_OK)
		return ret;

	get_wp_bits_reg_values(reg_values, bit_masks, write_masks, &flash->chip->reg_bits, bits);

	return FLASHROM_WP_OK;
}
