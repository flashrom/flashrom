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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "flash.h"

#define BIOS_ROM_ADDR		0x04
#define BIOS_ROM_DATA		0x08
#define INT_STATUS		0x0e
#define SELECT_REG_WINDOW	0x800

#define PCI_VENDOR_ID_3COM	0x10b7

struct pcidev_status nics_3com[] = {
	/* 3C90xB */
	{0x10b7, 0x9055, PCI_NT, "3COM", "3C90xB: PCI 10/100 Mbps; shared 10BASE-T/100BASE-TX"},
	{0x10b7, 0x9001, PCI_NT, "3COM", "3C90xB: PCI 10/100 Mbps; shared 10BASE-T/100BASE-T4" },
	{0x10b7, 0x9004, PCI_NT, "3COM", "3C90xB: PCI 10BASE-T (TPO)" },
	{0x10b7, 0x9005, PCI_NT, "3COM", "3C90xB: PCI 10BASE-T/10BASE2/AUI (COMBO)" },
	{0x10b7, 0x9006, PCI_NT, "3COM", "3C90xB: PCI 10BASE-T/10BASE2 (TPC)" },
	{0x10b7, 0x900a, PCI_NT, "3COM", "3C90xB: PCI 10BASE-FL" },
	{0x10b7, 0x905a, PCI_NT, "3COM", "3C90xB: PCI 10BASE-FX" },

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

	/*
	 * The lowest 16 bytes of the I/O mapped register space of (most) 3COM
	 * cards form a 'register window' into one of multiple (usually 8)
	 * register banks. For 3C90xB/3C90xC we need register window/bank 0.
	 */
	OUTW(SELECT_REG_WINDOW + 0, io_base_addr + INT_STATUS);

	return 0;
}

int nic3com_shutdown(void)
{
	free(pcidev_bdf);
	pci_cleanup(pacc);
#if defined(__FreeBSD__) || defined(__DragonFly__)
	close(io_fd);
#endif
	return 0;
}

void *nic3com_map(const char *descr, unsigned long phys_addr, size_t len)
{
	return 0;
}

void nic3com_unmap(void *virt_addr, size_t len)
{
}

void nic3com_chip_writeb(uint8_t val, volatile void *addr)
{
	OUTL((uint32_t)(intptr_t)addr, io_base_addr + BIOS_ROM_ADDR);
	OUTB(val, io_base_addr + BIOS_ROM_DATA);
}

uint8_t nic3com_chip_readb(const volatile void *addr)
{
	uint8_t val;

	OUTL((uint32_t)(intptr_t)addr, io_base_addr + BIOS_ROM_ADDR);
	val = INB(io_base_addr + BIOS_ROM_DATA);

	return val;
}
