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

#if defined(__i386__) || defined(__x86_64__)

#include <stdbool.h>
#include <stdint.h>

#include "acpi_ec.h"
#include "flash.h"
#include "hwaccess.h"

/* Standard ports */
#define EC_DATA     0x62
#define EC_STS_CMD  0x66

/* Standard commands */
#define EC_CMD_READ_REG   0x80 /* Read register's value */
#define EC_CMD_WRITE_REG  0x81 /* Write register's value */

/* Some of the status bits */
#define EC_STS_IBF  (1 << 1) /* EC's input buffer full (host can't write) */
#define EC_STS_OBF  (1 << 0) /* EC's output buffer full (host can read) */

bool ec_wait_for_ibuf(unsigned int max_checks)
{
	unsigned int i;

	if (!max_checks)
		max_checks = EC_MAX_STATUS_CHECKS;

	for (i = 0; (INB(EC_STS_CMD) & EC_STS_IBF) != 0; ++i) {
		if (i == max_checks)
			return false;
	}

	return true;
}

bool ec_wait_for_obuf(unsigned int max_checks)
{
	unsigned int i;

	if (!max_checks)
		max_checks = EC_MAX_STATUS_CHECKS;

	for (i = 0; (INB(EC_STS_CMD) & EC_STS_OBF) == 0; ++i) {
		if (i == max_checks)
			return false;
	}

	return true;
}

bool ec_write_cmd(uint8_t cmd, unsigned int max_checks)
{
	const bool success = ec_wait_for_ibuf(max_checks);
	if (success)
		OUTB(cmd, EC_STS_CMD);

	return success;
}

bool ec_read_byte(uint8_t *data, unsigned int max_checks)
{
	const bool success = ec_wait_for_obuf(max_checks);
	if (success)
		*data = INB(EC_DATA);

	return success;
}

bool ec_write_byte(uint8_t data, unsigned int max_checks)
{
	const bool success = ec_wait_for_ibuf(max_checks);
	if (success)
		OUTB(data, EC_DATA);

	return success;
}

bool ec_read_reg(uint8_t address, uint8_t *data, unsigned int max_checks)
{
	if (!ec_write_cmd(EC_CMD_READ_REG, max_checks))
		return false;

	if (!ec_write_byte(address, max_checks))
		return false;

	return ec_read_byte(data, max_checks);
}

bool ec_write_reg(uint8_t address, uint8_t data, unsigned int max_checks)
{
	if (!ec_write_cmd(EC_CMD_WRITE_REG, max_checks))
		return false;

	if (!ec_write_byte(address, max_checks))
		return false;

	return ec_write_byte(data, max_checks);
}

#endif  /* defined(__i386__) || defined(__x86_64__) */
