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

#ifndef __WRITEPROTECT_H__
#define __WRITEPROTECT_H__ 1

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "libflashrom.h"

#define MAX_BP_BITS 4

/* Chip protection range: start address and length. */
struct wp_range {
        size_t start, len;
};

/* Generic description of a chip's write protection configuration. */
struct flashrom_wp_cfg {
        enum flashrom_wp_mode mode;
        struct wp_range range;
};

/* Collection of multiple write protection ranges. */
struct flashrom_wp_ranges {
	struct wp_range *ranges;
	size_t count;
};

/*
 * Description of a chip's write protection configuration.
 *
 * It allows most WP code to store and manipulate a chip's configuration
 * without knowing the exact layout of bits in the chip's status registers.
 */
struct wp_bits  {
	/* Status register protection bit (SRP) */
	bool srp_bit_present;
	uint8_t srp;

	/* Status register lock bit (SRL) */
	bool srl_bit_present;
	uint8_t srl;

	/* Complement bit (CMP) */
	bool cmp_bit_present;
	uint8_t cmp;

	/* Sector/block protection bit (SEC) */
	bool sec_bit_present;
	uint8_t sec;

	/* Top/bottom protection bit (TB) */
	bool tb_bit_present;
	uint8_t tb;

	/* Block protection bits (BP) */
	size_t bp_bit_count;
	uint8_t bp[MAX_BP_BITS];
};

struct flashrom_flashctx;

/* Write WP configuration to the chip */
enum flashrom_wp_result wp_write_cfg(struct flashrom_flashctx *, const struct flashrom_wp_cfg *);

/* Read WP configuration from the chip */
enum flashrom_wp_result wp_read_cfg(struct flashrom_wp_cfg *, struct flashrom_flashctx *);

/* Get a list of protection ranges supported by the chip */
enum flashrom_wp_result wp_get_available_ranges(struct flashrom_wp_ranges **, struct flashrom_flashctx *);

/* Checks if writeprotect functions can be used with the current flash/programmer */
bool wp_operations_available(struct flashrom_flashctx *);

/*
 * Converts a writeprotect config to register values and masks that indicate which register bits affect WP state.
 * reg_values, bit_masks, and write_masks must all have length of at least MAX_REGISTERS.
 */
enum flashrom_wp_result wp_cfg_to_reg_values(uint8_t *reg_values, uint8_t *bit_masks, uint8_t *write_masks, struct flashrom_flashctx *, const struct flashrom_wp_cfg *);

#endif /* !__WRITEPROTECT_H__ */
