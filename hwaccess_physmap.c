/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Peter Stuge <peter@stuge.se>
 * Copyright (C) 2009 coresystems GmbH
 * Copyright (C) 2010 Carl-Daniel Hailfinger
 * Copyright (C) 2010 Rudolf Marek <r.marek@assembler.cz>
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

#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "flash.h"
#include "platform.h"
#include "hwaccess_physmap.h"

#if !defined(__DJGPP__) && !defined(__LIBPAYLOAD__)
/* No file access needed/possible to get mmap access permissions or access MSR. */
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#endif

#ifdef __DJGPP__
#include <dpmi.h>
#include <malloc.h>
#include <sys/nearptr.h>

#define ONE_MEGABYTE (1024 * 1024)
#define MEM_DEV "dpmi"

static void *realmem_map_aligned;

static void *map_first_meg(uintptr_t phys_addr, size_t len)
{
	void *realmem_map;
	size_t pagesize;

	if (realmem_map_aligned)
		return realmem_map_aligned + phys_addr;

	/* valloc() from DJGPP 2.05 does not work properly */
	pagesize = getpagesize();

	realmem_map = malloc(ONE_MEGABYTE + pagesize);

	if (!realmem_map)
		return ERROR_PTR;

	realmem_map_aligned = (void *)(((size_t) realmem_map +
		(pagesize - 1)) & ~(pagesize - 1));

	if (__djgpp_map_physical_memory(realmem_map_aligned, ONE_MEGABYTE, 0)) {
		free(realmem_map);
		realmem_map_aligned = NULL;
		return ERROR_PTR;
	}

	return realmem_map_aligned + phys_addr;
}

static void *sys_physmap(uintptr_t phys_addr, size_t len)
{
	int ret;
	__dpmi_meminfo mi;

	/* Enable 4GB limit on DS descriptor. */
	if (!__djgpp_nearptr_enable())
		return ERROR_PTR;

	if ((phys_addr + len - 1) < ONE_MEGABYTE) {
		/* We need to use another method to map first 1MB. */
		return map_first_meg(phys_addr, len);
	}

	mi.address = phys_addr;
	mi.size = len;
	ret = __dpmi_physical_address_mapping(&mi);

	if (ret != 0)
		return ERROR_PTR;

	return (void *) mi.address + __djgpp_conventional_base;
}

#define sys_physmap_rw_uncached	sys_physmap
#define sys_physmap_ro_cached	sys_physmap

static void sys_physunmap_unaligned(void *virt_addr, size_t len)
{
	__dpmi_meminfo mi;

	/* There is no known way to unmap the first 1 MB. The DPMI server will
	 * do this for us on exit.
	 */
	if ((virt_addr >= realmem_map_aligned) &&
	    ((virt_addr + len) <= (realmem_map_aligned + ONE_MEGABYTE))) {
		return;
	}

	mi.address = (unsigned long) virt_addr;
	__dpmi_free_physical_address_mapping(&mi);
}

#elif defined(__LIBPAYLOAD__)
#include <arch/virtual.h>

#define MEM_DEV ""

static void *sys_physmap(uintptr_t phys_addr, size_t len)
{
	return (void *)phys_to_virt(phys_addr);
}

#define sys_physmap_rw_uncached	sys_physmap
#define sys_physmap_ro_cached	sys_physmap

static void sys_physunmap_unaligned(void *virt_addr, size_t len)
{
}
#elif defined(__MACH__) && defined(__APPLE__)
#include <DirectHW/DirectHW.h>

#define MEM_DEV "DirectHW"

static void *sys_physmap(uintptr_t phys_addr, size_t len)
{
	/* The short form of ?: is a GNU extension.
	 * FIXME: map_physical returns NULL both for errors and for success
	 * if the region is mapped at virtual address zero. If in doubt, report
	 * an error until a better interface exists.
	 */
	return map_physical(phys_addr, len) ? : ERROR_PTR;
}

/* The OS X driver does not differentiate between mapping types. */
#define sys_physmap_rw_uncached	sys_physmap
#define sys_physmap_ro_cached	sys_physmap

static void sys_physunmap_unaligned(void *virt_addr, size_t len)
{
	unmap_physical(virt_addr, len);
}

#else
#include <sys/mman.h>

#if defined (__sun) && (defined(__i386) || defined(__amd64))
#  define MEM_DEV "/dev/xsvc"
#else
#  define MEM_DEV "/dev/mem"
#endif

static int fd_mem = -1;
static int fd_mem_cached = -1;

/* For MMIO access. Must be uncached, doesn't make sense to restrict to ro. */
static void *sys_physmap_rw_uncached(uintptr_t phys_addr, size_t len)
{
	void *virt_addr;

	if (-1 == fd_mem) {
		/* Open the memory device UNCACHED. Important for MMIO. */
		if (-1 == (fd_mem = open(MEM_DEV, O_RDWR | O_SYNC))) {
			msg_perr("Critical error: open(" MEM_DEV "): %s\n", strerror(errno));
			return ERROR_PTR;
		}
	}

	virt_addr = mmap(NULL, len, PROT_WRITE | PROT_READ, MAP_SHARED, fd_mem, (off_t)phys_addr);
	return MAP_FAILED == virt_addr ? ERROR_PTR : virt_addr;
}

