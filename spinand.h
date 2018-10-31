/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2007, 2008 Carl-Daniel Hailfinger
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

#ifndef __SPINAND_H__
#define __SPINAND_H__ 1

/*
 * Contains the SPI NAND headers
 */

/* de-facto standard for NAND flash */

#define SPINAND_ROW_ADDR_LEN 0x03
#define SPINAND_COLUMN_ADDR_LEN 0x02
#define SPINAND_MAX_PARAMETER_PAGE_SIZE 512

/* Read Page */
#define SPINAND_READ_PAGE		0x13
#define SPINAND_READ_PAGE_OUTSIZE	0x04
#define SPINAND_READ_PAGE_INSIZE	0x00

/* Read Cache (using n wires) */
#define SPINAND_READ_CACHE		0x03
/* #define SPINAND_READ_CACHE		0x0B */
#define SPINAND_READ_CACHE_X2		0x3B
#define SPINAND_READ_CACHE_X4		0x6B
#define SPINAND_READ_CACHE_OUTSIZE	0x04
#define SPINAND_READ_CACHE_INSIZE	0x00

/* Program Load */
#define SPINAND_PROGRAM_LOAD		0x02
#define SPINAND_PROGRAM_LOAD_OUTSIZE	0x04
#define SPINAND_PROGRAM_LOAD_INSIZE	0x00

/* Program Execute */
#define SPINAND_PROGRAM_EXECUTE		0x10
#define SPINAND_PROGRAM_EXECUTE_OUTSIZE	0x04
#define SPINAND_PROGRAM_EXECUTE_INSIZE	0x00

/* Protect Execute */
#define SPINAND_PROTECT_EXECUTE		0x2A
#define SPINAND_PROTECT_EXECUTE_OUTSIZE	0x04
#define SPINAND_PROTECT_EXECUTE_INSIZE	0x00

/* Program Load Random Data */
#define SPINAND_PLRD		0x84
#define SPINAND_PLRD_OUTSIZE	0x04
#define SPINAND_PLRD_INSIZE	0x00

/* Block Erase */
#define SPINAND_BE		0xD8
#define SPINAND_BE_OUTSIZE	0x04
#define SPINAND_BE_INSIZE	0x00

/* Write Enable */
#define SPINAND_WREN		0x06
#define SPINAND_WREN_OUTSIZE	0x01
#define SPINAND_WREN_INSIZE	0x00

/* Write Disable */
#define SPINAND_WRDI		0x04
#define SPINAND_WRDI_OUTSIZE	0x01
#define SPINAND_WRDI_INSIZE	0x00

/* Get Feature (Status register) */
#define SPINAND_GET_FEATURE		0x0F
#define SPINAND_GET_FEATURE_OUTSIZE	0x02
#define SPINAND_GET_FEATURE_INSIZE	0x00

/* Feature Table - B0h Address */
#define SPINAND_FEATURE_B0_IDR_E	(0x01 << 6)  // ID Read Enable
#define SPINAND_FEATURE_B0_ECC_E	(0x01 << 4)  // ECC Enable
#define SPINAND_FEATURE_B0_HSE	(0x01 << 1)  // High Speed Mode Enable

/* Feature Table - C0h Address */
#define SPINAND_FEATURE_C0_ECCS1	(0x01 << 5)  // ECC Status 1
#define SPINAND_FEATURE_C0_ECCS0	(0x01 << 4)  // ECC Status 0
#define SPINAND_FEATURE_C0_PRG_F	(0x01 << 3)  // Program Fail
#define SPINAND_FEATURE_C0_ERS_F	(0x01 << 2)  // Erase Fail
#define SPINAND_FEATURE_C0_WEL	(0x01 << 1)  // Write Enable Latch
#define SPINAND_FEATURE_C0_OIP	(0x01 << 0)  // Operation In Progress

/* Set Feature */
#define SPINAND_SET_FEATURE		0x1F
#define SPINAND_SET_FEATURE_OUTSIZE	0x03
#define SPINAND_SET_FEATURE_INSIZE	0x00

/* Parameter Page Magic */
const uint8_t spi_nand_magic_toshiba[] = {0x4E, 0x41, 0x4E, 0x44};  // NAND
const uint8_t spi_nand_magic_micron[] = {0x4F, 0x4E, 0x46, 0x49};  // ONFI

#endif		/* !__SPINAND_H__ */
