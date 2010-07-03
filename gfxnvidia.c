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
#include <sys/types.h>
#include "flash.h"

#define PCI_VENDOR_ID_NVIDIA	0x10de

uint8_t *nvidia_bar;

const struct pcidev_status gfx_nvidia[] = {
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

	{},
};

int gfxnvidia_init(void)
{
	uint32_t reg32;

	get_io_perms();

	io_base_addr = pcidev_init(PCI_VENDOR_ID_NVIDIA, PCI_BASE_ADDRESS_0,
				   gfx_nvidia, programmer_param);
	io_base_addr += 0x300000;
	msg_pinfo("Detected NVIDIA I/O base address: 0x%x.\n", io_base_addr);

	/* Allow access to flash interface (will disable screen). */
	reg32 = pci_read_long(pcidev_dev, 0x50);
	reg32 &= ~(1 << 0);
	pci_write_long(pcidev_dev, 0x50, reg32);

	nvidia_bar = physmap("NVIDIA", io_base_addr, 16 * 1024 * 1024);

	buses_supported = CHIP_BUSTYPE_PARALLEL;

	return 0;
}

int gfxnvidia_shutdown(void)
{
	uint32_t reg32;

	/* Disallow access to flash interface (and re-enable screen). */
	reg32 = pci_read_long(pcidev_dev, 0x50);
	reg32 |= (1 << 0);
	pci_write_long(pcidev_dev, 0x50, reg32);

	free(programmer_param);
	pci_cleanup(pacc);
	release_io_perms();
	return 0;
}

void gfxnvidia_chip_writeb(uint8_t val, chipaddr addr)
{
	mmio_writeb(val, nvidia_bar + addr);
}

uint8_t gfxnvidia_chip_readb(const chipaddr addr)
{
	return mmio_readb(nvidia_bar + addr);
}
