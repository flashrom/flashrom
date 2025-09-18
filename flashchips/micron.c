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
		.vendor		= "Micron",
		.name		= "MT25QL01G", /* L = 3V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_N25Q00A__3G,
		.total_size	= 131072,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32768} },
				.block_erase = SPI_BLOCK_ERASE_21,
			}, {
				.eraseblocks = { {4 * 1024, 32768} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_5C,
			}, {
				.eraseblocks = { {32 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 2048} },
				.block_erase = SPI_BLOCK_ERASE_DC,
			}, {
				.eraseblocks = { {64 * 1024, 2048} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {65536 * 1024, 2} },
				.block_erase = SPI_BLOCK_ERASE_C4,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_N25Q, /* TODO: config, lock, flag regs */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_N25Q, /* TODO: per 64kB sector lock registers */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron",
		.name		= "MT25QU01G", /* U = 1.8V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_N25Q00A__1G,
		.total_size	= 131072,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32768} },
				.block_erase = SPI_BLOCK_ERASE_21,
			}, {
				.eraseblocks = { {4 * 1024, 32768} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_5C,
			}, {
				.eraseblocks = { {32 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 2048} },
				.block_erase = SPI_BLOCK_ERASE_DC,
			}, {
				.eraseblocks = { {64 * 1024, 2048} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {65536 * 1024, 2} },
				.block_erase = SPI_BLOCK_ERASE_C4,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_N25Q, /* TODO: config, lock, flag regs */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_N25Q, /* TODO: per 64kB sector lock registers */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Micron",
		.name		= "MT25QL02G", /* L = 3V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_MT25QL02G,
		.total_size	= 262144,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 65536} },
				.block_erase = SPI_BLOCK_ERASE_21,
			}, {
				.eraseblocks = { {4 * 1024, 65536} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 8192} },
				.block_erase = SPI_BLOCK_ERASE_5C,
			}, {
				.eraseblocks = { {32 * 1024, 8192} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_DC,
			}, {
				.eraseblocks = { {64 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {65536 * 1024, 4} },
				.block_erase = SPI_BLOCK_ERASE_C4,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_N25Q, /* TODO: config, lock, flag regs */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_N25Q, /* TODO: per 64kB sector lock registers */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron",
		.name		= "MT25QU02G", /* U = 1.8V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_MT25QU02G,
		.total_size	= 262144,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 65536} },
				.block_erase = SPI_BLOCK_ERASE_21,
			}, {
				.eraseblocks = { {4 * 1024, 65536} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 8192} },
				.block_erase = SPI_BLOCK_ERASE_5C,
			}, {
				.eraseblocks = { {32 * 1024, 8192} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_DC,
			}, {
				.eraseblocks = { {64 * 1024, 4096} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {65536 * 1024, 4} },
				.block_erase = SPI_BLOCK_ERASE_C4,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_N25Q, /* TODO: config, lock, flag regs */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_N25Q, /* TODO: per 64kB sector lock registers */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Micron",
		.name		= "MT25QU128", /* U = 1.8V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_N25Q128__1E,
		.total_size	= 16384,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_WREN,
		.tested		= TEST_UNTESTED,
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
				.eraseblocks = { {16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}, {
				.eraseblocks = { {16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_N25Q, /* TODO: config, lock, flag regs */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_N25Q, /* TODO: per 64kB sector lock registers */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Micron",
		.name		= "MT25QL128", /* L = 3V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_N25Q128__3E,
		.total_size	= 16384,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
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
				.eraseblocks = { {32 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}, {
				.eraseblocks = { {16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_N25Q, /* TODO: config, lock, flag regs */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_N25Q, /* TODO: per 64kB sector lock registers */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron",
		.name		= "MT25QL256", /* L = 3V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_N25Q256__3E,
		.total_size	= 32768,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_WREN,
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
				.eraseblocks = { {32768 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}, {
				.eraseblocks = { {32768 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_N25Q, /* TODO: config, lock, flag regs */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_N25Q, /* TODO: per 64kB sector lock registers */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron",
		.name		= "MT25QU256", /* U = 1.8V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_N25Q256__1E,
		.total_size	= 32768,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_WREN,
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
				.eraseblocks = { {32768 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}, {
				.eraseblocks = { {32768 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_N25Q, /* TODO: config, lock, flag regs */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_N25Q, /* TODO: per 64kB sector lock registers */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Micron",
		.name		= "MT25QL512", /* L = 3V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_N25Q512__3G,
		.total_size	= 65536,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_WREN,
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
				.block_erase = SPI_BLOCK_ERASE_5C,
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
				.eraseblocks = { {65536 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}, {
				.eraseblocks = { {65536 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_N25Q, /* TODO: config, lock, flag regs */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_N25Q, /* TODO: per 64kB sector lock registers */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}, {STATUS1, 6, RW}},
			.tb     = {STATUS1, 5, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "Micron",
		.name		= "MT25QU512", /* U = 1.8V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_N25Q512__1G,
		.total_size	= 65536,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA_WREN,
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
				.block_erase = SPI_BLOCK_ERASE_5C,
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
				.eraseblocks = { {65536 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}, {
				.eraseblocks = { {65536 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_N25Q, /* TODO: config, lock, flag regs */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_N25Q, /* TODO: per 64kB sector lock registers */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1700, 2000},
	},
