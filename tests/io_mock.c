/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2021, Google Inc. All rights reserved.
 */

#include "io_mock.h"

static const struct io_mock *current_io = NULL;

void io_mock_register(const struct io_mock *io)
{
	/* A test can either register its own mock open function or fallback_open_state. */
	assert_true(io == NULL || io->iom_open == NULL || io->fallback_open_state == NULL);
	current_io = io;
}

const struct io_mock *get_io(void)
{
	return current_io;
}
