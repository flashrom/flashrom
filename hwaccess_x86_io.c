/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009,2010 Carl-Daniel Hailfinger
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

#include <errno.h>
#include <string.h>
#if !defined (__DJGPP__) && !defined(__LIBPAYLOAD__)
/* No file access needed/possible to get hardware access permissions. */
#include <unistd.h>
#include <fcntl.h>
#endif

#include "hwaccess_x86_io.h"
#include "flash.h"

#if USE_IOPERM
#include <sys/io.h>
#endif

#if USE_DEV_IO
int io_fd;
#endif

#if !(defined(__DJGPP__) || defined(__LIBPAYLOAD__))
static int release_io_perms(void *p)
{
#if defined (__sun)
	sysi86(SI86V86, V86SC_IOPL, 0);
#elif USE_DEV_IO
	close(io_fd);
#elif USE_IOPERM
	ioperm(0, 65536, 0);
#elif USE_IOPL
	iopl(0);
#endif
	return 0;
}

/* Get I/O permissions with automatic permission release on shutdown. */
int rget_io_perms(void)
{
	#if defined (__sun)
	if (sysi86(SI86V86, V86SC_IOPL, PS_IOPL) != 0) {
#elif USE_DEV_IO
	if ((io_fd = open("/dev/io", O_RDWR)) < 0) {
#elif USE_IOPERM
	if (ioperm(0, 65536, 1) != 0) {
#elif USE_IOPL
	if (iopl(3) != 0) {
#endif
		msg_perr("ERROR: Could not get I/O privileges (%s).\n", strerror(errno));
		msg_perr("You need to be root.\n");
		msg_perr("On OpenBSD set securelevel=-1 in /etc/rc.securelevel and\n"
			 "reboot, or reboot into single user mode.\n");
		msg_perr("On NetBSD reboot into single user mode or make sure\n"
			 "that your kernel configuration has the option INSECURE enabled.\n");
		return 1;
	} else {
		register_shutdown(release_io_perms, NULL);
	}
	return 0;
}

#else

/* DJGPP and libpayload environments have full PCI port I/O permissions by default. */
int rget_io_perms(void)
{
	return 0;
}
#endif
