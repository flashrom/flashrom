/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000 Silicon Integrated System Corporation
 * SPDX-FileCopyrightText: 2000 Ronald G. Minnich <rminnich@gmail.com>
 * SPDX-FileCopyrightText: 2000-2002 Alan Cox <alan@redhat.com>
 * SPDX-FileCopyrightText: 2002-2010 Jean Delvare <khali@linux-fr.org>
 * SPDX-FileCopyrightText: 2005-2009 coresystems GmbH
 * SPDX-FileCopyrightText: 2006-2009 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2009,2010 Michael Karcher
 * SPDX-FileCopyrightText: 2011-2013 Stefan Tauner
 * SPDX-FileCopyrightText: 2026 Antonio Vázquez Blanco
 *
 * Platform independent interface to handle strings handling.
 */

#include "platform/string.h"


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

