/*
 * This file is part of the flashrom project.
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2016 secunet Security Networks AG and Thomas Heijligen <thomas.heijligen@secunet.com>
 */

#include "platform.h"

/*
 * macro to return endian aware read function
 *
 * `___read(le, 8)`
 * 	expands to
 * `uint8_t read_le8 (const void *const base, const size_t offset)
 *  { return le_to_cpu8 (*(uint8_t *)((uintptr_t)base + offset)); }`
 */
#define ___read(endian, bits) \
	uint##bits##_t read_##endian##bits (const void *const base, const size_t offset) \
	{ return endian##_to_cpu##bits (*(uint##bits##_t *)((uintptr_t)base + offset)); }

/* read value from base at offset in little endian */
___read(le,  8)
___read(le, 16)
___read(le, 32)
___read(le, 64)

/* read value from base at offset in big endian */
___read(be,  8)
___read(be, 16)
___read(be, 32)
___read(be, 64)
