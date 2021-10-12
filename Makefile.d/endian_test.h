/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2011 Carl-Daniel Hailfinger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * This file determinate the target endian. It should only be used my the Makefile
 */

#if defined (__i386__) || defined (__x86_64__) || defined(__amd64__)
/* All x86 is little-endian. */
#define __FLASHROM_LITTLE_ENDIAN__ 1
#elif defined (__mips) || defined (__mips__) || defined (__MIPS__) || defined (mips)
/* MIPS can be either endian. */
#if defined (__MIPSEL) || defined (__MIPSEL__) || defined (_MIPSEL) || defined (MIPSEL)
#define __FLASHROM_LITTLE_ENDIAN__ 1
#elif defined (__MIPSEB) || defined (__MIPSEB__) || defined (_MIPSEB) || defined (MIPSEB)
#define __FLASHROM_BIG_ENDIAN__ 1
#endif
#elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || \
      defined(__ppc__) || defined(__ppc64__) || defined(_M_PPC) || defined(_ARCH_PPC) || \
      defined(_ARCH_PPC64) || defined(__ppc)
/* PowerPC can be either endian. */
#if defined (_BIG_ENDIAN) || defined (__BIG_ENDIAN__)
#define __FLASHROM_BIG_ENDIAN__ 1
#elif defined (_LITTLE_ENDIAN) || defined (__LITTLE_ENDIAN__)
#define __FLASHROM_LITTLE_ENDIAN__ 1
#endif
#elif defined(__arm__) || defined(__TARGET_ARCH_ARM) || defined(_ARM) || defined(_M_ARM) || defined(__arm) || \
      defined(__aarch64__)
/* ARM can be either endian. */
#if defined (__ARMEB__) || defined (__BIG_ENDIAN__)
#define __FLASHROM_BIG_ENDIAN__ 1
#elif defined (__ARMEL__) || defined (__LITTLE_ENDIAN__)
#define __FLASHROM_LITTLE_ENDIAN__ 1
#endif
#elif defined (__sparc__) || defined (__sparc)
/* SPARC is big endian in general (but allows to access data in little endian too). */
#define __FLASHROM_BIG_ENDIAN__ 1
#elif defined(__arc__)
#if defined(__BIG_ENDIAN__)
#define __FLASHROM_BIG_ENDIAN__ 1
#else
#define __FLASHROM_LITTLE_ENDIAN__ 1
#endif
#endif

#if !defined (__FLASHROM_BIG_ENDIAN__) && !defined (__FLASHROM_LITTLE_ENDIAN__)
/* If architecture-specific approaches fail try generic variants. First: BSD (works about everywhere). */
#if !(defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__WINDOWS__))
#include <sys/param.h>
#if defined (__BYTE_ORDER)
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define __FLASHROM_LITTLE_ENDIAN__
#elif __BYTE_ORDER == __BIG_ENDIAN
#define __FLASHROM_BIG_ENDIAN__
#else
#error Unknown byte order!
#endif
#endif /* defined __BYTE_ORDER */
#endif /* !IS_WINDOWS */
#if !defined (__FLASHROM_BIG_ENDIAN__) && !defined (__FLASHROM_LITTLE_ENDIAN__)
/* Nonstandard libc-specific macros for determining endianness. */
/* musl provides an endian.h as well... but it can not be detected from within C. */
#if defined(__GLIBC__)
#include <endian.h>
#if BYTE_ORDER == LITTLE_ENDIAN
#define __FLASHROM_LITTLE_ENDIAN__ 1
#elif BYTE_ORDER == BIG_ENDIAN
#define __FLASHROM_BIG_ENDIAN__ 1
#endif
#endif
#endif
#endif
#if defined(__FLASHROM_LITTLE_ENDIAN__)
"little"
#else
"big"
#endif
