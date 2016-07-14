/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2016 Hatim Kanchwala <hatim@hatimak.me>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "chipdrivers.h"
#include "flash.h"
#include "spi25_statusreg.h"

/* Return the top-most (highest) OTP region of flash. */
static enum otp_region top_otp_region(struct flashctx *flash) {
	enum otp_region otp_region = OTP_REG_1;
	struct region *region = flash->chip->otp->region;

	while (region[otp_region++].size != 0)
		;
	return otp_region - 2;
}

/* Some standard error checking used by program and erase functions. */
static int otp_error_check(struct flashctx *flash, enum otp_region otp_region,
	uint32_t start_byte, uint32_t len) {
	if (otp_region > top_otp_region(flash)) {
		msg_cdbg("Trying to access non-existent OTP region %d\n%s has only %d OTP regions\n",
			otp_region + 1, flash->chip->name, top_otp_region(flash) + 1);
		return 1;
	}
	if (start_byte + len > flash->chip->otp->region[otp_region].size) {
		msg_cdbg("OTP region for %s is %d bytes\n", flash->chip->name,
			flash->chip->otp->region[otp_region].size);
		return 1;
	}
	return 0;
}

/* === Eon chip specific functions === */
static uint8_t bp_bitfield, to_restore = 0;

static void save_bp(struct flashctx *flash) {
	uint8_t status = flash->chip->status_register->read(flash, SR1);
	uint32_t bp_bitmask = flash->chip->wp->bp_bitmask(flash);
	bp_bitfield = (status & bp_bitmask) >> pos_bit(flash, BP0);
}

static int restore_bp(struct flashctx *flash) {
	uint8_t status = flash->chip->status_register->read(flash, SR1);
	uint32_t bp_bitmask = flash->chip->wp->bp_bitmask(flash);
	status = ((status & ~bp_bitmask) | (bp_bitfield << pos_bit(flash, BP0))) & 0xff;
	return flash->chip->status_register->write(flash, SR1, status);
}

/* Enter OTP mode. If any Block Protect bits are set, then save
 * their state and temporarily unset them all. */
static int enter_otp_mode(struct flashctx *flash) {
	uint8_t bp = flash->chip->status_register->read(flash, SR1) & flash->chip->wp->bp_bitmask(flash);
	if (bp) {
		msg_cdbg("Need to unset all BP bits before entering OTP mode ...\n");
		msg_cdbg("BP bits will be restored to 0x%02x\n", bp >> pos_bit(flash, BP0));
		to_restore = 1;
		save_bp(flash);
	}
	return spi_enter_otp_mode(flash);
}

/* Exit OTP mode. If any Block Protect bits were set prior to issuing
 * an Enter OTP, then restore those bits after exiting. */
static int exit_otp_mode(struct flashctx *flash) {
	int result = spi_write_disable(flash);
	if (result) {
		msg_cdbg("Couldn't exit OTP mode\n");
		return result;
	}

	if (to_restore) {
		msg_cdbg("Restoring BP bits to their state prior to entering OTP mode ...\n");
		result = restore_bp(flash);
		if (result)
			msg_cdbg("Couldn't restore BP bits\n");
	}
	to_restore = 0;
	return result;
}

int eon_status_generic(struct flashctx *flash, enum otp_region otp_region) {
	enter_otp_mode(flash);
	uint8_t status = (flash->chip->status_register->read(flash, SR1) &
		(1 << pos_bit(flash, SRP0))) ? 1 : 0;
	exit_otp_mode(flash);
	return status;
}

int eon_print_status_generic(struct flashctx *flash) {
	enum otp_region top_region = top_otp_region(flash), region_n;
	msg_cdbg("%s contains %d OTP memory region%s (also called OTP sector%s) -\n",
		flash->chip->name, top_region + 1,
		(top_region == 0) ? "" : "s", (top_region == 0) ? "" : "s");

	for (region_n = OTP_REG_1; region_n <= top_region; region_n++) {
		msg_cdbg("OTP memory region %d: %d bytes, controlled by %s bit in status register %d "
			"(while in OTP mode)\n", region_n + 1, flash->chip->otp->region[region_n].size,
			statreg_bit_desc[flash->chip->otp->region[region_n].status_bit][0],
			(pos_bit(flash, flash->chip->otp->region[region_n].status_bit) / 8) + 1);
		if (flash->chip->otp->status(flash, region_n)) {
			msg_cdbg("OTP memory region %d is permanently locked and cannot be erased "
				"or written to\n", region_n + 1);
		}
	}
	return 0;
}

/* Read len bytes of the security register (corresponding to otp_region) into buf,
 * starting from start_byte. */
int eon_read_generic(struct flashctx *flash, uint8_t *buf, enum otp_region otp_region,
	uint32_t start_byte, uint32_t len) {
	int result = otp_error_check(flash, otp_region, start_byte, len);
	if (result) {
		msg_cerr("%s failed\n", __func__);
		return result;
	}

	enter_otp_mode(flash);
	result = flash->chip->read(flash, buf, flash->chip->otp->region[otp_region].addr | start_byte, len);
	exit_otp_mode(flash);

	if (result)
		msg_cerr("%s failed\n", __func__);
	return result;
}

