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

/* Adapted from the Intel FW hub stuff for 82802ax parts. */

#include "flash.h"

// I need that Berkeley bit-map printer
void print_sst_fwhub_status(uint8_t status)
{
	printf("%s", status & 0x80 ? "Ready:" : "Busy:");
	printf("%s", status & 0x40 ? "BE SUSPEND:" : "BE RUN/FINISH:");
	printf("%s", status & 0x20 ? "BE ERROR:" : "BE OK:");
	printf("%s", status & 0x10 ? "PROG ERR:" : "PROG OK:");
	printf("%s", status & 0x8 ? "VP ERR:" : "VPP OK:");
	printf("%s", status & 0x4 ? "PROG SUSPEND:" : "PROG RUN/FINISH:");
	printf("%s", status & 0x2 ? "WP|TBL#|WP#,ABORT:" : "UNLOCK:");
}

int check_sst_fwhub_block_lock(struct flashchip *flash, int offset)
{
	chipaddr registers = flash->virtual_registers;
	uint8_t blockstatus;

	blockstatus = chip_readb(registers + offset + 2);
	printf_debug("Lock status for 0x%06x (size 0x%06x) is %02x, ",
		     offset, flash->page_size, blockstatus);
	switch (blockstatus & 0x3) {
	case 0x0:
		printf_debug("full access\n");
		break;
	case 0x1:
		printf_debug("write locked\n");
		break;
	case 0x2:
		printf_debug("locked open\n");
		break;
	case 0x3:
		printf_debug("write locked down\n");
		break;
	}
	/* Return content of the write_locked bit */
	return blockstatus & 0x1;
}

int clear_sst_fwhub_block_lock(struct flashchip *flash, int offset)
{
	chipaddr registers = flash->virtual_registers;
	uint8_t blockstatus;

	blockstatus = check_sst_fwhub_block_lock(flash, offset);

	if (blockstatus) {
		printf_debug("Trying to clear lock for 0x%06x... ", offset)
		chip_writeb(0, registers + offset + 2);

		blockstatus = check_sst_fwhub_block_lock(flash, offset);
		printf_debug("%s\n", (blockstatus) ? "failed" : "OK");
	}

	return blockstatus;
}

/* probe_jedec works fine for probing */
int probe_sst_fwhub(struct flashchip *flash)
{
	int i;

	if (probe_jedec(flash) == 0)
		return 0;

	map_flash_registers(flash);

	for (i = 0; i < flash->total_size * 1024; i += flash->page_size)
		check_sst_fwhub_block_lock(flash, i);

	return 1;
}

int erase_sst_fwhub_block(struct flashchip *flash, unsigned int offset, unsigned int page_size)
{
	uint8_t blockstatus = clear_sst_fwhub_block_lock(flash, offset);

	if (blockstatus) {
		printf("Block lock clearing failed, not erasing block "
			"at 0x%06x\n", offset);
		return 1;
	}

	if (erase_block_jedec(flash, offset, page_size)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	toggle_ready_jedec(flash->virtual_memory);

	return 0;
}

int erase_sst_fwhub(struct flashchip *flash)
{
	int i;
	unsigned int total_size = flash->total_size * 1024;

	for (i = 0; i < total_size; i += flash->page_size) {
		if (erase_sst_fwhub_block(flash, i, flash->page_size)) {
			fprintf(stderr, "ERASE FAILED!\n");
			return -1;
		}
	}

	return 0;
}

int write_sst_fwhub(struct flashchip *flash, uint8_t *buf)
{
	int i;
	int total_size = flash->total_size * 1024;
	int page_size = flash->page_size;
	chipaddr bios = flash->virtual_memory;
	uint8_t blockstatus;

	// FIXME: We want block wide erase instead of ironing the whole chip
	if (erase_sst_fwhub(flash)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}

	printf("Programming page: ");
	for (i = 0; i < total_size / page_size; i++) {
		printf("%04d at address: 0x%08x", i, i * page_size);
		blockstatus = clear_sst_fwhub_block_lock(flash, i * page_size);
		if (blockstatus) {
			printf(" is locked down permanently, aborting\n");
			return 1;
		}
		write_sector_jedec(bios, buf + i * page_size,
				   bios + i * page_size, page_size);
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	}
	printf("\n");

	return 0;
}
