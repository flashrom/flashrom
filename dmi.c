/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000-2002 Alan Cox <alan@redhat.com>
 * Copyright (C) 2002-2010 Jean Delvare <khali@linux-fr.org>
 * Copyright (C) 2009,2010 Michael Karcher
 * Copyright (C) 2011-2013 Stefan Tauner
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

/* strnlen is in POSIX but was a GNU extension up to glibc 2.10 */
#if (__GLIBC__ == 2 && __GLIBC_MINOR__ < 10) || __GLIBC__ < 2
#define _GNU_SOURCE
#else
#define _POSIX_C_SOURCE 200809L
#endif

#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "flash.h"
#include "programmer.h"

#if defined(__i386__) || defined(__x86_64__)

/* Enable SMBIOS decoding. Currently legacy DMI decoding is enough. */
#define SM_SUPPORT 0

/* Strings longer than 4096 in DMI are just insane. */
#define DMI_MAX_ANSWER_LEN 4096

int has_dmi_support = 0;

static struct {
	const char *const keyword;
	const uint8_t type;
	const uint8_t offset;
	char *value;
} dmi_strings[] = {
	{ "system-manufacturer", 1, 0x04, NULL },
	{ "system-product-name", 1, 0x05, NULL },
	{ "system-version", 1, 0x06, NULL },
	{ "baseboard-manufacturer", 2, 0x04, NULL },
	{ "baseboard-product-name", 2, 0x05, NULL },
	{ "baseboard-version", 2, 0x06, NULL },
};

/* This list is used to identify supposed laptops. The is_laptop field has the
 * following meaning:
 * 	- 0: in all likelihood not a laptop
 * 	- 1: in all likelihood a laptop
 * 	- 2: chassis-type is not specific enough
 * A full list of chassis types can be found in the System Management BIOS
 * (SMBIOS) Reference Specification 2.7.0 section 7.4.1 "Chassis Types" at
 * http://www.dmtf.org/sites/default/files/standards/documents/DSP0134_2.7.0.pdf
 * The types below are the most common ones.
 */
static const struct {
	uint8_t type;
	uint8_t is_laptop;
	char *name;
} dmi_chassis_types[] = {
	{0x01, 2, "Other"},
	{0x02, 2, "Unknown"},
	{0x03, 0, "Desktop"},
	{0x04, 0, "Low Profile Desktop"},
	{0x06, 0, "Mini Tower"},
	{0x07, 0, "Tower"},
	{0x08, 1, "Portable"},
	{0x09, 1, "Laptop"},
	{0x0a, 1, "Notebook"},
	{0x0b, 1, "Hand Held"},
	{0x0e, 1, "Sub Notebook"},
	{0x11, 0, "Main Server Chassis"},
	{0x17, 0, "Rack Mount Chassis"},
	{0x18, 0, "Sealed-case PC"}, /* used by Supermicro (X8SIE) */
};

#if CONFIG_INTERNAL_DMI == 1
static bool dmi_checksum(const uint8_t * const buf, size_t len)
{
	uint8_t sum = 0;
	size_t a;

	for (a = 0; a < len; a++)
		sum += buf[a];
	return (sum == 0);
}

/** Retrieve a DMI string.
 *
 * See SMBIOS spec. section 6.1.3 "Text strings".
 * The table will be unmapped ASAP, hence return a duplicated & sanitized string that needs to be freed later.
 *
 * \param buf		the buffer to search through (usually appended directly to a DMI structure)
 * \param string_id	index of the string to look for
 * \param limit		pointer to the first byte beyond \em buf
 */
static char *dmi_string(const char *buf, uint8_t string_id, const char *limit)
{
	size_t i, len;

	if (string_id == 0)
		return strdup("Not Specified");

	while (string_id > 1 && string_id--) {
		if (buf >= limit) {
			msg_perr("DMI table is broken (string portion out of bounds)!\n");
			return strdup("<OUT OF BOUNDS>");
		}
		buf += strnlen(buf, limit - buf) + 1;
	}

	if (!*buf) /* as long as the current byte we're on isn't null */
		return strdup("<BAD INDEX>");

	len = strnlen(buf, limit - buf);
	char *newbuf = malloc(len + 1);
	if (newbuf == NULL) {
		msg_perr("Out of memory!\n");
		return NULL;
	}

	/* fix junk bytes in the string */
	for (i = 0; i < len && buf[i] != '\0'; i++) {
		if (isprint((unsigned char)buf[i]))
			newbuf[i] = buf[i];
		else
			newbuf[i] = ' ';
	}
	newbuf[i] = '\0';

	return newbuf;
}