/* Write len bytes to the security register (corresponding to otp_region) form buf,
 * starting from start_byte. */
int eon_write_generic(struct flashctx *flash, const uint8_t *buf, enum otp_region otp_region,
	uint32_t start_byte, uint32_t len) {
	int result = otp_error_check(flash, otp_region, start_byte, len);
	if (result) {
		msg_cerr("%s failed\n", __func__);
		return result;
	}
	if (flash->chip->otp->status(flash, otp_region)) {
		msg_cdbg("OTP memory region %d is permanently locked and cannot be written to\n",
			otp_region + 1);
		msg_cerr("%s failed\n", __func__);
		return 1;
	}

	enter_otp_mode(flash);
	result = flash->chip->write(flash, buf, flash->chip->otp->region[otp_region].addr | start_byte, len);
	exit_otp_mode(flash);

	if (result)
		msg_cerr("%s failed\n", __func__);
	return result;
}

/* Erase the security register corresponding to otp_region. */
int eon_erase_generic(struct flashctx *flash, enum otp_region otp_region) {
	int result = otp_error_check(flash, otp_region, 0x000000, 0);
	if (result) {
		msg_cerr("%s failed\n", __func__);
		return result;
	}
	if (flash->chip->otp->status(flash, otp_region)) {
		msg_cdbg("OTP memory region %d is permanently locked and cannot be written to\n",
			otp_region + 1);
		msg_cerr("%s failed\n", __func__);
		return 1;
	}

	enter_otp_mode(flash);
	result = spi_block_erase_20(flash, flash->chip->otp->region[otp_region].addr,
		flash->chip->otp->region[otp_region].size);
	exit_otp_mode(flash);

	if (result)
		msg_cerr("%s failed\n", __func__);
	return result;
}

/* Lock the OTP memory corresponding to otp_region. The corresponding bit
 * in the status register is set (which is one-time programmable). For Eon
 * chips, the SRP/SRP0/SRWD bit is served as OTP it while in OTP mode.
 * Note that if the bit was already set, the function does not consider
 * it a point of failure. */
int eon_lock_generic(struct flashctx *flash, enum otp_region otp_region) {
	int result = otp_error_check(flash, otp_region, 0x000000, 0);
	if (result) {
		msg_cerr("%s failed\n", __func__);
		return result;
	}
	enum status_register_bit status_bit = flash->chip->otp->region[otp_region].status_bit;
	if (pos_bit(flash, status_bit) == -1) {
		/* Check if such a bit even exists in the status register in the first place. */
		// TODO(hatim): This block does not seem to have many use cases as the error
		// can be avoided while reviewing patches itself
		msg_cdbg("OTP modifier bit %s for %s defined incorrectly\n",
			statreg_bit_desc[status_bit][0], flash->chip->name);
		msg_cerr("%s failed\n", __func__);
		return 1;
	}
	if (flash->chip->otp->status(flash, otp_region)) {
		msg_cdbg("OTP modifier bit already set, "
			"cannot alter value as it is one-time-programmable only\n");
		// FIXME(hatim): Should we return zero or non-zero here?
		return 0;
	}

	enter_otp_mode(flash);
	/* WRSR will set OTP modifier bit irrespective of status byte supplied. */
	flash->chip->status_register->write(flash, SR1, 1 << pos_bit(flash, SRP0));
	exit_otp_mode(flash);

	if (!flash->chip->otp->status(flash, otp_region)) {
		msg_cdbg("Unable to set OTP modifier bit\n");
		msg_cerr("%s failed\n", __func__);
		return 1;
	} else
		return 0;
}


/* === GigaDevice and (most) Winbond chip specific functions === */
/* Get the OTP modifier bit (these are usually the LB1, LB2, ... bits) from the status
 * registers. */
static int gd_w_get_otp_bit(struct flashctx *flash, enum status_register_bit modifier_bit) {
	enum status_register_num SRn = pos_bit(flash, modifier_bit) / 8;
	uint8_t modifier_bit_mask = 1 << (pos_bit(flash, modifier_bit) - (SRn * 8));

	uint8_t status = flash->chip->status_register->read(flash, SRn);
	return status & modifier_bit_mask;
}

/* Set the OTP modifier bit (these are usually the LB1, LB2, ... bits) in the status registers.
 * We take no value of the bit as an argument because they are one-time-programmable only and
 * they can only be set. */
static int gd_w_set_otp_bit(struct flashctx *flash, enum status_register_bit modifier_bit) {
	enum status_register_num SRn = pos_bit(flash, modifier_bit) / 8;
	uint8_t modifier_bit_mask = 1 << (pos_bit(flash, modifier_bit) - (SRn * 8));

	uint8_t status = flash->chip->status_register->read(flash, SRn);
	status = (status & ~modifier_bit_mask) & 0xff;
	status |= modifier_bit_mask;
	return flash->chip->status_register->write(flash, SRn, status);
}

