/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Carl-Daniel Hailfinger
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

#include "programmer.h"

int superio_count = 0;
#define SUPERIO_MAX_COUNT 3

struct superio superios[SUPERIO_MAX_COUNT];

int register_superio(struct superio s)
{
	if (superio_count == SUPERIO_MAX_COUNT)
		return 1;
	superios[superio_count++] = s;
	return 0;
}

void probe_superio(void)
{
	probe_superio_winbond();
	/* ITE probe causes SMSC LPC47N217 to power off the serial UART.
	 * Always probe for SMSC first, and if a SMSC Super I/O is detected
	 * at a given I/O port, do _not_ probe that port with the ITE probe.
	 * This means SMSC probing must be done before ITE probing.
	 */
	//probe_superio_smsc();
	probe_superio_ite();
}
