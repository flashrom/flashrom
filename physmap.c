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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flash.h"
#include "hwaccess.h"

/* Do we need any file access or ioctl for physmap or MSR? */
#if !defined(__DJGPP__) && !defined(__LIBPAYLOAD__)
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif

#ifdef __DJGPP__
#include <dpmi.h>
#include <sys/nearptr.h>

#define MEM_DEV "dpmi"

static void *realmem_map;

static void *map_first_meg(unsigned long phys_addr, size_t len)
{
	if (realmem_map)
		return realmem_map + phys_addr;

	realmem_map = valloc(1024 * 1024);

	if (!realmem_map)
		return ERROR_PTR;

	if (__djgpp_map_physical_memory(realmem_map, (1024 * 1024), 0)) {
		free(realmem_map);
		realmem_map = NULL;
		return ERROR_PTR;
	}

	return realmem_map + phys_addr;
}

static void *sys_physmap(unsigned long phys_addr, size_t len)
{
	int ret;
	__dpmi_meminfo mi;

	/* Enable 4GB limit on DS descriptor. */
	if (!__djgpp_nearptr_enable())
		return ERROR_PTR;

	if ((phys_addr + len - 1) < (1024 * 1024)) {
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

void physunmap(void *virt_addr, size_t len)
{
	__dpmi_meminfo mi;

	/* There is no known way to unmap the first 1 MB. The DPMI server will
	 * do this for us on exit.
	 */
	if ((virt_addr >= realmem_map) &&
	    ((virt_addr + len) <= (realmem_map + (1024 * 1024)))) {
		return;
	}

	mi.address = (unsigned long) virt_addr;
	__dpmi_free_physical_address_mapping(&mi);
}

#elif defined(__LIBPAYLOAD__)
#include <arch/virtual.h>

#define MEM_DEV ""

void *sys_physmap(unsigned long phys_addr, size_t len)
{
	return (void *)phys_to_virt(phys_addr);
}

#define sys_physmap_rw_uncached	sys_physmap
#define sys_physmap_ro_cached	sys_physmap

void physunmap(void *virt_addr, size_t len)
{
}

int setup_cpu_msr(int cpu)
{
	return 0;
}

void cleanup_cpu_msr(void)
{
}
#elif defined(__MACH__) && defined(__APPLE__)

#define MEM_DEV "DirectHW"

static void *sys_physmap(unsigned long phys_addr, size_t len)
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

void physunmap(void *virt_addr, size_t len)
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
static void *sys_physmap_rw_uncached(unsigned long phys_addr, size_t len)
{
	void *virt_addr;

	if (-1 == fd_mem) {
		/* Open the memory device UNCACHED. Important for MMIO. */
		if (-1 == (fd_mem = open(MEM_DEV, O_RDWR | O_SYNC))) {
			perror("Critical error: open(" MEM_DEV ")");
			exit(2);
		}
	}

	virt_addr = mmap(NULL, len, PROT_WRITE | PROT_READ, MAP_SHARED,
			 fd_mem, (off_t)phys_addr);
	return MAP_FAILED == virt_addr ? ERROR_PTR : virt_addr;
}

/* For reading DMI/coreboot/whatever tables. We should never write, and we
 * do not care about caching.
 */
static void *sys_physmap_ro_cached(unsigned long phys_addr, size_t len)
{
	void *virt_addr;

	if (-1 == fd_mem_cached) {
		/* Open the memory device CACHED. */
		if (-1 == (fd_mem_cached = open(MEM_DEV, O_RDWR))) {
			msg_perr("Critical error: open(" MEM_DEV "): %s",
				 strerror(errno));
			exit(2);
		}
	}

	virt_addr = mmap(NULL, len, PROT_READ, MAP_SHARED,
			 fd_mem_cached, (off_t)phys_addr);
	return MAP_FAILED == virt_addr ? ERROR_PTR : virt_addr;
}

void physunmap(void *virt_addr, size_t len)
{
	if (len == 0) {
		msg_pspew("Not unmapping zero size at %p\n", virt_addr);
		return;
	}
		
	munmap(virt_addr, len);
}
#endif

#define PHYSMAP_NOFAIL	0
#define PHYSMAP_MAYFAIL	1
#define PHYSMAP_RW	0
#define PHYSMAP_RO	1

static void *physmap_common(const char *descr, unsigned long phys_addr,
			    size_t len, int mayfail, int readonly)
{
	void *virt_addr;

	if (len == 0) {
		msg_pspew("Not mapping %s, zero size at 0x%08lx.\n",
			  descr, phys_addr);
		return ERROR_PTR;
	}

	if ((getpagesize() - 1) & len) {
		msg_perr("Mapping %s at 0x%08lx, unaligned size 0x%lx.\n",
			 descr, phys_addr, (unsigned long)len);
	}

	if ((getpagesize() - 1) & phys_addr) {
		msg_perr("Mapping %s, 0x%lx bytes at unaligned 0x%08lx.\n",
			 descr, (unsigned long)len, phys_addr);
	}

	if (readonly)
		virt_addr = sys_physmap_ro_cached(phys_addr, len);
	else
		virt_addr = sys_physmap_rw_uncached(phys_addr, len);

	if (ERROR_PTR == virt_addr) {
		if (NULL == descr)
			descr = "memory";
		msg_perr("Error accessing %s, 0x%lx bytes at 0x%08lx\n", descr,
			 (unsigned long)len, phys_addr);
		perror(MEM_DEV " mmap failed");
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
		if (!mayfail)
			exit(3);
	}

	return virt_addr;
}

void *physmap(const char *descr, unsigned long phys_addr, size_t len)
{
	return physmap_common(descr, phys_addr, len, PHYSMAP_NOFAIL,
			      PHYSMAP_RW);
}

void *physmap_try_ro(const char *descr, unsigned long phys_addr, size_t len)
{
	return physmap_common(descr, phys_addr, len, PHYSMAP_MAYFAIL,
			      PHYSMAP_RO);
}

#if defined(__i386__) || defined(__x86_64__)

#ifdef __linux__
/*
 * Reading and writing to MSRs, however requires instructions rdmsr/wrmsr,
 * which are ring0 privileged instructions so only the kernel can do the
 * read/write. This function, therefore, requires that the msr kernel module
 * be loaded to access these instructions from user space using device
 * /dev/cpu/0/msr.
 */

static int fd_msr = -1;

msr_t rdmsr(int addr)
{
	uint32_t buf[2];
	msr_t msr = { 0xffffffff, 0xffffffff };

	if (lseek(fd_msr, (off_t) addr, SEEK_SET) == -1) {
		perror("Could not lseek() to MSR");
		close(fd_msr);
		exit(1);
	}

	if (read(fd_msr, buf, 8) == 8) {
		msr.lo = buf[0];
		msr.hi = buf[1];
		return msr;
	}

	if (errno != EIO) {
		// A severe error.
		perror("Could not read() MSR");
		close(fd_msr);
		exit(1);
	}

	return msr;
}

int wrmsr(int addr, msr_t msr)
{
	uint32_t buf[2];
	buf[0] = msr.lo;
	buf[1] = msr.hi;

	if (lseek(fd_msr, (off_t) addr, SEEK_SET) == -1) {
		perror("Could not lseek() to MSR");
		close(fd_msr);
		exit(1);
	}

	if (write(fd_msr, buf, 8) != 8 && errno != EIO) {
		perror("Could not write() MSR");
		close(fd_msr);
		exit(1);
	}

	/* Some MSRs must not be written. */
	if (errno == EIO)
		return -1;

	return 0;
}

int setup_cpu_msr(int cpu)
{
	char msrfilename[64];
	memset(msrfilename, 0, sizeof(msrfilename));
	snprintf(msrfilename, sizeof(msrfilename), "/dev/cpu/%d/msr", cpu);

	if (fd_msr != -1) {
		msg_pinfo("MSR was already initialized\n");
		return -1;
	}

	fd_msr = open(msrfilename, O_RDWR);

	if (fd_msr < 0) {
		perror("Error while opening /dev/cpu/0/msr");
		msg_pinfo("Did you run 'modprobe msr'?\n");
		return -1;
	}

	return 0;
}

void cleanup_cpu_msr(void)
{
	if (fd_msr == -1) {
		msg_pinfo("No MSR initialized.\n");
		return;
	}

	close(fd_msr);

	/* Clear MSR file descriptor. */
	fd_msr = -1;
}
#else
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
#include <sys/ioctl.h>

typedef struct {
	int msr;
	uint64_t data;
} cpu_msr_args_t;
#define CPU_RDMSR _IOWR('c', 1, cpu_msr_args_t)
#define CPU_WRMSR _IOWR('c', 2, cpu_msr_args_t)

static int fd_msr = -1;

msr_t rdmsr(int addr)
{
	cpu_msr_args_t args;

	msr_t msr = { 0xffffffff, 0xffffffff };

	args.msr = addr;

	if (ioctl(fd_msr, CPU_RDMSR, &args) < 0) {
		perror("CPU_RDMSR");
		close(fd_msr);
		exit(1);
	}

	msr.lo = args.data & 0xffffffff;
	msr.hi = args.data >> 32;

	return msr;
}

int wrmsr(int addr, msr_t msr)
{
	cpu_msr_args_t args;

	args.msr = addr;
	args.data = (((uint64_t)msr.hi) << 32) | msr.lo;

	if (ioctl(fd_msr, CPU_WRMSR, &args) < 0) {
		perror("CPU_WRMSR");
		close(fd_msr);
		exit(1);
	}

	return 0;
}

int setup_cpu_msr(int cpu)
{
	char msrfilename[64];
	memset(msrfilename, 0, sizeof(msrfilename));
	snprintf(msrfilename, sizeof(msrfilename), "/dev/cpu%d", cpu);

	if (fd_msr != -1) {
		msg_pinfo("MSR was already initialized\n");
		return -1;
	}

	fd_msr = open(msrfilename, O_RDWR);

	if (fd_msr < 0) {
		perror("Error while opening /dev/cpu0");
		msg_pinfo("Did you install ports/sysutils/devcpu?\n");
		return -1;
	}

	return 0;
}

void cleanup_cpu_msr(void)
{
	if (fd_msr == -1) {
		msg_pinfo("No MSR initialized.\n");
		return;
	}

	close(fd_msr);

	/* Clear MSR file descriptor. */
	fd_msr = -1;
}

#else

#if defined(__MACH__) && defined(__APPLE__)
int setup_cpu_msr(int cpu)
{
	// Always succeed for now
	return 0;
}

void cleanup_cpu_msr(void)
{
	// Nothing, yet.
}
#elif defined(__LIBPAYLOAD__)
msr_t libpayload_rdmsr(int addr)
{
	msr_t msr;
	unsigned long long val = _rdmsr(addr);
	msr.lo = val & 0xffffffff;
	msr.hi = val >> 32;
	return msr;
}

int libpayload_wrmsr(int addr, msr_t msr)
{
	_wrmsr(addr, msr.lo | ((unsigned long long)msr.hi << 32));
	return 0;
}
#else
msr_t rdmsr(int addr)
{
	msr_t ret = { 0xffffffff, 0xffffffff };

	return ret;
}

int wrmsr(int addr, msr_t msr)
{
	return -1;
}

int setup_cpu_msr(int cpu)
{
	msg_pinfo("No MSR support for your OS yet.\n");
	return -1;
}

void cleanup_cpu_msr(void)
{
	// Nothing, yet.
}
#endif
#endif
#endif
#else
/* Does MSR exist on non-x86 architectures? */
#endif
