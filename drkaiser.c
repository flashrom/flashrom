/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Joerg Fischer <turboj@web.de>
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
#include "flash.h"
#include "programmer.h"

#define PCI_VENDOR_ID_DRKAISER		0x1803

#define PCI_MAGIC_DRKAISER_ADDR		0x50
#define PCI_MAGIC_DRKAISER_VALUE	0xa971

#define DRKAISER_MEMMAP_SIZE           (1024 * 128)

/* Mask to restrict flash accesses to the 128kB memory window. */
#define DRKAISER_MEMMAP_MASK		((1 << 17) - 1)

const struct pcidev_status drkaiser_pcidev[] = {
	{0x1803, 0x5057, OK, "Dr. Kaiser", "PC-Waechter (Actel FPGA)"},
	{},
};

static uint8_t *drkaiser_bar;

static int drkaiser_shutdown(void *data)
{
	physunmap(drkaiser_bar, DRKAISER_MEMMAP_SIZE);
	/* Flash write is disabled automatically by PCI restore. */
	pci_cleanup(pacc);
	release_io_perms();
	return 0;
};

int drkaiser_init(void)
{
	uint32_t addr;

	get_io_perms();

	addr = pcidev_init(PCI_BASE_ADDRESS_2, drkaiser_pcidev);

	/* Write magic register to enable flash write. */
	rpci_write_word(pcidev_dev, PCI_MAGIC_DRKAISER_ADDR,
		       PCI_MAGIC_DRKAISER_VALUE);

	/* Map 128kB flash memory window. */
	drkaiser_bar = physmap("Dr. Kaiser PC-Waechter flash memory",
			       addr, DRKAISER_MEMMAP_SIZE);

	buses_supported = CHIP_BUSTYPE_PARALLEL;

	if (register_shutdown(drkaiser_shutdown, NULL))
		return 1;
	return 0;
}

void drkaiser_chip_writeb(uint8_t val, chipaddr addr)
{
	pci_mmio_writeb(val, drkaiser_bar + (addr & DRKAISER_MEMMAP_MASK));
}

uint8_t drkaiser_chip_readb(const chipaddr addr)
{
	return pci_mmio_readb(drkaiser_bar + (addr & DRKAISER_MEMMAP_MASK));
}
