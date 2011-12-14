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
#include <ctype.h>
#include <limits.h>
#include "flash.h"
#include "programmer.h"

#if CONFIG_INTERNAL == 1
char *mainboard_vendor = NULL;
char *mainboard_part = NULL;
#endif
static int romimages = 0;

#define MAX_ROMLAYOUT	32

typedef struct {
	unsigned int start;
	unsigned int end;
	unsigned int included;
	char name[256];
} romlayout_t;

static romlayout_t rom_entries[MAX_ROMLAYOUT];

#if CONFIG_INTERNAL == 1 /* FIXME: Move the whole block to cbtable.c? */
static char *def_name = "DEFAULT";

int show_id(uint8_t *bios, int size, int force)
{
	unsigned int *walk;
	unsigned int mb_part_offset, mb_vendor_offset;
	char *mb_part, *mb_vendor;

	mainboard_vendor = def_name;
	mainboard_part = def_name;

	walk = (unsigned int *)(bios + size - 0x10);
	walk--;

	if ((*walk) == 0 || ((*walk) & 0x3ff) != 0) {
		/* We might have an NVIDIA chipset BIOS which stores the ID
		 * information at a different location.
		 */
		walk = (unsigned int *)(bios + size - 0x80);
		walk--;
	}

	/*
	 * Check if coreboot last image size is 0 or not a multiple of 1k or
	 * bigger than the chip or if the pointers to vendor ID or mainboard ID
	 * are outside the image of if the start of ID strings are nonsensical
	 * (nonprintable and not \0).
	 */
	mb_part_offset = *(walk - 1);
	mb_vendor_offset = *(walk - 2);
	if ((*walk) == 0 || ((*walk) & 0x3ff) != 0 || (*walk) > size ||
	    mb_part_offset > size || mb_vendor_offset > size) {
		msg_pinfo("Flash image seems to be a legacy BIOS. "
		          "Disabling coreboot-related checks.\n");
		return 0;
	}

	mb_part = (char *)(bios + size - mb_part_offset);
	mb_vendor = (char *)(bios + size - mb_vendor_offset);
	if (!isprint((unsigned char)*mb_part) ||
	    !isprint((unsigned char)*mb_vendor)) {
		msg_pinfo("Flash image seems to have garbage in the ID location."
		       " Disabling checks.\n");
		return 0;
	}

	msg_pdbg("coreboot last image size "
		     "(not ROM size) is %d bytes.\n", *walk);

	mainboard_part = strdup(mb_part);
	mainboard_vendor = strdup(mb_vendor);
	msg_pdbg("Manufacturer: %s\n", mainboard_vendor);
	msg_pdbg("Mainboard ID: %s\n", mainboard_part);

	/*
	 * If lb_vendor is not set, the coreboot table was
	 * not found. Nor was -m VENDOR:PART specified.
	 */
	if (!lb_vendor || !lb_part) {
		msg_pinfo("Note: If the following flash access fails, "
		       "try -m <vendor>:<mainboard>.\n");
		return 0;
	}

	/* These comparisons are case insensitive to make things
	 * a little less user^Werror prone. 
	 */
	if (!strcasecmp(mainboard_vendor, lb_vendor) &&
	    !strcasecmp(mainboard_part, lb_part)) {
		msg_pdbg("This firmware image matches this mainboard.\n");
	} else {
		if (force_boardmismatch) {
			msg_pinfo("WARNING: This firmware image does not "
			       "seem to fit to this machine - forcing it.\n");
		} else {
			msg_pinfo("ERROR: Your firmware image (%s:%s) does not "
			       "appear to\n       be correct for the detected "
			       "mainboard (%s:%s)\n\nOverride with -p internal:"
			       "boardmismatch=force if you are absolutely sure "
			       "that\nyou are using a correct "
			       "image for this mainboard or override\nthe detected "
			       "values with --mainboard <vendor>:<mainboard>.\n\n",
			       mainboard_vendor, mainboard_part, lb_vendor,
			       lb_part);
			exit(1);
		}
	}

	return 0;
}
#endif

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
				 "file reached before end of layout file.\n",
				 MAX_ROMLAYOUT);
			msg_gerr("Ignoring the rest of the layout file.\n");
			break;
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

int find_romentry(char *name)
{
	int i;

	if (!romimages)
		return -1;

	msg_ginfo("Looking for \"%s\"... ", name);

	for (i = 0; i < romimages; i++) {
		if (!strcmp(rom_entries[i].name, name)) {
			rom_entries[i].included = 1;
			msg_ginfo("found.\n");
			return i;
		}
	}
	msg_ginfo("not found.\n");	// Not found. Error.

	return -1;
}

int find_next_included_romentry(unsigned int start)
{
	int i;
	unsigned int best_start = UINT_MAX;
	int best_entry = -1;

	/* First come, first serve for overlapping regions. */
	for (i = 0; i < romimages; i++) {
		if (!rom_entries[i].included)
			continue;
		/* Already past the current entry? */
		if (start > rom_entries[i].end)
			continue;
		/* Inside the current entry? */
		if (start >= rom_entries[i].start)
			return i;
		/* Entry begins after start. */
		if (best_start > rom_entries[i].start) {
			best_start = rom_entries[i].start;
			best_entry = i;
		}
	}
	return best_entry;
}

int handle_romentries(struct flashctx *flash, uint8_t *oldcontents, uint8_t *newcontents)
{
	unsigned int start = 0;
	int entry;
	unsigned int size = flash->total_size * 1024;

	/* If no layout file was specified or the layout file was empty, assume
	 * that the user wants to flash the complete new image.
	 */
	if (!romimages)
		return 0;
	/* Non-included romentries are ignored.
	 * The union of all included romentries is used from the new image.
	 */
	while (start < size) {
		entry = find_next_included_romentry(start);
		/* No more romentries for remaining region? */
		if (entry < 0) {
			memcpy(newcontents + start, oldcontents + start,
			       size - start);
			break;
		}
		if (rom_entries[entry].start > start)
			memcpy(newcontents + start, oldcontents + start,
			       rom_entries[entry].start - start);
		/* Skip to location after current romentry. */
		start = rom_entries[entry].end + 1;
		/* Catch overflow. */
		if (!start)
			break;
	}
			
	return 0;
}
