/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000 Silicon Integrated System Corporation
 * SPDX-FileCopyrightText: 2004 Tyan Corp
 * SPDX-FileCopyrightText: 2005-2008 coresystems GmbH <stepan@openbios.org>
 * SPDX-FileCopyrightText: 2006-2009 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2009 Sean Nelson <audiohacked@gmail.com>
 * SPDX-FileCopyrightText: 2025 Antonio Vázquez Blanco <antoniovazquezblanco@gmail.com>
 */

	{
		.vendor		= "Nantronics",
		.name		= "N25S10",
		.bustype	= BUS_SPI,
		.manufacture_id	= NANTRONICS_ID_NOPREFIX,
		.model_id	= NANTRONICS_N25S10,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * KiB, 32} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {4 * KiB, 32} },
				.block_erase = SPI_BLOCK_ERASE_D7,
			}, {
				.eraseblocks = { {32 * KiB, 4} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * KiB, 2} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {128 * KiB, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {128 * KiB, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP3_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B), dual I/O read (0x3B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Nantronics",
		.name		= "N25S16",
		.bustype	= BUS_SPI,
		.manufacture_id	= NANTRONICS_ID_NOPREFIX,
		.model_id	= NANTRONICS_N25S16,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * KiB, 512} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {64 * KiB, 32} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {2 * MiB, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {2 * MiB, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP3_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B), dual I/O read (0x3B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Nantronics",
		.name		= "N25S20",
		.bustype	= BUS_SPI,
		.manufacture_id	= NANTRONICS_ID_NOPREFIX,
		.model_id	= NANTRONICS_N25S20,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * KiB, 64} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {4 * KiB, 64} },
				.block_erase = SPI_BLOCK_ERASE_D7,
			}, {
				.eraseblocks = { {32 * KiB, 8} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * KiB, 4} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {256 * KiB, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {256 * KiB, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP3_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B), dual I/O read (0x3B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Nantronics",
		.name		= "N25S40",
		.bustype	= BUS_SPI,
		.manufacture_id	= NANTRONICS_ID_NOPREFIX,
		.model_id	= NANTRONICS_N25S40,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * KiB, 128} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {4 * KiB, 128} },
				.block_erase = SPI_BLOCK_ERASE_D7,
			}, {
				.eraseblocks = { {32 * KiB, 16} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * KiB, 8} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {512 * KiB, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {512 * KiB, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP3_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B), dual I/O read (0x3B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Nantronics",
		.name		= "N25S80",
		.bustype	= BUS_SPI,
		.manufacture_id	= NANTRONICS_ID_NOPREFIX,
		.model_id	= NANTRONICS_N25S80,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * KiB, 256} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * KiB, 32} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * KiB, 16} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {1 * MiB, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {1 * MiB, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP3_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B), dual I/O read (0x3B) supported */
		.voltage	= {2700, 3600},
	},
