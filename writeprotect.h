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

#ifndef __WRITEPROTECT_H__
#define __WRITEPROTECT_H__ 1

#include "flash.h"

#define LEN_RANGES 64

struct range {
	uint32_t start;
	uint32_t len;	/* in kB */
};

extern struct wp a25l032_32a_wp;
extern struct wp gd_w_wp;
extern struct wp en25qh128_wp;
extern struct wp en25q128_wp;
extern struct wp en25qh64_wp;
extern struct wp en25q64_wp;
extern struct wp en25qh32_wp;
extern struct wp en25q32ab_wp;
extern struct wp en25qh16_wp;
extern struct wp en25q16_wp;
extern struct wp en25q32_wp;
extern struct wp en25q80a_wp;
extern struct wp en25q40_wp;
extern struct wp mx25l16xd_wp;
extern struct wp mx25l6405d_wp;
extern struct wp mx25lx5d_wp;
extern struct wp mx25lx65e_wp;

#endif		/* !__WRITEPROTECT_H__ */
