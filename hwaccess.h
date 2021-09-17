/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Carl-Daniel Hailfinger
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
 * Header file for hardware access and OS abstraction. Included from flash.h.
 */

#ifndef __HWACCESS_H__
#define __HWACCESS_H__ 1

#include "platform.h"

#if NEED_PCI == 1
/*
 * libpci headers use the variable name "index" which triggers shadowing
 * warnings on systems which have the index() function in a default #include
 * or as builtin.
 */
#define index shadow_workaround_index

#if !defined (__NetBSD__)
#include <pci/pci.h>
#else
#include <pciutils/pci.h>
#endif

#undef index
#endif /* NEED_PCI == 1 */

#define ___constant_swab8(x) ((uint8_t) (				\
	(((uint8_t)(x) & (uint8_t)0xffU))))

#define ___constant_swab16(x) ((uint16_t) (				\
	(((uint16_t)(x) & (uint16_t)0x00ffU) << 8) |			\
	(((uint16_t)(x) & (uint16_t)0xff00U) >> 8)))

#define ___constant_swab32(x) ((uint32_t) (				\
	(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |		\
	(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |		\
	(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |		\
	(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24)))

#define ___constant_swab64(x) ((uint64_t) (				\
	(((uint64_t)(x) & (uint64_t)0x00000000000000ffULL) << 56) |	\
	(((uint64_t)(x) & (uint64_t)0x000000000000ff00ULL) << 40) |	\
	(((uint64_t)(x) & (uint64_t)0x0000000000ff0000ULL) << 24) |	\
	(((uint64_t)(x) & (uint64_t)0x00000000ff000000ULL) <<  8) |	\
	(((uint64_t)(x) & (uint64_t)0x000000ff00000000ULL) >>  8) |	\
	(((uint64_t)(x) & (uint64_t)0x0000ff0000000000ULL) >> 24) |	\
	(((uint64_t)(x) & (uint64_t)0x00ff000000000000ULL) >> 40) |	\
	(((uint64_t)(x) & (uint64_t)0xff00000000000000ULL) >> 56)))

#if defined (__FLASHROM_BIG_ENDIAN__)

#define cpu_to_le(bits)							\
static inline uint##bits##_t cpu_to_le##bits(uint##bits##_t val)	\
{									\
	return ___constant_swab##bits(val);				\
}

cpu_to_le(8)
cpu_to_le(16)
cpu_to_le(32)
cpu_to_le(64)

#define cpu_to_be8
#define cpu_to_be16
#define cpu_to_be32
#define cpu_to_be64

#elif defined (__FLASHROM_LITTLE_ENDIAN__)

#define cpu_to_be(bits)							\
static inline uint##bits##_t cpu_to_be##bits(uint##bits##_t val)	\
{									\
	return ___constant_swab##bits(val);				\
}

cpu_to_be(8)
cpu_to_be(16)
cpu_to_be(32)
cpu_to_be(64)

#define cpu_to_le8
#define cpu_to_le16
#define cpu_to_le32
#define cpu_to_le64

#endif /* __FLASHROM_BIG_ENDIAN__ / __FLASHROM_LITTLE_ENDIAN__ */

#define be_to_cpu8 cpu_to_be8
#define be_to_cpu16 cpu_to_be16
#define be_to_cpu32 cpu_to_be32
#define be_to_cpu64 cpu_to_be64
#define le_to_cpu8 cpu_to_le8
#define le_to_cpu16 cpu_to_le16
#define le_to_cpu32 cpu_to_le32
#define le_to_cpu64 cpu_to_le64

#if NEED_RAW_ACCESS == 1
#if IS_X86

#include "hwaccess_x86_io.h"

#if !(defined(__MACH__) && defined(__APPLE__)) && !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__) && !defined(__DragonFly__) && !defined(__LIBPAYLOAD__)
typedef struct { uint32_t hi, lo; } msr_t;
msr_t rdmsr(int addr);
int wrmsr(int addr, msr_t msr);
#endif
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
/* FreeBSD already has conflicting definitions for wrmsr/rdmsr. */
#undef rdmsr
#undef wrmsr
#define rdmsr freebsd_rdmsr
#define wrmsr freebsd_wrmsr
typedef struct { uint32_t hi, lo; } msr_t;
msr_t freebsd_rdmsr(int addr);
int freebsd_wrmsr(int addr, msr_t msr);
#endif
#if defined(__LIBPAYLOAD__)
#include <arch/io.h>
#include <arch/msr.h>
typedef struct { uint32_t hi, lo; } msr_t;
msr_t libpayload_rdmsr(int addr);
int libpayload_wrmsr(int addr, msr_t msr);
#undef rdmsr
#define rdmsr libpayload_rdmsr
#define wrmsr libpayload_wrmsr
#endif

#elif IS_PPC

/* PCI port I/O is not yet implemented on PowerPC. */

#elif IS_MIPS

/* PCI port I/O is not yet implemented on MIPS. */

#elif IS_SPARC

/* PCI port I/O is not yet implemented on SPARC. */

#elif IS_ARM

/* Non memory mapped I/O is not supported on ARM. */

#elif IS_ARC

/* Non memory mapped I/O is not supported on ARC. */

#else

#error Unknown architecture, please check if it supports PCI port IO.

#endif /* IS_* */
#endif /* NEED_RAW_ACCESS == 1 */

#endif /* !__HWACCESS_H__ */
