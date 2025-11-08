/*
 * This file is part of the flashrom project.
 *
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
	uint8_t status;
	chipaddr bios = flash->virtual_memory;

	chip_writeb(flash, 0x30, bios);
	chip_writeb(flash, 0xD0, bios + address);

	status = wait_82802ab(flash);
	print_status_82802ab(status);

	/* FIXME: Check the status register for errors. */
	return 0;
}
