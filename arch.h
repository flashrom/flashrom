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
 * Header file for CPU architecture checking.
 */

#if defined (__i386__) || defined (__x86_64__)
#define __FLASHROM_ARCH__ "x86"
#elif defined (__mips) || defined (__mips__) || defined (_mips) || defined (mips)
#define __FLASHROM_ARCH__ "mips"
#elif defined(__powerpc__) || defined(__powerpc64__) || defined(__ppc__) || defined(__ppc64__)
#define __FLASHROM_ARCH__ "ppc"
#elif defined(__arm__)
#define __FLASHROM_ARCH__ "arm"
#endif
__FLASHROM_ARCH__
