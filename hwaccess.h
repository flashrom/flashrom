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
 *
 *
 * Header file for hardware access and OS abstraction. Included from flash.h.
 */

#ifndef __HWACCESS_H__
#define __HWACCESS_H__ 1

#if defined(__GLIBC__)
#include <sys/io.h>
#endif
#if NEED_PCI == 1
#include <pci/pci.h>
#endif

/* for iopl and outb under Solaris */
#if defined (__sun) && (defined(__i386) || defined(__amd64))
#include <strings.h>
#include <sys/sysi86.h>
#include <sys/psw.h>
#include <asm/sunddi.h>
#endif

#if (defined(__MACH__) && defined(__APPLE__))
#define __DARWIN__
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
  #include <machine/cpufunc.h>
  #define off64_t off_t
  #define lseek64 lseek
  #define OUTB(x, y) do { u_int outb_tmp = (y); outb(outb_tmp, (x)); } while (0)
  #define OUTW(x, y) do { u_int outw_tmp = (y); outw(outw_tmp, (x)); } while (0)
  #define OUTL(x, y) do { u_int outl_tmp = (y); outl(outl_tmp, (x)); } while (0)
  #define INB(x) __extension__ ({ u_int inb_tmp = (x); inb(inb_tmp); })
  #define INW(x) __extension__ ({ u_int inw_tmp = (x); inw(inw_tmp); })
  #define INL(x) __extension__ ({ u_int inl_tmp = (x); inl(inl_tmp); })
#else
#if defined(__DARWIN__)
    #include <DirectIO/darwinio.h>
    #define off64_t off_t
    #define lseek64 lseek
#endif
#if defined (__sun) && (defined(__i386) || defined(__amd64))
  /* Note different order for outb */
  #define OUTB(x,y) outb(y, x)
  #define OUTW(x,y) outw(y, x)
  #define OUTL(x,y) outl(y, x)
  #define INB  inb
  #define INW  inw
  #define INL  inl
#else
  #define OUTB outb
  #define OUTW outw
  #define OUTL outl
  #define INB  inb
  #define INW  inw
  #define INL  inl
#endif
#endif

#if defined(__NetBSD__)
  #define off64_t off_t
  #define lseek64 lseek
  #if defined(__i386__) || defined(__x86_64__)
    #include <sys/types.h>
    #include <machine/sysarch.h>
    #if defined(__i386__)
      #define iopl i386_iopl
    #elif defined(__x86_64__)
      #define iopl x86_64_iopl
    #endif
  #include <stdint.h>

static inline void
outb(uint8_t value, uint16_t port)
{
	asm volatile ("outb %b0,%w1": :"a" (value), "Nd" (port));
}

static inline uint8_t
inb(uint16_t port)
{
	uint8_t value;
	asm volatile ("inb %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

static inline void
outw(uint16_t value, uint16_t port)
{
	asm volatile ("outw %w0,%w1": :"a" (value), "Nd" (port));
}

static inline uint16_t
inw(uint16_t port)
{
	uint16_t value;
	asm volatile ("inw %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

static inline void
outl(uint32_t value, uint16_t port)
{
	asm volatile ("outl %0,%w1": :"a" (value), "Nd" (port));
}

static inline uint32_t
inl(uint16_t port)
{
	uint32_t value;
	asm volatile ("inl %1,%0":"=a" (value):"Nd" (port));
	return value;
}
  #endif
#endif

#if !defined(__DARWIN__) && !defined(__FreeBSD__) && !defined(__DragonFly__)
typedef struct { uint32_t hi, lo; } msr_t;
msr_t rdmsr(int addr);
int wrmsr(int addr, msr_t msr);
#endif
#if defined(__FreeBSD__) || defined(__DragonFly__)
/* FreeBSD already has conflicting definitions for wrmsr/rdmsr. */
#undef rdmsr
#undef wrmsr
#define rdmsr freebsd_rdmsr
#define wrmsr freebsd_wrmsr
typedef struct { uint32_t hi, lo; } msr_t;
msr_t freebsd_rdmsr(int addr);
int freebsd_wrmsr(int addr, msr_t msr);
#endif

#endif /* !__HWACCESS_H__ */
