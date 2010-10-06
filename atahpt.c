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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdlib.h>
#include <string.h>
#include "flash.h"
#include "programmer.h"

#define BIOS_ROM_ADDR		0x90
#define BIOS_ROM_DATA		0x94

#define REG_FLASH_ACCESS	0x58

#define PCI_VENDOR_ID_HPT	0x1103

struct pcidev_status ata_hpt[] = {
	{0x1103, 0x0004, NT, "Highpoint", "HPT366/368/370/370A/372/372N"},
	{0x1103, 0x0005, NT, "Highpoint", "HPT372A/372N"},
	{0x1103, 0x0006, NT, "Highpoint", "HPT302/302N"},

	{},
};

int atahpt_init(void)
{
	uint32_t reg32;

	get_io_perms();

	io_base_addr = pcidev_init(PCI_VENDOR_ID_HPT, PCI_BASE_ADDRESS_4,
				   ata_hpt);

	/* Enable flash access. */
	reg32 = pci_read_long(pcidev_dev, REG_FLASH_ACCESS);
	reg32 |= (1 << 24);
	pci_write_long(pcidev_dev, REG_FLASH_ACCESS, reg32);

	buses_supported = CHIP_BUSTYPE_PARALLEL;

	return 0;
}

int atahpt_shutdown(void)
{
	uint32_t reg32;

	/* Disable flash access again. */
	reg32 = pci_read_long(pcidev_dev, REG_FLASH_ACCESS);
	reg32 &= ~(1 << 24);
	pci_write_long(pcidev_dev, REG_FLASH_ACCESS, reg32);

	pci_cleanup(pacc);
	release_io_perms();
	return 0;
}

void atahpt_chip_writeb(uint8_t val, chipaddr addr)
{
	OUTL((uint32_t)addr, io_base_addr + BIOS_ROM_ADDR);
	OUTB(val, io_base_addr + BIOS_ROM_DATA);
}

uint8_t atahpt_chip_readb(const chipaddr addr)
{
	OUTL((uint32_t)addr, io_base_addr + BIOS_ROM_ADDR);
	return INB(io_base_addr + BIOS_ROM_DATA);
}
