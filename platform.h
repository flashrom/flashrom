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
 * Header file to determine target OS and CPU architecture.
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__ 1

// Helper defines for operating systems
#if defined(__gnu_linux__) || defined(__linux__)
#define IS_LINUX 1
#else
#define IS_LINUX 0
#endif
#if defined(__APPLE__) && defined(__MACH__) /* yes, both. */
#define IS_MACOSX 1
#else
#define IS_MACOSX 0
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__WINDOWS__)
#define IS_WINDOWS 1
#else
#define IS_WINDOWS 0
#endif

// Likewise for target architectures
#if defined (__i386__) || defined (__x86_64__) || defined(__amd64__)
	#define __FLASHROM_ARCH__ "x86"
	#define IS_X86 1
#elif defined (__mips) || defined (__mips__) || defined (__MIPS__) || defined (mips)
	#define __FLASHROM_ARCH__ "mips"
	#define IS_MIPS 1
#elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || \
      defined(__ppc__) || defined(__ppc64__) || defined(_M_PPC) || defined(_ARCH_PPC) || \
      defined(_ARCH_PPC64) || defined(__ppc)
	#define __FLASHROM_ARCH__ "ppc"
	#define IS_PPC 1
#elif defined(__arm__) || defined(__TARGET_ARCH_ARM) || defined(_ARM) || defined(_M_ARM) || defined(__arm) || \
      defined(__aarch64__)
	#define __FLASHROM_ARCH__ "arm"
	#define IS_ARM 1
#elif defined (__sparc__) || defined (__sparc)
	#define __FLASHROM_ARCH__ "sparc"
	#define IS_SPARC 1
#elif defined (__alpha__)
	#define __FLASHROM_ARCH__ "alpha"
	#define IS_ALPHA 1
#elif defined (__hppa__) || defined (__hppa)
	#define __FLASHROM_ARCH__ "hppa"
	#define IS_HPPA 1
#elif defined (__m68k__)
	#define __FLASHROM_ARCH__ "m68k"
	#define IS_M68K 1
#elif defined (__riscv)
	#define __FLASHROM_ARCH__ "riscv"
	#define IS_RISCV 1
#elif defined (__sh__)
	#define __FLASHROM_ARCH__ "sh"
	#define IS_SH 1
#elif defined(__s390__) || defined(__s390x__) || defined(__zarch__)
	#define __FLASHROM_ARCH__ "s390"
	#define IS_S390 1
#endif

#if !(IS_X86 || IS_MIPS || IS_PPC || IS_ARM || IS_SPARC || IS_ALPHA || IS_HPPA || IS_M68K || IS_RISCV || IS_SH || IS_S390)
#error Unknown architecture
#endif

/* The next big hunk tries to guess endianness from various preprocessor macros */
/* First some error checking in case some weird header has defined both.
 * NB: OpenBSD always defines _BIG_ENDIAN and _LITTLE_ENDIAN. */
#if defined (__LITTLE_ENDIAN__) && defined (__BIG_ENDIAN__)
#error Conflicting endianness #define
#endif

#if IS_X86

/* All x86 is little-endian. */
#define __FLASHROM_LITTLE_ENDIAN__ 1

#elif IS_MIPS

/* MIPS can be either endian. */
#if defined (__MIPSEL) || defined (__MIPSEL__) || defined (_MIPSEL) || defined (MIPSEL)
#define __FLASHROM_LITTLE_ENDIAN__ 1
#elif defined (__MIPSEB) || defined (__MIPSEB__) || defined (_MIPSEB) || defined (MIPSEB)
#define __FLASHROM_BIG_ENDIAN__ 1
#endif

#elif IS_PPC

/* PowerPC can be either endian. */
#if defined (_BIG_ENDIAN) || defined (__BIG_ENDIAN__)
#define __FLASHROM_BIG_ENDIAN__ 1
#elif defined (_LITTLE_ENDIAN) || defined (__LITTLE_ENDIAN__)
#define __FLASHROM_LITTLE_ENDIAN__ 1
#endif

#elif IS_ARM

/* ARM can be either endian. */
#if defined (__ARMEB__)
#define __FLASHROM_BIG_ENDIAN__ 1
#elif defined (__ARMEL__)
#define __FLASHROM_LITTLE_ENDIAN__ 1
#endif

#elif IS_SPARC
/* SPARC is big endian in general (but allows to access data in little endian too). */
#define __FLASHROM_BIG_ENDIAN__ 1

#endif /* IS_? */

#if !defined (__FLASHROM_BIG_ENDIAN__) && !defined (__FLASHROM_LITTLE_ENDIAN__)

/* If architecture-specific approaches fail try generic variants. First: BSD (works about everywhere). */
#if !IS_WINDOWS
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

#if !defined (__FLASHROM_BIG_ENDIAN__) && !defined (__FLASHROM_LITTLE_ENDIAN__)
#error Unable to determine endianness.
#endif

#endif /* !__PLATFORM_H__ */
