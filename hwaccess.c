/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009,2010 Carl-Daniel Hailfinger
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#if !defined (__DJGPP__)
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif
#include "flash.h"

#if defined(__i386__) || defined(__x86_64__)

/* sync primitive is not needed because x86 uses uncached accesses
 * which have a strongly ordered memory model.
 */
static inline void sync_primitive(void)
{
}

#if defined(__FreeBSD__) || defined(__DragonFly__)
int io_fd;
#endif

void get_io_perms(void)
{
#if defined(__DJGPP__)
	/* We have full permissions by default. */
	return;
#else
#if defined (__sun) && (defined(__i386) || defined(__amd64))
	if (sysi86(SI86V86, V86SC_IOPL, PS_IOPL) != 0) {
#elif defined(__FreeBSD__) || defined (__DragonFly__)
	if ((io_fd = open("/dev/io", O_RDWR)) < 0) {
#else 
	if (iopl(3) != 0) {
#endif
		msg_perr("ERROR: Could not get I/O privileges (%s).\n"
			"You need to be root.\n", strerror(errno));
		exit(1);
	}
#endif
}

void release_io_perms(void)
{
#if defined(__FreeBSD__) || defined(__DragonFly__)
	close(io_fd);
#endif
}

#elif defined(__powerpc__) || defined(__powerpc64__) || defined(__ppc__) || defined(__ppc64__)

static inline void sync_primitive(void)
{
	/* Prevent reordering and/or merging of reads/writes to hardware.
	 * Such reordering and/or merging would break device accesses which
	 * depend on the exact access order.
	 */
	asm("eieio" : : : "memory");
}

/* PCI port I/O is not yet implemented on PowerPC. */
void get_io_perms(void)
{
}

/* PCI port I/O is not yet implemented on PowerPC. */
void release_io_perms(void)
{
}

#elif defined (__mips) || defined (__mips__) || defined (_mips) || defined (mips)

/* sync primitive is not needed because /dev/mem on MIPS uses uncached accesses
 * in mode 2 which has a strongly ordered memory model.
 */
static inline void sync_primitive(void)
{
}

/* PCI port I/O is not yet implemented on MIPS. */
void get_io_perms(void)
{
}

/* PCI port I/O is not yet implemented on MIPS. */
void release_io_perms(void)
{
}

#else

#error Unknown architecture

#endif

void mmio_writeb(uint8_t val, void *addr)
{
	*(volatile uint8_t *) addr = val;
	sync_primitive();
}

void mmio_writew(uint16_t val, void *addr)
{
	*(volatile uint16_t *) addr = val;
	sync_primitive();
}

void mmio_writel(uint32_t val, void *addr)
{
	*(volatile uint32_t *) addr = val;
	sync_primitive();
}

uint8_t mmio_readb(void *addr)
{
	return *(volatile uint8_t *) addr;
}

uint16_t mmio_readw(void *addr)
{
	return *(volatile uint16_t *) addr;
}

uint32_t mmio_readl(void *addr)
{
	return *(volatile uint32_t *) addr;
}

void mmio_le_writeb(uint8_t val, void *addr)
{
	mmio_writeb(cpu_to_le8(val), addr);
}

void mmio_le_writew(uint16_t val, void *addr)
{
	mmio_writew(cpu_to_le16(val), addr);
}

void mmio_le_writel(uint32_t val, void *addr)
{
	mmio_writel(cpu_to_le32(val), addr);
}

uint8_t mmio_le_readb(void *addr)
{
	return le_to_cpu8(mmio_readb(addr));
}

uint16_t mmio_le_readw(void *addr)
{
	return le_to_cpu16(mmio_readw(addr));
}

uint32_t mmio_le_readl(void *addr)
{
	return le_to_cpu32(mmio_readl(addr));
}
