/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Joerg Fischer <turboj@web.de>
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

#include <stdlib.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"

#define PCI_VENDOR_ID_DRKAISER		0x1803

#define PCI_MAGIC_DRKAISER_ADDR		0x50
#define PCI_MAGIC_DRKAISER_VALUE	0xa971

#define DRKAISER_MEMMAP_SIZE           (1024 * 128)

/* Mask to restrict flash accesses to the 128kB memory window. */
#define DRKAISER_MEMMAP_MASK		((1 << 17) - 1)

const struct dev_entry drkaiser_pcidev[] = {
	{0x1803, 0x5057, OK, "Dr. Kaiser", "PC-Waechter (Actel FPGA)"},

	{0},
};

static uint8_t *drkaiser_bar;

static void drkaiser_chip_writeb(const struct flashctx *flash, uint8_t val,
				 chipaddr addr);
static uint8_t drkaiser_chip_readb(const struct flashctx *flash,
				   const chipaddr addr);
static const struct par_master par_master_drkaiser = {
		.chip_readb		= drkaiser_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= fallback_chip_readn,
		.chip_writeb		= drkaiser_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
};

int drkaiser_init(void)
{
	struct pci_dev *dev = NULL;
	uint32_t addr;

	if (rget_io_perms())
		return 1;

	dev = pcidev_init(drkaiser_pcidev, PCI_BASE_ADDRESS_2);
	if (!dev)
		return 1;

	addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_2);
	if (!addr)
		return 1;

	/* Write magic register to enable flash write. */
	rpci_write_word(dev, PCI_MAGIC_DRKAISER_ADDR, PCI_MAGIC_DRKAISER_VALUE);

	/* Map 128kB flash memory window. */
	drkaiser_bar = rphysmap("Dr. Kaiser PC-Waechter flash memory", addr, DRKAISER_MEMMAP_SIZE);
	if (drkaiser_bar == ERROR_PTR)
		return 1;

	max_rom_decode.parallel = 128 * 1024;
	register_par_master(&par_master_drkaiser, BUS_PARALLEL);

	return 0;
}

static void drkaiser_chip_writeb(const struct flashctx *flash, uint8_t val,
				 chipaddr addr)
{
	pci_mmio_writeb(val, drkaiser_bar + (addr & DRKAISER_MEMMAP_MASK));
}

static uint8_t drkaiser_chip_readb(const struct flashctx *flash,
				   const chipaddr addr)
{
	return pci_mmio_readb(drkaiser_bar + (addr & DRKAISER_MEMMAP_MASK));
}
