/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2005-2008 coresystems GmbH
 * (Written by Stefan Reinauer <stepan@coresystems.de> for coresystems GmbH)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "flash.h"
#include "programmer.h"

static int romimages = 0;

#define MAX_ROMLAYOUT	32

typedef struct {
	unsigned int start;
	unsigned int end;
	unsigned int included;
	char name[256];
} romlayout_t;

/* include_args lists arguments specified at the command line with -i. They
 * must be processed at some point so that desired regions are marked as
 * "included" in the rom_entries list.
 */
static char *include_args[MAX_ROMLAYOUT];
static int num_include_args = 0; /* the number of valid entries. */
static romlayout_t rom_entries[MAX_ROMLAYOUT];

#ifndef __LIBPAYLOAD__
int read_romlayout(char *name)
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

		if (romimages >= MAX_ROMLAYOUT) {
			msg_gerr("Maximum number of ROM images (%i) in layout "
				 "file reached.\n", MAX_ROMLAYOUT);
			return 1;
		}
		if (2 != fscanf(romlayout, "%s %s\n", tempstr, rom_entries[romimages].name))
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
			msg_gerr("Error parsing layout file.\n");
			fclose(romlayout);
			return 1;
		}
		rom_entries[romimages].start = strtol(tstr1, (char **)NULL, 16);
		rom_entries[romimages].end = strtol(tstr2, (char **)NULL, 16);
		rom_entries[romimages].included = 0;
		romimages++;
	}

	for (i = 0; i < romimages; i++) {
		msg_gdbg("romlayout %08x - %08x named %s\n",
			     rom_entries[i].start,
			     rom_entries[i].end, rom_entries[i].name);
	}

	fclose(romlayout);

	return 0;
}
#endif

/* returns the index of the entry (or a negative value if it is not found) */
int find_include_arg(const char *const name)
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
static int find_romentry(char *name)
{
	int i;

	if (!romimages)
		return -1;

	msg_gspew("Looking for region \"%s\"... ", name);
	for (i = 0; i < romimages; i++) {
		if (!strcmp(rom_entries[i].name, name)) {
			rom_entries[i].included = 1;
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
int process_include_args(void)
{
	int i;
	unsigned int found = 0;

	if (num_include_args == 0)
		return 0;

	/* User has specified an area, but no layout file is loaded. */
	if (!romimages) {
		msg_gerr("Region requested (with -i \"%s\"), "
			 "but no layout data is available.\n",
			 include_args[0]);
		return 1;
	}

	for (i = 0; i < num_include_args; i++) {
		if (find_romentry(include_args[i]) < 0) {
			msg_gerr("Invalid region specified: \"%s\".\n",
				 include_args[i]);
			return 1;
		}
		found++;
	}

	msg_ginfo("Using region%s: \"%s\"", num_include_args > 1 ? "s" : "",
		  include_args[0]);
	for (i = 1; i < num_include_args; i++)
		msg_ginfo(", \"%s\"", include_args[i]);
	msg_ginfo(".\n");
	return 0;
}

romlayout_t *get_next_included_romentry(unsigned int start)
{
	int i;
	unsigned int best_start = UINT_MAX;
	romlayout_t *best_entry = NULL;
	romlayout_t *cur;

	/* First come, first serve for overlapping regions. */
	for (i = 0; i < romimages; i++) {
		cur = &rom_entries[i];
		if (!cur->included)
			continue;
		/* Already past the current entry? */
		if (start > cur->end)
			continue;
		/* Inside the current entry? */
		if (start >= cur->start)
			return cur;
		/* Entry begins after start. */
		if (best_start > cur->start) {
			best_start = cur->start;
			best_entry = cur;
		}
	}
	return best_entry;
}

int handle_romentries(struct flashctx *flash, uint8_t *oldcontents, uint8_t *newcontents)
{
	unsigned int start = 0;
	romlayout_t *entry;
	unsigned int size = flash->total_size * 1024;

	/* If no regions were specified for inclusion, assume
	 * that the user wants to write the complete new image.
	 */
	if (num_include_args == 0)
		return 0;

	/* Non-included romentries are ignored.
	 * The union of all included romentries is used from the new image.
	 */
	while (start < size) {
		entry = get_next_included_romentry(start);
		/* No more romentries for remaining region? */
		if (!entry) {
			memcpy(newcontents + start, oldcontents + start,
			       size - start);
			break;
		}
		/* For non-included region, copy from old content. */
		if (entry->start > start)
			memcpy(newcontents + start, oldcontents + start,
			       entry->start - start);
		/* Skip to location after current romentry. */
		start = entry->end + 1;
		/* Catch overflow. */
		if (!start)
			break;
	}
	return 0;
}
