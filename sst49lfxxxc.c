/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000 Silicon Integrated System Corporation
 * SPDX-FileCopyrightText: 2005-2007 coresystems GmbH
 * SPDX-FileCopyrightText: 2009 Sean Nelson <audiohacked@gmail.com>
 */

#include "flash.h"
#include "parallel.h"
#include "chipdrivers.h"

int erase_sector_49lfxxxc(struct flashctx *flash, unsigned int address,
			  unsigned int sector_size)
{
	chipaddr bios = flash->virtual_memory;

	chip_writeb(flash, 0x30, bios);
	chip_writeb(flash, 0xD0, bios + address);

	return wait_82802ab(flash);
}
