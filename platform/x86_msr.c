/*
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2009 Peter Stuge <peter@stuge.se>
 * SPDX-FileCopyrightText: 2009 coresystems GmbH
 * SPDX-FileCopyrightText: 2010 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2010 Rudolf Marek <r.marek@assembler.cz>
 */

/* Default MSR implementation, for systems without MSR support. */

#include "platform/x86_msr.h"

#include "log.h"

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
