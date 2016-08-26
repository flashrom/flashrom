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
#include "otp.h"

/* === Eon === */
/* EN25Q16 */
struct otp en128_2048otp = {
	.region = {
		{
			.addr = 0x1FF000,
			.size = 128,
			.status_bit = SRP0,
		},
	},
	.status = &eon_status_generic,
	.print_status = &eon_print_status_generic,
	.read = &eon_read_generic,
	.write = &eon_write_generic,
	.erase = &eon_erase_generic,
	.lock = &eon_lock_generic,
};

/* EN25Q40 */
struct otp en256_512otp = {
	.region = {
		{
			.addr = 0x07F000,
			.size = 256,
			.status_bit = SRP0,
		},
	},
	.status = &eon_status_generic,
	.print_status = &eon_print_status_generic,
	.read = &eon_read_generic,
	.write = &eon_write_generic,
	.erase = &eon_erase_generic,
	.lock = &eon_lock_generic,
};

/* EN25Q80(A) */
struct otp en256_1024otp = {
	.region = {
		{
			.addr = 0x0FF000,
			.size = 256,
			.status_bit = SRP0,
		},
	},
	.status = &eon_status_generic,
	.print_status = &eon_print_status_generic,
	.read = &eon_read_generic,
	.write = &eon_write_generic,
	.erase = &eon_erase_generic,
	.lock = &eon_lock_generic,
};

/* EN25QH16 */
struct otp en512_2048otp = {
	.region = {
		{
			.addr = 0x1FF000,
			.size = 512,
			.status_bit = SRP0,
		},
	},
	.status = &eon_status_generic,
	.print_status = &eon_print_status_generic,
	.read = &eon_read_generic,
	.write = &eon_write_generic,
	.erase = &eon_erase_generic,
	.lock = &eon_lock_generic,
};

/* EN25Q32(A/B), EN25QH32 */
struct otp en512_4096otp = {
	.region = {
		{
			.addr = 0x3FF000,
			.size = 512,
			.status_bit = SRP0,
		},
	},
	.status = &eon_status_generic,
	.print_status = &eon_print_status_generic,
	.read = &eon_read_generic,
	.write = &eon_write_generic,
	.erase = &eon_erase_generic,
	.lock = &eon_lock_generic,
};

/* EN25Q64, EN25QH64 */
struct otp en512_8192otp = {
	.region = {
		{
			.addr = 0x7FF000,
			.size = 512,
			.status_bit = SRP0,
		},
	},
	.status = &eon_status_generic,
	.print_status = &eon_print_status_generic,
	.read = &eon_read_generic,
	.write = &eon_write_generic,
	.erase = &eon_erase_generic,
	.lock = &eon_lock_generic,
};

/* EN25Q128, EN25QH128 */
struct otp en512_16384otp = {
	.region = {
		{
			.addr = 0xFFF000,
			.size = 512,
			.status_bit = SRP0,
		},
	},
	.status = &eon_status_generic,
	.print_status = &eon_print_status_generic,
	.read = &eon_read_generic,
	.write = &eon_write_generic,
	.erase = &eon_erase_generic,
	.lock = &eon_lock_generic,
};

/* === GigaDevice and Winbond === */
// FIXME(hatim): Deal with chips with shared OTP modifier bit
/* GD25LQ16, GD25LQ80, GD25LQ40, W25Q40BL, W25Q64FV, W25Q128BV
 * (There is an additional 256 bytes security register
 * at 0x000000 which is reserved and can only be read.) */
// FIXME(hatim): Add support to interact with the reserved security register
struct otp gd_w256_3_otp = {
	.region = {
		{
			.addr = 0x001000,
			.size = 256,
			.status_bit = LB1,
		}, {
			.addr = 0x002000,
			.size = 256,
			.status_bit = LB2,
		}, {
			.addr = 0x003000,
			.size = 256,
			.status_bit = LB3,
		},
	},
	.status = &gd_w_status_generic,
	.print_status = &gd_w_print_status_generic,
	.read = &gd_w_read_generic,
	.write = &gd_w_write_generic,
	.erase = &gd_erase_generic,
	.lock = &gd_w_lock_generic,
};

/* GD25VQ16C, GD25VQ40C, GD25VQ80C, GD25Q16B, GD25Q32B, GD25Q64B, GD25Q80(B), GD25Q128B */
// FIXME(hatim): Datasheets show 4 registers for program, but single 1024 B register
// for read and erase; which could be modelled as a single 1024 B register starting 0x000000
struct otp gd256_4_otp = {
	.region = {
		{
			.addr = 0x000000,
			.size = 256,
			.status_bit = LB1, /* LB1 is same as LB */
		}, {
			.addr = 0x000100,
			.size = 256,
			.status_bit = LB1, /* LB1 is same as LB */
		}, {
			.addr = 0x000200,
			.size = 256,
			.status_bit = LB1, /* LB1 is same as LB */
		}, {
			.addr = 0x000300,
			.size = 256,
			.status_bit = LB1, /* LB1 is same as LB */
		},
	},
	.status = &gd_w_status_generic,
	.print_status = &gd_w_print_status_generic,
	.read = &gd_w_read_generic,
	.write = &gd_w_write_generic,
	.erase = &gd_erase_generic,
	.lock = &gd_w_lock_generic,
};

/* GD25VQ21B, GD25VQ41B, GD25Q128C */
struct otp gd512_3_otp = {
	.region = {
		{
			.addr = 0x001000,
			.size = 512,
			.status_bit = LB1,
		}, {
			.addr = 0x002000,
			.size = 512,
			.status_bit = LB2,
		}, {
			.addr = 0x003000,
			.size = 512,
			.status_bit = LB3,
		},
	},
	.status = &gd_w_status_generic,
	.print_status = &gd_w_print_status_generic,
	.read = &gd_w_read_generic,
	.write = &gd_w_write_generic,
	.erase = &gd_erase_generic,
	.lock = &gd_w_lock_generic,
};
