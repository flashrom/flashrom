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

/* MSR abstraction implementations for Linux, OpenBSD, FreeBSD/Dragonfly, OSX, libpayload
 * and a non-working default implementation on the bottom.
 */

#include "hwaccess_x86_msr.h"
#include "flash.h"

#ifdef __linux__
/*
 * Reading and writing to MSRs, however requires instructions rdmsr/wrmsr,
 * which are ring0 privileged instructions so only the kernel can do the
 * read/write. This function, therefore, requires that the msr kernel module
 * be loaded to access these instructions from user space using device
 * /dev/cpu/0/msr.
 */

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static int fd_msr = -1;

msr_t msr_read(int addr)
{
	uint32_t buf[2];
	msr_t msr = { 0xffffffff, 0xffffffff };

	if (lseek(fd_msr, (off_t) addr, SEEK_SET) == -1) {
		msg_perr("Could not lseek() MSR: %s\n", strerror(errno));
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
		msg_perr("Could not read() MSR: %s\n", strerror(errno));
		close(fd_msr);
		exit(1);
	}

	return msr;
}

int msr_write(int addr, msr_t msr)
{
	uint32_t buf[2];
	buf[0] = msr.lo;
	buf[1] = msr.hi;

	if (lseek(fd_msr, (off_t) addr, SEEK_SET) == -1) {
		msg_perr("Could not lseek() MSR: %s\n", strerror(errno));
		close(fd_msr);
		exit(1);
	}

	if (write(fd_msr, buf, 8) != 8 && errno != EIO) {
		msg_perr("Could not write() MSR: %s\n", strerror(errno));
		close(fd_msr);
		exit(1);
	}

	/* Some MSRs must not be written. */
	if (errno == EIO)
		return -1;

	return 0;
}

int msr_setup(int cpu)
{
	char msrfilename[64] = { 0 };
	snprintf(msrfilename, sizeof(msrfilename), "/dev/cpu/%d/msr", cpu);

	if (fd_msr != -1) {
		msg_pinfo("MSR was already initialized\n");
		return -1;
	}

	fd_msr = open(msrfilename, O_RDWR);

	if (fd_msr < 0) {
		msg_perr("Error while opening %s: %s\n", msrfilename, strerror(errno));
		msg_pinfo("Did you run 'modprobe msr'?\n");
		return -1;
	}

	return 0;
}

void msr_cleanup(void)
{
	if (fd_msr == -1) {
		msg_pinfo("No MSR initialized.\n");
		return;
	}

	close(fd_msr);

	/* Clear MSR file descriptor. */
	fd_msr = -1;
}
#elif defined(__OpenBSD__) && defined (__i386__) /* This does only work for certain AMD Geode LX systems see amdmsr(4). */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <machine/amdmsr.h>

static int fd_msr = -1;

msr_t msr_read(int addr)
{
	struct amdmsr_req args;

	msr_t msr = { 0xffffffff, 0xffffffff };

	args.addr = (uint32_t)addr;

	if (ioctl(fd_msr, RDMSR, &args) < 0) {
		msg_perr("Error while executing RDMSR ioctl: %s\n", strerror(errno));
		close(fd_msr);
		exit(1);
	}

	msr.lo = args.val & 0xffffffff;
	msr.hi = args.val >> 32;

	return msr;
}

int msr_write(int addr, msr_t msr)
{
	struct amdmsr_req args;

	args.addr = addr;
	args.val = (((uint64_t)msr.hi) << 32) | msr.lo;

	if (ioctl(fd_msr, WRMSR, &args) < 0) {
		msg_perr("Error while executing WRMSR ioctl: %s\n", strerror(errno));
		close(fd_msr);
		exit(1);
	}

	return 0;
}

int msr_setup(int cpu)
{
	char msrfilename[64] = { 0 };
	snprintf(msrfilename, sizeof(msrfilename), "/dev/amdmsr");

	if (fd_msr != -1) {
		msg_pinfo("MSR was already initialized\n");
		return -1;
	}

	fd_msr = open(msrfilename, O_RDWR);

	if (fd_msr < 0) {
		msg_perr("Error while opening %s: %s\n", msrfilename, strerror(errno));
		return -1;
	}

	return 0;
}

