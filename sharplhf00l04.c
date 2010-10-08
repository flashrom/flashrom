/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
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

/* FIXME: The datasheet is unclear whether we should use toggle_ready_jedec
 * or wait_82802ab.
 */

int erase_lhf00l04_block(struct flashchip *flash, unsigned int blockaddr, unsigned int blocklen)
{
	chipaddr bios = flash->virtual_memory + blockaddr;
	chipaddr wrprotect = flash->virtual_registers + blockaddr + 2;
	uint8_t status;

	// clear status register
	chip_writeb(0x50, bios);
	status = wait_82802ab(flash->virtual_memory);
	print_status_82802ab(status);
	// clear write protect
	msg_cspew("write protect is at 0x%lx\n", (wrprotect));
	msg_cspew("write protect is 0x%x\n", chip_readb(wrprotect));
	chip_writeb(0, wrprotect);
	msg_cspew("write protect is 0x%x\n", chip_readb(wrprotect));

	// now start it
	chip_writeb(0x20, bios);
	chip_writeb(0xd0, bios);
	programmer_delay(10);
	// now let's see what the register is
	status = wait_82802ab(flash->virtual_memory);
	print_status_82802ab(status);

	if (check_erased_range(flash, blockaddr, blocklen)) {
		msg_cerr("ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

int write_lhf00l04(struct flashchip *flash, uint8_t *buf)
{
	int i;
	int total_size = flash->total_size * 1024;
	int page_size = flash->page_size;
	chipaddr bios = flash->virtual_memory;

	for (i = 0; i < total_size / page_size; i++) {
		write_page_82802ab(bios, buf + i * page_size,
				    bios + i * page_size, page_size);
	}

	return 0;
}
