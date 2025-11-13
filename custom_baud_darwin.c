/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2022 Peter Stuge <peter@stuge.se>
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
