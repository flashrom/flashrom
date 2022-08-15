/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009-2010 Carl-Daniel Hailfinger
 * Copyright (C) 2013 Stefan Tauner
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef __LIBPAYLOAD__
#include <fcntl.h>
#include <sys/stat.h>
#endif

#include "flash.h"

int read_buf_from_file(unsigned char *buf, unsigned long size,
		       const char *filename)
{
#ifdef __LIBPAYLOAD__
	msg_gerr("Error: No file I/O support in libpayload\n");
	return 1;
#else
	int ret = 0;

	FILE *image;
	if (!strcmp(filename, "-"))
		image = fdopen(fileno(stdin), "rb");
	else
		image = fopen(filename, "rb");
	if (image == NULL) {
		msg_gerr("Error: opening file \"%s\" failed: %s\n", filename, strerror(errno));
		return 1;
	}

	struct stat image_stat;
	if (fstat(fileno(image), &image_stat) != 0) {
		msg_gerr("Error: getting metadata of file \"%s\" failed: %s\n", filename, strerror(errno));
		ret = 1;
		goto out;
	}
	if ((image_stat.st_size != (intmax_t)size) && strcmp(filename, "-")) {
		msg_gerr("Error: Image size (%jd B) doesn't match the expected size (%lu B)!\n",
			 (intmax_t)image_stat.st_size, size);
		ret = 1;
		goto out;
	}

	unsigned long numbytes = fread(buf, 1, size, image);
	if (numbytes != size) {
		msg_gerr("Error: Failed to read complete file. Got %ld bytes, "
			 "wanted %ld!\n", numbytes, size);
		ret = 1;
	}
out:
	(void)fclose(image);
	return ret;
#endif
}

/**
 * @brief Writes passed data buffer into a file
 *
 * @param buf      Buffer with data to write
 * @param size     Size of buffer
 * @param filename File path to write to
 * @return 0 on success
 */
int write_buf_to_file(const unsigned char *buf, unsigned long size, const char *filename)
{
#ifdef __LIBPAYLOAD__
	msg_gerr("Error: No file I/O support in libpayload\n");
	return 1;
#else
	FILE *image;
	int ret = 0;

	if (!filename) {
		msg_gerr("No filename specified.\n");
		return 1;
	}
	if ((image = fopen(filename, "wb")) == NULL) {
		msg_gerr("Error: opening file \"%s\" failed: %s\n", filename, strerror(errno));
		return 1;
	}

	unsigned long numbytes = fwrite(buf, 1, size, image);
	if (numbytes != size) {
		msg_gerr("Error: file %s could not be written completely.\n", filename);
		ret = 1;
		goto out;
	}
	if (fflush(image)) {
		msg_gerr("Error: flushing file \"%s\" failed: %s\n", filename, strerror(errno));
		ret = 1;
	}
	// Try to fsync() only regular files and if that function is available at all (e.g. not on MinGW).
#if defined(_POSIX_FSYNC) && (_POSIX_FSYNC != -1)
	struct stat image_stat;
	if (fstat(fileno(image), &image_stat) != 0) {
		msg_gerr("Error: getting metadata of file \"%s\" failed: %s\n", filename, strerror(errno));
		ret = 1;
		goto out;
	}
	if (S_ISREG(image_stat.st_mode)) {
		if (fsync(fileno(image))) {
			msg_gerr("Error: fsyncing file \"%s\" failed: %s\n", filename, strerror(errno));
			ret = 1;
		}
	}
#endif
out:
	if (fclose(image)) {
		msg_gerr("Error: closing file \"%s\" failed: %s\n", filename, strerror(errno));
		ret = 1;
	}
	return ret;
#endif
}
