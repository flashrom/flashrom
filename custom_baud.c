/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2017 Urja Rannikko <urjaman@gmail.com>
 */

#include <errno.h>

#include "custom_baud.h"

/* Stub, should not get called. */
int set_custom_baudrate(int fd, unsigned int baud, const enum custom_baud_stage stage, void *tio_wanted)
{
	errno = ENOSYS; /* Hoping "Function not supported" will make you look here. */
	return -1;
}

int use_custom_baud(unsigned int baud, const struct baudentry *baudtable)
{
	return 0;
}
