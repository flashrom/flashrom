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
 */

#include <stdlib.h>

#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

/* === Generic functions === */

/*
 * Writing SR2 or higher with an extended WRSR command requires
 * writing all lower SRx along with it, so just read the lower
 * SRx and write them back.
 */
static int spi_prepare_wrsr_ext(
		uint8_t write_cmd[4], size_t *const write_cmd_len,
		const struct flashctx *const flash,
		const enum flash_reg reg, const uint8_t value)
{
	enum flash_reg reg_it;
	size_t i = 0;

	write_cmd[i++] = JEDEC_WRSR;

	for (reg_it = STATUS1; reg_it < reg; ++reg_it) {
		uint8_t sr;

		if (spi_read_register(flash, reg_it, &sr)) {
			msg_cerr("Writing SR%d failed: failed to read SR%d for writeback.\n",
				 reg - STATUS1 + 1, reg_it - STATUS1 + 1);
			return 1;
		}
		write_cmd[i++] = sr;
	}

	write_cmd[i++] = value;
	*write_cmd_len = i;

	return 0;
}

int spi_write_register(const struct flashctx *flash, enum flash_reg reg, uint8_t value)
{
	int feature_bits = flash->chip->feature_bits;

	uint8_t write_cmd[4];
	size_t write_cmd_len = 0;

	/*
	 * Create SPI write command sequence based on the destination register
	 * and the chip's supported command set.
	 */
	switch (reg) {
	case STATUS1:
		write_cmd[0] = JEDEC_WRSR;
		write_cmd[1] = value;
		write_cmd_len = JEDEC_WRSR_OUTSIZE;
		break;
	case STATUS2:
		if (feature_bits & FEATURE_WRSR2) {
			write_cmd[0] = JEDEC_WRSR2;
			write_cmd[1] = value;
			write_cmd_len = JEDEC_WRSR2_OUTSIZE;
			break;
		}
		if (feature_bits & FEATURE_WRSR_EXT2) {
			if (spi_prepare_wrsr_ext(write_cmd, &write_cmd_len, flash, reg, value))
				return 1;
			break;
		}
		msg_cerr("Cannot write SR2: unsupported by chip\n");
		return 1;
	case STATUS3:
		if (feature_bits & FEATURE_WRSR3) {
			write_cmd[0] = JEDEC_WRSR3;
			write_cmd[1] = value;
			write_cmd_len = JEDEC_WRSR3_OUTSIZE;
			break;
		}
		if ((feature_bits & FEATURE_WRSR_EXT3) == FEATURE_WRSR_EXT3) {
			if (spi_prepare_wrsr_ext(write_cmd, &write_cmd_len, flash, reg, value))
				return 1;
			break;
		}
		msg_cerr("Cannot write SR3: unsupported by chip\n");
		return 1;
	case SECURITY:
		/*
		 * Security register doesn't have a normal write operation. Instead,
		 * there are separate commands that set individual OTP bits.
		 */
		msg_cerr("Cannot write SECURITY: unsupported by design\n");
		return 1;
	case CONFIG:
		/*
		 * This one is read via a separate command, but written as if it's SR2
		 * in FEATURE_WRSR_EXT2 case of WRSR command.
		 */
		if (feature_bits & FEATURE_CFGR) {
			write_cmd[0] = JEDEC_WRSR;
			if (spi_read_register(flash, STATUS1, &write_cmd[1])) {
				msg_cerr("Writing CONFIG failed: failed to read SR1 for writeback.\n");
				return 1;
			}
			write_cmd[2] = value;
			write_cmd_len = 3;
			break;
		}
		msg_cerr("Cannot write CONFIG: unsupported by chip\n");
		return 1;
	default:
		msg_cerr("Cannot write register: unknown register\n");
		return 1;
	}

	if (!spi_probe_opcode(flash, write_cmd[0])) {
		msg_pdbg("%s: write to register %d not supported by programmer, ignoring.\n", __func__, reg);
		return SPI_INVALID_OPCODE;
	}

	uint8_t enable_cmd;
	if (feature_bits & FEATURE_WRSR_WREN) {
		enable_cmd = JEDEC_WREN;
	} else if (feature_bits & FEATURE_WRSR_EWSR) {
		enable_cmd = JEDEC_EWSR;
	} else {
		msg_cdbg("Missing status register write definition, assuming "
			 "EWSR is needed\n");
		enable_cmd = JEDEC_EWSR;
	}

	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= &enable_cmd,
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= write_cmd_len,
		.writearr	= write_cmd,
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	int result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution\n", __func__);
		return result;
	}

	/*
	 * WRSR performs a self-timed erase before the changes take effect.
	 * This may take 50-85 ms in most cases, and some chips apparently
	 * allow running RDSR only once. Therefore pick an initial delay of
	 * 100 ms, then wait in 10 ms steps until a total of 5 s have elapsed.
	 *
	 * Newer chips with multiple status registers (SR2 etc.) are unlikely
	 * to have problems with multiple RDSR commands, so only wait for the
	 * initial 100 ms if the register we wrote to was SR1.
	 */
	int delay_ms = 5000;
	if (reg == STATUS1) {
		programmer_delay(flash, 100 * 1000);
		delay_ms -= 100;
	}

	for (; delay_ms > 0; delay_ms -= 10) {
		uint8_t status;
		result = spi_read_register(flash, STATUS1, &status);
		if (result)
			return result;
		if ((status & SPI_SR_WIP) == 0)
			return 0;
		programmer_delay(flash, 10 * 1000);
	}


	msg_cerr("Error: WIP bit after WRSR never cleared\n");
	return TIMEOUT_ERROR;
}

