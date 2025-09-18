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
		.vendor		= "MoselVitelic",
		.name		= "V29C51000B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29C51000B,
		.total_size	= 64,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 128} },
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			},
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "MoselVitelic",
		.name		= "V29C51000T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29C51000T,
		.total_size	= 64,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 128} },
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			},
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "MoselVitelic",
		.name		= "V29C51400B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29C51400B,
		.total_size	= 512,
		.page_size	= 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024, 512} },
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

	{
		.vendor		= "MoselVitelic",
		.name		= "V29C51400T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29C51400T,
		.total_size	= 512,
		.page_size	= 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024, 512} },
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

	{
		.vendor		= "MoselVitelic",
		.name		= "V29LC51000",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29LC51000,
		.total_size	= 64,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 128} },
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			},
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "MoselVitelic",
		.name		= "V29LC51001",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29LC51001,
		.total_size	= 128,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 256} },
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			},
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "MoselVitelic",
		.name		= "V29LC51002",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29LC51002,
		.total_size	= 256,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 512} },
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			},
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},