static void dmi_chassis_type(uint8_t code)
{
	int i;
	code &= 0x7f; /* bits 6:0 are chassis type, 7th bit is the lock bit */
	is_laptop = 2;
	for (i = 0; i < ARRAY_SIZE(dmi_chassis_types); i++) {
		if (code == dmi_chassis_types[i].type) {
			msg_pdbg("DMI string chassis-type: \"%s\"\n", dmi_chassis_types[i].name);
			is_laptop = dmi_chassis_types[i].is_laptop;
			break;
		}
	}
}

static void dmi_table(uint32_t base, uint16_t len, uint16_t num)
{
	int i = 0, j = 0;

	uint8_t *dmi_table_mem = physmap_ro("DMI Table", base, len);
	if (dmi_table_mem == NULL) {
		msg_perr("Unable to access DMI Table\n");
		return;
	}

	uint8_t *data = dmi_table_mem;
	uint8_t *limit = dmi_table_mem + len;

	/* SMBIOS structure header is always 4 B long and contains:
	 *  - uint8_t type;	// see dmi_chassis_types's type
	 *  - uint8_t length;	// data section w/ header w/o strings
	 *  - uint16_t handle;
	 */
	while (i < num && data + 4 < limit) {
		/* - If a short entry is found (less than 4 bytes), not only it
		 *   is invalid, but we cannot reliably locate the next entry.
		 * - If the length value indicates that this structure spreads
		 *   across the table border, something is fishy too.
		 * Better stop at this point, and let the user know his/her
		 * table is broken.
		 */
		if (data[1] < 4 || data + data[1] >= limit) {
			msg_perr("DMI table is broken (bogus header)!\n");
			break;
		}

		if(data[0] == 3) {
			if (data + 5 < limit)
				dmi_chassis_type(data[5]);
			else /* the table is broken, but laptop detection is optional, hence continue. */
				msg_pwarn("DMI table is broken (chassis_type out of bounds)!\n");
		} else
			for (j = 0; j < ARRAY_SIZE(dmi_strings); j++) {
				uint8_t offset = dmi_strings[j].offset;
				uint8_t type = dmi_strings[j].type;

				if (data[0] != type)
					continue;

				if (data[1] <= offset || data + offset >= limit) {
					msg_perr("DMI table is broken (offset out of bounds)!\n");
					goto out;
				}

				dmi_strings[j].value = dmi_string((const char *)(data + data[1]), data[offset],
								  (const char *)limit);
			}
		/* Find next structure by skipping data and string sections */
		data += data[1];
		while (data + 1 <= limit) {
			if (data[0] == 0 && data[1] == 0)
				break;
			data++;
		}
		data += 2;
		i++;
	}
out:
	physunmap(dmi_table_mem, len);
}

#if SM_SUPPORT
static int smbios_decode(uint8_t *buf)
{
	/* TODO: other checks mentioned in the conformance guidelines? */
	if (!dmi_checksum(buf, buf[0x05]) ||
	    (memcmp(buf + 0x10, "_DMI_", 5) != 0) ||
	    !dmi_checksum(buf + 0x10, 0x0F))
			return 0;

	dmi_table(mmio_readl(buf + 0x18), mmio_readw(buf + 0x16), mmio_readw(buf + 0x1C));

	return 1;
}
#endif

static int legacy_decode(uint8_t *buf)
{
	if (!dmi_checksum(buf, 0x0F))
		return 1;

	dmi_table(mmio_readl(buf + 0x08), mmio_readw(buf + 0x06), mmio_readw(buf + 0x0C));

	return 0;
}

int dmi_fill(void)
{
	size_t fp;
	uint8_t *dmi_mem;
	int ret = 1;

	msg_pdbg("Using Internal DMI decoder.\n");
	/* There are two ways specified to gain access to the SMBIOS table:
	 * - EFI's configuration table contains a pointer to the SMBIOS table. On linux it can be obtained from
	 *   sysfs. EFI's SMBIOS GUID is: {0xeb9d2d31,0x2d88,0x11d3,0x9a,0x16,0x0,0x90,0x27,0x3f,0xc1,0x4d}
	 * - Scanning physical memory address range 0x000F0000h to 0x000FFFFF for the anchor-string(s). */
	dmi_mem = physmap_ro("DMI", 0xF0000, 0x10000);
	if (dmi_mem == ERROR_PTR)
		return ret;

	for (fp = 0; fp <= 0xFFF0; fp += 16) {
#if SM_SUPPORT
		if (memcmp(dmi_mem + fp, "_SM_", 4) == 0 && fp <= 0xFFE0) {
			if (smbios_decode(dmi_mem + fp)) // FIXME: length check
				goto out;
		} else
#endif
		if (memcmp(dmi_mem + fp, "_DMI_", 5) == 0)
			if (legacy_decode(dmi_mem + fp) == 0) {
				ret = 0;
				goto out;
			}
	}
	msg_pinfo("No DMI table found.\n");
out:
	physunmap(dmi_mem, 0x10000);
	return ret;
}

#else /* CONFIG_INTERNAL_DMI */

