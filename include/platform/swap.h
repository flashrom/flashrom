/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2009 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2022 secunet Security Networks AG and Thomas Heijligen <thomas.heijligen@secunet.com>
 */

/*
 * inline functions and macros shared by endian conversion functions
 */

#ifndef ___SWAP_H___
#define ___SWAP_H___ 1

#include <stdint.h>

/* swap bytes */
/* OpenBSD has conflicting definitions for swapX, _swap_X and __swapX */
static inline uint8_t ___swap8(const uint8_t value)
{
	return  (value & (uint8_t)0xffU);
}

static inline uint16_t ___swap16(const uint16_t value)
{
	return  ((value & (uint16_t)0x00ffU) << 8) |
		((value & (uint16_t)0xff00U) >> 8);
}

static inline uint32_t ___swap32(const uint32_t value)
{
	return  ((value & (uint32_t)0x000000ffUL) << 24) |
		((value & (uint32_t)0x0000ff00UL) <<  8) |
		((value & (uint32_t)0x00ff0000UL) >>  8) |
		((value & (uint32_t)0xff000000UL) >> 24);
}

static inline uint64_t ___swap64(const uint64_t value)
{
	return  ((value & (uint64_t)0x00000000000000ffULL) << 56) |
		((value & (uint64_t)0x000000000000ff00ULL) << 40) |
		((value & (uint64_t)0x0000000000ff0000ULL) << 24) |
		((value & (uint64_t)0x00000000ff000000ULL) <<  8) |
		((value & (uint64_t)0x000000ff00000000ULL) >>  8) |
		((value & (uint64_t)0x0000ff0000000000ULL) >> 24) |
		((value & (uint64_t)0x00ff000000000000ULL) >> 40) |
		((value & (uint64_t)0xff00000000000000ULL) >> 56);
}

/*
 * macro to return the same value as passed.
 *
 * `___return_same(cpu_to_le, 8)`
 *	expands to
 * `uint8_t cpu_to_le8 (const uint8_t value) { return value; }`
 */
#define ___return_same(name, bits) \
	uint##bits##_t name##bits (const uint##bits##_t value) { return value; }

/*
 * macro to return the swapped value as passed.
 *
 * `___return_swapped(cpu_to_be, 8)`
 *	expands to
 * `uint8_t cpu_to_be8 (const uint8_t value) { return ___swap8 (value); }`
 */
#define ___return_swapped(name, bits) \
	uint##bits##_t name##bits (const uint##bits##_t value) { return ___swap##bits (value); }

#endif /* !___SWAP_H___ */
