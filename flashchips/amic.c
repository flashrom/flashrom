/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000 Silicon Integrated System Corporation
 * SPDX-FileCopyrightText: 2004 Tyan Corp
 * SPDX-FileCopyrightText: 2005-2008 coresystems GmbH <stepan@openbios.org>
 * SPDX-FileCopyrightText: 2006-2009 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2009 Sean Nelson <audiohacked@gmail.com>
 * SPDX-FileCopyrightText: 2025 Antonio Vázquez Blanco <antoniovazquezlbanco@gmail.com>
 */

	{
		.vendor		= "AMIC",
		.name		= "A25L010",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L010,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 32 } },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 64 * 1024, 2 } },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 128 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L016",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L016,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 512 } },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 64 * 1024, 32 } },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 2048 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L020",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L020,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 64 } },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 64 * 1024, 4 } },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 256 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L032",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L032,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 64B total; read 0x4B, 0x48; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 1024 } },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 64 * 1024, 64 } },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { { 64 * 1024, 64 } },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 4096 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 4096 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_SRWD_SEC_TB_BP2_WELWIP,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L040",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L040,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 128 } },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 64 * 1024, 8 } },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 512 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L05PT",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L05PT,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP1_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L05PU",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L05PU,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
				},
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP1_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L080",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L080,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 256 } },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 64 * 1024, 16 } },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 1024 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L10PT",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L10PT,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 1},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP1_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L10PU",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L10PU,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 1},
				},
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP1_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L16PT",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L16PT,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 31},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L16PU",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L16PU,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PR,
		.probe		= PROBE_SPI_RDID4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 31},
				},
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L20PT",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L20PT,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 3},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP1_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L20PU",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L20PU,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 3},
				},
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP1_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	/* The A25L40P{T,U} chips are distinguished by their
	 * erase block layouts, but without any distinction in RDID.
	 * This inexplicable quirk was verified by Rudolf Marek
	 * and discussed on the flashrom mailing list on 2010-07-12.
	 */
	{
		.vendor		= "AMIC",
		.name		= "A25L40PT",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L40PT,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PR,
		.probe		= PROBE_SPI_RDID4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 7},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L40PU",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L40PU,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 7},
				},
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L512",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L512,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 16 } },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 64 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 64 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L80P",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L80P,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PRE,
		.probe		= PROBE_SPI_RDID4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 15},
				},
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25LQ032/A25LQ32A",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25LQ032,
		.total_size	= 4096,
		.page_size	= 256,
		/* A25LQ32A supports SFDP */
		/* OTP: 64B total; read 0x4B, 0x48; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 1024 } },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 64 * 1024, 64 } },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { { 64 * 1024, 64 } },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 4096 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 4096 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_SRWD_SEC_TB_BP2_WELWIP,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25LQ16",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25LQ16,
		.total_size	= 2048,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, 0x48; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 512 } },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 64 * 1024, 32 } },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { { 64 * 1024, 32 } },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 2048 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 2048 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_SRWD_SEC_TB_BP2_WELWIP,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25LQ64",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25LQ64,
		.total_size	= 8192,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 512B total; enter 0xB1, exit 0xC1 */
		/* QPI enable 0x35, disable 0xF5 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 2048 } },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 32 * 1024, 256 } },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { { 64 * 1024, 128 } },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 8192 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 8192 * 1024, 1 } },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD, /* bit6 is quad enhance (sic!) */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP3_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A29002B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A29002B,
		.total_size	= 256,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,
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
			},
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "AMIC",
		.name		= "A29002T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A29002T,
		.total_size	= 256,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,
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
			},
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "AMIC",
		.name		= "A29040B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A29040B,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_OK_PREW,
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

	{
		.vendor		= "AMIC",
		.name		= "A49LF040A",
		.bustype	= BUS_LPC,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A49LF040A,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PR,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* routine is wrapper to probe_jedec (pm49fl00x.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = JEDEC_BLOCK_ERASE,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.unlock		= UNLOCK_REGSPACE2_UNIFORM_64K,
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600},
	},
