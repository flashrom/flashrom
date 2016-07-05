/*
 * This file is part of the flashrom project.
 * It handles everything related to status registers of the JEDEC family 25.
 *
 * Copyright (C) 2007, 2008, 2009, 2010 Carl-Daniel Hailfinger
 * Copyright (C) 2008 coresystems GmbH
 * Copyright (C) 2008 Ronald Hoogenboom <ronald@zonnet.nl>
 * Copyright (C) 2012 Stefan Tauner
 * Copyright (C) 2016 Hatim Kanchwala <hatim@hatimak.me>
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

#include "flash.h"
#include "chipdrivers.h"
#include "spi.h"
#include "spi25_statusreg.h"

/* The following multi-array contains descriptions of corresponding bits defined
 * in enum status_register_bit. It is imperative that the correspondence between
 * the two definitions remain intact. The first index defines the correspondence. */
char *statreg_bit_desc[][2] = {
		/* Name,	Description */
		{ "",		"" },	/* Corresponds to INVALID_BIT. */
		{ "RESV",	"" },	/* Corresponds to RESV. */
		{ "WIP",	"BUSY/Write In Progress (WIP)" },
		{ "WEL",	"Write Enable Latch (WEL)" },
		{ "SRP0",	"Status Register Write Disable (SRWD)/Software Register Protect (SRP/SRP0)" },
		{ "SRP1",	"Software Register Protect 1 (SRP1)" },
		{ "BPL",	"Block Protect Write Disable (BPL)" },
		{ "WP",		"WP# Disable (WPDIS)" },
		{ "CMP",	"Complement Protect (CMP)" },
		{ "WPS",	"Write Protect Scheme (WPS)" },
		{ "QE",		"Quad Enable (QE)" },
		{ "SUS",	"Erase/Program Suspend (SUS)" },
		{ "SUS1",	"Erase Suspend (SUS1)" },
		{ "SUS2",	"Program Suspend (SUS2)" },
		{ "DRV0",	"Output Driver Strength (DRV0)" },
		{ "DRV1",	"Output Driver Strength (DRV1)" },
		{ "RST",	"HOLD/Reset (RST)" },
		{ "HPF",	"HPF/HPM (High Performance Flag)" },
		{ "LPE",	"Low Power Enable (LPE)" },
		{ "AAI",	"Auto Address Increment (AAI)" },
		{ "APT",	"All Protect (APT)" },
		{ "CP",		"Continuously Program mode (CP)" },
		/* The order of the following bits must not be altered and
		 * newer entries must not be inserted between them. */
		{ "BP0",	"Block Protect 0 (BP0)" },
		{ "BP1",	"Block Protect 1 (BP1)" },
		{ "BP2",	"Block Protect 2 (BP2)" },
		{ "BP3",	"Block Protect 3 (BP3)" },
		{ "BP4",	"Block Protect 4 (BP4)" },
		{ "TB",		"Top/Bottom Block Protect (TB)" },
		{ "SEC",	"Sector/Block Protect (SEC)" },
		{ "LB1",	"Security Register Lock (LB/LB1)" },
		{ "LB2",	"Security Register Lock (LB2)" },
		{ "LB3",	"Security Register Lock (LB3)" },
};

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

// TODO(hatim): This function should be decommissioned once integration is complete
uint8_t spi_read_status_register(struct flashctx *flash)
{
	static const unsigned char cmd[JEDEC_RDSR_OUTSIZE] = { JEDEC_RDSR };
	/* FIXME: No workarounds for driver/hardware bugs in generic code. */
	unsigned char readarr[2]; /* JEDEC_RDSR_INSIZE=1 but wbsio needs 2 */
	int ret;

	/* Read Status Register */
	ret = spi_send_command(flash, sizeof(cmd), sizeof(readarr), cmd, readarr);
	if (ret) {
		msg_cerr("RDSR failed!\n");
		/* FIXME: We should propagate the error. */
		return 0;
	}

	return readarr[0];
}

