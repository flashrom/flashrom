/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000 Silicon Integrated System Corporation
 */

/*
 * Datasheet:
 *  - Name: Intel 82802AB/82802AC Firmware Hub (FWH)
 *  - URL: http://www.intel.com/design/chipsets/datashts/290658.htm
 *  - PDF: http://download.intel.com/design/chipsets/datashts/29065804.pdf
 *  - Order number: 290658-004
 */

#include "flash.h"
#include "parallel.h"
#include "chipdrivers.h"
#include "log.h"
#include <stdbool.h>

#define STATUS_82802AB_READY		(1U << 7)
/* SR.0 is reserved; SR.1 and SR.3--SR.5 report operation failures. */
#define STATUS_82802AB_ERROR_MASK	((1U << 5) | (1U << 4) | (1U << 3) | (1U << 1))
#define WAIT_82802AB_TIMEOUT_US		(1000U * 1000U)

static void print_status_82802ab(uint8_t status)
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
	const int shifted_bit = (flash->chip->feature_bits & FEATURE_ADDR_SHIFTED) ? 1 : 0;

	/* Reset to get a clean state */
	chip_writeb(flash, 0xFF, bios);
	programmer_delay(flash, 10);

	/* Enter ID mode */
	chip_writeb(flash, 0x90, bios);
	programmer_delay(flash, 10);

	id1 = chip_readb(flash, bios + (0x00 << shifted_bit));
	id2 = chip_readb(flash, bios + (0x01 << shifted_bit));

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
	flashcontent1 = chip_readb(flash, bios + (0x00 << shifted_bit));
	flashcontent2 = chip_readb(flash, bios + (0x01 << shifted_bit));

	if (id1 == flashcontent1)
		msg_cdbg(", id1 is normal flash content");
	if (id2 == flashcontent2)
		msg_cdbg(", id2 is normal flash content");

	msg_cdbg("\n");
	if (id1 != flash->chip->manufacture_id || id2 != flash->chip->model_id)
		return 0;

	return 1;
}

int wait_82802ab(struct flashctx *flash)
{
	uint8_t status;
	chipaddr bios = flash->virtual_memory;
	unsigned int timeout = WAIT_82802AB_TIMEOUT_US;

	chip_writeb(flash, 0x70, bios);

	do {
		status = chip_readb(flash, bios);
		if (status & STATUS_82802AB_READY)
			break;
		programmer_delay(flash, 1);
	} while (--timeout);

	/* Reset to get a clean state */
	chip_writeb(flash, 0xFF, bios);

	if (!(status & STATUS_82802AB_READY)) {
		msg_cerr("82802AB operation timed out\n");
		return 1;
	}

	print_status_82802ab(status);
	if (status & STATUS_82802AB_ERROR_MASK) {
		msg_cerr("82802AB operation failed with status 0x%02x\n", status);
		return 1;
	}

	return 0;
}

int erase_block_82802ab(struct flashctx *flash, unsigned int page,
			unsigned int pagesize)
{
	chipaddr bios = flash->virtual_memory;

	// clear status register
	chip_writeb(flash, 0x50, bios + page);

	// now start it
	chip_writeb(flash, 0x20, bios + page);
	chip_writeb(flash, 0xd0, bios + page);
	programmer_delay(flash, 10);

	if (wait_82802ab(flash)) {
		msg_cerr("82802AB block erase failed\n");
		return -1;
	}

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
		chip_writeb(flash, *src, dst);
		if (wait_82802ab(flash)) {
			msg_cerr("82802AB write failed at address 0x%06x\n", start + i);
			return -1;
		}
		dst++;
		src++;
		update_progress(flash, FLASHROM_PROGRESS_WRITE, 1);
	}

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
	for (i = 0; i < flash->chip->total_size * KiB; i+= (64 * KiB)) {
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
	if (wait_82802ab(flash))
		return -1;

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
	for (i = 0; i < flash->chip->total_size * KiB;
	     i += (i >= (64 * KiB) ? 64 * KiB : 8 * KiB)) {
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
		if (wait_82802ab(flash))
			return -1;
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
