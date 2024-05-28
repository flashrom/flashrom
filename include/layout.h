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

#define MAX_ROMLAYOUT	128

struct flash_region {
	char *name;
	/*
	 * Note that because start and end are chipoff_t, end is an inclusive upper
	 * bound: the length of a region is (end - start + 1) bytes and it is
	 * impossible to represent a region with zero length.
	 */
	chipoff_t start;
	chipoff_t end;
	bool read_prot;
	bool write_prot;
};

struct romentry {
	struct romentry *next;

	bool included;
	char *file;

	struct flash_region region;
};

struct flashrom_layout;

struct layout_include_args;

struct flashrom_flashctx;
const struct flashrom_layout *get_default_layout(const struct flashrom_flashctx *);
const struct flashrom_layout *get_layout(const struct flashrom_flashctx *);

int layout_from_file(struct flashrom_layout **, const char *name);

int register_include_arg(struct layout_include_args **, const char *arg);
int process_include_args(struct flashrom_layout *, const struct layout_include_args *);
void cleanup_include_args(struct layout_include_args **);

const struct romentry *layout_next_included_region(const struct flashrom_layout *, chipoff_t);
const struct romentry *layout_next_included(const struct flashrom_layout *, const struct romentry *);
const struct romentry *layout_next(const struct flashrom_layout *, const struct romentry *);
int included_regions_overlap(const struct flashrom_layout *);
void prepare_layout_for_extraction(struct flashrom_flashctx *);
int layout_sanity_checks(const struct flashrom_flashctx *);
int check_for_unwritable_regions(const struct flashrom_flashctx *flash, unsigned int start, unsigned int len);
void get_flash_region(const struct flashrom_flashctx *flash, int addr, struct flash_region *region);

#endif /* !__LAYOUT_H__ */
