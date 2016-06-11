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

/* enter 4-bytes addressing mode */
int spi_enter_4ba_b7(struct flashctx *flash);
int spi_enter_4ba_b7_we(struct flashctx *flash);

/* read/write flash bytes in 4-bytes addressing mode */
int spi_byte_program_4ba(struct flashctx *flash, unsigned int addr, uint8_t databyte);
int spi_nbyte_program_4ba(struct flashctx *flash, unsigned int addr, const uint8_t *bytes, unsigned int len);
int spi_nbyte_read_4ba(struct flashctx *flash, unsigned int addr, uint8_t *bytes, unsigned int len);

/* erase flash bytes in 4-bytes addressing mode */
int spi_block_erase_20_4ba(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_52_4ba(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_d8_4ba(struct flashctx *flash, unsigned int addr, unsigned int blocklen);


#endif /* __SPI_4BA_H__ */
