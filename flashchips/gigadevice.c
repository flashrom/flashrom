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
		.vendor		= "GigaDevice",
		.name		= "GD25B128B/GD25Q128B",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25Q128,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
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
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25LF128E",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25LF128E,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR_EXT2,
		.tested		= TEST_OK_PREWB,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1695, 1950},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}},
			.tb     = {STATUS1, 5, RW}, /* Called BP3 in datasheet, acts like TB */
			.sec    = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like SEC */
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25LF256F",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25LF256F,
		.total_size	= 32768,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR_EXT2 | FEATURE_4BA,
		.tested		= TEST_OK_PREWB,
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
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1650, 2000},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}, {STATUS1, 5, RW}},
			.tb    = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like TB */
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25LF512MF",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25LF512MF,
		.total_size	= 65536,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR_EXT2 | FEATURE_4BA,
		.tested		= TEST_OK_PREWB,
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
				.eraseblocks = { {64 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {64 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1695, 1950},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}, {STATUS1, 5, RW}},
			.tb     = {STATUS1, 6, RW},	/* Called BP4 in datasheet, acts like TB */
			.srl    = {STATUS2, 0, RW},
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25LQ128E/GD25LB128E/GD25LR128E/GD25LQ128D/GD25LQ128C",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25LQ128CD,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR_EXT2,
		.tested		= TEST_OK_PREWB,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1695, 1950},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}},
			.tb     = {STATUS1, 5, RW}, /* Called BP3 in datasheet, acts like TB */
			.sec    = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like SEC */
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25LQ255E",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25LQ255E,
		.total_size	= 32768,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR_EXT2 | FEATURE_4BA,
		.tested		= TEST_OK_PREWB,
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
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1650, 2000},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}},
			.tb     = {STATUS1, 5, RW}, /* Called BP3 in datasheet, acts like TB */
			.sec    = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like SEC */
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25LQ16",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25LQ16,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1695, 1950},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25LQ32",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25LQ32,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1695, 1950},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25LQ40",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25LQ40,
		.total_size	= 512,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
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
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1695, 1950},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25LQ64(B)",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25LQ64,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR_EXT2,
		.tested		= TEST_OK_PREWB,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1695, 1950},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}},
			.tb     = {STATUS1, 5, RW}, /* Called BP3 in datasheet, acts like TB */
			.sec    = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like SEC */
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25LQ80",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25LQ80,
		.total_size	= 1024,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1695, 1950},
	},

	{
		.vendor         = "GigaDevice",
		.name           = "GD25LB256E/GD25LR256E",
		.bustype        = BUS_SPI,
		.manufacture_id = GIGADEVICE_ID,
		.model_id       = GIGADEVICE_GD25LR256E,
		.total_size     = 32768,
		.page_size      = 256,
		.feature_bits   = FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI | FEATURE_4BA,
		.tested         = TEST_OK_PREWB,
		.probe          = PROBE_SPI_RDID,
		.probe_timing   = TIMING_ZERO,
		.block_erasers  =
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
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock      = SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock         = SPI_DISABLE_BLOCKPROTECT_BP4_SRWD,
		.write          = SPI_CHIP_WRITE256,
		.read           = SPI_CHIP_READ,
		.voltage        = {1650, 2000},
		.reg_bits       =
		{
			.srp    = {STATUS1, 7, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}, {STATUS1, 5, RW}},
			.tb     = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like TB */
		},
		.decode_range   = DECODE_RANGE_SPI25,

	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25LB256F/GD25LR256F",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25LQ255E,
		.total_size	= 32768,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR_EXT2 | FEATURE_4BA,
		.tested		= TEST_OK_PREWB,
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
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1650, 2000},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}, {STATUS1, 5, RW}},
			.tb     = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like TB */
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25LB512MF/GD25LR512MF",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25LB512MF,
		.total_size	= 65536,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR_EXT2 | FEATURE_4BA,
		.tested		= TEST_OK_PREWB,
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
				.eraseblocks = { {64 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {64 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock      = SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock         = SPI_DISABLE_BLOCKPROTECT_BP4_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1695, 1950},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}, {STATUS1, 5, RW}},
			.tb     = {STATUS1, 6, RW},	/* Called BP4 in datasheet, acts like TB */
			.srl    = {STATUS2, 0, RW},
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor         = "GigaDevice",
		.name           = "GD25LB512ME/GD25LR512ME",
		.bustype        = BUS_SPI,
		.manufacture_id = GIGADEVICE_ID,
		.model_id       = GIGADEVICE_GD25LR512ME,
		.total_size     = 65536,
		.page_size      = 256,
		/* OTP: 4096B total; read 0x48; write 0x42, erase 0x44 */
		.feature_bits   = FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI | FEATURE_4BA,
		.tested         = TEST_OK_PREWB,
		.probe          = PROBE_SPI_RDID,
		.probe_timing   = TIMING_ZERO,
		.block_erasers  =
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
				.eraseblocks = { {64 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {64 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock      = SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock         = SPI_DISABLE_BLOCKPROTECT_BP4_SRWD,
		.write          = SPI_CHIP_WRITE256,
		.read           = SPI_CHIP_READ,
		.voltage        = {1650, 2000},
		.reg_bits       =
		{
			.srp    = {STATUS1, 7, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}, {STATUS1, 5, RW}},
			.tb     = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like TB */
		},
		.decode_range   = DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25Q10",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25Q10,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25Q128E/GD25B128E/GD25R128E/GD25Q127C",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25Q128,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 3072B total; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR2,
		.tested		= TEST_OK_PREWB,
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
		/* TODO: 2nd status reg (read 0x35, write 0x31) and 3rd status reg (read 0x15, write 0x11) */
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}},
			.tb     = {STATUS1, 5, RW}, /* Called BP3 in datasheet, acts like TB */
			.sec    = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like SEC */
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25Q128C",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25Q128,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 1536B total; read 0x48; write 0x42, erase 0x44 */
		/* QPI: enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI | FEATURE_WRSR2,
		.tested		= TEST_OK_PREWB,
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
		/* TODO: 2nd status reg (read 0x35, write 0x31) and 3rd status reg (read 0x15, write 0x11) */
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}},
			.tb     = {STATUS1, 5, RW}, /* Called BP3 in datasheet, acts like TB */
			.sec    = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like SEC */
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25Q16(B)",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25Q16,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 (B version only) */
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25Q20(B)",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25Q20,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
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
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25Q256E/GD25B256E/GD25R256E/GD25Q256D",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25Q256D,
		.total_size	= 32768,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_4BA | FEATURE_WRSR2 | FEATURE_WRSR3,
		.tested		= TEST_OK_PREWB,
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
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 6, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}, {STATUS1, 5, RW}},
			.tb     = {STATUS1, 6, RW},	/* Called BP4 in datasheet, acts like TB */
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