#define DMI_COMMAND_LEN_MAX 300
static const char *dmidecode_command = "dmidecode";

static char *get_dmi_string(const char *string_name)
{
	FILE *dmidecode_pipe;
	char *result;
	char answerbuf[DMI_MAX_ANSWER_LEN];
	char commandline[DMI_COMMAND_LEN_MAX];

	snprintf(commandline, sizeof(commandline),
		 "%s -s %s", dmidecode_command, string_name);
	dmidecode_pipe = popen(commandline, "r");
	if (!dmidecode_pipe) {
		msg_perr("Opening DMI pipe failed!\n");
		return NULL;
	}

	/* Kill lines starting with '#', as recent dmidecode versions
	 * have the quirk to emit a "# SMBIOS implementations newer..."
	 * message even on "-s" if the SMBIOS declares a
	 * newer-than-supported version number, while it *should* only print
	 * the requested string.
	 */
	do {
		if (!fgets(answerbuf, DMI_MAX_ANSWER_LEN, dmidecode_pipe)) {
			if (ferror(dmidecode_pipe)) {
				msg_perr("DMI pipe read error\n");
				pclose(dmidecode_pipe);
				return NULL;
			} else {
				answerbuf[0] = 0;	/* Hit EOF */
			}
		}
	} while (answerbuf[0] == '#');

	/* Discard all output exceeding DMI_MAX_ANSWER_LEN to prevent deadlock on pclose. */
	while (!feof(dmidecode_pipe))
		getc(dmidecode_pipe);
	if (pclose(dmidecode_pipe) != 0) {
		msg_pwarn("dmidecode execution unsuccessful - continuing without DMI info\n");
		return NULL;
	}

	/* Chomp trailing newline. */
	if (answerbuf[0] != 0 && answerbuf[strlen(answerbuf) - 1] == '\n')
		answerbuf[strlen(answerbuf) - 1] = 0;

	result = strdup(answerbuf);
	if (result == NULL)
		msg_pwarn("Warning: Out of memory - DMI support fails");

	return result;
}

int dmi_fill(void)
{
	int i;
	char *chassis_type;

	msg_pdbg("Using External DMI decoder.\n");
	for (i = 0; i < ARRAY_SIZE(dmi_strings); i++) {
		dmi_strings[i].value = get_dmi_string(dmi_strings[i].keyword);
		if (dmi_strings[i].value == NULL)
			return 1;
	}

	chassis_type = get_dmi_string("chassis-type");
	if (chassis_type == NULL)
		return 0; /* chassis-type handling is optional anyway */

	msg_pdbg("DMI string chassis-type: \"%s\"\n", chassis_type);
	is_laptop = 2;
	for (i = 0; i < ARRAY_SIZE(dmi_chassis_types); i++) {
		if (strcasecmp(chassis_type, dmi_chassis_types[i].name) == 0) {
			is_laptop = dmi_chassis_types[i].is_laptop;
			break;
		}
	}
	free(chassis_type);
	return 0;
}

#endif /* CONFIG_INTERNAL_DMI */

static int dmi_shutdown(void *data)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(dmi_strings); i++) {
		free(dmi_strings[i].value);
		dmi_strings[i].value = NULL;
	}
	return 0;
}

void dmi_init(void)
{
	/* Register shutdown function before we allocate anything. */
	if (register_shutdown(dmi_shutdown, NULL)) {
		msg_pwarn("Warning: Could not register DMI shutdown function - continuing without DMI info.\n");
		return;
	}

	/* dmi_fill fills the dmi_strings array, and if possible sets the global is_laptop variable. */
	if (dmi_fill() != 0)
		return;

	switch (is_laptop) {
	case 1:
		msg_pdbg("Laptop detected via DMI.\n");
		break;
	case 2:
		msg_pdbg("DMI chassis-type is not specific enough.\n");
		break;
	}

	has_dmi_support = 1;
	int i;
	for (i = 0; i < ARRAY_SIZE(dmi_strings); i++) {
		msg_pdbg("DMI string %s: \"%s\"\n", dmi_strings[i].keyword,
			 (dmi_strings[i].value == NULL) ? "" : dmi_strings[i].value);
	}
}

/**
 * Does an substring/prefix/postfix/whole-string match.
 *
 * The pattern is matched as-is. The only metacharacters supported are '^'
 * at the beginning and '$' at the end. So you can look for "^prefix",
 * "suffix$", "substring" or "^complete string$".
 *
 * @param value The non-NULL string to check.
 * @param pattern The non-NULL pattern.
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

	for (i = 0; i < ARRAY_SIZE(dmi_strings); i++) {
		if (dmi_strings[i].value == NULL)
			continue;

		if (dmi_compare(dmi_strings[i].value, pattern))
			return 1;
	}

	return 0;
}

#endif // defined(__i386__) || defined(__x86_64__)
