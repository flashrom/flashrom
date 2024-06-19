/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009,2010 Carl-Daniel Hailfinger
 * Copyright (C) 2022 secunet Security Networks AG
 * (Written by Thomas Heijligen <thomas.heijligen@secunet.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * This file implements x86 I/O port permission and access handling.
 *
 * The first part of the file defines the platform dependend implementations to
 * use on each platform. For getting I/O permissions set IO_PORT_PERMISSION to
 * one of:
 *   - USE_DUMMY
 *   - USE_SUN
 *   - USE_DEVICE
 *   - USE_IOPERM
 *   - USE_IOPL
 * For the IN[B/W/L] and OUT[B/W/L] functions set IO_PORT_FUNCTION to one of:
 *   - USE_LIBC_TARGET_LAST
 *   - USE_LIBC_TARGET_FIRST
 *   - USE_DOS
 *   - USE_ASM
 *
 * The platform specific code for getting I/O permissions consists of two
 * functions. `platform_get_io_perms()` is called to get
 * permissions and `platform_release_io_perms()` is called for releasing those.
 */

#include <errno.h>
#include <string.h>

#include "flash.h"
#include "hwaccess_x86_io.h"

/* IO_PORT_FUNCTION */
#define USE_LIBC_TARGET_LAST	1
#define USE_LIBC_TARGET_FIRST	2
#define USE_ASM			3
#define USE_DOS			4

/* IO_PORT_PERMISSION */
#define USE_IOPL		5
#define USE_DEVICE		6
#define USE_DUMMY		7
#define USE_SUN			8
#define USE_IOPERM		9

#if defined(__ANDROID__)
#include <sys/io.h>
#include <unistd.h>

#define IO_PORT_PERMISSION USE_IOPERM
#define IO_PORT_FUNCTION USE_ASM
#endif

#if defined(__linux__) && !defined(__ANDROID__)
#include <sys/io.h>
#include <unistd.h>

#define IO_PORT_PERMISSION USE_IOPL
#define IO_PORT_FUNCTION USE_LIBC_TARGET_LAST
#endif

#if defined(__FreeBSD_kernel) && defined(__GLIBC__)
#include <sys/io.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define IO_PORT_PERMISSION USE_DEVICE
#define IO_PORT_FUNCTION USE_LIBC_TARGET_LAST
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/types.h>
#include <machine/cpufunc.h>
#include <fcntl.h>
#include <unistd.h>

#define IO_PORT_PERMISSION USE_DEVICE
#define IO_PORT_FUNCTION USE_LIBC_TARGET_FIRST
#endif

#if defined(__NetBSD__)
#include <sys/types.h>
#include <machine/sysarch.h>

#if defined(__i386__)
#define iopl i386_iopl
#elif defined(__x86_64__)
#define iopl x86_64_iopl
#endif

#define IO_PORT_PERMISSION USE_IOPL
#define IO_PORT_FUNCTION USE_ASM
#endif

#if defined(__OpenBSD__)
#include <sys/types.h>
#include <machine/sysarch.h>

#if defined(__i386__)
#define iopl i386_iopl
#elif defined(__amd64__)
#define iopl amd64_iopl
#endif

#define IO_PORT_PERMISSION USE_IOPL
#define IO_PORT_FUNCTION USE_ASM
#endif

#if defined(__MACH__) && defined(__APPLE__)
#include <DirectHW/DirectHW.h>

#define IO_PORT_PERMISSION USE_IOPL
#define IO_PORT_FUNCTION USE_LIBC_TARGET_LAST
#endif

#if defined(__DJGPP__)
#include <pc.h>

#define IO_PORT_PERMISSION USE_DUMMY
#define IO_PORT_FUNCTION USE_DOS
#endif

#if defined(__LIBPAYLOAD__)
#include <arch/io.h>

#define IO_PORT_PERMISSION USE_DUMMY
#define IO_PORT_FUNCTION USE_LIBC_TARGET_LAST
#endif

#if defined(__sun)
#include <sys/sysi86.h>
#include <sys/psw.h>
#include <asm/sunddi.h>

#define IO_PORT_PERMISSION USE_SUN
#define IO_PORT_FUNCTION USE_LIBC_TARGET_FIRST
#endif

#if defined(__gnu_hurd__)
#include <sys/io.h>

#define IO_PORT_PERMISSION USE_IOPERM
#define IO_PORT_FUNCTION USE_LIBC_TARGET_LAST
#endif

/* Make sure IO_PORT_PERMISSION and IO_PORT_FUNCTION are set */
#if IO_PORT_PERMISSION == 0 || IO_PORT_FUNCTION == 0
#error Unsupported or misconfigured platform.
#endif

/*
 * USE_DUMMY
 * This is for platforms which have no privilege levels.
 */
#if IO_PORT_PERMISSION == USE_DUMMY
static int platform_get_io_perms(void)
{
	return 0;
}

static int platform_release_io_perms(void *p)
{
	return 0;
}
#endif

/*
 * USE_SUN
 * This implementation uses SunOS system call sysi86 to handle I/O port permissions.
 */
#if IO_PORT_PERMISSION == USE_SUN
static int platform_get_io_perms(void)
{
	return sysi86(SI86V86, V86SC_IOPL, PS_IOPL);
}

static int platform_release_io_perms(void *p)
{
	sysi86(SI86V86, V86SC_IOPL, 0);
	return 0;
}
#endif

/*
 * USE_DEVICE
 * This implementation uses the virtual /dev/io file to handle I/O Port permissions.
 */
#if IO_PORT_PERMISSION == USE_DEVICE
int io_fd;

static int platform_get_io_perms(void)
{
	io_fd = open("/dev/io", O_RDWR);
	return (io_fd >= 0 ? 0 : -1);
}

static int platform_release_io_perms(void *p)
{
	close(io_fd);
	return 0;
}
#endif

/*
 * USE_IOPERM
 * This implementation uses the ioperm system call to handle I/O port permissions.
 */
#if IO_PORT_PERMISSION == USE_IOPERM
static int platform_get_io_perms(void)
{
	return ioperm(0, 65536, 1);
}

static int platform_release_io_perms(void *p)
{
	ioperm(0, 65536, 0);
	return 0;
}
#endif

/*
 * USE_IOPL
 * This implementation uses the iopl system call to handle I/O port permissions.
 */
#if IO_PORT_PERMISSION == USE_IOPL
static int platform_get_io_perms(void)
{
	return iopl(3);
}

static int platform_release_io_perms(void *p)
{
	iopl(0);
	return 0;
}
#endif

/**
 * @brief Get access to the x86 I/O ports
 *
 * This function abstracts the platform dependent function to get access to the
 * x86 I/O ports and musst be called before using IN[B/W/L] or OUT[B/W/L].
 * It registers a shutdown function to release those privileges.
 *
 * @return 0 on success, platform specific number on failure
 */
int rget_io_perms(void)
{
	if (platform_get_io_perms() == 0) {
		register_shutdown(platform_release_io_perms, NULL);
		return 0;
	}

	msg_perr("ERROR: Could not get I/O privileges (%s).\n", strerror(errno));
#if defined(__linux__) && !defined(__ANDROID__)
	if (getuid() != 0) {
		msg_perr("Make sure you are running flashrom with root privileges.\n");
	} else {
		msg_perr("Your kernel may prevent access based on security policies.\n"
		 "Issue a 'dmesg | grep flashrom' for further information\n");
	}
#elif defined(__OpenBSD__)
	msg_perr("On OpenBSD set securelevel=-1 in /etc/rc.securelevel and\n"
		 "reboot, or reboot into single user mode.\n");
#elif defined(__NetBSD__)
	msg_perr("On NetBSD reboot into single user mode or make sure\n"
		 "that your kernel configuration has the option INSECURE enabled.\n");
#else
	msg_perr("Make sure you are running flashrom with root privileges.\n");
#endif
	return 1;
}

/*
 * USE_LIBC_LAST
 * This implementation wraps the glibc functions with the port as 2nd argument.
 */
#if IO_PORT_FUNCTION == USE_LIBC_TARGET_LAST
void OUTB(uint8_t value, uint16_t port)
{
	outb(value, port);
}

void OUTW(uint16_t value, uint16_t port)
{
	outw(value, port);
}

void OUTL(uint32_t value, uint16_t port)
{
	outl(value, port);
}
#endif

/*
 * USE_LIBC_FIRST
 * This implementation uses libc based functions with the port as first argument
 * like on BSDs.
 */
#if IO_PORT_FUNCTION == USE_LIBC_TARGET_FIRST
void OUTB(uint8_t value, uint16_t port)
{
	outb(port, value);
}

void OUTW(uint16_t value, uint16_t port)
{
	outw(port, value);
}

void OUTL(uint32_t value, uint16_t port)
{
	outl(port, value);
}
#endif

/*
 * LIBC_xxx
 * INx functions can be shared between _LAST and _FIRST
 */
#if IO_PORT_FUNCTION == USE_LIBC_TARGET_LAST || IO_PORT_FUNCTION == USE_LIBC_TARGET_FIRST
uint8_t INB(uint16_t port)
{
	return inb(port);
}

uint16_t INW(uint16_t port)
{
	return inw(port);
}

uint32_t INL(uint16_t port)
{
	return inl(port);
}
#endif

/*
 * USE_DOS
 * DOS provides the functions under a differnt name.
 */
#if IO_PORT_FUNCTION == USE_DOS
void OUTB(uint8_t value, uint16_t port)
{
	outportb(port, value);
}

void OUTW(uint16_t value, uint16_t port)
{
	outportw(port, value);
}

void OUTL(uint32_t value, uint16_t port)
{
	outportl(port, value);
}

uint8_t INB(uint16_t port)
{
	return inportb(port);
}

uint16_t INW(uint16_t port)
{
	return inportw(port);
}

uint32_t INL(uint16_t port)
{
	return inportl(port);
}
#endif

/*
 * USE_ASM
 * Pure assembly implementation.
 */
#if IO_PORT_FUNCTION == USE_ASM
void OUTB(uint8_t value, uint16_t port)
{
	__asm__ volatile ("outb %b0,%w1": :"a" (value), "Nd" (port));
}

void OUTW(uint16_t value, uint16_t port)
{
	__asm__ volatile ("outw %w0,%w1": :"a" (value), "Nd" (port));
}

void OUTL(uint32_t value, uint16_t port)
{
	__asm__ volatile ("outl %0,%w1": :"a" (value), "Nd" (port));
}

uint8_t INB(uint16_t port)
{
	uint8_t value;
	__asm__ volatile ("inb %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

uint16_t INW(uint16_t port)
{
	uint16_t value;
	__asm__ volatile ("inw %w1,%0":"=a" (value):"Nd" (port));
	return value;
}

uint32_t INL(uint16_t port)
{
	uint32_t value;
	__asm__ volatile ("inl %1,%0":"=a" (value):"Nd" (port));
	return value;
}
#endif
