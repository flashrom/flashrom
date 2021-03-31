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

#if CONFIG_INTERNAL == 1
const struct programmer_entry programmer_internal = {
	.name			= "internal",
	.type			= OTHER,
	.devs.note		= NULL,
	.init			= internal_init,
	.map_flash_region	= physmap,
	.unmap_flash_region	= physunmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_DUMMY == 1
const struct programmer_entry programmer_dummy = {
	.name			= "dummy",
	.type			= OTHER,
				/* FIXME */
	.devs.note		= "Dummy device, does nothing and logs all accesses\n",
	.init			= dummy_init,
	.map_flash_region	= dummy_map,
	.unmap_flash_region	= dummy_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_MEC1308 == 1
const struct programmer_entry programmer_mec1308 = {
	.name			= "mec1308",
	.type			= OTHER,
	.devs.note		= "Microchip MEC1308 Embedded Controller.\n",
	.init			= mec1308_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_NIC3COM == 1
const struct programmer_entry programmer_nic3com = {
	.name			= "nic3com",
	.type			= PCI,
	.devs.dev		= nics_3com,
	.init			= nic3com_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_NICREALTEK == 1
const struct programmer_entry programmer_nicrealtek = {
	/* This programmer works for Realtek RTL8139 and SMC 1211. */
	.name			= "nicrealtek",
	.type			= PCI,
	.devs.dev		= nics_realtek,
	.init			= nicrealtek_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_NICNATSEMI == 1
const struct programmer_entry programmer_nicnatsemi = {
	.name			= "nicnatsemi",
	.type			= PCI,
	.devs.dev		= nics_natsemi,
	.init			= nicnatsemi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_GFXNVIDIA == 1
const struct programmer_entry programmer_gfxnvidia = {
	.name			= "gfxnvidia",
	.type			= PCI,
	.devs.dev		= gfx_nvidia,
	.init			= gfxnvidia_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_RAIDEN_DEBUG_SPI == 1
const struct programmer_entry programmer_raiden_debug_spi = {
	.name			= "raiden_debug_spi",
	.type			= USB,
	.devs.dev		= devs_raiden,
	.init			= raiden_debug_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_DRKAISER == 1
const struct programmer_entry programmer_drkaiser = {
	.name			= "drkaiser",
	.type			= PCI,
	.devs.dev		= drkaiser_pcidev,
	.init			= drkaiser_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_SATASII == 1
const struct programmer_entry programmer_satasii = {
	.name			= "satasii",
	.type			= PCI,
	.devs.dev		= satas_sii,
	.init			= satasii_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_ATAHPT == 1
const struct programmer_entry programmer_atahpt = {
	.name			= "atahpt",
	.type			= PCI,
	.devs.dev		= ata_hpt,
	.init			= atahpt_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_ATAVIA == 1
const struct programmer_entry programmer_atavia = {
	.name			= "atavia",
	.type			= PCI,
	.devs.dev		= ata_via,
	.init			= atavia_init,
	.map_flash_region	= atavia_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_ATAPROMISE == 1
const struct programmer_entry programmer_atapromise = {
	.name			= "atapromise",
	.type			= PCI,
	.devs.dev		= ata_promise,
	.init			= atapromise_init,
	.map_flash_region	= atapromise_map,
	.unmap_flash_region	= fallback_unmap,
    .delay			= internal_delay,
};
#endif

#if CONFIG_IT8212 == 1
const struct programmer_entry programmer_it8212 = {
	.name			= "it8212",
	.type			= PCI,
	.devs.dev		= devs_it8212,
	.init			= it8212_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_FT2232_SPI == 1
const struct programmer_entry programmer_ft2232_spi = {
	.name			= "ft2232_spi",
	.type			= USB,
	.devs.dev		= devs_ft2232spi,
	.init			= ft2232_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_SERPROG == 1
const struct programmer_entry programmer_serprog = {
	.name			= "serprog",
	.type			= OTHER,
				/* FIXME */
	.devs.note		= "All programmer devices speaking the serprog protocol\n",
	.init			= serprog_init,
	.map_flash_region	= serprog_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= serprog_delay,
};
#endif

#if CONFIG_BUSPIRATE_SPI == 1
const struct programmer_entry programmer_buspirate_spi = {
	.name			= "buspirate_spi",
	.type			= OTHER,
				/* FIXME */
	.devs.note		= "Dangerous Prototypes Bus Pirate\n",
	.init			= buspirate_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_DEDIPROG == 1
const struct programmer_entry programmer_dediprog = {
	.name			= "dediprog",
	.type			= USB,
	.devs.dev		= devs_dediprog,
	.init			= dediprog_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_DEVELOPERBOX_SPI == 1
const struct programmer_entry programmer_developerbox = {
	.name			= "developerbox",
	.type			= USB,
	.devs.dev		= devs_developerbox_spi,
	.init			= developerbox_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
    .delay			= internal_delay,
};
#endif

#if CONFIG_ENE_LPC == 1
const struct programmer_entry programmer_ene_lpc = {
	.name			= "ene_lpc",
	.type			= OTHER,
	.devs.note		= "ENE LPC interface keyboard controller\n",
	.init			= ene_lpc_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_RAYER_SPI == 1
const struct programmer_entry programmer_rayer_spi = {
	.name			= "rayer_spi",
	.type			= OTHER,
				/* FIXME */
	.devs.note		= "RayeR parallel port programmer\n",
	.init			= rayer_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_PONY_SPI == 1
const struct programmer_entry programmer_pony_spi = {
	.name			= "pony_spi",
	.type			= OTHER,
				/* FIXME */
	.devs.note		= "Programmers compatible with SI-Prog, serbang or AJAWe\n",
	.init			= pony_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_NICINTEL == 1
const struct programmer_entry programmer_nicintel = {
	.name			= "nicintel",
	.type			= PCI,
	.devs.dev		= nics_intel,
	.init			= nicintel_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_NICINTEL_SPI == 1
const struct programmer_entry programmer_nicintel_spi = {
	.name			= "nicintel_spi",
	.type			= PCI,
	.devs.dev		= nics_intel_spi,
	.init			= nicintel_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_NICINTEL_EEPROM == 1
const struct programmer_entry programmer_nicintel_eeprom = {
	.name			= "nicintel_eeprom",
	.type			= PCI,
	.devs.dev		= nics_intel_ee,
	.init			= nicintel_ee_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_OGP_SPI == 1
const struct programmer_entry programmer_ogp_spi = {
	.name			= "ogp_spi",
	.type			= PCI,
	.devs.dev		= ogp_spi,
	.init			= ogp_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_SATAMV == 1
const struct programmer_entry programmer_satamv = {
	.name			= "satamv",
	.type			= PCI,
	.devs.dev		= satas_mv,
	.init			= satamv_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_LINUX_MTD == 1
const struct programmer_entry programmer_linux_mtd = {
	.name			= "linux_mtd",
	.type			= OTHER,
	.devs.note		= "Device files /dev/mtd*\n",
	.init			= linux_mtd_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_LINUX_SPI == 1
const struct programmer_entry programmer_linux_spi = {
	.name			= "linux_spi",
	.type			= OTHER,
	.devs.note		= "Device files /dev/spidev*.*\n",
	.init			= linux_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_LSPCON_I2C_SPI == 1
const struct programmer_entry programmer_lspcon_i2c_spi = {
	.name			= "lspcon_i2c_spi",
	.type			= OTHER,
	.devs.note		= "Device files /dev/i2c-*.\n",
	.init			= lspcon_i2c_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_REALTEK_MST_I2C_SPI == 1
const struct programmer_entry programmer_realtek_mst_i2c_spi = {
	.name			= "realtek_mst_i2c_spi",
	.type			= OTHER,
	.devs.note		= "Device files /dev/i2c-*.\n",
	.init			= realtek_mst_i2c_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_USBBLASTER_SPI == 1
const struct programmer_entry programmer_usbblaster_spi = {
	.name			= "usbblaster_spi",
	.type			= USB,
	.devs.dev		= devs_usbblasterspi,
	.init			= usbblaster_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_MSTARDDC_SPI == 1
const struct programmer_entry programmer_mstarddc_spi = {
	.name			= "mstarddc_spi",
	.type			= OTHER,
	.devs.note		= "MSTAR DDC devices addressable via /dev/i2c-* on Linux.\n",
	.init			= mstarddc_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_PICKIT2_SPI == 1
const struct programmer_entry programmer_pickit2_spi = {
	.name			= "pickit2_spi",
	.type			= USB,
	.devs.dev		= devs_pickit2_spi,
	.init			= pickit2_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_CH341A_SPI == 1
const struct programmer_entry programmer_ch341a_spi = {
	.name			= "ch341a_spi",
	.type			= USB,
	.devs.dev		= devs_ch341a_spi,
	.init			= ch341a_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= ch341a_spi_delay,
};
#endif

#if CONFIG_DIGILENT_SPI == 1
const struct programmer_entry programmer_digilent_spi = {
	.name			= "digilent_spi",
	.type			= USB,
	.devs.dev		= devs_digilent_spi,
	.init			= digilent_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_JLINK_SPI == 1
const struct programmer_entry programmer_jlink_spi = {
	.name			= "jlink_spi",
	.type			= OTHER,
	.init			= jlink_spi_init,
	.devs.note		= "SEGGER J-Link and compatible devices\n",
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_NI845X_SPI == 1
const struct programmer_entry programmer_ni845x_spi = {
	.name			= "ni845x_spi",
	.type			= OTHER, // choose other because NI-845x uses own USB implementation
	.devs.note		= "National Instruments USB-845x\n",
	.init			= ni845x_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

#if CONFIG_STLINKV3_SPI == 1
const struct programmer_entry programmer_stlinkv3_spi = {
	.name			= "stlinkv3_spi",
	.type			= USB,
	.devs.dev		= devs_stlinkv3_spi,
	.init			= stlinkv3_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
#endif

const struct programmer_entry *const programmer_table[] = {

#if CONFIG_INTERNAL == 1
    &programmer_internal,
#endif

#if CONFIG_DUMMY == 1
    &programmer_dummy,
#endif

#if CONFIG_MEC1308 == 1
    &programmer_mec1308,
#endif

#if CONFIG_NIC3COM == 1
    &programmer_nic3com,
#endif

#if CONFIG_NICREALTEK == 1
    & programmer_nicrealtek,
#endif

#if CONFIG_NICNATSEMI == 1
    &programmer_nicnatsemi,
#endif

#if CONFIG_GFXNVIDIA == 1
    &programmer_gfxnvidia,
#endif

#if CONFIG_RAIDEN_DEBUG_SPI == 1
    &programmer_raiden_debug_spi,
#endif

#if CONFIG_DRKAISER == 1
    &programmer_drkaiser,
#endif

#if CONFIG_SATASII == 1
    &programmer_satasii,
#endif

#if CONFIG_ATAHPT == 1
    &programmer_atahpt,
#endif

#if CONFIG_ATAVIA == 1
    &programmer_atavia,
#endif

#if CONFIG_ATAPROMISE == 1
    &programmer_atapromise,
#endif

#if CONFIG_IT8212 == 1
    &programmer_it8212,
#endif

#if CONFIG_FT2232_SPI == 1
    &programmer_ft2232_spi,
#endif

#if CONFIG_SERPROG == 1
    &programmer_serprog,
#endif

#if CONFIG_BUSPIRATE_SPI == 1
    &programmer_buspirate_spi,
#endif

#if CONFIG_DEDIPROG == 1
    &programmer_dediprog,
#endif

#if CONFIG_DEVELOPERBOX_SPI == 1
    &programmer_developerbox,
#endif

#if CONFIG_ENE_LPC == 1
    &programmer_ene_lpc,
#endif

#if CONFIG_RAYER_SPI == 1
    &programmer_rayer_spi,
#endif

#if CONFIG_PONY_SPI == 1
    &programmer_pony_spi,
#endif

#if CONFIG_NICINTEL == 1
    &programmer_nicintel,
#endif

#if CONFIG_NICINTEL_SPI == 1
    &programmer_nicintel_spi,
#endif

#if CONFIG_NICINTEL_EEPROM == 1
    &programmer_nicintel_eeprom,
#endif

#if CONFIG_OGP_SPI == 1
    &programmer_ogp_spi,
#endif

#if CONFIG_SATAMV == 1
    &programmer_satamv,
#endif

#if CONFIG_LINUX_MTD == 1
    &programmer_linux_mtd,
#endif

#if CONFIG_LINUX_SPI == 1
    &programmer_linux_spi,
#endif

#if CONFIG_LSPCON_I2C_SPI == 1
    &programmer_lspcon_i2c_spi,
#endif

#if CONFIG_REALTEK_MST_I2C_SPI == 1
    &programmer_realtek_mst_i2c_spi,
#endif

#if CONFIG_USBBLASTER_SPI == 1
    &programmer_usbblaster_spi,
#endif

#if CONFIG_MSTARDDC_SPI == 1
    &programmer_mstarddc_spi,
#endif

#if CONFIG_PICKIT2_SPI == 1
    &programmer_pickit2_spi,
#endif

#if CONFIG_CH341A_SPI == 1
    &programmer_ch341a_spi,
#endif

#if CONFIG_DIGILENT_SPI == 1
    &programmer_digilent_spi,
#endif

#if CONFIG_JLINK_SPI == 1
    &programmer_jlink_spi,
#endif

#if CONFIG_NI845X_SPI == 1
    &programmer_ni845x_spi,
#endif

#if CONFIG_STLINKV3_SPI == 1
    &programmer_stlinkv3_spi,
#endif
};

const size_t programmer_table_size = ARRAY_SIZE(programmer_table);