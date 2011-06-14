/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Joerg Fischer <turboj@gmx.de>
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

#if defined(__i386__) || defined(__x86_64__)

#include <stdlib.h>
#include "flash.h"
#include "programmer.h"

#define PCI_VENDOR_ID_REALTEK	0x10ec
#define PCI_VENDOR_ID_SMC1211	0x1113

#define BIOS_ROM_ADDR		0xD4
#define BIOS_ROM_DATA		0xD7

const struct pcidev_status nics_realtek[] = {
	{0x10ec, 0x8139, OK, "Realtek", "RTL8139/8139C/8139C+"},
	{0x1113, 0x1211, OK, "SMC2", "1211TX"}, /* RTL8139 clone */
	{},
};

static int nicrealtek_shutdown(void *data)
{
	/* FIXME: We forgot to disable software access again. */
	pci_cleanup(pacc);
	release_io_perms();
	return 0;
}

int nicrealtek_init(void)
{
	get_io_perms();

	io_base_addr = pcidev_init(PCI_BASE_ADDRESS_0, nics_realtek);

	buses_supported = CHIP_BUSTYPE_PARALLEL;

	if (register_shutdown(nicrealtek_shutdown, NULL))
		return 1;
	return 0;
}

void nicrealtek_chip_writeb(uint8_t val, chipaddr addr)
{
	/* Output addr and data, set WE to 0, set OE to 1, set CS to 0,
	 * enable software access.
	 */
	OUTL(((uint32_t)addr & 0x01FFFF) | 0x0A0000 | (val << 24),
	     io_base_addr + BIOS_ROM_ADDR);
	/* Output addr and data, set WE to 1, set OE to 1, set CS to 1,
	 * enable software access.
	 */
	OUTL(((uint32_t)addr & 0x01FFFF) | 0x1E0000 | (val << 24),
	     io_base_addr + BIOS_ROM_ADDR);
}

uint8_t nicrealtek_chip_readb(const chipaddr addr)
{
	uint8_t val;

	/* FIXME: Can we skip reading the old data and simply use 0? */
	/* Read old data. */
	val = INB(io_base_addr + BIOS_ROM_DATA);
	/* Output new addr and old data, set WE to 1, set OE to 0, set CS to 0,
	 * enable software access.
	 */
	OUTL(((uint32_t)addr & 0x01FFFF) | 0x060000 | (val << 24),
	     io_base_addr + BIOS_ROM_ADDR);

	/* Read new data. */
	val = INB(io_base_addr + BIOS_ROM_DATA);
	/* Output addr and new data, set WE to 1, set OE to 1, set CS to 1,
	 * enable software access.
	 */
	OUTL(((uint32_t)addr & 0x01FFFF) | 0x1E0000 | (val << 24),
	     io_base_addr + BIOS_ROM_ADDR);

	return val;
}

#else
#error PCI port I/O access is not supported on this architecture yet.
#endif
