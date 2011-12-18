/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Carl-Daniel Hailfinger
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
 */

#include "flash.h"
#include "chipdrivers.h"
#include "spi.h"

/* Prettyprint the status register. Works for AMIC A25L series. */

static void spi_prettyprint_status_register_amic_a25_srwd(uint8_t status)
{
	msg_cdbg("Chip status register: Status Register Write Disable "
		     "(SRWD) is %sset\n", (status & (1 << 7)) ? "" : "not ");
}

int spi_prettyprint_status_register_amic_a25l05p(struct flashctx *flash)
{
	uint8_t status;

	status = spi_read_status_register(flash);
	msg_cdbg("Chip status register is %02x\n", status);

	spi_prettyprint_status_register_amic_a25_srwd(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bit(status, 5);
	spi_prettyprint_status_register_bit(status, 4);
	spi_prettyprint_status_register_bp3210(status, 1);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

int spi_prettyprint_status_register_amic_a25l40p(struct flashctx *flash)
{
	uint8_t status;

	status = spi_read_status_register(flash);
	msg_cdbg("Chip status register is %02x\n", status);

	spi_prettyprint_status_register_amic_a25_srwd(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bit(status, 5);
	spi_prettyprint_status_register_bp3210(status, 2);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

int spi_prettyprint_status_register_amic_a25l032(struct flashctx *flash)
{
	uint8_t status;

	status = spi_read_status_register(flash);
	msg_cdbg("Chip status register is %02x\n", status);

	spi_prettyprint_status_register_amic_a25_srwd(status);
	msg_cdbg("Chip status register: Sector Protect Size (SEC) "
		 "is %i KB\n", (status & (1 << 6)) ? 4 : 64);
	msg_cdbg("Chip status register: Top/Bottom (TB) "
		 "is %s\n", (status & (1 << 5)) ? "bottom" : "top");
	spi_prettyprint_status_register_bp3210(status, 2);
	spi_prettyprint_status_register_welwip(status);
	msg_cdbg("Chip status register 2 is NOT decoded!\n");
	return 0;
}

int spi_prettyprint_status_register_amic_a25lq032(struct flashctx *flash)
{
	uint8_t status;

	status = spi_read_status_register(flash);
	msg_cdbg("Chip status register is %02x\n", status);

	spi_prettyprint_status_register_amic_a25_srwd(status);
	msg_cdbg("Chip status register: Sector Protect Size (SEC) "
		 "is %i KB\n", (status & (1 << 6)) ? 4 : 64);
	msg_cdbg("Chip status register: Top/Bottom (TB) "
		 "is %s\n", (status & (1 << 5)) ? "bottom" : "top");
	spi_prettyprint_status_register_bp3210(status, 2);
	spi_prettyprint_status_register_welwip(status);
	msg_cdbg("Chip status register 2 is NOT decoded!\n");
	return 0;
}

/* FIXME: spi_disable_blockprotect is incorrect but works fine for chips using
 * spi_prettyprint_status_register_amic_a25l05p or
 * spi_prettyprint_status_register_amic_a25l40p.
 * FIXME: spi_disable_blockprotect is incorrect and will fail for chips using
 * spi_prettyprint_status_register_amic_a25l032 or
 * spi_prettyprint_status_register_amic_a25lq032 if those have locks controlled
 * by the second status register.
 */
