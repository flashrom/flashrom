/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000 Silicon Integrated System Corporation
 * SPDX-FileCopyrightText: 2004 Tyan Corp
 * SPDX-FileCopyrightText: 2005-2008 coresystems GmbH <stepan@openbios.org>
 * SPDX-FileCopyrightText: 2006-2009 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2009 Sean Nelson <audiohacked@gmail.com>
 * SPDX-FileCopyrightText: 2025 Antonio Vázquez Blanco <antoniovazquezblanco@gmail.com>
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