{
		.vendor		= "GigaDevice",
		.name		= "GD25B512MF/GD25R512MF",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25B512MF,
		.total_size	= 65536,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR2 | FEATURE_WRSR3 | FEATURE_4BA,
		.tested		= TEST_OK_PREWB,
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
				.eraseblocks = { {64 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {64 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD,	/* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 6, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}, {STATUS1, 5, RW}},
			.tb     = {STATUS1, 6, RW},	/* Called BP4 in datasheet, acts like TB */
			.cmp    = {STATUS3, 3, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25Q32(B)",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25Q32,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR2,
		.tested		= TEST_OK_PREWB,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}},
			.tb     = {STATUS1, 5, RW}, /* Called BP3 in datasheet, acts like TB */
			.sec    = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like SEC */
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25Q40(B)",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25Q40,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
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
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25Q512",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25Q512,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25Q64(B)",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25Q64,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR2,
		.tested		= TEST_OK_PREWB,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}},
			.tb     = {STATUS1, 5, RW}, /* Called BP3 in datasheet, acts like TB */
			.sec    = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like SEC */
			.cmp    = {STATUS2, 6, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25Q80(B)",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25Q80,
		.total_size	= 1024,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 (B version only) */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
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
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25T80",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25T80,
		.total_size	= 1024,
		.page_size	= 256,
		/* OTP: 256B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN, /* TODO: improve */
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25VQ16C",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25VQ16C,
		.total_size	= 2 * 1024,
		.page_size	= 256,
		/* Supports SFDP */
		/* OTP: 1024B total; read 0x48, write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 512} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 32 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { { 64 * 1024, 32} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25VQ21B",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25VQ21B,
		.total_size	= 256,
		.page_size	= 256,
		/* OTP: 1536B total; read 0x48, write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 64} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 32 * 1024, 8} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { { 64 * 1024, 4} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25VQ40C",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25VQ41B,
		.total_size	= 512,
		.page_size	= 256,
		/* Supports SFDP */
		/* OTP: 1024B total; read 0x48, write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 32 * 1024, 16} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { { 64 * 1024, 8} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25VQ41B",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25VQ41B,
		.total_size	= 512,
		.page_size	= 256,
		/* OTP: 1536B total; read 0x48, write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 32 * 1024, 16} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { { 64 * 1024, 8} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25VQ80C",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25VQ80C,
		.total_size	= 1024,
		.page_size	= 256,
		/* Supports SFDP */
		/* OTP: 1024B total; read 0x48, write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { { 32 * 1024, 32} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { { 64 * 1024, 16} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2300, 3600},
	},

{
		.vendor		= "GigaDevice",
		.name		= "GD25F64F",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25F64F,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR2 | FEATURE_WRSR3,
		.tested		= TEST_OK_PREWB,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			.srp    = {STATUS1, 7, RW},
			.srl    = {STATUS2, 0, RW},
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}},
			.tb     = {STATUS1, 5, RW}, /* Called BP3 in datasheet, acts like TB */
			.sec    = {STATUS1, 6, RW}, /* Called BP4 in datasheet, acts like SEC */
			.cmp    = {STATUS3, 4, RW},
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

{
		.vendor		= "GigaDevice",
		.name		= "GD25F128F",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25F128F,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 3x2048B; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_WRSR2 | FEATURE_WRSR3,
		.tested		= TEST_OK_PREWB,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD, /* TODO: 2nd status reg (read with 0x35) */
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}, {STATUS1, 5, RW}},
			.tb     = {STATUS1, 6, RW},	/* Called BP4 in datasheet, acts like TB */
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25F256F",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25F256F,
		.total_size	= 32768,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI | FEATURE_WRSR2 | FEATURE_WRSR3 | FEATURE_4BA,
		.tested		= TEST_OK_PREWB,
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
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {32 * 1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
		.reg_bits	=
		{
			.bp     = {{STATUS1, 2, RW}, {STATUS1, 3, RW}, {STATUS1, 4, RW}, {STATUS1, 5, RW}},
			.tb     = {STATUS1, 6, RW},	/* Called BP4 in datasheet, acts like TB */
		},
		.decode_range	= DECODE_RANGE_SPI25,
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25WQ80E",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25WQ80E,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_BP4_SRWD,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 3600},
	},
