/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Sean Nelson <audiohacked@gmail.com>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdio.h>
#include <stdarg.h>
#include "flash.h"

/* Please note that level is the verbosity, not the importance of the message. */
int print(enum msglevel level, const char *fmt, ...)
{
	va_list ap;
	int ret = 0;
	FILE *output_type = stdout;

	if (level == MSG_ERROR)
		output_type = stderr;

	if (level <= verbose) {
		va_start(ap, fmt);
		ret = vfprintf(output_type, fmt, ap);
		va_end(ap);
		/* msg_*spew usually happens inside chip accessors in possibly
		 * time-critical operations. Don't slow them down by flushing.
		 */
		if (level != MSG_SPEW)
			fflush(output_type);
	}
	return ret;
}
