/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009,2010 Carl-Daniel Hailfinger
 * Copyright (C) 2016 Hatim Kanchwala <hatim@hatimak.me>
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"

/* Remove the #define below if you don't want SPI flash chip emulation. */
#define EMULATE_SPI_CHIP 1

#if EMULATE_SPI_CHIP
#define EMULATE_CHIP 1
#include "spi.h"
#endif

#if EMULATE_CHIP
#include <sys/types.h>
#include <sys/stat.h>
#endif

#if EMULATE_CHIP
static uint8_t *flashchip_contents = NULL;
static uint8_t *gd25q128c_otp_1_contents = NULL;
static uint8_t *gd25q128c_otp_2_contents = NULL;
static uint8_t *gd25q128c_otp_3_contents = NULL;
static uint8_t *en25qh128_otp_contents = NULL;
static uint8_t *w25q40v_otp_1_contents = NULL;
static uint8_t *w25q40v_otp_2_contents = NULL;
static uint8_t *w25q40v_otp_3_contents = NULL;
enum emu_chip {
	EMULATE_NONE,
	EMULATE_ST_M25P10_RES,
	EMULATE_SST_SST25VF040_REMS,
	EMULATE_SST_SST25VF032B,
	EMULATE_MACRONIX_MX25L6436,
	EMULATE_GIGADEVICE_GD25Q128C,
	EMULATE_EON_EN25QH128,
	EMULATE_WINBOND_W25Q40_V,
};
static enum emu_chip emu_chip = EMULATE_NONE;
static char *emu_persistent_image = NULL;
static unsigned int emu_chip_size = 0;
#if EMULATE_SPI_CHIP
static unsigned int emu_max_byteprogram_size = 0;
static unsigned int emu_max_aai_size = 0;
static unsigned int emu_jedec_se_size = 0;
static unsigned int emu_jedec_be_52_size = 0;
static unsigned int emu_jedec_be_d8_size = 0;
static unsigned int emu_jedec_ce_60_size = 0;
static unsigned int emu_jedec_ce_c7_size = 0;
unsigned char spi_blacklist[256];
unsigned char spi_ignorelist[256];
int spi_blacklist_size = 0;
int spi_ignorelist_size = 0;
static uint32_t emu_status = 0;
static char *emu_persistent_gd25_otp_1_image = NULL;
static char *emu_persistent_gd25_otp_2_image = NULL;
static char *emu_persistent_gd25_otp_3_image = NULL;
static char *emu_persistent_en25_otp_image = NULL;
static char *emu_persistent_w25_otp_1_image = NULL;
static char *emu_persistent_w25_otp_2_image = NULL;
static char *emu_persistent_w25_otp_3_image = NULL;
static uint8_t read_only_bits = 0;
static uint8_t en25qh128_otp_mode = 0, en25qh128_otp_bit = 0;

/* A legit complete SFDP table based on the MX25L6436E (rev. 1.8) datasheet. */
static const uint8_t sfdp_table[] = {
	0x53, 0x46, 0x44, 0x50, // @0x00: SFDP signature
	0x00, 0x01, 0x01, 0xFF, // @0x04: revision 1.0, 2 headers
	0x00, 0x00, 0x01, 0x09, // @0x08: JEDEC SFDP header rev. 1.0, 9 DW long
	0x1C, 0x00, 0x00, 0xFF, // @0x0C: PTP0 = 0x1C (instead of 0x30)
	0xC2, 0x00, 0x01, 0x04, // @0x10: Macronix header rev. 1.0, 4 DW long
	0x48, 0x00, 0x00, 0xFF, // @0x14: PTP1 = 0x48 (instead of 0x60)
	0xFF, 0xFF, 0xFF, 0xFF, // @0x18: hole.
	0xE5, 0x20, 0xC9, 0xFF, // @0x1C: SFDP parameter table start
	0xFF, 0xFF, 0xFF, 0x03, // @0x20
	0x00, 0xFF, 0x08, 0x6B, // @0x24
	0x08, 0x3B, 0x00, 0xFF, // @0x28
	0xEE, 0xFF, 0xFF, 0xFF, // @0x2C
	0xFF, 0xFF, 0x00, 0x00, // @0x30
	0xFF, 0xFF, 0x00, 0xFF, // @0x34
	0x0C, 0x20, 0x0F, 0x52, // @0x38
	0x10, 0xD8, 0x00, 0xFF, // @0x3C: SFDP parameter table end
	0xFF, 0xFF, 0xFF, 0xFF, // @0x40: hole.
	0xFF, 0xFF, 0xFF, 0xFF, // @0x44: hole.
	0x00, 0x36, 0x00, 0x27, // @0x48: Macronix parameter table start
	0xF4, 0x4F, 0xFF, 0xFF, // @0x4C
	0xD9, 0xC8, 0xFF, 0xFF, // @0x50
	0xFF, 0xFF, 0xFF, 0xFF, // @0x54: Macronix parameter table end
};

// TODO(hatim): Add SFDP table for GD25Q128C, EN25QH128 and W25Q40.V

#endif
#endif

static unsigned int spi_write_256_chunksize = 256;

static int dummy_spi_send_command(struct flashctx *flash, unsigned int writecnt, unsigned int readcnt,
				  const unsigned char *writearr, unsigned char *readarr);
static int dummy_spi_write_256(struct flashctx *flash, const uint8_t *buf,
			       unsigned int start, unsigned int len);
static void dummy_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr);
static void dummy_chip_writew(const struct flashctx *flash, uint16_t val, chipaddr addr);
static void dummy_chip_writel(const struct flashctx *flash, uint32_t val, chipaddr addr);
static void dummy_chip_writen(const struct flashctx *flash, const uint8_t *buf, chipaddr addr, size_t len);
static uint8_t dummy_chip_readb(const struct flashctx *flash, const chipaddr addr);
static uint16_t dummy_chip_readw(const struct flashctx *flash, const chipaddr addr);
static uint32_t dummy_chip_readl(const struct flashctx *flash, const chipaddr addr);
static void dummy_chip_readn(const struct flashctx *flash, uint8_t *buf, const chipaddr addr, size_t len);

