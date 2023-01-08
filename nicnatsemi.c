/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Andrew Morgan <ziltro@ziltro.com>
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
 */

#include <stdlib.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess_x86_io.h"
#include "platform/pci.h"

#define PCI_VENDOR_ID_NATSEMI	0x100b

#define BOOT_ROM_ADDR		0x50
#define BOOT_ROM_DATA		0x54

struct nicnatsemi_data {
	uint32_t io_base_addr;
};

static const struct dev_entry nics_natsemi[] = {
	{0x100b, 0x0020, NT, "National Semiconductor", "DP83815/DP83816"},
	{0x100b, 0x0022, NT, "National Semiconductor", "DP83820"},

	{0},
};

static void nicnatsemi_chip_writeb(const struct flashctx *flash, uint8_t val,
				   chipaddr addr)
{
	const struct nicnatsemi_data *data = flash->mst->par.data;

	OUTL((uint32_t)addr & 0x0001FFFF, data->io_base_addr + BOOT_ROM_ADDR);
	/*
	 * The datasheet requires 32 bit accesses to this register, but it seems
	 * that requirement might only apply if the register is memory mapped.
	 * Bits 8-31 of this register are apparently don't care, and if this
	 * register is I/O port mapped, 8 bit accesses to the lowest byte of the
	 * register seem to work fine. Due to that, we ignore the advice in the
	 * data sheet.
	 */
	OUTB(val, data->io_base_addr + BOOT_ROM_DATA);
}

static uint8_t nicnatsemi_chip_readb(const struct flashctx *flash,
				     const chipaddr addr)
{
	const struct nicnatsemi_data *data = flash->mst->par.data;

	OUTL(((uint32_t)addr & 0x0001FFFF), data->io_base_addr + BOOT_ROM_ADDR);
	/*
	 * The datasheet requires 32 bit accesses to this register, but it seems
	 * that requirement might only apply if the register is memory mapped.
	 * Bits 8-31 of this register are apparently don't care, and if this
	 * register is I/O port mapped, 8 bit accesses to the lowest byte of the
	 * register seem to work fine. Due to that, we ignore the advice in the
	 * data sheet.
	 */
	return INB(data->io_base_addr + BOOT_ROM_DATA);
}

static int nicnatsemi_shutdown(void *par_data)
{
	free(par_data);
	return 0;
}

static const struct par_master par_master_nicnatsemi = {
	.chip_readb	= nicnatsemi_chip_readb,
	.chip_writeb	= nicnatsemi_chip_writeb,
	.shutdown	= nicnatsemi_shutdown,
};

static int nicnatsemi_init(const struct programmer_cfg *cfg)
{
	struct pci_dev *dev = NULL;
	uint32_t io_base_addr;

	if (rget_io_perms())
		return 1;

	dev = pcidev_init(cfg, nics_natsemi, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!io_base_addr)
		return 1;

	struct nicnatsemi_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for PAR master data\n");
		return 1;
	}
	data->io_base_addr = io_base_addr;

	/* The datasheet shows address lines MA0-MA16 in one place and MA0-MA15
	 * in another. My NIC has MA16 connected to A16 on the boot ROM socket
	 * so I'm assuming it is accessible. If not then next line wants to be
	 * max_rom_decode.parallel = 65536; and the mask in the read/write
	 * functions below wants to be 0x0000FFFF.
	 */
	max_rom_decode.parallel = 131072;
	return register_par_master(&par_master_nicnatsemi, BUS_PARALLEL, data);
}


const struct programmer_entry programmer_nicnatsemi = {
	.name			= "nicnatsemi",
	.type			= PCI,
	.devs.dev		= nics_natsemi,
	.init			= nicnatsemi_init,
};
