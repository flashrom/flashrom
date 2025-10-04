/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2022 The Chromium OS Authors
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
