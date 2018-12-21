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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "flash.h"
#include "programmer.h"
#include "layout.h"

struct romentry entries[MAX_ROMLAYOUT];
static struct flashrom_layout layout = { entries, 0 };

/* include_args holds the arguments specified at the command line with -i. They must be processed at some point
 * so that desired regions are marked as "included" in the layout. */
static char *include_args[MAX_ROMLAYOUT];
static int num_include_args = 0; /* the number of valid include_args. */

struct flashrom_layout *get_global_layout(void)
{
	return &layout;
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
	FILE *romlayout;
	char tempstr[256];
	int i;

	romlayout = fopen(name, "r");

	if (!romlayout) {
		msg_gerr("ERROR: Could not open ROM layout (%s).\n",
			name);
		return -1;
	}

	while (!feof(romlayout)) {
		char *tstr1, *tstr2;

		if (layout.num_entries >= MAX_ROMLAYOUT) {
			msg_gerr("Maximum number of ROM images (%i) in layout "
				 "file reached.\n", MAX_ROMLAYOUT);
			(void)fclose(romlayout);
			return 1;
		}
		if (2 != fscanf(romlayout, "%255s %255s\n", tempstr, layout.entries[layout.num_entries].name))
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
			(void)fclose(romlayout);
			return 1;
		}
		layout.entries[layout.num_entries].start = strtol(tstr1, (char **)NULL, 16);
		layout.entries[layout.num_entries].end = strtol(tstr2, (char **)NULL, 16);
		layout.entries[layout.num_entries].included = 0;
		layout.num_entries++;
	}

	for (i = 0; i < layout.num_entries; i++) {
		msg_gdbg("romlayout %08x - %08x named %s\n",
			     layout.entries[i].start,
			     layout.entries[i].end, layout.entries[i].name);
	}

	(void)fclose(romlayout);

	return 0;
}
#endif

/* returns the index of the entry (or a negative value if it is not found) */
static int find_include_arg(const char *const name)
{
	unsigned int i;
	for (i = 0; i < num_include_args; i++) {
		if (!strcmp(include_args[i], name))
			return i;
	}
	return -1;
}

/* register an include argument (-i) for later processing */
int register_include_arg(char *name)
{
	if (num_include_args >= MAX_ROMLAYOUT) {
		msg_gerr("Too many regions included (%i).\n", num_include_args);
		return 1;
	}

	if (name == NULL) {
		msg_gerr("<NULL> is a bad region name.\n");
		return 1;
	}

	if (find_include_arg(name) != -1) {
		msg_gerr("Duplicate region name: \"%s\".\n", name);
		return 1;
	}

	include_args[num_include_args] = name;
	num_include_args++;
	return 0;
}

/* returns the index of the entry (or a negative value if it is not found) */
static int find_romentry(struct flashrom_layout *const l, char *name)
{
	int i;

	if (l->num_entries == 0)
		return -1;

	msg_gspew("Looking for region \"%s\"... ", name);
	for (i = 0; i < l->num_entries; i++) {
		if (!strcmp(l->entries[i].name, name)) {
			l->entries[i].included = 1;
			msg_gspew("found.\n");
			return i;
		}
	}
	msg_gspew("not found.\n");
	return -1;
}

/* process -i arguments
 * returns 0 to indicate success, >0 to indicate failure
 */
int process_include_args(struct flashrom_layout *const l)
{
	int i;

	if (num_include_args == 0)
		return 0;

	/* User has specified an area, but no layout file is loaded. */
	if (l->num_entries == 0) {
		msg_gerr("Region requested (with -i \"%s\"), "
			 "but no layout data is available.\n",
			 include_args[0]);
		return 1;
	}

	for (i = 0; i < num_include_args; i++) {
		if (find_romentry(l, include_args[i]) < 0) {
			msg_gerr("Invalid region specified: \"%s\".\n",
				 include_args[i]);
			return 1;
		}
	}

	msg_ginfo("Using region%s: \"%s\"", num_include_args > 1 ? "s" : "",
		  include_args[0]);
	for (i = 1; i < num_include_args; i++)
		msg_ginfo(", \"%s\"", include_args[i]);
	msg_ginfo(".\n");
	return 0;
}

void layout_cleanup(void)
{
	int i;
	for (i = 0; i < num_include_args; i++) {
		free(include_args[i]);
		include_args[i] = NULL;
	}
	num_include_args = 0;

	for (i = 0; i < layout.num_entries; i++) {
		layout.entries[i].included = 0;
	}
	layout.num_entries = 0;
}

/* Validate and - if needed - normalize layout entries. */
int normalize_romentries(const struct flashctx *flash)
{
	chipsize_t total_size = flash->chip->total_size * 1024;
	int ret = 0;

	int i;
	for (i = 0; i < layout.num_entries; i++) {
		if (layout.entries[i].start >= total_size || layout.entries[i].end >= total_size) {
			msg_gwarn("Warning: Address range of region \"%s\" exceeds the current chip's "
				  "address space.\n", layout.entries[i].name);
			if (layout.entries[i].included)
				ret = 1;
		}
		if (layout.entries[i].start > layout.entries[i].end) {
			msg_gerr("Error: Size of the address range of region \"%s\" is not positive.\n",
				  layout.entries[i].name);
			ret = 1;
		}
	}

	return ret;
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
