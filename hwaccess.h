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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/*
 * Header file for hardware access and OS abstraction. Included from flash.h.
 */

#ifndef __HWACCESS_H__
#define __HWACCESS_H__ 1

#include "platform.h"

#if IS_X86
#if defined(__GLIBC__)
#include <sys/io.h>
#endif
#endif

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


/* The next big hunk tries to guess endianess from various preprocessor macros */
/* First some error checking in case some weird header has defines both.
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

#if NEED_PCI == 1
#if IS_X86

#define __FLASHROM_HAVE_OUTB__ 1

/* for iopl and outb under Solaris */
#if defined (__sun)
#include <sys/sysi86.h>
#include <sys/psw.h>
#include <asm/sunddi.h>
#endif

/* Clarification about OUTB/OUTW/OUTL argument order:
 * OUT[BWL](val, port)
 */

#if defined(__FreeBSD__) || defined(__DragonFly__)
  /* Note that Debian/kFreeBSD (FreeBSD kernel with glibc) has conflicting
   * out[bwl] definitions in machine/cpufunc.h and sys/io.h at least in some
   * versions. Use machine/cpufunc.h only for plain FreeBSD/DragonFlyBSD.
   */
  #include <machine/cpufunc.h>
  #define OUTB(x, y) do { u_int outb_tmp = (y); outb(outb_tmp, (x)); } while (0)
  #define OUTW(x, y) do { u_int outw_tmp = (y); outw(outw_tmp, (x)); } while (0)
  #define OUTL(x, y) do { u_int outl_tmp = (y); outl(outl_tmp, (x)); } while (0)
  #define INB(x) __extension__ ({ u_int inb_tmp = (x); inb(inb_tmp); })
  #define INW(x) __extension__ ({ u_int inw_tmp = (x); inw(inw_tmp); })
  #define INL(x) __extension__ ({ u_int inl_tmp = (x); inl(inl_tmp); })
#else

#if defined (__sun)
  /* Note different order for outb */
  #define OUTB(x,y) outb(y, x)
  #define OUTW(x,y) outw(y, x)
  #define OUTL(x,y) outl(y, x)
  #define INB  inb
  #define INW  inw
  #define INL  inl
#else

#ifdef __DJGPP__

#include <pc.h>

  #define OUTB(x,y) outportb(y, x)
  #define OUTW(x,y) outportw(y, x)
  #define OUTL(x,y) outportl(y, x)

  #define INB  inportb
  #define INW  inportw
  #define INL  inportl

#else

#if defined(__MACH__) && defined(__APPLE__)
    /* Header is part of the DirectHW library. */
    #include <DirectHW/DirectHW.h>
#endif

  /* This is the usual glibc interface. */
  #define OUTB outb
  #define OUTW outw
  #define OUTL outl
  #define INB  inb
  #define INW  inw
  #define INL  inl
#endif
#endif
#endif

#if defined(__NetBSD__) || defined (__OpenBSD__)
  #if defined(__i386__) || defined(__x86_64__)
    #include <sys/types.h>
    #include <machine/sysarch.h>
    #if defined(__NetBSD__)
      #if defined(__i386__)
        #define iopl i386_iopl
      #elif defined(__x86_64__)
        #define iopl x86_64_iopl
      #endif
    #elif defined (__OpenBSD__)
      #if defined(__i386__)
        #define iopl i386_iopl
      #elif defined(__amd64__)
        #define iopl amd64_iopl
      #endif
    #endif

static inline void outb(uint8_t value, uint16_t port)
{
	__asm__ volatile ("outb %b0,%w1": :"a" (value), "Nd" (port));
}

static inline uint8_t inb(uint16_t port)
{
	uint8_t value;
	__asm__ volatile ("inb %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

static inline void outw(uint16_t value, uint16_t port)
{
	__asm__ volatile ("outw %w0,%w1": :"a" (value), "Nd" (port));
}

static inline uint16_t inw(uint16_t port)
{
	uint16_t value;
	__asm__ volatile ("inw %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

static inline void outl(uint32_t value, uint16_t port)
{
	__asm__ volatile ("outl %0,%w1": :"a" (value), "Nd" (port));
}

static inline uint32_t inl(uint16_t port)
{
	uint32_t value;
	__asm__ volatile ("inl %1,%0":"=a" (value):"Nd" (port));
	return value;
}
  #endif
#endif

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

#else

#error Unknown architecture, please check if it supports PCI port IO.

#endif /* IS_* */
#endif /* NEED_PCI == 1 */

#endif /* !__HWACCESS_H__ */
