/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000 Silicon Integrated System Corporation
 * SPDX-FileCopyrightText: 2000 Ronald G. Minnich <rminnich@gmail.com>
 * SPDX-FileCopyrightText: 2005-2009 coresystems GmbH
 * SPDX-FileCopyrightText: 2006-2009 Carl-Daniel Hailfinger
 */

#ifndef __HELPERS_H__
#define __HELPERS_H__

#include <stdint.h>
#include <stddef.h>

uint32_t address_to_bits(uint32_t addr);
unsigned int bitcount(unsigned long a);
#undef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
int max(int a, int b);
int min(int a, int b);
char *strcat_realloc(char *dest, const char *src);
void tolower_string(char *str);
uint8_t reverse_byte(uint8_t x);
void reverse_bytes(uint8_t *dst, const uint8_t *src, size_t length);
#ifdef __MINGW32__
char* strtok_r(char *str, const char *delim, char **nextp);
char *strndup(const char *str, size_t size);
#endif
#if defined(__DJGPP__) || (!defined(__LIBPAYLOAD__) && !defined(HAVE_STRNLEN))
size_t strnlen(const char *str, size_t n);
#endif

#endif /* __HELPERS_H__ */
