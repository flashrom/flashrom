/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2005-2008 coresystems GmbH
 * (Written by Stefan Reinauer <stepan@coresystems.de> for coresystems GmbH)
 * Copyright (C) 2011-2013 Stefan Tauner
 * Copyright (C) 2016 secunet Security Networks AG
 * (Written by Nico Huber <nico.huber@secunet.com> for secunet)
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

#ifndef __LAYOUT_H__
#define __LAYOUT_H__ 1

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Types and macros regarding the maximum flash space size supported by generic code. */
typedef uint32_t chipoff_t; /* Able to store any addressable offset within a supported flash memory. */
typedef uint32_t chipsize_t; /* Able to store the number of bytes of any supported flash memory. */
#define FL_MAX_CHIPOFF_BITS (24)
#define FL_MAX_CHIPOFF ((chipoff_t)(1ULL<<FL_MAX_CHIPOFF_BITS)-1)
#define PRIxCHIPOFF "06"PRIx32
#define PRIuCHIPSIZE PRIu32

#define MAX_ROMLAYOUT	32

struct romentry {
	chipoff_t start;
	chipoff_t end;
	bool included;
	char name[256];
};

struct flashrom_layout {
	/* entries store the entries specified in a layout file and associated run-time data */
	struct romentry *entries;
	/* the number of successfully parsed entries */
	size_t num_entries;
};

struct single_layout {
	struct flashrom_layout base;
	struct romentry entry;
};

struct flashrom_layout *get_global_layout(void);
struct flashrom_flashctx;
const struct flashrom_layout *get_layout(const struct flashrom_flashctx *const flashctx);

int process_include_args(struct flashrom_layout *);
const struct romentry *layout_next_included_region(const struct flashrom_layout *, chipoff_t);

#endif				/* !__LAYOUT_H__ */
