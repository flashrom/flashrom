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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "flash.h"
#include "programmer.h"

#define MAX_ROMLAYOUT	32
#define MAX_ENTRY_LEN	1024
#define WHITESPACE_CHARS " \t"
#define INCLUDE_INSTR "source"
#define MAX_NESTING_LVL 5

typedef struct {
	chipoff_t start;
	chipoff_t end;
	bool start_topalign;
	bool end_topalign;
	bool included;
	char *name;
	char *file;
} romentry_t;

/* rom_entries store the entries specified in a layout file and associated run-time data */
static romentry_t rom_entries[MAX_ROMLAYOUT];
static int num_rom_entries = 0; /* the number of successfully parsed rom_entries */

/* include_args holds the arguments specified at the command line with -i. They must be processed at some point
 * so that desired regions are marked as "included" in the rom_entries list. */
static char *include_args[MAX_ROMLAYOUT];
static int num_include_args = 0; /* the number of valid include_args. */

#ifndef __LIBPAYLOAD__
/* returns the index of the entry (or a negative value if it is not found) */
static int find_romentry(char *name)
{
	int i;
	msg_gspew("Looking for region \"%s\"... ", name);
	for (i = 0; i < num_rom_entries; i++) {
		if (strcmp(rom_entries[i].name, name) == 0) {
			msg_gspew("found.\n");
			return i;
		}
	}
	msg_gspew("not found.\n");
	return -1;
}

/* FIXME: While libpayload has no file I/O itself, code using libflashrom could still provide layout information
 * obtained by other means like user input or by fetching it from somewhere else. Therefore the parsing code
 * should be separated from the file reading code eventually. */
/** Parse one line in a layout file.
 * @param	file_name The name of the file this line originates from.
 * @param	linecnt Line number the input string originates from.
 * @param	entry If not NULL fill it with the parsed data, else just detect errors and print diagnostics.
 * @return	-1 on error,
 *		0 if the line could be parsed into a layout entry succesfully,
 *		1 if a file was successfully sourced.
 */
