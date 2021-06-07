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

static struct romentry entries[MAX_ROMLAYOUT];
static struct flashrom_layout global_layout = { entries, 0 };

struct flashrom_layout *get_global_layout(void)
{
	return &global_layout;
}

const struct flashrom_layout *get_layout(const struct flashrom_flashctx *const flashctx)
{
	if (flashctx->layout && flashctx->layout->num_entries)
		return flashctx->layout;
	else
		return &flashctx->fallback_layout.base;
}

#ifndef __LIBPAYLOAD__
int read_romlayout(const char *name)
{
	struct flashrom_layout *const layout = get_global_layout();
	FILE *romlayout;
	char tempstr[256], tempname[256];
	unsigned int i;
	int ret = 1;

	romlayout = fopen(name, "r");

	if (!romlayout) {
		msg_gerr("ERROR: Could not open layout file (%s).\n",
			name);
		return -1;
	}

	while (!feof(romlayout)) {
		char *tstr1, *tstr2;

		if (layout->num_entries >= MAX_ROMLAYOUT) {
			msg_gerr("Maximum number of ROM images (%i) in layout "
				 "file reached.\n", MAX_ROMLAYOUT);
			goto _close_ret;
		}
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
		layout->entries[layout->num_entries].start = strtol(tstr1, (char **)NULL, 16);
		layout->entries[layout->num_entries].end = strtol(tstr2, (char **)NULL, 16);
		layout->entries[layout->num_entries].included = false;
		layout->entries[layout->num_entries].file = NULL;
		layout->entries[layout->num_entries].name = strdup(tempname);
		if (!layout->entries[layout->num_entries].name) {
			msg_gerr("Error adding layout entry: %s\n", strerror(errno));
			goto _close_ret;
		}
		layout->num_entries++;
	}

	for (i = 0; i < layout->num_entries; i++) {
		msg_gdbg("romlayout %08x - %08x named %s\n",
			     layout->entries[i].start,
			     layout->entries[i].end, layout->entries[i].name);
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
	size_t i;
	for (i = 0; i < l->num_entries; ++i) {
		if (!strcmp(l->entries[i].name, name)) {
			l->entries[i].included = true;
			if (file)
				l->entries[i].file = strdup(file);
			return 0;
		}
	}
	return 1;
}

/* returns -1 if an entry is not found, 0 if found. */
static int find_romentry(struct flashrom_layout *const l, char *name, char *file)
{
	if (l->num_entries == 0)
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
	size_t i;
	for (i = 0; i < l->num_entries; ++i) {
		if (!strcmp(l->entries[i].name, name)) {
			*start = l->entries[i].start;
			*len = l->entries[i].end - l->entries[i].start + 1;
			return 0;
		}
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
	if (l->num_entries == 0) {
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
	size_t i;
	int overlap_detected = 0;

	for (i = 0; i < l->num_entries; i++) {
		size_t j;

		if (!l->entries[i].included)
			continue;

		for (j = i + 1; j < l->num_entries; j++) {
			if (!l->entries[j].included)
				continue;

			if (l->entries[i].start > l->entries[j].end)
				continue;

			if (l->entries[i].end < l->entries[j].start)
				continue;

			msg_gdbg("Regions %s [0x%08x-0x%08x] and "
				"%s [0x%08x-0x%08x] overlap\n",
				l->entries[i].name, l->entries[i].start,
				l->entries[i].end, l->entries[j].name,
				l->entries[j].start, l->entries[j].end);
			overlap_detected = 1;
		}
	}
	return overlap_detected;
}

void layout_cleanup(struct layout_include_args **args)
{
	struct flashrom_layout *const layout = get_global_layout();
	unsigned int i;
	struct layout_include_args *tmp;

	while (*args) {
		tmp = (*args)->next;
		free((*args)->name);
		free((*args)->file);
		free(*args);
		*args = tmp;
	}

	for (i = 0; i < layout->num_entries; i++) {
		free(layout->entries[i].name);
		free(layout->entries[i].file);
		layout->entries[i].included = false;
	}
	layout->num_entries = 0;
}

/* Validate and - if needed - normalize layout entries. */
int normalize_romentries(const struct flashctx *flash)
{
	struct flashrom_layout *const layout = get_global_layout();
	chipsize_t total_size = flash->chip->total_size * 1024;
	int ret = 0;

	unsigned int i;
	for (i = 0; i < layout->num_entries; i++) {
		if (layout->entries[i].start >= total_size || layout->entries[i].end >= total_size) {
			msg_gwarn("Warning: Address range of region \"%s\" exceeds the current chip's "
				  "address space.\n", layout->entries[i].name);
			if (layout->entries[i].included)
				ret = 1;
		}
		if (layout->entries[i].start > layout->entries[i].end) {
			msg_gerr("Error: Size of the address range of region \"%s\" is not positive.\n",
				  layout->entries[i].name);
			ret = 1;
		}
	}

	return ret;
}

void prepare_layout_for_extraction(struct flashctx *flash)
{
	const struct flashrom_layout *const l = get_layout(flash);
	unsigned int i, j;

	for (i = 0; i < l->num_entries; ++i) {
		l->entries[i].included = true;

		if (!l->entries[i].file)
			l->entries[i].file = strdup(l->entries[i].name);

		for (j = 0; l->entries[i].file[j]; j++) {
			if (isspace(l->entries[i].file[j]))
				l->entries[i].file[j] = '_';
		}
	}
}

const struct romentry *layout_next_included_region(
		const struct flashrom_layout *const l, const chipoff_t where)
{
	unsigned int i;
	const struct romentry *lowest = NULL;

	for (i = 0; i < l->num_entries; ++i) {
		if (!l->entries[i].included)
			continue;
		if (l->entries[i].end < where)
			continue;
		if (!lowest || lowest->start > l->entries[i].start)
			lowest = &l->entries[i];
	}

	return lowest;
}

const struct romentry *layout_next_included(
		const struct flashrom_layout *const layout, const struct romentry *iterator)
{
	const struct romentry *const end = layout->entries + layout->num_entries;

	if (iterator)
		++iterator;
	else
		iterator = &layout->entries[0];

	for (; iterator < end; ++iterator) {
		if (!iterator->included)
			continue;
		return iterator;
	}
	return NULL;
}

/**
 * @addtogroup flashrom-layout
 * @{
 */

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
	unsigned int i;

	if (!layout || layout == get_global_layout())
		return;

	for (i = 0; i < layout->num_entries; ++i) {
		free(layout->entries[i].name);
		free(layout->entries[i].file);
	}
	free(layout);
}

/** @} */ /* end flashrom-layout */
