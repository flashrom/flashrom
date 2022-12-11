/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2022 Peter Stuge <peter@stuge.se>
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

#include <termios.h>
#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>
#include <errno.h>

#include "custom_baud.h"

int use_custom_baud(unsigned int baud, const struct baudentry *baudtable)
{
	int i;

	if (baud > 230400)
		return 1;

	for (i = 0; baudtable[i].baud; i++) {
		if (baudtable[i].baud == baud)
			return 0;

		if (baudtable[i].baud > baud)
			return 1;
	}

	return 1;
}

int set_custom_baudrate(int fd, unsigned int baud, const enum custom_baud_stage stage, void *tio_wanted)
{
	struct termios *wanted;
	speed_t speed;

	switch (stage) {
	case BEFORE_FLAGS:
		break;

	case WITH_FLAGS:
		wanted = tio_wanted;
		return cfsetspeed(wanted, B19200);

	case AFTER_FLAGS:
		speed = baud;
		return ioctl(fd, IOSSIOSPEED, &speed);
	}

	return 0;
}
