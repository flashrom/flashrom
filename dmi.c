/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000-2002 Alan Cox <alan@redhat.com>
 * SPDX-FileCopyrightText: 2002-2010 Jean Delvare <khali@linux-fr.org>
 * SPDX-FileCopyrightText: 2009,2010 Michael Karcher
 * SPDX-FileCopyrightText: 2011-2013 Stefan Tauner
 */

#include "dmi.h"

#include "platform/string.h"
#include "flash.h"
#include "hwaccess_physmap.h"
#include "log.h"
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>


/* Strings longer than 4096 in DMI are just insane. */
#define DMI_MAX_ANSWER_LEN 4096

static bool g_has_dmi_support = false;

bool dmi_is_supported(void)
{
	return g_has_dmi_support;
}

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
 *	- 0: in all likelihood not a laptop
 *	- 1: in all likelihood a laptop
 *	- 2: chassis-type is not specific enough
 * A full list of chassis types can be found in the System Management BIOS
 * (SMBIOS) Specification 3.10.0 section 7.4.1 "Chassis Types" at
 * https://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.10.0.pdf
 * The types below are the most common ones.
 */
static const struct {
	uint8_t type;
	uint8_t is_laptop;
	const char *name;
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
	{0x19, 0, "Multi-system"}, /* used by Supermicro (X7DWT) */
	{0x1f, 1, "Convertible"},
	{0x20, 1, "Detachable"},
	{0x21, 0, "IoT Gateway"},
	{0x22, 0, "Embedded PC"},
	{0x23, 0, "Mini PC"},
	{0x24, 0, "Stick PC"},
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

static int dmi_chassis_type(uint8_t code)
{
	unsigned int i;
	code &= 0x7f; /* bits 6:0 are chassis type, 7th bit is the lock bit */
	int is_laptop = 2;
	for (i = 0; i < ARRAY_SIZE(dmi_chassis_types); i++) {
		if (code == dmi_chassis_types[i].type) {
			msg_pdbg("DMI string chassis-type: \"%s\"\n", dmi_chassis_types[i].name);
			is_laptop = dmi_chassis_types[i].is_laptop;
			break;
		}
	}
	return is_laptop;
}

/*
 * Walk the structure table at physical address base. SMBIOS 3 entry points do
 * not carry a structure count, callers pass UINT_MAX for num in that case and
 * the end-of-table structure (type 127) or the length limit stops the walk.
 */
static void dmi_table(uintptr_t base, size_t len, unsigned int num, int *is_laptop)
{
	unsigned int i = 0, j = 0;

	uint8_t *dmi_table_mem = physmap_ro("DMI Table", base, len);
	if (dmi_table_mem == ERROR_PTR) {
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
		 * Better stop at this point, and let the user know their
		 * table is broken.
		 */
		if (data[1] < 4 || data + data[1] >= limit) {
			msg_perr("DMI table is broken (bogus header)!\n");
			break;
		}

		if (data[0] == 127) /* End-of-table */
			break;

		if(data[0] == 3) {
			if (data + 5 < limit)
				*is_laptop = dmi_chassis_type(data[5]);
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

/* Longest entry point structure: the SMBIOS 2.1 one is 0x1f bytes, the SMBIOS 3.0 one 0x18. */
#define SMBIOS_EP_MAX_LEN 0x20

/* SMBIOS 2.1 entry point ("_SM_"), SMBIOS spec section 5.2.1. Returns 0 on success.
 * Some 2.1 implementations report a length of 0x1e, following an erratum in that spec. */
static int smbios_decode(const uint8_t *buf, size_t len, int *is_laptop)
{
	if (len < 0x1e || buf[0x05] < 0x1e || buf[0x05] > len ||
	    !dmi_checksum(buf, buf[0x05]) ||
	    (memcmp(buf + 0x10, "_DMI_", 5) != 0) ||
	    !dmi_checksum(buf + 0x10, 0x0F))
			return 1;

	dmi_table(mmio_readl(buf + 0x18), mmio_readw(buf + 0x16), mmio_readw(buf + 0x1C), is_laptop);

	return 0;
}

/* SMBIOS 3.0 entry point ("_SM3_"), SMBIOS spec section 5.2.2. Returns 0 on success. */
static int smbios3_decode(const uint8_t *buf, size_t len, int *is_laptop)
{
	if (len < 0x18 || buf[0x06] < 0x18 || buf[0x06] > len || !dmi_checksum(buf, buf[0x06]))
		return 1;

	const uint64_t table = (uint64_t)mmio_readl(buf + 0x14) << 32 | mmio_readl(buf + 0x10);
	const uint32_t table_len = mmio_readl(buf + 0x0C);

	if (table > UINTPTR_MAX) {
		msg_pwarn("SMBIOS table at 0x%" PRIx64 " is out of reach on this platform.\n", table);
		return 1;
	}

	dmi_table((uintptr_t)table, table_len, UINT_MAX, is_laptop);

	return 0;
}

/* Legacy "_DMI_" anchor, also the second half of a 2.1 entry point. Returns 0 on success. */
static int legacy_decode(const uint8_t *buf, size_t len, int *is_laptop)
{
	if (len < 0x0F || !dmi_checksum(buf, 0x0F))
		return 1;

	dmi_table(mmio_readl(buf + 0x08), mmio_readw(buf + 0x06), mmio_readw(buf + 0x0C), is_laptop);

	return 0;
}

/* Decode whichever entry point anchor starts at buf. Returns 0 on success. */
static int dmi_decode_entry_point(const uint8_t *buf, size_t len, int *is_laptop)
{
	if (len >= 5 && memcmp(buf, "_SM3_", 5) == 0)
		return smbios3_decode(buf, len, is_laptop);
	if (len >= 4 && memcmp(buf, "_SM_", 4) == 0)
		return smbios_decode(buf, len, is_laptop);
	if (len >= 5 && memcmp(buf, "_DMI_", 5) == 0)
		return legacy_decode(buf, len, is_laptop);
	return 1;
}

#if defined(__FreeBSD__)
#include <errno.h>
#include <kenv.h>

/* Decode the entry point recorded by the loader. Returns 0 on success. */
static int dmi_fill_from_loader(int *is_laptop)
{
	char value[32] = { 0 };

	if (kenv(KENV_GET, "hint.smbios.0.mem", value, sizeof(value) - 1) < 0) {
		msg_pdbg("The loader did not record an SMBIOS entry point (%s).\n", strerror(errno));
		return 1;
	}

	char *end;
	errno = 0;
	const unsigned long long addr = strtoull(value, &end, 0);
	if (errno != 0 || end == value || *end != '\0' || addr > UINTPTR_MAX) {
		msg_pwarn("Ignoring bogus hint.smbios.0.mem=\"%s\".\n", value);
		return 1;
	}
	msg_pdbg("SMBIOS entry point at 0x%llx according to the loader.\n", addr);

	uint8_t *ep = physmap_ro("SMBIOS entry point", (uintptr_t)addr, SMBIOS_EP_MAX_LEN);
	if (ep == ERROR_PTR)
		return 1;

	const int ret = dmi_decode_entry_point(ep, SMBIOS_EP_MAX_LEN, is_laptop);
	physunmap(ep, SMBIOS_EP_MAX_LEN);
	if (ret)
		msg_pwarn("The SMBIOS entry point recorded by the loader is invalid.\n");
	return ret;
}
#endif

/* Scan the legacy BIOS range for an anchor string. Returns 0 on success. */
static int dmi_fill_from_legacy_range(int *is_laptop)
{
	size_t fp;
	uint8_t *dmi_mem;
	int ret = 1;

	dmi_mem = physmap_ro("DMI", 0xF0000, 0x10000);
	if (dmi_mem == ERROR_PTR)
		return ret;

	for (fp = 0; fp <= 0xFFF0; fp += 16) {
		if (dmi_decode_entry_point(dmi_mem + fp, 0x10000 - fp, is_laptop) == 0) {
			ret = 0;
			break;
		}
	}
	if (ret)
		msg_pinfo("No DMI table found.\n");
	physunmap(dmi_mem, 0x10000);
	return ret;
}

static int dmi_fill(int *is_laptop)
{
	msg_pdbg("Using Internal DMI decoder.\n");
	/* There are two ways specified to gain access to the SMBIOS table:
	 * - EFI's configuration table contains a pointer to the SMBIOS table.
	 *   EFI's SMBIOS GUID is: {0xeb9d2d31,0x2d88,0x11d3,0x9a,0x16,0x0,0x90,0x27,0x3f,0xc1,0x4d}
	 * - Scanning physical memory address range 0x000F0000h to 0x000FFFFF for the anchor-string(s). */
#if defined(__FreeBSD__)
	if (dmi_fill_from_loader(is_laptop) == 0)
		return 0;
#endif
	return dmi_fill_from_legacy_range(is_laptop);
}

#else /* CONFIG_INTERNAL_DMI */

#define DMI_COMMAND_LEN_MAX 300
#if IS_WINDOWS
static const char *dmidecode_command = "dmidecode.exe 2>NUL";
#else
static const char *dmidecode_command = "dmidecode 2>/dev/null";
#endif

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
			}
			answerbuf[0] = 0;	/* Hit EOF */
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

static int dmi_fill(int *is_laptop)
{
	unsigned int i;
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
	*is_laptop = 2;
	for (i = 0; i < ARRAY_SIZE(dmi_chassis_types); i++) {
		if (strcasecmp(chassis_type, dmi_chassis_types[i].name) == 0) {
			*is_laptop = dmi_chassis_types[i].is_laptop;
			break;
		}
	}
	free(chassis_type);
	return 0;
}

#endif /* CONFIG_INTERNAL_DMI */

static int dmi_shutdown(void *data)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(dmi_strings); i++) {
		free(dmi_strings[i].value);
		dmi_strings[i].value = NULL;
	}
	g_has_dmi_support = false;
	return 0;
}

void dmi_init(int *is_laptop)
{
	/* Register shutdown function before we allocate anything. */
	if (register_shutdown(dmi_shutdown, NULL)) {
		msg_pwarn("Warning: Could not register DMI shutdown function - continuing without DMI info.\n");
		return;
	}

	/* dmi_fill fills the dmi_strings array, and if possible set the is_laptop parameter. */
	if (dmi_fill(is_laptop) != 0)
		return;

	switch (*is_laptop) {
	case 1:
		msg_pdbg("Laptop detected via DMI.\n");
		break;
	case 2:
		msg_pdbg("DMI chassis-type is not specific enough.\n");
		break;
	}

	g_has_dmi_support = true;
	unsigned int i;
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
	bool anchored = false;
	int patternlen;

	msg_pspew("matching %s against %s\n", value, pattern);
	/* The empty string is part of all strings! */
	if (pattern[0] == 0)
		return 1;

	if (pattern[0] == '^') {
		anchored = true;
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
		anchored = true;
	}

	if (anchored)
		return strncmp(value, pattern, patternlen) == 0;
	else
		return strstr(value, pattern) != NULL;
}

int dmi_match(const char *pattern)
{
	unsigned int i;

	if (!dmi_is_supported())
		return 0;

	for (i = 0; i < ARRAY_SIZE(dmi_strings); i++) {
		if (dmi_strings[i].value == NULL)
			continue;

		if (dmi_compare(dmi_strings[i].value, pattern))
			return 1;
	}

	return 0;
}
