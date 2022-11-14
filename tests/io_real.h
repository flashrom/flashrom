/*
 * This file is part of the flashrom project.
 *
 * Copyright 2022 Google LLC
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

#ifndef IO_REAL_H
#define IO_REAL_H

/* Detect file io that should not be mocked, for example code coverage writing
 * gcda files. Call io_mock_register with functions that defer to real io.
 */
void maybe_unmock_io(const char* pathname);

#endif /* IO_REAL_H */
