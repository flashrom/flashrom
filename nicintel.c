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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* Datasheet: http://download.intel.com/design/network/datashts/82559_Fast_Ethernet_Multifunction_PCI_Cardbus_Controller_Datasheet.pdf */

#include <stdlib.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"

uint8_t *nicintel_bar;
uint8_t *nicintel_control_bar;

const struct dev_entry nics_intel[] = {
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
				 chipaddr addr);
static uint8_t nicintel_chip_readb(const struct flashctx *flash,
				   const chipaddr addr);
static const struct par_master par_master_nicintel = {
		.chip_readb		= nicintel_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= fallback_chip_readn,
		.chip_writeb		= nicintel_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
};

int nicintel_init(void)
{
	struct pci_dev *dev = NULL;
	uintptr_t addr;

	/* Needed only for PCI accesses on some platforms.
	 * FIXME: Refactor that into get_mem_perms/rget_io_perms/get_pci_perms?
	 */
	if (rget_io_perms())
		return 1;

	/* FIXME: BAR2 is not available if the device uses the CardBus function. */
	dev = pcidev_init(nics_intel, PCI_BASE_ADDRESS_2);
	if (!dev)
		return 1;

	addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_2);
	if (!addr)
		return 1;

	nicintel_bar = rphysmap("Intel NIC flash", addr, NICINTEL_MEMMAP_SIZE);
	if (nicintel_bar == ERROR_PTR)
		return 1;

	addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!addr)
		return 1;

	nicintel_control_bar = rphysmap("Intel NIC control/status reg", addr, NICINTEL_CONTROL_MEMMAP_SIZE);
	if (nicintel_control_bar == ERROR_PTR)
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
	pci_rmmio_writew(0x0001, nicintel_control_bar + CSR_FCR);

	max_rom_decode.parallel = NICINTEL_MEMMAP_SIZE;
	register_par_master(&par_master_nicintel, BUS_PARALLEL);

	return 0;
}

static void nicintel_chip_writeb(const struct flashctx *flash, uint8_t val,
				 chipaddr addr)
{
	pci_mmio_writeb(val, nicintel_bar + (addr & NICINTEL_MEMMAP_MASK));
}

static uint8_t nicintel_chip_readb(const struct flashctx *flash,
				   const chipaddr addr)
{
	return pci_mmio_readb(nicintel_bar + (addr & NICINTEL_MEMMAP_MASK));
}
