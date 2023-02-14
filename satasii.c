/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Rudolf Marek <r.marek@assembler.cz>
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

/* Datasheets can be found on http://www.siliconimage.com. Great thanks! */

#include <stdlib.h>
#include "programmer.h"
#include "hwaccess_physmap.h"
#include "platform/pci.h"

#define PCI_VENDOR_ID_SII	0x1095

#define SATASII_MEMMAP_SIZE	0x100

struct satasii_data {
	uint8_t *bar;
};

static const struct dev_entry satas_sii[] = {
	{0x1095, 0x0680, OK, "Silicon Image", "PCI0680 Ultra ATA-133 Host Ctrl"},
	{0x1095, 0x3112, OK, "Silicon Image", "SiI 3112 [SATALink/SATARaid] SATA Ctrl"},
	{0x1095, 0x3114, OK, "Silicon Image", "SiI 3114 [SATALink/SATARaid] SATA Ctrl"},
	{0x1095, 0x3124, OK, "Silicon Image", "SiI 3124 PCI-X SATA Ctrl"},
	{0x1095, 0x3132, OK, "Silicon Image", "SiI 3132 SATA Raid II Ctrl"},
	{0x1095, 0x3512, OK, "Silicon Image", "SiI 3512 [SATALink/SATARaid] SATA Ctrl"},

	{0},
};

static uint32_t satasii_wait_done(const uint8_t *bar)
{
	uint32_t ctrl_reg;
	int i = 0;
	while ((ctrl_reg = pci_mmio_readl(bar)) & (1 << 25)) {
		if (++i > 10000) {
			msg_perr("%s: control register stuck at %08"PRIx32", ignoring.\n",
				 __func__, pci_mmio_readl(bar));
			break;
		}
	}
	return ctrl_reg;
}

static void satasii_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr)
{
	const struct satasii_data *data = flash->mst->par.data;
	uint32_t data_reg;
	uint32_t ctrl_reg = satasii_wait_done(data->bar);

	/* Mask out unused/reserved bits, set writes and start transaction. */
	ctrl_reg &= 0xfcf80000;
	ctrl_reg |= (1 << 25) | (0 << 24) | ((uint32_t) addr & 0x7ffff);

	data_reg = (pci_mmio_readl((data->bar + 4)) & ~0xff) | val;
	pci_mmio_writel(data_reg, (data->bar + 4));
	pci_mmio_writel(ctrl_reg, data->bar);

	satasii_wait_done(data->bar);
}

static uint8_t satasii_chip_readb(const struct flashctx *flash, const chipaddr addr)
{
	const struct satasii_data *data = flash->mst->par.data;
	uint32_t ctrl_reg = satasii_wait_done(data->bar);

	/* Mask out unused/reserved bits, set reads and start transaction. */
	ctrl_reg &= 0xfcf80000;
	ctrl_reg |= (1 << 25) | (1 << 24) | ((uint32_t) addr & 0x7ffff);

	pci_mmio_writel(ctrl_reg, data->bar);

	satasii_wait_done(data->bar);

	return (pci_mmio_readl(data->bar + 4)) & 0xff;
}

static int satasii_shutdown(void *par_data)
{
	free(par_data);
	return 0;
}

static const struct par_master par_master_satasii = {
	.chip_readb	= satasii_chip_readb,
	.chip_writeb	= satasii_chip_writeb,
	.shutdown	= satasii_shutdown,
};

static int satasii_init(const struct programmer_cfg *cfg)
{
	struct pci_dev *dev = NULL;
	uint32_t addr;
	uint16_t reg_offset, id;
	uint8_t *bar;

	dev = pcidev_init(cfg, satas_sii, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	id = dev->device_id;

	if ((id == 0x3132) || (id == 0x3124)) {
		addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
		if (!addr)
			return 1;
		reg_offset = 0x70;
	} else {
		addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_5);
		if (!addr)
			return 1;
		reg_offset = 0x50;
	}

	bar = rphysmap("SATA SiI registers", addr, SATASII_MEMMAP_SIZE);
	if (bar == ERROR_PTR)
		return 1;
	bar += reg_offset;

	/* Check if ROM cycle are OK. */
	if ((id != 0x0680) && (!(pci_mmio_readl(bar) & (1 << 26))))
		msg_pwarn("Warning: Flash seems unconnected.\n");

	struct satasii_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for PAR master data\n");
		return 1;
	}
	data->bar = bar;

	return register_par_master(&par_master_satasii, BUS_PARALLEL, data);
}
const struct programmer_entry programmer_satasii = {
	.name			= "satasii",
	.type			= PCI,
	.devs.dev		= satas_sii,
	.init			= satasii_init,
};
