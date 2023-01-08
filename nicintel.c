/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2011 Carl-Daniel Hailfinger
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

/* Datasheet: http://download.intel.com/design/network/datashts/82559_Fast_Ethernet_Multifunction_PCI_Cardbus_Controller_Datasheet.pdf */

#include <stdlib.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess_physmap.h"
#include "platform/pci.h"

struct nicintel_data {
	uint8_t *nicintel_bar;
	uint8_t *nicintel_control_bar;
};

static const struct dev_entry nics_intel[] = {
	{PCI_VENDOR_ID_INTEL, 0x1209, NT, "Intel", "8255xER/82551IT Fast Ethernet Controller"},
	{PCI_VENDOR_ID_INTEL, 0x1229, OK, "Intel", "82557/8/9/0/1 Ethernet Pro 100"},

	{0},
};

/* Arbitrary limit, taken from the datasheet I just had lying around.
 * 128 kByte on the 82559 device. Or not. Depends on whom you ask.
 */
#define NICINTEL_MEMMAP_SIZE (128 * 1024)
#define NICINTEL_MEMMAP_MASK (NICINTEL_MEMMAP_SIZE - 1)

#define NICINTEL_CONTROL_MEMMAP_SIZE	0x10

#define CSR_FCR 0x0c

static void nicintel_chip_writeb(const struct flashctx *flash, uint8_t val,
				 chipaddr addr)
{
	const struct nicintel_data *data = flash->mst->par.data;

	pci_mmio_writeb(val, data->nicintel_bar + (addr & NICINTEL_MEMMAP_MASK));
}

static uint8_t nicintel_chip_readb(const struct flashctx *flash,
				   const chipaddr addr)
{
	const struct nicintel_data *data = flash->mst->par.data;

	return pci_mmio_readb(data->nicintel_bar + (addr & NICINTEL_MEMMAP_MASK));
}

static int nicintel_shutdown(void *par_data)
{
	free(par_data);
	return 0;
}

static const struct par_master par_master_nicintel = {
	.chip_readb	= nicintel_chip_readb,
	.chip_writeb	= nicintel_chip_writeb,
	.shutdown	= nicintel_shutdown,
};

static int nicintel_init(const struct programmer_cfg *cfg)
{
	struct pci_dev *dev = NULL;
	uintptr_t addr;
	uint8_t *bar;
	uint8_t *control_bar;

	/* FIXME: BAR2 is not available if the device uses the CardBus function. */
	dev = pcidev_init(cfg, nics_intel, PCI_BASE_ADDRESS_2);
	if (!dev)
		return 1;

	addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_2);
	if (!addr)
		return 1;

	bar = rphysmap("Intel NIC flash", addr, NICINTEL_MEMMAP_SIZE);
	if (bar == ERROR_PTR)
		return 1;

	addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!addr)
		return 1;

	control_bar = rphysmap("Intel NIC control/status reg", addr, NICINTEL_CONTROL_MEMMAP_SIZE);
	if (control_bar == ERROR_PTR)
		return 1;

	/* FIXME: This register is pretty undocumented in all publicly available
	 * documentation from Intel. Let me quote the complete info we have:
	 * "Flash Control Register: The Flash Control register allows the CPU to
	 *  enable writes to an external Flash. The Flash Control Register is a
	 *  32-bit field that allows access to an external Flash device."
	 * Ah yes, we also know where it is, but we have absolutely _no_ idea
	 * what we should do with it. Write 0x0001 because we have nothing
	 * better to do with our time.
	 */
	pci_rmmio_writew(0x0001, control_bar + CSR_FCR);

	struct nicintel_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for PAR master data\n");
		return 1;
	}
	data->nicintel_bar = bar;
	data->nicintel_control_bar = control_bar;

	max_rom_decode.parallel = NICINTEL_MEMMAP_SIZE;
	return register_par_master(&par_master_nicintel, BUS_PARALLEL, data);
}

const struct programmer_entry programmer_nicintel = {
	.name			= "nicintel",
	.type			= PCI,
	.devs.dev		= nics_intel,
	.init			= nicintel_init,
};
