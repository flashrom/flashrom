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
#include <string.h>
#include <sys/types.h>
#include "flash.h"

#define PCI_VENDOR_ID_DRKAISER		0x1803

#define PCI_MAGIC_DRKAISER_ADDR		0x50
#define PCI_MAGIC_DRKAISER_VALUE	0xa971

struct pcidev_status drkaiser_pcidev[] = {
	{0x1803, 0x5057, PCI_OK, "Dr. Kaiser", "PC-Waechter (Actel FPGA)"},
	{},
};

uint8_t *drkaiser_bar;

int drkaiser_init(void)
{
	uint32_t addr;

	get_io_perms();
	pcidev_init(PCI_VENDOR_ID_DRKAISER, PCI_BASE_ADDRESS_2,
		    drkaiser_pcidev, programmer_param);

	/* Write magic register to enable flash write. */
	pci_write_word(pcidev_dev, PCI_MAGIC_DRKAISER_ADDR,
		       PCI_MAGIC_DRKAISER_VALUE);

	/* TODO: Mask lower bits? How many? 3? 7? */
	addr = pci_read_long(pcidev_dev, PCI_BASE_ADDRESS_2) & ~0x03;

	/* Map 128KB flash memory window. */
	drkaiser_bar = physmap("Dr. Kaiser PC-Waechter flash memory",
			       addr, 128 * 1024);

	buses_supported = CHIP_BUSTYPE_PARALLEL;
	return 0;
}

int drkaiser_shutdown(void)
{
	/* Write protect the flash again. */
	pci_write_word(pcidev_dev, PCI_MAGIC_DRKAISER_ADDR, 0);
	free(programmer_param);
	pci_cleanup(pacc);
	release_io_perms();
	return 0;
};

void drkaiser_chip_writeb(uint8_t val, chipaddr addr)
{
	mmio_writeb(val, drkaiser_bar + addr);
}

uint8_t drkaiser_chip_readb(const chipaddr addr)
{
	return mmio_readb(drkaiser_bar + addr);
}
