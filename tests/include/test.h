/*
 * This file is part of the flashrom project.
 *
 * Copyright 2020 Google LLC
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

#ifndef _TESTS_TEST_H
#define _TESTS_TEST_H

/*
 * Standard test header that should be included in all tests. For now it just encapsulates the
 * include dependencies for Cmocka. Test-specific APIs that are so generic we would want them
 * available everywhere could also be added here.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#define NON_ZERO (0xf000baaa)

#define MOCK_FD (0x10ec)

#define SKIP_TEST(name) \
	void name (void **state) { skip(); }

/*
 * Having this as function allows to set a breakpoint on the address,
 * as it has a named symbol associated with the address number.
 */
void *not_null(void);

#define LOG_ME printf("%s is called\n", __func__)

#endif /* _TESTS_TEST_H */
