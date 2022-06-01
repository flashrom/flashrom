/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2022 The Chromium OS Authors
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "libflashrom.h"

void log_rust(enum flashrom_log_level level, const char *format);

// LOG_LEVEL = 0 (ERROR) means no messages are printed.
enum flashrom_log_level current_level = 0;

static int log_c(enum flashrom_log_level level, const char *format, va_list ap)
{
	static char *buf = NULL;
	static int len = 0;
	if (level >= current_level) {
		return 0;
	}

	va_list ap2;
	va_copy(ap2, ap);
	// when buf is NULL, len is zero and this will not write to buf
	int req_len = vsnprintf(buf, len, format, ap2);
	va_end(ap2);

	if (req_len > len) {
		char *new_buf = realloc(buf, req_len + 1);
		if (!new_buf) {
			return 0;
		}
		buf = new_buf;
		len = req_len + 1;
		req_len = vsnprintf(buf, len, format, ap);
	}

	if (req_len > 0) {
		log_rust(level, buf);
	}
	return req_len;
}

void set_log_callback()
{
	flashrom_set_log_callback(log_c);
}
