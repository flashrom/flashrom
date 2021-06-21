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

const struct programmer_entry *const programmer_table[] = {

#if CONFIG_INTERNAL == 1
    &programmer_internal,
#endif

#if CONFIG_DUMMY == 1
    &programmer_dummy,
#endif

#if CONFIG_NIC3COM == 1
    &programmer_nic3com,
#endif

#if CONFIG_NICREALTEK == 1
    &programmer_nicrealtek,
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
#if CONFIG_ITE_EC == 1
    &programmer_ite_ec,
#endif
};

const size_t programmer_table_size = ARRAY_SIZE(programmer_table);