static const struct spi_master spi_master_dummyflasher = {
	.type		= SPI_CONTROLLER_DUMMY,
	.features	= SPI_MASTER_4BA,
	.max_data_read	= MAX_DATA_READ_UNLIMITED,
	.max_data_write	= MAX_DATA_UNSPECIFIED,
	.command	= dummy_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= default_spi_read,
	.write_256	= dummy_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

static const struct par_master par_master_dummy = {
		.chip_readb		= dummy_chip_readb,
		.chip_readw		= dummy_chip_readw,
		.chip_readl		= dummy_chip_readl,
		.chip_readn		= dummy_chip_readn,
		.chip_writeb		= dummy_chip_writeb,
		.chip_writew		= dummy_chip_writew,
		.chip_writel		= dummy_chip_writel,
		.chip_writen		= dummy_chip_writen,
};

enum chipbustype dummy_buses_supported = BUS_NONE;

static int dummy_shutdown(void *data)
{
	msg_pspew("%s\n", __func__);
#if EMULATE_CHIP
	if (emu_chip != EMULATE_NONE) {
		if (emu_persistent_image) {
			msg_pdbg("Writing %s\n", emu_persistent_image);
			write_buf_to_file(flashchip_contents, emu_chip_size, emu_persistent_image);
			free(emu_persistent_image);
			emu_persistent_image = NULL;
		}
		free(flashchip_contents);
		if (emu_chip == EMULATE_GIGADEVICE_GD25Q128C) {
			if (emu_persistent_gd25_otp_1_image) {
				msg_pdbg("Writing %s\n", emu_persistent_gd25_otp_1_image);
				write_buf_to_file(gd25q128c_otp_1_contents, 512, emu_persistent_gd25_otp_1_image);
				free(emu_persistent_gd25_otp_1_image);
				emu_persistent_gd25_otp_1_image = NULL;
			}
			if (emu_persistent_gd25_otp_2_image) {
				msg_pdbg("Writing %s\n", emu_persistent_gd25_otp_2_image);
				write_buf_to_file(gd25q128c_otp_2_contents, 512, emu_persistent_gd25_otp_2_image);
				free(emu_persistent_gd25_otp_2_image);
				emu_persistent_gd25_otp_2_image = NULL;
			}
			if (emu_persistent_gd25_otp_3_image) {
				msg_pdbg("Writing %s\n", emu_persistent_gd25_otp_3_image);
				write_buf_to_file(gd25q128c_otp_3_contents, 512, emu_persistent_gd25_otp_3_image);
				free(emu_persistent_gd25_otp_3_image);
				emu_persistent_gd25_otp_3_image = NULL;
			}
			free(gd25q128c_otp_1_contents);
			free(gd25q128c_otp_2_contents);
			free(gd25q128c_otp_3_contents);
		}
		if (emu_chip == EMULATE_EON_EN25QH128) {
			if (emu_persistent_en25_otp_image) {
				msg_pdbg("Writing %s\n", emu_persistent_en25_otp_image);
				write_buf_to_file(en25qh128_otp_contents, 512, emu_persistent_en25_otp_image);
				free(emu_persistent_en25_otp_image);
				emu_persistent_en25_otp_image = NULL;
			}
			free(en25qh128_otp_contents);
		}
		if (emu_chip == EMULATE_WINBOND_W25Q40_V) {
			if (emu_persistent_w25_otp_1_image) {
				msg_pdbg("Writing %s\n", emu_persistent_w25_otp_1_image);
				write_buf_to_file(w25q40v_otp_1_contents, 256, emu_persistent_w25_otp_1_image);
				free(emu_persistent_w25_otp_1_image);
				emu_persistent_w25_otp_1_image = NULL;
			}
			if (emu_persistent_w25_otp_2_image) {
				msg_pdbg("Writing %s\n", emu_persistent_w25_otp_2_image);
				write_buf_to_file(w25q40v_otp_2_contents, 256, emu_persistent_w25_otp_2_image);
				free(emu_persistent_w25_otp_2_image);
				emu_persistent_w25_otp_2_image = NULL;
			}
			if (emu_persistent_w25_otp_3_image) {
				msg_pdbg("Writing %s\n", emu_persistent_w25_otp_3_image);
				write_buf_to_file(w25q40v_otp_3_contents, 256, emu_persistent_w25_otp_3_image);
				free(emu_persistent_w25_otp_3_image);
				emu_persistent_w25_otp_3_image = NULL;
			}
			free(w25q40v_otp_1_contents);
			free(w25q40v_otp_2_contents);
			free(w25q40v_otp_3_contents);
		}
	}
#endif
	return 0;
}

int dummy_init(void)
{
	char *bustext = NULL;
	char *tmp = NULL;
	int i;
#if EMULATE_SPI_CHIP
	char *status = NULL;
#endif
#if EMULATE_CHIP
	struct stat image_stat;
#endif

	msg_pspew("%s\n", __func__);

	bustext = extract_programmer_param("bus");
	msg_pdbg("Requested buses are: %s\n", bustext ? bustext : "default");
	if (!bustext)
		bustext = strdup("parallel+lpc+fwh+spi");
	/* Convert the parameters to lowercase. */
	tolower_string(bustext);

	dummy_buses_supported = BUS_NONE;
	if (strstr(bustext, "parallel")) {
		dummy_buses_supported |= BUS_PARALLEL;
		msg_pdbg("Enabling support for %s flash.\n", "parallel");
	}
	if (strstr(bustext, "lpc")) {
		dummy_buses_supported |= BUS_LPC;
		msg_pdbg("Enabling support for %s flash.\n", "LPC");
	}
	if (strstr(bustext, "fwh")) {
		dummy_buses_supported |= BUS_FWH;
		msg_pdbg("Enabling support for %s flash.\n", "FWH");
	}
	if (strstr(bustext, "spi")) {
		dummy_buses_supported |= BUS_SPI;
		msg_pdbg("Enabling support for %s flash.\n", "SPI");
	}
	if (dummy_buses_supported == BUS_NONE)
		msg_pdbg("Support for all flash bus types disabled.\n");
	free(bustext);

	tmp = extract_programmer_param("spi_write_256_chunksize");
	if (tmp) {
		spi_write_256_chunksize = atoi(tmp);
		free(tmp);
		if (spi_write_256_chunksize < 1) {
			msg_perr("invalid spi_write_256_chunksize\n");
			return 1;
		}
	}

	tmp = extract_programmer_param("spi_blacklist");
	if (tmp) {
		i = strlen(tmp);
		if (!strncmp(tmp, "0x", 2)) {
			i -= 2;
			memmove(tmp, tmp + 2, i + 1);
		}
		if ((i > 512) || (i % 2)) {
			msg_perr("Invalid SPI command blacklist length\n");
			free(tmp);
			return 1;
		}
		spi_blacklist_size = i / 2;
		for (i = 0; i < spi_blacklist_size * 2; i++) {
			if (!isxdigit((unsigned char)tmp[i])) {
				msg_perr("Invalid char \"%c\" in SPI command "
					 "blacklist\n", tmp[i]);
				free(tmp);
				return 1;
			}
		}
		for (i = 0; i < spi_blacklist_size; i++) {
			unsigned int tmp2;
			/* SCNx8 is apparently not supported by MSVC (and thus
			 * MinGW), so work around it with an extra variable
			 */
			sscanf(tmp + i * 2, "%2x", &tmp2);
			spi_blacklist[i] = (uint8_t)tmp2;
		}
		msg_pdbg("SPI blacklist is ");
		for (i = 0; i < spi_blacklist_size; i++)
			msg_pdbg("%02x ", spi_blacklist[i]);
		msg_pdbg(", size %i\n", spi_blacklist_size);
	}
	free(tmp);

	tmp = extract_programmer_param("spi_ignorelist");
	if (tmp) {
		i = strlen(tmp);
		if (!strncmp(tmp, "0x", 2)) {
			i -= 2;
			memmove(tmp, tmp + 2, i + 1);
		}
		if ((i > 512) || (i % 2)) {
			msg_perr("Invalid SPI command ignorelist length\n");
			free(tmp);
			return 1;
		}
		spi_ignorelist_size = i / 2;
		for (i = 0; i < spi_ignorelist_size * 2; i++) {
			if (!isxdigit((unsigned char)tmp[i])) {
				msg_perr("Invalid char \"%c\" in SPI command "
					 "ignorelist\n", tmp[i]);
				free(tmp);
				return 1;
			}
		}
		for (i = 0; i < spi_ignorelist_size; i++) {
			unsigned int tmp2;
			/* SCNx8 is apparently not supported by MSVC (and thus
			 * MinGW), so work around it with an extra variable
			 */
			sscanf(tmp + i * 2, "%2x", &tmp2);
			spi_ignorelist[i] = (uint8_t)tmp2;
		}
		msg_pdbg("SPI ignorelist is ");
		for (i = 0; i < spi_ignorelist_size; i++)
			msg_pdbg("%02x ", spi_ignorelist[i]);
		msg_pdbg(", size %i\n", spi_ignorelist_size);
	}
	free(tmp);

#if EMULATE_CHIP
	tmp = extract_programmer_param("emulate");
	if (!tmp) {
		msg_pdbg("Not emulating any flash chip.\n");
		/* Nothing else to do. */
		goto dummy_init_out;
	}
#if EMULATE_SPI_CHIP
	if (!strcmp(tmp, "M25P10.RES")) {
		emu_chip = EMULATE_ST_M25P10_RES;
		emu_chip_size = 128 * 1024;
		emu_max_byteprogram_size = 128;
		emu_max_aai_size = 0;
		emu_jedec_se_size = 0;
		emu_jedec_be_52_size = 0;
		emu_jedec_be_d8_size = 32 * 1024;
		emu_jedec_ce_60_size = 0;
		emu_jedec_ce_c7_size = emu_chip_size;
		msg_pdbg("Emulating ST M25P10.RES SPI flash chip (RES, page "
			 "write)\n");
	}
	if (!strcmp(tmp, "SST25VF040.REMS")) {
		emu_chip = EMULATE_SST_SST25VF040_REMS;
		emu_chip_size = 512 * 1024;
		emu_max_byteprogram_size = 1;
		emu_max_aai_size = 0;
		emu_jedec_se_size = 4 * 1024;
		emu_jedec_be_52_size = 32 * 1024;
		emu_jedec_be_d8_size = 0;
		emu_jedec_ce_60_size = emu_chip_size;
		emu_jedec_ce_c7_size = 0;
		msg_pdbg("Emulating SST SST25VF040.REMS SPI flash chip (REMS, "
			 "byte write)\n");
	}
	if (!strcmp(tmp, "SST25VF032B")) {
		emu_chip = EMULATE_SST_SST25VF032B;
		emu_chip_size = 4 * 1024 * 1024;
		emu_max_byteprogram_size = 1;
		emu_max_aai_size = 2;
		emu_jedec_se_size = 4 * 1024;
		emu_jedec_be_52_size = 32 * 1024;
		emu_jedec_be_d8_size = 64 * 1024;
		emu_jedec_ce_60_size = emu_chip_size;
		emu_jedec_ce_c7_size = emu_chip_size;
		msg_pdbg("Emulating SST SST25VF032B SPI flash chip (RDID, AAI "
			 "write)\n");
	}
	if (!strcmp(tmp, "MX25L6436")) {
		emu_chip = EMULATE_MACRONIX_MX25L6436;
		emu_chip_size = 8 * 1024 * 1024;
		emu_max_byteprogram_size = 256;
		emu_max_aai_size = 0;
		emu_jedec_se_size = 4 * 1024;
		emu_jedec_be_52_size = 32 * 1024;
		emu_jedec_be_d8_size = 64 * 1024;
		emu_jedec_ce_60_size = emu_chip_size;
		emu_jedec_ce_c7_size = emu_chip_size;
		msg_pdbg("Emulating Macronix MX25L6436 SPI flash chip (RDID, "
			 "SFDP)\n");
	}
	if (!strcmp(tmp, "GD25Q128C")) {
		emu_chip = EMULATE_GIGADEVICE_GD25Q128C;
		emu_chip_size = 16 * 1024 * 1024;
		emu_max_byteprogram_size = 256;
		emu_max_aai_size = 0;
		emu_jedec_se_size = 4 * 1024;
		emu_jedec_be_52_size = 32 * 1024;
		emu_jedec_be_d8_size = 64 * 1024;
		emu_jedec_ce_60_size = emu_chip_size;
		emu_jedec_ce_c7_size = emu_chip_size;
		msg_pdbg("Emulating GigaDevice GD25Q128C SPI flash chip (RDID)\n");
	}
	if (!strcmp(tmp, "EN25QH128")) {
		emu_chip = EMULATE_EON_EN25QH128;
		emu_chip_size = 16 * 1024 * 1024;
		emu_max_byteprogram_size = 256;
		emu_max_aai_size = 0;
		emu_jedec_se_size = 4 * 1024;
		emu_jedec_be_52_size = 32 * 1024;
		emu_jedec_be_d8_size = 64 * 1024;
		emu_jedec_ce_60_size = emu_chip_size;
		emu_jedec_ce_c7_size = emu_chip_size;
		msg_pdbg("Emulating Eon EN25QH128 SPI flash chip (RDID)\n");
	}
	if (!strcmp(tmp, "W25Q40.V")) {
		emu_chip = EMULATE_WINBOND_W25Q40_V;
		emu_chip_size = 512 * 1024;
		emu_max_byteprogram_size = 256;
		emu_max_aai_size = 0;
		emu_jedec_se_size = 4 * 1024;
		emu_jedec_be_52_size = 32 * 1024;
		emu_jedec_be_d8_size = 64 * 1024;
		emu_jedec_ce_60_size = emu_chip_size;
		emu_jedec_ce_c7_size = emu_chip_size;
		msg_pdbg("Emulating Winbond W25Q40.V SPI flash chip (RDID)\n");
	}
#endif
	if (emu_chip == EMULATE_NONE) {
		msg_perr("Invalid chip specified for emulation: %s\n", tmp);
		free(tmp);
		return 1;
	}
	free(tmp);
	flashchip_contents = malloc(emu_chip_size);
	if (!flashchip_contents) {
		msg_perr("Out of memory!\n");
		return 1;
	}

	/* Allocate memory for each of the 512 bytes Security Register of GD25Q128C. */
	if (emu_chip == EMULATE_GIGADEVICE_GD25Q128C) {
		gd25q128c_otp_1_contents = malloc(512);
		if (!gd25q128c_otp_1_contents) {
			msg_perr("Out of memory!\n");
			return 1;
		}
		gd25q128c_otp_2_contents = malloc(512);
		if (!gd25q128c_otp_2_contents) {
			msg_perr("Out of memory!\n");
			return 1;
		}
		gd25q128c_otp_3_contents = malloc(512);
		if (!gd25q128c_otp_3_contents) {
			msg_perr("Out of memory!\n");
			return 1;
		}
	}

	/* Allocate memory for the 512 byte security sector of EN25QH128. */
	if (emu_chip == EMULATE_EON_EN25QH128) {
		en25qh128_otp_contents = malloc(512);
		if (!en25qh128_otp_contents) {
			msg_perr("Out of memory!\n");
			return 1;
		}
	}

	/* Allocate memory for each of the 256 bytes Security Register of GD25Q128C. */
	if (emu_chip == EMULATE_WINBOND_W25Q40_V) {
		w25q40v_otp_1_contents = malloc(256);
		if (!w25q40v_otp_1_contents) {
			msg_perr("Out of memory!\n");
			return 1;
		}
		w25q40v_otp_2_contents = malloc(256);
		if (!w25q40v_otp_2_contents) {
			msg_perr("Out of memory!\n");
			return 1;
		}
		w25q40v_otp_3_contents = malloc(256);
		if (!w25q40v_otp_3_contents) {
			msg_perr("Out of memory!\n");
			return 1;
		}
	}

#ifdef EMULATE_SPI_CHIP
	status = extract_programmer_param("spi_status");
	if (status) {
		char *endptr;
		errno = 0;
		emu_status = strtoul(status, &endptr, 0);
		free(status);
		if (errno != 0 || status == endptr) {
			msg_perr("Error: initial status register specified, "
				 "but the value could not be converted.\n");
			return 1;
		}
		if (emu_chip == EMULATE_GIGADEVICE_GD25Q128C) {
			msg_pdbg("Initial status registers -\n"
				"\tSR1 is set to 0x%02x\n"
				"\tSR2 is set to 0x%02x\n"
				"\tSR3 is set to 0x%02x\n",
				emu_status & 0xff, (emu_status >> 8) & 0xff, (emu_status >> 16) & 0xff);
		} else if (emu_chip == EMULATE_WINBOND_W25Q40_V) {
			msg_pdbg("Initial status registers -\n"
				"\tSR1 is set to 0x%02x\n"
				"\tSR2 is set to 0x%02x\n",
				emu_status & 0xff, (emu_status >> 8) & 0xff);
		} else {
			msg_pdbg("Initial status register is set to 0x%02x.\n",
			 emu_status);
		}
	}
#endif

	msg_pdbg("Filling fake flash chip with 0xff, size %i\n", emu_chip_size);
	memset(flashchip_contents, 0xff, emu_chip_size);

	/* Will be freed by shutdown function if necessary. */
	emu_persistent_image = extract_programmer_param("image");
	/* We will silently (in default verbosity) ignore the file if it does not exist (yet) or the size does
	 * not match the emulated chip. */
	if (emu_persistent_image && !stat(emu_persistent_image, &image_stat)) {
		msg_pdbg("Found persistent image %s, %jd B ",
			 emu_persistent_image, (intmax_t)image_stat.st_size);
		if (image_stat.st_size == emu_chip_size) {
			msg_pdbg("matches.\n");
			msg_pdbg("Reading %s\n", emu_persistent_image);
			read_buf_from_file(flashchip_contents, emu_chip_size,
					   emu_persistent_image);
		} else {
			msg_pdbg("doesn't match.\n");
		}
	}

	/* Each OTP image will be freed by shutdown function, if necessary. GD25Q128C has 3 Security Registers
	 * each of 512 bytes.
	 * We will silently (in default verbosity) ignore the file(s) if it does not exist (yet)
	 * or the size does not match the security register size on chip. */
	if (emu_chip == EMULATE_GIGADEVICE_GD25Q128C) {
		msg_pdbg("Filling fake security register 1, 2 and 3 with 0xff, size 512 bytes each\n");
		memset(gd25q128c_otp_1_contents, 0xff, 512);
		memset(gd25q128c_otp_2_contents, 0xff, 512);
		memset(gd25q128c_otp_3_contents, 0xff, 512);

		emu_persistent_gd25_otp_1_image = extract_programmer_param("otp_1");
		emu_persistent_gd25_otp_2_image = extract_programmer_param("otp_2");
		emu_persistent_gd25_otp_3_image = extract_programmer_param("otp_3");

		if (emu_persistent_gd25_otp_1_image && !stat(emu_persistent_gd25_otp_1_image, &image_stat)) {
			msg_pdbg("Found persistent image %s, %jd B for security register 1, ",
				 emu_persistent_gd25_otp_1_image, (intmax_t)image_stat.st_size);
			if (image_stat.st_size == 512) {
				msg_pdbg("matches.\n");
				msg_pdbg("Reading %s\n", emu_persistent_gd25_otp_1_image);
				read_buf_from_file(gd25q128c_otp_1_contents, 512,
						   emu_persistent_gd25_otp_1_image);
			} else {
				msg_pdbg("doesn't match.\n");
			}
		}
		if (emu_persistent_gd25_otp_2_image && !stat(emu_persistent_gd25_otp_2_image, &image_stat)) {
			msg_pdbg("Found persistent image %s, %jd B for security register 2, ",
				 emu_persistent_gd25_otp_2_image, (intmax_t)image_stat.st_size);
			if (image_stat.st_size == 512) {
				msg_pdbg("matches.\n");
				msg_pdbg("Reading %s\n", emu_persistent_gd25_otp_2_image);
				read_buf_from_file(gd25q128c_otp_2_contents, 512,
						   emu_persistent_gd25_otp_2_image);
			} else {
				msg_pdbg("doesn't match.\n");
			}
		}
		if (emu_persistent_gd25_otp_3_image && !stat(emu_persistent_gd25_otp_3_image, &image_stat)) {
			msg_pdbg("Found persistent image %s, %jd B for security register 3, ",
				 emu_persistent_gd25_otp_3_image, (intmax_t)image_stat.st_size);
			if (image_stat.st_size == 512) {
				msg_pdbg("matches.\n");
				msg_pdbg("Reading %s\n", emu_persistent_gd25_otp_3_image);
				read_buf_from_file(gd25q128c_otp_3_contents, 512,
						   emu_persistent_gd25_otp_3_image);
			} else {
				msg_pdbg("doesn't match.\n");
			}
		}
	}

	/* Eon EN25QH128 has a security sector of 512 bytes. OTP image will be freed by shutdown
	 * function, if necessary. We will silently (in default verbosity) ignore the file(s)
	 * if it does not exist (yet) or the size does not match the security register size on chip. */
	if (emu_chip == EMULATE_EON_EN25QH128) {
		msg_pdbg("Filling fake security sector with 0xff, size 512 bytes\n");
		memset(en25qh128_otp_contents, 0xff, 512);
		emu_persistent_en25_otp_image = extract_programmer_param("otp");
		if (emu_persistent_en25_otp_image && !stat(emu_persistent_en25_otp_image, &image_stat)) {
			msg_pdbg("Found persistent image %s, %jd B for security sector, ",
				 emu_persistent_en25_otp_image, (intmax_t)image_stat.st_size);
			if (image_stat.st_size == 512) {
				msg_pdbg("matches.\n");
				msg_pdbg("Reading %s\n", emu_persistent_en25_otp_image);
				read_buf_from_file(en25qh128_otp_contents, 512,
						   emu_persistent_en25_otp_image);
			} else {
				msg_pdbg("doesn't match.\n");
			}
		}
	}

	/* W25Q40.V has 3 Security Registers each of 256 bytes. */
	if (emu_chip == EMULATE_WINBOND_W25Q40_V) {
		msg_pdbg("Filling fake security register 1, 2 and 3 with 0xff, size 256 bytes each\n");
		memset(w25q40v_otp_1_contents, 0xff, 256);
		memset(w25q40v_otp_2_contents, 0xff, 256);
		memset(w25q40v_otp_3_contents, 0xff, 256);

		emu_persistent_w25_otp_1_image = extract_programmer_param("otp_1");
		emu_persistent_w25_otp_2_image = extract_programmer_param("otp_2");
		emu_persistent_w25_otp_3_image = extract_programmer_param("otp_3");

		if (emu_persistent_w25_otp_1_image && !stat(emu_persistent_w25_otp_1_image, &image_stat)) {
			msg_pdbg("Found persistent image %s, %jd B for security register 1, ",
				 emu_persistent_w25_otp_1_image, (intmax_t)image_stat.st_size);
			if (image_stat.st_size == 256) {
				msg_pdbg("matches.\n");
				msg_pdbg("Reading %s\n", emu_persistent_w25_otp_1_image);
				read_buf_from_file(w25q40v_otp_1_contents, 256,
						   emu_persistent_w25_otp_1_image);
			} else {
				msg_pdbg("doesn't match.\n");
			}
		}
		if (emu_persistent_w25_otp_2_image && !stat(emu_persistent_w25_otp_2_image, &image_stat)) {
			msg_pdbg("Found persistent image %s, %jd B for security register 2, ",
				 emu_persistent_w25_otp_2_image, (intmax_t)image_stat.st_size);
			if (image_stat.st_size == 256) {
				msg_pdbg("matches.\n");
				msg_pdbg("Reading %s\n", emu_persistent_w25_otp_2_image);
				read_buf_from_file(w25q40v_otp_2_contents, 256,
						   emu_persistent_w25_otp_2_image);
			} else {
				msg_pdbg("doesn't match.\n");
			}
		}
		if (emu_persistent_w25_otp_3_image && !stat(emu_persistent_w25_otp_3_image, &image_stat)) {
			msg_pdbg("Found persistent image %s, %jd B for security register 3, ",
				 emu_persistent_w25_otp_3_image, (intmax_t)image_stat.st_size);
			if (image_stat.st_size == 256) {
				msg_pdbg("matches.\n");
				msg_pdbg("Reading %s\n", emu_persistent_w25_otp_3_image);
				read_buf_from_file(w25q40v_otp_3_contents, 256,
						   emu_persistent_w25_otp_3_image);
			} else {
				msg_pdbg("doesn't match.\n");
			}
		}
	}
#endif

dummy_init_out:
	if (register_shutdown(dummy_shutdown, NULL)) {
		free(flashchip_contents);
		if (emu_chip == EMULATE_GIGADEVICE_GD25Q128C) {
			free(gd25q128c_otp_1_contents);
			free(gd25q128c_otp_2_contents);
			free(gd25q128c_otp_3_contents);
		}
		if (emu_chip == EMULATE_EON_EN25QH128) {
			free(en25qh128_otp_contents);
		}
		if (emu_chip == EMULATE_WINBOND_W25Q40_V) {
			free(w25q40v_otp_1_contents);
			free(w25q40v_otp_2_contents);
			free(w25q40v_otp_3_contents);
		}
		return 1;
	}
	if (dummy_buses_supported & (BUS_PARALLEL | BUS_LPC | BUS_FWH))
		register_par_master(&par_master_dummy,
				    dummy_buses_supported & (BUS_PARALLEL | BUS_LPC | BUS_FWH));
	if (dummy_buses_supported & BUS_SPI)
		register_spi_master(&spi_master_dummyflasher);

	return 0;
}

void *dummy_map(const char *descr, uintptr_t phys_addr, size_t len)
{
	msg_pspew("%s: Mapping %s, 0x%zx bytes at 0x%0*" PRIxPTR "\n",
		  __func__, descr, len, PRIxPTR_WIDTH, phys_addr);
	return (void *)phys_addr;
}

void dummy_unmap(void *virt_addr, size_t len)
{
	msg_pspew("%s: Unmapping 0x%zx bytes at %p\n", __func__, len, virt_addr);
}

static void dummy_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr)
{
	msg_pspew("%s: addr=0x%" PRIxPTR ", val=0x%02x\n", __func__, addr, val);
}

