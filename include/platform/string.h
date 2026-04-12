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


#ifndef __PLATFORM_STRING_H__
#define __PLATFORM_STRING_H__

/* strnlen is in POSIX but was a GNU extension up to glibc 2.10 */
#if (__GLIBC__ == 2 && __GLIBC_MINOR__ < 10) || __GLIBC__ < 2
#define _GNU_SOURCE
#else
#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include <string.h>

#ifdef __MINGW32__
char* strtok_r(char *str, const char *delim, char **nextp);
char *strndup(const char *str, size_t size);
#endif

#if defined(__DJGPP__) || (!defined(__LIBPAYLOAD__) && !defined(HAVE_STRNLEN))
size_t strnlen(const char *str, size_t n);
#endif

#endif /* ___PLATFOMR_STRING_H__ */

