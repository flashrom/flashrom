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

#ifndef __OTP_H__
#define __OTP_H__ 1

#define MAX_OTP_REGIONS 4

enum otp_region { OTP_REG_1 = 0, OTP_REG_2, OTP_REG_3 };

extern struct otp en128_2048otp;
extern struct otp en256_512otp;
extern struct otp en256_1024otp;
extern struct otp en512_2048otp;
extern struct otp en512_4096otp;
extern struct otp en512_8192otp;
extern struct otp en512_16384otp;
extern struct otp gd_w256_3_otp;
extern struct otp gd256_4_otp;
extern struct otp gd512_3_otp;

#endif		/* !__OTP_H__ */
