/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2011 Carl-Daniel Hailfinger
 * Copyright (C) 2012 Kyösti Mälkki <kyosti.malkki@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
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

static uint8_t *it8212_bar = NULL;

#define PCI_VENDOR_ID_ITE 0x1283

const struct dev_entry devs_it8212[] = {
	{PCI_VENDOR_ID_ITE, 0x8212, NT, "ITE", "8212F PATA RAID"},

	{},
};

#define IT8212_MEMMAP_SIZE (128 * 1024)
#define IT8212_MEMMAP_MASK (IT8212_MEMMAP_SIZE - 1)

static void it8212_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr);
static uint8_t it8212_chip_readb(const struct flashctx *flash, const chipaddr addr);
static const struct par_master par_master_it8212 = {
		.chip_readb		= it8212_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= fallback_chip_readn,
		.chip_writeb		= it8212_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
};

int it8212_init(void)
{
	if (rget_io_perms())
		return 1;

	struct pci_dev *dev = pcidev_init(devs_it8212, PCI_ROM_ADDRESS);
	if (!dev)
		return 1;

	/* Bit 0 is address decode enable, 17-31 the base address, everything else reserved/zero. */
	uint32_t io_base_addr = pcidev_readbar(dev, PCI_ROM_ADDRESS) & 0xFFFFFFFE;
	if (!io_base_addr)
		return 1;

	it8212_bar = rphysmap("IT8212F flash", io_base_addr, IT8212_MEMMAP_SIZE);
	if (it8212_bar == ERROR_PTR)
		return 1;

	/* Restore ROM BAR decode state automatically at shutdown. */
	rpci_write_long(dev, PCI_ROM_ADDRESS, io_base_addr | 0x01);

	max_rom_decode.parallel = IT8212_MEMMAP_SIZE;
	register_par_master(&par_master_it8212, BUS_PARALLEL);
	return 0;
}

static void it8212_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr)
{
	pci_mmio_writeb(val, it8212_bar + (addr & IT8212_MEMMAP_MASK));
}

static uint8_t it8212_chip_readb(const struct flashctx *flash, const chipaddr addr)
{
	return pci_mmio_readb(it8212_bar + (addr & IT8212_MEMMAP_MASK));
}
