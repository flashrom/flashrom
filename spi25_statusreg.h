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

#ifndef __SPI25_STATUSREG_H__
#define __SPI25_STATUSREG_H__ 1

#include "flash.h"

/* It has been observed that chips have at most 3 status registers. Please
 * update the define if a chip with higher status registers is found. */
#define MAX_STATUS_REGISTERS 3

enum bit_state {
	DONT_CARE = -1,	/* Don't care */
	OFF = 0,
	ON = 1,
};

/* The following enum defines bits found in status registers. Datasheets
 * will mostly name bits in the same manner. */
// TODO(hatim): Add other remaining bits
enum status_register_bit {
	INVALID_BIT = 0,	/* This must always stay at the top. */
	RESV, WIP, WEL,		/* WIP is also referred as BUSY. */
	/* SRP0 is same as SRP and SRWD. */
	SRP0, SRP1, BPL, WP, CMP, WPS, QE, SUS, SUS1, SUS2, DRV0, DRV1, RST, HPF, LPE, AAI,
	APT, CP,
	/* The order of the following bits must not be altered and newer entries must not
	 * be inserted in between them. */
	BP0, BP1, BP2, BP3, BP4, TB, SEC, LB1, LB2, LB3 /* LB1 is same as LB. */
};

enum status_register_num { SR1 = 0, SR2, SR3 };

/* The following enum defines write protection modes available for status registers. */
enum wp_mode {
	WP_MODE_INVALID = 0,

	/* Status register is unlocked and can be written to after a WREN. */
	WP_MODE_SOFTWARE,

	/* When WP# is low, status register is locked and can not be written to.
	 * In this mode SRP0=1, and SRP1=1 if present. */
	WP_MODE_HARDWARE_PROTECTED,

	/* When WP# is high status register is unlocked and can be written
	 * to after WREN. In this mode SRP0=1. */
	WP_MODE_HARDWARE_UNPROTECTED,

	/* Status register is protected and can not be written to until next
	 * power down/up cycle, post which status register will be unlocked
	 * and can be written to after a WREN. */
	WP_MODE_POWER_CYCLE,

	/* Status register is permanently protected and cannot be written to. */
	WP_MODE_PERMANENT,
};

/* Describes corresponding bits from enum status_register_bit */
extern char *statreg_bit_desc[][2];

#endif		/* !__SPI25_STATUSREG_H__ */
