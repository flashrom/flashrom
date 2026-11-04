/*
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2009 Peter Stuge <peter@stuge.se>
 * SPDX-FileCopyrightText: 2009 coresystems GmbH
 * SPDX-FileCopyrightText: 2010 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2010 Rudolf Marek <r.marek@assembler.cz>
 */

#include "platform/x86_msr.h"

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
