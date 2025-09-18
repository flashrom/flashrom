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
		.vendor		= "Bright",
		.name		= "BM29F040",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= BRIGHT_ID,
		.model_id	= BRIGHT_BM29F040,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PR,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			},
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},
