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
		.vendor		= "Fudan",
		.name		= "FM25F005",
		.bustype	= BUS_SPI,
		.manufacture_id	= FUDAN_ID_NOPREFIX,
		.model_id	= FUDAN_FM25F005,
		.total_size	= 64,
		.page_size	= 256,
		/* OTP: 256B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_TB_BPL,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B), dual I/O  (0x3B) supported */
		.voltage	= {2700, 3600}, /* 2.3-2.7V acceptable results in lower performance */
	},

	{
		.vendor		= "Fudan",
		.name		= "FM25F01",
		.bustype	= BUS_SPI,
		.manufacture_id	= FUDAN_ID_NOPREFIX,
		.model_id	= FUDAN_FM25F01,
		.total_size	= 128,
		.page_size	= 256,
		/* OTP: 256B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 2} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_TB_BPL,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B), dual I/O  (0x3B) supported */
		.voltage	= {2700, 3600}, /* 2.3-2.7V acceptable results in lower performance */
	},

	{
		.vendor		= "Fudan",
		.name		= "FM25F02(A)",
		.bustype	= BUS_SPI,
		.manufacture_id	= FUDAN_ID_NOPREFIX,
		.model_id	= FUDAN_FM25F02,
		.total_size	= 256,
		.page_size	= 256,
		/* OTP: 256B total; enter 0x3A, (A version only:) read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 8} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {1024 * 256, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {1024 * 256, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			},
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_TB_BPL,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B), dual I/O  (0x3B) supported */
		.voltage	= {2700, 3600}, /* 2.3-2.7V acceptable results in lower performance */
	},

	{
		.vendor		= "Fudan",
		.name		= "FM25F04(A)",
		.bustype	= BUS_SPI,
		.manufacture_id	= FUDAN_ID_NOPREFIX,
		.model_id	= FUDAN_FM25F04,
		.total_size	= 512,
		.page_size	= 256,
		/* OTP: 256B total; enter 0x3A, (A version only:) read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {1024 * 512, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {1024 * 512, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			},
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_TB_BPL,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B), dual I/O  (0x3B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Fudan",
		.name		= "FM25Q04",
		.bustype	= BUS_SPI,
		.manufacture_id	= FUDAN_ID_NOPREFIX,
		.model_id	= FUDAN_FM25Q04,
		.total_size	= 512,
		.page_size	= 256,
		/* supports SFDP */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				/* 128 * 4KB sectors */
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				/* 16 * 32KB blocks */
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				/* 8 * 64KB blocks  */
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				/* Full chip erase (0x60)  */
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				/* Full chip erase (0xC7)  */
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			},
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_TB_BPL,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Fudan",
		.name		= "FM25Q08",
		.bustype	= BUS_SPI,
		.manufacture_id	= FUDAN_ID_NOPREFIX,
		.model_id	= FUDAN_FM25Q08,
		.total_size	= 1024,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
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
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			},
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_TB_BPL, /* bit6 selects size of protected blocks; TODO: SR2 */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Fudan",
		.name		= "FM25Q16",
		.bustype	= BUS_SPI,
		.manufacture_id	= FUDAN_ID_NOPREFIX,
		.model_id	= FUDAN_FM25Q16,
		.total_size	= 2048,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PR,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_TB_BPL, /* bit6 selects size of protected blocks; TODO: SR2 */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Fudan",
		.name		= "FM25Q32",
		.bustype	= BUS_SPI,
		.manufacture_id	= FUDAN_ID_NOPREFIX,
		.model_id	= FUDAN_FM25Q32,
		.total_size	= 4096,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
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
			},
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_TB_BPL, /* bit6 selects size of protected blocks; TODO: SR2 */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Fudan",
		.name		= "FM25Q64",
		.bustype	= BUS_SPI,
		.manufacture_id	= FUDAN_ID_NOPREFIX,
		.model_id	= FUDAN_FM25Q64,
		.total_size	= 8192,
		.page_size	= 256,
		/* supports SFDP */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				/* 2048 * 4KB sectors */
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				/* 256 * 32KB blocks */
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				/* 128 * 64KB blocks  */
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				/* Full chip erase (0x60)  */
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				/* Full chip erase (0xC7)  */
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			},
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_TB_BPL, /* bit6 selects size of protected blocks; TODO: SR2 */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Fudan",
		.name		= "FM25Q128",
		.bustype	= BUS_SPI,
		.manufacture_id	= FUDAN_ID_NOPREFIX,
		.model_id	= FUDAN_FM25Q128,
		.total_size	= 16384,
		.page_size	= 256,
		/* supports SFDP */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PR,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				/* 4096 * 4KB sectors */
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				/* 512 * 32KB blocks */
				.eraseblocks = { {32 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				/* 256 * 64KB blocks  */
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				/* Full chip erase (0x60)  */
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				/* Full chip erase (0xC7)  */
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			},
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_TB_BPL, /* bit6 selects size of protected blocks; TODO: SR2 */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},
