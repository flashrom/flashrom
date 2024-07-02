/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2004 Tyan Corp
 * Copyright (C) 2005-2008 coresystems GmbH <stepan@openbios.org>
 * Copyright (C) 2006-2009 Carl-Daniel Hailfinger
 * Copyright (C) 2009 Sean Nelson <audiohacked@gmail.com>
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
 */

	{
		.vendor		= "Sharp",
		.name		= "LH28F008BJT-BTLZ1",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SHARP_ID,
		.model_id	= SHARP_LH28F008BJ__PB,
		.total_size	= 1024,
		.page_size	= 64 * 1024,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 15}
				 },
				.block_erase = ERASE_BLOCK_82802AB,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = ERASE_SECTOR_49LFXXXC,
			}
		},
		.unlock		= UNLOCK_LH28F008BJT,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Sharp",
		.name		= "LHF00L04",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= SHARP_ID,
		.model_id	= SHARP_LHF00L04,
		.total_size	= 1024,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET | FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 15},
					{8 * 1024, 8}
				 },
				.block_erase = ERASE_BLOCK_82802AB,
			}, {
				.eraseblocks = {
					{1024 * 1024, 1}
				},
				.block_erase = NO_BLOCK_ERASE_FUNC, /* 30 D0, only in A/A mux mode */
			},
		},
		.unlock		= UNLOCK_REGSPACE2_UNIFORM_64K,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600},
	},
