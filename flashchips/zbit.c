/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2026 Stump Creek Technical Services LLC <mstoops@stumpcreektechnicalservices.com>
 */

	{
        /*
         * https://semic-boutique.com/wp-content/uploads/2016/05/ZB25VQ16.pdf
         * The example chip was found in a YOGASLEEP ROHM sound machine:
         * https://yogasleep.com/products/rohm
         * Photos, etc. can be found at
         * https://github.com/stump-creek-technical-services-llc/hardware_hacking_YOGASLEEP_ROHM_3RUS1WTBU/blob/main/README.md#zbit-flash-rom
         */
		.vendor		= "Zbit Semiconductor, Inc.",
		.name		= "ZB25VQ16",
		.bustype	= BUS_SPI,
		.manufacture_id	= ZBIT_ID,
		.model_id	= ZBIT_ZB25VQ16,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EWSR | FEATURE_WRSR_WREN,
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
