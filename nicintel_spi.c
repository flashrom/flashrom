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
 */

/*
 * Datasheets:
 * PCI/PCI-X Family of Gigabit Ethernet Controllers Software Developer's Manual
 * 82540EP/EM, 82541xx, 82544GC/EI, 82545GM/EM, 82546GB/EB, and 82547xx
 * http://www.intel.com/content/www/us/en/ethernet-controllers/pci-pci-x-family-gbe-controllers-software-dev-manual.html
 *
 * PCIe GbE Controllers Open Source Software Developer's Manual
 * http://www.intel.com/content/www/us/en/ethernet-controllers/pcie-gbe-controllers-open-source-manual.html
 *
 * Intel 82574 Gigabit Ethernet Controller Family Datasheet
 * http://www.intel.com/content/www/us/en/ethernet-controllers/82574l-gbe-controller-datasheet.html
 *
 * Intel 82599 10 GbE Controller Datasheet (331520)
 * http://www.intel.com/content/dam/www/public/us/en/documents/datasheets/82599-10-gbe-controller-datasheet.pdf
 */

#include <stdlib.h>
#include <unistd.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess_physmap.h"
#include "platform/pci.h"

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
 * 00b = not allowed (on some cards this sends an erase command if bit 31 (FL_ER) of FLA is set)
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
#define FL_LOCKED  6
#define FL_ABORT   7
#define FL_CLR_ERR 8
/* Currently unused */
// #define FL_BUSY	30
// #define FL_ER	31

struct nicintel_spi_data {
	uint8_t *spibar;
};

static const struct dev_entry nics_intel_spi[] = {
	{PCI_VENDOR_ID_INTEL, 0x105e, OK, "Intel", "82571EB Gigabit Ethernet Controller"},
	{PCI_VENDOR_ID_INTEL, 0x1076, OK, "Intel", "82541GI Gigabit Ethernet Controller"},
	{PCI_VENDOR_ID_INTEL, 0x107c, OK, "Intel", "82541PI Gigabit Ethernet Controller"},
	{PCI_VENDOR_ID_INTEL, 0x10b9, OK, "Intel", "82572EI Gigabit Ethernet Controller"},
	{PCI_VENDOR_ID_INTEL, 0x10d3, OK, "Intel", "82574L Gigabit Ethernet Controller"},

	{PCI_VENDOR_ID_INTEL, 0x10d8, NT, "Intel", "82599 10 Gigabit Unprogrammed Network Controller"},
	{PCI_VENDOR_ID_INTEL, 0x10f7, NT, "Intel", "82599 10 Gigabit KX4 Dual Port Network Controller"},
	{PCI_VENDOR_ID_INTEL, 0x10f8, NT, "Intel", "82599 10 Gigabit Dual Port Backplane Controller"},
	{PCI_VENDOR_ID_INTEL, 0x10f9, NT, "Intel", "82599 10 Gigabit CX4 Dual Port Network Controller"},
	{PCI_VENDOR_ID_INTEL, 0x10fb, NT, "Intel", "82599 10-Gigabit SFI/SFP+ Network Controller"},
	{PCI_VENDOR_ID_INTEL, 0x10fc, OK, "Intel", "82599 10 Gigabit XAUI/BX4 Dual Port Network Controller"},
	{PCI_VENDOR_ID_INTEL, 0x1517, NT, "Intel", "82599 10 Gigabit KR Network Controller"},
	{PCI_VENDOR_ID_INTEL, 0x151c, NT, "Intel", "82599 10 Gigabit TN Network Controller"},
	{PCI_VENDOR_ID_INTEL, 0x1529, NT, "Intel", "82599 10 Gigabit Dual Port Network Controller with FCoE"},
	{PCI_VENDOR_ID_INTEL, 0x152a, NT, "Intel", "82599 10 Gigabit Dual Port Backplane Controller with FCoE"},
	{PCI_VENDOR_ID_INTEL, 0x1557, NT, "Intel", "82599 10 Gigabit SFI Network Controller"},

	{PCI_VENDOR_ID_INTEL, 0x1531, OK, "Intel", "I210 Gigabit Network Connection Unprogrammed"},
	{PCI_VENDOR_ID_INTEL, 0x1532, NT, "Intel", "I211 Gigabit Network Connection Unprogrammed"},
	{PCI_VENDOR_ID_INTEL, 0x1533, NT, "Intel", "I210 Gigabit Network Connection"},
	{PCI_VENDOR_ID_INTEL, 0x1536, NT, "Intel", "I210 Gigabit Network Connection SERDES Fiber"},
	{PCI_VENDOR_ID_INTEL, 0x1537, NT, "Intel", "I210 Gigabit Network Connection SERDES Backplane"},
	{PCI_VENDOR_ID_INTEL, 0x1538, NT, "Intel", "I210 Gigabit Network Connection SGMII"},
	{PCI_VENDOR_ID_INTEL, 0x1539, NT, "Intel", "I211 Gigabit Network Connection"},

	{0},
};

