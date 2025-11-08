/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2005-2008 coresystems GmbH (Written by Stefan Reinauer <stepan@coresystems.de> for coresystems GmbH)
 * SPDX-FileCopyrightText: 2011-2013 Stefan Tauner
 * SPDX-FileCopyrightText: 2016 secunet Security Networks AG (Written by Nico Huber <nico.huber@secunet.com> for secunet)
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

struct protected_ranges {
	int count;
	struct flash_region *ranges;
};

struct flashrom_layout;

struct layout_include_args;

struct flashrom_flashctx;
const struct flashrom_layout *get_default_layout(const struct flashrom_flashctx *);
const struct flashrom_layout *get_layout(const struct flashrom_flashctx *);

int layout_from_file(struct flashrom_layout **, const char *name);

int register_include_arg(struct layout_include_args **, const char *arg);
int process_include_args(struct flashrom_layout *, const struct layout_include_args *);
int check_include_args_filename(const struct layout_include_args *);
void cleanup_include_args(struct layout_include_args **);

const struct romentry *layout_next_included_region(const struct flashrom_layout *, chipoff_t);
const struct romentry *layout_next_included(const struct flashrom_layout *, const struct romentry *);
const struct romentry *layout_next(const struct flashrom_layout *, const struct romentry *);
int included_regions_overlap(const struct flashrom_layout *);
void prepare_layout_for_extraction(struct flashrom_flashctx *);
int layout_sanity_checks(const struct flashrom_flashctx *);
int check_for_unwritable_regions(const struct flashrom_flashctx *flash, unsigned int start, unsigned int len);
void get_flash_region(const struct flashrom_flashctx *flash, int addr, struct flash_region *region);
/*
 * Return chipset-level protections.
 * ranges parameter has to be freed by the caller with release_protected_ranges
 */
void get_protected_ranges(const struct flashrom_flashctx *flash, struct protected_ranges *ranges);
void release_protected_ranges(const struct flashrom_flashctx *flash, struct protected_ranges *ranges);

#endif /* !__LAYOUT_H__ */
