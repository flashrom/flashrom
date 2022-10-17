/*
 * This file is part of the flashrom project.
 *
 * Copyright 2020 Google LLC
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

#ifndef TESTS_H
#define TESTS_H

/* helpers.c */
void address_to_bits_test_success(void **state);
void bitcount_test_success(void **state);
void minmax_test_success(void **state);
void strcat_realloc_test_success(void **state);
void tolower_string_test_success(void **state);
void reverse_byte_test_success(void **state);
void reverse_bytes_test_success(void **state);

/* flashrom.c */
void flashbuses_to_text_test_success(void **state);

/* spi25.c */
void spi_write_enable_test_success(void **state);
void spi_write_disable_test_success(void **state);
void probe_spi_rdid_test_success(void **state);
void probe_spi_rdid4_test_success(void **state);
void probe_spi_rems_test_success(void **state);
void probe_spi_res1_test_success(void **state);
void probe_spi_res2_test_success(void **state);
void probe_spi_res3_test_success(void **state);
void probe_spi_at25f_test_success(void **state);
void probe_spi_st95_test_success(void **state); /* spi95.c */

/* lifecycle.c */
void dummy_basic_lifecycle_test_success(void **state);
void dummy_probe_lifecycle_test_success(void **state);
void dummy_probe_variable_size_test_success(void **state);
void dummy_init_fails_unhandled_param_test_success(void **state);
void dummy_init_success_invalid_param_test_success(void **state);
void dummy_init_success_unhandled_param_test_success(void **state);
void dummy_null_prog_param_test_success(void **state);
void nicrealtek_basic_lifecycle_test_success(void **state);
void raiden_debug_basic_lifecycle_test_success(void **state);
void dediprog_basic_lifecycle_test_success(void **state);
void linux_mtd_probe_lifecycle_test_success(void **state);
void linux_spi_probe_lifecycle_test_success(void **state);
void parade_lspcon_basic_lifecycle_test_success(void **state);
void parade_lspcon_no_allow_brick_test_success(void **state);
void mediatek_i2c_spi_basic_lifecycle_test_success(void **state);
void mediatek_i2c_no_allow_brick_test_success(void **state);
void realtek_mst_basic_lifecycle_test_success(void **state);
void realtek_mst_no_allow_brick_test_success(void **state);

/* layout.c */
void included_regions_dont_overlap_test_success(void **state);
void included_regions_overlap_test_success(void **state);
void region_not_included_overlap_test_success(void **state);
void layout_pass_sanity_checks_test_success(void **state);
void layout_region_invalid_address_test_success(void **state);
void layout_region_invalid_range_test_success(void **state);

/* chip.c */
void erase_chip_test_success(void **state);
void erase_chip_with_dummyflasher_test_success(void **state);
void read_chip_test_success(void **state);
void read_chip_with_dummyflasher_test_success(void **state);
void write_chip_test_success(void **state);
void write_chip_with_dummyflasher_test_success(void **state);
void verify_chip_test_success(void **state);
void verify_chip_with_dummyflasher_test_success(void **state);

/* chip_wp.c */
void invalid_wp_range_dummyflasher_test_success(void **state);
void set_wp_range_dummyflasher_test_success(void **state);
void switch_wp_mode_dummyflasher_test_success(void **state);
void wp_init_from_status_dummyflasher_test_success(void **state);
void full_chip_erase_with_wp_dummyflasher_test_success(void **state);
void partial_chip_erase_with_wp_dummyflasher_test_success(void **state);

#endif /* TESTS_H */
