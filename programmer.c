/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009,2010,2011 Carl-Daniel Hailfinger
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

#include "flash.h"
#include "programmer.h"

/* The limit of 4 is totally arbitrary. */
#define MASTERS_MAX 4
struct registered_master registered_masters[MASTERS_MAX];
int registered_master_count = 0;

/* This function copies the struct registered_master parameter. */
int register_master(const struct registered_master *mst)
{
	if (registered_master_count >= MASTERS_MAX) {
		msg_perr("Tried to register more than %i master "
			 "interfaces.\n", MASTERS_MAX);
		return ERROR_FLASHROM_LIMIT;
	}
	registered_masters[registered_master_count] = *mst;
	registered_master_count++;

	return 0;
}

enum chipbustype get_buses_supported(void)
{
	int i;
	enum chipbustype ret = BUS_NONE;

	for (i = 0; i < registered_master_count; i++)
		ret |= registered_masters[i].buses_supported;

	return ret;
}
