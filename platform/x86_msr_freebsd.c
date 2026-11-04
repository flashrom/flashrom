/*
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2009 Peter Stuge <peter@stuge.se>
 * SPDX-FileCopyrightText: 2009 coresystems GmbH
 * SPDX-FileCopyrightText: 2010 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2010 Rudolf Marek <r.marek@assembler.cz>
 */

#include "platform/x86_msr.h"

#include "log.h"
#include "platform/string.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/cpuctl.h>

static int fd_msr = -1;

msr_t msr_read(int addr)
{
	cpuctl_msr_args_t args;

	msr_t msr = { 0xffffffff, 0xffffffff };

	args.msr = addr;

	if (ioctl(fd_msr, CPUCTL_RDMSR, &args) < 0) {
		msg_perr("Error while executing CPUCTL_RDMSR ioctl: %s\n", strerror(errno));
		close(fd_msr);
		exit(1);
	}

	msr.lo = args.data & 0xffffffff;
	msr.hi = args.data >> 32;

	return msr;
}

int msr_write(int addr, msr_t msr)
{
	cpuctl_msr_args_t args;

	args.msr = addr;
	args.data = (((uint64_t)msr.hi) << 32) | msr.lo;

	if (ioctl(fd_msr, CPUCTL_WRMSR, &args) < 0) {
		msg_perr("Error while executing CPUCTL_WRMSR ioctl: %s\n", strerror(errno));
		close(fd_msr);
		exit(1);
	}

	return 0;
}

int msr_setup(int cpu)
{
	char msrfilename[64] = { 0 };
	snprintf(msrfilename, sizeof(msrfilename), "/dev/cpuctl%d", cpu);

	if (fd_msr != -1) {
		msg_pinfo("MSR was already initialized\n");
		return -1;
	}

	fd_msr = open(msrfilename, O_RDWR);

	if (fd_msr < 0) {
		msg_perr("Error while opening %s: %s\n", msrfilename, strerror(errno));
		msg_pinfo("Try 'kldload cpuctl' or add cpuctl_load=\"YES\" to /boot/loader.conf.\n");
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