/* Generic function to read status register SRn (n = 1, 2, or 3). */
// TODO(hatim): This should eventually replace calls to spi_read_status_register().
uint8_t spi_read_status_register_generic(struct flashctx *flash, enum status_register_num SRn)
{
	// TODO(hatim): Check whether flash has SRn in the first place
	static unsigned char const cmd[MAX_STATUS_REGISTERS][JEDEC_RDSR_OUTSIZE] = {
		{ JEDEC_RDSR }, { JEDEC_RDSR2 }, { JEDEC_RDSR3 }
	};
	unsigned char readarr[JEDEC_RDSR_INSIZE];

	if (spi_send_command(flash, sizeof(cmd[SRn]), sizeof(readarr), cmd[SRn], readarr))
		msg_cerr("%s for SR%d failed.\n", __func__, SRn + 1);
	return (uint8_t)readarr[0];
}

/* Called by spi_write_status_register_generic() to set status for chips with
 * exactly 2 status registers. */
static int spi_write_status_generic_2(struct flashctx *flash, uint16_t status)
{
	int result = spi_write_enable(flash);
	if (result) {
		msg_cerr("%s failed\n", __func__);
		return result;
	}

	unsigned char cmd[JEDEC_WRSR_2_OUTSIZE] = { JEDEC_WRSR, status & 0xff, (status >> 8) & 0xff };
	result = spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
	if (result)
		msg_cerr("%s failed\n", __func__);
	// FIXME(hatim): Verify whether status was indeed written (?)
	return result;
}

/* Called by spi_write_status_register_generic() to set status for chips with
 * exactly 3 status registers. */
static int spi_write_status_generic_3(struct flashctx *flash, enum status_register_num SRn, uint8_t status)
{
	int result = spi_write_enable(flash);
	if (result) {
		msg_cerr("%s failed\n", __func__);
		return result;
	}

	unsigned char cmd[JEDEC_WRSR_OUTSIZE];
	switch (SRn) {
	case SR1:
		cmd[0] = JEDEC_WRSR1;
		break;
	case SR2:
		cmd[0] = JEDEC_WRSR2;
		break;
	case SR3:
		cmd[0] = JEDEC_WRSR3;
		break;
	}
	cmd[1] = status;
	result = spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
	if (result)
		msg_cerr("%s failed\n", __func__);
	// FIXME(hatim): Verify whether status was indeed written (?)
	return result;
}

/* Generic function to set status register SRn (n = 1, 2, or 3) to status. */
// TODO(hatim): This should eventually replace calls to spi_write_status_register()
// and, consequently spi_write_status_register() should be made static.
int spi_write_status_register_generic(struct flashctx *flash, enum status_register_num SRn, uint8_t status)
{
	// TODO(hatim): Check whether flash has SRn in the first place
	int result = 0;
	uint8_t other_status;
	uint16_t total_status;

	switch (top_status_register(flash)) {
	case SR1:
		result = spi_write_status_register(flash, (int)status);
		break;
	case SR2:
		/* Use specific read function if available */
		if (flash->chip->status_register->read)
			other_status = flash->chip->status_register->read(flash, SRn);
		else
			other_status = spi_read_status_register_generic(flash, (SRn == SR1) ? SR2 : SR1);
		total_status = (SRn == SR1) ? ((other_status << 8) | status) : ((status << 8) | other_status);
		result = spi_write_status_generic_2(flash, total_status);
		break;
	case SR3:
		result = spi_write_status_generic_3(flash, SRn, status);
		break;
	}
	if (result)
		msg_cerr("%s failed\n", __func__);
	return result;
}

/* Return top-most (highest) status register. */
enum status_register_num top_status_register(struct flashctx *flash) {
	enum status_register_num SRn = SR1;
	enum status_register_bit (*layout)[8] = flash->chip->status_register->layout;

	while (layout[SRn++][0] != INVALID_BIT)
		;
	return SRn - 2;
}

/* Given a bit, return position of the bit in status register or -1 if not found.
 * Counting starts from 0 for bit_0 of SR1, moves to 8 for bit_0 of SR2 and,
 * finally moves to 23 for bit_7 of SR3. */
