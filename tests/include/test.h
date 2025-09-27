/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2020 Google LLC
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
