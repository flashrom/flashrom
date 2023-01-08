/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Uwe Hermann <uwe@hermann-uwe.de>
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

#include <stdlib.h>
#include <string.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess_x86_io.h"
#include "platform/pci.h"

#define BIOS_ROM_ADDR		0x90
#define BIOS_ROM_DATA		0x94

#define REG_FLASH_ACCESS	0x58
#define BIT_FLASH_ACCESS	BIT(24)

#define PCI_VENDOR_ID_HPT	0x1103

struct atahpt_data {
	struct pci_dev *dev;
	uint32_t io_base_addr;
	uint32_t flash_access;
};

static const struct dev_entry ata_hpt[] = {
	{0x1103, 0x0004, NT, "Highpoint", "HPT366/368/370/370A/372/372N"},
	{0x1103, 0x0005, NT, "Highpoint", "HPT372A/372N"},
	{0x1103, 0x0006, NT, "Highpoint", "HPT302/302N"},

	{0},
};

static void atahpt_chip_writeb(const struct flashctx *flash, uint8_t val,
			       chipaddr addr)
{
	struct atahpt_data *data = flash->mst->par.data;

	OUTL((uint32_t)addr, data->io_base_addr + BIOS_ROM_ADDR);
	OUTB(val, data->io_base_addr + BIOS_ROM_DATA);
}

static uint8_t atahpt_chip_readb(const struct flashctx *flash,
				 const chipaddr addr)
{
	struct atahpt_data *data = flash->mst->par.data;

	OUTL((uint32_t)addr, data->io_base_addr + BIOS_ROM_ADDR);
	return INB(data->io_base_addr + BIOS_ROM_DATA);
}

static int atahpt_shutdown(void *par_data)
{
	struct atahpt_data *data = par_data;

	/* Restore original flash access state. */
	pci_write_long(data->dev, REG_FLASH_ACCESS, data->flash_access);

	free(par_data);
	return 0;
}

static const struct par_master par_master_atahpt = {
	.chip_readb	= atahpt_chip_readb,
	.chip_writeb	= atahpt_chip_writeb,
	.shutdown	= atahpt_shutdown,
};

static int atahpt_init(const struct programmer_cfg *cfg)
{
	struct pci_dev *dev = NULL;
	uint32_t io_base_addr;

	if (rget_io_perms())
		return 1;

	dev = pcidev_init(cfg, ata_hpt, PCI_BASE_ADDRESS_4);
	if (!dev)
		return 1;

	io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_4);
	if (!io_base_addr)
		return 1;

	struct atahpt_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for PAR master data\n");
		return 1;
	}
	data->dev = dev;
	data->io_base_addr = io_base_addr;

	/* Enable flash access. */
	data->flash_access = pci_read_long(dev, REG_FLASH_ACCESS);
	pci_write_long(dev, REG_FLASH_ACCESS, data->flash_access | BIT_FLASH_ACCESS);

	return register_par_master(&par_master_atahpt, BUS_PARALLEL, data);
}

const struct programmer_entry programmer_atahpt = {
	.name			= "atahpt",
	.type			= PCI,
	.devs.dev		= ata_hpt,
	.init			= atahpt_init,
};
