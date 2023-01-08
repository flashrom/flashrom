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
 */

#include <stdlib.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess_physmap.h"
#include "platform/pci.h"

struct it8212_data {
	struct pci_dev *dev;
	uint8_t *bar;
	uint32_t decode_access;
};

#define PCI_VENDOR_ID_ITE 0x1283

static const struct dev_entry devs_it8212[] = {
	{PCI_VENDOR_ID_ITE, 0x8212, NT, "ITE", "8212F PATA RAID"},

	{0},
};

#define IT8212_MEMMAP_SIZE (128 * 1024)
#define IT8212_MEMMAP_MASK (IT8212_MEMMAP_SIZE - 1)

static void it8212_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr)
{
	const struct it8212_data *data = flash->mst->par.data;

	pci_mmio_writeb(val, data->bar + (addr & IT8212_MEMMAP_MASK));
}

static uint8_t it8212_chip_readb(const struct flashctx *flash, const chipaddr addr)
{
	const struct it8212_data *data = flash->mst->par.data;

	return pci_mmio_readb(data->bar + (addr & IT8212_MEMMAP_MASK));
}

static int it8212_shutdown(void *par_data)
{
	struct it8212_data *data = par_data;

	/* Restore ROM BAR decode state. */
	pci_write_long(data->dev, PCI_ROM_ADDRESS, data->decode_access);

	free(par_data);
	return 0;
}

static const struct par_master par_master_it8212 = {
	.chip_readb	= it8212_chip_readb,
	.chip_writeb	= it8212_chip_writeb,
	.shutdown	= it8212_shutdown,
};

static int it8212_init(const struct programmer_cfg *cfg)
{
	uint8_t *bar;

	struct pci_dev *dev = pcidev_init(cfg, devs_it8212, PCI_ROM_ADDRESS);
	if (!dev)
		return 1;

	/* Bit 0 is address decode enable, 17-31 the base address, everything else reserved/zero. */
	uint32_t io_base_addr = pcidev_readbar(dev, PCI_ROM_ADDRESS) & 0xFFFFFFFE;
	if (!io_base_addr)
		return 1;

	bar = rphysmap("IT8212F flash", io_base_addr, IT8212_MEMMAP_SIZE);
	if (bar == ERROR_PTR)
		return 1;

	struct it8212_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for PAR master data\n");
		return 1;
	}
	data->dev = dev;
	data->bar = bar;

	/* Enable ROM BAR decoding. */
	data->decode_access = pci_read_long(dev, PCI_ROM_ADDRESS);
	pci_write_long(dev, PCI_ROM_ADDRESS, io_base_addr | 0x01);

	max_rom_decode.parallel = IT8212_MEMMAP_SIZE;
	return register_par_master(&par_master_it8212, BUS_PARALLEL, data);
}
const struct programmer_entry programmer_it8212 = {
	.name			= "it8212",
	.type			= PCI,
	.devs.dev		= devs_it8212,
	.init			= it8212_init,
};