char pos_bit(struct flashctx *flash, enum status_register_bit bit) {
	enum status_register_num SRn;
	char pos = -1;
	int j;

	for (SRn = SR1; SRn <= top_status_register(flash); SRn++) {
		for (j = 0; j < 8; j++) {
			if (flash->chip->status_register->layout[SRn][j] == bit) {
				pos = (8 * SRn) + j;
				break;
			}
		}
	}
	return pos;
}

// TODO(hatim): Improve code, merge different switch-cases into one
enum wp_mode get_wp_mode_generic(struct flashctx *flash)
{
	// TODO(hatim): Check whether chip has said status register and bits in the first place
	int result, status, status_tmp, srp_mask;

	if (pos_bit(flash, SRP1) != -1) {
		/* The following code assumes that SRP0 and SRP1 are present in the first and second
		 * status registers as bit_7 and bit_8 respectively. */
		status = flash->chip->status_register->read(flash, SR1);
		status |= flash->chip->status_register->read(flash, SR2) << 8;
		srp_mask = 1 << 7 | 1 << 8;

		switch ((status & srp_mask) >> 7) {
		case 0x00:
			return WP_MODE_SOFTWARE;
		case 0x01:
			/* Make (SRP1, SRP0) = (0, 0). */
			status_tmp = (status & ~srp_mask) & 0xffff;
			/* Only need to write SRP0=0, which is always present in the first status register. */
			if (flash->chip->status_register->write(flash, SR1, (uint8_t)(status_tmp & 0xff))) {
				/* FIXME: If flash->chip->status_register->write() verifies whether status
				 * supplied to it is indeed written, then, if we are in this block, we should
				 * simply return WP_MODE_HARDWARE_PROTECTED because failure is a sufficient
				 * indication for hardware protection mode to be on. */
				msg_cerr("%s failed\n", __func__);
				return WP_MODE_INVALID;
			}
			status_tmp = flash->chip->status_register->read(flash, SR1);
			status_tmp |= flash->chip->status_register->read(flash, SR2) << 8;
			result = (status_tmp & srp_mask) >> 7;
			if (result == 0x01) {
				return WP_MODE_HARDWARE_PROTECTED;
			} else if (result == 0x00) {
				status_tmp = ((status_tmp & ~srp_mask) | (0x01 << 7)) & 0xffff;
				if (flash->chip->status_register->write(flash, SR1,
					(uint8_t)(status_tmp & 0xff))) {
					msg_cerr("%s failed\n", __func__);
					return WP_MODE_SOFTWARE;
				}
				return WP_MODE_HARDWARE_UNPROTECTED;
			} else {
				return WP_MODE_INVALID;
			}
		case 0x02:
			return WP_MODE_POWER_CYCLE;
		case 0x03:
			return WP_MODE_PERMANENT;
		default:
			return WP_MODE_INVALID;
		}
	} else {
		status = flash->chip->status_register->read(flash, SR1);
		srp_mask = 1 << 7;

		switch ((status & srp_mask) >> 7) {
		case 0x00:
			return WP_MODE_SOFTWARE;
		case 0x01:
			/* Make SRP0 = 0. */
			status_tmp = (status & ~srp_mask) & 0xff;
			result = flash->chip->status_register->write(flash, SR1, (uint8_t)status_tmp);
			if (result) {
				/* FIXME: If flash->chip->status_register->write() verifies whether status
				 * supplied to it is indeed written, then, if we are in this block, we should
				 * simply return WP_MODE_HARDWARE_PROTECTED because failure is a sufficient
				 * indication for hardware protection mode to be on. */
				msg_cerr("%s failed\n", __func__);
				return WP_MODE_INVALID;
			}
			status_tmp = flash->chip->status_register->read(flash, SR1);
			result = (status_tmp & srp_mask) >> 7;
			if (result == 0x01) {
				return WP_MODE_HARDWARE_PROTECTED;
			} else if (result == 0x00) {
				status_tmp = ((status_tmp & ~srp_mask) | (0x01 << 7)) & 0xff;
				if (flash->chip->status_register->write(flash, SR1, (uint8_t)status_tmp)) {
					msg_cerr("%s failed\n", __func__);
					return WP_MODE_SOFTWARE;
				}
				return WP_MODE_HARDWARE_UNPROTECTED;
			} else {
				return WP_MODE_INVALID;
			}
		default:
			return WP_MODE_INVALID;
		}
	}
}