int spi_read_register(const struct flashctx *flash, enum flash_reg reg, uint8_t *value)
{
	int feature_bits = flash->chip->feature_bits;
	uint8_t read_cmd;

	switch (reg) {
	case STATUS1:
		read_cmd = JEDEC_RDSR;
		break;
	case STATUS2:
		if (feature_bits & (FEATURE_WRSR_EXT2 | FEATURE_WRSR2)) {
			read_cmd = JEDEC_RDSR2;
			break;
		}
		msg_cerr("Cannot read SR2: unsupported by chip\n");
		return 1;
	case STATUS3:
		if ((feature_bits & FEATURE_WRSR_EXT3) == FEATURE_WRSR_EXT3
		    || (feature_bits & FEATURE_WRSR3)) {
			read_cmd = JEDEC_RDSR3;
			break;
		}
		msg_cerr("Cannot read SR3: unsupported by chip\n");
		return 1;
	case SECURITY:
		if (feature_bits & FEATURE_SCUR) {
			read_cmd = JEDEC_RDSCUR;
			break;
		}
		msg_cerr("Cannot read SECURITY: unsupported by chip\n");
		return 1;
	case CONFIG:
		if (feature_bits & FEATURE_CFGR) {
			read_cmd = JEDEC_RDCR;
			break;
		}
		msg_cerr("Cannot read CONFIG: unsupported by chip\n");
		return 1;
	default:
		msg_cerr("Cannot read register: unknown register\n");
		return 1;
	}

	if (!spi_probe_opcode(flash, read_cmd)) {
		msg_pdbg("%s: read from register %d not supported by programmer.\n", __func__, reg);
		return SPI_INVALID_OPCODE;
	}

	/* FIXME: No workarounds for driver/hardware bugs in generic code. */
	/* JEDEC_RDSR_INSIZE=1 but wbsio needs 2 */
	uint8_t readarr[2];

	int ret = spi_send_command(flash, sizeof(read_cmd), sizeof(readarr), &read_cmd, readarr);
	if (ret) {
		msg_cerr("Register read failed!\n");
		return ret;
	}

	*value = readarr[0];
	msg_cspew("%s: read_cmd 0x%02x returned 0x%02x\n", __func__, read_cmd, readarr[0]);
	return 0;
}

