/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2017 Urja Rannikko <urjaman@gmail.com>
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

#include "platform.h"
#include "custom_baud.h"

#if IS_LINUX
#include <sys/ioctl.h>
#include <fcntl.h>
#include <asm-generic/termbits.h>
#include <asm-generic/ioctls.h>

/*
 * This include hell above is why this is in a separate source file. See eg.
 * https://www.downtowndougbrown.com/2013/11/linux-custom-serial-baud-rates/
 * https://stackoverflow.com/questions/12646324/how-to-set-a-custom-baud-rate-on-linux
 * https://github.com/jbkim/Linux-custom-baud-rate
 * for more info.
 */

int set_custom_baudrate(int fd, unsigned int baud)
{
	struct termios2 tio;
	if (ioctl(fd, TCGETS2, &tio)) {
		return -1;
	}
	tio.c_cflag &= ~CBAUD;
	tio.c_cflag |= BOTHER;
	tio.c_ispeed = baud;
	tio.c_ospeed = baud;
	return ioctl(fd, TCSETS2, &tio);
}

int use_custom_baud(unsigned int baud, const struct baudentry *baudtable)
{
	int i;
	for (i = 0; baudtable[i].baud; i++) {
		if (baudtable[i].baud == baud)
			return 0;

		if (baudtable[i].baud > baud)
			return 1;
	}
	return 1;
}

#else
#include <errno.h>

/* Stub, should not get called. */
int set_custom_baudrate(int fd, unsigned int baud)
{
	errno = ENOSYS; /* Hoping "Function not supported" will make you look here. */
	return -1;
}

int use_custom_baud(unsigned int baud, const struct baudentry *baudtable)
{
	return 0;
}
#endif
