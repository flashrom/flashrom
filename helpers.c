/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009-2010 Carl-Daniel Hailfinger
 * Copyright (C) 2013 Stefan Tauner
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "flash.h"

/* Returns the minimum number of bits needed to represent the given address.
 * FIXME: use mind-blowing implementation. */
uint32_t address_to_bits(uint32_t addr)
{
	unsigned int lzb = 0;
	while (((1 << (31 - lzb)) & ~addr) != 0)
		lzb++;
	return 32 - lzb;
}

int bitcount(unsigned long a)
{
	int i = 0;
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

/* FIXME: Find a better solution for MinGW. Maybe wrap strtok_s (C11) if it becomes available */
#ifdef __MINGW32__
char* strtok_r(char *str, const char *delim, char **nextp)
{
	if (str == NULL)
		str = *nextp;

	str += strspn(str, delim); /* Skip leading delimiters */
	if (*str == '\0')
		return NULL;

	char *ret = str;
	str += strcspn(str, delim); /* Find end of token */
	if (*str != '\0')
		*str++ = '\0';

	*nextp = str;
	return ret;
}
#endif

/* There is no strnlen in DJGPP */
#if defined(__DJGPP__)
size_t strnlen(const char *str, size_t n)
{
	size_t i;
	for (i = 0; i < n && str[i] != '\0'; i++)
		;
	return i;
}
#endif
