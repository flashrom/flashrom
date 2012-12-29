/*
 * This file is part of the flashrom project.
 * It handles everything related to status registers of the JEDEC family 25.
 *
 * Copyright (C) 2007, 2008, 2009, 2010 Carl-Daniel Hailfinger
 * Copyright (C) 2008 coresystems GmbH
 * Copyright (C) 2008 Ronald Hoogenboom <ronald@zonnet.nl>
 * Copyright (C) 2012 Stefan Tauner
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

/* === Generic functions === */
int spi_write_status_enable(struct flashctx *flash)
{
	static const unsigned char cmd[JEDEC_EWSR_OUTSIZE] = { JEDEC_EWSR };
	int result;

	/* Send EWSR (Enable Write Status Register). */
	result = spi_send_command(flash, sizeof(cmd), JEDEC_EWSR_INSIZE, cmd, NULL);

	if (result)
		msg_cerr("%s failed\n", __func__);

	return result;
}

static int spi_write_status_register_flag(struct flashctx *flash, int status, const unsigned char enable_opcode)
{
	int result;
	int i = 0;
	/*
	 * WRSR requires either EWSR or WREN depending on chip type.
	 * The code below relies on the fact hat EWSR and WREN have the same
	 * INSIZE and OUTSIZE.
	 */
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ enable_opcode },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_WRSR_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WRSR, (unsigned char) status },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution\n", __func__);
		/* No point in waiting for the command to complete if execution
		 * failed.
		 */
		return result;
	}
	/* WRSR performs a self-timed erase before the changes take effect.
	 * This may take 50-85 ms in most cases, and some chips apparently
	 * allow running RDSR only once. Therefore pick an initial delay of
	 * 100 ms, then wait in 10 ms steps until a total of 5 s have elapsed.
	 */
	programmer_delay(100 * 1000);
	while (spi_read_status_register(flash) & SPI_SR_WIP) {
		if (++i > 490) {
			msg_cerr("Error: WIP bit after WRSR never cleared\n");
			return TIMEOUT_ERROR;
		}
		programmer_delay(10 * 1000);
	}
	return 0;
}

int spi_write_status_register(struct flashctx *flash, int status)
{
	int feature_bits = flash->chip->feature_bits;
	int ret = 1;

	if (!(feature_bits & (FEATURE_WRSR_WREN | FEATURE_WRSR_EWSR))) {
		msg_cdbg("Missing status register write definition, assuming "
			 "EWSR is needed\n");
		feature_bits |= FEATURE_WRSR_EWSR;
	}
	if (feature_bits & FEATURE_WRSR_WREN)
		ret = spi_write_status_register_flag(flash, status, JEDEC_WREN);
	if (ret && (feature_bits & FEATURE_WRSR_EWSR))
		ret = spi_write_status_register_flag(flash, status, JEDEC_EWSR);
	return ret;
}

uint8_t spi_read_status_register(struct flashctx *flash)
{
	static const unsigned char cmd[JEDEC_RDSR_OUTSIZE] = { JEDEC_RDSR };
	/* FIXME: No workarounds for driver/hardware bugs in generic code. */
	unsigned char readarr[2]; /* JEDEC_RDSR_INSIZE=1 but wbsio needs 2 */
	int ret;

	/* Read Status Register */
	ret = spi_send_command(flash, sizeof(cmd), sizeof(readarr), cmd, readarr);
	if (ret)
		msg_cerr("RDSR failed!\n");

	return readarr[0];
}

/* A generic brute-force block protection disable works like this:
 * Write 0x00 to the status register. Check if any locks are still set (that
 * part is chip specific). Repeat once.
 */
int spi_disable_blockprotect(struct flashctx *flash)
{
	uint8_t status;
	int result;

	status = spi_read_status_register(flash);
	/* If block protection is disabled, stop here. */
	if ((status & 0x3c) == 0)
		return 0;

	msg_cdbg("Some block protection in effect, disabling... ");
	result = spi_write_status_register(flash, status & ~0x3c);
	if (result) {
		msg_cerr("spi_write_status_register failed.\n");
		return result;
	}
	status = spi_read_status_register(flash);
	if ((status & 0x3c) != 0) {
		msg_cerr("Block protection could not be disabled!\n");
		return 1;
	}
	msg_cdbg("done.\n");
	return 0;
}

static void spi_prettyprint_status_register_hex(uint8_t status)
{
	msg_cdbg("Chip status register is 0x%02x.\n", status);
}

/* Common highest bit: Status Register Write Disable (SRWD). */
static void spi_prettyprint_status_register_srwd(uint8_t status)
{
	msg_cdbg("Chip status register: Status Register Write Disable (SRWD) is %sset\n",
		 (status & (1 << 7)) ? "" : "not ");
}

/* Common highest bit: Block Protect Write Disable (BPL). */
static void spi_prettyprint_status_register_bpl(uint8_t status)
{
	msg_cdbg("Chip status register: Block Protect Write Disable (BPL) is %sset\n",
		 (status & (1 << 7)) ? "" : "not ");
}

/* Common lowest 2 bits: WEL and WIP. */
static void spi_prettyprint_status_register_welwip(uint8_t status)
{
	msg_cdbg("Chip status register: Write Enable Latch (WEL) is %sset\n",
		 (status & (1 << 1)) ? "" : "not ");
	msg_cdbg("Chip status register: Write In Progress (WIP/BUSY) is %sset\n",
		 (status & (1 << 0)) ? "" : "not ");
}