static int parse_entry(const char *file_name, unsigned int linecnt, char *buf, romentry_t *entry)
{
	msg_gdbg2("String to parse: \"%s\".\n", buf);

	/* Skip all white space in the beginning. */
	char *tmp_str = buf + strspn(buf, WHITESPACE_CHARS);
	char *endptr;

	/* Check for include command. */
	if (strncmp(tmp_str, INCLUDE_INSTR, strlen(INCLUDE_INSTR)) == 0) {
		tmp_str += strlen(INCLUDE_INSTR);
		tmp_str += strspn(tmp_str, WHITESPACE_CHARS);
		if (unquote_string(&tmp_str, NULL, WHITESPACE_CHARS) != 0) {
			msg_gerr("Error parsing version 2 layout entry in file \"%s\" at line %d:\n"
				 "Could not find file name in \"%s\".\n",
				 file_name, linecnt, buf);
				return -1;
		}
		msg_gspew("Source command found with filename \"%s\".\n", tmp_str);

		static unsigned int nesting_lvl = 0;
		if (nesting_lvl >= MAX_NESTING_LVL) {
			msg_gerr("Error: Nesting level exeeded limit of %u.\n", MAX_NESTING_LVL);
			msg_gerr("Unable to import \"%s\" in layout file \"%s\".\n", tmp_str, file_name);
			return -1;
		}

		nesting_lvl++;
		int ret;
		/* If a relative path is given, append it to the dirname of the current file. */
		if (*tmp_str != '/') {
			/* We need space for: dirname of file_name, '/' , the file name in tmp_strand and '\0'.
			 * Since the dirname of file_name is shorter than file_name this is more than enough: */
			char *path = malloc(strlen(file_name) + strlen(tmp_str) + 2);
			if (path == NULL) {
				msg_gerr("Out of memory!\n");
				return -1;
			}
			strcpy(path, file_name);

			/* A less insane but incomplete dirname implementation... */
			endptr = strrchr(path, '/');
			if (endptr != NULL) {
				endptr[0] = '/';
				endptr[1] = '\0';
			} else {
				/* This is safe because the original file name was at least one char. */
				path[0] = '.';
				path[1] = '/';
				path[2] = '\0';
			}
			strcat(path, tmp_str);
			ret = read_romlayout(path);
			free(path);
		} else
			ret = read_romlayout(tmp_str);
		nesting_lvl--;
		return ret >= 0 ? 1 : -1; /* Only return values < 0 are errors. */
	}

	/* Parse start address. */
	errno = 0;
	long long tmp_addr = strtoll(tmp_str, &endptr, 0);
	if (errno != 0 || endptr == tmp_str) {
		msg_gerr("Error parsing version 2 layout entry in file \"%s\" at line %d:\n"
			 "Could not convert start address in \"%s\".\n", file_name, linecnt, buf);
		return -1;
	}
	bool start_topalign = (tmp_str[0] == '-');
	if (llabs(tmp_addr) > FL_MAX_CHIPADDR) {
		msg_gerr("Error parsing version 2 layout entry in file \"%s\" at line %d:\n"
			 "Start address (%s0x%llx) in \"%s\" is beyond the supported range (max 0x%"
			 PRIxCHIPADDR ").\n", file_name, linecnt, start_topalign ? "-" : "",
			 llabs(tmp_addr), buf, FL_MAX_CHIPADDR);
		return -1;
	}
	chipoff_t start = (chipoff_t)llabs(tmp_addr);

	tmp_str = endptr + strspn(endptr, WHITESPACE_CHARS);
	if (*tmp_str != ':') {
		msg_gerr("Error parsing version 2 layout entry in file \"%s\" at line %d:\n"
			 "Address separator does not follow start address in \"%s\".\n",
			 file_name, linecnt, buf);
		return -1;
	}
	tmp_str++;

	/* Parse end address. */
	errno = 0;
	tmp_addr = strtoll(tmp_str, &endptr, 0);
	if (errno != 0 || endptr == tmp_str) {
		msg_gerr("Error parsing version 2 layout entry in file \"%s\" at line %d:\n"
			 "Could not convert end address in \"%s\".\n", file_name, linecnt, buf);
		return -1;
	}
	bool end_topalign = (tmp_str[0] == '-');
	if (llabs(tmp_addr) > FL_MAX_CHIPADDR) {
		msg_gerr("Error parsing version 2 layout entry in file \"%s\" at line %d:\n"
			 "End address (%s0x%llx) in \"%s\" is beyond the supported range (max 0x%"
			 PRIxCHIPADDR ").\n", file_name, linecnt, end_topalign ? "-" : "",
			 llabs(tmp_addr), buf, FL_MAX_CHIPADDR);
		return -1;
	}
	chipoff_t end = (chipoff_t)llabs(tmp_addr);

	size_t skip = strspn(endptr, WHITESPACE_CHARS);
	if (skip == 0) {
		msg_gerr("Error parsing version 2 layout entry in file \"%s\" at line %d:\n"
			 "End address is not followed by white space in \"%s\"\n", file_name, linecnt, buf);
		return -1;
	}

	/* Parse region name. */
	tmp_str = endptr + skip;
	/* The region name is either enclosed by quotes or ends with the first whitespace. */
	if (unquote_string(&tmp_str, &endptr, WHITESPACE_CHARS) != 0) {
		msg_gerr("Error parsing version 2 layout entry in file \"%s\" at line %d:\n"
			 "Could not find region name in \"%s\".\n", file_name, linecnt, buf);
		return -1;
	}

	msg_gdbg2("Parsed entry: 0x%" PRIxCHIPADDR " (%s) : 0x%" PRIxCHIPADDR " (%s) named \"%s\"\n",
		  start, start_topalign ? "top-aligned" : "bottom-aligned",
		  end, end_topalign ? "top-aligned" : "bottom-aligned", tmp_str);

	/* We can only check address ranges if the chip size is available in case one address is relative to
	 * the top and the other to the bottom. But if they are both relative to the same end we can without
	 * knowing the complete size. This allows us to bail out before probing. */
	if ((!start_topalign && !end_topalign && start > end) ||
	    (start_topalign && end_topalign && start < end)) {
		msg_gerr("Error parsing version 2 layout entry in file \"%s\" at line %d:\n"
			 "Length of region \"%s\" is not positive.\n", file_name, linecnt, tmp_str);
		return -1;
	}

	if (find_romentry(tmp_str) >= 0) {
		msg_gerr("Error parsing version 2 layout entry in file \"%s\" at line %d:\n"
			 "Region name \"%s\" used multiple times.\n", file_name, linecnt, tmp_str);
		return -1;
	}

	endptr += strspn(endptr, WHITESPACE_CHARS);
	if (strlen(endptr) != 0)
		msg_gwarn("Warning parsing version 2 layout entry in file \"%s\" at line %d:\n"
			  "Region name \"%s\" is not followed by white space only.\n",
			  file_name, linecnt, tmp_str);


	if (entry != NULL) {
		entry->name = strdup(tmp_str);
		if (entry->name == NULL) {
			msg_gerr("Out of memory!\n");
			return -1;
		}

		entry->start = start;
		entry->end = end;
		entry->included = 0;
		entry->start_topalign = start_topalign;
		entry->end_topalign = end_topalign;
		entry->file = NULL;
	}
	return 0;
}

