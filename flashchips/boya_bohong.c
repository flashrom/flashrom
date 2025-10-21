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
		.vendor		= "Boya/BoHong Microelectronics",
		.name		= "B.25D16A",
		.bustype	= BUS_SPI,
		.manufacture_id	= BOYA_BOHONG_ID,
		.model_id	= BOYA_BOHONG_B_25D16A,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PR,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Boya/BoHong Microelectronics",
		.name		= "B.25D80A",
		.bustype	= BUS_SPI,
		.manufacture_id	= BOYA_BOHONG_ID,
		.model_id	= BOYA_BOHONG_B__25D80A,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PR,
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
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Boya/BoHong Microelectronics",
		.name		= "B.25Q64AS",
		.bustype	= BUS_SPI,
		.manufacture_id	= BOYA_BOHONG_ID,
		.model_id	= BOYA_BOHONG_B_25Q64AS,
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
				.eraseblocks = { {16 * 512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {16 * 512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Boya/BoHong Microelectronics",
		.name		= "B.25Q128AS",
		.bustype	= BUS_SPI,
		.manufacture_id	= BOYA_BOHONG_ID,
		.model_id	= BOYA_BOHONG_B_25Q128AS,
		.total_size	= 16384,
		.page_size	= 256,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT25FS040,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},
    /* JEDEC ID 0x684017 - Huahong/Bohong BH25Q64BS (8MByte / 64Mbit) */
{
    .vendor = "Huahong/Bohong Microelectronics",
    .name = "BH25Q64BS",
    .bustype = BUS_SPI,
    .manufacture_id = 0x68,
    .model_id = BOYA_BOHONG_B_25Q64AS,         /* Memory Type 0x40, Capacity 0x17 */
    .total_size = 8192,         /* 8MB in kB */
    .page_size = 256,
    .feature_bits = FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI, 
    .tested = TEST_UNTESTED,    /* Change to TEST_OK_PREW after verification */
    .probe = PROBE_SPI_RDID,
    .probe_timing = TIMING_ZERO,
    .block_erasers =
    {
        {
            .eraseblocks = { {4 * 1024, 2048} },    /* 4KB Sector Erase (20H) */
            .block_erase = SPI_BLOCK_ERASE_20,
        }, {
            .eraseblocks = { {32 * 1024, 256} },    /* 32KB Block Erase (52H) */
            .block_erase = SPI_BLOCK_ERASE_52,
        }, {
            .eraseblocks = { {64 * 1024, 128} },    /* 64KB Block Erase (D8H) */
            .block_erase = SPI_BLOCK_ERASE_D8,
        }, {
            .eraseblocks = { {8192 * 1024, 1} },    /* Full Chip Erase (60H or C7H) */
            .block_erase = SPI_BLOCK_ERASE_60,
        }
    },
    .printlock = SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
    .unlock = SPI_DISABLE_BLOCKPROTECT_BP4_SRWD,
    .write = SPI_CHIP_WRITE256, /* Page Program size 256 bytes */
    .read = SPI_CHIP_READ,
    .voltage = {2700, 3600},    /* 2.7V to 3.6V */
},
