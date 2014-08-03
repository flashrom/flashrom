/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2005-2007 coresystems GmbH
 * Copyright (C) 2009 Sean Nelson <audiohacked@gmail.com>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "flash.h"
#include "chipdrivers.h"

int erase_sector_49lfxxxc(struct flashctx *flash, unsigned int address,
			  unsigned int sector_size)
{
	uint8_t status;
	chipaddr bios = flash->virtual_memory;

	chip_writeb(flash, 0x30, bios);
	chip_writeb(flash, 0xD0, bios + address);

	status = wait_82802ab(flash);
	print_status_82802ab(status);

	/* FIXME: Check the status register for errors. */
	return 0;
}
