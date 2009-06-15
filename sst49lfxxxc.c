/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2005-2007 coresystems GmbH
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

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "flash.h"

#define SECTOR_ERASE		0x30
#define BLOCK_ERASE		0x20
#define ERASE			0xD0
#define AUTO_PGRM		0x10
#define RESET			0xFF
#define READ_ID			0x90
#define READ_STATUS		0x70
#define CLEAR_STATUS		0x50

#define STATUS_BPS		(1 << 1)
#define	STATUS_ESS		(1 << 6)
#define	STATUS_WSMS		(1 << 7)

static int write_lockbits_49lfxxxc(struct flashchip *flash, unsigned char bits)
{
	chipaddr registers = flash->virtual_registers;
	int i, left = flash->total_size * 1024;
	unsigned long address;

	printf_debug("\nbios=0x%08lx\n", registers);
	for (i = 0; left > 65536; i++, left -= 65536) {
		printf_debug("lockbits at address=0x%08lx is 0x%01x\n",
			     registers + (i * 65536) + 2,
			     chip_readb(registers + (i * 65536) + 2));
		chip_writeb(bits, registers + (i * 65536) + 2);
	}
	address = i * 65536;
	printf_debug("lockbits at address=0x%08lx is 0x%01x\n",
		     registers + address + 2,
		     chip_readb(registers + address + 2));
	chip_writeb(bits, registers + address + 2);
	address += 32768;
	printf_debug("lockbits at address=0x%08lx is 0x%01x\n",
		     registers + address + 2,
		     chip_readb(registers + address + 2));
	chip_writeb(bits, registers + address + 2);
	address += 8192;
	printf_debug("lockbits at address=0x%08lx is 0x%01x\n",
		     registers + address + 2,
		     chip_readb(registers + address + 2));
	chip_writeb(bits, registers + address + 2);
	address += 8192;
	printf_debug("lockbits at address=0x%08lx is 0x%01x\n",
		     registers + address + 2,
		     chip_readb(registers + address + 2));
	chip_writeb(bits, registers + address + 2);

	return 0;
}

static int erase_sector_49lfxxxc(struct flashchip *flash, unsigned long address, int sector_size)
{
	unsigned char status;
	chipaddr bios = flash->virtual_memory;

	chip_writeb(SECTOR_ERASE, bios);
	chip_writeb(ERASE, bios + address);

	do {
		status = chip_readb(bios);
		if (status & (STATUS_ESS | STATUS_BPS)) {
			printf("sector erase FAILED at address=0x%08lx status=0x%01x\n", bios + address, status);
			chip_writeb(CLEAR_STATUS, bios);
			return (-1);
		}
	} while (!(status & STATUS_WSMS));

	if (check_erased_range(flash, address, sector_size)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	return 0;
}

static int write_sector_49lfxxxc(chipaddr bios, uint8_t *src, chipaddr dst,
				 unsigned int page_size)
{
	int i;
	unsigned char status;

	chip_writeb(CLEAR_STATUS, bios);
	for (i = 0; i < page_size; i++) {
		/* transfer data from source to destination */
		if (*src == 0xFF) {
			dst++, src++;
			/* If the data is 0xFF, don't program it */
			continue;
		}
		/*issue AUTO PROGRAM command */
		chip_writeb(AUTO_PGRM, bios);
		chip_writeb(*src++, dst++);

		do {
			status = chip_readb(bios);
			if (status & (STATUS_ESS | STATUS_BPS)) {
				printf("sector write FAILED at address=0x%08lx status=0x%01x\n", dst, status);
				chip_writeb(CLEAR_STATUS, bios);
				return (-1);
			}
		} while (!(status & STATUS_WSMS));
	}

	return 0;
}

int probe_49lfxxxc(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;
	uint8_t id1, id2;

	chip_writeb(RESET, bios);

	chip_writeb(READ_ID, bios);
	id1 = chip_readb(bios);
	id2 = chip_readb(bios + 0x01);

	chip_writeb(RESET, bios);

	printf_debug("%s: id1 0x%02x, id2 0x%02x\n", __FUNCTION__, id1, id2);

	if (!(id1 == flash->manufacture_id && id2 == flash->model_id))
		return 0;

	map_flash_registers(flash);

	return 1;
}

int erase_49lfxxxc(struct flashchip *flash)
{
	chipaddr bios = flash->virtual_memory;
	int i;
	unsigned int total_size = flash->total_size * 1024;

	write_lockbits_49lfxxxc(flash, 0);
	for (i = 0; i < total_size; i += flash->page_size)
		if (erase_sector_49lfxxxc(flash, i, flash->page_size))
			return (-1);

	chip_writeb(RESET, bios);

	return 0;
}

int write_49lfxxxc(struct flashchip *flash, uint8_t *buf)
{
	int i;
	int total_size = flash->total_size * 1024;
	int page_size = flash->page_size;
	chipaddr bios = flash->virtual_memory;

	write_lockbits_49lfxxxc(flash, 0);
	printf("Programming page: ");
	for (i = 0; i < total_size / page_size; i++) {
		/* erase the page before programming */
		if (erase_sector_49lfxxxc(flash, i * page_size, flash->page_size)) {
			fprintf(stderr, "ERASE FAILED!\n");
			return -1;
		}

		/* write to the sector */
		printf("%04d at address: 0x%08x", i, i * page_size);
		write_sector_49lfxxxc(bios, buf + i * page_size,
				      bios + i * page_size, page_size);
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	}
	printf("\n");

	chip_writeb(RESET, bios);

	return 0;
}
