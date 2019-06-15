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

static struct flashrom_layout *global_layout;

struct flashrom_layout *get_global_layout(void)
{
	if (!global_layout)
		flashrom_layout_new(&global_layout, 0);
	return global_layout;
}

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
	while ((entry = mutable_layout_next(layout, entry))) {
		if (!strcmp(entry->name, name))
			return entry;
	}
	return NULL;
}

#ifndef __LIBPAYLOAD__
int read_romlayout(const char *name)
{
	struct flashrom_layout *const layout = get_global_layout();
	FILE *romlayout;
	char tempstr[256], tempname[256];
	int ret = 1;

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
		if (flashrom_layout_add_region(layout,
				strtol(tstr1, NULL, 16), strtol(tstr2, NULL, 16), tempname))
			goto _close_ret;
	}
	ret = 0;

_close_ret:
	(void)fclose(romlayout);
	return ret;
}
#endif

/* register an include argument (-i) for later processing */
int register_include_arg(struct layout_include_args **args, const char *arg)
{
	struct layout_include_args *tmp;
	char *colon;
	char *name;
	char *file;

	if (arg == NULL) {
		msg_gerr("<NULL> is a bad region name.\n");
		return 1;
	}

	/* -i <image>[:<file>] */
	colon = strchr(arg, ':');
	if (colon && !colon[1]) {
		msg_gerr("Missing filename parameter in %s\n", arg);
		return 1;
	}
	name = colon ? strndup(arg, colon - arg) : strdup(arg);
	file = colon ? strdup(colon + 1) : NULL;

	for (tmp = *args; tmp; tmp = tmp->next) {
		if (!strcmp(tmp->name, name)) {
			msg_gerr("Duplicate region name: \"%s\".\n", name);
			goto error;
		}
	}

	tmp = malloc(sizeof(*tmp));
	if (tmp == NULL) {
		msg_gerr("Could not allocate memory");
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

/* returns 0 to indicate success, 1 to indicate failure */
static int include_region(struct flashrom_layout *const l, const char *name,
			  const char *file)
{
	struct romentry *const entry = _layout_entry_by_name(l, name);
	if (entry) {
		entry->included = true;
		if (file)
			entry->file = strdup(file);
		return 0;
	}
	return 1;
}

/* returns -1 if an entry is not found, 0 if found. */
static int find_romentry(struct flashrom_layout *const l, char *name, char *file)
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

int get_region_range(struct flashrom_layout *const l, const char *name,
		     unsigned int *start, unsigned int *len)
{
	const struct romentry *const entry = _layout_entry_by_name(l, name);
	if (entry) {
		*start = entry->start;
		*len = entry->end - entry->start + 1;
		return 0;
	}
	return 1;
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
	if (!l->head) {
		msg_gerr("Region requested (with -i \"%s\"), "
			 "but no layout data is available.\n",
			 args->name);
		return 1;
	}

	tmp = args;
	while (tmp) {
		if (find_romentry(l, tmp->name, tmp->file) < 0) {
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
			if (rhs->included)
				continue;

			if (lhs->start > rhs->end)
				continue;

			if (lhs->end < rhs->start)
				continue;

			msg_gdbg("Regions %s [0x%08x-0x%08x] and %s [0x%08x-0x%08x] overlap\n",
				 lhs->name, lhs->start, lhs->end, rhs->name, rhs->start, rhs->end);
			overlap_detected = 1;
		}
	}
	return overlap_detected;
}

void layout_cleanup(struct layout_include_args **args)
{
	struct flashrom_layout *const layout = get_global_layout();
	struct layout_include_args *tmp;

	while (*args) {
		tmp = (*args)->next;
		free((*args)->name);
		free((*args)->file);
		free(*args);
		*args = tmp;
	}

	global_layout = NULL;
	flashrom_layout_release(layout);
}

/* Validate and - if needed - normalize layout entries. */
int normalize_romentries(const struct flashctx *flash)
{
	struct flashrom_layout *const layout = get_global_layout();
	chipsize_t total_size = flash->chip->total_size * 1024;
	int ret = 0;

	const struct romentry *entry = NULL;
	while ((entry = layout_next(layout, entry))) {
		if (entry->start >= total_size || entry->end >= total_size) {
			msg_gwarn("Warning: Address range of region \"%s\" "
				  "exceeds the current chip's address space.\n", entry->name);
			if (entry->included)
				ret = 1;
		}
		if (entry->start > entry->end) {
			msg_gerr("Error: Size of the address range of region \"%s\" is not positive.\n",
				  entry->name);
			ret = 1;
		}
	}

	return ret;
}

void prepare_layout_for_extraction(struct flashctx *flash)
{
	const struct flashrom_layout *const l = get_layout(flash);
	struct romentry *entry = NULL;
	unsigned int i;

	while ((entry = mutable_layout_next(l, entry))) {
		entry->included = true;

		if (!entry->file)
			entry->file = strdup(entry->name);

		for (i = 0; entry->file[i]; ++i) {
			if (isspace(entry->file[i]))
				entry->file[i] = '_';
		}
	}
}

const struct romentry *layout_next_included_region(
		const struct flashrom_layout *const l, const chipoff_t where)
{
	const struct romentry *entry = NULL, *lowest = NULL;

	while ((entry = layout_next(l, entry))) {
		if (!entry->included)
			continue;
		if (entry->end < where)
			continue;
		if (!lowest || lowest->start > entry->start)
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

/**
 * @addtogroup flashrom-layout
 * @{
 */

/**
 * @brief Create a new, empty layout.
 *
 * @param layout Pointer to returned layout reference.
 * @param count  Number of layout entries to allocate.
 *
 * @return 0 on success,
 *         1 if out of memory.
 */
int flashrom_layout_new(struct flashrom_layout **const layout, const unsigned int count)
{
	*layout = malloc(sizeof(**layout));
	if (!*layout) {
		msg_gerr("Error creating layout: %s\n", strerror(errno));
		return 1;
	}

	const struct flashrom_layout tmp = { 0 };
	**layout = tmp;
	return 0;
}

/**
 * @brief Add another region to an existing layout.
 *
 * @param layout The existing layout.
 * @param start  Start address of the region.
 * @param end    End address (inclusive) of the region.
 * @param name   Name of the region.
 *
 * @return 0 on success,
 *         1 if out of memory.
 */
int flashrom_layout_add_region(
		struct flashrom_layout *const layout,
		const size_t start, const size_t end, const char *const name)
{
	struct romentry *const entry = malloc(sizeof(*entry));
	if (!entry)
		goto _err_ret;

	const struct romentry tmp = {
		.next		= layout->head,
		.start		= start,
		.end		= end,
		.included	= false,
		.name		= strdup(name),
		.file		= NULL,
	};
	*entry = tmp;
	if (!entry->name)
		goto _err_ret;

	msg_gdbg("Added layout entry %08zx - %08zx named %s\n", start, end, name);
	layout->head = entry;
	return 0;

_err_ret:
	msg_gerr("Error adding layout entry: %s\n", strerror(errno));
	free(entry);
	return 1;
}

/**
 * @brief Mark given region as included.
 *
 * @param layout The layout to alter.
 * @param name   The name of the region to include.
 *
 * @return 0 on success,
 *         1 if the given name can't be found.
 */
int flashrom_layout_include_region(struct flashrom_layout *const layout, const char *name)
{
	return include_region(layout, name, NULL);
}

/**
 * @brief Free a layout.
 *
 * @param layout Layout to free.
 */
void flashrom_layout_release(struct flashrom_layout *const layout)
{
	if (layout == global_layout)
		return;

	if (!layout)
		return;

	while (layout->head) {
		struct romentry *const entry = layout->head;
		layout->head = entry->next;
		free(entry->file);
		free(entry->name);
		free(entry);
	}
	free(layout);
}

/** @} */ /* end flashrom-layout */
