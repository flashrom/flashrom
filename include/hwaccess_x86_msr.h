/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Carl-Daniel Hailfinger
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

#ifndef __HWACCESS_X86_MSR_H__
#define __HWACCESS_X86_MSR_H__ 1

#include <stdint.h>

typedef struct { uint32_t hi, lo; } msr_t;

msr_t msr_read(int addr);
int msr_write(int addr, msr_t msr);
int msr_setup(int cpu);
void msr_cleanup(void);

#endif /* __HWACCESS_X86_MSR_H__ */