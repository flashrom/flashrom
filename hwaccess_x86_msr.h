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

#if !(defined(__MACH__) && defined(__APPLE__)) && !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__) && !defined(__DragonFly__) && !defined(__LIBPAYLOAD__)
typedef struct { uint32_t hi, lo; } msr_t;
msr_t rdmsr(int addr);
int wrmsr(int addr, msr_t msr);
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
/* FreeBSD already has conflicting definitions for wrmsr/rdmsr. */
#undef rdmsr
#undef wrmsr
#define rdmsr freebsd_rdmsr
#define wrmsr freebsd_wrmsr
typedef struct { uint32_t hi, lo; } msr_t;
msr_t freebsd_rdmsr(int addr);
int freebsd_wrmsr(int addr, msr_t msr);
#endif

#if defined(__LIBPAYLOAD__)
#include <arch/io.h>
#include <arch/msr.h>
typedef struct { uint32_t hi, lo; } msr_t;
msr_t libpayload_rdmsr(int addr);
int libpayload_wrmsr(int addr, msr_t msr);
#undef rdmsr
#define rdmsr libpayload_rdmsr
#define wrmsr libpayload_wrmsr
#endif

int setup_cpu_msr(int cpu);
void cleanup_cpu_msr(void);

#endif /* __HWACCESS_X86_MSR_H__ */