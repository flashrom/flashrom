/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2005-2008 coresystems GmbH
 * (Written by Stefan Reinauer <stepan@coresystems.de> for coresystems GmbH)
 * Copyright (C) 2011-2013 Stefan Tauner
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "flash.h"
#include "programmer.h"
#include "layout.h"

struct flashrom_layout {
	struct romentry *head;
};

struct layout_include_args {
	char *name;
	char *file;
	struct layout_include_args *next;
};

const struct flashrom_layout *get_default_layout(const struct flashrom_flashctx *const flashctx)
{
	return flashctx->default_layout;
}

const struct flashrom_layout *get_layout(const struct flashrom_flashctx *const flashctx)
{
	if (flashctx->layout)
		return flashctx->layout;
	else
		return get_default_layout(flashctx);
}

static struct romentry *mutable_layout_next(
		const struct flashrom_layout *const layout, struct romentry *iterator)
{
	return iterator ? iterator->next : layout->head;
}

static struct romentry *_layout_entry_by_name(
		const struct flashrom_layout *const layout, const char *name)
{
	struct romentry *entry = NULL;
	if (!layout || !name)
		return NULL;
	while ((entry = mutable_layout_next(layout, entry))) {
		if (!strcmp(entry->region.name, name))
			return entry;
	}
	return NULL;
}

#ifndef __LIBPAYLOAD__
int layout_from_file(struct flashrom_layout **layout, const char *name)
{
	FILE *romlayout;
	char tempstr[256], tempname[256];
	int ret = 1;

	if (flashrom_layout_new(layout))
		return 1;

	romlayout = fopen(name, "r");

	if (!romlayout) {
		msg_gerr("ERROR: Could not open layout file (%s).\n",
			name);
		return -1;
	}

	while (!feof(romlayout)) {
		char *tstr1, *tstr2;

		if (2 != fscanf(romlayout, "%255s %255s\n", tempstr, tempname))
			continue;
#if 0
		// fscanf does not like arbitrary comments like that :( later
		if (tempstr[0] == '#') {
			continue;
		}
#endif
		tstr1 = strtok(tempstr, ":");
		tstr2 = strtok(NULL, ":");
		if (!tstr1 || !tstr2) {
			msg_gerr("Error parsing layout file. Offending string: \"%s\"\n", tempstr);
			goto _close_ret;
		}
		if (flashrom_layout_add_region(*layout,
				strtol(tstr1, NULL, 16), strtol(tstr2, NULL, 16), tempname))
			goto _close_ret;
	}
	ret = 0;

_close_ret:
	(void)fclose(romlayout);
	return ret;
}
#endif

static bool parse_include_args(const char *arg, char **name, char **file)
{
	char *colon;
	char *tmp_name;
	char *tmp_file = NULL; /* file is optional, so defaults to NULL */

	if (arg == NULL) {
		msg_gerr("<NULL> is a bad region name.\n");
		return false;
	}

	/* -i <image>[:<file>] */
	colon = strchr(arg, ':');
	if (colon && !colon[1]) {
		msg_gerr("Missing filename parameter in %s\n", arg);
		return false;
	}

	if (colon) {
		tmp_name = strndup(arg, colon - arg);
		if (!tmp_name) {
			msg_gerr("Out of memory\n");
			goto error;
		}

		tmp_file = strdup(colon + 1);
		if (!tmp_file) {
			msg_gerr("Out of memory\n");
			goto error;
		}
	} else {
		tmp_name = strdup(arg);
	}

	*name = tmp_name;
	*file = tmp_file;

	return true;

error:
	free(tmp_name);
	free(tmp_file);
	return false;
}

/* register an include argument (-i) for later processing */
int register_include_arg(struct layout_include_args **args, const char *arg)
{
	struct layout_include_args *tmp;
	char *name;
	char *file;

	if (!parse_include_args(arg, &name, &file))
		return 1;

	for (tmp = *args; tmp; tmp = tmp->next) {
		if (!strcmp(tmp->name, name)) {
			msg_gerr("Duplicate region name: \"%s\".\n", name);
			goto error;
		}
	}

	tmp = malloc(sizeof(*tmp));
	if (tmp == NULL) {
		msg_gerr("Out of memory\n");
		goto error;
	}

	tmp->name = name;
	tmp->file = file;
	tmp->next = *args;
	*args = tmp;
	return 0;

error:
	free(name);
	free(file);
	return 1;
}

