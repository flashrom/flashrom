/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Joerg Fischer <turboj@gmx.de>
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
#include "flash.h"
#include "programmer.h"
#include "hwaccess_x86_io.h"
#include "platform/pci.h"

#define PCI_VENDOR_ID_REALTEK	0x10ec
#define PCI_VENDOR_ID_SMC1211	0x1113

struct nicrealtek_data {
	uint32_t io_base_addr;
	int bios_rom_addr;
	int bios_rom_data;
};

static const struct dev_entry nics_realtek[] = {
	{0x10ec, 0x8139, OK, "Realtek", "RTL8139/8139C/8139C+"},
	{0x10ec, 0x8169, NT, "Realtek", "RTL8169"},
	{0x1113, 0x1211, OK, "SMC", "1211TX"}, /* RTL8139 clone */

	{0},
};

static void nicrealtek_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr)
{
	struct nicrealtek_data *data = flash->mst->par.data;

	/* Output addr and data, set WE to 0, set OE to 1, set CS to 0,
	 * enable software access.
	 */
	OUTL(((uint32_t)addr & 0x01FFFF) | 0x0A0000 | (val << 24),
	     data->io_base_addr + data->bios_rom_addr);
	/* Output addr and data, set WE to 1, set OE to 1, set CS to 1,
	 * enable software access.
	 */
	OUTL(((uint32_t)addr & 0x01FFFF) | 0x1E0000 | (val << 24),
	     data->io_base_addr + data->bios_rom_addr);
}

static uint8_t nicrealtek_chip_readb(const struct flashctx *flash, const chipaddr addr)
{
	struct nicrealtek_data *data = flash->mst->par.data;
	uint8_t val;

	/* FIXME: Can we skip reading the old data and simply use 0? */
	/* Read old data. */
	val = INB(data->io_base_addr + data->bios_rom_data);
	/* Output new addr and old data, set WE to 1, set OE to 0, set CS to 0,
	 * enable software access.
	 */
	OUTL(((uint32_t)addr & 0x01FFFF) | 0x060000 | (val << 24),
	     data->io_base_addr + data->bios_rom_addr);

	/* Read new data. */
	val = INB(data->io_base_addr + data->bios_rom_data);
	/* Output addr and new data, set WE to 1, set OE to 1, set CS to 1,
	 * enable software access.
	 */
	OUTL(((uint32_t)addr & 0x01FFFF) | 0x1E0000 | (val << 24),
	     data->io_base_addr + data->bios_rom_addr);

	return val;
}

static int nicrealtek_shutdown(void *data)
{
	/* FIXME: We forgot to disable software access again. */
	free(data);
	return 0;
}

static const struct par_master par_master_nicrealtek = {
	.chip_readb	= nicrealtek_chip_readb,
	.chip_writeb	= nicrealtek_chip_writeb,
	.shutdown	= nicrealtek_shutdown,
};

static int nicrealtek_init(const struct programmer_cfg *cfg)
{
	struct pci_dev *dev = NULL;
	uint32_t io_base_addr = 0;
	int bios_rom_addr;
	int bios_rom_data;

	if (rget_io_perms())
		return 1;

	dev = pcidev_init(cfg, nics_realtek, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!io_base_addr)
		return 1;

	/* Beware, this ignores the vendor ID! */
	switch (dev->device_id) {
	case 0x8139: /* RTL8139 */
	case 0x1211: /* SMC 1211TX */
	default:
		bios_rom_addr = 0xD4;
		bios_rom_data = 0xD7;
		break;
	case 0x8169: /* RTL8169 */
		bios_rom_addr = 0x30;
		bios_rom_data = 0x33;
		break;
	}

	struct nicrealtek_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for PAR master data\n");
		return 1;
	}
	data->io_base_addr = io_base_addr;
	data->bios_rom_addr = bios_rom_addr;
	data->bios_rom_data = bios_rom_data;

	return register_par_master(&par_master_nicrealtek, BUS_PARALLEL, data);
}

const struct programmer_entry programmer_nicrealtek = {
	.name			= "nicrealtek",
	.type			= PCI,
	.devs.dev		= nics_realtek,
	.init			= nicrealtek_init,
};
