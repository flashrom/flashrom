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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess_physmap.h"
#include "platform/pci.h"

#define PCI_VENDOR_ID_NVIDIA	0x10de

/* Mask to restrict flash accesses to a 128kB memory window.
 * FIXME: Is this size a one-fits-all or card dependent?
 */
#define GFXNVIDIA_MEMMAP_MASK		((1 << 17) - 1)
#define GFXNVIDIA_MEMMAP_SIZE		(16 * 1024 * 1024)

#define REG_FLASH_ACCESS	0x50
#define BIT_FLASH_ACCESS	BIT(0)

struct gfxnvidia_data {
	struct pci_dev *dev;
	uint8_t *bar;
	uint32_t flash_access;
};

static const struct dev_entry gfx_nvidia[] = {
	{0x10de, 0x0010, NT, "NVIDIA", "Mutara V08 [NV2]" },
	{0x10de, 0x0018, NT, "NVIDIA", "RIVA 128" },
	{0x10de, 0x0020, NT, "NVIDIA", "RIVA TNT" },
	{0x10de, 0x0028, NT, "NVIDIA", "RIVA TNT2/TNT2 Pro" },
	{0x10de, 0x0029, NT, "NVIDIA", "RIVA TNT2 Ultra" },
	{0x10de, 0x002c, NT, "NVIDIA", "Vanta/Vanta LT" },
	{0x10de, 0x002d, OK, "NVIDIA", "RIVA TNT2 Model 64/Model 64 Pro" },
	{0x10de, 0x00a0, NT, "NVIDIA", "Aladdin TNT2" },
	{0x10de, 0x0100, NT, "NVIDIA", "GeForce 256" },
	{0x10de, 0x0101, NT, "NVIDIA", "GeForce DDR" },
	{0x10de, 0x0103, NT, "NVIDIA", "Quadro" },
	{0x10de, 0x0110, NT, "NVIDIA", "GeForce2 MX" },
	{0x10de, 0x0111, NT, "NVIDIA", "GeForce2 MX" },
	{0x10de, 0x0112, NT, "NVIDIA", "GeForce2 GO" },
	{0x10de, 0x0113, NT, "NVIDIA", "Quadro2 MXR" },
	{0x10de, 0x0150, NT, "NVIDIA", "GeForce2 GTS/Pro" },
	{0x10de, 0x0151, NT, "NVIDIA", "GeForce2 GTS" },
	{0x10de, 0x0152, NT, "NVIDIA", "GeForce2 Ultra" },
	{0x10de, 0x0153, NT, "NVIDIA", "Quadro2 Pro" },
	{0x10de, 0x0200, NT, "NVIDIA", "GeForce 3 nFX" },
	{0x10de, 0x0201, NT, "NVIDIA", "GeForce 3 nFX" },
	{0x10de, 0x0202, NT, "NVIDIA", "GeForce 3 nFX Ultra" },
	{0x10de, 0x0203, NT, "NVIDIA", "Quadro 3 DDC" },

	{0},
};

static void gfxnvidia_chip_writeb(const struct flashctx *flash, uint8_t val,
				  chipaddr addr)
{
	const struct gfxnvidia_data *data = flash->mst->par.data;

	pci_mmio_writeb(val, data->bar + (addr & GFXNVIDIA_MEMMAP_MASK));
}

static uint8_t gfxnvidia_chip_readb(const struct flashctx *flash,
				    const chipaddr addr)
{
	const struct gfxnvidia_data *data = flash->mst->par.data;

	return pci_mmio_readb(data->bar + (addr & GFXNVIDIA_MEMMAP_MASK));
}

static int gfxnvidia_shutdown(void *par_data)
{
	struct gfxnvidia_data *data = par_data;

	/* Restore original flash interface access state. */
	pci_write_long(data->dev, REG_FLASH_ACCESS, data->flash_access);

	free(par_data);
	return 0;
}

static const struct par_master par_master_gfxnvidia = {
	.chip_readb	= gfxnvidia_chip_readb,
	.chip_writeb	= gfxnvidia_chip_writeb,
	.shutdown	= gfxnvidia_shutdown,
};

static int gfxnvidia_init(const struct programmer_cfg *cfg)
{
	struct pci_dev *dev = NULL;
	uint32_t reg32;
	uint8_t *bar;

	dev = pcidev_init(cfg, gfx_nvidia, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	uint32_t io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!io_base_addr)
		return 1;

	io_base_addr += 0x300000;
	msg_pinfo("Detected NVIDIA I/O base address: 0x%"PRIx32".\n", io_base_addr);

	bar = rphysmap("NVIDIA", io_base_addr, GFXNVIDIA_MEMMAP_SIZE);
	if (bar == ERROR_PTR)
		return 1;

	struct gfxnvidia_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for PAR master data\n");
		return 1;
	}
	data->dev = dev;
	data->bar = bar;

	/* Allow access to flash interface (will disable screen). */
	data->flash_access = pci_read_long(dev, REG_FLASH_ACCESS);
	reg32 = data->flash_access & ~BIT_FLASH_ACCESS;
	pci_write_long(dev, REG_FLASH_ACCESS, reg32);

	/* Write/erase doesn't work. */
	programmer_may_write = false;
	return register_par_master(&par_master_gfxnvidia, BUS_PARALLEL, data);
}

const struct programmer_entry programmer_gfxnvidia = {
	.name			= "gfxnvidia",
	.type			= PCI,
	.devs.dev		= gfx_nvidia,
	.init			= gfxnvidia_init,
};
