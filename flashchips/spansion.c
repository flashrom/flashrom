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
		.vendor		= "Spansion",
		.name		= "S25FL004A",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL004A,
		.total_size	= 512,
		.page_size	= 256,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN, /* TODO: improve */
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL008A",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL008A,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PRE,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN, /* TODO: improve */
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL016A",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL016A,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN, /* TODO: improve */
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL032A/P",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL032A,
		.total_size	= 4096,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
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
		.vendor		= "Spansion",
		.name		= "S25FL064A/P",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL064A,
		.total_size	= 8192,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
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
		.vendor		= "Spansion",
		.name		= "S25FL116K/S25FL216K", /* FIXME: separate them */
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL216,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 (S25FL116K only) */
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
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 2048 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 2048 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP3_SRWD, /* #WP pin write-protects SRWP bit. */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and dual I/O (0x3B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL127S-256kB", /* uniform 256kB sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL128,
		.total_size	= 16384,
		.page_size	= 512,
		/* supports 4B addressing */
		/* OTP: 1024B total, 32B reserved; read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD, /* #WP pin write-protects SRWP bit. */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL127S-64kB", /* hybrid: 32 (top or bottom) 4 kB sub-sectors + 64 kB sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL128,
		.total_size	= 16384,
		.page_size	= 256,
		/* supports 4B addressing */
		/* OTP: 1024B total, 32B reserved; read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		/* FIXME: we should distinguish the configuration on probing time like we do for AT45DB chips */
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of 32 so-called "parameter sectors" with
				 * opcode 0x20 which may be configured to be on top or bottom of the address
				 * space. Trying to access an address outside these 4kB blocks does have no
				 * effect on the memory contents, e.g.
				.eraseblocks = {
					{4 * 1024, 32},
					{64 * 1024, 254} // inaccessible
				},
				.block_erase = SPI_BLOCK_ERASE_20,
			}, { */
				.eraseblocks = { { 64 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD, /* #WP pin write-protects SRWP bit. */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL128L",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL128L,
		.total_size	= 16384,
		.page_size	= 256,
		/* 4 x 256B Security Region (OTP) */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_WRSR_EXT3 | FEATURE_OTP,
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
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			/*
			 * Note: This chip has a read-only Status Register 2 that is not
			 *	 counted here. Registers are mapped as follows:
			 *	 STATUS1 ... Status Register 1
			 *	 STATUS2 ... Configuration Register 1
			 *	 STATUS3 ... Configuration Register 2
			 */
			.srp	= {STATUS1, 7, RW},
			.srl	= {STATUS2, 0, RW},
			.bp	= {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}},
			.tb	= {STATUS1, 5, RW},
			.sec	= {STATUS1, 6, RW},
			.cmp	= {STATUS2, 6, RW},
			.wps	= {STATUS3, 2, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL128P......0", /* uniform 64 kB sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL128,
		.total_size	= 16384,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP3_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL128P......1", /* uniform 256kB sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL128,
		.total_size	= 16384,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL128S......0", /* hybrid: 32 (top or bottom) 4 kB sub-sectors + 64 kB sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL128,
		.total_size	= 16384,
		.page_size	= 256,
		/* supports 4B addressing */
		/* OTP: 1024B total, 32B reserved; read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 32 so-called "parameter sectors" with
				 * opcode 0x20. Trying to access an address outside these 4kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{4 * 1024, 32},
					{64 * 1024, 254} // inaccessible
				},
				.block_erase = SPI_BLOCK_ERASE_20,
			}, { */
				.eraseblocks = { { 64 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD_SR2,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD, /* TODO: various other locks */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL128S......1", /* uniform 256 kB sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL128,
		.total_size	= 16384,
		.page_size	= 512,
		/* supports 4B addressing */
		/* OTP: 1024B total, 32B reserved; read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD_SR2,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD, /* TODO: various other locks */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL128S_UL Uniform 128 kB Sectors",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL128S_UL,
		.total_size	= 16384,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_BIG_SPANSION,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			},
		},
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL128S_US Uniform 64 kB Sectors",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL128S_US,
		.total_size	= 16384,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_BIG_SPANSION,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			},
		},
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL129P......0", /* hybrid: 32 (top or bottom) 4 kB sub-sectors + 64 kB sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL128,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 506B total, 16B reserved; read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
			/* FIXME: This chip supports erasing of the 32 so-called "parameter sectors" with
			 * opcode 0x20. Trying to access an address outside these 4kB blocks does have no
			 * effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{4 * 1024, 32},
					{64 * 1024, 254} // inaccessible
				},
				.block_erase = SPI_BLOCK_ERASE_20,
			}, { */
			/* FIXME: Additionally it also supports erase opcode 40h for the respective 2*4 kB pairs
				.eraseblocks = {
					{8 * 1024, 16},
					{64 * 1024, 254} // inaccessible
				},
				.block_erase = SPI_BLOCK_ERASE_40,
			}, { */
				.eraseblocks = { { 64 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD, /* TODO: Configuration register */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL129P......1", /* uniform 256 kB sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL128,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 506B total, 16B reserved; read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD, /* TODO: Configuration register */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL132K",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL132K,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 768B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 4096 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 4096 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD, /* TODO: improve */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD, /* #WP pin write-protects SRWP bit. */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL164K",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL164K,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_SRWD, /* TODO: improve */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD, /* #WP pin write-protects SRWP bit. */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL204K",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL204,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PR,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP3_SRWD, /* #WP pin write-protects SRWP bit. */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and dual I/O (0x3B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL208K",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL208,
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
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP3_SRWD, /* #WP pin write-protects SRWP bit. */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and dual I/O (0x3B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL256L",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL256L,
		.total_size	= 32768,
		.page_size	= 256,
		/* 4 x 256B Security Region (OTP) */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_WRSR_EXT3 | FEATURE_OTP |
				  FEATURE_4BA_ENTER | FEATURE_4BA_NATIVE,
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
				.block_erase = SPI_BLOCK_ERASE_53,
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
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {32768 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP3_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP3_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			/*
			 * Note: This chip has a read-only Status Register 2 that is not
			 *	 counted here. Registers are mapped as follows:
			 *	 STATUS1 ... Status Register 1
			 *	 STATUS2 ... Configuration Register 1
			 *	 STATUS3 ... Configuration Register 2
			 */
			.srp	= {STATUS1, 7, RW},
			.srl	= {STATUS2, 0, RW},
			.bp	= {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}, {STATUS1, 5, RW}},
			.tb	= {STATUS1, 6, RW},
			.cmp	= {STATUS2, 6, RW},
			.wps	= {STATUS3, 2, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL256S Large Sectors",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL256S_UL,
		.total_size	= 16384,  /* This is just half the size.... */
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_BIG_SPANSION,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 64} },
				.block_erase = S25FL_BLOCK_ERASE,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			},
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD_SR2,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL256S Small Sectors",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL256S_US,
		.total_size	= 16384,   /* This is just half the size.... */
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_BIG_SPANSION,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = S25FL_BLOCK_ERASE,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			},
		},
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL256S......0", /* hybrid: 32 (top or bottom) 4 kB sub-sectors + 64 kB sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL256,
		.total_size	= 32768,
		.page_size	= 256,
		/* OTP: 1024B total, 32B reserved; read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP |
				  FEATURE_4BA_NATIVE | FEATURE_4BA_ENTER_EAR7 | FEATURE_4BA_EAR_1716,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 32 so-called "parameter sectors" with
				 * opcode 0x20. Trying to access an address outside these 4kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{4 * 1024, 32},
					{64 * 1024, 254} // inaccessible
				},
				.block_erase = SPI_BLOCK_ERASE_20,
			}, { */
				.eraseblocks = { { 64 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_DC,
			}, {
				.eraseblocks = { { 64 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 32768 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 32768 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD_SR2,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD, /* TODO: various other locks */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL512S",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL512S_UL,
		.total_size	= 65536, /* 512 Mb (=> 64 MB)) */
		.page_size	= 256,
		/* OTP: 1024B total, 32B reserved; read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP |
				  FEATURE_4BA_NATIVE | FEATURE_4BA_ENTER_EAR7 | FEATURE_4BA_EAR_1716,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_BIG_SPANSION,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 256 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_DC,
			}, {
				.eraseblocks = { { 256 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 65536 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 65536 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD_SR2,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD, /* TODO: various other locks */
		.write		= SPI_CHIP_WRITE256, /* Multi I/O supported, IGNORE for now */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FS128S Large Sectors",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FS128S_L,
		.total_size	= 16384,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_BIG_SPANSION,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = S25FS_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			},
		},
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FS128S Small Sectors",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FS128S_S,
		.total_size	= 16384,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_BIG_SPANSION,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = S25FS_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			},
		},
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FS512S",
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FS512S_UL,
		.total_size	= 65536,
		.page_size	= 256, /* 256 or 512 bytes page programming buffer */
		/* OTP: 1024B total, 32B reserved; read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI |
				  FEATURE_4BA_NATIVE | FEATURE_4BA_ENTER,
				  /* Note on FEATURE_4BA_ENTER: the command set only defines command 0xB7
				     to enter 4BA mode, but there is no counterpart command "Exit 4BA mode",
				     and the code 0xE9 is assigned to another command ("Password unlock"). */
				  /* Note on FEATURE_4BA_ENTER_EAR7 (not set): the "Extended address mode" bit
				     is stored in configuration register CR2V[7], which can only be read / written
				     by generic commands ("Read any register" / "Write any register"), that itself
				     expect a 3- or 4-byte address of register in question, which in turn depends on
				     the same register, that is initially set from non-volatile register CR2NV[7]
				     that defaults to 0, but can be programmed to 1 for starting in 32-bit mode. */
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_BIG_SPANSION,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 256 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_DC,
			}, {
				.eraseblocks = { { 256 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { { 65536 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { { 65536 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP2_EP_SRWD_SR2,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD, /* TODO: various other locks */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1700, 2000},
	},