// TODO(hatim): Improve code, merge different switch-cases into one
int set_wp_mode_generic(struct flashctx *flash, enum wp_mode wp_mode)
{
	int srp_mask = 1 << 7, srp_bitfield, srp_bitfield_new, result;
	int status = flash->chip->status_register->read(flash, SR1);

	if (pos_bit(flash, SRP1) != -1) {
		/* The following code assumes that SRP0 and SRP1 are present and that they
		 * are present in the first and second status registers as bit_7 and bit_8
		 * respectively. */
		status |= flash->chip->status_register->read(flash, SR2) << 8;
		/* SRP1 is bit_8. */
		srp_mask |= 1 << 8;
		switch (wp_mode) {
		case WP_MODE_SOFTWARE:
			srp_bitfield = 0x00;
			break;
		case WP_MODE_HARDWARE_UNPROTECTED:
			srp_bitfield = 0x01;
			break;
		case WP_MODE_POWER_CYCLE:
			srp_bitfield = 0x02;
			break;
		case WP_MODE_PERMANENT:
			srp_bitfield = 0x03;
			break;
		default:
			msg_cdbg("Invalid write protection mode for status register(s)\n");
			msg_cerr("%s failed\n", __func__);
			return -1;
		}
		status = ((status & ~srp_mask) | (srp_bitfield << 7)) & 0xffff;
		result = flash->chip->status_register->write(flash, SR1, (uint8_t)(status & 0xff));
		if (result) {
			msg_cerr("%s failed", __func__);
			return result;
		}
		result = flash->chip->status_register->write(flash, SR2, (uint8_t)((status >> 8) & 0xff));
		if (result) {
			msg_cerr("%s failed", __func__);
			return result;
		}
		status = flash->chip->status_register->read(flash, SR1);
		status |= flash->chip->status_register->read(flash, SR2) << 8;
		srp_bitfield_new = (status & srp_mask ) >> 7;
		if (srp_bitfield_new != srp_bitfield) {
			msg_cdbg("Setting write protection mode for status register(s) failed\n");
			msg_cerr("%s failed\n", __func__);
			return -1;
		}
	} else {
		switch (wp_mode) {
		case WP_MODE_SOFTWARE:
			srp_bitfield = 0x00;
			break;
		case WP_MODE_HARDWARE_UNPROTECTED:
			srp_bitfield = 0x01;
			break;
		case WP_MODE_POWER_CYCLE:
		case WP_MODE_PERMANENT:
		default:
			msg_cdbg("Invalid write protection mode for status register(s)\n");
			msg_cerr("%s failed\n", __func__);
			return -1;
		}
		status = ((status & ~srp_mask) | (srp_bitfield << 7)) & 0xff;
		result = flash->chip->status_register->write(flash, SR1, (uint8_t)status);
		if (result) {
			msg_cerr("%s failed", __func__);
			return result;
		}
		status = flash->chip->status_register->read(flash, SR1);
		srp_bitfield_new = (status & srp_mask ) >> 7;
		if (srp_bitfield_new != srp_bitfield) {
			msg_cdbg("Setting write protection mode for status register(s) failed\n");
			msg_cerr("%s failed\n", __func__);
			return -1;
		}
	}
	return 0;
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

	status = spi_read_status_register(flash);
	if ((status & bp_mask) == 0) {
		msg_cdbg2("Block protection is disabled.\n");
		return 0;
	}

	msg_cdbg("Some block protection in effect, disabling... ");
	if ((status & lock_mask) != 0) {
		msg_cdbg("\n\tNeed to disable the register lock first... ");
		if (wp_mask != 0 && (status & wp_mask) == 0) {
			msg_cerr("Hardware protection is active, disabling write protection is impossible.\n");
			return 1;
		}
		/* All bits except the register lock bit (often called SPRL, SRWD, WPEN) are readonly. */
		result = spi_write_status_register(flash, status & ~lock_mask);
		if (result) {
			msg_cerr("spi_write_status_register failed.\n");
			return result;
		}
		status = spi_read_status_register(flash);
		if ((status & lock_mask) != 0) {
			msg_cerr("Unsetting lock bit(s) failed.\n");
			return 1;
		}
		msg_cdbg("done.\n");
	}
	/* Global unprotect. Make sure to mask the register lock bit as well. */
	result = spi_write_status_register(flash, status & ~(bp_mask | lock_mask) & unprotect_mask);
	if (result) {
		msg_cerr("spi_write_status_register failed.\n");
		return result;
	}
	status = spi_read_status_register(flash);
	if ((status & bp_mask) != 0) {
		msg_cerr("Block protection could not be disabled!\n");
		flash->chip->printlock(flash);
		return 1;
	}
	msg_cdbg("disabled.\n");
	return 0;
}

