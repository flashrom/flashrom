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

// Likewise for target architectures
#if defined (__i386__) || defined (__x86_64__) || defined(__amd64__)
	#define IS_X86 1
#elif defined (__mips) || defined (__mips__) || defined (__MIPS__) || defined (mips)
	#define IS_MIPS 1
#elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || \
      defined(__ppc__) || defined(__ppc64__) || defined(_M_PPC) || defined(_ARCH_PPC) || \
      defined(_ARCH_PPC64) || defined(__ppc)
	#define IS_PPC 1
#elif defined(__arm__) || defined(__TARGET_ARCH_ARM) || defined(_ARM) || defined(_M_ARM) || defined(__arm) || \
      defined(__aarch64__)
	#define IS_ARM 1
#elif defined (__sparc__) || defined (__sparc)
	#define IS_SPARC 1
#elif defined (__alpha__)
	#define IS_ALPHA 1
#elif defined (__hppa__) || defined (__hppa)
	#define IS_HPPA 1
#elif defined (__m68k__)
	#define IS_M68K 1
#elif defined (__riscv)
	#define IS_RISCV 1
#elif defined (__sh__)
	#define IS_SH 1
#elif defined(__s390__) || defined(__s390x__) || defined(__zarch__)
	#define IS_S390 1
#elif defined(__arc__)
	#define IS_ARC 1
#endif

#if !(IS_X86 || IS_MIPS || IS_PPC || IS_ARM || IS_SPARC || IS_ALPHA || IS_HPPA || IS_M68K || IS_RISCV || IS_SH || IS_S390 || IS_ARC)
#error Unknown architecture
#endif

#endif /* !__PLATFORM_H__ */
