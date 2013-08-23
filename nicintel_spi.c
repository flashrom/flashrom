/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Carl-Daniel Hailfinger
 * Copyright (C) 2010 Idwer Vollering
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

/*
 * Datasheet:
 * PCI/PCI-X Family of Gigabit Ethernet Controllers Software Developer's Manual
 * 82540EP/EM, 82541xx, 82544GC/EI, 82545GM/EM, 82546GB/EB, and 82547xx
 * http://download.intel.com/design/network/manuals/8254x_GBe_SDM.pdf
 */

#include <stdlib.h>
#include <unistd.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"

#define PCI_VENDOR_ID_INTEL 0x8086
#define MEMMAP_SIZE getpagesize()

/* EEPROM/Flash Control & Data Register */
#define EECD	0x10
/* Flash Access Register */
#define FLA	0x1c

/*
 * Register bits of EECD.
 * Table 13-6
 *
 * Bit 04, 05: FWE (Flash Write Enable Control)
 * 00b = not allowed
 * 01b = flash writes disabled
 * 10b = flash writes enabled
 * 11b = not allowed
 */
#define FLASH_WRITES_DISABLED	0x10 /* FWE: 10000b */
#define FLASH_WRITES_ENABLED	0x20 /* FWE: 100000b */

/* Flash Access register bits
 * Table 13-9
 */
#define FL_SCK	0
#define FL_CS	1
#define FL_SI	2
#define FL_SO	3
#define FL_REQ	4
#define FL_GNT	5
/* Currently unused */
// #define FL_BUSY	30
// #define FL_ER	31

uint8_t *nicintel_spibar;

const struct dev_entry nics_intel_spi[] = {
	{PCI_VENDOR_ID_INTEL, 0x105e, OK, "Intel", "82571EB Gigabit Ethernet Controller"},
	{PCI_VENDOR_ID_INTEL, 0x1076, OK, "Intel", "82541GI Gigabit Ethernet Controller"},
	{PCI_VENDOR_ID_INTEL, 0x107c, OK, "Intel", "82541PI Gigabit Ethernet Controller"},
	{PCI_VENDOR_ID_INTEL, 0x10b9, OK, "Intel", "82572EI Gigabit Ethernet Controller"},

	{0},
};

static void nicintel_request_spibus(void)
{
	uint32_t tmp;

	tmp = pci_mmio_readl(nicintel_spibar + FLA);
	tmp |= 1 << FL_REQ;
	pci_mmio_writel(tmp, nicintel_spibar + FLA);

	/* Wait until we are allowed to use the SPI bus. */
	while (!(pci_mmio_readl(nicintel_spibar + FLA) & (1 << FL_GNT))) ;
}

static void nicintel_release_spibus(void)
{
	uint32_t tmp;

	tmp = pci_mmio_readl(nicintel_spibar + FLA);
	tmp &= ~(1 << FL_REQ);
	pci_mmio_writel(tmp, nicintel_spibar + FLA);
}

static void nicintel_bitbang_set_cs(int val)
{
	uint32_t tmp;

	tmp = pci_mmio_readl(nicintel_spibar + FLA);
	tmp &= ~(1 << FL_CS);
	tmp |= (val << FL_CS);
	pci_mmio_writel(tmp,  nicintel_spibar + FLA);
}

static void nicintel_bitbang_set_sck(int val)
{
	uint32_t tmp;

	tmp = pci_mmio_readl(nicintel_spibar + FLA);
	tmp &= ~(1 << FL_SCK);
	tmp |= (val << FL_SCK);
	pci_mmio_writel(tmp, nicintel_spibar + FLA);
}

static void nicintel_bitbang_set_mosi(int val)
{
	uint32_t tmp;

	tmp = pci_mmio_readl(nicintel_spibar + FLA);
	tmp &= ~(1 << FL_SI);
	tmp |= (val << FL_SI);
	pci_mmio_writel(tmp, nicintel_spibar + FLA);
}

static int nicintel_bitbang_get_miso(void)
{
	uint32_t tmp;

	tmp = pci_mmio_readl(nicintel_spibar + FLA);
	tmp = (tmp >> FL_SO) & 0x1;
	return tmp;
}

static const struct bitbang_spi_master bitbang_spi_master_nicintel = {
	.type = BITBANG_SPI_MASTER_NICINTEL,
	.set_cs = nicintel_bitbang_set_cs,
	.set_sck = nicintel_bitbang_set_sck,
	.set_mosi = nicintel_bitbang_set_mosi,
	.get_miso = nicintel_bitbang_get_miso,
	.request_bus = nicintel_request_spibus,
	.release_bus = nicintel_release_spibus,
	.half_period = 1,
};

static int nicintel_spi_shutdown(void *data)
{
	uint32_t tmp;

	/* Disable writes manually. See the comment about EECD in nicintel_spi_init() for details. */
	tmp = pci_mmio_readl(nicintel_spibar + EECD);
	tmp &= ~FLASH_WRITES_ENABLED;
	tmp |= FLASH_WRITES_DISABLED;
	pci_mmio_writel(tmp, nicintel_spibar + EECD);

	return 0;
}

int nicintel_spi_init(void)
{
	struct pci_dev *dev = NULL;
	uint32_t tmp;

	if (rget_io_perms())
		return 1;

	dev = pcidev_init(nics_intel_spi, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!io_base_addr)
		return 1;

	nicintel_spibar = rphysmap("Intel Gigabit NIC w/ SPI flash", io_base_addr, MEMMAP_SIZE);
	/* Automatic restore of EECD on shutdown is not possible because EECD
	 * does not only contain FLASH_WRITES_DISABLED|FLASH_WRITES_ENABLED,
	 * but other bits with side effects as well. Those other bits must be
	 * left untouched.
	 */
	tmp = pci_mmio_readl(nicintel_spibar + EECD);
	tmp &= ~FLASH_WRITES_DISABLED;
	tmp |= FLASH_WRITES_ENABLED;
	pci_mmio_writel(tmp, nicintel_spibar + EECD);

	/* test if FWE is really set to allow writes */
	tmp = pci_mmio_readl(nicintel_spibar + EECD);
	if ( (tmp & FLASH_WRITES_DISABLED) || !(tmp & FLASH_WRITES_ENABLED) ) {
		msg_perr("Enabling flash write access failed.\n");
		return 1;
	}

	if (register_shutdown(nicintel_spi_shutdown, NULL))
		return 1;

	if (bitbang_spi_init(&bitbang_spi_master_nicintel))
		return 1;

	return 0;
}