static void dummy_chip_writew(const struct flashctx *flash, uint16_t val, chipaddr addr)
{
	msg_pspew("%s: addr=0x%" PRIxPTR ", val=0x%04x\n", __func__, addr, val);
}

static void dummy_chip_writel(const struct flashctx *flash, uint32_t val, chipaddr addr)
{
	msg_pspew("%s: addr=0x%" PRIxPTR ", val=0x%08x\n", __func__, addr, val);
}

static void dummy_chip_writen(const struct flashctx *flash, const uint8_t *buf, chipaddr addr, size_t len)
{
	size_t i;
	msg_pspew("%s: addr=0x%" PRIxPTR ", len=0x%zx, writing data (hex):", __func__, addr, len);
	for (i = 0; i < len; i++) {
		if ((i % 16) == 0)
			msg_pspew("\n");
		msg_pspew("%02x ", buf[i]);
	}
}

static uint8_t dummy_chip_readb(const struct flashctx *flash, const chipaddr addr)
{
	msg_pspew("%s:  addr=0x%" PRIxPTR ", returning 0xff\n", __func__, addr);
	return 0xff;
}

static uint16_t dummy_chip_readw(const struct flashctx *flash, const chipaddr addr)
{
	msg_pspew("%s:  addr=0x%" PRIxPTR ", returning 0xffff\n", __func__, addr);
	return 0xffff;
}

