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
		.vendor		= "ST",
		.name		= "M29F002B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29F002B,
		.total_size	= 256,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_AAA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 3},
				},
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4750, 5250}, /* 4.75-5.25V for type -X, others 4.5-5.5V */
	},

	{
		.vendor		= "ST",
		.name		= "M29F002T/NT",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29F002T,
		.total_size	= 256,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_AAA | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 3},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4750, 5250}, /* 4.75-5.25V for type -X, others 4.5-5.5V */
	},

	{
		.vendor		= "ST",
		.name		= "M29F040B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29F040B,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO, /* datasheet specifies no timing */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		/* FIXME: this has WORD/BYTE sequences; 2AA for word, 555 for byte */
		.vendor		= "ST",
		.name		= "M29F400BB",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29F400BB,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_SHIFTED | FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= 10, // FIXME: check datasheet. Using the 10 us from probe_m29f400bt
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 7},
				},
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		/* FIXME: this has WORD/BYTE sequences; 2AA for word, 555 for byte */
		.vendor		= "ST",
		.name		= "M29F400BT",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29F400BT,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_SHIFTED | FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= 10, // FIXME: check datasheet. Using the 10 us from probe_m29f400bt
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 7},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "ST",
		.name		= "M29W010B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29W010B,
		.total_size	= 128,
		.page_size	= 16 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {16 * 1024, 8} },
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "ST",
		.name		= "M29W040B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29W040B,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "ST",
		.name		= "M29W512B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29W512B,
		.total_size	= 64,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "ST",
		.name		= "M50FLW040A",
		.bustype	= BUS_FWH | BUS_LPC, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FLW040A,
		.total_size	= 512,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_FIXME,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 16}, /* sector */
					{64 * 1024, 5}, /* block */
					{4 * 1024, 16}, /* sector */
					{4 * 1024, 16}, /* sector */
				},
				.block_erase = STM50_SECTOR_ERASE,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = ERASE_BLOCK_82802AB,
			}
		},
		.unlock		= UNLOCK_REGSPACE2_UNIFORM_64K,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FLW040B",
		.bustype	= BUS_FWH | BUS_LPC, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FLW040B,
		.total_size	= 512,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_FIXME,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 16}, /* sector */
					{4 * 1024, 16}, /* sector */
					{64 * 1024, 5}, /* block */
					{4 * 1024, 16}, /* sector */
				},
				.block_erase = STM50_SECTOR_ERASE,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = ERASE_BLOCK_82802AB,
			}
		},
		.unlock		= UNLOCK_REGSPACE2_UNIFORM_64K,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FLW080A",
		.bustype	= BUS_FWH | BUS_LPC, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FLW080A,
		.total_size	= 1024,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_FIXME,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 16}, /* sector */
					{64 * 1024, 13}, /* block */
					{4 * 1024, 16}, /* sector */
					{4 * 1024, 16}, /* sector */
				},
				.block_erase = STM50_SECTOR_ERASE,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = ERASE_BLOCK_82802AB,
			}
		},
		.printlock	= PRINTLOCK_REGSPACE2_BLOCK_ERASER_0,
		.unlock		= UNLOCK_REGSPACE2_BLOCK_ERASER_0,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FLW080B",
		.bustype	= BUS_FWH | BUS_LPC, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FLW080B,
		.total_size	= 1024,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_FIXME,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 16}, /* sector */
					{4 * 1024, 16}, /* sector */
					{64 * 1024, 13}, /* block */
					{4 * 1024, 16}, /* sector */
				},
				.block_erase = STM50_SECTOR_ERASE,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = ERASE_BLOCK_82802AB,
			}
		},
		.printlock	= PRINTLOCK_REGSPACE2_BLOCK_ERASER_0,
		.unlock		= UNLOCK_REGSPACE2_BLOCK_ERASER_0,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FW002",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FW002,
		.total_size	= 256,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (82802ab.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 3},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = ERASE_BLOCK_82802AB,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = NO_BLOCK_ERASE_FUNC, /* Only in A/A mux mode */
			}
		},
		.printlock	= PRINTLOCK_REGSPACE2_BLOCK_ERASER_0,
		.unlock		= UNLOCK_REGSPACE2_BLOCK_ERASER_0,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FW016",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FW016,
		.total_size	= 2048,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (82802ab.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = ERASE_BLOCK_82802AB,
			}
		},
		.unlock		= UNLOCK_REGSPACE2_UNIFORM_64K,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FW040",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FW040,
		.total_size	= 512,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (82802ab.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = ERASE_BLOCK_82802AB,
			}
		},
		.unlock		= UNLOCK_REGSPACE2_UNIFORM_64K,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FW080",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FW080,
		.total_size	= 1024,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (82802ab.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = ERASE_BLOCK_82802AB,
			}
		},
		.unlock		= UNLOCK_REGSPACE2_UNIFORM_64K,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50LPW080",
		.bustype	= BUS_LPC, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50LPW080,
		.total_size	= 1024,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = ERASE_BLOCK_82802AB,
			}
		},
		.unlock		= UNLOCK_REGSPACE2_UNIFORM_64K,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50LPW116",
		.bustype	= BUS_LPC, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50LPW116,
		.total_size	= 2048,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 16},
					{64 * 1024, 30},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = ERASE_BLOCK_82802AB,
			}
		},
		.printlock	= PRINTLOCK_REGSPACE2_BLOCK_ERASER_0,
		.unlock		= UNLOCK_REGSPACE2_BLOCK_ERASER_0,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M95M02",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M95M02,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_NO_ERASE | FEATURE_ERASED_ZERO,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_ST95,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_EMULATION,
			}
		},

		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP1_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP1_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2500, 5500},
	},