void msr_cleanup(void)
{
	if (fd_msr == -1) {
		msg_pinfo("No MSR initialized.\n");
		return;
	}

	close(fd_msr);

	/* Clear MSR file descriptor. */
	fd_msr = -1;
}

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>

typedef struct {
	int msr;
	uint64_t data;
} cpu_msr_args_t;
#define CPU_RDMSR _IOWR('c', 1, cpu_msr_args_t)
#define CPU_WRMSR _IOWR('c', 2, cpu_msr_args_t)

static int fd_msr = -1;

msr_t msr_read(int addr)
{
	cpu_msr_args_t args;

	msr_t msr = { 0xffffffff, 0xffffffff };

	args.msr = addr;

	if (ioctl(fd_msr, CPU_RDMSR, &args) < 0) {
		msg_perr("Error while executing CPU_RDMSR ioctl: %s\n", strerror(errno));
		close(fd_msr);
		exit(1);
	}

	msr.lo = args.data & 0xffffffff;
	msr.hi = args.data >> 32;

	return msr;
}

int msr_write(int addr, msr_t msr)
{
	cpu_msr_args_t args;

	args.msr = addr;
	args.data = (((uint64_t)msr.hi) << 32) | msr.lo;

	if (ioctl(fd_msr, CPU_WRMSR, &args) < 0) {
		msg_perr("Error while executing CPU_WRMSR ioctl: %s\n", strerror(errno));
		close(fd_msr);
		exit(1);
	}

	return 0;
}

int msr_setup(int cpu)
{
	char msrfilename[64] = { 0 };
	snprintf(msrfilename, sizeof(msrfilename), "/dev/cpu%d", cpu);

	if (fd_msr != -1) {
		msg_pinfo("MSR was already initialized\n");
		return -1;
	}

	fd_msr = open(msrfilename, O_RDWR);

	if (fd_msr < 0) {
		msg_perr("Error while opening %s: %s\n", msrfilename, strerror(errno));
		msg_pinfo("Did you install ports/sysutils/devcpu?\n");
		return -1;
	}

	return 0;
}

void msr_cleanup(void)
{
	if (fd_msr == -1) {
		msg_pinfo("No MSR initialized.\n");
		return;
	}

	close(fd_msr);

	/* Clear MSR file descriptor. */
	fd_msr = -1;
}

#elif defined(__MACH__) && defined(__APPLE__)
/*
 * DirectHW has identical, but conflicting typedef for msr_t. We redefine msr_t
 * to directhw_msr_t for DirectHW.
 * rdmsr() and wrmsr() are provided by DirectHW and need neither setup nor cleanup.
 */
#define msr_t directhw_msr_t
#include <DirectHW/DirectHW.h>
#undef msr_t

msr_t msr_read(int addr)
{
	directhw_msr_t msr;
	msr = rdmsr(addr);
	return (msr_t){msr.hi, msr.lo};
}

int msr_write(int addr, msr_t msr)
{
	return wrmsr(addr, (directhw_msr_t){msr.hi, msr.lo});
}

int msr_setup(int cpu)
{
	// Always succeed for now
	return 0;
}

void msr_cleanup(void)
{
	// Nothing, yet.
}
#elif defined(__LIBPAYLOAD__)
#include <arch/msr.h>

msr_t msr_read(int addr)
{
	msr_t msr;
	unsigned long long val = _rdmsr(addr);
	msr.lo = val & 0xffffffff;
	msr.hi = val >> 32;
	return msr;
}

int msr_write(int addr, msr_t msr)
{
	_wrmsr(addr, msr.lo | ((unsigned long long)msr.hi << 32));
	return 0;
}

int msr_setup(int cpu)
{
	return 0;
}

void msr_cleanup(void)
{
}
#else
/* default MSR implementation */
msr_t msr_read(int addr)
{
	msr_t ret = { 0xffffffff, 0xffffffff };

	return ret;
}

int msr_write(int addr, msr_t msr)
{
	return -1;
}

int msr_setup(int cpu)
{
	msg_pinfo("No MSR support for your OS yet.\n");
	return -1;
}

void msr_cleanup(void)
{
	// Nothing, yet.
}
#endif // OS switches for MSR code
