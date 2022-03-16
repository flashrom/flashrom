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

/* strndup is a POSIX function not present in MinGW */
char *strndup(const char *src, size_t maxlen)
{
	char *retbuf;
	size_t len;
	for (len = 0; len < maxlen; len++)
		if (src[len] == '\0')
			break;
	if ((retbuf = malloc(1 + len)) != NULL) {
		memcpy(retbuf, src, len);
		retbuf[len] = '\0';
	}
	return retbuf;
}
#endif

/* There is no strnlen in DJGPP */
#if defined(__DJGPP__) || (!defined(__LIBPAYLOAD__) && !defined(HAVE_STRNLEN))
size_t strnlen(const char *str, size_t n)
{
	size_t i;
	for (i = 0; i < n && str[i] != '\0'; i++)
		;
	return i;
}
#endif
