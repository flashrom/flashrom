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

/* Prettyprint the status register. Works for Atmel A25/A26 series. */

static void spi_prettyprint_status_register_atmel_at25_wpen(uint8_t status)
{
	msg_cdbg("Chip status register: Write Protect Enable (WPEN) "
		 "is %sset\n", (status & (1 << 7)) ? "" : "not ");
}

static void spi_prettyprint_status_register_atmel_at25_srpl(uint8_t status)
{
	msg_cdbg("Chip status register: Sector Protection Register Lock (SRPL) "
		 "is %sset\n", (status & (1 << 7)) ? "" : "not ");
}

static void spi_prettyprint_status_register_atmel_at25_epewpp(uint8_t status)
{
	msg_cdbg("Chip status register: Erase/Program Error (EPE) "
		 "is %sset\n", (status & (1 << 5)) ? "" : "not ");
	msg_cdbg("Chip status register: WP# pin (WPP) "
		 "is %sasserted\n", (status & (1 << 4)) ? "not " : "");
}

static void spi_prettyprint_status_register_atmel_at25_swp(uint8_t status)
{
	msg_cdbg("Chip status register: Software Protection Status (SWP): ");
	switch (status & (3 << 2)) {
	case 0x0 << 2:
		msg_cdbg("no sectors are protected\n");
		break;
	case 0x1 << 2:
		msg_cdbg("some sectors are protected\n");
		/* FIXME: Read individual Sector Protection Registers. */
		break;
	case 0x3 << 2:
		msg_cdbg("all sectors are protected\n");
		break;
	default:
		msg_cdbg("reserved for future use\n");
		break;
	}
}

