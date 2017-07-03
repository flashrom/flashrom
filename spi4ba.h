/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2014 Boris Baykov
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
*/

/*
 * JEDEC flash chips instructions for 4-bytes addressing
 * SPI chip driver functions for 4-bytes addressing
 */

#ifndef __SPI_4BA_H__
#define __SPI_4BA_H__ 1

/* Enter 4-byte Address Mode */
#define JEDEC_ENTER_4_BYTE_ADDR_MODE		0xB7
#define JEDEC_ENTER_4_BYTE_ADDR_MODE_OUTSIZE	0x01
#define JEDEC_ENTER_4_BYTE_ADDR_MODE_INSIZE	0x00

/* Exit 4-byte Address Mode */
#define JEDEC_EXIT_4_BYTE_ADDR_MODE		0xE9
#define JEDEC_EXIT_4_BYTE_ADDR_MODE_OUTSIZE	0x01
#define JEDEC_EXIT_4_BYTE_ADDR_MODE_INSIZE	0x00

/* Write Extended Address Register */
#define JEDEC_WRITE_EXT_ADDR_REG		0xC5
#define JEDEC_WRITE_EXT_ADDR_REG_OUTSIZE	0x02
#define JEDEC_WRITE_EXT_ADDR_REG_INSIZE		0x00

/* Read Extended Address Register */
#define JEDEC_READ_EXT_ADDR_REG			0xC8
#define JEDEC_READ_EXT_ADDR_REG_OUTSIZE		0x01
#define JEDEC_READ_EXT_ADDR_REG_INSIZE		0x01

/* Read the memory with 4-byte address
   From ANY mode (3-bytes or 4-bytes) it works with 4-byte address */
#define JEDEC_READ_4BA		0x13
#define JEDEC_READ_4BA_OUTSIZE	0x05
/*      JEDEC_READ_4BA_INSIZE : any length */

/* Write memory byte with 4-byte address
   From ANY mode (3-bytes or 4-bytes) it works with 4-byte address */
#define JEDEC_BYTE_PROGRAM_4BA		0x12
#define JEDEC_BYTE_PROGRAM_4BA_OUTSIZE	0x06
#define JEDEC_BYTE_PROGRAM_4BA_INSIZE	0x00

/* Sector Erase 0x21 (with 4-byte address), usually 4k size.
   From ANY mode (3-bytes or 4-bytes) it works with 4-byte address */
#define JEDEC_SE_4BA		0x21
#define JEDEC_SE_4BA_OUTSIZE	0x05
#define JEDEC_SE_4BA_INSIZE	0x00

/* Block Erase 0x5C (with 4-byte address), usually 32k size.
   From ANY mode (3-bytes or 4-bytes) it works with 4-byte address */
#define JEDEC_BE_5C_4BA		0x5C
#define JEDEC_BE_5C_4BA_OUTSIZE	0x05
#define JEDEC_BE_5C_4BA_INSIZE	0x00

/* Block Erase 0xDC (with 4-byte address), usually 64k size.
   From ANY mode (3-bytes or 4-bytes) it works with 4-byte address */
#define JEDEC_BE_DC_4BA		0xdc
#define JEDEC_BE_DC_4BA_OUTSIZE	0x05
#define JEDEC_BE_DC_4BA_INSIZE	0x00

/* enter 4-bytes addressing mode */
int spi_enter_4ba_b7(struct flashctx *flash);
int spi_enter_4ba_b7_we(struct flashctx *flash);

/* exit 4-bytes addressing mode */
int spi_exit_4ba_e9(struct flashctx *flash);
int spi_exit_4ba_e9_we(struct flashctx *flash);

/* read/write flash bytes in 4-bytes addressing mode */
int spi_byte_program_4ba(struct flashctx *flash, unsigned int addr, uint8_t databyte);
int spi_nbyte_program_4ba(struct flashctx *flash, unsigned int addr, const uint8_t *bytes, unsigned int len);
int spi_nbyte_read_4ba(struct flashctx *flash, unsigned int addr, uint8_t *bytes, unsigned int len);

/* erase flash bytes in 4-bytes addressing mode */
int spi_block_erase_20_4ba(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_52_4ba(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_d8_4ba(struct flashctx *flash, unsigned int addr, unsigned int blocklen);

/* read/write flash bytes from 3-bytes addressing mode using extended address register */
int spi_byte_program_4ba_ereg(struct flashctx *flash, unsigned int addr, uint8_t databyte);
int spi_nbyte_program_4ba_ereg(struct flashctx *flash, unsigned int addr, const uint8_t *bytes, unsigned int len);
int spi_nbyte_read_4ba_ereg(struct flashctx *flash, unsigned int addr, uint8_t *bytes, unsigned int len);

/* erase flash bytes from 3-bytes addressing mode using extended address register */
int spi_block_erase_20_4ba_ereg(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_52_4ba_ereg(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_d8_4ba_ereg(struct flashctx *flash, unsigned int addr, unsigned int blocklen);

/* read/write flash bytes with 4-bytes address from any mode (3-byte or 4-byte) */
int spi_byte_program_4ba_direct(struct flashctx *flash, unsigned int addr, uint8_t databyte);
int spi_nbyte_program_4ba_direct(struct flashctx *flash, unsigned int addr, const uint8_t *bytes, unsigned int len);
int spi_nbyte_read_4ba_direct(struct flashctx *flash, unsigned int addr, uint8_t *bytes, unsigned int len);

/* erase flash bytes with 4-bytes address from any mode (3-byte or 4-byte) */
int spi_block_erase_21_4ba_direct(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_5c_4ba_direct(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_dc_4ba_direct(struct flashctx *flash, unsigned int addr, unsigned int blocklen);


#endif /* __SPI_4BA_H__ */
