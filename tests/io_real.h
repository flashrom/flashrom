/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2022 Google LLC
 */

#ifndef IO_REAL_H
#define IO_REAL_H

/* Detect file io that should not be mocked, for example code coverage writing
 * gcda files. Call io_mock_register with functions that defer to real io.
 */
void maybe_unmock_io(const char* pathname);

#endif /* IO_REAL_H */
