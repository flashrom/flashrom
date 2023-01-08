/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
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

#define BIOS_ROM_ADDR		0x04
#define BIOS_ROM_DATA		0x08
#define INT_STATUS		0x0e
#define INTERNAL_CONFIG		0x00
#define SELECT_REG_WINDOW	0x800

#define PCI_VENDOR_ID_3COM	0x10b7

struct nic3com_data {
	uint32_t io_base_addr;
	uint32_t internal_conf;
	uint16_t id;
};

static const struct dev_entry nics_3com[] = {
	/* 3C90xB */
	{0x10b7, 0x9055, OK, "3COM", "3C90xB: PCI 10/100 Mbps; shared 10BASE-T/100BASE-TX"},
	{0x10b7, 0x9001, NT, "3COM", "3C90xB: PCI 10/100 Mbps; shared 10BASE-T/100BASE-T4" },
	{0x10b7, 0x9004, OK, "3COM", "3C90xB: PCI 10BASE-T (TPO)" },
	{0x10b7, 0x9005, NT, "3COM", "3C90xB: PCI 10BASE-T/10BASE2/AUI (COMBO)" },
	{0x10b7, 0x9006, OK, "3COM", "3C90xB: PCI 10BASE-T/10BASE2 (TPC)" },
	{0x10b7, 0x900a, NT, "3COM", "3C90xB: PCI 10BASE-FL" },
	{0x10b7, 0x905a, NT, "3COM", "3C90xB: PCI 10BASE-FX" },
	{0x10b7, 0x9058, OK, "3COM", "3C905B: Cyclone 10/100/BNC" },

	/* 3C905C */
	{0x10b7, 0x9200, OK, "3COM", "3C905C: EtherLink 10/100 PCI (TX)" },

	/* 3C980C */
	{0x10b7, 0x9805, NT, "3COM", "3C980C: EtherLink Server 10/100 PCI (TX)" },

	{0},
};

static void nic3com_chip_writeb(const struct flashctx *flash, uint8_t val,
				chipaddr addr)
{
	struct nic3com_data *data = flash->mst->par.data;

	OUTL((uint32_t)addr, data->io_base_addr + BIOS_ROM_ADDR);
	OUTB(val, data->io_base_addr + BIOS_ROM_DATA);
}

static uint8_t nic3com_chip_readb(const struct flashctx *flash,
				  const chipaddr addr)
{
	struct nic3com_data *data = flash->mst->par.data;

	OUTL((uint32_t)addr, data->io_base_addr + BIOS_ROM_ADDR);
	return INB(data->io_base_addr + BIOS_ROM_DATA);
}

static int nic3com_shutdown(void *par_data)
{
	struct nic3com_data *data = par_data;
	const uint16_t id = data->id;

	/* 3COM 3C90xB cards need a special fixup. */
	if (id == 0x9055 || id == 0x9001 || id == 0x9004 || id == 0x9005
	    || id == 0x9006 || id == 0x900a || id == 0x905a || id == 0x9058) {
		/* Select register window 3 and restore the receiver status. */
		OUTW(SELECT_REG_WINDOW + 3, data->io_base_addr + INT_STATUS);
		OUTL(data->internal_conf, data->io_base_addr + INTERNAL_CONFIG);
	}

	free(data);
	return 0;
}

static const struct par_master par_master_nic3com = {
	.chip_readb	= nic3com_chip_readb,
	.chip_writeb	= nic3com_chip_writeb,
	.shutdown	= nic3com_shutdown,
};

static int nic3com_init(const struct programmer_cfg *cfg)
{
	struct pci_dev *dev = NULL;
	uint32_t io_base_addr = 0;
	uint32_t internal_conf = 0;
	uint16_t id;

	if (rget_io_perms())
		return 1;

	dev = pcidev_init(cfg, nics_3com, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!io_base_addr)
		return 1;

	id = dev->device_id;

	/* 3COM 3C90xB cards need a special fixup. */
	if (id == 0x9055 || id == 0x9001 || id == 0x9004 || id == 0x9005
	    || id == 0x9006 || id == 0x900a || id == 0x905a || id == 0x9058) {
		/* Select register window 3 and save the receiver status. */
		OUTW(SELECT_REG_WINDOW + 3, io_base_addr + INT_STATUS);
		internal_conf = INL(io_base_addr + INTERNAL_CONFIG);

		/* Set receiver type to MII for full BIOS ROM access. */
		OUTL((internal_conf & 0xf00fffff) | 0x00600000, io_base_addr);
	}

	/*
	 * The lowest 16 bytes of the I/O mapped register space of (most) 3COM
	 * cards form a 'register window' into one of multiple (usually 8)
	 * register banks. For 3C90xB/3C90xC we need register window/bank 0.
	 */
	OUTW(SELECT_REG_WINDOW + 0, io_base_addr + INT_STATUS);

	struct nic3com_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for PAR master data\n");
		goto init_err_cleanup_exit;
	}
	data->io_base_addr = io_base_addr;
	data->internal_conf = internal_conf;
	data->id = id;

	max_rom_decode.parallel = 128 * 1024;

	return register_par_master(&par_master_nic3com, BUS_PARALLEL, data);

init_err_cleanup_exit:
	/* 3COM 3C90xB cards need a special fixup. */
	if (id == 0x9055 || id == 0x9001 || id == 0x9004 || id == 0x9005
	    || id == 0x9006 || id == 0x900a || id == 0x905a || id == 0x9058) {
		/* Select register window 3 and restore the receiver status. */
		OUTW(SELECT_REG_WINDOW + 3, io_base_addr + INT_STATUS);
		OUTL(internal_conf, io_base_addr + INTERNAL_CONFIG);
	}
	return 1;
}

const struct programmer_entry programmer_nic3com = {
	.name			= "nic3com",
	.type			= PCI,
	.devs.dev		= nics_3com,
	.init			= nic3com_init,
};