/* Scan the first line for the determinant version comment and parse it, or assume it is version 1. */
static int detect_layout_version(FILE *romlayout)
{
	int c;
	do { /* Skip white space */
		c = fgetc(romlayout);
		if (c == EOF)
			return -1;
	} while (isblank(c));
	ungetc(c, romlayout);

	const char* vcomment = "# flashrom layout v";
	char buf[strlen(vcomment) + 1]; /* comment + \0 */
	if (fgets(buf, sizeof(buf), romlayout) == NULL)
		return -1;
	if (strcmp(vcomment, buf) != 0)
		return 1;
	int version;
	if (fscanf(romlayout, "%d", &version) != 1)
		return -1;
	if (version < 2) {
		msg_gwarn("Warning: Layout file declares itself to be version %d, but self declaration has\n"
			  "only been possible since version 2. Continuing anyway.\n", version);
	}
	return version;
}

int read_romlayout(const char *name)
{
	FILE *romlayout = fopen(name, "r");
	if (romlayout == NULL) {
		msg_gerr("ERROR: Could not open layout file \"%s\".\n", name);
		return -1;
	}

	const int version = detect_layout_version(romlayout);
	if (version < 0) {
		msg_gerr("Could not determine version of layout file \"%s\".\n", name);
		fclose(romlayout);
		return 1;
	}
	if (version != 2) {
		msg_gerr("Layout file version %d is not supported in this version of flashrom.\n", version);
		fclose(romlayout);
		return 1;
	}
	rewind(romlayout);

	msg_gdbg("Parsing layout file \"%s\" according to version %d.\n", name, version);
	unsigned int linecnt = 0;
	while (!feof(romlayout)) {
		char buf[MAX_ENTRY_LEN];
		char *curchar = buf;

		while (true) {
			/* Make sure that we ignore various newline sequences by checking for \r too.
			 * NB: This might introduce empty lines. */
			char c = fgetc(romlayout);
			if (c == '#') {
				do { /* Skip characters in comments */
					c = fgetc(romlayout);
				} while (c != EOF && c != '\n' && c != '\r');
				linecnt++;
				continue;
			}
			if (c == EOF || c == '\n' || c == '\r') {
				*curchar = '\0';
				linecnt++;
				break;
			}
			if (curchar == &buf[MAX_ENTRY_LEN - 1]) {
				msg_gerr("Line %d of layout file \"%s\" is longer than the allowed %d chars.\n",
					 linecnt, name, MAX_ENTRY_LEN);
				fclose(romlayout);
				return 1;
			}
			*curchar = c;
			curchar++;
		}
		msg_gspew("Parsing line %d of \"%s\".\n", linecnt, name);

		/* Skip all whitespace or empty lines */
		if (strspn(buf, WHITESPACE_CHARS) == strlen(buf))
			continue;

		romentry_t *entry = (num_rom_entries >= MAX_ROMLAYOUT) ? NULL : &rom_entries[num_rom_entries];
		int ret = parse_entry(name, linecnt, buf, entry);
		if (ret < 0) {
			fclose(romlayout);
			return 1;
		}
		/* Only 0 indicates the successfully parsing of an entry, others are errors or imports. */
		if (ret == 0)
			num_rom_entries++;
	}
	fclose(romlayout);
	if (num_rom_entries >= MAX_ROMLAYOUT) {
		msg_gerr("Found %d entries in layout file which is more than the %i allowed.\n",
			 num_rom_entries + 1, MAX_ROMLAYOUT);
		return 1;
	}
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

/* process -i arguments
 * returns 0 to indicate success, >0 to indicate failure
 */
int process_include_args(void)
{
	int i;
	unsigned int found = 0;

	if (num_include_args == 0)
		return 0;

	/* User has specified an include argument, but no layout file is loaded. */
	if (num_rom_entries == 0) {
		msg_gerr("Region requested (with -i/--include \"%s\"),\n"
			 "but no layout data is available. To include one use the -l/--layout syntax).\n",
			 include_args[0]);
		return 1;
	}

	for (i = 0; i < num_include_args; i++) {
		char *file;
		char *name = include_args[i];
		int ret = unquote_string(&name, &file, ":"); /* -i <region>[:<file>] */
		if (ret != 0) {
			msg_gerr("Invalid include argument specified: \"%s\".\n", name);
			return 1;
		}
		int idx = find_romentry(name);
		if (idx < 0) {
			msg_gerr("Invalid region name specified: \"%s\".\n", name);
			return 1;
		}
		rom_entries[idx].included = 1;
		found++;

		if (file[0] != '\0') {
			/* The remaining characters are interpreted as possible quoted filename.  */
			ret = unquote_string(&file, NULL, NULL);
			if (ret != 0) {
				msg_gerr("Invalid region file name specified: \"%s\".\n", file);
				return 1;
			}
#ifdef __LIBPAYLOAD__
			msg_gerr("Error: No file I/O support in libpayload\n");
			return 1;
#else
			file = strdup(file);
			if (file == NULL) {
				msg_gerr("Out of memory!\n");
				return 1;
			}
			rom_entries[idx].file = file;
#endif
		}
	}

	msg_ginfo("Using region%s: ", num_rom_entries > 1 ? "s" : "");
	bool first = true;
	for (i = 0; i < num_rom_entries; i++)
		if (rom_entries[i].included) {
			if (first)
				first = false;
			else
				msg_ginfo(", ");
			msg_ginfo("\"%s\"", rom_entries[i].name);
	}
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

	for (i = 0; i < num_rom_entries; i++) {
		free(rom_entries[i].name);
		rom_entries[i].name = NULL;
		free(rom_entries[i].file);
		rom_entries[i].file = NULL;
		rom_entries[i].included = 0;
	}
	num_rom_entries = 0;
}

romentry_t *get_next_included_romentry(unsigned int start)
{
	int i;
	unsigned int best_start = UINT_MAX;
	romentry_t *best_entry = NULL;
	romentry_t *cur;

	/* First come, first serve for overlapping regions. */
	for (i = 0; i < num_rom_entries; i++) {
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

/* Validate and - if needed - normalize layout entries. */
int normalize_romentries(const struct flashctx *flash)
{
	chipsize_t total_size = flash->chip->total_size * 1024;
	int ret = 0;

	int i;
	for (i = 0; i < num_rom_entries; i++) {
		romentry_t *cur = &rom_entries[i];
		/* Normalize top-aligned address. */
		if (cur->start_topalign || cur->end_topalign) {
			if (cur->start_topalign) {
				cur->start = total_size - cur->start - 1;
				cur->start_topalign = 0;
			}

			if (cur->end_topalign) {
				cur->end = total_size - cur->end - 1;
				cur->end_topalign = 0;
			}

			msg_gspew("Normalized entry %d \"%s\": 0x%" PRIxCHIPADDR " - 0x%" PRIxCHIPADDR "\n",
				  i, cur->name, cur->start, cur->end);
		}

		if (cur->start >= total_size || cur->end >= total_size) {
			msg_gwarn("Warning: Address range of region \"%s\" exceeds the current chip's "
				  "address space.\n", cur->name);
			if (cur->included)
				ret = 1;
		}
		if (cur->start > cur->end) {
			msg_gerr("Error: Size of the address range of region \"%s\" is not positive.\n",
				  cur->name);
			ret = 1;
		}
	}

	return ret;
}

static int copy_old_content(struct flashctx *flash, int oldcontents_valid, uint8_t *oldcontents, uint8_t *newcontents, unsigned int start, unsigned int size)
{
	if (!oldcontents_valid) {
		/* oldcontents is a zero-filled buffer. By reading the current data into oldcontents here, we
		 * avoid a rewrite of identical regions even if an initial full chip read didn't happen. */
		msg_gdbg2("Read a chunk starting at 0x%06x (len=0x%06x).\n", start, size);
		int ret = flash->chip->read(flash, oldcontents + start, start, size);
		if (ret != 0) {
			msg_gerr("Failed to read chunk 0x%06x-0x%06x.\n", start, start + size - 1);
			return 1;
		}
	}
	memcpy(newcontents + start, oldcontents + start, size);
	return 0;
}

/**
 * Modify @newcontents so that it contains the data that should be on the chip eventually. In the case the user
 * wants to update only parts of it, copy the chunks to be preserved from @oldcontents to @newcontents. If
 * @oldcontents is not valid, we need to fetch the current data from the chip first.
 */
int build_new_image(struct flashctx *flash, bool oldcontents_valid, uint8_t *oldcontents, uint8_t *newcontents)
{
	unsigned int start = 0;
	romentry_t *entry;
	unsigned int size = flash->chip->total_size * 1024;

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
			copy_old_content(flash, oldcontents_valid, oldcontents, newcontents, start,
					 size - start);
			break;
		}
		/* For non-included region, copy from old content. */
		if (entry->start > start)
			copy_old_content(flash, oldcontents_valid, oldcontents, newcontents, start,
					 entry->start - start);

		/* If a file name is specified for this region, read the file contents and
		 * overwrite @newcontents in the range specified by @entry. */
		if (entry->file != NULL) {
			if (read_buf_from_file(newcontents + entry->start, entry->end - entry->start + 1,
			    entry->file, "the region's size") != 0)
				return 1;
		}

		/* Skip to location after current romentry. */
		start = entry->end + 1;
		/* Catch overflow. */
		if (!start)
			break;
	}
	return 0;
}