/* A common block protection disable that tries to unset the status register bits masked by 0x3C. */
int spi_disable_blockprotect(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x3C, 0, 0, 0xFF);
}

int spi_disable_blockprotect_sst26_global_unprotect(struct flashctx *flash)
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
int spi_disable_blockprotect_bp1_srwd(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x0C, 1 << 7, 0, 0xFF);
}

/* A common block protection disable that tries to unset the status register bits masked by 0x1C (BP0-2) and
 * protected/locked by bit #7. Useful when bit #5 is neither a protection bit nor reserved (and hence possibly
 * non-0). */
int spi_disable_blockprotect_bp2_srwd(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x1C, 1 << 7, 0, 0xFF);
}

/* A common block protection disable that tries to unset the status register bits masked by 0x3C (BP0-3) and
 * protected/locked by bit #7. */
int spi_disable_blockprotect_bp3_srwd(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x3C, 1 << 7, 0, 0xFF);
}

/* A common block protection disable that tries to unset the status register bits masked by 0x7C (BP0-4) and
 * protected/locked by bit #7. */
int spi_disable_blockprotect_bp4_srwd(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x7C, 1 << 7, 0, 0xFF);
}

// TODO(hatim): This function should be decommissioned once integration is complete
static void spi_prettyprint_status_register_hex(uint8_t status)
{
	msg_cdbg("Chip status register is 0x%02x.\n", status);
}

/* Common highest bit: Status Register Write Disable (SRWD) or Status Register Protect (SRP). */
// TODO(hatim): This function should be decommissioned once integration is complete
static void spi_prettyprint_status_register_srwd(uint8_t status)
{
	msg_cdbg("Chip status register: Status Register Write Disable (SRWD, SRP, ...) is %sset\n",
		 (status & (1 << 7)) ? "" : "not ");
}

/* Common highest bit: Block Protect Write Disable (BPL). */
// TODO(hatim): This function should be decommissioned once integration is complete
static void spi_prettyprint_status_register_bpl(uint8_t status)
{
	msg_cdbg("Chip status register: Block Protect Write Disable (BPL) is %sset\n",
		 (status & (1 << 7)) ? "" : "not ");
}

/* Common lowest 2 bits: WEL and WIP. */
// TODO(hatim): This function should be decommissioned once integration is complete
static void spi_prettyprint_status_register_welwip(uint8_t status)
{
	msg_cdbg("Chip status register: Write Enable Latch (WEL) is %sset\n",
		 (status & (1 << 1)) ? "" : "not ");
	msg_cdbg("Chip status register: Write In Progress (WIP/BUSY) is %sset\n",
		 (status & (1 << 0)) ? "" : "not ");
}

