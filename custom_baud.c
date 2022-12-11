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