static void nicintel_request_spibus(void *spi_data)
{
	struct nicintel_spi_data *data = spi_data;
	uint32_t tmp;

	tmp = pci_mmio_readl(data->spibar + FLA);
	tmp |= BIT(FL_REQ);
	pci_mmio_writel(tmp, data->spibar + FLA);

	/* Wait until we are allowed to use the SPI bus. */
	while (!(pci_mmio_readl(data->spibar + FLA) & BIT(FL_GNT))) ;
}

static void nicintel_release_spibus(void *spi_data)
{
	struct nicintel_spi_data *data = spi_data;
	uint32_t tmp;

	tmp = pci_mmio_readl(data->spibar + FLA);
	tmp &= ~BIT(FL_REQ);
	pci_mmio_writel(tmp, data->spibar + FLA);
}

static void nicintel_bitbang_set_cs(int val, void *spi_data)
{
	struct nicintel_spi_data *data = spi_data;
	uint32_t tmp;

	tmp = pci_mmio_readl(data->spibar + FLA);
	tmp &= ~BIT(FL_CS);
	tmp |= (val << FL_CS);
	pci_mmio_writel(tmp, data->spibar + FLA);
}

static void nicintel_bitbang_set_sck(int val, void *spi_data)
{
	struct nicintel_spi_data *data = spi_data;
	uint32_t tmp;

	tmp = pci_mmio_readl(data->spibar + FLA);
	tmp &= ~BIT(FL_SCK);
	tmp |= (val << FL_SCK);
	pci_mmio_writel(tmp, data->spibar + FLA);
}

static void nicintel_bitbang_set_mosi(int val, void *spi_data)
{
	struct nicintel_spi_data *data = spi_data;
	uint32_t tmp;

	tmp = pci_mmio_readl(data->spibar + FLA);
	tmp &= ~BIT(FL_SI);
	tmp |= (val << FL_SI);
	pci_mmio_writel(tmp, data->spibar + FLA);
}

static void nicintel_bitbang_set_sck_set_mosi(int sck, int mosi, void *spi_data)
{
	struct nicintel_spi_data *data = spi_data;
	uint32_t tmp;

	tmp = pci_mmio_readl(data->spibar + FLA);
	tmp &= ~BIT(FL_SCK);
	tmp &= ~BIT(FL_SI);
	tmp |= (sck << FL_SCK);
	tmp |= (mosi << FL_SI);
	pci_mmio_writel(tmp, data->spibar + FLA);
}

static int nicintel_bitbang_get_miso(void *spi_data)
{
	struct nicintel_spi_data *data = spi_data;
	uint32_t tmp;

	tmp = pci_mmio_readl(data->spibar + FLA);
	tmp = (tmp >> FL_SO) & 0x1;
	return tmp;
}

static int nicintel_bitbang_set_sck_get_miso(int sck, void *spi_data)
{
	struct nicintel_spi_data *data = spi_data;
	uint32_t tmp;

	tmp = pci_mmio_readl(data->spibar + FLA);
	tmp &= ~BIT(FL_SCK);
	tmp |= (sck << FL_SCK);
	pci_mmio_writel(tmp, data->spibar + FLA);
	return (tmp >> FL_SO) & 0x1;
}

static const struct bitbang_spi_master bitbang_spi_master_nicintel = {
	.set_cs			= nicintel_bitbang_set_cs,
	.set_sck		= nicintel_bitbang_set_sck,
	.set_mosi		= nicintel_bitbang_set_mosi,
	.set_sck_set_mosi	= nicintel_bitbang_set_sck_set_mosi,
	.set_sck_get_miso	= nicintel_bitbang_set_sck_get_miso,
	.get_miso		= nicintel_bitbang_get_miso,
	.request_bus		= nicintel_request_spibus,
	.release_bus		= nicintel_release_spibus,
	.half_period		= 1,
};

