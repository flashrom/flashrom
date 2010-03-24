/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2008 Peter Stuge <peter@stuge.se>
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

int printlock_w39v040c(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t lock;

	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0x90, bios + 0x5555);
	programmer_delay(10);

	lock = chip_readb(bios + 0xfff2);

	chip_writeb(0xAA, bios + 0x5555);
	programmer_delay(10);
	chip_writeb(0x55, bios + 0x2AAA);
	programmer_delay(10);
	chip_writeb(0xF0, bios + 0x5555);
	programmer_delay(40);

	msg_cdbg("%s: Boot block #TBL is %slocked, rest of chip #WP is %slocked.\n",
		__func__, lock & 0x4 ? "" : "un", lock & 0x8 ? "" : "un");
	return 0;
}
