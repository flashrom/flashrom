/*
 * This file is part of the flashrom project.
 *
 * Copyright 2021 Google LLC
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

/*
 * This header is used instead of hwaccess_x86_io.h for unit tests
 * (see flashrom_test_dep in meson.build).
 *
 * There is no hardware in unit test environment and all hardware operations
 * need to be mocked.
 */

/*
 * The same guard is used intentionally for hwaccess_x86_io.h and
 * hwaccess_x86_io_unittest.h. When build is made for the test environment,
 * hwaccess_x86_io_unittest.h is included first, and it effectively
 * replaces hwaccess_x86_io.h.
 */
#ifndef __HWACCESS_X86_IO_H__
#define __HWACCESS_X86_IO_H__ 1

#define OUTB(v, p) test_outb(v, p)
#define OUTW(v, p) test_outw(v, p)
#define OUTL(v, p) test_outl(v, p)
#define INB(p) test_inb(p)
#define INW(p) test_inw(p)
#define INL(p) test_inl(p)

#include <stdint.h>

/*
 * Dummy implementation of iopl from sys/io.h.
 * sys/io.h by itself is platform-specific, so instead of including
 * the header we just have this dummy function, which is sufficient
 * for test purposes.
 */
static inline int iopl(int level)
{
	return 0;
}

/* All functions below are mocked in unit tests. */

void test_outb(uint8_t value, uint16_t port);
uint8_t test_inb(uint16_t port);
void test_outw(uint16_t value, uint16_t port);
uint16_t test_inw(uint16_t port);
void test_outl(uint32_t value, uint16_t port);
uint32_t test_inl(uint16_t port);

#endif /* !__HWACCESS_X86_IO_H__ */
