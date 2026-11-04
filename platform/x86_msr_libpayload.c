/*
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2009 Peter Stuge <peter@stuge.se>
 * SPDX-FileCopyrightText: 2009 coresystems GmbH
 * SPDX-FileCopyrightText: 2010 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2010 Rudolf Marek <r.marek@assembler.cz>
 */

#include "platform/x86_msr.h"

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
