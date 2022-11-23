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
 */

/*
 * Datasheet:
 *  - Name: Intel 82802AB/82802AC Firmware Hub (FWH)
 *  - URL: http://www.intel.com/design/chipsets/datashts/290658.htm
 *  - PDF: http://download.intel.com/design/chipsets/datashts/29065804.pdf
 *  - Order number: 290658-004
 */

#include <stdbool.h>
#include "flash.h"
#include "chipdrivers.h"

void print_status_82802ab(uint8_t status)
{
	msg_cdbg("%s", status & 0x80 ? "Ready:" : "Busy:");
	msg_cdbg("%s", status & 0x40 ? "BE SUSPEND:" : "BE RUN/FINISH:");
	msg_cdbg("%s", status & 0x20 ? "BE ERROR:" : "BE OK:");
	msg_cdbg("%s", status & 0x10 ? "PROG ERR:" : "PROG OK:");
	msg_cdbg("%s", status & 0x8 ? "VP ERR:" : "VPP OK:");
	msg_cdbg("%s", status & 0x4 ? "PROG SUSPEND:" : "PROG RUN/FINISH:");
	msg_cdbg("%s", status & 0x2 ? "WP|TBL#|WP#,ABORT:" : "UNLOCK:");
}

int probe_82802ab(struct flashctx *flash)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t id1, id2, flashcontent1, flashcontent2;
	int shifted = (flash->chip->feature_bits & FEATURE_ADDR_SHIFTED) ? 1 : 0;

	/* Reset to get a clean state */
	chip_writeb(flash, 0xFF, bios);
	programmer_delay(flash, 10);

	/* Enter ID mode */
	chip_writeb(flash, 0x90, bios);
	programmer_delay(flash, 10);

	id1 = chip_readb(flash, bios + (0x00 << shifted));
	id2 = chip_readb(flash, bios + (0x01 << shifted));

	/* Leave ID mode */
	chip_writeb(flash, 0xFF, bios);

	programmer_delay(flash, 10);

	msg_cdbg("%s: id1 0x%02x, id2 0x%02x", __func__, id1, id2);

	if (!oddparity(id1))
		msg_cdbg(", id1 parity violation");

	/*
	 * Read the product ID location again. We should now see normal
	 * flash contents.
	 */
	flashcontent1 = chip_readb(flash, bios + (0x00 << shifted));
	flashcontent2 = chip_readb(flash, bios + (0x01 << shifted));

	if (id1 == flashcontent1)
		msg_cdbg(", id1 is normal flash content");
	if (id2 == flashcontent2)
		msg_cdbg(", id2 is normal flash content");

	msg_cdbg("\n");
	if (id1 != flash->chip->manufacture_id || id2 != flash->chip->model_id)
		return 0;

	return 1;
}

/* FIXME: needs timeout */
uint8_t wait_82802ab(struct flashctx *flash)
{
	uint8_t status;
	chipaddr bios = flash->virtual_memory;

	chip_writeb(flash, 0x70, bios);

	while ((chip_readb(flash, bios) & 0x80) == 0)	// it's busy
		;

	status = chip_readb(flash, bios);

	/* Reset to get a clean state */
	chip_writeb(flash, 0xFF, bios);

	return status;
}

int erase_block_82802ab(struct flashctx *flash, unsigned int page,
			unsigned int pagesize)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t status;

	// clear status register
	chip_writeb(flash, 0x50, bios + page);

	// now start it
	chip_writeb(flash, 0x20, bios + page);
	chip_writeb(flash, 0xd0, bios + page);
	programmer_delay(flash, 10);

	// now let's see what the register is
	status = wait_82802ab(flash);
	print_status_82802ab(status);

	/* FIXME: Check the status register for errors. */
	return 0;
}

