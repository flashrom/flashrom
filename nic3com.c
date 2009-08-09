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

#define BIOS_ROM_ADDR		0x04
#define BIOS_ROM_DATA		0x08
#define INT_STATUS		0x0e
#define INTERNAL_CONFIG		0x00
#define SELECT_REG_WINDOW	0x800

#define PCI_VENDOR_ID_3COM	0x10b7

uint32_t internal_conf;
uint16_t id;

struct pcidev_status nics_3com[] = {
	/* 3C90xB */
	{0x10b7, 0x9055, PCI_OK, "3COM", "3C90xB: PCI 10/100 Mbps; shared 10BASE-T/100BASE-TX"},
	{0x10b7, 0x9001, PCI_NT, "3COM", "3C90xB: PCI 10/100 Mbps; shared 10BASE-T/100BASE-T4" },
	{0x10b7, 0x9004, PCI_NT, "3COM", "3C90xB: PCI 10BASE-T (TPO)" },
	{0x10b7, 0x9005, PCI_NT, "3COM", "3C90xB: PCI 10BASE-T/10BASE2/AUI (COMBO)" },
	{0x10b7, 0x9006, PCI_NT, "3COM", "3C90xB: PCI 10BASE-T/10BASE2 (TPC)" },
	{0x10b7, 0x900a, PCI_NT, "3COM", "3C90xB: PCI 10BASE-FL" },
	{0x10b7, 0x905a, PCI_NT, "3COM", "3C90xB: PCI 10BASE-FX" },
	{0x10b7, 0x9058, PCI_OK, "3COM", "3C905B: Cyclone 10/100/BNC" },

	/* 3C905C */
	{0x10b7, 0x9200, PCI_OK, "3COM", "3C905C: EtherLink 10/100 PCI (TX)" },

	/* 3C980C */
	{0x10b7, 0x9805, PCI_NT, "3COM", "3C980C: EtherLink Server 10/100 PCI (TX)" },

	{},
};

int nic3com_init(void)
{
	get_io_perms();

	io_base_addr = pcidev_init(PCI_VENDOR_ID_3COM, nics_3com);
	id = pcidev_dev->device_id;

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

	buses_supported = CHIP_BUSTYPE_PARALLEL;

	return 0;
}

int nic3com_shutdown(void)
{
	/* 3COM 3C90xB cards need a special fixup. */
	if (id == 0x9055 || id == 0x9001 || id == 0x9004 || id == 0x9005
	    || id == 0x9006 || id == 0x900a || id == 0x905a || id == 0x9058) {
		/* Select register window 3 and restore the receiver status. */
		OUTW(SELECT_REG_WINDOW + 3, io_base_addr + INT_STATUS);
		OUTL(internal_conf, io_base_addr + INTERNAL_CONFIG);
	}

	free(pcidev_bdf);
	pci_cleanup(pacc);
	release_io_perms();
	return 0;
}

void nic3com_chip_writeb(uint8_t val, chipaddr addr)
{
	OUTL((uint32_t)addr, io_base_addr + BIOS_ROM_ADDR);
	OUTB(val, io_base_addr + BIOS_ROM_DATA);
}

uint8_t nic3com_chip_readb(const chipaddr addr)
{
	OUTL((uint32_t)addr, io_base_addr + BIOS_ROM_ADDR);
	return INB(io_base_addr + BIOS_ROM_DATA);
}
