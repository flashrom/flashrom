/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2015 Joseph C. Lehner <joseph.c.lehner@gmail.com>
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

#if defined(__i386__) || defined(__x86_64__)

#include <string.h>
#include <stdlib.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"

#define MAX_ROM_DECODE (32 * 1024)
#define ADDR_MASK (MAX_ROM_DECODE - 1)

/*
 * In the absence of any public docs on the PDC2026x family, this programmer was created through a mix of
 * reverse-engineering and trial and error.
 *
 * The only device tested is an Ultra100 controller, but the logic for programming the other 2026x controllers
 * is the same, so it should, in theory, work for those as well.
 *
 * While the tested Ultra100 controller used a 128 kB MX29F001T chip, A16 and A15 showed continuity to ground,
 * thus limiting the the programmer on this card to 32 kB. Without other controllers to test this programmer on,
 * this is currently a hard limit. Note that ROM files for these controllers are 16 kB only.
 *
 * Since flashrom does not support accessing flash chips larger than the size limit of the programmer (the
 * tested Ultra100 uses a 128 kB MX29F001T chip), the chip size is hackishly adjusted in atapromise_limit_chip.
 */

static uint32_t io_base_addr = 0;
static uint32_t rom_base_addr = 0;

static uint8_t *atapromise_bar = NULL;
static size_t rom_size = 0;

const struct dev_entry ata_promise[] = {
	{0x105a, 0x4d38, NT, "Promise", "PDC20262 (FastTrak66/Ultra66)"},
	{0x105a, 0x0d30, NT, "Promise", "PDC20265 (FastTrak100 Lite/Ultra100)"},
	{0x105a, 0x4d30, OK, "Promise", "PDC20267 (FastTrak100/Ultra100)"},
	{0},
};

static void atapromise_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr);
static uint8_t atapromise_chip_readb(const struct flashctx *flash, const chipaddr addr);

static const struct par_master par_master_atapromise = {
		.chip_readb		= atapromise_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= fallback_chip_readn,
		.chip_writeb		= atapromise_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
};

void *atapromise_map(const char *descr, uintptr_t phys_addr, size_t len)
{
	/* In case fallback_map ever returns something other than NULL. */
	return NULL;
}

static void atapromise_limit_chip(struct flashchip *chip)
{
	unsigned int i, size;
	unsigned int usable_erasers = 0;

	size = chip->total_size * 1024;

	/* Chip is small enough or already limited. */
	if (size <= rom_size)
		return;

	/* Undefine all block_erasers that don't operate on the whole chip,
	 * and adjust the eraseblock size of those which do.
	 */
	for (i = 0; i < NUM_ERASEFUNCTIONS; ++i) {
		if (chip->block_erasers[i].eraseblocks[0].size != size) {
			chip->block_erasers[i].eraseblocks[0].count = 0;
			chip->block_erasers[i].block_erase = NULL;
		} else {
			chip->block_erasers[i].eraseblocks[0].size = rom_size;
			usable_erasers++;
		}
	}

	if (usable_erasers) {
		chip->total_size = rom_size / 1024;
		if (chip->page_size > rom_size)
			chip->page_size = rom_size;
	} else {
		msg_pdbg("Failed to adjust size of chip \"%s\" (%d kB).\n", chip->name, chip->total_size);
	}
}

int atapromise_init(void)
{
	struct pci_dev *dev = NULL;

	if (rget_io_perms())
		return 1;

	dev = pcidev_init(ata_promise, PCI_BASE_ADDRESS_4);
	if (!dev)
		return 1;

	io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_4) & 0xfffe;
	if (!io_base_addr) {
		return 1;
	}

	/* Not exactly sure what this does, because flashing seems to work
	 * well without it. However, PTIFLASH does it, so we do it too.
	 */
	OUTB(1, io_base_addr + 0x10);

	rom_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_5);
	if (!rom_base_addr) {
		msg_pdbg("Failed to read BAR5\n");
		return 1;
	}

	rom_size = dev->rom_size > MAX_ROM_DECODE ? MAX_ROM_DECODE : dev->rom_size;
	atapromise_bar = (uint8_t*)rphysmap("Promise", rom_base_addr, rom_size);
	if (atapromise_bar == ERROR_PTR) {
		return 1;
	}

	max_rom_decode.parallel = rom_size;
	register_par_master(&par_master_atapromise, BUS_PARALLEL);

	msg_pwarn("Do not use this device as a generic programmer. It will leave anything outside\n"
		  "the first %zu kB of the flash chip in an undefined state. It works fine for the\n"
		  "purpose of updating the firmware of this device (padding may necessary).\n",
		  rom_size / 1024);

	return 0;
}

static void atapromise_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr)
{
	uint32_t data;

	atapromise_limit_chip(flash->chip);
	data = (rom_base_addr + (addr & ADDR_MASK)) << 8 | val;
	OUTL(data, io_base_addr + 0x14);
}

static uint8_t atapromise_chip_readb(const struct flashctx *flash, const chipaddr addr)
{
	atapromise_limit_chip(flash->chip);
	return pci_mmio_readb(atapromise_bar + (addr & ADDR_MASK));
}

#else
#error PCI port I/O access is not supported on this architecture yet.
#endif
