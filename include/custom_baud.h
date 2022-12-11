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

#ifndef __CUSTOM_BAUD_H__
#define __CUSTOM_BAUD_H__ 1

struct baudentry {
	int flag;
	unsigned int baud;
};

enum custom_baud_stage {
	BEFORE_FLAGS = 0,
	WITH_FLAGS,
	AFTER_FLAGS
};

int set_custom_baudrate(int fd, unsigned int baud, const enum custom_baud_stage stage, void *tio_wanted);

/* Returns 1 if non-exact rate would be used, and setting a custom rate is supported.
   The baudtable must be in ascending order and terminated with a 0-baud entry. */
int use_custom_baud(unsigned int baud, const struct baudentry *baudtable);

#endif
