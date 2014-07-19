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
#include "hwaccess.h"

#define PCI_VENDOR_ID_REALTEK	0x10ec
#define PCI_VENDOR_ID_SMC1211	0x1113

static uint32_t io_base_addr = 0;
static int bios_rom_addr, bios_rom_data;

const struct dev_entry nics_realtek[] = {
	{0x10ec, 0x8139, OK, "Realtek", "RTL8139/8139C/8139C+"},
	{0x10ec, 0x8169, NT, "Realtek", "RTL8169"},
	{0x1113, 0x1211, OK, "SMC", "1211TX"}, /* RTL8139 clone */

	{0},
};

static void nicrealtek_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr);
static uint8_t nicrealtek_chip_readb(const struct flashctx *flash, const chipaddr addr);
static const struct par_master par_master_nicrealtek = {
		.chip_readb		= nicrealtek_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= fallback_chip_readn,
		.chip_writeb		= nicrealtek_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
};

static int nicrealtek_shutdown(void *data)
{
	/* FIXME: We forgot to disable software access again. */
	return 0;
}

int nicrealtek_init(void)
{
	struct pci_dev *dev = NULL;

	if (rget_io_perms())
		return 1;

	dev = pcidev_init(nics_realtek, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!io_base_addr)
		return 1;

	/* Beware, this ignores the vendor ID! */
	switch (dev->device_id) {
	case 0x8139: /* RTL8139 */
	case 0x1211: /* SMC 1211TX */
	default:
		bios_rom_addr = 0xD4;
		bios_rom_data = 0xD7;
		break;
	case 0x8169: /* RTL8169 */
		bios_rom_addr = 0x30;
		bios_rom_data = 0x33;
		break;
	}

	if (register_shutdown(nicrealtek_shutdown, NULL))
		return 1;

	register_par_master(&par_master_nicrealtek, BUS_PARALLEL);

	return 0;
}

static void nicrealtek_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr)
{
	/* Output addr and data, set WE to 0, set OE to 1, set CS to 0,
	 * enable software access.
	 */
	OUTL(((uint32_t)addr & 0x01FFFF) | 0x0A0000 | (val << 24),
	     io_base_addr + bios_rom_addr);
	/* Output addr and data, set WE to 1, set OE to 1, set CS to 1,
	 * enable software access.
	 */
	OUTL(((uint32_t)addr & 0x01FFFF) | 0x1E0000 | (val << 24),
	     io_base_addr + bios_rom_addr);
}

static uint8_t nicrealtek_chip_readb(const struct flashctx *flash, const chipaddr addr)
{
	uint8_t val;

	/* FIXME: Can we skip reading the old data and simply use 0? */
	/* Read old data. */
	val = INB(io_base_addr + bios_rom_data);
	/* Output new addr and old data, set WE to 1, set OE to 0, set CS to 0,
	 * enable software access.
	 */
	OUTL(((uint32_t)addr & 0x01FFFF) | 0x060000 | (val << 24),
	     io_base_addr + bios_rom_addr);

	/* Read new data. */
	val = INB(io_base_addr + bios_rom_data);
	/* Output addr and new data, set WE to 1, set OE to 1, set CS to 1,
	 * enable software access.
	 */
	OUTL(((uint32_t)addr & 0x01FFFF) | 0x1E0000 | (val << 24),
	     io_base_addr + bios_rom_addr);

	return val;
}

#else
#error PCI port I/O access is not supported on this architecture yet.
#endif
