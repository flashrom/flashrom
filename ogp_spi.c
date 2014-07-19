/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Mark Marshall
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

#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"

#define PCI_VENDOR_ID_OGP 0x1227

/* These are the register addresses for the OGD1 / OGA1.  If they are
 * different for later versions of the hardware then we will need
 * logic to select between the different hardware versions. */
#define OGA1_XP10_BPROM_SI			     0x0040 /*	W */
#define OGA1_XP10_BPROM_SO			     0x0040 /*	R */
#define OGA1_XP10_BPROM_CE_BAR			     0x0044 /*	W */
#define OGA1_XP10_BPROM_SCK			     0x0048 /*	W */
#define OGA1_XP10_BPROM_REG_SEL			     0x004C /*	W */
#define OGA1_XP10_CPROM_SI			     0x0050 /*	W */
#define OGA1_XP10_CPROM_SO			     0x0050 /*	R */
#define OGA1_XP10_CPROM_CE_BAR			     0x0054 /*	W */
#define OGA1_XP10_CPROM_SCK			     0x0058 /*	W */
#define OGA1_XP10_CPROM_REG_SEL			     0x005C /*	W */

static uint8_t *ogp_spibar;

static uint32_t ogp_reg_sel;
static uint32_t ogp_reg_siso;
static uint32_t ogp_reg__ce;
static uint32_t ogp_reg_sck;

const struct dev_entry ogp_spi[] = {
	{PCI_VENDOR_ID_OGP, 0x0000, OK, "Open Graphics Project", "Development Board OGD1"},

	{0},
};

static void ogp_request_spibus(void)
{
	pci_mmio_writel(1, ogp_spibar + ogp_reg_sel);
}

static void ogp_release_spibus(void)
{
	pci_mmio_writel(0, ogp_spibar + ogp_reg_sel);
}

static void ogp_bitbang_set_cs(int val)
{
	pci_mmio_writel(val, ogp_spibar + ogp_reg__ce);
}

static void ogp_bitbang_set_sck(int val)
{
	pci_mmio_writel(val, ogp_spibar + ogp_reg_sck);
}

static void ogp_bitbang_set_mosi(int val)
{
	pci_mmio_writel(val, ogp_spibar + ogp_reg_siso);
}

static int ogp_bitbang_get_miso(void)
{
	uint32_t tmp;

	tmp = pci_mmio_readl(ogp_spibar + ogp_reg_siso);
	return tmp & 0x1;
}

static const struct bitbang_spi_master bitbang_spi_master_ogp = {
	.type = BITBANG_SPI_MASTER_OGP,
	.set_cs = ogp_bitbang_set_cs,
	.set_sck = ogp_bitbang_set_sck,
	.set_mosi = ogp_bitbang_set_mosi,
	.get_miso = ogp_bitbang_get_miso,
	.request_bus = ogp_request_spibus,
	.release_bus = ogp_release_spibus,
	.half_period = 0,
};

int ogp_spi_init(void)
{
	struct pci_dev *dev = NULL;
	char *type;

	type = extract_programmer_param("rom");

	if (!type) {
		msg_perr("Please use flashrom -p ogp_spi:rom=... to specify "
			 "which flashchip you want to access.\n");
		return 1;
	} else if (!strcasecmp(type, "bprom") || !strcasecmp(type, "bios")) {
		ogp_reg_sel  = OGA1_XP10_BPROM_REG_SEL;
		ogp_reg_siso = OGA1_XP10_BPROM_SI;
		ogp_reg__ce  = OGA1_XP10_BPROM_CE_BAR;
		ogp_reg_sck  = OGA1_XP10_BPROM_SCK;
	} else if (!strcasecmp(type, "cprom") || !strcasecmp(type, "s3")) {
		ogp_reg_sel  = OGA1_XP10_CPROM_REG_SEL;
		ogp_reg_siso = OGA1_XP10_CPROM_SI;
		ogp_reg__ce  = OGA1_XP10_CPROM_CE_BAR;
		ogp_reg_sck  = OGA1_XP10_CPROM_SCK;
	} else {
		msg_perr("Invalid or missing rom= parameter.\n");
		free(type);
		return 1;
	}
	free(type);

	if (rget_io_perms())
		return 1;

	dev = pcidev_init(ogp_spi, PCI_BASE_ADDRESS_0);
	if (!dev)
		return 1;

	uint32_t io_base_addr = pcidev_readbar(dev, PCI_BASE_ADDRESS_0);
	if (!io_base_addr)
		return 1;

	ogp_spibar = rphysmap("OGP registers", io_base_addr, 4096);
	if (ogp_spibar == ERROR_PTR)
		return 1;

	if (register_spi_bitbang_master(&bitbang_spi_master_ogp))
		return 1;

	return 0;
}