static int nicintel_spi_shutdown(void *spi_data)
{
	struct nicintel_spi_data *data = spi_data;
	uint32_t tmp;

	/* Disable writes manually. See the comment about EECD in nicintel_spi_init() for details. */
	tmp = pci_mmio_readl(data->spibar + EECD);
	tmp &= ~FLASH_WRITES_ENABLED;
	tmp |= FLASH_WRITES_DISABLED;
	pci_mmio_writel(tmp, data->spibar + EECD);

	free(data);
	return 0;
}

static int nicintel_spi_82599_enable_flash(struct nicintel_spi_data *data)
{
	uint32_t tmp;

	/* Automatic restore of EECD on shutdown is not possible because EECD
	 * does not only contain FLASH_WRITES_DISABLED|FLASH_WRITES_ENABLED,
	 * but other bits with side effects as well. Those other bits must be
	 * left untouched.
	 */
	tmp = pci_mmio_readl(data->spibar + EECD);
	tmp &= ~FLASH_WRITES_DISABLED;
	tmp |= FLASH_WRITES_ENABLED;
	pci_mmio_writel(tmp, data->spibar + EECD);

	/* test if FWE is really set to allow writes */
	tmp = pci_mmio_readl(data->spibar + EECD);
	if ( (tmp & FLASH_WRITES_DISABLED) || !(tmp & FLASH_WRITES_ENABLED) ) {
		msg_perr("Enabling flash write access failed.\n");
		return 1;
	}

	if (register_shutdown(nicintel_spi_shutdown, data))
		return 1;

	return 0;
}

static int nicintel_spi_i210_shutdown(void *data)
{
	free(data);
	return 0;
}

static int nicintel_spi_i210_enable_flash(struct nicintel_spi_data *data)
{
	uint32_t tmp;

	tmp = pci_mmio_readl(data->spibar + FLA);
	if (tmp & BIT(FL_LOCKED)) {
		msg_perr("Flash is in Secure Mode. Abort.\n");
		return 1;
	}

	if (tmp & BIT(FL_ABORT)) {
		tmp |= BIT(FL_CLR_ERR);
		pci_mmio_writel(tmp, data->spibar + FLA);
		tmp = pci_mmio_readl(data->spibar + FLA);
		if (!(tmp & BIT(FL_ABORT))) {
			msg_perr("Unable to clear Flash Access Error. Abort\n");
			return 1;
		}
	}

	if (register_shutdown(nicintel_spi_i210_shutdown, data))
		return 1;

	return 0;
}

static int nicintel_spi_init(const struct programmer_cfg *cfg)
{
	struct pci_dev *dev = NULL;

	dev = pcidev_init(cfg, nics_intel_spi, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	uint32_t io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!io_base_addr)
		return 1;

	struct nicintel_spi_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for SPI master data\n");
		return 1;
	}

	if ((dev->device_id & 0xfff0) == 0x1530) {
		data->spibar = rphysmap("Intel I210 Gigabit w/ SPI flash", io_base_addr + 0x12000,
					   MEMMAP_SIZE);
		if (!data->spibar || nicintel_spi_i210_enable_flash(data)) {
				free(data);
				return 1;
		}
	} else if (dev->device_id < 0x10d8) {
		data->spibar = rphysmap("Intel Gigabit NIC w/ SPI flash", io_base_addr,
					   MEMMAP_SIZE);
		if (!data->spibar || nicintel_spi_82599_enable_flash(data)) {
				free(data);
				return 1;
		}
	} else {
		data->spibar = rphysmap("Intel 10 Gigabit NIC w/ SPI flash", io_base_addr + 0x10000,
					   MEMMAP_SIZE);
		if (!data->spibar || nicintel_spi_82599_enable_flash(data)) {
				free(data);
				return 1;
		}
	}

	if (register_spi_bitbang_master(&bitbang_spi_master_nicintel, data))
		return 1; /* shutdown function does cleanup */

	return 0;
}

const struct programmer_entry programmer_nicintel_spi = {
	.name			= "nicintel_spi",
	.type			= PCI,
	.devs.dev		= nics_intel_spi,
	.init			= nicintel_spi_init,
};
