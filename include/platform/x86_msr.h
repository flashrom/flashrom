/*
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2009 Carl-Daniel Hailfinger
 */

#ifndef __PLATFORM_X86_MSR_H__
#define __PLATFORM_X86_MSR_H__

#include <stdint.h>

typedef struct { uint32_t hi, lo; } msr_t;

msr_t msr_read(int addr);
int msr_write(int addr, msr_t msr);
int msr_setup(int cpu);
void msr_cleanup(void);

#endif /* __PLATFORM_X86_MSR_H__ */
