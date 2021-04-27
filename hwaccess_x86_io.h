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

#ifndef __HWACCESS_X86_IO_H__
#define __HWACCESS_X86_IO_H__ 1

/* sys/io.h provides iopl(2) and x86 I/O port access functions (inb, outb etc).
 * It is included in glibc (thus available also on debian/kFreeBSD) but also in other libcs that mimic glibc,
 * e.g. musl and uclibc. Because we cannot detect the libc or existence of the header or of the instructions
 * themselves safely in here we use some heuristic below:
 * On Android we don't have the header file and no way for I/O port access at all. However, sys/glibc-syscalls.h
 * refers to an iopl implementation and we therefore include at least that one for now. On non-Android we assume
 * that a Linux system's libc has a suitable sys/io.h or (on non-Linux) we depend on glibc to offer it. */
#if defined(__ANDROID__)
#include <sys/glibc-syscalls.h>
#elif defined(__linux__) || defined(__GLIBC__)
#include <sys/io.h>
#endif

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
  #include <sys/types.h>
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

#endif /* __HWACCESS_X86_IO_H__ */
