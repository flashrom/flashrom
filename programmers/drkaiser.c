/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2009 Joerg Fischer <turboj@web.de>
 */

#include <stdlib.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess_physmap.h"
#include "pcidev.h"

#define PCI_VENDOR_ID_DRKAISER		0x1803

#define PCI_MAGIC_DRKAISER_ADDR		0x50
#define PCI_MAGIC_DRKAISER_VALUE	0xa971

#define DRKAISER_MEMMAP_SIZE           (1024 * 128)

/* Mask to restrict flash accesses to the 128kB memory window. */
#define DRKAISER_MEMMAP_MASK		((1 << 17) - 1)

struct drkaiser_data {
	struct pci_dev *dev;
	uint8_t *bar;
	uint16_t flash_access;
};

static const struct dev_entry drkaiser_pcidev[] = {
	{0x1803, 0x5057, OK, "Dr. Kaiser", "PC-Waechter (Actel FPGA)"},

	{0},
};

static void drkaiser_chip_writeb(const struct flashctx *flash, uint8_t val,
				 chipaddr addr)
{
	struct drkaiser_data *data = flash->mst->par.data;

	pci_mmio_writeb(val, data->bar + (addr & DRKAISER_MEMMAP_MASK));
}

static uint8_t drkaiser_chip_readb(const struct flashctx *flash,
				   const chipaddr addr)
{
	struct drkaiser_data *data = flash->mst->par.data;

	return pci_mmio_readb(data->bar + (addr & DRKAISER_MEMMAP_MASK));
}

static int drkaiser_shutdown(void *par_data)
{
	struct drkaiser_data *data = par_data;

	/* Restore original flash writing state. */
	pci_write_word(data->dev, PCI_MAGIC_DRKAISER_ADDR, data->flash_access);

	free(par_data);
	return 0;
}

static const struct par_master par_master_drkaiser = {
	.chip_readb	= drkaiser_chip_readb,
	.chip_writeb	= drkaiser_chip_writeb,
	.shutdown	= drkaiser_shutdown,
};

static int drkaiser_init(const struct programmer_cfg *cfg)
{
	struct pci_dev *dev = NULL;
	uint32_t addr;
	uint8_t *bar;

	dev = pcidev_init(cfg, drkaiser_pcidev, PCI_BASE_ADDRESS_2);
	if (!dev)
		return 1;

	addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_2);
	if (!addr)
		return 1;

	/* Map 128kB flash memory window. */
	bar = rphysmap("Dr. Kaiser PC-Waechter flash memory", addr, DRKAISER_MEMMAP_SIZE);
	if (bar == ERROR_PTR)
		return 1;

	struct drkaiser_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for PAR master data\n");
		return 1;
	}
	data->dev = dev;
	data->bar = bar;

	/* Write magic register to enable flash write. */
	data->flash_access = pci_read_word(dev, PCI_MAGIC_DRKAISER_ADDR);
	pci_write_word(dev, PCI_MAGIC_DRKAISER_ADDR, PCI_MAGIC_DRKAISER_VALUE);

	max_rom_decode.parallel = 128 * 1024;

	return register_par_master(&par_master_drkaiser, BUS_PARALLEL, data);
}

const struct programmer_entry programmer_drkaiser = {
	.name			= "drkaiser",
	.type			= PCI,
	.devs.dev		= drkaiser_pcidev,
	.init			= drkaiser_init,
};
