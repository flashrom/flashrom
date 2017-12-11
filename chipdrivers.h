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
 * struct flashctx as first parameter.
 */

#ifndef __CHIPDRIVERS_H__
#define __CHIPDRIVERS_H__ 1

#include "flash.h"	/* for chipaddr and flashctx */
#include "spi25_statusreg.h"	/* For enum status_register_num */
#include "writeprotect.h"

/* spi.c */
int spi_aai_write(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
int spi_chip_write_256(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
int spi_chip_read(struct flashctx *flash, uint8_t *buf, unsigned int start, int unsigned len);

/* spi25.c */
int probe_spi_rdid(struct flashctx *flash);
int probe_spi_rdid4(struct flashctx *flash);
int probe_spi_rems(struct flashctx *flash);
int probe_spi_res1(struct flashctx *flash);
int probe_spi_res2(struct flashctx *flash);
int probe_spi_res3(struct flashctx *flash);
int probe_spi_at25f(struct flashctx *flash);
int spi_write_enable(struct flashctx *flash);
int spi_write_disable(struct flashctx *flash);
int spi_block_erase_20(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_50(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_52(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_60(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_62(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_81(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_c4(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_c7(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_d7(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_d8(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_db(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
erasefunc_t *spi_get_erasefn_from_opcode(uint8_t opcode);
int spi_chip_write_1(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
int spi_byte_program(struct flashctx *flash, unsigned int addr, uint8_t databyte);
int spi_nbyte_program(struct flashctx *flash, unsigned int addr, const uint8_t *bytes, unsigned int len);
int spi_nbyte_read(struct flashctx *flash, unsigned int addr, uint8_t *bytes, unsigned int len);
int spi_read_chunked(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len, unsigned int chunksize);
int spi_write_chunked(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len, unsigned int chunksize);

/* spi25_statusreg.c */
uint8_t spi_read_status_register(struct flashctx *flash);
uint8_t spi_read_status_register_generic(struct flashctx *flash, enum status_register_num SRn);
int spi_write_status_register(struct flashctx *flash, int status);
int spi_write_status_register_generic(struct flashctx *flash, enum status_register_num SRn, uint8_t status);
enum status_register_num top_status_register(struct flashctx *flash);
char pos_bit(struct flashctx *flash, enum status_register_bit bit);
enum wp_mode get_wp_mode_generic(struct flashctx *flash);
int set_wp_mode_generic(struct flashctx *flash, enum wp_mode wp_mode);
int spi_prettyprint_status_register_generic(struct flashctx *flash, enum status_register_num SRn);
int spi_prettyprint_status_register_wp_generic(struct flashctx *flash);
void spi_prettyprint_status_register_bit(uint8_t status, int bit);
int spi_prettyprint_status_register_plain(struct flashctx *flash);
int spi_prettyprint_status_register_default_welwip(struct flashctx *flash);
int spi_prettyprint_status_register_bp1_srwd(struct flashctx *flash);
int spi_prettyprint_status_register_bp2_srwd(struct flashctx *flash);
int spi_prettyprint_status_register_bp3_srwd(struct flashctx *flash);
int spi_prettyprint_status_register_bp4_srwd(struct flashctx *flash);
int spi_prettyprint_status_register_bp2_bpl(struct flashctx *flash);
int spi_prettyprint_status_register_bp2_tb_bpl(struct flashctx *flash);
int spi_disable_blockprotect(struct flashctx *flash);
int spi_disable_blockprotect_bp1_srwd(struct flashctx *flash);
int spi_disable_blockprotect_bp2_srwd(struct flashctx *flash);
int spi_disable_blockprotect_bp3_srwd(struct flashctx *flash);
int spi_disable_blockprotect_bp4_srwd(struct flashctx *flash);
int spi_prettyprint_status_register_amic_a25l032(struct flashctx *flash);
int spi_prettyprint_status_register_at25df(struct flashctx *flash);
int spi_prettyprint_status_register_at25df_sec(struct flashctx *flash);
int spi_prettyprint_status_register_at25f(struct flashctx *flash);
int spi_prettyprint_status_register_at25f512a(struct flashctx *flash);
int spi_prettyprint_status_register_at25f512b(struct flashctx *flash);
int spi_prettyprint_status_register_at25f4096(struct flashctx *flash);
int spi_prettyprint_status_register_at25fs010(struct flashctx *flash);
int spi_prettyprint_status_register_at25fs040(struct flashctx *flash);
int spi_prettyprint_status_register_at26df081a(struct flashctx *flash);
int spi_disable_blockprotect_at2x_global_unprotect(struct flashctx *flash);
int spi_disable_blockprotect_at2x_global_unprotect_sec(struct flashctx *flash);
int spi_disable_blockprotect_at25f(struct flashctx *flash);
int spi_disable_blockprotect_at25f512a(struct flashctx *flash);
int spi_disable_blockprotect_at25f512b(struct flashctx *flash);
int spi_disable_blockprotect_at25fs010(struct flashctx *flash);
int spi_disable_blockprotect_at25fs040(struct flashctx *flash);
int spi_prettyprint_status_register_en25s_wp(struct flashctx *flash);
int spi_prettyprint_status_register_n25q(struct flashctx *flash);
int spi_disable_blockprotect_n25q(struct flashctx *flash);
int spi_prettyprint_status_register_bp2_ep_srwd(struct flashctx *flash);
int spi_disable_blockprotect_bp2_ep_srwd(struct flashctx *flash);
int spi_prettyprint_status_register_sst25(struct flashctx *flash);
int spi_prettyprint_status_register_sst25vf016(struct flashctx *flash);
int spi_prettyprint_status_register_sst25vf040b(struct flashctx *flash);

/* writeprotect.c */
struct range *sec_block_range_pattern(struct flashctx *flash);
char get_cmp(struct flashctx *flash);
int set_cmp(struct flashctx *flash, uint8_t cmp);
uint32_t bp_bitmask_generic(struct flashctx *flash);
struct range *bp_to_range(struct flashctx *flash, unsigned char bp_config);
int range_to_bp_bitfield(struct flashctx *flash, uint32_t start, uint32_t len);
int print_range_generic(struct flashctx *flash);
int print_table_generic(struct flashctx *flash);
int set_range_generic(struct flashctx *flash, uint32_t start, uint32_t len);
int disable_generic(struct flashctx *flash);
struct range *range_table_global(struct flashctx *flash);
struct range *a25l032_range_table(struct flashctx *flash);

/* sfdp.c */
int probe_spi_sfdp(struct flashctx *flash);

/* opaque.c */
int probe_opaque(struct flashctx *flash);
int read_opaque(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len);
int write_opaque(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
int erase_opaque(struct flashctx *flash, unsigned int blockaddr, unsigned int blocklen);

/* at45db.c */
int probe_spi_at45db(struct flashctx *flash);
int spi_prettyprint_status_register_at45db(struct flashctx *flash);
int spi_disable_blockprotect_at45db(struct flashctx *flash);
int spi_read_at45db(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len);
int spi_read_at45db_e8(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len);
int spi_write_at45db(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
int spi_erase_at45db_page(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_erase_at45db_block(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_erase_at45db_sector(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_erase_at45db_chip(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_erase_at45cs_sector(struct flashctx *flash, unsigned int addr, unsigned int blocklen);

/* 82802ab.c */
uint8_t wait_82802ab(struct flashctx *flash);
int probe_82802ab(struct flashctx *flash);
int erase_block_82802ab(struct flashctx *flash, unsigned int page, unsigned int pagesize);
int write_82802ab(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
void print_status_82802ab(uint8_t status);
int unlock_28f004s5(struct flashctx *flash);
int unlock_lh28f008bjt(struct flashctx *flash);

/* jedec.c */
uint8_t oddparity(uint8_t val);
void toggle_ready_jedec(const struct flashctx *flash, chipaddr dst);
void data_polling_jedec(const struct flashctx *flash, chipaddr dst, uint8_t data);
int probe_jedec(struct flashctx *flash);
int probe_jedec_29gl(struct flashctx *flash);
int write_jedec(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
int write_jedec_1(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);
int erase_sector_jedec(struct flashctx *flash, unsigned int page, unsigned int pagesize);
int erase_block_jedec(struct flashctx *flash, unsigned int page, unsigned int blocksize);
int erase_chip_block_jedec(struct flashctx *flash, unsigned int page, unsigned int blocksize);

int unlock_regspace2_uniform_32k(struct flashctx *flash);
int unlock_regspace2_uniform_64k(struct flashctx *flash);
int unlock_regspace2_block_eraser_0(struct flashctx *flash);
int unlock_regspace2_block_eraser_1(struct flashctx *flash);
int printlock_regspace2_uniform_64k(struct flashctx *flash);
int printlock_regspace2_block_eraser_0(struct flashctx *flash);
int printlock_regspace2_block_eraser_1(struct flashctx *flash);

/* sst28sf040.c */
int erase_chip_28sf040(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int erase_sector_28sf040(struct flashctx *flash, unsigned int address, unsigned int sector_size);
int write_28sf040(struct flashctx *flash, const uint8_t *buf,unsigned int start, unsigned int len);
int unprotect_28sf040(struct flashctx *flash);
int protect_28sf040(struct flashctx *flash);

/* sst49lfxxxc.c */
int erase_sector_49lfxxxc(struct flashctx *flash, unsigned int address, unsigned int sector_size);

/* sst_fwhub.c */
int printlock_sst_fwhub(struct flashctx *flash);
int unlock_sst_fwhub(struct flashctx *flash);

/* w39.c */
int printlock_w39f010(struct flashctx * flash);
int printlock_w39l010(struct flashctx * flash);
int printlock_w39l020(struct flashctx * flash);
int printlock_w39l040(struct flashctx * flash);
int printlock_w39v040a(struct flashctx *flash);
int printlock_w39v040b(struct flashctx *flash);
int printlock_w39v040c(struct flashctx *flash);
int printlock_w39v040fa(struct flashctx *flash);
int printlock_w39v040fb(struct flashctx *flash);
int printlock_w39v040fc(struct flashctx *flash);
int printlock_w39v080a(struct flashctx *flash);
int printlock_w39v080fa(struct flashctx *flash);
int printlock_w39v080fa_dual(struct flashctx *flash);
int printlock_at49f(struct flashctx *flash);

/* w29ee011.c */
int probe_w29ee011(struct flashctx *flash);

/* stm50.c */
int erase_sector_stm50(struct flashctx *flash, unsigned int block, unsigned int blocksize);

/* en29lv640b.c */
int probe_en29lv640b(struct flashctx *flash);
int write_en29lv640b(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);

/* spi4ba.c */
int spi_enter_4ba_b7(struct flashctx *flash);
int spi_enter_4ba_b7_we(struct flashctx *flash);
int spi_exit_4ba_e9(struct flashctx *flash);
int spi_exit_4ba_e9_we(struct flashctx *flash);
int spi_byte_program_4ba(struct flashctx *flash, unsigned int addr, uint8_t databyte);
int spi_nbyte_program_4ba(struct flashctx *flash, unsigned int addr, const uint8_t *bytes, unsigned int len);
int spi_nbyte_read_4ba(struct flashctx *flash, unsigned int addr, uint8_t *bytes, unsigned int len);
int spi_block_erase_20_4ba(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_52_4ba(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_d8_4ba(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_byte_program_4ba_ereg(struct flashctx *flash, unsigned int addr, uint8_t databyte);
int spi_nbyte_program_4ba_ereg(struct flashctx *flash, unsigned int addr, const uint8_t *bytes, unsigned int len);
int spi_nbyte_read_4ba_ereg(struct flashctx *flash, unsigned int addr, uint8_t *bytes, unsigned int len);
int spi_block_erase_20_4ba_ereg(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_52_4ba_ereg(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_d8_4ba_ereg(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_byte_program_4ba_direct(struct flashctx *flash, unsigned int addr, uint8_t databyte);
int spi_nbyte_program_4ba_direct(struct flashctx *flash, unsigned int addr, const uint8_t *bytes, unsigned int len);
int spi_nbyte_read_4ba_direct(struct flashctx *flash, unsigned int addr, uint8_t *bytes, unsigned int len);
int spi_block_erase_21_4ba_direct(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_5c_4ba_direct(struct flashctx *flash, unsigned int addr, unsigned int blocklen);
int spi_block_erase_dc_4ba_direct(struct flashctx *flash, unsigned int addr, unsigned int blocklen);

#endif /* !__CHIPDRIVERS_H__ */
