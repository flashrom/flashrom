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

#include "platform.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#if !defined (__DJGPP__) && !defined(__LIBPAYLOAD__)
/* No file access needed/possible to get hardware access permissions. */
#include <unistd.h>
#include <fcntl.h>
#endif
#include "flash.h"
#include "hwaccess.h"

#if !(IS_LINUX || IS_MACOSX || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__) || defined(__DJGPP__) || defined(__LIBPAYLOAD__) || defined(__sun))
#error "Unknown operating system"
#endif

#define USE_IOPL	(IS_LINUX || IS_MACOSX || defined(__NetBSD__) || defined(__OpenBSD__))
#define USE_DEV_IO	(defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__))

#if IS_X86 && USE_DEV_IO
int io_fd;
#endif

/* Prevent reordering and/or merging of reads/writes to hardware.
 * Such reordering and/or merging would break device accesses which depend on the exact access order.
 */
static inline void sync_primitive(void)
{
/* This is not needed for...
 * - x86: uses uncached accesses which have a strongly ordered memory model.
 * - MIPS: uses uncached accesses in mode 2 on /dev/mem which has also a strongly ordered memory model.
 * - ARM: uses a strongly ordered memory model for device memories.
 *
 * See also https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/Documentation/memory-barriers.txt
 */
#if IS_PPC // cf. http://lxr.free-electrons.com/source/arch/powerpc/include/asm/barrier.h
	asm("eieio" : : : "memory");
#elif IS_SPARC
#if defined(__sparc_v9__) || defined(__sparcv9)
	/* Sparc V9 CPUs support three different memory orderings that range from x86-like TSO to PowerPC-like
	 * RMO. The modes can be switched at runtime thus to make sure we maintain the right order of access we
	 * use the strongest hardware memory barriers that exist on Sparc V9. */
	asm volatile ("membar #Sync" ::: "memory");
#elif defined(__sparc_v8__) || defined(__sparcv8)
	/* On SPARC V8 there is no RMO just PSO and that does not apply to I/O accesses... but if V8 code is run
	 * on V9 CPUs it might apply... or not... we issue a write barrier anyway. That's the most suitable
	 * operation in the V8 instruction set anyway. If you know better then please tell us. */
	asm volatile ("stbar");
#else
	#error Unknown and/or unsupported SPARC instruction set version detected.
#endif
#endif
}

#if IS_X86 && !(defined(__DJGPP__) || defined(__LIBPAYLOAD__))
static int release_io_perms(void *p)
{
#if defined (__sun)
	sysi86(SI86V86, V86SC_IOPL, 0);
#elif USE_DEV_IO
	close(io_fd);
#elif USE_IOPL
	iopl(0);
#endif
	return 0;
}
#endif

/* Get I/O permissions with automatic permission release on shutdown. */
int rget_io_perms(void)
{
#if IS_X86 && !(defined(__DJGPP__) || defined(__LIBPAYLOAD__))
#if defined (__sun)
	if (sysi86(SI86V86, V86SC_IOPL, PS_IOPL) != 0) {
#elif USE_DEV_IO
	if ((io_fd = open("/dev/io", O_RDWR)) < 0) {
#elif USE_IOPL
	if (iopl(3) != 0) {
#endif
		msg_perr("ERROR: Could not get I/O privileges (%s).\n", strerror(errno));
		msg_perr("You need to be root.\n");
#if defined (__OpenBSD__)
		msg_perr("If you are root already please set securelevel=-1 in /etc/rc.securelevel and\n"
			 "reboot, or reboot into single user mode.\n");
#elif defined(__NetBSD__)
		msg_perr("If you are root already please reboot into single user mode or make sure\n"
			 "that your kernel configuration has the option INSECURE enabled.\n");
#endif
		return 1;
	} else {
		register_shutdown(release_io_perms, NULL);
	}
#else
	/* DJGPP and libpayload environments have full PCI port I/O permissions by default. */
	/* PCI port I/O support is unimplemented on PPC/MIPS and unavailable on ARM. */
#endif
	return 0;
}

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

void mmio_readn(void *addr, uint8_t *buf, size_t len)
{
	memcpy(buf, addr, len);
	return;
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

enum mmio_write_type {
	mmio_write_type_b,
	mmio_write_type_w,
	mmio_write_type_l,
};

struct undo_mmio_write_data {
	void *addr;
	int reg;
	enum mmio_write_type type;
	union {
		uint8_t bdata;
		uint16_t wdata;
		uint32_t ldata;
	};
};

int undo_mmio_write(void *p)
{
	struct undo_mmio_write_data *data = p;
	msg_pdbg("Restoring MMIO space at %p\n", data->addr);
	switch (data->type) {
	case mmio_write_type_b:
		mmio_writeb(data->bdata, data->addr);
		break;
	case mmio_write_type_w:
		mmio_writew(data->wdata, data->addr);
		break;
	case mmio_write_type_l:
		mmio_writel(data->ldata, data->addr);
		break;
	}
	/* p was allocated in register_undo_mmio_write. */
	free(p);
	return 0;
}

#define register_undo_mmio_write(a, c)					\
{									\
	struct undo_mmio_write_data *undo_mmio_write_data;		\
	undo_mmio_write_data = malloc(sizeof(struct undo_mmio_write_data)); \
	if (!undo_mmio_write_data) {					\
		msg_gerr("Out of memory!\n");				\
		exit(1);						\
	}								\
	undo_mmio_write_data->addr = a;					\
	undo_mmio_write_data->type = mmio_write_type_##c;		\
	undo_mmio_write_data->c##data = mmio_read##c(a);		\
	register_shutdown(undo_mmio_write, undo_mmio_write_data);	\
}

#define register_undo_mmio_writeb(a) register_undo_mmio_write(a, b)
#define register_undo_mmio_writew(a) register_undo_mmio_write(a, w)
#define register_undo_mmio_writel(a) register_undo_mmio_write(a, l)

void rmmio_writeb(uint8_t val, void *addr)
{
	register_undo_mmio_writeb(addr);
	mmio_writeb(val, addr);
}

void rmmio_writew(uint16_t val, void *addr)
{
	register_undo_mmio_writew(addr);
	mmio_writew(val, addr);
}

void rmmio_writel(uint32_t val, void *addr)
{
	register_undo_mmio_writel(addr);
	mmio_writel(val, addr);
}

void rmmio_le_writeb(uint8_t val, void *addr)
{
	register_undo_mmio_writeb(addr);
	mmio_le_writeb(val, addr);
}

void rmmio_le_writew(uint16_t val, void *addr)
{
	register_undo_mmio_writew(addr);
	mmio_le_writew(val, addr);
}

void rmmio_le_writel(uint32_t val, void *addr)
{
	register_undo_mmio_writel(addr);
	mmio_le_writel(val, addr);
}

void rmmio_valb(void *addr)
{
	register_undo_mmio_writeb(addr);
}

void rmmio_valw(void *addr)
{
	register_undo_mmio_writew(addr);
}

void rmmio_vall(void *addr)
{
	register_undo_mmio_writel(addr);
}
