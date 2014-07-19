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

#define PCI_VENDOR_ID_NATSEMI	0x100b

#define BOOT_ROM_ADDR		0x50
#define BOOT_ROM_DATA		0x54

static uint32_t io_base_addr = 0;
const struct dev_entry nics_natsemi[] = {
	{0x100b, 0x0020, NT, "National Semiconductor", "DP83815/DP83816"},
	{0x100b, 0x0022, NT, "National Semiconductor", "DP83820"},

	{0},
};

static void nicnatsemi_chip_writeb(const struct flashctx *flash, uint8_t val,
				   chipaddr addr);
static uint8_t nicnatsemi_chip_readb(const struct flashctx *flash,
				     const chipaddr addr);
static const struct par_master par_master_nicnatsemi = {
		.chip_readb		= nicnatsemi_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= fallback_chip_readn,
		.chip_writeb		= nicnatsemi_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
};

int nicnatsemi_init(void)
{
	struct pci_dev *dev = NULL;

	if (rget_io_perms())
		return 1;

	dev = pcidev_init(nics_natsemi, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!io_base_addr)
		return 1;

	/* The datasheet shows address lines MA0-MA16 in one place and MA0-MA15
	 * in another. My NIC has MA16 connected to A16 on the boot ROM socket
	 * so I'm assuming it is accessible. If not then next line wants to be
	 * max_rom_decode.parallel = 65536; and the mask in the read/write
	 * functions below wants to be 0x0000FFFF.
	 */
	max_rom_decode.parallel = 131072;
	register_par_master(&par_master_nicnatsemi, BUS_PARALLEL);

	return 0;
}

static void nicnatsemi_chip_writeb(const struct flashctx *flash, uint8_t val,
				   chipaddr addr)
{
	OUTL((uint32_t)addr & 0x0001FFFF, io_base_addr + BOOT_ROM_ADDR);
	/*
	 * The datasheet requires 32 bit accesses to this register, but it seems
	 * that requirement might only apply if the register is memory mapped.
	 * Bits 8-31 of this register are apparently don't care, and if this
	 * register is I/O port mapped, 8 bit accesses to the lowest byte of the
	 * register seem to work fine. Due to that, we ignore the advice in the
	 * data sheet.
	 */
	OUTB(val, io_base_addr + BOOT_ROM_DATA);
}

static uint8_t nicnatsemi_chip_readb(const struct flashctx *flash,
				     const chipaddr addr)
{
	OUTL(((uint32_t)addr & 0x0001FFFF), io_base_addr + BOOT_ROM_ADDR);
	/*
	 * The datasheet requires 32 bit accesses to this register, but it seems
	 * that requirement might only apply if the register is memory mapped.
	 * Bits 8-31 of this register are apparently don't care, and if this
	 * register is I/O port mapped, 8 bit accesses to the lowest byte of the
	 * register seem to work fine. Due to that, we ignore the advice in the
	 * data sheet.
	 */
	return INB(io_base_addr + BOOT_ROM_DATA);
}

#else
#error PCI port I/O access is not supported on this architecture yet.
#endif
