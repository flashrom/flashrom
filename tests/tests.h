#ifndef TESTS_H
#define TESTS_H

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

#endif /* TESTS_H */