static char *sanitise_filename(char *filename)
{
	for (unsigned i = 0; filename[i]; i++) {
		if (isspace((unsigned char)filename[i]))
			filename[i] = '_';
	}
	return filename;
}

/* returns 0 to indicate success, 1 to indicate failure */
static int include_region(struct flashrom_layout *const l, const char *name,
			  const char *file)
{
	struct romentry *const entry = _layout_entry_by_name(l, name);
	if (entry) {
		entry->included = true;
		if (file)
			entry->file = sanitise_filename(strdup(file));
		return 0;
	}
	return 1;
}

/* returns 0 to indicate success, 1 to indicate failure */
static int exclude_region(struct flashrom_layout *const l, const char *name)
{
	struct romentry *const entry = _layout_entry_by_name(l, name);
	if (entry) {
		entry->included = false;
		return 0;
	}
	return 1;
}

/* returns -1 if an entry is not found, 0 if found. */
static int romentry_exists(struct flashrom_layout *const l, char *name, char *file)
{
	if (!l->head)
		return -1;

	msg_gspew("Looking for region \"%s\"... ", name);
	if (include_region(l, name, file)) {
		msg_gspew("not found.\n");
		return -1;
	}
	msg_gspew("found.\n");
	return 0;
}

/* process -i arguments
 * returns 0 to indicate success, >0 to indicate failure
 */
int process_include_args(struct flashrom_layout *l, const struct layout_include_args *const args)
{
	unsigned int found = 0;
	const struct layout_include_args *tmp;

	if (args == NULL)
		return 0;

	/* User has specified an include argument, but no layout is loaded. */
	if (!l || !l->head) {
		msg_gerr("Region requested (with -i \"%s\"), "
			 "but no layout data is available.\n",
			 args->name);
		return 1;
	}

	tmp = args;
	while (tmp) {
		if (romentry_exists(l, tmp->name, tmp->file) < 0) {
			msg_gerr("Invalid region specified: \"%s\".\n",
				 tmp->name);
			return 1;
		}
		tmp = tmp->next;
		found++;
	}

	msg_ginfo("Using region%s: ", found > 1 ? "s" : "");
	tmp = args;
	while (tmp) {
		msg_ginfo("\"%s\"", tmp->name);
		if (tmp->file)
			msg_ginfo(":\"%s\"", tmp->file);
		if (found > 1)
			msg_ginfo(", ");
		found--;
		tmp = tmp->next;
	}
	msg_ginfo(".\n");
	return 0;
}

/* returns boolean 1 if any regions overlap, 0 otherwise */
int included_regions_overlap(const struct flashrom_layout *const l)
{
	const struct romentry *lhs = NULL;
	int overlap_detected = 0;

	while ((lhs = layout_next(l, lhs))) {
		if (!lhs->included)
			continue;

		const struct romentry *rhs = lhs;
		while ((rhs = layout_next(l, rhs))) {
			if (!rhs->included)
				continue;

			const struct flash_region *rhsr = &rhs->region;
			const struct flash_region *lhsr = &lhs->region;

			if (lhsr->start > rhsr->end)
				continue;

			if (lhsr->end < rhsr->start)
				continue;

			msg_gwarn("Regions %s [0x%08"PRIx32"-0x%08"PRIx32"] and %s [0x%08"PRIx32"-0x%08"PRIx32"] overlap\n",
				  lhsr->name, lhsr->start, lhsr->end, rhsr->name, rhsr->start, rhsr->end);
			overlap_detected = 1;
		}
	}
	return overlap_detected;
}

