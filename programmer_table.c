/*
 * This file is part of the flashrom project.
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

#include "programmer.h"

const struct programmer_entry programmer_table[] = {
#if CONFIG_INTERNAL == 1
	{
		.name			= "internal",
		.type			= OTHER,
		.devs.note		= NULL,
		.init			= internal_init,
		.map_flash_region	= physmap,
		.unmap_flash_region	= physunmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_DUMMY == 1
	{
		.name			= "dummy",
		.type			= OTHER,
					/* FIXME */
		.devs.note		= "Dummy device, does nothing and logs all accesses\n",
		.init			= dummy_init,
		.map_flash_region	= dummy_map,
		.unmap_flash_region	= dummy_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_MEC1308 == 1
	{
		.name			= "mec1308",
		.type			= OTHER,
		.devs.note		= "Microchip MEC1308 Embedded Controller.\n",
		.init			= mec1308_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_NIC3COM == 1
	{
		.name			= "nic3com",
		.type			= PCI,
		.devs.dev		= nics_3com,
		.init			= nic3com_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_NICREALTEK == 1
	{
		/* This programmer works for Realtek RTL8139 and SMC 1211. */
		.name			= "nicrealtek",
		.type			= PCI,
		.devs.dev		= nics_realtek,
		.init			= nicrealtek_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_NICNATSEMI == 1
	{
		.name			= "nicnatsemi",
		.type			= PCI,
		.devs.dev		= nics_natsemi,
		.init			= nicnatsemi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_GFXNVIDIA == 1
	{
		.name			= "gfxnvidia",
		.type			= PCI,
		.devs.dev		= gfx_nvidia,
		.init			= gfxnvidia_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_RAIDEN_DEBUG_SPI == 1
	{
		.name			= "raiden_debug_spi",
		.type			= USB,
		.devs.dev		= devs_raiden,
		.init			= raiden_debug_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_DRKAISER == 1
	{
		.name			= "drkaiser",
		.type			= PCI,
		.devs.dev		= drkaiser_pcidev,
		.init			= drkaiser_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_SATASII == 1
	{
		.name			= "satasii",
		.type			= PCI,
		.devs.dev		= satas_sii,
		.init			= satasii_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_ATAHPT == 1
	{
		.name			= "atahpt",
		.type			= PCI,
		.devs.dev		= ata_hpt,
		.init			= atahpt_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_ATAVIA == 1
	{
		.name			= "atavia",
		.type			= PCI,
		.devs.dev		= ata_via,
		.init			= atavia_init,
		.map_flash_region	= atavia_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_ATAPROMISE == 1
	{
		.name			= "atapromise",
		.type			= PCI,
		.devs.dev		= ata_promise,
		.init			= atapromise_init,
		.map_flash_region	= atapromise_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_IT8212 == 1
	{
		.name			= "it8212",
		.type			= PCI,
		.devs.dev		= devs_it8212,
		.init			= it8212_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_FT2232_SPI == 1
	{
		.name			= "ft2232_spi",
		.type			= USB,
		.devs.dev		= devs_ft2232spi,
		.init			= ft2232_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_SERPROG == 1
	{
		.name			= "serprog",
		.type			= OTHER,
					/* FIXME */
		.devs.note		= "All programmer devices speaking the serprog protocol\n",
		.init			= serprog_init,
		.map_flash_region	= serprog_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= serprog_delay,
	},
#endif

#if CONFIG_BUSPIRATE_SPI == 1
	{
		.name			= "buspirate_spi",
		.type			= OTHER,
					/* FIXME */
		.devs.note		= "Dangerous Prototypes Bus Pirate\n",
		.init			= buspirate_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_DEDIPROG == 1
	{
		.name			= "dediprog",
		.type			= USB,
		.devs.dev		= devs_dediprog,
		.init			= dediprog_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_DEVELOPERBOX_SPI == 1
	{
		.name			= "developerbox",
		.type			= USB,
		.devs.dev		= devs_developerbox_spi,
		.init			= developerbox_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_ENE_LPC == 1
	{
		.name			= "ene_lpc",
		.type			= OTHER,
		.devs.note		= "ENE LPC interface keyboard controller\n",
		.init			= ene_lpc_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_RAYER_SPI == 1
	{
		.name			= "rayer_spi",
		.type			= OTHER,
					/* FIXME */
		.devs.note		= "RayeR parallel port programmer\n",
		.init			= rayer_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_PONY_SPI == 1
	{
		.name			= "pony_spi",
		.type			= OTHER,
					/* FIXME */
		.devs.note		= "Programmers compatible with SI-Prog, serbang or AJAWe\n",
		.init			= pony_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_NICINTEL == 1
	{
		.name			= "nicintel",
		.type			= PCI,
		.devs.dev		= nics_intel,
		.init			= nicintel_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_NICINTEL_SPI == 1
	{
		.name			= "nicintel_spi",
		.type			= PCI,
		.devs.dev		= nics_intel_spi,
		.init			= nicintel_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_NICINTEL_EEPROM == 1
	{
		.name			= "nicintel_eeprom",
		.type			= PCI,
		.devs.dev		= nics_intel_ee,
		.init			= nicintel_ee_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_OGP_SPI == 1
	{
		.name			= "ogp_spi",
		.type			= PCI,
		.devs.dev		= ogp_spi,
		.init			= ogp_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_SATAMV == 1
	{
		.name			= "satamv",
		.type			= PCI,
		.devs.dev		= satas_mv,
		.init			= satamv_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_LINUX_MTD == 1
	{
		.name			= "linux_mtd",
		.type			= OTHER,
		.devs.note		= "Device files /dev/mtd*\n",
		.init			= linux_mtd_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_LINUX_SPI == 1
	{
		.name			= "linux_spi",
		.type			= OTHER,
		.devs.note		= "Device files /dev/spidev*.*\n",
		.init			= linux_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_LSPCON_I2C_SPI == 1
	{
		.name			= "lspcon_i2c_spi",
		.type			= OTHER,
		.devs.note		= "Device files /dev/i2c-*.\n",
		.init			= lspcon_i2c_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_REALTEK_MST_I2C_SPI == 1
	{
		.name			= "realtek_mst_i2c_spi",
		.type			= OTHER,
		.devs.note		= "Device files /dev/i2c-*.\n",
		.init			= realtek_mst_i2c_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_USBBLASTER_SPI == 1
	{
		.name			= "usbblaster_spi",
		.type			= USB,
		.devs.dev		= devs_usbblasterspi,
		.init			= usbblaster_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_MSTARDDC_SPI == 1
	{
		.name			= "mstarddc_spi",
		.type			= OTHER,
		.devs.note		= "MSTAR DDC devices addressable via /dev/i2c-* on Linux.\n",
		.init			= mstarddc_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_PICKIT2_SPI == 1
	{
		.name			= "pickit2_spi",
		.type			= USB,
		.devs.dev		= devs_pickit2_spi,
		.init			= pickit2_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_CH341A_SPI == 1
	{
		.name			= "ch341a_spi",
		.type			= USB,
		.devs.dev		= devs_ch341a_spi,
		.init			= ch341a_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= ch341a_spi_delay,
	},
#endif

#if CONFIG_DIGILENT_SPI == 1
	{
		.name			= "digilent_spi",
		.type			= USB,
		.devs.dev		= devs_digilent_spi,
		.init			= digilent_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_JLINK_SPI == 1
	{
		.name			= "jlink_spi",
		.type			= OTHER,
		.init			= jlink_spi_init,
		.devs.note		= "SEGGER J-Link and compatible devices\n",
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_NI845X_SPI == 1
	{
		.name			= "ni845x_spi",
		.type			= OTHER, // choose other because NI-845x uses own USB implementation
		.devs.note		= "National Instruments USB-845x\n",
		.init			= ni845x_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif

#if CONFIG_STLINKV3_SPI == 1
	{
		.name			= "stlinkv3_spi",
		.type			= USB,
		.devs.dev		= devs_stlinkv3_spi,
		.init			= stlinkv3_spi_init,
		.map_flash_region	= fallback_map,
		.unmap_flash_region	= fallback_unmap,
		.delay			= internal_delay,
	},
#endif
};

const size_t programmer_table_size = ARRAY_SIZE(programmer_table);