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
		.vendor		= "ISSI",
		.name		= "IS25LP016",
		.bustype	= BUS_SPI,
		.manufacture_id = ISSI_ID_SPI,
		.model_id	= ISSI_IS25LP016,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 1024B total; read 0x48; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_D7,
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
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP3_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS25LP064",
		.bustype	= BUS_SPI,
		.manufacture_id	= ISSI_ID_SPI,
		.model_id	= ISSI_IS25LP064,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 1024B total; read 0x48; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = SPI_BLOCK_ERASE_D7,
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
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS25LP128",
		.bustype	= BUS_SPI,
		.manufacture_id	= ISSI_ID_SPI,
		.model_id	= ISSI_IS25LP128,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 1024B total; read 0x48; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_D7,
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
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS25LP256",
		.bustype	= BUS_SPI,
		.manufacture_id	= ISSI_ID_SPI,
		.model_id	= ISSI_IS25LP256,
		.total_size	= 32768,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total; read 0x68; write 0x62, erase 0x64, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP |
				  FEATURE_4BA | FEATURE_4BA_ENTER_EAR7 | FEATURE_4BA_EAR_1716,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 8192} },
				.block_erase = SPI_BLOCK_ERASE_21,
			}, {
				.eraseblocks = { {4 * 1024, 8192} },
				.block_erase = SPI_BLOCK_ERASE_20,
				/* could also use SPI_BLOCK_ERASE_D7 */
			}, {
				.eraseblocks = { {32 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_5C,
			}, {
				.eraseblocks = { {32 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_DC,
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
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2300, 3600},
	},


	{
		.vendor		= "ISSI",
		.name		= "IS25LQ016",
		.bustype	= BUS_SPI,
		.manufacture_id	= ISSI_ID_SPI,
		.model_id	= ISSI_IS25LQ016,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 256B total; read 0x4b; write 0xb1 */
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
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_D7,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS25WP016",
		.bustype	= BUS_SPI,
		.manufacture_id	= ISSI_ID_SPI,
		.model_id	= ISSI_IS25WP016,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 1024B total; read 0x48; write 0x42 */
		/* QPI enable 0x35, disable 0xF5 */
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
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_D7,
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
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS25WP020",
		.bustype	= BUS_SPI,
		.manufacture_id	= ISSI_ID_SPI,
		.model_id	= ISSI_IS25WP020,
		.total_size	= 256,
		.page_size	= 256,
		/* OTP: 1024B total; read 0x48; write 0x42 */
		/* QPI enable 0x35, disable 0xF5 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_D7,
			}, {
				.eraseblocks = { {32 * 1024, 8} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS25WP032",
		.bustype	= BUS_SPI,
		.manufacture_id	= ISSI_ID_SPI,
		.model_id	= ISSI_IS25WP032,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 1024B total; read 0x48; write 0x42 */
		/* QPI enable 0x35, disable 0xF5 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_D7,
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
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS25WP040",
		.bustype	= BUS_SPI,
		.manufacture_id	= ISSI_ID_SPI,
		.model_id	= ISSI_IS25WP040,
		.total_size	= 512,
		.page_size	= 256,
		/* OTP: 1024B total; read 0x48; write 0x42 */
		/* QPI enable 0x35, disable 0xF5 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_D7,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS25WP064",
		.bustype	= BUS_SPI,
		.manufacture_id	= ISSI_ID_SPI,
		.model_id	= ISSI_IS25WP064,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 1024B total; read 0x48; write 0x42 */
		/* QPI enable 0x35, disable 0xF5 */
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
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = SPI_BLOCK_ERASE_D7,
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
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS25WP080",
		.bustype	= BUS_SPI,
		.manufacture_id	= ISSI_ID_SPI,
		.model_id	= ISSI_IS25WP080,
		.total_size	= 1024,
		.page_size	= 256,
		/* OTP: 1024B total; read 0x48; write 0x42 */
		/* QPI enable 0x35, disable 0xF5 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_D7,
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
			}
		},
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS25WP128",
		.bustype	= BUS_SPI,
		.manufacture_id	= ISSI_ID_SPI,
		.model_id	= ISSI_IS25WP128,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 1024B total; read 0x48; write 0x42 */
		/* QPI enable 0x35, disable 0xF5 */
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
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_D7,
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
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS25WP256",
		.bustype	= BUS_SPI,
		.manufacture_id	= ISSI_ID_SPI,
		.model_id	= ISSI_IS25WP256,
		.total_size	= 32768,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total; read 0x68; write 0x62, erase 0x64, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP |
				  FEATURE_4BA | FEATURE_4BA_ENTER_EAR7 | FEATURE_4BA_EAR_1716,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 8192} },
				.block_erase = SPI_BLOCK_ERASE_21,
			}, {
				.eraseblocks = { {4 * 1024, 8192} },
				.block_erase = SPI_BLOCK_ERASE_20,
				/* could also use SPI_BLOCK_ERASE_D7 */
			}, {
				.eraseblocks = { {32 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_5C,
			}, {
				.eraseblocks = { {32 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_DC,
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
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS25WQ040",
		.bustype	= BUS_SPI,
		.manufacture_id	= ISSI_ID_SPI,
		.model_id	= ISSI_IS25WQ040,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_D7,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS29GL064B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ISSI_ID,
		.model_id	= ISSI_PMC_IS29GL064B,
		.total_size	= 8192,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC_29GL,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 127},
				},
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			},
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS29GL064H/L",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ISSI_ID,
		.model_id	= ISSI_PMC_IS29GL064HL,
		.total_size	= 8192,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC_29GL,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			},
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS29GL064T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ISSI_ID,
		.model_id	= ISSI_PMC_IS29GL064T,
		.total_size	= 8192,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC_29GL,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 127},
					{8 * 1024, 8},
				},
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			},
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "ISSI",
		.name		= "IS29GL128H/L",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ISSI_ID,
		.model_id	= ISSI_PMC_IS29GL128HL,
		.total_size	= 16384,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC_29GL,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 128} },
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			},
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {2700, 3600},
	},
