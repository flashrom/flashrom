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
		.vendor		= "Atmel",
		.name		= "AT25DF011",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25DF011,
		.total_size	= 128,
		.page_size	= 256,
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
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
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25DF021",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25DF021,
		.total_size	= 256,
		.page_size	= 256,
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600}, /* 2.3-3.6V & 2.7-3.6V models available */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25DF021A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25DF021A,
		.total_size	= 256,
		.page_size	= 256,
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1650, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25DF041A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25DF041A,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600}, /* 2.3-3.6V & 2.7-3.6V models available */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25DF081",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25DF081,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
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
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1600, 2000}, /* Datasheet says range is 1.65-1.95 V */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25DF081A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25DF081A,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
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
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF_SEC,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT_SEC,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25DF161",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25DF161,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF_SEC,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT_SEC,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	/*The AT26DF321 has the same ID as the AT25DF321. */
	{
		.vendor		= "Atmel",
		.name		= "AT25DF321",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25DF321,
		.total_size	= 4096,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25DF321A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25DF321A,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF_SEC,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT_SEC,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25DF641(A)",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25DF641,
		.total_size	= 8192,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF_SEC,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT_SEC,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25DL081",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25DF081,
		.total_size	= 1024,
		.page_size	= 256,
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF_SEC,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT_SEC,
		.write		= SPI_CHIP_WRITE256, /* Dual I/O  (0xA2) supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B), dual I/O  (0x3B) supported */
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25DL161",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25DL161,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF_SEC,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT_SEC,
		.write		= SPI_CHIP_WRITE256, /* Dual I/O  (0xA2) supported */
		.read		= SPI_CHIP_READ, /* Fast read (0x0B), dual I/O  (0x3B) supported */
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25DQ161",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25DQ161,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF_SEC,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT_SEC,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		/* The A suffix indicates 33MHz instead of 20MHz clock rate.
		 * All other properties seem to be the same.*/
		.name		= "AT25F1024(A)",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25F1024,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_AT25F,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_62,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25F,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT25F,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25F2048",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25F2048,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_AT25F,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_62,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25F,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT25F,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25F4096",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25F4096,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_AT25F,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_62,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25F4096,
		/* "Bits 5-6 are 0s when device is not in an internal write cycle." Better leave them alone: */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP2_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25F512",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25F512,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_AT25F,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_62,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25F,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT25F,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25F512A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25F512A,
		.total_size	= 64,
		.page_size	= 128,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_AT25F,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_62,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25F512A,
		/* FIXME: It is not correct to use this one, because the BP1 bit is N/A. */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT25F512A,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25F512B",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25F512B,
		.total_size	= 64,
		.page_size	= 256,
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
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
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_62,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25F512B,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT25F512B,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25FS010",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25FS010,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = SPI_BLOCK_ERASE_D7,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = SPI_BLOCK_ERASE_52,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = SPI_BLOCK_ERASE_D8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25FS010,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT25FS010,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25FS040",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25FS040,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25FS040,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT25FS040,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25SF041",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25SF041,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2500, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25SF081",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25SF081,
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
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25SF128A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25SF128A,
		.total_size	= 16384,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PR,
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
		.unlock		= SPI_DISABLE_BLOCKPROTECT_BP4_SRWD,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25SF161",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25SF161,
		.total_size	= 2048,
		.page_size	= 256,
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
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2500, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25SF321",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25SF321,
		.total_size	= 4096,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PR,
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
				.eraseblocks = { {4096 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {4096 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2500, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25SL128A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT25SL128A,
		.total_size	= 16384,
		.page_size	= 256,
		/* supports SFDP */
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN, /* TODO: improve */
		.unlock		= SPI_DISABLE_BLOCKPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {1700, 2000},
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
		.vendor		= "Atmel",
		.name		= "AT26DF041",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT26DF041,
		.total_size	= 512,
		.page_size	= 256,
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256, 2048} },
				.block_erase = SPI_BLOCK_ERASE_81,
			}, {
				.eraseblocks = { {2 * 1024, 256} },
				.block_erase = SPI_BLOCK_ERASE_50,
			}, {
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = SPI_BLOCK_ERASE_20,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		/* Supports also an incompatible page write (of exactly 256 B) and an auto-erasing write. */
		.write		= SPI_CHIP_WRITE1,
		.read		= SPI_CHIP_READ, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600}, /* 3.0-3.6V for higher speed, 2.7-3.6V normal */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT26DF081A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT26DF081A,
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
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_BLOCK_ERASE_C7,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT26DF081A,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT26DF161",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT26DF161,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT25DF,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT26DF161A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT26DF161A,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT26DF081A,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT2X_GLOBAL_UNPROTECT,
		.write		= SPI_CHIP_WRITE256,
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT26F004",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT26F004,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= {.probe = NT, .read = NT, .erase = NT, .write = BAD, .wp = NA},
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
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN, /* TODO: improve */
		.write		= 0, /* Incompatible Page write */
		.read		= SPI_CHIP_READ,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT29C010A",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT29C010A,
		.total_size	= 128,
		.page_size	= 128,
		.feature_bits	= FEATURE_LONG_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_JEDEC,
		.probe_timing	= 10000, /* 10mS, Enter=Exec */
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC,	/* FIXME */
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT29C020",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT29C020,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_LONG_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_JEDEC,
		.probe_timing	= 10000,			/* 10ms */
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT29C040A",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT29C040A,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_LONG_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= 10000,			/* 10 ms */
		.block_erasers	=
		{
			{
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT29C512",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT29C512,
		.total_size	= 64,
		.page_size	= 128,
		.feature_bits	= FEATURE_LONG_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_JEDEC,
		.probe_timing	= 10000, /* 10mS, Enter=Exec */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45CS1282",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45CS1282,
		.total_size	= 16896, /* No power of two sizes */
		.page_size	= 1056, /* No power of two sizes */
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77 (4 dummy bytes); write 0x9A (via buffer) */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1056, 1},    /* sector 0a:      opcode 50h */
					{248 * 1056, 1},  /* sector 0b:      opcode 7Ch */
					{256 * 1056, 63}, /* sectors 1 - 63: opcode 7Ch */
				},
				.block_erase = SPI_ERASE_AT45CS_SECTOR,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_PLAIN,
		.write		= SPI_WRITE_AT45DB,
		.read		= SPI_READ_AT45DB,
		.voltage	= {2700, 3600},
		.gran		= WRITE_GRAN_1056BYTES,
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB011D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB011D,
		.total_size	= 128, /* or 132, determined from status register */
		.page_size	= 256, /* or 264, determined from status register */
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_AT45DB,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256, 512} },
				.block_erase = SPI_ERASE_AT45DB_PAGE,
			}, {
				.eraseblocks = { {8 * 256, 512/8} },
				.block_erase = SPI_ERASE_AT45DB_BLOCK,
			}, {
				.eraseblocks = {
					{8 * 256, 1},
					{120 * 256, 1},
					{128 * 256, 3},
				},
				.block_erase = SPI_ERASE_AT45DB_SECTOR
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = SPI_ERASE_AT45DB_CHIP,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT45DB,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT45DB, /* Impossible if locked down or #WP is low */
		/* granularity will be set by the probing function. */
		.write		= SPI_WRITE_AT45DB,
		.read		= SPI_READ_AT45DB, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB021D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB021D,
		.total_size	= 256, /* or 264, determined from status register */
		.page_size	= 256, /* or 264, determined from status register */
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_AT45DB,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256, 1024} },
				.block_erase = SPI_ERASE_AT45DB_PAGE,
			}, {
				.eraseblocks = { {8 * 256, 1024/8} },
				.block_erase = SPI_ERASE_AT45DB_BLOCK,
			}, {
				.eraseblocks = {
					{8 * 256, 1},
					{120 * 256, 1},
					{128 * 256, 7},
				},
				.block_erase = SPI_ERASE_AT45DB_SECTOR
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = SPI_ERASE_AT45DB_CHIP,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT45DB,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT45DB, /* Impossible if locked down or #WP is low */
		/* granularity will be set by the probing function. */
		.write		= SPI_WRITE_AT45DB,
		.read		= SPI_READ_AT45DB, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB041D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB041D,
		.total_size	= 512, /* or 528, determined from status register */
		.page_size	= 256, /* or 264, determined from status register */
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_AT45DB,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256, 2048} },
				.block_erase = SPI_ERASE_AT45DB_PAGE,
			}, {
				.eraseblocks = { {8 * 256, 2048/8} },
				.block_erase = SPI_ERASE_AT45DB_BLOCK,
			}, {
				.eraseblocks = {
					{8 * 256, 1},
					{248 * 256, 1},
					{256 * 256, 7},
				},
				.block_erase = SPI_ERASE_AT45DB_SECTOR
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = SPI_ERASE_AT45DB_CHIP,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT45DB,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT45DB, /* Impossible if locked down or #WP is low */
		/* granularity will be set by the probing function. */
		.write		= SPI_WRITE_AT45DB,
		.read		= SPI_READ_AT45DB, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600}, /* 2.5-3.6V & 2.7-3.6V models available */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB081D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB081D,
		.total_size	= 1024, /* or 1056, determined from status register */
		.page_size	= 256, /* or 264, determined from status register */
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_AT45DB,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256, 4096} },
				.block_erase = SPI_ERASE_AT45DB_PAGE,
			}, {
				.eraseblocks = { {8 * 256, 4096/8} },
				.block_erase = SPI_ERASE_AT45DB_BLOCK,
			}, {
				.eraseblocks = {
					{8 * 256, 1},
					{248 * 256, 1},
					{256 * 256, 15},
				},
				.block_erase = SPI_ERASE_AT45DB_SECTOR
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = SPI_ERASE_AT45DB_CHIP,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT45DB,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT45DB, /* Impossible if locked down or #WP is low */
		/* granularity will be set by the probing function. */
		.write		= SPI_WRITE_AT45DB,
		.read		= SPI_READ_AT45DB, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600}, /* 2.5-3.6V & 2.7-3.6V models available */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB161D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB161D,
		.total_size	= 2048, /* or 2112, determined from status register */
		.page_size	= 512, /* or 528, determined from status register */
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_AT45DB,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 4096} },
				.block_erase = SPI_ERASE_AT45DB_PAGE,
			}, {
				.eraseblocks = { {8 * 512, 4096/8} },
				.block_erase = SPI_ERASE_AT45DB_BLOCK,
			}, {
				.eraseblocks = {
					{8 * 512, 1},
					{248 * 512, 1},
					{256 * 512, 15},
				},
				.block_erase = SPI_ERASE_AT45DB_SECTOR
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = SPI_ERASE_AT45DB_CHIP,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT45DB,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT45DB, /* Impossible if locked down or #WP is low */
		/* granularity will be set by the probing function. */
		.write		= SPI_WRITE_AT45DB,
		.read		= SPI_READ_AT45DB, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600}, /* 2.5-3.6V & 2.7-3.6V models available */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB321C",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB321C,
		.total_size	= 4224, /* No power of two sizes */
		.page_size	= 528, /* No power of two sizes */
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77 (4 dummy bytes); write 0x9A (via buffer) */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {528, 8192} },
				.block_erase = SPI_ERASE_AT45DB_PAGE,
			}, {
				.eraseblocks = { {8 * 528, 8192/8} },
				.block_erase = SPI_ERASE_AT45DB_BLOCK,
			}, /* Although the datasheets describes sectors (which can be write protected)
			    * there seems to be no erase functions for them.
			{
				.eraseblocks = {
					{8 * 528, 1},
					{120 * 528, 1},
					{128 * 528, 63},
				},
				.block_erase = SPI_ERASE_AT45DB_SECTOR
			}, */ {
				.eraseblocks = { {4224 * 1024, 1} },
				.block_erase = SPI_ERASE_AT45DB_CHIP,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT45DB, /* Bit 0 is undefined, no lockdown */
		.write		= SPI_WRITE_AT45DB,
		.read		= SPI_READ_AT45DB_E8, /* 3 address and 4 dummy bytes */
		.voltage	= {2700, 3600},
		.gran		= WRITE_GRAN_528BYTES,
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB321D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB321D,
		.total_size	= 4096, /* or 4224, determined from status register */
		.page_size	= 512, /* or 528, determined from status register */
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_AT45DB,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 8192} },
				.block_erase = SPI_ERASE_AT45DB_PAGE,
			}, {
				.eraseblocks = { {8 * 512, 8192/8} },
				.block_erase = SPI_ERASE_AT45DB_BLOCK,
			}, {
				.eraseblocks = {
					{8 * 512, 1},
					{120 * 512, 1},
					{128 * 512, 63},
				},
				.block_erase = SPI_ERASE_AT45DB_SECTOR
			}, {
				.eraseblocks = { {4096 * 1024, 1} },
				.block_erase = SPI_ERASE_AT45DB_CHIP,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT45DB,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT45DB, /* Impossible if locked down or #WP is low */
		/* granularity will be set by the probing function. */
		.write		= SPI_WRITE_AT45DB,
		.read		= SPI_READ_AT45DB, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600}, /* 2.5-3.6V & 2.7-3.6V models available */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB321E",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB321C,
		.total_size	= 4096, /* or 4224, determined from status register */
		.page_size	= 512, /* or 528, determined from status register */
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_SPI_AT45DB,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 8192} },
				.block_erase = SPI_ERASE_AT45DB_PAGE,
			}, {
				.eraseblocks = { {8 * 512, 8192/8} },
				.block_erase = SPI_ERASE_AT45DB_BLOCK,
			}, {
				.eraseblocks = {
					{8 * 512, 1},
					{120 * 512, 1},
					{128 * 512, 63},
				},
				.block_erase = SPI_ERASE_AT45DB_SECTOR
			}, {
				.eraseblocks = { {4096 * 1024, 1} },
				.block_erase = SPI_ERASE_AT45DB_CHIP,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT45DB, /* has a 2nd status register */
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT45DB, /* Impossible if locked down or #WP is low */
		/* granularity will be set by the probing function. */
		.write		= SPI_WRITE_AT45DB,
		.read		= SPI_READ_AT45DB, /* Fast read (0x0B) supported */
		.voltage	= {2500, 3600}, /* 2.3-3.6V & 2.5-3.6V models available */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB642D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB642D,
		.total_size	= 8192, /* or 8448, determined from status register */
		.page_size	= 1024, /* or 1056, determined from status register */
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_SPI_AT45DB,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024, 8192} },
				.block_erase = SPI_ERASE_AT45DB_PAGE,
			}, {
				.eraseblocks = { {8 * 1024, 8192/8} },
				.block_erase = SPI_ERASE_AT45DB_BLOCK,
			}, {
				.eraseblocks = {
					{8 * 1024, 1},
					{248 * 1024, 1},
					{256 * 1024, 31},
				},
				.block_erase = SPI_ERASE_AT45DB_SECTOR
			}, {
				.eraseblocks = { {8192 * 1024, 1} },
				.block_erase = SPI_ERASE_AT45DB_CHIP,
			}
		},
		.printlock	= SPI_PRETTYPRINT_STATUS_REGISTER_AT45DB,
		.unlock		= SPI_DISABLE_BLOCKPROTECT_AT45DB, /* Impossible if locked down or #WP is low */
		/* granularity will be set by the probing function. */
		.write		= SPI_WRITE_AT45DB,
		.read		= SPI_READ_AT45DB, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT49(H)F010",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT49F010,
		.total_size	= 128,
		.page_size	= 0, /* unused */
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.printlock	= PRINTLOCK_AT49F,
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT49BV512",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT49BV512,
		.total_size	= 64,
		.page_size	= 64,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT49F002(N)",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT49F002N,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{96 * 1024, 1},
					{128 * 1024, 1},
				},
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT49F002(N)T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT49F002NT,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PR,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{128 * 1024, 1},
					{96 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = JEDEC_SECTOR_ERASE,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
		},
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT49F020",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT49F020,
		.total_size	= 256,
		.page_size	= 0, /* unused */
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PRE,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
			/* Chip features an optional permanent write protection
			 * of the first 8 kB. The erase function is the same as
			 * above, but 00000H to 01FFFH will not be erased.
			 * FIXME: add another eraser when partial erasers are
			 * supported.
			 */
		},
		.printlock	= PRINTLOCK_AT49F,
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT49F040",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT49F040,
		.total_size	= 512,
		.page_size	= 0, /* unused */
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,  /* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
			/* Chip features an optional permanent write protection
			 * of the first 16 kB. The erase function is the same as
			 * above, but 00000H to 03FFFH will not be erased.
			 * FIXME: add another eraser when partial erasers are
			 * supported.
			 */
		},
		.printlock	= PRINTLOCK_AT49F,
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT49F080",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT49F080,
		.total_size	= 1024,
		.page_size	= 0, /* unused */
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,  /* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
			/* Chip features an optional permanent write protection
			 * of the first 16 kB. The erase function is the same as
			 * above, but 00000H to 03FFFH will not be erased.
			 * FIXME: add another eraser when partial erasers are
			 * supported.
			 */
		},
		.printlock	= PRINTLOCK_AT49F,
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		/* 'top' version of AT49F080. equal in all aspects but the boot block address */
		.vendor		= "Atmel",
		.name		= "AT49F080T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT49F080T,
		.total_size	= 1024,
		.page_size	= 0, /* unused */
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_JEDEC,
		.probe_timing	= TIMING_ZERO,  /* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = JEDEC_CHIP_BLOCK_ERASE,
			}
			/* Chip features an optional permanent write protection
			 * of the first 16 kB. The erase function is the same as
			 * above, but FC000H to FFFFFH will not be erased.
			 * FIXME: add another eraser when partial erasers are
			 * supported.
			 */
		},
		.printlock	= PRINTLOCK_AT49F,
		.write		= WRITE_JEDEC1,
		.read		= READ_MEMMAPPED,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT49LH002",
		.bustype	= BUS_LPC | BUS_FWH, /* A/A Mux */
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT49LH002,
		.total_size	= 256,
		.page_size	= 0, /* unused */
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
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
				.block_erase = NO_BLOCK_ERASE_FUNC, /* TODO: Implement. */
			}, {
				.eraseblocks = {
					{64 * 1024, 4},
				},
				.block_erase = ERASE_BLOCK_82802AB,
			},
		},
		.printlock	= PRINTLOCK_REGSPACE2_BLOCK_ERASER_0,
		.unlock		= UNLOCK_REGSPACE2_BLOCK_ERASER_0,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT49LH004",
		.bustype	= BUS_LPC | BUS_FWH, /* A/A Mux */
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT49LH004,
		.total_size	= 512,
		.page_size	= 0, /* unused */
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 7},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = ERASE_BLOCK_82802AB,
			}, {
				.eraseblocks = {
					{64 * 1024, 8},
				},
				.block_erase = NO_BLOCK_ERASE_FUNC, /* TODO: Implement. */
			},
		},
		.printlock	= PRINTLOCK_REGSPACE2_BLOCK_ERASER_0,
		.unlock		= UNLOCK_REGSPACE2_BLOCK_ERASER_0,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT49LH00B4",
		.bustype	= BUS_LPC | BUS_FWH, /* A/A Mux */
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT49LH00B4,
		.total_size	= 512,
		.page_size	= 0, /* unused */
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= PROBE_AT82802AB,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 2},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 7},
				},
				.block_erase = NO_BLOCK_ERASE_FUNC, /* TODO: Implement. */
			}, {
				.eraseblocks = {
					{64 * 1024, 8},
				},
				.block_erase = ERASE_BLOCK_82802AB,
			},
		},
		.printlock	= PRINTLOCK_REGSPACE2_BLOCK_ERASER_0,
		.unlock		= UNLOCK_REGSPACE2_BLOCK_ERASER_0,
		.write		= WRITE_82802AB,
		.read		= READ_MEMMAPPED,
		.voltage	= {3000, 3600},
	},