static uint32_t dummy_chip_readl(const struct flashctx *flash, const chipaddr addr)
{
	msg_pspew("%s:  addr=0x%" PRIxPTR ", returning 0xffffffff\n", __func__, addr);
	return 0xffffffff;
}

static void dummy_chip_readn(const struct flashctx *flash, uint8_t *buf, const chipaddr addr, size_t len)
{
	msg_pspew("%s:  addr=0x%" PRIxPTR ", len=0x%zx, returning array of 0xff\n", __func__, addr, len);
	memset(buf, 0xff, len);
	return;
}

#if EMULATE_SPI_CHIP

static int emulate_spi_chip_response(unsigned int writecnt,
				     unsigned int readcnt,
				     const unsigned char *writearr,
				     unsigned char *readarr)
{
	unsigned int offs, i, toread;
	static int unsigned aai_offs;
	const unsigned char sst25vf040_rems_response[2] = {0xbf, 0x44};
	const unsigned char sst25vf032b_rems_response[2] = {0xbf, 0x4a};
	const unsigned char mx25l6436_rems_response[2] = {0xc2, 0x16};
	const unsigned char gd25q128c_rems_response[2] = {0xc8, 0x17};
	const unsigned char en25qh128_rems_response[2] = {0x1c, 0x17};
	const unsigned char w25q40v_rems_response[2] = {0xef, 0x12};

	if (writecnt == 0) {
		msg_perr("No command sent to the chip!\n");
		return 1;
	}
	/* spi_blacklist has precedence over spi_ignorelist. */
	for (i = 0; i < spi_blacklist_size; i++) {
		if (writearr[0] == spi_blacklist[i]) {
			msg_pdbg("Refusing blacklisted SPI command 0x%02x\n",
				 spi_blacklist[i]);
			return SPI_INVALID_OPCODE;
		}
	}
	for (i = 0; i < spi_ignorelist_size; i++) {
		if (writearr[0] == spi_ignorelist[i]) {
			msg_cdbg("Ignoring ignorelisted SPI command 0x%02x\n",
				 spi_ignorelist[i]);
			/* Return success because the command does not fail,
			 * it is simply ignored.
			 */
			return 0;
		}
	}

	if (emu_max_aai_size && (emu_status & SPI_SR_AAI)) {
		if (writearr[0] != JEDEC_AAI_WORD_PROGRAM &&
		    writearr[0] != JEDEC_WRDI &&
		    writearr[0] != JEDEC_RDSR) {
			msg_perr("Forbidden opcode (0x%02x) attempted during "
				 "AAI sequence!\n", writearr[0]);
			return 0;
		}
	}

	switch (writearr[0]) {
	case JEDEC_RES:
		if (writecnt < JEDEC_RES_OUTSIZE)
			break;
		/* offs calculation is only needed for SST chips which treat RES like REMS. */
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		offs += writecnt - JEDEC_REMS_OUTSIZE;
		switch (emu_chip) {
		case EMULATE_ST_M25P10_RES:
			if (readcnt > 0)
				memset(readarr, 0x10, readcnt);
			break;
		case EMULATE_SST_SST25VF040_REMS:
			for (i = 0; i < readcnt; i++)
				readarr[i] = sst25vf040_rems_response[(offs + i) % 2];
			break;
		case EMULATE_SST_SST25VF032B:
			for (i = 0; i < readcnt; i++)
				readarr[i] = sst25vf032b_rems_response[(offs + i) % 2];
			break;
		case EMULATE_MACRONIX_MX25L6436:
			if (readcnt > 0)
				memset(readarr, 0x16, readcnt);
			break;
		case EMULATE_GIGADEVICE_GD25Q128C:
			if (readcnt > 0)
				memset(readarr, 0x17, readcnt);
			break;
		case EMULATE_EON_EN25QH128:
			if (readcnt > 0)
				memset(readarr, 0x17, readcnt);
			break;
		case EMULATE_WINBOND_W25Q40_V:
			if (readcnt > 0)
				memset(readarr, 0x12, readcnt);
			break;
		default: /* ignore */
			break;
		}
		break;
	case JEDEC_REMS:
		/* REMS response has wraparound and uses an address parameter. */
		if (writecnt < JEDEC_REMS_OUTSIZE)
			break;
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		offs += writecnt - JEDEC_REMS_OUTSIZE;
		switch (emu_chip) {
		case EMULATE_SST_SST25VF040_REMS:
			for (i = 0; i < readcnt; i++)
				readarr[i] = sst25vf040_rems_response[(offs + i) % 2];
			break;
		case EMULATE_SST_SST25VF032B:
			for (i = 0; i < readcnt; i++)
				readarr[i] = sst25vf032b_rems_response[(offs + i) % 2];
			break;
		case EMULATE_MACRONIX_MX25L6436:
			for (i = 0; i < readcnt; i++)
				readarr[i] = mx25l6436_rems_response[(offs + i) % 2];
			break;
		case EMULATE_GIGADEVICE_GD25Q128C:
			for (i = 0; i < readcnt; i++)
				readarr[i] = gd25q128c_rems_response[(offs + i) % 2];
			break;
		case EMULATE_EON_EN25QH128:
			for (i = 0; i < readcnt; i++)
				readarr[i] = en25qh128_rems_response[(offs + i) % 2];
			break;
		case EMULATE_WINBOND_W25Q40_V:
			for (i = 0; i < readcnt; i++)
				readarr[i] = w25q40v_rems_response[(offs + i) % 2];
			break;
		default: /* ignore */
			break;
		}
		break;
	case JEDEC_RDID:
		switch (emu_chip) {
		case EMULATE_SST_SST25VF032B:
			if (readcnt > 0)
				readarr[0] = 0xbf;
			if (readcnt > 1)
				readarr[1] = 0x25;
			if (readcnt > 2)
				readarr[2] = 0x4a;
			break;
		case EMULATE_MACRONIX_MX25L6436:
			if (readcnt > 0)
				readarr[0] = 0xc2;
			if (readcnt > 1)
				readarr[1] = 0x20;
			if (readcnt > 2)
				readarr[2] = 0x17;
			break;
		case EMULATE_GIGADEVICE_GD25Q128C:
			if (readcnt > 0)
				readarr[0] = 0xc8;
			if (readcnt > 1)
				readarr[1] = 0x40;
			if (readcnt > 2)
				readarr[2] = 0x18;
			break;
		case EMULATE_EON_EN25QH128:
			if (readcnt > 0)
				readarr[0] = 0x1c;
			if (readcnt > 1)
				readarr[1] = 0x70;
			if (readcnt > 2)
				readarr[2] = 0x18;
			break;
		case EMULATE_WINBOND_W25Q40_V:
			if (readcnt > 0)
				readarr[0] = 0xef;
			if (readcnt > 1)
				readarr[1] = 0x40;
			if (readcnt > 2)
				readarr[2] = 0x13;
			break;
		default: /* ignore */
			break;
		}
		break;
	case JEDEC_RDSR:
		if (emu_chip == EMULATE_EON_EN25QH128 && en25qh128_otp_mode) {
			memset(readarr, (emu_status & 0x7F) | (en25qh128_otp_bit << 7), readcnt);
			break;
		}
		memset(readarr, emu_status, readcnt);
		break;
	case JEDEC_RDSR2:
		if (emu_chip != EMULATE_GIGADEVICE_GD25Q128C && emu_chip != EMULATE_WINBOND_W25Q40_V)
			break;
		memset(readarr, (emu_status >> 8) & 0xff, readcnt);
		break;
	case JEDEC_RDSR3:
		if (emu_chip != EMULATE_GIGADEVICE_GD25Q128C)
			break;
		memset(readarr, (emu_status >> 16) & 0xff, readcnt);
		break;
	/* FIXME: this should be chip-specific. */
	case JEDEC_EWSR:
	case JEDEC_WREN:
		emu_status |= SPI_SR_WEL;
		break;
	case JEDEC_WRSR1:
		if (!(emu_status & SPI_SR_WEL)) {
			msg_perr("WRSR1 attempted, but WEL is 0!\n");
			break;
		}
		if (emu_chip == EMULATE_EON_EN25QH128 && en25qh128_otp_mode) {
			en25qh128_otp_bit = 1;
			msg_pdbg("OTP bit set...\n");
			break;
		}
		if (emu_chip == EMULATE_WINBOND_W25Q40_V) {
			/* Make sure reserved bits and read-only bits are not set.
			 * For W25Q40.V, SUS (bit_7) and reserved (bit_2) bits in SR2 are read-only. */
			read_only_bits = 0x84;
			/* If any of the Lock Bits in SR2, LB[1..3] are set, then they are read-only. */
			if (emu_status & 0x00ff00 & (1 << 3))
				read_only_bits |= 1 << 3;
			if (emu_status & 0x00ff00 & (1 << 4))
				read_only_bits |= 1 << 4;
			if (emu_status & 0x00ff00 & (1 << 5))
				read_only_bits |= 1 << 5;
			if (writecnt == 3)
				emu_status |= ((writearr[2] & ~read_only_bits) & 0xff) << 8;
			else
				emu_status &= 0x00ff;
		}
		/* FIXME: add some reasonable simulation of the busy flag */
		emu_status |= (writearr[1] & ~SPI_SR_WIP) & 0xffff;
		msg_pdbg2("WRSR1 wrote 0x%02x.\n", emu_status & 0xffff);
		break;
	case JEDEC_WRSR2:
		if (emu_chip != EMULATE_GIGADEVICE_GD25Q128C)
			break;
		if (!(emu_status & SPI_SR_WEL)) {
			msg_perr("WRSR2 attempted, but WEL is 0!\n");
			break;
		}
		/* FIXME: add some reasonable simulation of the busy flag */
		/* Make sure reserved bits and read-only bits are not set.
		 * For GD25Q128C, SUS1 (bit_7) and SUS2 (bit_2) bits in SR2 are read-only. */
		read_only_bits = 0x84;
		/* If any of the Lock Bits, LB[1..3] are set, then they are read-only. */
		if (emu_status & 0x00ff00 & (1 << 3))
			read_only_bits |= 1 << 3;
		if (emu_status & 0x00ff00 & (1 << 4))
			read_only_bits |= 1 << 4;
		if (emu_status & 0x00ff00 & (1 << 5))
			read_only_bits |= 1 << 5;
		emu_status |= (writearr[1] & ~read_only_bits) & 0xff00;
		break;
	case JEDEC_WRSR3:
		if (emu_chip != EMULATE_GIGADEVICE_GD25Q128C)
			break;
		if (!(emu_status & SPI_SR_WEL)) {
			msg_perr("WRSR3 attempted, but WEL is 0!\n");
			break;
		}
		/* FIXME: add some reasonable simulation of the busy flag */
		/* Make sure reserved bits and read-only bits are not set.
		 * For GD25Q128C, bit_{0, 1, 3, 4} in SR3 are reserved. */
		read_only_bits = 0x1b;
		emu_status |= (writearr[1] & ~read_only_bits) &0xff0000;
		break;
	case JEDEC_READ:
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		if (emu_chip == EMULATE_EON_EN25QH128 && en25qh128_otp_mode) {
			if (en25qh128_otp_bit) {
				msg_perr("OTP bit is set, cannot erase OTP sector anymore\n");
				break;
			}
			if ((~(offs >> 12) & 0xfff) || (offs & 0xfff) >= 0x200) {
				msg_perr("Address out of range\n");
				break;
			}
			memcpy(readarr, en25qh128_otp_contents + (offs & 0xfff), readcnt);
			break;
		}
		/* Truncate to emu_chip_size. */
		offs %= emu_chip_size;
		if (readcnt > 0)
			memcpy(readarr, flashchip_contents + offs, readcnt);
		break;
	case JEDEC_BYTE_PROGRAM:
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		if (emu_chip == EMULATE_EON_EN25QH128 && en25qh128_otp_mode) {
			if (en25qh128_otp_bit) {
				msg_perr("OTP bit is set, cannot program OTP sector anymore\n");
				break;
			}
			if ((~(offs >> 12) & 0xfff) || (offs & 0xfff) >= 0x200) {
				msg_perr("Address out of range\n");
				break;
			}
			memcpy(en25qh128_otp_contents + (offs & 0xfff), writearr + 4, writecnt - 4);
			break;
		}
		/* Truncate to emu_chip_size. */
		offs %= emu_chip_size;
		if (writecnt < 5) {
			msg_perr("BYTE PROGRAM size too short!\n");
			return 1;
		}
		if (writecnt - 4 > emu_max_byteprogram_size) {
			msg_perr("Max BYTE PROGRAM size exceeded!\n");
			return 1;
		}
		memcpy(flashchip_contents + offs, writearr + 4, writecnt - 4);
		break;
	case JEDEC_AAI_WORD_PROGRAM:
		if (!emu_max_aai_size)
			break;
		if (!(emu_status & SPI_SR_AAI)) {
			if (writecnt < JEDEC_AAI_WORD_PROGRAM_OUTSIZE) {
				msg_perr("Initial AAI WORD PROGRAM size too "
					 "short!\n");
				return 1;
			}
			if (writecnt > JEDEC_AAI_WORD_PROGRAM_OUTSIZE) {
				msg_perr("Initial AAI WORD PROGRAM size too "
					 "long!\n");
				return 1;
			}
			emu_status |= SPI_SR_AAI;
			aai_offs = writearr[1] << 16 | writearr[2] << 8 |
				   writearr[3];
			/* Truncate to emu_chip_size. */
			aai_offs %= emu_chip_size;
			memcpy(flashchip_contents + aai_offs, writearr + 4, 2);
			aai_offs += 2;
		} else {
			if (writecnt < JEDEC_AAI_WORD_PROGRAM_CONT_OUTSIZE) {
				msg_perr("Continuation AAI WORD PROGRAM size "
					 "too short!\n");
				return 1;
			}
			if (writecnt > JEDEC_AAI_WORD_PROGRAM_CONT_OUTSIZE) {
				msg_perr("Continuation AAI WORD PROGRAM size "
					 "too long!\n");
				return 1;
			}
			memcpy(flashchip_contents + aai_offs, writearr + 1, 2);
			aai_offs += 2;
		}
		break;
	case JEDEC_WRDI:
		if (emu_max_aai_size)
			emu_status &= ~SPI_SR_AAI;
		if (emu_chip == EMULATE_EON_EN25QH128)
			en25qh128_otp_mode = 0;
		break;
	case JEDEC_SE:
		if (!emu_jedec_se_size)
			break;
		if (writecnt != JEDEC_SE_OUTSIZE) {
			msg_perr("SECTOR ERASE 0x20 outsize invalid!\n");
			return 1;
		}
		if (readcnt != JEDEC_SE_INSIZE) {
			msg_perr("SECTOR ERASE 0x20 insize invalid!\n");
			return 1;
		}
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		if (emu_chip == EMULATE_EON_EN25QH128 && en25qh128_otp_mode) {
			if (en25qh128_otp_bit) {
				msg_perr("OTP bit is set, cannot erase OTP sector anymore\n");
				break;
			}
			if ((~(offs >> 12) & 0xfff) || (offs & 0xfff) >= 0x200) {
				msg_perr("Address out of range\n");
				break;
			}
			memset(en25qh128_otp_contents, 0xff, 512);
			break;
		}
		if (offs & (emu_jedec_se_size - 1))
			msg_pdbg("Unaligned SECTOR ERASE 0x20: 0x%x\n", offs);
		offs &= ~(emu_jedec_se_size - 1);
		memset(flashchip_contents + offs, 0xff, emu_jedec_se_size);
		break;
	case JEDEC_BE_52:
		if (!emu_jedec_be_52_size)
			break;
		if (writecnt != JEDEC_BE_52_OUTSIZE) {
			msg_perr("BLOCK ERASE 0x52 outsize invalid!\n");
			return 1;
		}
		if (readcnt != JEDEC_BE_52_INSIZE) {
			msg_perr("BLOCK ERASE 0x52 insize invalid!\n");
			return 1;
		}
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		if (offs & (emu_jedec_be_52_size - 1))
			msg_pdbg("Unaligned BLOCK ERASE 0x52: 0x%x\n", offs);
		offs &= ~(emu_jedec_be_52_size - 1);
		memset(flashchip_contents + offs, 0xff, emu_jedec_be_52_size);
		break;
	case JEDEC_BE_D8:
		if (!emu_jedec_be_d8_size)
			break;
		if (writecnt != JEDEC_BE_D8_OUTSIZE) {
			msg_perr("BLOCK ERASE 0xd8 outsize invalid!\n");
			return 1;
		}
		if (readcnt != JEDEC_BE_D8_INSIZE) {
			msg_perr("BLOCK ERASE 0xd8 insize invalid!\n");
			return 1;
		}
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		if (offs & (emu_jedec_be_d8_size - 1))
			msg_pdbg("Unaligned BLOCK ERASE 0xd8: 0x%x\n", offs);
		offs &= ~(emu_jedec_be_d8_size - 1);
		memset(flashchip_contents + offs, 0xff, emu_jedec_be_d8_size);
		break;
	case JEDEC_CE_60:
		if (!emu_jedec_ce_60_size)
			break;
		if (writecnt != JEDEC_CE_60_OUTSIZE) {
			msg_perr("CHIP ERASE 0x60 outsize invalid!\n");
			return 1;
		}
		if (readcnt != JEDEC_CE_60_INSIZE) {
			msg_perr("CHIP ERASE 0x60 insize invalid!\n");
			return 1;
		}
		/* JEDEC_CE_60_OUTSIZE is 1 (no address) -> no offset. */
		/* emu_jedec_ce_60_size is emu_chip_size. */
		memset(flashchip_contents, 0xff, emu_jedec_ce_60_size);
		break;
	case JEDEC_CE_C7:
		if (!emu_jedec_ce_c7_size)
			break;
		if (writecnt != JEDEC_CE_C7_OUTSIZE) {
			msg_perr("CHIP ERASE 0xc7 outsize invalid!\n");
			return 1;
		}
		if (readcnt != JEDEC_CE_C7_INSIZE) {
			msg_perr("CHIP ERASE 0xc7 insize invalid!\n");
			return 1;
		}
		/* JEDEC_CE_C7_OUTSIZE is 1 (no address) -> no offset. */
		/* emu_jedec_ce_c7_size is emu_chip_size. */
		memset(flashchip_contents, 0xff, emu_jedec_ce_c7_size);
		break;
	case JEDEC_SFDP:
		// TODO(hatim): SFDP for GD25Q128C, EN25QH128 and W25Q40.V
		if (emu_chip != EMULATE_MACRONIX_MX25L6436)
			break;
		if (writecnt < 4)
			break;
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];

		/* SFDP expects one dummy byte after the address. */
		if (writecnt == 4) {
			/* The dummy byte was not written, make sure it is read instead.
			 * Shifting and shortening the read array does achieve this goal.
			 */
			readarr++;
			readcnt--;
		} else {
			/* The response is shifted if more than 5 bytes are written, because SFDP data is
			 * already shifted out by the chip while those superfluous bytes are written. */
			offs += writecnt - 5;
		}

		/* The SFDP spec implies that the start address of an SFDP read may be truncated to fit in the
		 * SFDP table address space, i.e. the start address may be wrapped around at SFDP table size.
		 * This is a reasonable implementation choice in hardware because it saves a few gates. */
		if (offs >= sizeof(sfdp_table)) {
			msg_pdbg("Wrapping the start address around the SFDP table boundary (using 0x%x "
				 "instead of 0x%x).\n", (unsigned int)(offs % sizeof(sfdp_table)), offs);
			offs %= sizeof(sfdp_table);
		}
		toread = min(sizeof(sfdp_table) - offs, readcnt);
		memcpy(readarr, sfdp_table + offs, toread);
		if (toread < readcnt)
			msg_pdbg("Crossing the SFDP table boundary in a single "
				 "continuous chunk produces undefined results "
				 "after that point.\n");
		break;
	case JEDEC_READ_SEC_REG:
		if (emu_chip != EMULATE_GIGADEVICE_GD25Q128C && emu_chip != EMULATE_WINBOND_W25Q40_V)
			break;
		if (writecnt != JEDEC_READ_SEC_REG_OUTSIZE) {
			msg_perr("READ SECURITY REGISTER size not proper!\n");
			break;
		}
		/* writearr[1..3] holds the address, writearr[1] must be 0x00,
		 * (writearr[2..3] & 01ff) holds the byte address pointing to within
		 * the security register range, and (writearr[2] & 0xf0) must be either
		 * of 0x01, 0x02 or 0x03 corresponding to security register 1, 2 or 3 resp. */
		if (writearr[1] || (writearr[2] & 0x0e))
			break;
		offs = (writearr[2] & 0x01) << 8 | writearr[3];
		/* Truncate to security register size, i.e., 512 bytes for GD25Q128C,
		 * or 256 bytes for W25Q40.V. */
		offs %= (emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ? 512 : 256;
		if (readcnt > 0) {
			switch ((writearr[2] & 0xf0) >> 4) {
			case 0x01:
				memcpy(readarr, ((emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ?
					gd25q128c_otp_1_contents : w25q40v_otp_1_contents) + offs, readcnt);
				break;
			case 0x02:
				memcpy(readarr, ((emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ?
					gd25q128c_otp_2_contents : w25q40v_otp_2_contents) + offs, readcnt);
				break;
			case 0x03:
				memcpy(readarr, ((emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ?
					gd25q128c_otp_3_contents : w25q40v_otp_3_contents) + offs, readcnt);
				break;
			default:
				break;
			}
		}
		break;
	case JEDEC_PROG_BYTE_SEC_REG:
		if (emu_chip != EMULATE_GIGADEVICE_GD25Q128C && emu_chip != EMULATE_WINBOND_W25Q40_V)
			break;
		if (writecnt < JEDEC_PROG_BYTE_SEC_REG_OUTSIZE) {
			msg_perr("PROGRAM SECURITY REGISTER size too short!\n");
			break;
		}
		/* writearr[1..3] holds the address, writearr[1] must be 0x00,
		 * (writearr[2..3] & 01ff) holds the byte address pointing to within
		 * the security register range, and (writearr[2] & 0xf0) must be either
		 * of 0x01, 0x02 or 0x03 corresponding to security register 1, 2 or 3 resp. */
		if (writearr[1] || (writearr[2] & 0x0e))
			break;
		offs = (writearr[2] & 0x01) << 8 | writearr[3];
		/* Truncate to security register size, i.e., 512 bytes for GD25Q128C,
		 * or 256 bytes for W25Q40.V. */
		offs %= (emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ? 512 : 256;
		/* If corresponding Lock Bits are set then the register is locked against
		 * any further write attempts. */
		switch ((writearr[2] & 0xf0) >> 4) {
		case 0x01:
			/* LB1 is NOT set */
			if (!(emu_status & 0x00ff00 & (1 << 3)))
				memcpy(((emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ?
					gd25q128c_otp_1_contents : w25q40v_otp_1_contents) + offs,
					writearr + 4, writecnt - 4);
			break;
		case 0x02:
			/* LB2 is NOT set */
			if (!(emu_status & 0x00ff00 & (1 << 4)))
				memcpy(((emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ?
					gd25q128c_otp_2_contents : w25q40v_otp_2_contents) + offs,
					writearr + 4, writecnt - 4);
			break;
		case 0x03:
			/* LB3 is NOT set */
			if (!(emu_status & 0x00ff00 & (1 << 5)))
				memcpy(((emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ?
					gd25q128c_otp_3_contents : w25q40v_otp_3_contents) + offs,
					writearr + 4, writecnt - 4);
			break;
		default:
			break;
		}
		break;
	case JEDEC_ERASE_SEC_REG:
		if (emu_chip != EMULATE_GIGADEVICE_GD25Q128C && emu_chip != EMULATE_WINBOND_W25Q40_V)
			break;
		if (writecnt != JEDEC_ERASE_SEC_REG_OUTSIZE) {
			msg_perr("ERASE SECURITY REGISTER size not proper!\n");
			break;
		}
		/* writearr[1..3] holds the address, writearr[1] must be 0x00,
		 * (writearr[2..3] & 01ff) holds the byte address pointing to within
		 * the security register range, and (writearr[2] & 0xf0) must be either
		 * of 0x01, 0x02 or 0x03 corresponding to security register 1, 2 or 3 resp. */
		if (writearr[1] || (writearr[2] & 0x0e))
			break;
		/* If corresponding Lock Bits are set then the register is locked against
		 * any further erase attempts. */
		switch ((writearr[2] & 0xf0) >> 4) {
		case 0x01:
			/* LB1 is NOT set */
			if (!(emu_status & 0x00ff00 & (1 << 3)))
				memset((emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ?
					gd25q128c_otp_1_contents : w25q40v_otp_1_contents, 0xff,
					(emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ? 512 : 256);
			break;
		case 0x02:
			/* LB2 is NOT set */
			if (!(emu_status & 0x00ff00 & (1 << 4)))
				memset((emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ?
					gd25q128c_otp_2_contents : w25q40v_otp_2_contents, 0xff,
					(emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ? 512 : 256);
			break;
		case 0x03:
			/* LB3 is NOT set */
			if (!(emu_status & 0x00ff00 & (1 << 5)))
				memset((emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ?
					gd25q128c_otp_3_contents : w25q40v_otp_3_contents, 0xff,
					(emu_chip == EMULATE_GIGADEVICE_GD25Q128C) ? 512 : 256);
			break;
		default:
			break;
		}
		break;
	case JEDEC_ENTER_OTP:
		/* Eon chip specific opcode, not observed in other
		 * manufacturers (yet, please update this when required). */
		if (emu_chip != EMULATE_EON_EN25QH128)
			break;
		en25qh128_otp_mode = 1;
		msg_pdbg("Entered OTP mode...\n");
		break;
	default:
		/* No special response. */
		break;
	}
	if (writearr[0] != JEDEC_WREN && writearr[0] != JEDEC_EWSR)
		emu_status &= ~SPI_SR_WEL;
	return 0;
}
#endif

static int dummy_spi_send_command(struct flashctx *flash, unsigned int writecnt,
				  unsigned int readcnt,
				  const unsigned char *writearr,
				  unsigned char *readarr)
{
	int i;

	msg_pspew("%s:", __func__);

	msg_pspew(" writing %u bytes:", writecnt);
	for (i = 0; i < writecnt; i++)
		msg_pspew(" 0x%02x", writearr[i]);

	/* Response for unknown commands and missing chip is 0xff. */
	memset(readarr, 0xff, readcnt);
#if EMULATE_SPI_CHIP
	switch (emu_chip) {
	case EMULATE_ST_M25P10_RES:
	case EMULATE_SST_SST25VF040_REMS:
	case EMULATE_SST_SST25VF032B:
	case EMULATE_MACRONIX_MX25L6436:
	case EMULATE_GIGADEVICE_GD25Q128C:
	case EMULATE_EON_EN25QH128:
	case EMULATE_WINBOND_W25Q40_V:
		if (emulate_spi_chip_response(writecnt, readcnt, writearr,
					      readarr)) {
			msg_pdbg("Invalid command sent to flash chip!\n");
			return 1;
		}
		break;
	default:
		break;
	}
#endif
	msg_pspew(" reading %u bytes:", readcnt);
	for (i = 0; i < readcnt; i++)
		msg_pspew(" 0x%02x", readarr[i]);
	msg_pspew("\n");
	return 0;
}

static int dummy_spi_write_256(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	return spi_write_chunked(flash, buf, start, len,
				 spi_write_256_chunksize);
}
