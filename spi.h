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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef __SPI_H__
#define __SPI_H__ 1

/*
 * Contains the generic SPI headers
 */

/* Read Electronic ID */
#define JEDEC_RDID		0x9f
#define JEDEC_RDID_OUTSIZE	0x01
/* INSIZE may be 0x04 for some chips*/
#define JEDEC_RDID_INSIZE	0x03

/* Some Atmel AT25F* models have bit 3 as don't care bit in commands */
#define AT25F_RDID		0x15	/* 0x15 or 0x1d */
#define AT25F_RDID_OUTSIZE	0x01
#define AT25F_RDID_INSIZE	0x02

/* Read Electronic Manufacturer Signature */
#define JEDEC_REMS		0x90
#define JEDEC_REMS_OUTSIZE	0x04
#define JEDEC_REMS_INSIZE	0x02

/* Read Serial Flash Discoverable Parameters (SFDP) */
#define JEDEC_SFDP		0x5a
#define JEDEC_SFDP_OUTSIZE	0x05	/* 8b op, 24b addr, 8b dummy */
/*      JEDEC_SFDP_INSIZE : any length */

/* Read Electronic Signature */
#define JEDEC_RES		0xab
#define JEDEC_RES_OUTSIZE	0x04
/* INSIZE may be 0x02 for some chips*/
#define JEDEC_RES_INSIZE	0x01

/* Write Enable */
#define JEDEC_WREN		0x06
#define JEDEC_WREN_OUTSIZE	0x01
#define JEDEC_WREN_INSIZE	0x00

/* Write Disable */
#define JEDEC_WRDI		0x04
#define JEDEC_WRDI_OUTSIZE	0x01
#define JEDEC_WRDI_INSIZE	0x00

/* Chip Erase 0x60 is supported by Macronix/SST chips. */
#define JEDEC_CE_60		0x60
#define JEDEC_CE_60_OUTSIZE	0x01
#define JEDEC_CE_60_INSIZE	0x00

/* Chip Erase 0x62 is supported by Atmel AT25F chips. */
#define JEDEC_CE_62		0x62
#define JEDEC_CE_62_OUTSIZE	0x01
#define JEDEC_CE_62_INSIZE	0x00

/* Chip Erase 0xc7 is supported by SST/ST/EON/Macronix chips. */
#define JEDEC_CE_C7		0xc7
#define JEDEC_CE_C7_OUTSIZE	0x01
#define JEDEC_CE_C7_INSIZE	0x00

/* Block Erase 0x50 is supported by Atmel AT26DF chips. */
#define JEDEC_BE_50		0x50
#define JEDEC_BE_50_OUTSIZE	0x04
#define JEDEC_BE_50_INSIZE	0x00

/* Block Erase 0x52 is supported by SST and old Atmel chips. */
#define JEDEC_BE_52		0x52
#define JEDEC_BE_52_OUTSIZE	0x04
#define JEDEC_BE_52_INSIZE	0x00

/* Block Erase 0x81 is supported by Atmel AT26DF chips. */
#define JEDEC_BE_81		0x81
#define JEDEC_BE_81_OUTSIZE	0x04
#define JEDEC_BE_81_INSIZE	0x00

/* Block Erase 0xc4 is supported by Micron chips. */
#define JEDEC_BE_C4		0xc4
#define JEDEC_BE_C4_OUTSIZE	0x04
#define JEDEC_BE_C4_INSIZE	0x00

/* Block Erase 0xd8 is supported by EON/Macronix chips. */
#define JEDEC_BE_D8		0xd8
#define JEDEC_BE_D8_OUTSIZE	0x04
#define JEDEC_BE_D8_INSIZE	0x00

/* Block Erase 0xd7 is supported by PMC chips. */
#define JEDEC_BE_D7		0xd7
#define JEDEC_BE_D7_OUTSIZE	0x04
#define JEDEC_BE_D7_INSIZE	0x00

/* Sector Erase 0x20 is supported by Macronix/SST chips. */
#define JEDEC_SE		0x20
#define JEDEC_SE_OUTSIZE	0x04
#define JEDEC_SE_INSIZE		0x00

/* Page Erase 0xDB */
#define JEDEC_PE		0xDB
#define JEDEC_PE_OUTSIZE	0x04
#define JEDEC_PE_INSIZE		0x00

/* Read Status Register */
#define JEDEC_RDSR		0x05
#define JEDEC_RDSR_OUTSIZE	0x01
#define JEDEC_RDSR_INSIZE	0x01

/* Status Register Bits */
#define SPI_SR_WIP	(0x01 << 0)
#define SPI_SR_WEL	(0x01 << 1)
#define SPI_SR_AAI	(0x01 << 6)

/* Write Status Enable */
#define JEDEC_EWSR		0x50
#define JEDEC_EWSR_OUTSIZE	0x01
#define JEDEC_EWSR_INSIZE	0x00

/* Write Status Register */
#define JEDEC_WRSR		0x01
#define JEDEC_WRSR_OUTSIZE	0x02
#define JEDEC_WRSR_INSIZE	0x00

/* Read the memory */
#define JEDEC_READ		0x03
#define JEDEC_READ_OUTSIZE	0x04
/*      JEDEC_READ_INSIZE : any length */

/* Write memory byte */
#define JEDEC_BYTE_PROGRAM		0x02
#define JEDEC_BYTE_PROGRAM_OUTSIZE	0x05
#define JEDEC_BYTE_PROGRAM_INSIZE	0x00

/* Write AAI word (SST25VF080B) */
#define JEDEC_AAI_WORD_PROGRAM			0xad
#define JEDEC_AAI_WORD_PROGRAM_OUTSIZE		0x06
#define JEDEC_AAI_WORD_PROGRAM_CONT_OUTSIZE	0x03
#define JEDEC_AAI_WORD_PROGRAM_INSIZE		0x00

/* Error codes */
#define SPI_GENERIC_ERROR	-1
#define SPI_INVALID_OPCODE	-2
#define SPI_INVALID_ADDRESS	-3
#define SPI_INVALID_LENGTH	-4
#define SPI_FLASHROM_BUG	-5
#define SPI_PROGRAMMER_ERROR	-6

#endif		/* !__SPI_H__ */