/* For reading DMI/coreboot/whatever tables. We should never write, and we
 * do not care about caching.
 */
static void *sys_physmap_ro_cached(uintptr_t phys_addr, size_t len)
{
	void *virt_addr;

	if (-1 == fd_mem_cached) {
		/* Open the memory device CACHED. */
		if (-1 == (fd_mem_cached = open(MEM_DEV, O_RDWR))) {
			msg_perr("Critical error: open(" MEM_DEV "): %s\n", strerror(errno));
			return ERROR_PTR;
		}
	}

	virt_addr = mmap(NULL, len, PROT_READ, MAP_SHARED, fd_mem_cached, (off_t)phys_addr);
	return MAP_FAILED == virt_addr ? ERROR_PTR : virt_addr;
}

static void sys_physunmap_unaligned(void *virt_addr, size_t len)
{
	munmap(virt_addr, len);
}
#endif

#define PHYSM_RW	0
#define PHYSM_RO	1
#define PHYSM_NOCLEANUP	0
#define PHYSM_CLEANUP	1
#define PHYSM_EXACT	0
#define PHYSM_ROUND	1

/* Round start to nearest page boundary below and set len so that the resulting address range ends at the lowest
 * possible page boundary where the original address range is still entirely contained. It returns the
 * difference between the rounded start address and the original start address. */
static uintptr_t round_to_page_boundaries(uintptr_t *start, size_t *len)
{
	uintptr_t page_size = getpagesize();
	uintptr_t page_mask = ~(page_size-1);
	uintptr_t end = *start + *len;
	uintptr_t old_start = *start;
	msg_gspew("page_size=%" PRIxPTR "\n", page_size);
	msg_gspew("pre-rounding:  start=0x%0*" PRIxPTR ", len=0x%zx, end=0x%0*" PRIxPTR "\n",
		  PRIxPTR_WIDTH, *start, *len, PRIxPTR_WIDTH, end);
	*start = *start & page_mask;
	end = (end + page_size - 1) & page_mask;
	*len = end - *start;
	msg_gspew("post-rounding: start=0x%0*" PRIxPTR ", len=0x%zx, end=0x%0*" PRIxPTR "\n",
		  PRIxPTR_WIDTH, *start, *len, PRIxPTR_WIDTH, *start + *len);
	return old_start - *start;
}

struct undo_physmap_data {
	void *virt_addr;
	size_t len;
};

static int undo_physmap(void *data)
{
	if (data == NULL) {
		msg_perr("%s: tried to physunmap without valid data!\n", __func__);
		return 1;
	}
	struct undo_physmap_data *d = data;
	physunmap_unaligned(d->virt_addr, d->len);
	free(data);
	return 0;
}

static void *physmap_common(const char *descr, uintptr_t phys_addr, size_t len, bool readonly, bool autocleanup,
			    bool round)
{
	void *virt_addr;
	uintptr_t offset = 0;

	if (len == 0) {
		msg_pspew("Not mapping %s, zero size at 0x%0*" PRIxPTR ".\n", descr, PRIxPTR_WIDTH, phys_addr);
		return ERROR_PTR;
	}

	if (round)
		offset = round_to_page_boundaries(&phys_addr, &len);

	if (readonly)
		virt_addr = sys_physmap_ro_cached(phys_addr, len);
	else
		virt_addr = sys_physmap_rw_uncached(phys_addr, len);

	if (ERROR_PTR == virt_addr) {
		if (NULL == descr)
			descr = "memory";
		msg_perr("Error accessing %s, 0x%zx bytes at 0x%0*" PRIxPTR "\n",
			 descr, len, PRIxPTR_WIDTH, phys_addr);
		msg_perr(MEM_DEV " mmap failed: %s\n", strerror(errno));
#ifdef __linux__
		if (EINVAL == errno) {
			msg_perr("In Linux this error can be caused by the CONFIG_NONPROMISC_DEVMEM (<2.6.27),\n");
			msg_perr("CONFIG_STRICT_DEVMEM (>=2.6.27) and CONFIG_X86_PAT kernel options.\n");
			msg_perr("Please check if either is enabled in your kernel before reporting a failure.\n");
			msg_perr("You can override CONFIG_X86_PAT at boot with the nopat kernel parameter but\n");
			msg_perr("disabling the other option unfortunately requires a kernel recompile. Sorry!\n");
		}
#elif defined (__OpenBSD__)
		msg_perr("Please set securelevel=-1 in /etc/rc.securelevel "
			 "and reboot, or reboot into\n"
			 "single user mode.\n");
#endif
		return ERROR_PTR;
	}

	if (autocleanup) {
		struct undo_physmap_data *d = malloc(sizeof(*d));
		if (d == NULL) {
			msg_perr("%s: Out of memory!\n", __func__);
			physunmap_unaligned(virt_addr, len);
			return ERROR_PTR;
		}

		d->virt_addr = virt_addr;
		d->len = len;
		if (register_shutdown(undo_physmap, d) != 0) {
			msg_perr("%s: Could not register shutdown function!\n", __func__);
			physunmap_unaligned(virt_addr, len);
			return ERROR_PTR;
		}
	}

	return virt_addr + offset;
}

