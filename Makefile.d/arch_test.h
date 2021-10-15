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
 * This file determinate the target architecture. It should only be used
 * by the Makefile
 */

#if defined (__i386__) || defined (__x86_64__) || defined(__amd64__)
	#define __FLASHROM_ARCH__ "x86"
#elif defined (__mips) || defined (__mips__) || defined (__MIPS__) || defined (mips)
	#define __FLASHROM_ARCH__ "mips"
#elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || \
      defined(__ppc__) || defined(__ppc64__) || defined(_M_PPC) || defined(_ARCH_PPC) || \
      defined(_ARCH_PPC64) || defined(__ppc)
	#define __FLASHROM_ARCH__ "ppc"
#elif defined(__arm__) || defined(__TARGET_ARCH_ARM) || defined(_ARM) || defined(_M_ARM) || defined(__arm) || \
      defined(__aarch64__)
	#define __FLASHROM_ARCH__ "arm"
#elif defined (__sparc__) || defined (__sparc)
	#define __FLASHROM_ARCH__ "sparc"
#elif defined (__alpha__)
	#define __FLASHROM_ARCH__ "alpha"
#elif defined (__hppa__) || defined (__hppa)
	#define __FLASHROM_ARCH__ "hppa"
#elif defined (__m68k__)
	#define __FLASHROM_ARCH__ "m68k"
#elif defined (__riscv)
	#define __FLASHROM_ARCH__ "riscv"
#elif defined (__sh__)
	#define __FLASHROM_ARCH__ "sh"
#elif defined(__s390__) || defined(__s390x__) || defined(__zarch__)
	#define __FLASHROM_ARCH__ "s390"
#elif defined(__arc__)
	#define __FLASHROM_ARCH__ "arc"
#elif defined(__ARC64__)
	#define __FLASHROM_ARCH__ "arc64"
#elif defined(__e2k__)
	#define __FLASHROM_ARCH__ "e2k"
#else
	#define __FLASHROM_ARCH__ "unknown"
#endif
__FLASHROM_ARCH__