/* chunksize is 1 */
int write_82802ab(struct flashctx *flash, const uint8_t *src, unsigned int start, unsigned int len)
{
	unsigned int i;
	chipaddr dst = flash->virtual_memory + start;

	for (i = 0; i < len; i++) {
		/* transfer data from source to destination */
		chip_writeb(flash, 0x40, dst);
		chip_writeb(flash, *src++, dst++);
		wait_82802ab(flash);
		update_progress(flash, FLASHROM_PROGRESS_WRITE, i + 1, len);
	}

	/* FIXME: Ignore errors for now. */
	return 0;
}

static int unlock_28f004s5(struct flashctx *flash)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t mcfg, bcfg;
	bool need_unlock = false, can_unlock = false;
	unsigned int i;

	/* Clear status register */
	chip_writeb(flash, 0x50, bios);

	/* Read identifier codes */
	chip_writeb(flash, 0x90, bios);

	/* Read master lock-bit */
	mcfg = chip_readb(flash, bios + 0x3);
	msg_cdbg("master lock is ");
	if (mcfg) {
		msg_cdbg("locked!\n");
	} else {
		msg_cdbg("unlocked!\n");
		can_unlock = true;
	}

	/* Read block lock-bits */
	for (i = 0; i < flash->chip->total_size * 1024; i+= (64 * 1024)) {
		bcfg = chip_readb(flash, bios + i + 2); // read block lock config
		msg_cdbg("block lock at %06x is %slocked!\n", i, bcfg ? "" : "un");
		if (bcfg) {
			need_unlock = true;
		}
	}

	/* Reset chip */
	chip_writeb(flash, 0xFF, bios);

	/* Unlock: clear block lock-bits, if needed */
	if (can_unlock && need_unlock) {
		msg_cdbg("Unlock: ");
		chip_writeb(flash, 0x60, bios);
		chip_writeb(flash, 0xD0, bios);
		chip_writeb(flash, 0xFF, bios);
		msg_cdbg("Done!\n");
	}

	/* Error: master locked or a block is locked */
	if (!can_unlock && need_unlock) {
		msg_cerr("At least one block is locked and lockdown is active!\n");
		return -1;
	}

	return 0;
}

static int unlock_lh28f008bjt(struct flashctx *flash)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t mcfg, bcfg;
	bool need_unlock = false, can_unlock = false;
	unsigned int i;

	/* Wait if chip is busy */
	wait_82802ab(flash);

	/* Read identifier codes */
	chip_writeb(flash, 0x90, bios);

	/* Read master lock-bit */
	mcfg = chip_readb(flash, bios + 0x3);
	msg_cdbg("master lock is ");
	if (mcfg) {
		msg_cdbg("locked!\n");
	} else {
		msg_cdbg("unlocked!\n");
		can_unlock = true;
	}

	/* Read block lock-bits, 8 * 8 KB + 15 * 64 KB */
	for (i = 0; i < flash->chip->total_size * 1024;
	     i += (i >= (64 * 1024) ? 64 * 1024 : 8 * 1024)) {
		bcfg = chip_readb(flash, bios + i + 2); /* read block lock config */
		msg_cdbg("block lock at %06x is %slocked!\n", i,
			 bcfg ? "" : "un");
		if (bcfg)
			need_unlock = true;
	}

	/* Reset chip */
	chip_writeb(flash, 0xFF, bios);

	/* Unlock: clear block lock-bits, if needed */
	if (can_unlock && need_unlock) {
		msg_cdbg("Unlock: ");
		chip_writeb(flash, 0x60, bios);
		chip_writeb(flash, 0xD0, bios);
		chip_writeb(flash, 0xFF, bios);
		wait_82802ab(flash);
		msg_cdbg("Done!\n");
	}

	/* Error: master locked or a block is locked */
	if (!can_unlock && need_unlock) {
		msg_cerr("At least one block is locked and lockdown is active!\n");
		return -1;
	}

	return 0;
}

blockprotect_func_t *lookup_82802ab_blockprotect_func_ptr(const struct flashchip *const chip)
{
	switch (chip->unlock) {
		case UNLOCK_28F004S5: return unlock_28f004s5;
		case UNLOCK_LH28F008BJT: return unlock_lh28f008bjt;
		default: return NULL; /* fallthough */
	};
}