static int spi_restore_status(struct flashctx *flash, void *data)
{
	uint8_t status = *(uint8_t *)data;
	free(data);

	msg_cdbg("restoring chip status (0x%02x)\n", status);
	return spi_write_register(flash, STATUS1, status);
}

/* A generic block protection disable.
 * Tests if a protection is enabled with the block protection mask (bp_mask) and returns success otherwise.
 * Tests if the register bits are locked with the lock_mask (lock_mask).
 * Tests if a hardware protection is active (i.e. low pin/high bit value) with the write protection mask
 * (wp_mask) and bails out in that case.
 * If there are register lock bits set we try to disable them by unsetting those bits of the previous register
 * contents that are set in the lock_mask. We then check if removing the lock bits has worked and continue as if
 * they never had been engaged:
 * If the lock bits are out of the way try to disable engaged protections.
 * To support uncommon global unprotects (e.g. on most AT2[56]xx1(A)) unprotect_mask can be used to force
 * bits to 0 additionally to those set in bp_mask and lock_mask. Only bits set in unprotect_mask are potentially
 * preserved when doing the final unprotect.
 *
 * To sum up:
 * bp_mask: set those bits that correspond to the bits in the status register that indicate an active protection
 *          (which should be unset after this function returns).
 * lock_mask: set the bits that correspond to the bits that lock changing the bits above.
 * wp_mask: set the bits that correspond to bits indicating non-software revocable protections.
 * unprotect_mask: set the bits that should be preserved if possible when unprotecting.
 */
static int spi_disable_blockprotect_generic(struct flashctx *flash, uint8_t bp_mask, uint8_t lock_mask, uint8_t wp_mask, uint8_t unprotect_mask)
{
	uint8_t status;
	int result;

	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;

	if ((status & bp_mask) == 0) {
		msg_cdbg2("Block protection is disabled.\n");
		return 0;
	}

	/* Restore status register content upon exit in finalize_flash_access(). */
	uint8_t *data = calloc(1, sizeof(uint8_t));
	if (!data) {
		msg_cerr("Out of memory!\n");
		return 1;
	}
	*data = status;
	register_chip_restore(spi_restore_status, flash, data);

	msg_cdbg("Some block protection in effect, disabling... ");
	if ((status & lock_mask) != 0) {
		msg_cdbg("\n\tNeed to disable the register lock first... ");
		if (wp_mask != 0 && (status & wp_mask) == 0) {
			msg_cerr("Hardware protection is active, disabling write protection is impossible.\n");
			return 1;
		}
		/* All bits except the register lock bit (often called SPRL, SRWD, WPEN) are readonly. */
		result = spi_write_register(flash, STATUS1, status & ~lock_mask);
		if (result) {
			msg_cerr("Could not write status register 1.\n");
			return result;
		}

		ret = spi_read_register(flash, STATUS1, &status);
		if (ret)
			return ret;

		if ((status & lock_mask) != 0) {
			msg_cerr("Unsetting lock bit(s) failed.\n");
			return 1;
		}
		msg_cdbg("done.\n");
	}
	/* Global unprotect. Make sure to mask the register lock bit as well. */
	result = spi_write_register(flash, STATUS1, status & ~(bp_mask | lock_mask) & unprotect_mask);
	if (result) {
		msg_cerr("Could not write status register 1.\n");
		return result;
	}

	ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;

	if ((status & bp_mask) != 0) {
		msg_cerr("Block protection could not be disabled!\n");
		printlockfunc_t *printlock = lookup_printlock_func_ptr(flash);
		if (printlock)
			printlock(flash);
		return 1;
	}
	msg_cdbg("disabled.\n");
	return 0;
}

/* A common block protection disable that tries to unset the status register bits masked by 0x3C. */
static int spi_disable_blockprotect(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x3C, 0, 0, 0xFF);
}