void cleanup_include_args(struct layout_include_args **args)
{
	struct layout_include_args *tmp;

	while (*args) {
		tmp = (*args)->next;
		free((*args)->name);
		free((*args)->file);
		free(*args);
		*args = tmp;
	}
}

int layout_sanity_checks(const struct flashrom_flashctx *const flash)
{
	const struct flashrom_layout *const layout = get_layout(flash);
	const chipsize_t total_size = flash->chip->total_size * 1024;
	int ret = 0;

	const struct romentry *entry = NULL;
	while ((entry = layout_next(layout, entry))) {
		const struct flash_region *region = &entry->region;
		if (region->start >= total_size || region->end >= total_size) {
			msg_gwarn("Warning: Address range of region \"%s\" "
				  "exceeds the current chip's address space.\n", region->name);
			if (entry->included)
				ret = 1;
		}
		if (region->start > region->end) {
			msg_gerr("Error: Size of the address range of region \"%s\" is not positive.\n",
				  region->name);
			ret = 1;
		}
	}

	return ret;
}

void prepare_layout_for_extraction(struct flashctx *flash)
{
	const struct flashrom_layout *const l = get_layout(flash);
	struct romentry *entry = NULL;

	while ((entry = mutable_layout_next(l, entry))) {
		entry->included = true;

		if (!entry->file)
			entry->file = sanitise_filename(strdup(entry->region.name));
	}
}

const struct romentry *layout_next_included_region(
		const struct flashrom_layout *const l, const chipoff_t where)
{
	const struct romentry *entry = NULL, *lowest = NULL;

	while ((entry = layout_next(l, entry))) {
		if (!entry->included)
			continue;
		if (entry->region.end < where)
			continue;
		if (!lowest || lowest->region.start > entry->region.start)
			lowest = entry;
	}

	return lowest;
}

const struct romentry *layout_next_included(
		const struct flashrom_layout *const layout, const struct romentry *iterator)
{
	while ((iterator = layout_next(layout, iterator))) {
		if (iterator->included)
			break;
	}
	return iterator;
}

const struct romentry *layout_next(
		const struct flashrom_layout *const layout, const struct romentry *iterator)
{
	return iterator ? iterator->next : layout->head;
}

int flashrom_layout_new(struct flashrom_layout **const layout)
{
	*layout = calloc(1, sizeof(**layout));
	if (!*layout) {
		msg_gerr("Error creating layout: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

int flashrom_layout_add_region(
		struct flashrom_layout *const layout,
		const size_t start, const size_t end, const char *const name)
{
	struct romentry *const entry = malloc(sizeof(*entry));
	if (!entry)
		goto _err_ret;

	const struct romentry tmp = {
		.next		= layout->head,
		.included	= false,
		.file		= NULL,
		.region		= {
					.start	= start,
					.end	= end,
					.name	= strdup(name),
				},
	};
	*entry = tmp;
	if (!entry->region.name)
		goto _err_ret;

	msg_gdbg("Added layout entry %08zx - %08zx named %s\n", start, end, name);
	layout->head = entry;
	return 0;

_err_ret:
	msg_gerr("Error adding layout entry: %s\n", strerror(errno));
	free(entry);
	return 1;
}

int flashrom_layout_include_region(struct flashrom_layout *const layout, const char *name)
{
	return include_region(layout, name, NULL);
}

int flashrom_layout_exclude_region(struct flashrom_layout *const layout, const char *name)
{
	return exclude_region(layout, name);
}

int flashrom_layout_get_region_range(struct flashrom_layout *const l, const char *name,
			      unsigned int *start, unsigned int *len)
{
	const struct romentry *const entry = _layout_entry_by_name(l, name);
	if (entry) {
		const struct flash_region *region = &entry->region;
		*start = region->start;
		*len = region->end - region->start + 1;
		return 0;
	}
	return 1;
}

void flashrom_layout_release(struct flashrom_layout *const layout)
{
	if (!layout)
		return;

	while (layout->head) {
		struct romentry *const entry = layout->head;
		layout->head = entry->next;
		free(entry->file);
		free(entry->region.name);
		free(entry);
	}
	free(layout);
}
