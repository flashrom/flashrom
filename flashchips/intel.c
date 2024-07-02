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
		.vendor		= "Intel",
		.name		= "25F160S33B8",
		.bustype	= BUS_SPI,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_25F160S33B8,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 506B total (2x 8B, 30x 16B, 1x 10B); read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 8 so-called "parameter blocks" with
				 * opcode 0x40. Trying to access an address outside these 8 8kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 31} // inaccessible
				},
				.block_erase = SPI_BLOCK_ERASE_40,
			}, { */
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_EP_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,	/* also fast read 0x0B */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "25F160S33T8",
		.bustype	= BUS_SPI,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_25F160S33T8,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 506B total (2x 8B, 30x 16B, 1x 10B); read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 8 so-called "parameter blocks" with
				 * opcode 0x40. Trying to access an address outside these 8 8kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{64 * 1024, 31}, // inaccessible
					{8 * 1024, 8}
				},
				.block_erase = SPI_BLOCK_ERASE_40,
			}, { */
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_EP_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,	/* also fast read 0x0B */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "25F320S33B8",
		.bustype	= BUS_SPI,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_25F320S33B8,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 506B total (2x 8B, 30x 16B, 1x 10B); read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 8 so-called "parameter blocks" with
				 * opcode 0x40. Trying to access an address outside these 8 8kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 63} // inaccessible
				},
				.block_erase = SPI_BLOCK_ERASE_40,
			}, { */
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_EP_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,	/* also fast read 0x0B */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "25F320S33T8",
		.bustype	= BUS_SPI,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_25F320S33T8,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 506B total (2x 8B, 30x 16B, 1x 10B); read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 8 so-called "parameter blocks" with
				 * opcode 0x40. Trying to access an address outside these 8 8kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{64 * 1024, 63}, // inaccessible
					{8 * 1024, 8}
				},
				.block_erase = SPI_BLOCK_ERASE_40,
			}, { */
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_EP_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,	/* also fast read 0x0B */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "25F640S33B8",
		.bustype	= BUS_SPI,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_25F640S33B8,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 506B total (2x 8B, 30x 16B, 1x 10B); read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 8 so-called "parameter blocks" with
				 * opcode 0x40. Trying to access an address outside these 8 8kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 127} // inaccessible
				},
				.block_erase = SPI_BLOCK_ERASE_40,
			}, { */
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_EP_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,	/* also fast read 0x0B */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "25F640S33T8",
		.bustype	= BUS_SPI,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_25F640S33T8,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 506B total (2x 8B, 30x 16B, 1x 10B); read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 8 so-called "parameter blocks" with
				 * opcode 0x40. Trying to access an address outside these 8 8kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{64 * 1024, 127}, // inaccessible
					{8 * 1024, 8}
				},
				.block_erase = SPI_BLOCK_ERASE_40,
			}, { */
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_EP_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,	/* also fast read 0x0B */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "28F001BN/BX-B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F001B,
		.total_size	= 128,
		.page_size	= 128 * 1024, /* 8k + 2x4k + 112k */
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 1},
					{4 * 1024, 2},
					{112 * 1024, 1},
				},
				.block_erase = ERASE_BLOCK_82802AB,
			},
		},
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Intel",
		.name		= "28F001BN/BX-T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F001T,
		.total_size	= 128,
		.page_size	= 128 * 1024, /* 112k + 2x4k + 8k */
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{112 * 1024, 1},
					{4 * 1024, 2},
					{8 * 1024, 1},
				},
				.block_erase = ERASE_BLOCK_82802AB,
			},
		},
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Intel",
		.name		= "28F002BC/BL/BV/BX-T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F002T,
		.total_size	= 256,
		.page_size	= 256 * 1024,
		.tested		= TEST_OK_PRE,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_ZERO, /* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{128 * 1024, 1},
					{96 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = ERASE_BLOCK_82802AB,
			},
		},
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
	},

	{
		.vendor		= "Intel",
		.name		= "28F004B5/BE/BV/BX-B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F004B,
		.total_size	= 512,
		.page_size	= 128 * 1024, /* maximal block size */
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{96 * 1024, 1},
					{128 * 1024, 3},
				},
				.block_erase = ERASE_BLOCK_82802AB,
			},
		},
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
	},

	{
		.vendor		= "Intel",
		.name		= "28F004B5/BE/BV/BX-T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F004T,
		.total_size	= 512,
		.page_size	= 128 * 1024, /* maximal block size */
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{128 * 1024, 3},
					{96 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = ERASE_BLOCK_82802AB,
			},
		},
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
	},

	{
		.vendor		= "Intel",
		.name		= "28F008S3/S5/SC",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F004S3,
		.total_size	= 512,
		.page_size	= 256,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = ERASE_BLOCK_82802AB,
			},
		},
		.unlock		= UNLOCK_28F004S5,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
	},

	{
		.vendor		= "Intel",
		.name		= "28F400BV/BX/CE/CV-B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F400B,
		.total_size	= 512,
		.page_size	= 128 * 1024, /* maximal block size */
		.feature_bits	= FEATURE_ADDR_SHIFTED,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{96 * 1024, 1},
					{128 * 1024, 3},
				},
				.block_erase = ERASE_BLOCK_82802AB,
			},
		},
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
	},

	{
		.vendor		= "Intel",
		.name		= "28F400BV/BX/CE/CV-T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F400T,
		.total_size	= 512,
		.page_size	= 128 * 1024, /* maximal block size */
		.feature_bits	= FEATURE_ADDR_SHIFTED,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{128 * 1024, 3},
					{96 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = ERASE_BLOCK_82802AB,
			},
		},
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
	},

	{
		.vendor		= "Intel",
		.name		= "AT82802AB",
		.bustype	= BUS_FWH,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_82802AB,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_IGNORED, /* routine does not use probe_timing (82802ab.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = ERASE_BLOCK_82802AB,
			},
		},
		.unlock		= UNLOCK_REGSPACE2_UNIFORM_64K,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "82802AC",
		.bustype	= BUS_FWH,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_82802AC,
		.total_size	= 1024,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_IGNORED, /* routine does not use probe_timing (82802ab.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = ERASE_BLOCK_82802AB,
			},
		},
		.unlock		= UNLOCK_REGSPACE2_UNIFORM_64K,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600},
	},