static int spi_disable_blockprotect_sst26_global_unprotect(struct flashctx *flash)
{
	int result = spi_write_enable(flash);
	if (result)
		return result;

	static const unsigned char cmd[] = { 0x98 }; /* ULBPR */
	result = spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
	if (result)
		msg_cerr("ULBPR failed\n");
	return result;
}

/* A common block protection disable that tries to unset the status register bits masked by 0x0C (BP0-1) and
 * protected/locked by bit #7. Useful when bits 4-5 may be non-0). */
static int spi_disable_blockprotect_bp1_srwd(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x0C, 1 << 7, 0, 0xFF);
}

/* A common block protection disable that tries to unset the status register bits masked by 0x1C (BP0-2) and
 * protected/locked by bit #7. Useful when bit #5 is neither a protection bit nor reserved (and hence possibly
 * non-0). */
static int spi_disable_blockprotect_bp2_srwd(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x1C, 1 << 7, 0, 0xFF);
}

/* A common block protection disable that tries to unset the status register bits masked by 0x3C (BP0-3) and
 * protected/locked by bit #7. */
static int spi_disable_blockprotect_bp3_srwd(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x3C, 1 << 7, 0, 0xFF);
}

/* A common block protection disable that tries to unset the status register bits masked by 0x7C (BP0-4) and
 * protected/locked by bit #7. */
static int spi_disable_blockprotect_bp4_srwd(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x7C, 1 << 7, 0, 0xFF);
}

static void spi_prettyprint_status_register_hex(uint8_t status)
{
	msg_cdbg("Chip status register is 0x%02x.\n", status);
}

/* Common highest bit: Status Register Write Disable (SRWD) or Status Register Protect (SRP). */
static void spi_prettyprint_status_register_srwd(uint8_t status)
{
	msg_cdbg("Chip status register: Status Register Write Disable (SRWD, SRP, ...) is %sset\n",
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
	case 4:
		msg_cdbg("Chip status register: Block Protect 4 (BP4) is %sset\n",
			 (status & (1 << 6)) ? "" : "not ");
		/* Fall through. */
	case 3:
		msg_cdbg("Chip status register: Block Protect 3 (BP3) is %sset\n",
			 (status & (1 << 5)) ? "" : "not ");
		/* Fall through. */
	case 2:
		msg_cdbg("Chip status register: Block Protect 2 (BP2) is %sset\n",
			 (status & (1 << 4)) ? "" : "not ");
		/* Fall through. */
	case 1:
		msg_cdbg("Chip status register: Block Protect 1 (BP1) is %sset\n",
			 (status & (1 << 3)) ? "" : "not ");
		/* Fall through. */
	case 0:
		msg_cdbg("Chip status register: Block Protect 0 (BP0) is %sset\n",
			 (status & (1 << 2)) ? "" : "not ");
	}
}

/* Unnamed bits. */
void spi_prettyprint_status_register_bit(uint8_t status, int bit)
{
	msg_cdbg("Chip status register: Bit %i is %sset\n", bit, (status & (1 << bit)) ? "" : "not ");
}

static int spi_prettyprint_status_register_plain(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_hex(status);
	return 0;
}

/* Print the plain hex value and the welwip bits only. */
static int spi_prettyprint_status_register_default_welwip(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_welwip(status);
	return 0;
}

/* Works for many chips of the
 * AMIC A25L series
 * and MX MX25L512
 */
