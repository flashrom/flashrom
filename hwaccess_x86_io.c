/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009,2010 Carl-Daniel Hailfinger
 * Copyright (C) 2022 secunet Security Networks AG
 * (Written by Thomas Heijligen <thomas.heijligen@secunet.com)
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

/*
 * This file implements x86 I/O port permission handling.
 *
 * For this purpose the platform specific code is encapsuled in two functions
 * and compiled only on the respective platform. `platform_get_io_perms()` is
 * called to get the permissions and `platform_release_io_perms()` is called for
 * releasing those.
 */

#include <errno.h>
#include <string.h>

#include "flash.h"
#include "hwaccess_x86_io.h"

#if defined(__DJGPP__) || defined(__LIBPAYLOAD) /* DJGPP, libpayload */
static int platform_get_io_perms(void)
{
	return 0;
}

static int platform_release_io_perms(void *p)
{
	return 0;
}
#endif

#if defined(__sun) /* SunOS */
#include <sys/sysi86.h>

static int platform_get_io_perms(void)
{
	return sysi86(SI86V86, V86SC_IOPL, PS_IOPL);
}

static int platform_release_io_perms(void *p)
{
	sysi86(SI86V86, V86SC_IOPL, 0);
	return 0;
}
#endif

#if USE_DEV_IO /* FreeBSD, DragonFlyBSD */
#include <fcntl.h>
#include <unistd.h>

int io_fd;

static int platform_get_io_perms(void)
{
	io_fd = open("/dev/io", O_RDWR);
	return (io_fd >= 0 ? 0 : -1);
}

static int platform_release_io_perms(void *p)
{
	close(io_fd);
	return 0;
}
#endif

#if USE_IOPERM /* GNU Hurd */
#include <sys/io.h>

static int platform_get_io_perms(void)
{
	return ioperm(0, 65536, 1);
}

static int platform_release_io_perms(void *p)
{
	ioperm(0, 65536, 0);
	return 0;
}
#endif

#if USE_IOPL /* Linux, Darwin, NetBSD, OpenBSD */
static int platform_get_io_perms(void)
{
	return iopl(3);
}

static int platform_release_io_perms(void *p)
{
	iopl(0);
	return 0;
}
#endif

/**
 * @brief Get access to the x86 I/O ports
 *
 * This function abstracts the platform dependent function to get access to the
 * x86 I/O ports and musst be called before using IN[B/W/L] or OUT[B/W/L].
 * It registers a shutdown function to release those privileges.
 *
 * @return 0 on success, platform specific number on failure
 */
int rget_io_perms(void)
{
	if (platform_get_io_perms() == 0) {
		register_shutdown(platform_release_io_perms, NULL);
		return 0;
	}
	msg_perr("ERROR: Could not get I/O privileges (%s).\n", strerror(errno));
	msg_perr("Make sure you are root. If you are root, your kernel may still\n"
		 "prevent access based on security policies.\n");
	msg_perr("On OpenBSD set securelevel=-1 in /etc/rc.securelevel and\n"
		 "reboot, or reboot into single user mode.\n");
	msg_perr("On NetBSD reboot into single user mode or make sure\n"
		 "that your kernel configuration has the option INSECURE enabled.\n");
	return 1;
}
