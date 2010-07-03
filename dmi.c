/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009,2010 Michael Karcher
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "flash.h"

int has_dmi_support = 0;

#if STANDALONE

/* Stub to indicate missing DMI functionality.
 * has_dmi_support is 0 by default, so nothing to do here.
 * Because dmidecode is not available on all systems, the goal is to implement
 * the DMI subset we need directly in this file.
 */
void dmi_init(void)
{
}

int dmi_match(const char *pattern)
{
	return 0;
}

#else /* STANDALONE */

static const char *dmidecode_names[] = {
	"system-manufacturer",
	"system-product-name",
	"system-version",
	"baseboard-manufacturer",
	"baseboard-product-name",
	"baseboard-version",
};

#define DMI_COMMAND_LEN_MAX 260
static const char *dmidecode_command = "dmidecode";

static char *dmistrings[ARRAY_SIZE(dmidecode_names)];

/* Strings longer than 4096 in DMI are just insane. */
#define DMI_MAX_ANSWER_LEN 4096

static char *get_dmi_string(const char *string_name)
{
	FILE *dmidecode_pipe;
	char *result;
	char answerbuf[DMI_MAX_ANSWER_LEN];
	char commandline[DMI_COMMAND_LEN_MAX + 40];

	snprintf(commandline, sizeof(commandline),
		 "%s -s %s", dmidecode_command, string_name);
	dmidecode_pipe = popen(commandline, "r");
	if (!dmidecode_pipe) {
		msg_perr("DMI pipe open error\n");
		return NULL;
	}
	if (!fgets(answerbuf, DMI_MAX_ANSWER_LEN, dmidecode_pipe)) {
		if(ferror(dmidecode_pipe)) {
			msg_perr("DMI pipe read error\n");
			pclose(dmidecode_pipe);
			return NULL;
		} else {
			answerbuf[0] = 0;	/* Hit EOF */
		}
	}
	/* Toss all output above DMI_MAX_ANSWER_LEN away to prevent
	   deadlock on pclose. */
	while (!feof(dmidecode_pipe))
		getc(dmidecode_pipe);
	if (pclose(dmidecode_pipe) != 0) {
		msg_pinfo("dmidecode execution unsucessfull - continuing "
			"without DMI info\n");
		return NULL;
	}

	/* Chomp trailing newline. */
	if (answerbuf[0] != 0 &&
	    answerbuf[strlen(answerbuf) - 1] == '\n')
		answerbuf[strlen(answerbuf) - 1] = 0;
	msg_pdbg("DMI string %s: \"%s\"\n", string_name, answerbuf);

	result = strdup(answerbuf);
	if (!result)
		puts("WARNING: Out of memory - DMI support fails");

	return result;
}

void dmi_init(void)
{
	int i;
	char *chassis_type;

	has_dmi_support = 1;
	for (i = 0; i < ARRAY_SIZE(dmidecode_names); i++) {
		dmistrings[i] = get_dmi_string(dmidecode_names[i]);
		if (!dmistrings[i]) {
			has_dmi_support = 0;
			return;
		}
	}

	chassis_type = get_dmi_string("chassis-type");
	if (chassis_type && (!strcmp(chassis_type, "Notebook") ||
			     !strcmp(chassis_type, "Portable"))) {
		msg_pdbg("Laptop detected via DMI\n");
		is_laptop = 1;
	}
	free(chassis_type);
}

/**
 * Does an substring/prefix/postfix/whole-string match.
 *
 * The pattern is matched as-is. The only metacharacters supported are '^'
 * at the beginning and '$' at the end. So you can look for "^prefix",
 * "suffix$", "substring" or "^complete string$".
 *
 * @param value The string to check.
 * @param pattern The pattern.
 * @return Nonzero if pattern matches.
 */
static int dmi_compare(const char *value, const char *pattern)
{
	int anchored = 0;
	int patternlen;

	msg_pspew("matching %s against %s\n", value, pattern);
	/* The empty string is part of all strings! */
	if (pattern[0] == 0)
		return 1;

	if (pattern[0] == '^') {
		anchored = 1;
		pattern++;
	}

	patternlen = strlen(pattern);
	if (pattern[patternlen - 1] == '$') {
		int valuelen = strlen(value);
		patternlen--;
		if (patternlen > valuelen)
			return 0;

		/* full string match: require same length */
		if (anchored && (valuelen != patternlen))
			return 0;

		/* start character to make ends match */
		value += valuelen - patternlen;
		anchored = 1;
	}

	if (anchored)
		return strncmp(value, pattern, patternlen) == 0;
	else
		return strstr(value, pattern) != NULL;
}

int dmi_match(const char *pattern)
{
	int i;

	if (!has_dmi_support)
		return 0;

	for (i = 0; i < ARRAY_SIZE(dmidecode_names); i++)
		if (dmi_compare(dmistrings[i], pattern))
			return 1;

	return 0;
}

#endif /* STANDALONE */
