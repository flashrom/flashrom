/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2016 secunet Security Networks AG
 * (written by Thomas Heijligen <thomas.heijligen@secunet.com>)
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
