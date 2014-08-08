/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Sean Nelson <audiohacked@gmail.com>
 * Copyright (C) 2011 Carl-Daniel Hailfinger
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
#include <string.h>
#include <errno.h>
#include "flash.h"

int verbose_screen = MSG_INFO;
int verbose_logfile = MSG_DEBUG2;

#ifndef STANDALONE
static FILE *logfile = NULL;

int close_logfile(void)
{
	if (!logfile)
		return 0;
	/* No need to call fflush() explicitly, fclose() already does that. */
	if (fclose(logfile)) {
		/* fclose returned an error. Stop writing to be safe. */
		logfile = NULL;
		msg_gerr("Closing the log file returned error %s\n", strerror(errno));
		return 1;
	}
	logfile = NULL;
	return 0;
}

int open_logfile(const char * const filename)
{
	if (!filename) {
		msg_gerr("No logfile name specified.\n");
		return 1;
	}
	if ((logfile = fopen(filename, "w")) == NULL) {
		msg_gerr("Error: opening log file \"%s\" failed: %s\n", filename, strerror(errno));
		return 1;
	}
	return 0;
}

void start_logging(void)
{
	enum msglevel oldverbose_screen = verbose_screen;

	/* Shut up the console. */
	verbose_screen = MSG_ERROR;
	print_version();
	verbose_screen = oldverbose_screen;
}
#endif /* !STANDALONE */

/* Please note that level is the verbosity, not the importance of the message. */
int print(enum msglevel level, const char *fmt, ...)
{
	va_list ap;
	int ret = 0;
	FILE *output_type = stdout;

	if (level < MSG_INFO)
		output_type = stderr;

	if (level <= verbose_screen) {
		va_start(ap, fmt);
		ret = vfprintf(output_type, fmt, ap);
		va_end(ap);
		/* msg_*spew often happens inside chip accessors in possibly
		 * time-critical operations. Don't slow them down by flushing. */
		if (level != MSG_SPEW)
			fflush(output_type);
	}
#ifndef STANDALONE
	if ((level <= verbose_logfile) && logfile) {
		va_start(ap, fmt);
		ret = vfprintf(logfile, fmt, ap);
		va_end(ap);
		if (level != MSG_SPEW)
			fflush(logfile);
	}
#endif /* !STANDALONE */
	return ret;
}
