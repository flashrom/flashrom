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
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{F,S,V}29C51001B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C51001B,
		.total_size	= 128,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
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
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{F,S,V}29C51001T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C51001T,
		.total_size	= 128,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
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
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{F,S,V}29C51002B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C51002B,
		.total_size	= 256,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
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
	},

	{
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{F,S,V}29C51002T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C51002T,
		.total_size	= 256,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
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
	},

	{
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{F,S,V}29C51004B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C51004B,
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
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{F,S,V}29C51004T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C51004T,
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
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{S,V}29C31004B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C31004B,
		.total_size	= 512,
		.page_size	= 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
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
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{S,V}29C31004T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C31004T,
		.total_size	= 512,
		.page_size	= 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
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
		.voltage	= {3000, 3600},
	},
