/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2012 Rudolf Marek <r.marek@assembler.cz>
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

/*
 * WARNING!
 * This chip uses the standard JEDEC addresses in 16-bit mode as word
 * addresses. In byte mode, 0xAAA has to be used instead of 0x555 and
 * 0x555 instead of 0x2AA. Do *not* blindly replace with standard JEDEC
 * functions.
 */

/* chunksize is 1 */
int write_en29lv640b(struct flashctx *flash, const uint8_t *src, unsigned int start, unsigned int len)
{
	int i;
	chipaddr bios = flash->virtual_memory;
	chipaddr dst = flash->virtual_memory + start;

	for (i = 0; i < len; i += 2) {
		chip_writeb(flash, 0xAA, bios + 0xAAA);
		chip_writeb(flash, 0x55, bios + 0x555);
		chip_writeb(flash, 0xA0, bios + 0xAAA);

		/* Transfer data from source to destination. */
		chip_writew(flash, (*src) | ((*(src + 1)) << 8 ), dst);
		toggle_ready_jedec(flash, dst);
#if 0
		/* We only want to print something in the error case. */
		msg_cerr("Value in the flash at address 0x%lx = %#x, want %#x\n",
			 (dst - bios), chip_readb(flash, dst), *src);
#endif
		dst += 2;
		src += 2;
	}

	/* FIXME: Ignore errors for now. */
	return 0;
}

int probe_en29lv640b(struct flashctx *flash)
{
	chipaddr bios = flash->virtual_memory;
	uint16_t id1, id2;

	chip_writeb(flash, 0xAA, bios + 0xAAA);
	chip_writeb(flash, 0x55, bios + 0x555);
	chip_writeb(flash, 0x90, bios + 0xAAA);

	programmer_delay(10);

	id1 = chip_readb(flash, bios + 0x200);
	id1 |= (chip_readb(flash, bios) << 8);

	id2 = chip_readb(flash, bios + 0x02);

	chip_writeb(flash, 0xF0, bios + 0xAAA);

	programmer_delay(10);

	msg_cdbg("%s: id1 0x%04x, id2 0x%04x\n", __func__, id1, id2);

	if (id1 == flash->chip->manufacture_id && id2 == flash->chip->model_id)
		return 1;

	return 0;
}