void physunmap_unaligned(void *virt_addr, size_t len)
{
	/* No need to check for zero size, such mappings would have yielded ERROR_PTR. */
	if (virt_addr == ERROR_PTR) {
		msg_perr("Trying to unmap a nonexisting mapping!\n"
			 "Please report a bug at flashrom@flashrom.org\n");
		return;
	}

	sys_physunmap_unaligned(virt_addr, len);
}

void physunmap(void *virt_addr, size_t len)
{
	uintptr_t tmp;

	/* No need to check for zero size, such mappings would have yielded ERROR_PTR. */
	if (virt_addr == ERROR_PTR) {
		msg_perr("Trying to unmap a nonexisting mapping!\n"
			 "Please report a bug at flashrom@flashrom.org\n");
		return;
	}
	tmp = (uintptr_t)virt_addr;
	/* We assume that the virtual address of a page-aligned physical address is page-aligned as well. By
	 * extension, rounding a virtual unaligned address as returned by physmap should yield the same offset
	 * between rounded and original virtual address as between rounded and original physical address.
	 */
	round_to_page_boundaries(&tmp, &len);
	virt_addr = (void *)tmp;
	physunmap_unaligned(virt_addr, len);
}

void *physmap(const char *descr, uintptr_t phys_addr, size_t len)
{
	return physmap_common(descr, phys_addr, len, PHYSM_RW, PHYSM_NOCLEANUP, PHYSM_ROUND);
}

void *rphysmap(const char *descr, uintptr_t phys_addr, size_t len)
{
	return physmap_common(descr, phys_addr, len, PHYSM_RW, PHYSM_CLEANUP, PHYSM_ROUND);
}

void *physmap_ro(const char *descr, uintptr_t phys_addr, size_t len)
{
	return physmap_common(descr, phys_addr, len, PHYSM_RO, PHYSM_NOCLEANUP, PHYSM_ROUND);
}

void *physmap_ro_unaligned(const char *descr, uintptr_t phys_addr, size_t len)
{
	return physmap_common(descr, phys_addr, len, PHYSM_RO, PHYSM_NOCLEANUP, PHYSM_EXACT);
}

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
// cf. http://lxr.free-electrons.com/source/arch/powerpc/include/asm/barrier.h
#if defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || \
      defined(__ppc__) || defined(__ppc64__) || defined(_M_PPC) || defined(_ARCH_PPC) || \
      defined(_ARCH_PPC64) || defined(__ppc)
	__asm__("eieio" : : : "memory");
#elif (__sparc__) || defined (__sparc)
#if defined(__sparc_v9__) || defined(__sparcv9)
	/* Sparc V9 CPUs support three different memory orderings that range from x86-like TSO to PowerPC-like
	 * RMO. The modes can be switched at runtime thus to make sure we maintain the right order of access we
	 * use the strongest hardware memory barriers that exist on Sparc V9. */
	__asm__ volatile ("membar #Sync" ::: "memory");
#elif defined(__sparc_v8__) || defined(__sparcv8)
	/* On SPARC V8 there is no RMO just PSO and that does not apply to I/O accesses... but if V8 code is run
	 * on V9 CPUs it might apply... or not... we issue a write barrier anyway. That's the most suitable
	 * operation in the V8 instruction set anyway. If you know better then please tell us. */
	__asm__ volatile ("stbar");
#else
	#error Unknown and/or unsupported SPARC instruction set version detected.
#endif
#endif
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

uint8_t mmio_readb(const void *addr)
{
	return *(volatile const uint8_t *) addr;
}

uint16_t mmio_readw(const void *addr)
{
	return *(volatile const uint16_t *) addr;
}

uint32_t mmio_readl(const void *addr)
{
	return *(volatile const uint32_t *) addr;
}

void mmio_readn(const void *addr, uint8_t *buf, size_t len)
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

uint8_t mmio_le_readb(const void *addr)
{
	return le_to_cpu8(mmio_readb(addr));
}

uint16_t mmio_le_readw(const void *addr)
{
	return le_to_cpu16(mmio_readw(addr));
}

uint32_t mmio_le_readl(const void *addr)
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

static int undo_mmio_write(void *p)
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
	undo_mmio_write_data = malloc(sizeof(*undo_mmio_write_data));	\
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
