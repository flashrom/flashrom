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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdlib.h>
#include <string.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"

#define PCI_VENDOR_ID_NVIDIA	0x10de

/* Mask to restrict flash accesses to a 128kB memory window.
 * FIXME: Is this size a one-fits-all or card dependent?
 */
#define GFXNVIDIA_MEMMAP_MASK		((1 << 17) - 1)
#define GFXNVIDIA_MEMMAP_SIZE		(16 * 1024 * 1024)

uint8_t *nvidia_bar;

const struct dev_entry gfx_nvidia[] = {
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
				  chipaddr addr);
static uint8_t gfxnvidia_chip_readb(const struct flashctx *flash,
				    const chipaddr addr);
static const struct par_master par_master_gfxnvidia = {
		.chip_readb		= gfxnvidia_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= fallback_chip_readn,
		.chip_writeb		= gfxnvidia_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
};

int gfxnvidia_init(void)
{
	struct pci_dev *dev = NULL;
	uint32_t reg32;

	if (rget_io_perms())
		return 1;

	dev = pcidev_init(gfx_nvidia, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	uint32_t io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!io_base_addr)
		return 1;

	io_base_addr += 0x300000;
	msg_pinfo("Detected NVIDIA I/O base address: 0x%x.\n", io_base_addr);

	nvidia_bar = rphysmap("NVIDIA", io_base_addr, GFXNVIDIA_MEMMAP_SIZE);
	if (nvidia_bar == ERROR_PTR)
		return 1;

	/* Allow access to flash interface (will disable screen). */
	reg32 = pci_read_long(dev, 0x50);
	reg32 &= ~(1 << 0);
	rpci_write_long(dev, 0x50, reg32);

	/* Write/erase doesn't work. */
	programmer_may_write = 0;
	register_par_master(&par_master_gfxnvidia, BUS_PARALLEL);

	return 0;
}

static void gfxnvidia_chip_writeb(const struct flashctx *flash, uint8_t val,
				  chipaddr addr)
{
	pci_mmio_writeb(val, nvidia_bar + (addr & GFXNVIDIA_MEMMAP_MASK));
}

static uint8_t gfxnvidia_chip_readb(const struct flashctx *flash,
				    const chipaddr addr)
{
	return pci_mmio_readb(nvidia_bar + (addr & GFXNVIDIA_MEMMAP_MASK));
}
