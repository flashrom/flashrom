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

/*
 * Datasheet:
 *  - Name: Intel 82802AB/82802AC Firmware Hub (FWH)
 *  - URL: http://www.intel.com/design/chipsets/datashts/290658.htm
 *  - PDF: http://download.intel.com/design/chipsets/datashts/29065804.pdf
 *  - Order number: 290658-004
 */

#include <string.h>
#include <stdlib.h>
#include "flash.h"
#include "chipdrivers.h"

// I need that Berkeley bit-map printer
void print_status_82802ab(uint8_t status)
{
	printf_debug("%s", status & 0x80 ? "Ready:" : "Busy:");
	printf_debug("%s", status & 0x40 ? "BE SUSPEND:" : "BE RUN/FINISH:");
	printf_debug("%s", status & 0x20 ? "BE ERROR:" : "BE OK:");
	printf_debug("%s", status & 0x10 ? "PROG ERR:" : "PROG OK:");
	printf_debug("%s", status & 0x8 ? "VP ERR:" : "VPP OK:");
	printf_debug("%s", status & 0x4 ? "PROG SUSPEND:" : "PROG RUN/FINISH:");
	printf_debug("%s", status & 0x2 ? "WP|TBL#|WP#,ABORT:" : "UNLOCK:");
}

int probe_82802ab(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t id1, id2;

	/* Reset to get a clean state */
	chip_writeb(0xFF, bios);
	programmer_delay(10);

	/* Enter ID mode */
	chip_writeb(0x90, bios);
	programmer_delay(10);

	id1 = chip_readb(bios);
	id2 = chip_readb(bios + 0x01);

	/* Leave ID mode */
	chip_writeb(0xFF, bios);

	programmer_delay(10);

	printf_debug("%s: id1 0x%02x, id2 0x%02x\n", __func__, id1, id2);

	if (id1 != flash->manufacture_id || id2 != flash->model_id)
		return 0;

	if (flash->feature_bits & FEATURE_REGISTERMAP)
		map_flash_registers(flash);

	return 1;
}

uint8_t wait_82802ab(chipaddr bios)
{
	uint8_t status;

	chip_writeb(0x70, bios);
	if ((chip_readb(bios) & 0x80) == 0) {	// it's busy
		while ((chip_readb(bios) & 0x80) == 0) ;
	}

	status = chip_readb(bios);

	/* Reset to get a clean state */
	chip_writeb(0xFF, bios);

	return status;
}

int unlock_82802ab(struct flashchip *flash)
{
	int i;
	//chipaddr wrprotect = flash->virtual_registers + page + 2;

	for (i = 0; i < flash->total_size * 1024; i+= flash->page_size)
	{
		chip_writeb(0, flash->virtual_registers + i + 2);
	}

	return 0;
}

int erase_block_82802ab(struct flashchip *flash, unsigned int page, unsigned int pagesize)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t status;

	// clear status register
	chip_writeb(0x50, bios + page);

	// now start it
	chip_writeb(0x20, bios + page);
	chip_writeb(0xd0, bios + page);
	programmer_delay(10);

	// now let's see what the register is
	status = wait_82802ab(bios);
	print_status_82802ab(status);

	if (check_erased_range(flash, page, pagesize)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	printf("DONE BLOCK 0x%x\n", page);

	return 0;
}

int erase_82802ab(struct flashchip *flash)
{
	int i;
	unsigned int total_size = flash->total_size * 1024;

	printf("total_size is %d; flash->page_size is %d\n",
	       total_size, flash->page_size);
	for (i = 0; i < total_size; i += flash->page_size)
		if (erase_block_82802ab(flash, i, flash->page_size)) {
			fprintf(stderr, "ERASE FAILED!\n");
			return -1;
		}
	printf("DONE ERASE\n");

	return 0;
}

void write_page_82802ab(chipaddr bios, uint8_t *src,
			chipaddr dst, int page_size)
{
	int i;

	for (i = 0; i < page_size; i++) {
		/* transfer data from source to destination */
		chip_writeb(0x40, dst);
		chip_writeb(*src++, dst++);
		wait_82802ab(bios);
	}
}

int write_82802ab(struct flashchip *flash, uint8_t *buf)
{
	int i;
	int total_size = flash->total_size * 1024;
	int page_size = flash->page_size;
	chipaddr bios = flash->virtual_memory;
	uint8_t *tmpbuf = malloc(page_size);

	if (!tmpbuf) {
		printf("Could not allocate memory!\n");
		exit(1);
	}
	printf("Programming page: \n");
	for (i = 0; i < total_size / page_size; i++) {
		printf
		    ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		printf("%04d at address: 0x%08x", i, i * page_size);

		/* Auto Skip Blocks, which already contain the desired data
		 * Faster, because we only write, what has changed
		 * More secure, because blocks, which are excluded
		 * (with the exclude or layout feature)
		 * or not erased and rewritten; their data is retained also in
		 * sudden power off situations
		 */
		chip_readn(tmpbuf, bios + i * page_size, page_size);
		if (!memcmp((void *)(buf + i * page_size), tmpbuf, page_size)) {
			printf("SKIPPED\n");
			continue;
		}

		/* erase block by block and write block by block; this is the most secure way */
		if (erase_block_82802ab(flash, i * page_size, page_size)) {
			fprintf(stderr, "ERASE FAILED!\n");
			return -1;
		}
		write_page_82802ab(bios, buf + i * page_size,
				   bios + i * page_size, page_size);
	}
	printf("\n");
	free(tmpbuf);

	return 0;
}

int unlock_28f004s5(struct flashrom *flash)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t mcfg, bcfg, need_unlock = 0, can_unlock = 0;

	/* Clear status register */
	chip_writeb(0x50, bios);

	/* Read identifier codes */
	chip_writeb(0x90, bios);

	/* Read master lock-bit */
	mcfg = chip_readb(bios + 0x3);
	msg_cinfo("master lock is ");
	if (mcfg) {
		msg_cdbg("locked!\n");
	} else {
		msg_cdbg("unlocked!\n");
		can_unlock = 1;
	}
	
	/* Read block lock-bits */
	for (i = 0; i < flash->total_size * 1024; i+= (64 * 1024)) {
		bcfg = chip_readb(bios + i + 2); // read block lock config
		msg_cdbg("block lock at %06x is %slocked!\n", i, bcfg ? "" : "un");
		if (bcfg) {
			need_unlock = 1;
		}
	}

	/* Reset chip */
	chip_writeb(0xFF, bios);

	/* Unlock: clear block lock-bits, if needed */
	if (can_unlock && need_unlock) {
		chip_writeb(0x60, bios);
		chip_writeb(0xD0, bios);
		chip_writeb(0xFF, bios);
	}

	/* Error: master locked or a block is locked */
	if (!can_unlock && need_unlock) {
		msg_cerr("At least one block is locked and lockdown is active!\n");
		return -1;
	}

	return 0;
}