static int spi_prettyprint_status_register_bp1_srwd(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
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
 * PMC Pm25LD series
 */
static int spi_prettyprint_status_register_bp2_srwd(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
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
static int spi_prettyprint_status_register_bp3_srwd(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_srwd(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bp(status, 3);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

static int spi_prettyprint_status_register_bp4_srwd(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_srwd(status);
	spi_prettyprint_status_register_bp(status, 4);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

static int spi_prettyprint_status_register_bp2_bpl(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_bpl(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bit(status, 5);
	spi_prettyprint_status_register_bp(status, 2);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

static int spi_prettyprint_status_register_bp2_tb_bpl(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_bpl(status);
	spi_prettyprint_status_register_bit(status, 6);
	msg_cdbg("Chip status register: Top/Bottom (TB) is %s\n", (status & (1 << 5)) ? "bottom" : "top");
	spi_prettyprint_status_register_bp(status, 2);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

static int spi_prettyprint_status_register_srwd_sec_tb_bp2_welwip(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
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

static int spi_prettyprint_status_register_at25df(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;

	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_atmel_at25_srpl(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_atmel_at25_epewpp(status);
	spi_prettyprint_status_register_atmel_at25_swp(status);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

static int spi_prettyprint_status_register_at25df_sec(struct flashctx *flash)
{
	/* FIXME: We should check the security lockdown. */
	msg_cdbg("Ignoring security lockdown (if present)\n");
	msg_cdbg("Ignoring status register byte 2\n");
	return spi_prettyprint_status_register_at25df(flash);
}

/* used for AT25F512, AT25F1024(A), AT25F2048 */
static int spi_prettyprint_status_register_at25f(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;

	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_atmel_at25_wpen(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bit(status, 5);
	spi_prettyprint_status_register_bit(status, 4);
	spi_prettyprint_status_register_bp(status, 1);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

static int spi_prettyprint_status_register_at25f512a(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;

	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_atmel_at25_wpen(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bit(status, 5);
	spi_prettyprint_status_register_bit(status, 4);
	spi_prettyprint_status_register_bit(status, 3);
	spi_prettyprint_status_register_bp(status, 0);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

static int spi_prettyprint_status_register_at25f512b(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_atmel_at25_srpl(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_atmel_at25_epewpp(status);
	spi_prettyprint_status_register_bit(status, 3);
	spi_prettyprint_status_register_bp(status, 0);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

static int spi_prettyprint_status_register_at25f4096(struct flashctx *flash)
{
	uint8_t status;

	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;

	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_atmel_at25_wpen(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bit(status, 5);
	spi_prettyprint_status_register_bp(status, 2);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

static int spi_prettyprint_status_register_at25fs010(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
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

static int spi_prettyprint_status_register_at25fs040(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_atmel_at25_wpen(status);
	spi_prettyprint_status_register_bp(status, 4);
	/* FIXME: Pretty-print detailed sector protection status. */
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

static int spi_prettyprint_status_register_at26df081a(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_atmel_at25_srpl(status);
	msg_cdbg("Chip status register: Sequential Program Mode Status (SPM) is %sset\n",
		 (status & (1 << 6)) ? "" : "not ");
	spi_prettyprint_status_register_atmel_at25_epewpp(status);
	spi_prettyprint_status_register_atmel_at25_swp(status);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

/* Some Atmel DataFlash chips support per sector protection bits and the write protection bits in the status
 * register do indicate if none, some or all sectors are protected. It is possible to globally (un)lock all
 * sectors at once by writing 0 not only the protection bits (2 and 3) but also completely unrelated bits (4 and
 * 5) which normally are not touched.
 * Affected are all known Atmel chips matched by AT2[56]D[FLQ]..1A? but the AT26DF041. */
static int spi_disable_blockprotect_at2x_global_unprotect(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x0C, 1 << 7, 1 << 4, 0x00);
}

static int spi_disable_blockprotect_at2x_global_unprotect_sec(struct flashctx *flash)
{
	/* FIXME: We should check the security lockdown. */
	msg_cinfo("Ignoring security lockdown (if present)\n");
	return spi_disable_blockprotect_at2x_global_unprotect(flash);
}

static int spi_disable_blockprotect_at25f(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x0C, 1 << 7, 0, 0xFF);
}

static int spi_disable_blockprotect_at25f512a(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x04, 1 << 7, 0, 0xFF);
}

static int spi_disable_blockprotect_at25f512b(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x04, 1 << 7, 1 << 4, 0xFF);
}

static int spi_disable_blockprotect_at25fs010(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x6C, 1 << 7, 0, 0xFF);
 }

static int spi_disable_blockprotect_at25fs040(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x7C, 1 << 7, 0, 0xFF);
}

/* === Eon === */

static int spi_prettyprint_status_register_en25s_wp(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_srwd(status);
	msg_cdbg("Chip status register: WP# disable (WPDIS) is %sabled\n", (status & (1 << 6)) ? "en " : "dis");
	spi_prettyprint_status_register_bp(status, 3);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

/* === Intel/Numonyx/Micron - Spansion === */

static int spi_disable_blockprotect_n25q(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x5C, 1 << 7, 0, 0xFF);
}

static int spi_prettyprint_status_register_n25q(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_srwd(status);
	if (flash->chip->total_size <= 32 / 8 * 1024) /* N25Q16 and N25Q32: reserved */
		spi_prettyprint_status_register_bit(status, 6);
	else
		msg_cdbg("Chip status register: Block Protect 3 (BP3) is %sset\n",
			 (status & (1 << 6)) ? "" : "not ");
	msg_cdbg("Chip status register: Top/Bottom (TB) is %s\n", (status & (1 << 5)) ? "bottom" : "top");
	spi_prettyprint_status_register_bp(status, 2);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

/* Used by Intel/Numonyx S33 and Spansion S25FL-S chips */
/* TODO: Clear P_FAIL and E_FAIL with Clear SR Fail Flags Command (30h) here? */
static int spi_disable_blockprotect_bp2_ep_srwd(struct flashctx *flash)
{
	return spi_disable_blockprotect_bp2_srwd(flash);
}

blockprotect_func_t *lookup_blockprotect_func_ptr(const struct flashchip *const chip)
{
	switch (chip->unlock) {
		case SPI_DISABLE_BLOCKPROTECT: return spi_disable_blockprotect;
		case SPI_DISABLE_BLOCKPROTECT_BP2_EP_SRWD: return spi_disable_blockprotect_bp2_ep_srwd;
		case SPI_DISABLE_BLOCKPROTECT_BP1_SRWD: return spi_disable_blockprotect_bp1_srwd;
		case SPI_DISABLE_BLOCKPROTECT_BP2_SRWD: return spi_disable_blockprotect_bp2_srwd;
		case SPI_DISABLE_BLOCKPROTECT_BP3_SRWD: return spi_disable_blockprotect_bp3_srwd;
		case SPI_DISABLE_BLOCKPROTECT_BP4_SRWD: return spi_disable_blockprotect_bp4_srwd;
		case SPI_DISABLE_BLOCKPROTECT_AT45DB: return spi_disable_blockprotect_at45db; /* at45db.c */
		case SPI_DISABLE_BLOCKPROTECT_AT25F: return spi_disable_blockprotect_at25f;
		case SPI_DISABLE_BLOCKPROTECT_AT25FS010: return spi_disable_blockprotect_at25fs010;
		case SPI_DISABLE_BLOCKPROTECT_AT25FS040: return spi_disable_blockprotect_at25fs040;
		case SPI_DISABLE_BLOCKPROTECT_AT25F512A: return spi_disable_blockprotect_at25f512a;
		case SPI_DISABLE_BLOCKPROTECT_AT25F512B: return spi_disable_blockprotect_at25f512b;
		case SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT: return spi_disable_blockprotect_at2x_global_unprotect;
		case SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT_SEC: return spi_disable_blockprotect_at2x_global_unprotect_sec;
		case SPI_DISABLE_BLOCKPROTECT_SST26_GLOBAL_UNPROTECT: return spi_disable_blockprotect_sst26_global_unprotect;
		case SPI_DISABLE_BLOCKPROTECT_N25Q: return spi_disable_blockprotect_n25q;
		/* fallthough to lookup_jedec_blockprotect_func_ptr() */
		case UNLOCK_REGSPACE2_BLOCK_ERASER_0:
		case UNLOCK_REGSPACE2_BLOCK_ERASER_1:
		case UNLOCK_REGSPACE2_UNIFORM_32K:
		case UNLOCK_REGSPACE2_UNIFORM_64K:
			return lookup_jedec_blockprotect_func_ptr(chip);
		/* fallthough to lookup_82802ab_blockprotect_func_ptr() */
		case UNLOCK_28F004S5:
		case UNLOCK_LH28F008BJT:
			return lookup_82802ab_blockprotect_func_ptr(chip);
		case UNLOCK_SST_FWHUB: return unlock_sst_fwhub; /* sst_fwhub.c */
		case UNPROTECT_28SF040: return unprotect_28sf040; /* sst28sf040.c */
	/* default: non-total function, 0 indicates no unlock function set.
	 * We explicitly do not want a default catch-all case in the switch
	 * to ensure unhandled enum's are compiler warnings.
	 */
		case NO_BLOCKPROTECT_FUNC: return NULL;
	};

	return NULL;
}

/* Used by Intel/Numonyx S33 and Spansion S25FL-S chips */
static int spi_prettyprint_status_register_bp2_ep_srwd(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_srwd(status);
	msg_cdbg("Chip status register: Program Fail Flag (P_FAIL) is %sset\n",
		 (status & (1 << 6)) ? "" : "not ");
	msg_cdbg("Chip status register: Erase Fail Flag (E_FAIL) is %sset\n",
		 (status & (1 << 5)) ? "" : "not ");
	spi_prettyprint_status_register_bp(status, 2);
	spi_prettyprint_status_register_welwip(status);
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

static int spi_prettyprint_status_register_sst25(struct flashctx *flash)
{
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_sst25_common(status);
	return 0;
}

static int spi_prettyprint_status_register_sst25vf016(struct flashctx *flash)
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
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_sst25_common(status);
	msg_cdbg("Resulting block protection : %s\n", bpt[(status & 0x1c) >> 2]);
	return 0;
}

static int spi_prettyprint_status_register_sst25vf040b(struct flashctx *flash)
{
	static const char *const bpt[] = {
		"none",
		"0x70000-0x7ffff",
		"0x60000-0x7ffff",
		"0x40000-0x7ffff",
		"all blocks", "all blocks", "all blocks", "all blocks"
	};
	uint8_t status;
	int ret = spi_read_register(flash, STATUS1, &status);
	if (ret)
		return ret;
	spi_prettyprint_status_register_sst25_common(status);
	msg_cdbg("Resulting block protection : %s\n", bpt[(status & 0x1c) >> 2]);
	return 0;
}

printlockfunc_t *lookup_printlock_func_ptr(struct flashctx *flash)
{
	switch (flash->chip->printlock) {
		case PRINTLOCK_AT49F: return &printlock_at49f;
		case PRINTLOCK_REGSPACE2_BLOCK_ERASER_0: return &printlock_regspace2_block_eraser_0;
		case PRINTLOCK_REGSPACE2_BLOCK_ERASER_1: return &printlock_regspace2_block_eraser_1;
		case PRINTLOCK_SST_FWHUB: return &printlock_sst_fwhub;
		case PRINTLOCK_W39F010: return &printlock_w39f010;
		case PRINTLOCK_W39L010: return &printlock_w39l010;
		case PRINTLOCK_W39L020: return &printlock_w39l020;
		case PRINTLOCK_W39L040: return &printlock_w39l040;
		case PRINTLOCK_W39V040A: return &printlock_w39v040a;
		case PRINTLOCK_W39V040B: return &printlock_w39v040b;
		case PRINTLOCK_W39V040C: return &printlock_w39v040c;
		case PRINTLOCK_W39V040FA: return &printlock_w39v040fa;
		case PRINTLOCK_W39V040FB: return &printlock_w39v040fb;
		case PRINTLOCK_W39V040FC: return &printlock_w39v040fc;
		case PRINTLOCK_W39V080A: return &printlock_w39v080a;
		case PRINTLOCK_W39V080FA: return &printlock_w39v080fa;
		case PRINTLOCK_W39V080FA_DUAL: return &printlock_w39v080fa_dual;
		case SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF: return &spi_prettyprint_status_register_at25df;
		case SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF_SEC: return &spi_prettyprint_status_register_at25df_sec;
		case SPI_PRETTYPRINT_STATUS_REGISTER_AT25F: return &spi_prettyprint_status_register_at25f;
		case SPI_PRETTYPRINT_STATUS_REGISTER_AT25F4096: return &spi_prettyprint_status_register_at25f4096;
		case SPI_PRETTYPRINT_STATUS_REGISTER_AT25F512A: return &spi_prettyprint_status_register_at25f512a;
		case SPI_PRETTYPRINT_STATUS_REGISTER_AT25F512B: return &spi_prettyprint_status_register_at25f512b;
		case SPI_PRETTYPRINT_STATUS_REGISTER_AT25FS010: return &spi_prettyprint_status_register_at25fs010;
		case SPI_PRETTYPRINT_STATUS_REGISTER_AT25FS040: return &spi_prettyprint_status_register_at25fs040;
		case SPI_PRETTYPRINT_STATUS_REGISTER_AT26DF081A: return &spi_prettyprint_status_register_at26df081a;
		case SPI_PRETTYPRINT_STATUS_REGISTER_AT45DB: return &spi_prettyprint_status_register_at45db;
		case SPI_PRETTYPRINT_STATUS_REGISTER_BP1_SRWD: return &spi_prettyprint_status_register_bp1_srwd;
		case SPI_PRETTYPRINT_STATUS_REGISTER_BP2_BPL: return &spi_prettyprint_status_register_bp2_bpl;
		case SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD: return &spi_prettyprint_status_register_bp2_ep_srwd;
		case SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD: return &spi_prettyprint_status_register_bp2_srwd;
		case SPI_PRETTYPRINT_STATUS_REGISTER_BP2_TB_BPL: return &spi_prettyprint_status_register_bp2_tb_bpl;
		case SPI_PRETTYPRINT_STATUS_REGISTER_SRWD_SEC_TB_BP2_WELWIP: return &spi_prettyprint_status_register_srwd_sec_tb_bp2_welwip;
		case SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD: return &spi_prettyprint_status_register_bp3_srwd;
		case SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD: return &spi_prettyprint_status_register_bp4_srwd;
		case SPI_PRETTYPRINT_STATUS_REGISTER_DEFAULT_WELWIP: return &spi_prettyprint_status_register_default_welwip;
		case SPI_PRETTYPRINT_STATUS_REGISTER_EN25S_WP: return &spi_prettyprint_status_register_en25s_wp;
		case SPI_PRETTYPRINT_STATUS_REGISTER_N25Q: return &spi_prettyprint_status_register_n25q;
		case SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN: return &spi_prettyprint_status_register_plain;
		case SPI_PRETTYPRINT_STATUS_REGISTER_SST25: return &spi_prettyprint_status_register_sst25;
		case SPI_PRETTYPRINT_STATUS_REGISTER_SST25VF016: return &spi_prettyprint_status_register_sst25vf016;
		case SPI_PRETTYPRINT_STATUS_REGISTER_SST25VF040B: return &spi_prettyprint_status_register_sst25vf040b;
	/* default: non-total function, 0 indicates no unlock function set.
	 * We explicitly do not want a default catch-all case in the switch
	 * to ensure unhandled enum's are compiler warnings.
	 */
		case NO_PRINTLOCK_FUNC: return NULL;
	};

	return NULL;
}
