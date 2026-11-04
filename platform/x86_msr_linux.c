/*
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2009 Peter Stuge <peter@stuge.se>
 * SPDX-FileCopyrightText: 2009 coresystems GmbH
 * SPDX-FileCopyrightText: 2010 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2010 Rudolf Marek <r.marek@assembler.cz>
 */

/*
 * Reading and writing to MSRs, however requires instructions rdmsr/wrmsr,
 * which are ring0 privileged instructions so only the kernel can do the
 * read/write. This function, therefore, requires that the msr kernel module
 * be loaded to access these instructions from user space using device
 * /dev/cpu/0/msr.
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
