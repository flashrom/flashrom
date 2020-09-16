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

#endif /* TESTS_H */
