/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Peter Stuge <peter@stuge.se>
 * Copyright (C) 2009 coresystems GmbH
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "flash.h"

#ifdef __DARWIN__
#include <DirectIO/darwinio.h>

#define MEM_DEV "DirectIO"

void *sys_physmap(unsigned long phys_addr, size_t len)
{
	return map_physical(phys_addr, len);
}

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

void *sys_physmap(unsigned long phys_addr, size_t len)
{
	void *virt_addr;

	if (-1 == fd_mem) {
		/* Open the memory device UNCACHED. Important for MMIO. */
		if (-1 == (fd_mem = open(MEM_DEV, O_RDWR | O_SYNC))) {
			perror("Critical error: open(" MEM_DEV ")");
			exit(2);
		}
	}

	virt_addr = mmap(0, len, PROT_WRITE | PROT_READ, MAP_SHARED,
			 fd_mem, (off_t)phys_addr);
	return MAP_FAILED == virt_addr ? NULL : virt_addr;
}

void physunmap(void *virt_addr, size_t len)
{
	if (len == 0) {
		printf_debug("Not unmapping zero size at %p\n", virt_addr);
		return;
	}
		
	munmap(virt_addr, len);
}
#endif

void *physmap(const char *descr, unsigned long phys_addr, size_t len)
{
	void *virt_addr;

	if (len == 0) {
		printf_debug("Not mapping %s, zero size at 0x%08lx.\n",
			     descr, phys_addr);
		return NULL;
	}
		
	if ((getpagesize() - 1) & len) {
		fprintf(stderr, "Mapping %s at 0x%08lx, unaligned size 0x%lx.\n",
			descr, phys_addr, (unsigned long)len);
	}

	if ((getpagesize() - 1) & phys_addr) {
		fprintf(stderr, "Mapping %s, 0x%lx bytes at unaligned 0x%08lx.\n",
			descr, (unsigned long)len, phys_addr);
	}

	virt_addr = sys_physmap(phys_addr, len);

	if (NULL == virt_addr) {
		if (NULL == descr)
			descr = "memory";
		fprintf(stderr, "Error accessing %s, 0x%lx bytes at 0x%08lx\n", descr, (unsigned long)len, phys_addr);
		perror(MEM_DEV " mmap failed");
		if (EINVAL == errno) {
			fprintf(stderr, "In Linux this error can be caused by the CONFIG_NONPROMISC_DEVMEM (<2.6.27),\n");
			fprintf(stderr, "CONFIG_STRICT_DEVMEM (>=2.6.27) and CONFIG_X86_PAT kernel options.\n");
			fprintf(stderr, "Please check if either is enabled in your kernel before reporting a failure.\n");
			fprintf(stderr, "You can override CONFIG_X86_PAT at boot with the nopat kernel parameter but\n");
			fprintf(stderr, "disabling the other option unfortunately requires a kernel recompile. Sorry!\n");
		}
		exit(3);
	}

	return virt_addr;
}

#ifdef __linux__
/*
 * Reading and writing to MSRs, however requires instructions rdmsr/wrmsr,
 * which are ring0 privileged instructions so only the kernel can do the
 * read/write.  This function, therefore, requires that the msr kernel module
 * be loaded to access these instructions from user space using device
 * /dev/cpu/0/msr.
 */

static int fd_msr = -1;

msr_t rdmsr(int addr)
{
	uint8_t buf[8];
	msr_t msr = { 0xffffffff, 0xffffffff };

	if (lseek(fd_msr, (off_t) addr, SEEK_SET) == -1) {
		perror("Could not lseek() to MSR");
		close(fd_msr);
		exit(1);
	}

	if (read(fd_msr, buf, 8) == 8) {
		msr.lo = *(uint32_t *)buf;
		msr.hi = *(uint32_t *)(buf + 4);

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
	if (lseek(fd_msr, (off_t) addr, SEEK_SET) == -1) {
		perror("Could not lseek() to MSR");
		close(fd_msr);
		exit(1);
	}

	if (write(fd_msr, &msr, 8) != 8 && errno != EIO) {
		perror("Could not write() MSR");
		close(fd_msr);
		exit(1);
	}

	/* some MSRs must not be written */
	if (errno == EIO)
		return -1;

	return 0;
}

int setup_cpu_msr(int cpu)
{
	char msrfilename[64];
	memset(msrfilename, 0, 64);
	sprintf(msrfilename, "/dev/cpu/%d/msr", cpu);

	if (fd_msr != -1) {
		printf("MSR was already initialized\n");
		return -1;
	}

	fd_msr = open(msrfilename, O_RDWR);

	if (fd_msr < 0) {
		perror("Error while opening /dev/cpu/0/msr");
		printf("Did you run 'modprobe msr'?\n");
		return -1;
	}

	return 0;
}

void cleanup_cpu_msr(void)
{
	if (fd_msr == -1) {
		printf("No MSR initialized.\n");
		return;
	}

	close(fd_msr);

	/* Clear MSR file descriptor */
	fd_msr = -1;
}
#else
#ifdef __DARWIN__
int setup_cpu_msr(int cpu)
{
	// Always succeed for now
	return 0;
}

void cleanup_cpu_msr(void)
{
	// Nothing, yet.
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
	printf("No MSR support for your OS yet.\n");
	return -1;
}

void cleanup_cpu_msr(void)
{
	// Nothing, yet.
}
#endif
#endif

