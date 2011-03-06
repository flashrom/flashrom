/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Carl-Daniel Hailfinger
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
 *
 *
 * Header file for flash chip drivers. Included from flash.h.
 * As a general rule, every function listed here should take a pointer to
 * struct flashchip as first parameter.
 */

#ifndef __CHIPDRIVERS_H__
#define __CHIPDRIVERS_H__ 1

/* spi.c, should probably be in spi_chip.c */
int probe_spi_rdid(struct flashchip *flash);
int probe_spi_rdid4(struct flashchip *flash);
int probe_spi_rems(struct flashchip *flash);
int probe_spi_res1(struct flashchip *flash);
int probe_spi_res2(struct flashchip *flash);
int spi_write_enable(void);
int spi_write_disable(void);
int spi_block_erase_20(struct flashchip *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_52(struct flashchip *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_d7(struct flashchip *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_d8(struct flashchip *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_60(struct flashchip *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_c7(struct flashchip *flash, unsigned int addr, unsigned int blocklen);
int spi_chip_write_1(struct flashchip *flash, uint8_t *buf, int start, int len);
int spi_chip_write_256(struct flashchip *flash, uint8_t *buf, int start, int len);
int spi_chip_read(struct flashchip *flash, uint8_t *buf, int start, int len);
uint8_t spi_read_status_register(void);
int spi_prettyprint_status_register_at25df(struct flashchip *flash);
int spi_prettyprint_status_register_at25df_sec(struct flashchip *flash);
int spi_prettyprint_status_register_at25f(struct flashchip *flash);
int spi_prettyprint_status_register_at25fs010(struct flashchip *flash);
int spi_prettyprint_status_register_at25fs040(struct flashchip *flash);
int spi_disable_blockprotect(struct flashchip *flash);
int spi_disable_blockprotect_at25df(struct flashchip *flash);
int spi_disable_blockprotect_at25df_sec(struct flashchip *flash);
int spi_disable_blockprotect_at25f(struct flashchip *flash);
int spi_disable_blockprotect_at25fs010(struct flashchip *flash);
int spi_disable_blockprotect_at25fs040(struct flashchip *flash);
int spi_byte_program(int addr, uint8_t databyte);
int spi_nbyte_program(int addr, uint8_t *bytes, int len);
int spi_nbyte_read(int addr, uint8_t *bytes, int len);
int spi_read_chunked(struct flashchip *flash, uint8_t *buf, int start, int len, int chunksize);
int spi_write_chunked(struct flashchip *flash, uint8_t *buf, int start, int len, int chunksize);
int spi_aai_write(struct flashchip *flash, uint8_t *buf, int start, int len);

/* 82802ab.c */
uint8_t wait_82802ab(struct flashchip *flash);
int probe_82802ab(struct flashchip *flash);
int erase_block_82802ab(struct flashchip *flash, unsigned int page, unsigned int pagesize);
int write_82802ab(struct flashchip *flash, uint8_t *buf, int start, int len);
void print_status_82802ab(uint8_t status);
int unlock_82802ab(struct flashchip *flash);
int unlock_28f004s5(struct flashchip *flash);

/* jedec.c */
uint8_t oddparity(uint8_t val);
void toggle_ready_jedec(chipaddr dst);
void data_polling_jedec(chipaddr dst, uint8_t data);
int write_byte_program_jedec(chipaddr bios, uint8_t *src,
			     chipaddr dst);
int probe_jedec(struct flashchip *flash);
int write_jedec(struct flashchip *flash, uint8_t *buf, int start, int len);
int write_jedec_1(struct flashchip *flash, uint8_t *buf, int start, int len);
int erase_sector_jedec(struct flashchip *flash, unsigned int page, unsigned int pagesize);
int erase_block_jedec(struct flashchip *flash, unsigned int page, unsigned int blocksize);
int erase_chip_block_jedec(struct flashchip *flash, unsigned int page, unsigned int blocksize);

/* m29f400bt.c */
int probe_m29f400bt(struct flashchip *flash);
int block_erase_m29f400bt(struct flashchip *flash, unsigned int start, unsigned int len);
int block_erase_chip_m29f400bt(struct flashchip *flash, unsigned int start, unsigned int len);
int write_m29f400bt(struct flashchip *flash, uint8_t *buf, int start, int len);
void protect_m29f400bt(chipaddr bios);

/* pm49fl00x.c */
int unlock_49fl00x(struct flashchip *flash);
int lock_49fl00x(struct flashchip *flash);

/* sst28sf040.c */
int erase_chip_28sf040(struct flashchip *flash, unsigned int addr, unsigned int blocklen);
int erase_sector_28sf040(struct flashchip *flash, unsigned int address, unsigned int sector_size);
int write_28sf040(struct flashchip *flash, uint8_t *buf, int start, int len);
int unprotect_28sf040(struct flashchip *flash);
int protect_28sf040(struct flashchip *flash);

/* sst49lfxxxc.c */
int erase_sector_49lfxxxc(struct flashchip *flash, unsigned int address, unsigned int sector_size);
int unlock_49lfxxxc(struct flashchip *flash);

/* sst_fwhub.c */
int printlock_sst_fwhub(struct flashchip *flash);
int unlock_sst_fwhub(struct flashchip *flash);

/* w39.c */
int printlock_w39l040(struct flashchip * flash);
int printlock_w39v040a(struct flashchip *flash);
int printlock_w39v040b(struct flashchip *flash);
int printlock_w39v040c(struct flashchip *flash);
int printlock_w39v040fa(struct flashchip *flash);
int printlock_w39v040fb(struct flashchip *flash);
int printlock_w39v040fc(struct flashchip *flash);
int printlock_w39v080a(struct flashchip *flash);
int printlock_w39v080fa(struct flashchip *flash);
int printlock_w39v080fa_dual(struct flashchip *flash);
int unlock_w39v040fb(struct flashchip *flash);
int unlock_w39v080fa(struct flashchip *flash);

/* w29ee011.c */
int probe_w29ee011(struct flashchip *flash);

/* stm50flw0x0x.c */
int erase_sector_stm50flw0x0x(struct flashchip *flash, unsigned int block, unsigned int blocksize);
int erase_chip_stm50flw0x0x(struct flashchip *flash, unsigned int addr, unsigned int blocklen);
int unlock_stm50flw0x0x(struct flashchip *flash);

#endif /* !__CHIPDRIVERS_H__ */
