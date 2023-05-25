/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2022 Yangfl
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

#ifndef __NAND_H__
#define __NAND_H__

#include <inttypes.h>

/*
 * Contains the SPI NAND headers
 */

enum {SPI_NAND_HW_ECC = -1, SPI_NAND_SW_ECC0, SPI_NAND_SW_ECC1, SPI_NAND_SW_ECC2 };

/* NAND flags */
#define SPI_NAND_BUF	(1 << 0)

/* de-facto standard for NAND flash */

#define JEDEC_NAND_ROW_ADDR_LEN 0x03
#define JEDEC_NAND_COLUMN_ADDR_LEN 0x02

#define JEDEC_NAND_PARAMETER_PAGE_SIZE 512

#define JEDEC_NAND_PAGE_SIZE	2112

/* Read Page */
#define JEDEC_NAND_READ_PAGE		0x13
#define JEDEC_NAND_READ_PAGE_OUTSIZE	0x04
#define JEDEC_NAND_READ_PAGE_INSIZE	0x00

/* Read Cache (using n wires) */
#define JEDEC_NAND_READ_CACHE		0x03
/* #define JEDEC_NAND_READ_CACHE	0x0B */
#define JEDEC_NAND_READ_CACHE_X2	0x3B
#define JEDEC_NAND_READ_CACHE_X4	0x6B
#define JEDEC_NAND_READ_CACHE_OUTSIZE	0x04
#define JEDEC_NAND_READ_CACHE_INSIZE	0x00

/* Program Load */
#define JEDEC_NAND_PROGRAM_LOAD		0x02
#define JEDEC_NAND_PROGRAM_LOAD_OUTSIZE	0x03
#define JEDEC_NAND_PROGRAM_LOAD_INSIZE	0x00

/* Program Execute */
#define JEDEC_NAND_PROGRAM_EXECUTE		0x10
#define JEDEC_NAND_PROGRAM_EXECUTE_OUTSIZE	0x04
#define JEDEC_NAND_PROGRAM_EXECUTE_INSIZE	0x00

/* Protect Execute */
#define JEDEC_NAND_PROTECT_EXECUTE		0x2A
#define JEDEC_NAND_PROTECT_EXECUTE_OUTSIZE	0x04
#define JEDEC_NAND_PROTECT_EXECUTE_INSIZE	0x00

/* Program Load Random Data */
#define JEDEC_NAND_PLRD		0x84
#define JEDEC_NAND_PLRD_OUTSIZE	0x04
#define JEDEC_NAND_PLRD_INSIZE	0x00

/* Block Erase */
#define JEDEC_NAND_BE		0xD8
#define JEDEC_NAND_BE_OUTSIZE	0x04
#define JEDEC_NAND_BE_INSIZE	0x00

/* Write Enable */
#define JEDEC_NAND_WREN		0x06
#define JEDEC_NAND_WREN_OUTSIZE	0x01
#define JEDEC_NAND_WREN_INSIZE	0x00

/* Write Disable */
#define JEDEC_NAND_WRDI		0x04
#define JEDEC_NAND_WRDI_OUTSIZE	0x01
#define JEDEC_NAND_WRDI_INSIZE	0x00

/* Get Feature (Status register) */
#define JEDEC_NAND_GET_FEATURE		0x0F
#define JEDEC_NAND_GET_FEATURE_OUTSIZE	0x02
#define JEDEC_NAND_GET_FEATURE_INSIZE	0x00

#define JEDEC_NAND_REG_PROTECT	0xA0
#define JEDEC_NAND_REG_CONFIG	0xB0
#define JEDEC_NAND_REG_STATUS	0xC0

/* Feature Table - B0h Address */
#define JEDEC_NAND_FEATURE_B0_IDR_E	(0x01 << 6)  // ID Read Enable
#define JEDEC_NAND_FEATURE_B0_ECC_E	(0x01 << 4)  // ECC Enable
#define JEDEC_NAND_FEATURE_B0_BUF	(0x01 << 3)  // Buffer / Continuous Read Mode
#define JEDEC_NAND_FEATURE_B0_HSE	(0x01 << 1)  // High Speed Mode Enable

/* Feature Table - C0h Address */
#define JEDEC_NAND_FEATURE_C0_ECCS1	(0x01 << 5)  // ECC Status 1
#define JEDEC_NAND_FEATURE_C0_ECCS0	(0x01 << 4)  // ECC Status 0
#define JEDEC_NAND_FEATURE_C0_PRG_F	(0x01 << 3)  // Program Fail
#define JEDEC_NAND_FEATURE_C0_ERS_F	(0x01 << 2)  // Erase Fail
#define JEDEC_NAND_FEATURE_C0_WEL	(0x01 << 1)  // Write Enable Latch
#define JEDEC_NAND_FEATURE_C0_OIP	(0x01 << 0)  // Operation In Progress

/* Set Feature */
#define JEDEC_NAND_SET_FEATURE		0x1F
#define JEDEC_NAND_SET_FEATURE_OUTSIZE	0x03
#define JEDEC_NAND_SET_FEATURE_INSIZE	0x00

/* Parameter Page Magic */
const uint8_t spi_nand_magic0[] = {0x4E, 0x41, 0x4E, 0x44};  // NAND - Kioxia
const uint8_t spi_nand_magic1[] = {0x4F, 0x4E, 0x46, 0x49};  // ONFI - Micron/Winbond

struct nand_param_page {
	/* offset 0 */
	uint8_t  signature[4];		/* "NAND" */
	uint8_t  reversed4[28];		/* All 0 */

	/* offset 32 */
	uint8_t  manufacturer[12];	/* Device manufacturer: "TOSHIBA     ", space padding */
	uint8_t  model[20];		/* Device model, space padding */

	/* offset 64 */
	uint8_t  manufacture_id;	/* Manufacturer ID: 0x98 */
	uint8_t  reversed65[15];	/* All 0 */

	/* offset 80 */
	uint32_t page_size;		/* Number of data bytes per page */
	uint16_t spare_size;		/* Number of spare bytes per page */
	uint32_t partial_page_size;	/* Number of data bytes per partial page */
	uint16_t partial_spare_size;	/* Number of spare bytes per partial page */
	uint32_t block_pages;		/* Number of pages per block */
	uint32_t unit_blocks;		/* Number of blocks per unit */
	uint8_t  units;			/* Number of logical units */
	uint8_t  reversed101[1];	/* 0 */

	/* offset 102 */
	uint8_t  bits;			/* Number of bits per cell */
	uint16_t bad_blocks;		/* Bad blocks maximum per unit */
	uint16_t endurance;		/* Block endurance */
	uint8_t  guaranteed_blocks;	/* Guaranteed valid blocks at beginning of target */
	uint8_t  reversed108[2];	/* All 0 */
	uint8_t  programs;		/* Number of programs per page */
	uint8_t  reversed111[1];	/* 0 */
	uint8_t  ecc_bits;		/* Number of ECC bits */
	uint8_t  reversed113[15];	/* All 0 */

	/* offset 128 */
	uint8_t  capacitance;		/* I/O pin capacitance */
	uint8_t  reversed129[4];	/* All 0 */

	uint16_t tprog;			/* maximum page program time */
	uint16_t tberase;		/* maximum block erase time */
	uint16_t tr;			/* maximum page read time */
	uint8_t  reversed139[115];	/* All 0 */

	/* offset 254 */
	uint16_t crc;		/* Integrity CRC */
}  __attribute__((packed));

#endif		/* !__NAND_H__ */
