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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/*
 * Header file to determine target OS and CPU architecture.
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__ 1

// Helper defines for operating systems
#define IS_LINUX	(defined(__gnu_linux__) || defined(__linux__))
#define IS_MACOSX	(defined(__APPLE__) && defined(__MACH__)) /* yes, both. */
#define IS_WINDOWS	(defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__WINDOWS__))

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
#endif

#if !(IS_X86 || IS_MIPS || IS_PPC || IS_ARM || IS_SPARC)
#error Unknown architecture
#endif

#endif /* !__PLATFORM_H__ */
