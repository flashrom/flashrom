/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2009-2010 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2013 Stefan Tauner
 */

#ifndef __HELPERS_FILEIO_H__
#define __HELPERS_FILEIO_H__

int read_buf_from_file(unsigned char *buf, unsigned long size, const char *filename);
int write_buf_to_file(const unsigned char *buf, unsigned long size, const char *filename);

#endif /* __HELPERS_FILEIO_H__ */
