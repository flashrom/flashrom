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
		.vendor		= "ESI",
		.name		= "ES25P16",
		.bustype	= BUS_SPI,
		.manufacture_id	= EXCEL_ID_NOPREFIX,
		.model_id	= EXCEL_ES25P16,
		.total_size	= 2 * 1024,
		.page_size	= 256,
		/* 256-byte parameter page separate from memory array:
		 * supports read (0x53), fast read (0x5B), erase (0xD5) and program (0x52) instructions. */
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast Read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "ESI",
		.name		= "ES25P40",
		.bustype	= BUS_SPI,
		.manufacture_id	= EXCEL_ID_NOPREFIX,
		.model_id	= EXCEL_ES25P40,
		.total_size	= 512,
		.page_size	= 256,
		/* 256-byte parameter page separate from memory array:
		 * supports read (0x53), fast read (0x5B), erase (0xD5) and program (0x52) instructions. */
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast Read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "ESI",
		.name		= "ES25P80",
		.bustype	= BUS_SPI,
		.manufacture_id	= EXCEL_ID_NOPREFIX,
		.model_id	= EXCEL_ES25P80,
		.total_size	= 1024,
		.page_size	= 256,
		/* 256-byte parameter page separate from memory array:
		 * supports read (0x53), fast read (0x5B), erase (0xD5) and program (0x52) instructions. */
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast Read (0x0B) supported */
		.voltage	= {2700, 3600},
	},
