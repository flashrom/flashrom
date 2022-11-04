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

#include "io_mock.h"
#include "wraps.h"

#include "io_real.h"
#include <string.h>

static int io_real_open(void *state, const char *pathname, int flags, mode_t mode)
{
	LOG_ME;
	return __real_open(pathname, flags, mode);
}

static FILE *io_real_fopen(void *state, const char *pathname, const char *mode)
{
	LOG_ME;
	return __real_fopen(pathname, mode);
}

static FILE *io_real_fdopen(void *state, int fd, const char *mode)
{
	LOG_ME;
	return __real_fdopen(fd, mode);
}

static size_t io_real_fwrite(void *state, const void *ptr, size_t size, size_t nmemb, FILE *fp)
{
	return __real_fwrite(ptr, size, nmemb, fp);
}

static int io_real_fclose(void *state, FILE *fp)
{
	LOG_ME;
	return __real_fclose(fp);
}

/* Mock ios that defer to the real io functions.
 * These exist so that code coverage can print to real files on disk.
 */
static const struct io_mock real_io = {
	.iom_open = io_real_open,
	.iom_fopen = io_real_fopen,
	.iom_fwrite = io_real_fwrite,
	.iom_fdopen = io_real_fdopen,
	.iom_fclose = io_real_fclose,
};

/* Return 0 if string ends with suffix. */
static int check_suffix(const char *string, const char *suffix)
{
	int len_l = strlen(string);
	int len_r = strlen(suffix);
	if (len_l > len_r)
		return strcmp(string + len_l - len_r, suffix);
	return 1;
}

void maybe_unmock_io(const char *pathname)
{
	const char *gcov_suffix = ".gcda";
	const char *llvm_cov_suffix = ".profraw";
	if (!check_suffix(pathname, gcov_suffix) || !check_suffix(pathname, llvm_cov_suffix))
		io_mock_register(&real_io);
}