int gd_w_status_generic(struct flashctx *flash, enum otp_region otp_region) {
	return gd_w_get_otp_bit(flash, flash->chip->otp->region[otp_region].status_bit) ? 1 : 0;
}

int gd_w_print_status_generic(struct flashctx *flash) {
	enum otp_region top_region = top_otp_region(flash), region_n;
	msg_cdbg("%s contains %d OTP memory region%s (also called Security Register%s) -\n",
		flash->chip->name, top_region + 1,
		(top_region == 0) ? "" : "s", (top_region == 0) ? "" : "s");

	for (region_n = OTP_REG_1; region_n <= top_region; region_n++) {
		msg_cdbg("OTP memory region %d: %d bytes, controlled by %s bit in status register %d\n",
			region_n + 1,
			flash->chip->otp->region[region_n].size,
			statreg_bit_desc[flash->chip->otp->region[region_n].status_bit][0],
			(pos_bit(flash, flash->chip->otp->region[region_n].status_bit) / 8) + 1);
		if (flash->chip->otp->status(flash, region_n)) {
			msg_cdbg("OTP memory region %d is permanently locked and cannot be erased "
				"or written to\n", region_n + 1);
		}
	}
	return 0;
}

/* Read len bytes of the security register (corresponding to otp_region) into buf,
 * starting from start_byte. */
int gd_w_read_generic(struct flashctx *flash, uint8_t *buf, enum otp_region otp_region,
	uint32_t start_byte, uint32_t len) {
	int result = otp_error_check(flash, otp_region, start_byte, len);
	if (result) {
		msg_cerr("%s failed\n", __func__);
		return result;
	}

	/* Prefix the first couple of pre-defined bits of the security register address. */
	result = spi_sec_reg_read(flash, buf, flash->chip->otp->region[otp_region].addr | start_byte, len);
	if (result)
		msg_cerr("%s failed\n", __func__);
	return result;
}

/* Write len bytes to the security register (corresponding to otp_region) form buf,
 * starting from start_byte. */
int gd_w_write_generic(struct flashctx *flash, const uint8_t *buf, enum otp_region otp_region,
	uint32_t start_byte, uint32_t len) {
	int result = otp_error_check(flash, otp_region, start_byte, len);
	if (result) {
		msg_cerr("%s failed\n", __func__);
		return result;
	}
	if (flash->chip->otp->status(flash, otp_region)) {
		msg_cdbg("OTP memory region %d is permanently locked and cannot be written to\n",
			otp_region + 1);
		msg_cerr("%s failed\n", __func__);
		return 1;
	}

	/* Prefix the first couple of pre-defined bits of the security register address. */
	result = spi_sec_reg_prog(flash, buf, flash->chip->otp->region[otp_region].addr | start_byte, len);
	if (result)
		msg_cerr("%s failed\n", __func__);
	return result;
}

/* Erase the security register corresponding to otp_region. */
int gd_erase_generic(struct flashctx *flash, enum otp_region otp_region) {
	int result = otp_error_check(flash, otp_region, 0x000000, 0);
	if (result) {
		msg_cerr("%s failed\n", __func__);
		return result;
	}
	if (flash->chip->otp->status(flash, otp_region)) {
		msg_cdbg("OTP memory region %d is permanently locked and cannot be erased\n",
			otp_region + 1);
		msg_cerr("%s failed\n", __func__);
		return 1;
	}

	result = spi_sec_reg_erase(flash, flash->chip->otp->region[otp_region].addr);
	if (result)
		msg_cerr("%s failed\n", __func__);
	return result;
}

/* Lock the OTP memory corresponding to otp_region. The corresponding bit
 * in the status register is set (which is one-time programmable).
 * Note that if the bit was already set, the function does not consider
 * it a point of failure. */
int gd_w_lock_generic(struct flashctx *flash, enum otp_region otp_region) {
	int result = otp_error_check(flash, otp_region, 0x000000, 0);
	if (result) {
		msg_cerr("%s failed\n", __func__);
		return result;
	}

	enum status_register_bit status_bit = flash->chip->otp->region[otp_region].status_bit;
	if (pos_bit(flash, status_bit) == -1) {
		/* Check if such a bit even exists in the status register in the first place. */
		// TODO(hatim): This block does not seem to have many use cases as the error
		// can be avoided while reviewing patches itself
		msg_cdbg("OTP modifier bit %s for %s defined incorrectly\n",
			statreg_bit_desc[status_bit][0], flash->chip->name);
		msg_cerr("%s failed\n", __func__);
		return 1;
	}
	if (flash->chip->otp->status(flash, otp_region)) {
		msg_cdbg("OTP modifier bit already set, "
			"cannot alter value as it is one-time-programmable only\n");
		// FIXME(hatim): Should we return zero or non-zero here?
		return 0;
	} else {
		result = gd_w_set_otp_bit(flash, status_bit);
		if (result)
			msg_cerr("%s failed\n", __func__);
		if (!flash->chip->otp->status(flash, otp_region)) {
			msg_cdbg("Unable to set OTP modifier bit\n");
			msg_cerr("%s failed\n", __func__);
			return 1;
		}
		return result;
	}
}