/* Common block protection (BP) bits. */
// TODO(hatim): This function should be decommissioned once integration is complete
static void spi_prettyprint_status_register_bp(uint8_t status, int bp)
{
	switch (bp) {
	/* Fall through. */
	case 4:
		msg_cdbg("Chip status register: Block Protect 4 (BP4) is %sset\n",
			 (status & (1 << 6)) ? "" : "not ");
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
// TODO(hatim): This function should be decommissioned once integration is complete
void spi_prettyprint_status_register_bit(uint8_t status, int bit)
{
	msg_cdbg("Chip status register: Bit %i is %sset\n", bit, (status & (1 << bit)) ? "" : "not ");
}

// TODO(hatim): This function should be decommissioned once integration is complete
int spi_prettyprint_status_register_plain(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);
	return 0;
}

/* Print the plain hex value and the welwip bits only. */
// TODO(hatim): This function should be decommissioned once integration is complete
int spi_prettyprint_status_register_default_welwip(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_welwip(status);
	return 0;
}

/* Works for many chips of the
 * AMIC A25L series
 * and MX MX25L512
 */
// TODO(hatim): This function should be decommissioned once integration is complete
int spi_prettyprint_status_register_bp1_srwd(struct flashctx *flash)
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
 * PMC Pm25LD series
 */
// TODO(hatim): This function should be decommissioned once integration is complete
int spi_prettyprint_status_register_bp2_srwd(struct flashctx *flash)
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
// TODO(hatim): This function should be decommissioned once integration is complete
int spi_prettyprint_status_register_bp3_srwd(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_srwd(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bp(status, 3);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

// TODO(hatim): This function should be decommissioned once integration is complete
int spi_prettyprint_status_register_bp4_srwd(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_srwd(status);
	spi_prettyprint_status_register_bp(status, 4);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

// TODO(hatim): This function should be decommissioned once integration is complete
int spi_prettyprint_status_register_bp2_bpl(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_bpl(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bit(status, 5);
	spi_prettyprint_status_register_bp(status, 2);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

// TODO(hatim): This function should be decommissioned once integration is complete
int spi_prettyprint_status_register_bp2_tb_bpl(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_bpl(status);
	spi_prettyprint_status_register_bit(status, 6);
	msg_cdbg("Chip status register: Top/Bottom (TB) is %s\n", (status & (1 << 5)) ? "bottom" : "top");
	spi_prettyprint_status_register_bp(status, 2);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

/* TODO: Use in place of printlock, after asigning a struct status_register member. This supersedes
 * functionality of all prettyprint functions defined above. */
int spi_prettyprint_status_register_generic(struct flashctx *flash, enum status_register_num SRn)
{
	uint8_t status = flash->chip->status_register->read(flash, SRn);
	enum status_register_bit bit;
	int i;

	msg_cdbg("Chip status register %d is 0x%02x.\n", SRn + 1, status);
	for (i = 7; i >= 0; i--) {
		switch (bit = flash->chip->status_register->layout[SRn][i]) {
		case RESV:
			msg_cdbg("Chip status register %d: Bit %d is reserved\n", SRn + 1, i);
			break;
		case TB:
			msg_cdbg("Chip status register %d : %s is %s\n", SRn + 1, statreg_bit_desc[bit][1],
				(status & (1 << i)) ? "bottom" : "top");
			break;
		case SEC:
			msg_cdbg("Chip status register %d: %s is %s\n", SRn + 1, statreg_bit_desc[bit][1],
				(status & (1 << i)) ? "sectors" : "blocks");
		default:
			msg_cdbg("Chip status register %d: %s is %sset\n", SRn + 1, statreg_bit_desc[bit][1],
				(status & (1 << i)) ? "" : "not ");
			break;
		}
	}
	return 0;
}

int spi_prettyprint_status_register_wp_generic(struct flashctx *flash)
{
	uint8_t multiple = (top_status_register(flash) == SR1) ? 0 : 1;
	switch (flash->chip->status_register->get_wp_mode(flash)) {
	case WP_MODE_INVALID:
		msg_cdbg("Invalid write protection mode for status register%s\n", (multiple) ? "s" : "");
		break;
	case WP_MODE_POWER_CYCLE:
		msg_cdbg("Power supply lock down, cannot write to status register%s until next power "
			"down-up cycle\n", (multiple) ? "s" : "");
		break;
	case WP_MODE_PERMANENT:
		msg_cdbg("Status register%s permanently locked\n", (multiple) ? "s are" : " is");
		break;
	case WP_MODE_HARDWARE_PROTECTED:
		msg_cerr("Hardware protection of status register%s is active (WP# pin low),"
			" disabling impossible\n", (multiple) ? "s" : "");
		break;
	case WP_MODE_HARDWARE_UNPROTECTED:
		msg_cdbg("Hardware protection is active (WP# pin high), writes to status "
			"register%s still possible\n", (multiple) ? "s are" : " is");
		break;
	case WP_MODE_SOFTWARE:
		msg_cdbg("Write protection for status register%s is NOT in effect\n", (multiple) ? "s" : "");
		break;
	}
	return 0;
}

/* === Amic ===
 * FIXME: spi_disable_blockprotect is incorrect but works fine for chips using
 * spi_prettyprint_status_register_bp1_srwd or
 * spi_prettyprint_status_register_bp2_srwd.
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

/* used for AT25F512, AT25F1024(A), AT25F2048 */
int spi_prettyprint_status_register_at25f(struct flashctx *flash)
{
	uint8_t status;

	status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_atmel_at25_wpen(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bit(status, 5);
	spi_prettyprint_status_register_bit(status, 4);
	spi_prettyprint_status_register_bp(status, 1);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

int spi_prettyprint_status_register_at25f512a(struct flashctx *flash)
{
	uint8_t status;

	status = spi_read_status_register(flash);
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

int spi_prettyprint_status_register_at25f4096(struct flashctx *flash)
{
	uint8_t status;

	status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_atmel_at25_wpen(status);
	spi_prettyprint_status_register_bit(status, 6);
	spi_prettyprint_status_register_bit(status, 5);
	spi_prettyprint_status_register_bp(status, 2);
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

/* Some Atmel DataFlash chips support per sector protection bits and the write protection bits in the status
 * register do indicate if none, some or all sectors are protected. It is possible to globally (un)lock all
 * sectors at once by writing 0 not only the protection bits (2 and 3) but also completely unrelated bits (4 and
 * 5) which normally are not touched.
 * Affected are all known Atmel chips matched by AT2[56]D[FLQ]..1A? but the AT26DF041. */
int spi_disable_blockprotect_at2x_global_unprotect(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x0C, 1 << 7, 1 << 4, 0x00);
}

int spi_disable_blockprotect_at2x_global_unprotect_sec(struct flashctx *flash)
{
	/* FIXME: We should check the security lockdown. */
	msg_cinfo("Ignoring security lockdown (if present)\n");
	return spi_disable_blockprotect_at2x_global_unprotect(flash);
}

int spi_disable_blockprotect_at25f(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x0C, 1 << 7, 0, 0xFF);
}

int spi_disable_blockprotect_at25f512a(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x04, 1 << 7, 0, 0xFF);
}

int spi_disable_blockprotect_at25f512b(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x04, 1 << 7, 1 << 4, 0xFF);
}

int spi_disable_blockprotect_at25fs010(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x6C, 1 << 7, 0, 0xFF);
 }

int spi_disable_blockprotect_at25fs040(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x7C, 1 << 7, 0, 0xFF);
}

/* === Eon === */

int spi_prettyprint_status_register_en25s_wp(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
	spi_prettyprint_status_register_hex(status);

	spi_prettyprint_status_register_srwd(status);
	msg_cdbg("Chip status register: WP# disable (WPDIS) is %sabled\n", (status & (1 << 6)) ? "en " : "dis");
	spi_prettyprint_status_register_bp(status, 3);
	spi_prettyprint_status_register_welwip(status);
	return 0;
}

/* === Intel/Numonyx/Micron - Spansion === */

int spi_disable_blockprotect_n25q(struct flashctx *flash)
{
	return spi_disable_blockprotect_generic(flash, 0x5C, 1 << 7, 0, 0xFF);
}

int spi_prettyprint_status_register_n25q(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
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
int spi_disable_blockprotect_bp2_ep_srwd(struct flashctx *flash)
{
	return spi_disable_blockprotect_bp2_srwd(flash);
}

/* Used by Intel/Numonyx S33 and Spansion S25FL-S chips */
int spi_prettyprint_status_register_bp2_ep_srwd(struct flashctx *flash)
{
	uint8_t status = spi_read_status_register(flash);
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
