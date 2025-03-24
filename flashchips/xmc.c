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
		.vendor		= "XMC",
		.name		= "XM25QH80B",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M45PE80,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {1 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {1 * 1024 * 1024, 1} },
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
		.vendor		= "XMC",
		.name		= "XM25QU80B",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QU80B,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256 } },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {1 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {1 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QH16C/XM25QH16D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QH16C,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QU16C",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QU16C,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QH32C/XM25QH32D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QH32C,
		.total_size	= 4096,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QU32C",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QU32C,
		.total_size	= 4096,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QH64C/XM25QH64D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QH64C,
		.total_size	= 8192,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QU64C/XM25LU64C",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QU64C,
		.total_size	= 8192,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QH64A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QH64A,
		.total_size	= 8192,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 512B total; enter 0x3A */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 8192 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 8192 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD, /* bit6 is quad enable */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP3_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QH128A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QH128A,
		.total_size	= 16384,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI | FEATURE_WRSR2,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}},
			.tb     = {STATUS1, 5, RW},
			.sec    = {STATUS1, 6, RW},
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QH128C/XM25QH128D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QH128C,
		.total_size	= 16384,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI | FEATURE_WRSR2,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_SRWD_SEC_TB_BP2_WELWIP,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}},
			.tb     = {STATUS1, 5, RW},
			.sec    = {STATUS1, 6, RW},
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QU128C/XM25QU128D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QU128C,
		.total_size	= 16384,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN, /* TODO: improve */
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QH256C/XM25QH256D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QH256C,
		.total_size	= 32768,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_ENTER_WREN |
				  FEATURE_4BA_EAR_C5C8 | FEATURE_4BA_READ | FEATURE_4BA_FAST_READ |
				  FEATURE_4BA_WRITE | FEATURE_WRSR2,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 8192} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN, /* TODO: improve */
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}, {STATUS1, 5, RW}},
			.tb     = {STATUS1, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QU256C/XM25QU256D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QU256C,
		.total_size	= 32768,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_ENTER_WREN
			| FEATURE_4BA_EAR_C5C8 | FEATURE_4BA_READ | FEATURE_4BA_FAST_READ
			| FEATURE_4BA_WRITE,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 8192} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN, /* TODO: improve */
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "XMC",
		.name		= "XM25RU256C",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25RU256C,
		.total_size	= 32768,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_ENTER_WREN |
				  FEATURE_4BA_EAR_C5C8 | FEATURE_4BA_READ | FEATURE_4BA_FAST_READ |
				  FEATURE_4BA_WRITE,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 8192} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN, /* TODO: improve */
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QH512C/XM25QH512D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QH512C,
		.total_size	= 64 * 1024,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		/* FOUR_BYTE_ADDR: supports 4-bytes addressing mode */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_ENTER_WREN
			| FEATURE_4BA_EAR_C5C8 | FEATURE_4BA_READ | FEATURE_4BA_FAST_READ,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 16384} },
				.block_erase = SPI_BLOCK_ERASE_21,
			}, {
				.eraseblocks = { {4 * 1024, 16384} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 2048} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_DC,
			}, {
				.eraseblocks = { {64 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {64 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {64 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN, /* TODO: improve */
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "XMC",
		.name		= "XM25QU512C/XM25QU512D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= XMC_XM25QU512C,
		.total_size	= 64 * 1024,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		/* FOUR_BYTE_ADDR: supports 4-bytes addressing mode */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_ENTER_WREN
			| FEATURE_4BA_EAR_C5C8 | FEATURE_4BA_READ | FEATURE_4BA_FAST_READ,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 16384} },
				.block_erase = SPI_BLOCK_ERASE_21,
			}, {
				.eraseblocks = { {4 * 1024, 16384} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 2048} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_DC,
			}, {
				.eraseblocks = { {64 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {64 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {64 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN, /* TODO: improve */
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},