int spi_prettyprint_status_register_at25df(struct flashctx *flash)
{
	uint8_t status;

	status = spi_read_status_register(flash);
	msg_cdbg("Chip status register is %02x\n", status);

	spi_prettyprint_status_register_atmel_at25_srpl(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_atmel_at25_epewpp(status);
	spi_prettyprint_status_register_atmel_at25_swp(status);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

int spi_prettyprint_status_register_at25df_sec(struct flashctx *flash)
{
	/* FIXME: We should check the security lockdown. */
	msg_cdbg("Ignoring security lockdown (if present)\n");
	msg_cdbg("Ignoring status register byte 2\n");
	return spi_prettyprint_status_register_at25df(flash);
}

int spi_prettyprint_status_register_at25f(struct flashctx *flash)
{
	uint8_t status;

	status = spi_read_status_register(flash);
	msg_cdbg("Chip status register is %02x\n", status);

	spi_prettyprint_status_register_atmel_at25_srpl(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_atmel_at25_epewpp(status);
	spi_prettyprint_status_register_bit(status, 3);
	spi_prettyprint_status_register_bp(status, 0);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

int spi_prettyprint_status_register_at25fs010(struct flashctx *flash)
{
	uint8_t status;

	status = spi_read_status_register(flash);
	msg_cdbg("Chip status register is %02x\n", status);

	spi_prettyprint_status_register_atmel_at25_wpen(status);
	msg_cdbg("Chip status register: Bit 6 / Block Protect 4 (BP4) is "
		 "%sset\n", (status & (1 << 6)) ? "" : "not ");
	msg_cdbg("Chip status register: Bit 5 / Block Protect 3 (BP3) is "
		 "%sset\n", (status & (1 << 5)) ? "" : "not ");
	spi_prettyprint_status_register_bit(status, 4);
	msg_cdbg("Chip status register: Bit 3 / Block Protect 1 (BP1) is "
		 "%sset\n", (status & (1 << 3)) ? "" : "not ");
	msg_cdbg("Chip status register: Bit 2 / Block Protect 0 (BP0) is "
		 "%sset\n", (status & (1 << 2)) ? "" : "not ");
	/* FIXME: Pretty-print detailed sector protection status. */
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

int spi_prettyprint_status_register_at25fs040(struct flashctx *flash)
{
	uint8_t status;

	status = spi_read_status_register(flash);
	msg_cdbg("Chip status register is %02x\n", status);

	spi_prettyprint_status_register_atmel_at25_wpen(status);
	spi_prettyprint_status_register_bp(status, 4);
	/* FIXME: Pretty-print detailed sector protection status. */
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

int spi_prettyprint_status_register_atmel_at26df081a(struct flashctx *flash)
{
	uint8_t status;

	status = spi_read_status_register(flash);
	msg_cdbg("Chip status register is %02x\n", status);

	spi_prettyprint_status_register_atmel_at25_srpl(status);
	msg_cdbg("Chip status register: Sequential Program Mode Status (SPM) "
		 "is %sset\n", (status & (1 << 6)) ? "" : "not ");
	spi_prettyprint_status_register_atmel_at25_epewpp(status);
	spi_prettyprint_status_register_atmel_at25_swp(status);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

int spi_disable_blockprotect_at25df(struct flashctx *flash)
{
	uint8_t status;
	int result;

	status = spi_read_status_register(flash);
	/* If block protection is disabled, stop here. */
	if ((status & (3 << 2)) == 0)
		return 0;

	msg_cdbg("Some block protection in effect, disabling... ");
	if (status & (1 << 7)) {
		msg_cdbg("Need to disable Sector Protection Register Lock\n");
		if ((status & (1 << 4)) == 0) {
			msg_cerr("WP# pin is active, disabling "
				 "write protection is impossible.\n");
			return 1;
		}
		/* All bits except bit 7 (SPRL) are readonly. */
		result = spi_write_status_register(flash, status & ~(1 << 7));
		if (result) {
			msg_cerr("spi_write_status_register failed.\n");
			return result;
		}
		
	}
	/* Global unprotect. Make sure to mask SPRL as well. */
	result = spi_write_status_register(flash, status & ~0xbc);
	if (result) {
		msg_cerr("spi_write_status_register failed.\n");
		return result;
	}
	status = spi_read_status_register(flash);
	if ((status & (3 << 2)) != 0) {
		msg_cerr("Block protection could not be disabled!\n");
		return 1;
	}
	msg_cdbg("done.\n");
	return 0;
}

int spi_disable_blockprotect_at25df_sec(struct flashctx *flash)
{
	/* FIXME: We should check the security lockdown. */
	msg_cinfo("Ignoring security lockdown (if present)\n");
	return spi_disable_blockprotect_at25df(flash);
}

int spi_disable_blockprotect_at25f(struct flashctx *flash)
{
	/* spi_disable_blockprotect_at25df is not really the right way to do
	 * this, but the side effects of said function work here as well.
	 */
	return spi_disable_blockprotect_at25df(flash);
}

int spi_disable_blockprotect_at25fs010(struct flashctx *flash)
{
	uint8_t status;
	int result;

	status = spi_read_status_register(flash);
	/* If block protection is disabled, stop here. */
	if ((status & 0x6c) == 0)
		return 0;

	msg_cdbg("Some block protection in effect, disabling... ");
	if (status & (1 << 7)) {
		msg_cdbg("Need to disable Status Register Write Protect\n");
		/* Clear bit 7 (WPEN). */
		result = spi_write_status_register(flash, status & ~(1 << 7));
		if (result) {
			msg_cerr("spi_write_status_register failed.\n");
			return result;
		}
	}
	/* Global unprotect. Make sure to mask WPEN as well. */
	result = spi_write_status_register(flash, status & ~0xec);
	if (result) {
		msg_cerr("spi_write_status_register failed.\n");
		return result;
	}
	status = spi_read_status_register(flash);
	if ((status & 0x6c) != 0) {
		msg_cerr("Block protection could not be disabled!\n");
		return 1;
	}
	msg_cdbg("done.\n");
	return 0;
}

int spi_disable_blockprotect_at25fs040(struct flashctx *flash)
{
	uint8_t status;
	int result;

	status = spi_read_status_register(flash);
	/* If block protection is disabled, stop here. */
	if ((status & 0x7c) == 0)
		return 0;

	msg_cdbg("Some block protection in effect, disabling... ");
	if (status & (1 << 7)) {
		msg_cdbg("Need to disable Status Register Write Protect\n");
		/* Clear bit 7 (WPEN). */
		result = spi_write_status_register(flash, status & ~(1 << 7));
		if (result) {
			msg_cerr("spi_write_status_register failed.\n");
			return result;
		}
	}
	/* Global unprotect. Make sure to mask WPEN as well. */
	result = spi_write_status_register(flash, status & ~0xfc);
	if (result) {
		msg_cerr("spi_write_status_register failed.\n");
		return result;
	}
	status = spi_read_status_register(flash);
	if ((status & 0x7c) != 0) {
		msg_cerr("Block protection could not be disabled!\n");
		return 1;
	}
	msg_cdbg("done.\n");
	return 0;
}