/* Common block protection (BP) bits. */
static void spi_prettyprint_status_register_bp(uint8_t status, int bp)
{
	switch (bp) {
	/* Fall through. */
	case 4:
		msg_cdbg("Chip status register: Block Protect 4 (BP4) is %sset\n",
			 (status & (1 << 5)) ? "" : "not ");
	case 3:
		msg_cdbg("Chip status register: Block Protect 3 (BP3) is %sset\n",
			 (status & (1 << 5)) ? "" : "not ");
	case 2:
		msg_cdbg("Chip status register: Block Protect 2 (BP2) is %sset\n",
			 (status & (1 << 4)) ? "" : "not ");
	case 1:
		msg_cdbg("Chip status register: Block Protect 1 (BP1) is %sset\n",
			 (status & (1 << 3)) ? "" : "not ");
	case 0:
		msg_cdbg("Chip status register: Block Protect 0 (BP0) is %sset\n",
			 (status & (1 << 2)) ? "" : "not ");
	}
}

/* Unnamed bits. */
static void spi_prettyprint_status_register_bit(uint8_t status, int bit)
{
	msg_cdbg("Chip status register: Bit %i is %sset\n", bit, (status & (1 << bit)) ? "" : "not ");
}

int spi_prettyprint_status_register_plain(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);
	return 0;
}

/* Works for many chips of the
 * AMIC A25L series
 * and MX MX25L512
 */
int spi_prettyprint_status_register_default_bp1(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_srwd(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bit(status, 5);
	spi_prettyprint_status_register_bit(status, 4);
	spi_prettyprint_status_register_bp(status, 1);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

/* Works for many chips of the
 * AMIC A25L series
 */
int spi_prettyprint_status_register_default_bp2(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_srwd(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bit(status, 5);
	spi_prettyprint_status_register_bp(status, 2);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

/* Works for many chips of the
 * ST M25P series
 * MX MX25L series
 */
int spi_prettyprint_status_register_default_bp3(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_srwd(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bp(status, 3);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

/* === Amic ===
 * FIXME: spi_disable_blockprotect is incorrect but works fine for chips using
 * spi_prettyprint_status_register_default_bp1 or
 * spi_prettyprint_status_register_default_bp2.
 * FIXME: spi_disable_blockprotect is incorrect and will fail for chips using
 * spi_prettyprint_status_register_amic_a25l032 if those have locks controlled
 * by the second status register.
 */

int spi_prettyprint_status_register_amic_a25l032(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_srwd(status);
	msg_cdbg("Chip status register: Sector Protect Size (SEC) is %i KB\n", (status & (1 << 6)) ? 4 : 64);
	msg_cdbg("Chip status register: Top/Bottom (TB) is %s\n", (status & (1 << 5)) ? "bottom" : "top");
	spi_prettyprint_status_register_bp(status, 2);
	spi_prettyprint_status_register_welwip(status);
	msg_cdbg("Chip status register 2 is NOT decoded!\n");
	return 0;
}

/* === Atmel === */

static void spi_prettyprint_status_register_atmel_at25_wpen(uint8_t status)
{
	msg_cdbg("Chip status register: Write Protect Enable (WPEN) is %sset\n",
		 (status & (1 << 7)) ? "" : "not ");
}

static void spi_prettyprint_status_register_atmel_at25_srpl(uint8_t status)
{
	msg_cdbg("Chip status register: Sector Protection Register Lock (SRPL) is %sset\n",
		 (status & (1 << 7)) ? "" : "not ");
}

static void spi_prettyprint_status_register_atmel_at25_epewpp(uint8_t status)
{
	msg_cdbg("Chip status register: Erase/Program Error (EPE) is %sset\n",
		 (status & (1 << 5)) ? "" : "not ");
	msg_cdbg("Chip status register: WP# pin (WPP) is %sasserted\n",
		 (status & (1 << 4)) ? "not " : "");
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
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

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

int spi_prettyprint_status_register_at25f512b(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

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
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

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
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_atmel_at25_wpen(status);
	spi_prettyprint_status_register_bp(status, 4);
	/* FIXME: Pretty-print detailed sector protection status. */
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

int spi_prettyprint_status_register_at26df081a(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_atmel_at25_srpl(status);
	msg_cdbg("Chip status register: Sequential Program Mode Status (SPM) is %sset\n",
		 (status & (1 << 6)) ? "" : "not ");
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

int spi_disable_blockprotect_at25f512b(struct flashctx *flash)
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

/* === SST === */

static void spi_prettyprint_status_register_sst25_common(uint8_t status)
{
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_bpl(status);
	msg_cdbg("Chip status register: Auto Address Increment Programming (AAI) is %sset\n",
		 (status & (1 << 6)) ? "" : "not ");
	spi_prettyprint_status_register_bp(status, 3);
	spi_prettyprint_status_register_welwip(status);
}

int spi_prettyprint_status_register_sst25(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_sst25_common(status);
	return 0;
}

int spi_prettyprint_status_register_sst25vf016(struct flashctx *flash)
{
	static const char *const bpt[] = {
		"none",
		"1F0000H-1FFFFFH",
		"1E0000H-1FFFFFH",
		"1C0000H-1FFFFFH",
		"180000H-1FFFFFH",
		"100000H-1FFFFFH",
		"all", "all"
	};
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_sst25_common(status);
	msg_cdbg("Resulting block protection : %s\n", bpt[(status & 0x1c) >> 2]);
	return 0;
}

int spi_prettyprint_status_register_sst25vf040b(struct flashctx *flash)
{
	static const char *const bpt[] = {
		"none",
		"0x70000-0x7ffff",
		"0x60000-0x7ffff",
		"0x40000-0x7ffff",
		"all blocks", "all blocks", "all blocks", "all blocks"
	};
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_sst25_common(status);
	msg_cdbg("Resulting block protection : %s\n", bpt[(status & 0x1c) >> 2]);
	return 0;
}
