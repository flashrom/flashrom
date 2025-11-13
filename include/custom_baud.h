/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2017 Urja Rannikko <urjaman@gmail.com>
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
