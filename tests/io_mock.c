/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2021, Google Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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
