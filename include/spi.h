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

#ifndef __SPI_H__
#define __SPI_H__ 1

/*
 * Contains the generic SPI headers
 */

#define JEDEC_MAX_ADDR_LEN	0x04

/* Read Electronic ID */
#define JEDEC_RDID		0x9f
#define JEDEC_RDID_OUTSIZE	0x01
/* INSIZE may be 0x04 for some chips*/
#define JEDEC_RDID_INSIZE	0x03

/* Some ST M95X model */
#define ST_M95_RDID		0x83
#define ST_M95_RDID_3BA_OUTSIZE	0x04	/* 8b op, 24bit addr where size >64KiB */
#define ST_M95_RDID_2BA_OUTSIZE	0x03	/* 8b op, 16bit addr where size <=64KiB */
#define ST_M95_RDID_OUTSIZE_MAX 0x04	/* ST_M95_RDID_3BA_OUTSIZE */
#define ST_M95_RDID_INSIZE	0x03

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

/* Block Erase 0xdc is supported by Spansion chips, takes 4 byte address */
#define JEDEC_BE_DC		0xdc
#define JEDEC_BE_DC_OUTSIZE	0x05
#define JEDEC_BE_DC_INSIZE	0x00

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

/* Read Status Register 2 */
#define JEDEC_RDSR2		0x35
#define JEDEC_RDSR2_OUTSIZE	0x01
#define JEDEC_RDSR2_INSIZE	0x01

/* Read Status Register 3 */
#define JEDEC_RDSR3		0x15
#define JEDEC_RDSR3_OUTSIZE	0x01
#define JEDEC_RDSR3_INSIZE	0x01

/* Status Register Bits */
#define SPI_SR_WIP	(0x01 << 0)
#define SPI_SR_WEL	(0x01 << 1)
#define SPI_SR_ERA_ERR	(0x01 << 5)
#define SPI_SR_AAI	(0x01 << 6)

/* Write Status Enable */
#define JEDEC_EWSR		0x50
#define JEDEC_EWSR_OUTSIZE	0x01
#define JEDEC_EWSR_INSIZE	0x00

/* Write Status Register */
#define JEDEC_WRSR		0x01
#define JEDEC_WRSR_OUTSIZE	0x02
#define JEDEC_WRSR_INSIZE	0x00

/* Write Status Register 2 */
#define JEDEC_WRSR2		0x31
#define JEDEC_WRSR2_OUTSIZE	0x02
#define JEDEC_WRSR2_INSIZE	0x00

/* Write Status Register 3 */
#define JEDEC_WRSR3		0x11
#define JEDEC_WRSR3_OUTSIZE	0x02
#define JEDEC_WRSR3_INSIZE	0x00

/* Read Security Register */
#define JEDEC_RDSCUR		0x2b
#define JEDEC_RDSCUR_OUTSIZE	0x01
#define JEDEC_RDSCUR_INSIZE	0x01

/* Write Security Register */
#define JEDEC_WRSCUR		0x2f
#define JEDEC_WRSCUR_OUTSIZE	0x01
#define JEDEC_WRSCUR_INSIZE	0x00

/* Enter 4-byte Address Mode */
#define JEDEC_ENTER_4_BYTE_ADDR_MODE	0xB7

/* Exit 4-byte Address Mode */
#define JEDEC_EXIT_4_BYTE_ADDR_MODE	0xE9

/* Write Extended Address Register */
#define JEDEC_WRITE_EXT_ADDR_REG	0xC5
#define ALT_WRITE_EXT_ADDR_REG_17	0x17

/* Read Extended Address Register */
#define JEDEC_READ_EXT_ADDR_REG		0xC8
#define ALT_READ_EXT_ADDR_REG_16	0x16

/* Read the memory */
#define JEDEC_READ		0x03
#define JEDEC_READ_OUTSIZE	0x04
/*      JEDEC_READ_INSIZE : any length */

/* Read the memory (with delay after sending address) */
#define JEDEC_READ_FAST		0x0b

/* Write memory byte */
#define JEDEC_BYTE_PROGRAM		0x02
#define JEDEC_BYTE_PROGRAM_OUTSIZE	0x05
#define JEDEC_BYTE_PROGRAM_INSIZE	0x00

/* Write AAI word (SST25VF080B) */
#define JEDEC_AAI_WORD_PROGRAM			0xad
#define JEDEC_AAI_WORD_PROGRAM_OUTSIZE		0x06
#define JEDEC_AAI_WORD_PROGRAM_CONT_OUTSIZE	0x03
#define JEDEC_AAI_WORD_PROGRAM_INSIZE		0x00

/* Read the memory with 4-byte address
   From ANY mode (3-bytes or 4-bytes) it works with 4-byte address */
#define JEDEC_READ_4BA		0x13

/* Read the memory with 4-byte address (and delay after sending address)
   From ANY mode (3-bytes or 4-bytes) it works with 4-byte address */
#define JEDEC_READ_4BA_FAST	0x0c

/* Write memory byte with 4-byte address
   From ANY mode (3-bytes or 4-bytes) it works with 4-byte address */
#define JEDEC_BYTE_PROGRAM_4BA	0x12

/* Error codes */
#define SPI_GENERIC_ERROR	-1
#define SPI_INVALID_OPCODE	-2
#define SPI_INVALID_ADDRESS	-3
#define SPI_INVALID_LENGTH	-4
#define SPI_FLASHROM_BUG	-5
#define SPI_PROGRAMMER_ERROR	-6

void clear_spi_id_cache(void);

#endif		/* !__SPI_H__ */
