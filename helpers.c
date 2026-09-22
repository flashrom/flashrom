/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2009-2010 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2013 Stefan Tauner
 */

#include "helpers.h"

#include "log.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include "platform/string.h"

/* Returns the minimum number of bits needed to represent the given address.
 * FIXME: use mind-blowing implementation. */
uint32_t address_to_bits(uint32_t addr)
{
	unsigned int lzb = 0;
	while (((1u << (31 - lzb)) & ~addr) != 0)
		lzb++;
	return 32 - lzb;
}

unsigned int bitcount(unsigned long a)
{
	unsigned int i = 0;
	for (; a != 0; a >>= 1)
		if (a & 1)
			i++;
	return i;
}

int max(int a, int b)
{
	return (a > b) ? a : b;
}

int min(int a, int b)
{
	return (a < b) ? a : b;
}

char *strcat_realloc(char *dest, const char *src)
{
	dest = realloc(dest, strlen(dest) + strlen(src) + 1);
	if (!dest) {
		msg_gerr("Out of memory!\n");
		return NULL;
	}
	strcat(dest, src);
	return dest;
}

void tolower_string(char *str)
{
	for (; *str != '\0'; str++)
		*str = (char)tolower((unsigned char)*str);
}

uint8_t reverse_byte(uint8_t x)
{
	x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
	x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
	x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);

	return x;
}

void reverse_bytes(uint8_t *dst, const uint8_t *src, size_t length)
{
	size_t i;

	for (i = 0; i < length; i++)
		dst[i] = reverse_byte(src[i]);
}

/* Parse a voltage= parameter value into millivolts. Accepts an optional
 * decimal point ("," or "."), and an optional "V", "mV" or "millivolt" unit.
 * Might be useful for various USB devices. Returns -1 on error. */
int parse_voltage(char *voltage)
{
	char *tmp = NULL;
	int i;
	int millivolt = 0, fraction = 0;

	if (!voltage || !strlen(voltage)) {
		msg_perr("Empty voltage= specified.\n");
		return -1;
	}
	millivolt = (int)strtol(voltage, &tmp, 0);
	voltage = tmp;
	/* Handle "," and "." as decimal point. Everything after it is assumed
	 * to be in decimal notation.
	 */
	if ((*voltage == '.') || (*voltage == ',')) {
		voltage++;
		for (i = 0; i < 3; i++) {
			fraction *= 10;
			/* Don't advance if the current character is invalid,
			 * but continue multiplying.
			 */
			if ((*voltage < '0') || (*voltage > '9'))
				continue;
			fraction += *voltage - '0';
			voltage++;
		}
		/* Throw away remaining digits. */
		voltage += strspn(voltage, "0123456789");
	}
	/* The remaining string must be empty or "mV" or "V". */
	tolower_string(voltage);

	/* No unit or "V". */
	if ((*voltage == '\0') || !strcmp(voltage, "v")) {
		millivolt *= 1000;
		millivolt += fraction;
	} else if (!strcmp(voltage, "mv") ||
		   !strcmp(voltage, "milliv")) {
		/* No adjustment. fraction is discarded. */
	} else {
		/* Garbage at the end of the string. */
		msg_perr("Garbage voltage= specified.\n");
		return -1;
	}
	return millivolt;
}

/* Parse a usbpath= parameter value. Accepts input in the format:
 * bus-port.port.port... up to a total depth of max_ports including bus.
 * Bus must be an integer between 0 and 255.
 * Port numbers must be integers between 1 and 255.
 * Populates usbpath starting with bus, port1, port2, etc and returns total depth.
 * Might be useful for various USB devices. Returns -1 on error. */
int parse_usbpath(const char* arg, uint8_t* usbpath, int max_ports)
{
	unsigned long val;
	int count = 0;
	char *p, *end;

	if (!isdigit((unsigned char)*arg))
		return -1;

	errno = 0;
	val = strtoul(arg, &end, 10);
	if (errno || *end != '-' || 255 < val)
		return -1;

	usbpath[count++] = (uint8_t)val;
	end = end + 1;

	while (*end && count < max_ports) {
		p = end;
		if (!isdigit((unsigned char)*p))
			return -1;

		val = strtoul(p, &end, 10);
		if (errno || val < 1 || 255 < val)
			return -1;

		usbpath[count++] = (uint8_t)val;

		// if there's more stuff, it had better be '.' delimited
		if (*end && *end != '.') {
			return -1;
		} else if (*end) {
			// skip the '.'
			end = end + 1;
			// can't end on a trailing '.'
			if (!*end)
				return -1;
		}
	}
	// still stuff left to parse but we ran out of space
	if (*end)
		return -1;

	// must have at least bus + one port
	if (count < 2)
		return -1;

	return count;
}