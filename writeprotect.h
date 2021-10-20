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

#define MAX_BP_BITS 4

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

#endif /* !__WRITEPROTECT_H__ */
