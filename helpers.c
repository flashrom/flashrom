/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009-2010, 2013 Carl-Daniel Hailfinger
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

/** Parse a \em possibly quoted string.
 *
 * \a startp points to the string which should be parsed. If the string does not start with a quote, it is
 * terminated at the first character contained in \a delimiters by replacing it with '\0'. If the string starts
 * with a quote, it is terminated at the second quote by replacing it with '\0'. In the latter case a character
 * contained in \a delimiters has to follow the terminating quote.
 *
 * If \a delimiters is NULL an empty set is assumed.
 *
 * After returning \a startp will point to a string that is either the first quoted part of the
 * original string with the quotation marks removed, or the first word of that string before any delimiter.
 *
 * If \a endp is not NULL it will be set to point to the first character after the parsed string
 * (i.e. a delimiter), or to the '\0' at the end of the string if there are no more subsequent characters.
 *
 * @param start		Points to the input string.
 * @param end		Is set to the first char following the input string (possibly NULL).
 * @param delimiters	Set of characters which terminate a string (possibly NULL).
 * @return		-1 if a quotation mark is not matched,
 *			0 on success,
 *			1 if the parsed string is empty.
 */
int unquote_string(char **startp, char **endp, const char *delimiters)
{
	msg_gspew("unquoting \'%s\'\n", *startp);

	if (delimiters == NULL)
		delimiters = "";

	char *end;
	if (**startp == '"') {
		(*startp)++;
		size_t len = strcspn(*startp, "\"");
		end = *startp + len + 1;
		/* Catch unclosed quotes and string not followed immediately by delimiter or end-of-string. */
		if ((*startp)[len] == '\0' && strcspn(end, delimiters) != 0)
			return -1;

		/* Eliminate closing quote. */
		(*startp)[len] = '\0';
	} else {
		size_t len = strcspn(*startp, delimiters);
		/* Check for (forbidden) quotes in the middle. */
		if (strcspn(*startp, "\"") < len)
			return -1;
		end = *startp + len;
	}

	/* end points to the first character after the (possibly quoted) string, i.e. the delimiter or \0. */
	if (*end != '\0') {
		*end = '\0';
		end++;
	}
	if (endp != NULL)
		*endp = end;

	if (strlen(*startp) == 0) {
		*startp = '\0';
		return 1;
	}

	msg_gspew("%s: start=\'%s\', end=\'%s\'\n", __func__, *startp, end);
	return 0;
}
