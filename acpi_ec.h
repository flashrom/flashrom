/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2021, TUXEDO Computers GmbH
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
 */

#ifndef __EC_H__
#define __EC_H__ 1

#if defined(__i386__) || defined(__x86_64__)

#include <stdbool.h>
#include <stdint.h>

/* How many iterations to wait for input or output buffer */
#define EC_MAX_STATUS_CHECKS  100000

/*
 * Generic IO functions for ACPI-compliant embedded controllers
 */
bool ec_wait_for_ibuf(unsigned int max_checks);
bool ec_wait_for_obuf(unsigned int max_checks);

bool ec_write_cmd(uint8_t cmd, unsigned int max_checks);
bool ec_read_byte(uint8_t *data, unsigned int max_checks);
bool ec_write_byte(uint8_t data, unsigned int max_checks);

/* These implement standard ACPI commands and thus use standard ports */
bool ec_read_reg(uint8_t address, uint8_t *data, unsigned int max_checks);
bool ec_write_reg(uint8_t address, uint8_t data, unsigned int max_checks);

#endif /* defined(__i386__) || defined(__x86_64__) */

#endif /* !__EC_H__ */
