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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "flash.h"
#include "flashchips.h"
#include "chipdrivers.h"

/**
 * List of supported flash chips.
 *
 * Please keep the list sorted by vendor name and chip family, so that the output of 'flashrom -L' is roughly
 * alphabetically sorted. Within families keep them in order of density.
 */
const struct flashchip flashchips[] = {

	/*
	 * .vendor		= Vendor name
	 * .name		= Chip name
	 * .bustype		= Supported flash bus types (Parallel, LPC...)
	 * .manufacture_id	= Manufacturer chip ID
	 * .model_id		= Model chip ID
	 * .total_size		= Total size in (binary) kbytes
	 * .page_size		= Page or eraseblock(?) size in bytes
	 * .tested		= Test status
	 * .probe		= Probe function
	 * .probe_timing	= Probe function delay
	 * .block_erasers[]	= Array of erase layouts and erase functions
	 * {
	 *	.eraseblocks[]	= Array of { blocksize, blockcount }
	 *	.block_erase	= Block erase function
	 * }
	 * .printlock		= Chip lock status function
	 * .unlock		= Chip unlock function
	 * .write		= Chip write function
	 * .read		= Chip read function
	 * .voltage		= Voltage range in millivolt
	 */

	{
		.vendor		= "AMD",
		.name		= "Am29F010A/B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29F010B,	/* Same as Am29F010A */
		.total_size	= 128,
		.page_size	= 16 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PRE,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {16 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "AMD",
		.name		= "Am29F002(N)BB",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29F002BB,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_SHORT_RESET | FEATURE_ADDR_2AA,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 3},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4750, 5250}, /* 4.75-5.25V for type -55, others 4.5-5.5V */
	},

	{
		.vendor		= "AMD",
		.name		= "Am29F002(N)BT",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29F002BT,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_EITHER_RESET | FEATURE_ADDR_2AA,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
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
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4750, 5250}, /* 4.75-5.25V for type -55, others 4.5-5.5V */
	},

	{
		.vendor		= "AMD",
		.name		= "Am29F016D",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29F016D,
		.total_size	= 2 * 1024,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "AMD",
		.name		= "Am29F040B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29F040B,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "AMD",
		.name		= "Am29F080B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29F080B,
		.total_size	= 1024,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "AMD",
		.name		= "Am29LV001BB",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29LV001BB,
		.total_size	= 128,
		.page_size	= 64 * 1024, /* unused */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 1},
					{4 * 1024, 2},
					{16 * 1024, 7},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* 3.0-3.6V for type -45R, others 2.7-3.6V */
	},

	{
		.vendor		= "AMD",
		.name		= "Am29LV001BT",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29LV001BT,
		.total_size	= 128,
		.page_size	= 64 * 1024, /* unused */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 7},
					{4 * 1024, 2},
					{8 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* 3.0-3.6V for type -45R, others 2.7-3.6V */
	},

	{
		.vendor		= "AMD",
		.name		= "Am29LV002BB",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29LV002BB,
		.total_size	= 256,
		.page_size	= 64 * 1024, /* unused */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 3},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* 3.0-3.6V for type -55, others 2.7-3.6V */
	},

	{
		.vendor		= "AMD",
		.name		= "Am29LV002BT",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29LV002BT,
		.total_size	= 256,
		.page_size	= 64 * 1024, /* unused */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
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
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* 3.0-3.6V for type -55, others 2.7-3.6V */
	},

	{
		.vendor		= "AMD",
		.name		= "Am29LV004BB",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29LV004BB,
		.total_size	= 512,
		.page_size	= 64 * 1024, /* unused */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 7},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMD",
		.name		= "Am29LV004BT",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29LV004BT,
		.total_size	= 512,
		.page_size	= 64 * 1024, /* unused */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
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
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMD",
		.name		= "Am29LV008BB",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29LV008BB,
		.total_size	= 1024,
		.page_size	= 64 * 1024, /* unused */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 15},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600} /* 3.0-3.6V for type -70R, others 2.7-3.6V */
	},

	{
		.vendor		= "AMD",
		.name		= "Am29LV008BT",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29LV008BT,
		.total_size	= 1024,
		.page_size	= 64 * 1024, /* unused */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 15},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600} /* 3.0-3.6V for type -70R, others 2.7-3.6V */
	},

	{
		.vendor		= "AMD",
		.name		= "Am29LV040B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29LV040B,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_OK_PRE,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* 3.0-3.6V for type -60R, others 2.7-3.6V*/
	},

	{
		.vendor		= "AMD",
		.name		= "Am29LV081B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID,
		.model_id	= AMD_AM29LV080B,
		.total_size	= 1024,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET, /* datasheet specifies address as don't care */
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* 3.0-3.6V for type -70R, others 2.7-3.6V */
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L05PT",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L05PT,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L05PU",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L05PU,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L10PT",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L10PT,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 1},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L10PU",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L10PU,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 1},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L20PT",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L20PT,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 3},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L20PU",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L20PU,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 3},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	/* The A25L40P{T,U} chips are distinguished by their
	 * erase block layouts, but without any distinction in RDID.
	 * This inexplicable quirk was verified by Rudolf Marek
	 * and discussed on the flashrom mailing list on 2010-07-12.
	 */
	{
		.vendor		= "AMIC",
		.name		= "A25L40PT",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L40PT,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PR,
		.probe		= probe_spi_rdid4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 7},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L40PU",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L40PU,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PR,
		.probe		= probe_spi_rdid4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 7},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L80P",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L80P,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PRE,
		.probe		= probe_spi_rdid4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 15},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L16PT",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L16PT,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 31},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L16PU",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= AMIC_A25L16PU,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PR,
		.probe		= probe_spi_rdid4,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 31},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L512",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L512,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 16 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { { 64 * 1024, 1 } },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 64 * 1024, 1 } },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L010",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L010,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 32 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { { 64 * 1024, 2 } },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 128 * 1024, 1 } },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L020",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L020,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 64 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { { 64 * 1024, 4 } },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 256 * 1024, 1 } },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L040",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L040,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 128 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { { 64 * 1024, 8 } },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 512 * 1024, 1 } },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L080",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L080,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 256 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { { 64 * 1024, 16 } },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 1024 * 1024, 1 } },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L016",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L016,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 512 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { { 64 * 1024, 32 } },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 2048 * 1024, 1 } },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25L032",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25L032,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 64B total; read 0x4B, 0x48; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 1024 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { { 64 * 1024, 64 } },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { { 64 * 1024, 64 } },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 4096 * 1024, 1 } },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 4096 * 1024, 1 } },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_amic_a25l032, /* bit5: T/B, bit6: prot size */
		.unlock		= spi_disable_blockprotect_bp2_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25LQ16",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25LQ16,
		.total_size	= 2048,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, 0x48; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { { 4 * 1024, 512 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { { 64 * 1024, 32 } },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { { 64 * 1024, 32 } },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 2048 * 1024, 1 } },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 2048 * 1024, 1 } },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_amic_a25l032, /* bit5: T/B, bit6: prot size */
		.unlock		= spi_disable_blockprotect_bp2_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25LQ032/A25LQ32A",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25LQ032,
		.total_size	= 4096,
		.page_size	= 256,
		/* A25LQ32A supports SFDP */
		/* OTP: 64B total; read 0x4B, 0x48; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 1024 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { { 64 * 1024, 64 } },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { { 64 * 1024, 64 } },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 4096 * 1024, 1 } },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 4096 * 1024, 1 } },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_amic_a25l032, /* bit5: T/B, bit6: prot size */
		.unlock		= spi_disable_blockprotect_bp2_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A25LQ64",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A25LQ032,
		.total_size	= 8192,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 512B total; enter 0xB1, exit 0xC1 */
		/* QPI enable 0x35, disable 0xF5 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 2048 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { { 32 * 1024, 256 } },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { { 64 * 1024, 128 } },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 8192 * 1024, 1 } },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 8192 * 1024, 1 } },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enhance (sic!) */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "AMIC",
		.name		= "A29002B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A29002B,
		.total_size	= 256,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 3},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "AMIC",
		.name		= "A29002T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A29002T,
		.total_size	= 256,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
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
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "AMIC",
		.name		= "A29040B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A29040B,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "AMIC",
		.name		= "A49LF040A",
		.bustype	= BUS_LPC,
		.manufacture_id	= AMIC_ID_NOPREFIX,
		.model_id	= AMIC_A49LF040A,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PR,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* routine is wrapper to probe_jedec (pm49fl00x.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.unlock		= unlock_regspace2_uniform_64k,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 8} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25df,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600}, /* 2.3-3.6V & 2.7-3.6V models available */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25df,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25df,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25df_sec,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect_sec,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.tested		= TEST_OK_PROBE,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25df_sec,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect_sec,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 128} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25df,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 128} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25df_sec,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect_sec,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25df_sec,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect_sec,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25DL081",
		.bustype	= BUS_SPI,
		.manufacture_id = ATMEL_ID,
		.model_id	= ATMEL_AT25DF081,
		.total_size	= 1024,
		.page_size	= 256,
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25df_sec,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect_sec,
		.write		= spi_chip_write_256, /* Dual I/O  (0xA2) supported */
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O  (0x3B) supported */
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT25DL161",
		.bustype	= BUS_SPI,
		.manufacture_id = ATMEL_ID,
		.model_id	= ATMEL_AT25DL161,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25df_sec,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect_sec,
		.write		= spi_chip_write_256, /* Dual I/O  (0xA2) supported */
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O  (0x3B) supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25df_sec,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect_sec,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_at25f,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_62,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25f,
		.unlock		= spi_disable_blockprotect_at25f,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_at25f,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_62,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25f512a,
		/* FIXME: It is not correct to use this one, because the BP1 bit is N/A. */
		.unlock		= spi_disable_blockprotect_at25f512a,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_62,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25f512b,
		.unlock		= spi_disable_blockprotect_at25f512b,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_at25f,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_62,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25f,
		.unlock		= spi_disable_blockprotect_at25f,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_at25f,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_62,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25f,
		.unlock		= spi_disable_blockprotect_at25f,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_at25f,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_62,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25f4096,
		/* "Bits 5-6 are 0s when device is not in an internal write cycle." Better leave them alone: */
		.unlock		= spi_disable_blockprotect_bp2_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25fs010,
		.unlock		= spi_disable_blockprotect_at25fs010,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25fs040,
		.unlock		= spi_disable_blockprotect_at25fs040,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256, 2048} },
				.block_erase = spi_block_erase_81,
			}, {
				.eraseblocks = { {2 * 1024, 256} },
				.block_erase = spi_block_erase_50,
			}, {
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain,
		/* Supports also an incompatible page write (of exactly 256 B) and an auto-erasing write. */
		.write		= spi_chip_write_1,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at26df081a,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at25df,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_at26df081a,
		.unlock		= spi_disable_blockprotect_at2x_global_unprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	/*The AT26DF321 has the same ID as the AT25DF321. */
	/*{
		.vendor		= "Atmel",
		.name		= "AT26DF321",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT26DF321,
		.total_size	= 4096,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.printlock	= spi_prettyprint_status_register_at26df081a,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
	  },*/

	{
		.vendor		= "Atmel",
		.name		= "AT26F004",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT26F004,
		.total_size	= 512,
		.page_size	= 256,
		.tested		= {.probe = NT, .read = NT, .erase = NT, .write = BAD },
		.feature_bits	= FEATURE_WRSR_WREN,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.write		= NULL /* Incompatible Page write */,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
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
		.probe		= probe_jedec,
		.probe_timing	= 10000, /* 10mS, Enter=Exec */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
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
		.tested		= TEST_OK_PRE,
		.probe		= probe_jedec,
		.probe_timing	= 10000, /* 10mS, Enter=Exec */
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec,	/* FIXME */
		.read		= read_memmapped,
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
		.probe		= probe_jedec,
		.probe_timing	= 10000,			/* 10ms */
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec,
		.read		= read_memmapped,
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
		.probe		= probe_jedec,
		.probe_timing	= 10000,			/* 10 ms */
		.block_erasers	=
		{
			{
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45CS1282",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45CS1282,
		.total_size	= 16896 /* No power of two sizes */,
		.page_size	= 1056 /* No power of two sizes */,
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77 (4 dummy bytes); write 0x9A (via buffer) */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1056, 1},    /* sector 0a:      opcode 50h */
					{248 * 1056, 1},  /* sector 0b:      opcode 7Ch */
					{256 * 1056, 63}, /* sectors 1 - 63: opcode 7Ch */
				},
				.block_erase = spi_erase_at45cs_sector,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain,
		.gran		= write_gran_1056bytes,
		.write		= spi_write_at45db,
		.read		= spi_read_at45db,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB011D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB011D,
		.total_size	= 128 /* or 132, determined from status register */,
		.page_size	= 256 /* or 264, determined from status register */,
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_at45db,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256, 512} },
				.block_erase = spi_erase_at45db_page,
			}, {
				.eraseblocks = { {8 * 256, 512/8} },
				.block_erase = spi_erase_at45db_block,
			}, {
				.eraseblocks = {
					{8 * 256, 1},
					{120 * 256, 1},
					{128 * 256, 3},
				},
				.block_erase = spi_erase_at45db_sector
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_erase_at45db_chip,
			}
		},
		.unlock		= spi_disable_blockprotect_at45db, /* Impossible if locked down or #WP is low */
		.printlock	= spi_prettyprint_status_register_at45db,
		/* granularity will be set by the probing function. */
		.write		= spi_write_at45db,
		.read		= spi_read_at45db, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB021D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB021D,
		.total_size	= 256 /* or 264, determined from status register */,
		.page_size	= 256 /* or 264, determined from status register */,
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_at45db,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256, 1024} },
				.block_erase = spi_erase_at45db_page,
			}, {
				.eraseblocks = { {8 * 256, 1024/8} },
				.block_erase = spi_erase_at45db_block,
			}, {
				.eraseblocks = {
					{8 * 256, 1},
					{120 * 256, 1},
					{128 * 256, 7},
				},
				.block_erase = spi_erase_at45db_sector
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_erase_at45db_chip,
			}
		},
		.unlock		= spi_disable_blockprotect_at45db, /* Impossible if locked down or #WP is low */
		.printlock	= spi_prettyprint_status_register_at45db,
		/* granularity will be set by the probing function. */
		.write		= spi_write_at45db,
		.read		= spi_read_at45db, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB041D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB041D,
		.total_size	= 512 /* or 528, determined from status register */,
		.page_size	= 256 /* or 264, determined from status register */,
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_at45db,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256, 2048} },
				.block_erase = spi_erase_at45db_page,
			}, {
				.eraseblocks = { {8 * 256, 2048/8} },
				.block_erase = spi_erase_at45db_block,
			}, {
				.eraseblocks = {
					{8 * 256, 1},
					{248 * 256, 1},
					{256 * 256, 7},
				},
				.block_erase = spi_erase_at45db_sector
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_erase_at45db_chip,
			}
		},
		.unlock		= spi_disable_blockprotect_at45db, /* Impossible if locked down or #WP is low */
		.printlock	= spi_prettyprint_status_register_at45db,
		/* granularity will be set by the probing function. */
		.write		= spi_write_at45db,
		.read		= spi_read_at45db, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600}, /* 2.5-3.6V & 2.7-3.6V models available */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB081D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB081D,
		.total_size	= 1024 /* or 1056, determined from status register */,
		.page_size	= 256 /* or 264, determined from status register */,
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_at45db,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256, 4096} },
				.block_erase = spi_erase_at45db_page,
			}, {
				.eraseblocks = { {8 * 256, 4096/8} },
				.block_erase = spi_erase_at45db_block,
			}, {
				.eraseblocks = {
					{8 * 256, 1},
					{248 * 256, 1},
					{256 * 256, 15},
				},
				.block_erase = spi_erase_at45db_sector
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_erase_at45db_chip,
			}
		},
		.unlock		= spi_disable_blockprotect_at45db, /* Impossible if locked down or #WP is low */
		.printlock	= spi_prettyprint_status_register_at45db,
		/* granularity will be set by the probing function. */
		.write		= spi_write_at45db,
		.read		= spi_read_at45db, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600}, /* 2.5-3.6V & 2.7-3.6V models available */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB161D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB161D,
		.total_size	= 2048 /* or 2112, determined from status register */,
		.page_size	= 512 /* or 528, determined from status register */,
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_at45db,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 4096} },
				.block_erase = spi_erase_at45db_page,
			}, {
				.eraseblocks = { {8 * 512, 4096/8} },
				.block_erase = spi_erase_at45db_block,
			}, {
				.eraseblocks = {
					{8 * 512, 1},
					{248 * 512, 1},
					{256 * 512, 15},
				},
				.block_erase = spi_erase_at45db_sector
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = spi_erase_at45db_chip,
			}
		},
		.unlock		= spi_disable_blockprotect_at45db, /* Impossible if locked down or #WP is low */
		.printlock	= spi_prettyprint_status_register_at45db,
		/* granularity will be set by the probing function. */
		.write		= spi_write_at45db,
		.read		= spi_read_at45db, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600}, /* 2.5-3.6V & 2.7-3.6V models available */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB321C",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB321C,
		.total_size	= 4224 /* No power of two sizes */,
		.page_size	= 528 /* No power of two sizes */,
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77 (4 dummy bytes); write 0x9A (via buffer) */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {528, 8192} },
				.block_erase = spi_erase_at45db_page,
			}, {
				.eraseblocks = { {8 * 528, 8192/8} },
				.block_erase = spi_erase_at45db_block,
			}, /* Although the datasheets describes sectors (which can be write protected)
			    * there seems to be no erase functions for them.
			  {
				.eraseblocks = {
					{8 * 528, 1},
					{120 * 528, 1},
					{128 * 528, 63},
				},
				.block_erase = spi_erase_at45db_sector
			}, */ {
				.eraseblocks = { {4224 * 1024, 1} },
				.block_erase = spi_erase_at45db_chip,
			}
		},
		.printlock	= spi_prettyprint_status_register_at45db, /* Bit 0 is undefined, no lockdown */
		.gran		= write_gran_528bytes,
		.write		= spi_write_at45db,
		.read		= spi_read_at45db_e8, /* 3 address and 4 dummy bytes */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB321D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB321D,
		.total_size	= 4096 /* or 4224, determined from status register */,
		.page_size	= 512 /* or 528, determined from status register */,
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_at45db,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 8192} },
				.block_erase = spi_erase_at45db_page,
			}, {
				.eraseblocks = { {8 * 512, 8192/8} },
				.block_erase = spi_erase_at45db_block,
			}, {
				.eraseblocks = {
					{8 * 512, 1},
					{120 * 512, 1},
					{128 * 512, 63},
				},
				.block_erase = spi_erase_at45db_sector
			}, {
				.eraseblocks = { {4096 * 1024, 1} },
				.block_erase = spi_erase_at45db_chip,
			}
		},
		.unlock		= spi_disable_blockprotect_at45db, /* Impossible if locked down or #WP is low */
		.printlock	= spi_prettyprint_status_register_at45db,
		/* granularity will be set by the probing function. */
		.write		= spi_write_at45db,
		.read		= spi_read_at45db, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600}, /* 2.5-3.6V & 2.7-3.6V models available */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB321E",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB321C,
		.total_size	= 4096 /* or 4224, determined from status register */,
		.page_size	= 512 /* or 528, determined from status register */,
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_at45db,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 8192} },
				.block_erase = spi_erase_at45db_page,
			}, {
				.eraseblocks = { {8 * 512, 8192/8} },
				.block_erase = spi_erase_at45db_block,
			}, {
				.eraseblocks = {
					{8 * 512, 1},
					{120 * 512, 1},
					{128 * 512, 63},
				},
				.block_erase = spi_erase_at45db_sector
			}, {
				.eraseblocks = { {4096 * 1024, 1} },
				.block_erase = spi_erase_at45db_chip,
			}
		},
		.unlock		= spi_disable_blockprotect_at45db, /* Impossible if locked down or #WP is low */
		.printlock	= spi_prettyprint_status_register_at45db, /* has a 2nd status register */
		/* granularity will be set by the probing function. */
		.write		= spi_write_at45db,
		.read		= spi_read_at45db, /* Fast read (0x0B) supported */
		.voltage	= {2500, 3600}, /* 2.3-3.6V & 2.5-3.6V models available */
	},

	{
		.vendor		= "Atmel",
		.name		= "AT45DB642D",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= ATMEL_AT45DB642D,
		.total_size	= 8192 /* or 8448, determined from status register */,
		.page_size	= 1024 /* or 1056, determined from status register */,
		/* does not support EWSR nor WREN and has no writable status register bits whatsoever */
		/* OTP: 128B total, 64B pre-programmed; read 0x77; write 0x9B */
		.feature_bits	= FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_at45db,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024, 8192} },
				.block_erase = spi_erase_at45db_page,
			}, {
				.eraseblocks = { {8 * 1024, 8192/8} },
				.block_erase = spi_erase_at45db_block,
			}, {
				.eraseblocks = {
					{8 * 1024, 1},
					{248 * 1024, 1},
					{256 * 1024, 31},
				},
				.block_erase = spi_erase_at45db_sector
			}, {
				.eraseblocks = { {8192 * 1024, 1} },
				.block_erase = spi_erase_at45db_chip,
			}
		},
		.unlock		= spi_disable_blockprotect_at45db, /* Impossible if locked down or #WP is low */
		.printlock	= spi_prettyprint_status_register_at45db,
		/* granularity will be set by the probing function. */
		.write		= spi_write_at45db,
		.read		= spi_read_at45db, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
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
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
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
		.probe		= probe_jedec,
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
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
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
		.probe		= probe_jedec,
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
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
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
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_at49f,
 		.write		= write_jedec_1,
		.read		= read_memmapped,
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
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
			/* Chip features an optional permanent write protection
			 * of the first 8 kB. The erase function is the same as
			 * above, but 00000H to 01FFFH will not be erased.
			 * FIXME: add another eraser when partial erasers are
			 * supported.
			 */
		},
		.printlock	= printlock_at49f,
		.write		= write_jedec_1,
		.read		= read_memmapped,
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
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,  /* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
			/* Chip features an optional permanent write protection
			 * of the first 16 kB. The erase function is the same as
			 * above, but 00000H to 03FFFH will not be erased.
			 * FIXME: add another eraser when partial erasers are
			 * supported.
			 */
		},
		.printlock	= printlock_at49f,
		.write		= write_jedec_1,
		.read		= read_memmapped,
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
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,  /* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
			/* Chip features an optional permanent write protection
			 * of the first 16 kB. The erase function is the same as
			 * above, but 00000H to 03FFFH will not be erased.
			 * FIXME: add another eraser when partial erasers are
			 * supported.
			 */
		},
		.printlock	= printlock_at49f,
		.write		= write_jedec_1,
		.read		= read_memmapped,
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
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,  /* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
			/* Chip features an optional permanent write protection
			 * of the first 16 kB. The erase function is the same as
			 * above, but FC000H to FFFFFH will not be erased.
			 * FIXME: add another eraser when partial erasers are
			 * supported.
			 */
		},
		.printlock	= printlock_at49f,
		.write		= write_jedec_1,
		.read		= read_memmapped,
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
		.probe		= probe_82802ab,
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
				.block_erase = NULL, /* TODO: Implement. */
			}, {
				.eraseblocks = {
					{64 * 1024, 4},
				},
				.block_erase = erase_block_82802ab,
			},
		},
		.printlock	= printlock_regspace2_block_eraser_0,
		.unlock		= unlock_regspace2_block_eraser_0,
		.write		= write_82802ab,
		.read		= read_memmapped,
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
		.probe		= probe_82802ab,
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
				.block_erase = NULL, /* TODO: Implement. */
			}, {
				.eraseblocks = {
					{64 * 1024, 8},
				},
				.block_erase = erase_block_82802ab,
			},
		},
		.printlock	= printlock_regspace2_block_eraser_0,
		.unlock		= unlock_regspace2_block_eraser_0,
		.write		= write_82802ab,
		.read		= read_memmapped,
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
		.probe		= probe_82802ab,
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
				.block_erase = erase_block_82802ab,
			}, {
				.eraseblocks = {
					{64 * 1024, 8},
				},
				.block_erase = NULL, /* TODO: Implement. */
			},
		},
		.printlock	= printlock_regspace2_block_eraser_0,
		.unlock		= unlock_regspace2_block_eraser_0,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Catalyst",
		.name		= "CAT28F512",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= CATALYST_ID,
		.model_id	= CATALYST_CAT28F512,
		.total_size	= 64,
		.page_size	= 0, /* unused */
		.feature_bits	= 0,
		.tested		= {.probe = OK, .read = OK, .erase = BAD, .write = BAD },
		.probe		= probe_jedec, /* FIXME! */
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = NULL, /* TODO */
			},
		},
		.write		= NULL, /* TODO */
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Bright",
		.name		= "BM29F040",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= BRIGHT_ID,
		.model_id	= BRIGHT_BM29F040,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PR,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "ESMT",
		.name		= "F49B002UA",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ESMT_ID,
		.model_id	= ESMT_F49B002UA,
		.total_size	= 256,
		.page_size	= 4096,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
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
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "ESMT",
		.name		= "F25L008A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ESMT_ID,
		.model_id	= ESMT_F25L008A,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EITHER,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_1,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "ESMT",
		.name		= "F25L32PA",
		.bustype	= BUS_SPI,
		.manufacture_id	= ESMT_ID,
		.model_id	= ESMT_F25L32PA,
		.total_size	= 4096,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EITHER | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_bpl,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B05",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B05,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B05T",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B05,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25P05",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B05,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{32 * 1024, 2} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B10",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B10,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 3},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B10T",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B10,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{32 * 1024, 3},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25P10",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B10,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B20",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B20,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 3}
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B20T",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B20,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 3},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25P20",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B20,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B40",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B40,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 7}
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B40T",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B40,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 7},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25P40",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B40,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B80",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B80,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 15}
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B80T",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B80,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 15},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25P80",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B80,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B16",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B16,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 31},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B16T",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B16,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 31},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25P16",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B16,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B32",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B32,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 512B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 63},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B32T",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B32,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 512B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 63},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25P32", /* Uniform version of EN25B32 */
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B32,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 512B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B64",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B64,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 512B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 2},
					{8 * 1024, 1},
					{16 * 1024, 1},
					{32 * 1024, 1},
					{64 * 1024, 127},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25B64T",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B64,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 512B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 127},
					{32 * 1024, 1},
					{16 * 1024, 1},
					{8 * 1024, 1},
					{4 * 1024, 2},
				},
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25P64",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25B64,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 512B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25F05",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25F05,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25F10",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25F10,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25F20",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25F20,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25F40",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25F40,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25F80",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25F80,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25F16",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25F16,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25F32",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25F32,
		.total_size	= 4096,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25F64",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25F64,
		.total_size	= 8192,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25Q40",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25Q40,
		.total_size	= 512,
		.page_size	= 256,
		/* OTP: 256B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25Q80(A)",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25Q80,
		.total_size	= 1024,
		.page_size	= 256,
		/* OTP: 256B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		/* Note: EN25D16 is an evil twin which shares the model ID
		   but has different write protection capabilities */
		.vendor		= "Eon",
		.name		= "EN25Q16",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25Q16,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: D16 512B/Q16 128B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				/* not supported by Q16 version */
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25Q32(A/B)",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25Q32,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 512B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25Q64",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25Q64,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 512B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25Q128",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25Q128,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 512B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
	},

	{
		.vendor		= "Eon",
		.name		= "EN25QH16",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25QH16,
		.total_size	= 2048,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 512B total; enter 0x3A */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 2048, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 2048, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25QH32",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25QH32,
		.total_size	= 4096,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 512B total; enter 0x3A */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 4096, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 4096, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25QH64",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25QH64,
		.total_size	= 8192,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 512B total; enter 0x3A */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PR,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 8192 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 8192 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25QH128",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25QH128,
		.total_size	= 16384,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 512B total; enter 0x3A */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25S10",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25S10,
		.total_size	= 128,
		.page_size	= 256,
		/* OTP: 256B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_20,
			},  {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25S20",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25S20,
		.total_size	= 256,
		.page_size	= 256,
		/* OTP: 256B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25S40",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25S40,
		.total_size	= 512,
		.page_size	= 256,
		/* OTP: 256B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25S80",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25S80,
		.total_size	= 1024,
		.page_size	= 256,
		/* OTP: 256B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25S16",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25S16,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 512B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_en25s_wp,
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25S32",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25S32,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 512B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 128} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4096 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4096 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_en25s_wp,
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "Eon",
		.name		= "EN25S64",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= EON_EN25S64,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 512B total; enter 0x3A */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8192 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8192 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_en25s_wp,
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "Eon",
		.name		= "EN29F010",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= EON_ID,
		.model_id	= EON_EN29F010,
		.total_size	= 128,
		.page_size	= 128,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PRE,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {16 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			},
			{
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Eon",
		.name		= "EN29F002(A)(N)B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= EON_ID,
		.model_id	= EON_EN29F002B,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_ADDR_AAA | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PR,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 3},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Eon",
		.name		= "EN29F002(A)(N)T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= EON_ID,
		.model_id	= EON_EN29F002T,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_ADDR_AAA | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 3},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Eon",
		.name		= "EN29LV040(A)",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= EON_ID,
		.model_id	= EON_EN29LV040,
		.total_size	= 512,
		.page_size	= 4 * 1024,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			},
			{
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* 3.0-3.6V for type -45R and 55R, others 2.7-3.6V */
	},

	{
		.vendor		= "Eon",
		.name		= "EN29LV640B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= EON_ID,
		.model_id	= EON_EN29LV640B,
		.total_size	= 8192,
		.page_size	= 8192,
		.feature_bits	= FEATURE_ADDR_SHIFTED,
		.tested		= TEST_OK_PREW,
		.probe		= probe_en29lv640b,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 127},
				},
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_en29lv640b,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN29GL064(A)B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= EON_ID,
		.model_id	= EON_EN29GL064B,
		.total_size	= 8192,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 127},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN29GL064(A)T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= EON_ID,
		.model_id	= EON_EN29GL064T,
		.total_size	= 8192,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 127},
					{8 * 1024, 8},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN29GL064H/L",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= EON_ID,
		.model_id	= EON_EN29GL064HL,
		.total_size	= 8192,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Eon",
		.name		= "EN29GL128",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= EON_ID,
		.model_id	= EON_EN29GL128HL,
		.total_size	= 16384,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Fujitsu",
		.name		= "MBM29F004BC",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= FUJITSU_ID,
		.model_id	= FUJITSU_MBM29F004BC,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 7},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= NULL,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Fujitsu",
		.name		= "MBM29F004TC",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= FUJITSU_ID,
		.model_id	= FUJITSU_MBM29F004TC,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 7},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= NULL,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		/* FIXME: this has WORD/BYTE sequences; 2AA for word, 555 for byte */
		.vendor		= "Fujitsu",
		.name		= "MBM29F400BC",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= FUJITSU_ID,
		.model_id	= FUJITSU_MBM29F400BC,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_SHIFTED | FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= 10, // FIXME: check datasheet. Using the 10 us from probe_m29f400bt
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 7},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4750, 5250}, /* 4.75-5.25V for type -55, others 4.5-5.5V */
	},

	{
		.vendor		= "Fujitsu",
		.name		= "MBM29F400TC",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= FUJITSU_ID,
		.model_id	= FUJITSU_MBM29F400TC,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_SHIFTED | FEATURE_ADDR_AAA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= 10, // FIXME: check datasheet. Using the 10 us from probe_m29f400bt
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 7},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4750, 5250}, /* 4.75-5.25V for type -55, others 4.5-5.5V */
	},

	{
		.vendor		= "Fujitsu",
		.name		= "MBM29LV160BE",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= FUJITSU_ID,
		.model_id	= FUJITSU_MBM29LV160BE,
		.total_size	= 2 * 1024,
		.page_size	= 0,
		.feature_bits	= FEATURE_ADDR_SHIFTED | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= 10, // FIXME: check datasheet. Using the 10 us from probe_m29f400bt
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 31},
				},
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1, /* Supports a fast mode too */
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* 3.0-3.6V for type -70, others 2.7-3.6V */
	},

	{
		.vendor		= "Fujitsu",
		.name		= "MBM29LV160TE",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= FUJITSU_ID,
		.model_id	= FUJITSU_MBM29LV160TE,
		.total_size	= 2 * 1024,
		.page_size	= 0,
		.feature_bits	= FEATURE_ADDR_SHIFTED | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= 10, // FIXME: check datasheet. Using the 10 us from probe_m29f400bt
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 31},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1, /* Supports a fast mode too */
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* 3.0-3.6V for type -70, others 2.7-3.6V */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1695, 1950},
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1695, 1950},
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 128} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
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
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1695, 1950},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25LQ128",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25LQ128,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 512} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1695, 1950},
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
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 2} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 8} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
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
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
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
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 128} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
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
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "GigaDevice",
		.name		= "GD25Q128B",
		.bustype	= BUS_SPI,
		.manufacture_id	= GIGADEVICE_ID,
		.model_id	= GIGADEVICE_GD25Q128,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 512} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp4_srwd,
		.unlock		= spi_disable_blockprotect_bp4_srwd, /* TODO: 2nd status reg (read with 0x35) */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Hyundai",
		.name		= "HY29F002T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= HYUNDAI_ID,
		.model_id	= HYUNDAI_HY29F002T,
		.total_size	= 256,
		.page_size	= 256 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET, /* Some revisions may need FEATURE_ADDR_2AA */
		.tested		= TEST_OK_PRE,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO, /* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 3},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4750, 5250}, /* 4.75-5.25V for type -45, others 4.5-5.5V */
	},

	{
		.vendor		= "Hyundai",
		.name		= "HY29F002B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= HYUNDAI_ID,
		.model_id	= HYUNDAI_HY29F002B,
		.total_size	= 256,
		.page_size	= 256 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET, /* Some revisions may need FEATURE_ADDR_2AA */
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO, /* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 3},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4750, 5250}, /* 4.75-5.25V for type -45, others 4.5-5.5V */
	},

	{
		.vendor		= "Hyundai",
		.name		= "HY29F040A",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= HYUNDAI_ID,
		.model_id	= HYUNDAI_HY29F040A,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Intel",
		.name		= "25F160S33B8",
		.bustype	= BUS_SPI,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_25F160S33B8,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 506B total (2x 8B, 30x 16B, 1x 10B); read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 8 so-called "parameter blocks" with
				 * opcode 0x40. Trying to access an address outside these 8 8kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 31} // inaccessible
				},
				.block_erase = spi_block_erase_40,
			}, { */
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_ep_srwd,
		.unlock		= spi_disable_blockprotect_bp2_ep_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,	/* also fast read 0x0B */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "25F160S33T8",
		.bustype	= BUS_SPI,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_25F160S33T8,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 506B total (2x 8B, 30x 16B, 1x 10B); read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 8 so-called "parameter blocks" with
				 * opcode 0x40. Trying to access an address outside these 8 8kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{64 * 1024, 31}, // inaccessible
					{8 * 1024, 8}
				},
				.block_erase = spi_block_erase_40,
			}, { */
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_ep_srwd,
		.unlock		= spi_disable_blockprotect_bp2_ep_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,	/* also fast read 0x0B */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "25F320S33B8",
		.bustype	= BUS_SPI,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_25F320S33B8,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 506B total (2x 8B, 30x 16B, 1x 10B); read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 8 so-called "parameter blocks" with
				 * opcode 0x40. Trying to access an address outside these 8 8kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 63} // inaccessible
				},
				.block_erase = spi_block_erase_40,
			}, { */
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_ep_srwd,
		.unlock		= spi_disable_blockprotect_bp2_ep_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,	/* also fast read 0x0B */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "25F320S33T8",
		.bustype	= BUS_SPI,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_25F320S33T8,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 506B total (2x 8B, 30x 16B, 1x 10B); read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 8 so-called "parameter blocks" with
				 * opcode 0x40. Trying to access an address outside these 8 8kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{64 * 1024, 63}, // inaccessible
					{8 * 1024, 8}
				},
				.block_erase = spi_block_erase_40,
			}, { */
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_ep_srwd,
		.unlock		= spi_disable_blockprotect_bp2_ep_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,	/* also fast read 0x0B */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "25F640S33B8",
		.bustype	= BUS_SPI,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_25F640S33B8,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 506B total (2x 8B, 30x 16B, 1x 10B); read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 8 so-called "parameter blocks" with
				 * opcode 0x40. Trying to access an address outside these 8 8kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 127} // inaccessible
				},
				.block_erase = spi_block_erase_40,
			}, { */
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_ep_srwd,
		.unlock		= spi_disable_blockprotect_bp2_ep_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,	/* also fast read 0x0B */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "25F640S33T8",
		.bustype	= BUS_SPI,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_25F640S33T8,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 506B total (2x 8B, 30x 16B, 1x 10B); read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				/* This chip supports erasing of the 8 so-called "parameter blocks" with
				 * opcode 0x40. Trying to access an address outside these 8 8kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{64 * 1024, 127}, // inaccessible
					{8 * 1024, 8}
				},
				.block_erase = spi_block_erase_40,
			}, { */
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_ep_srwd,
		.unlock		= spi_disable_blockprotect_bp2_ep_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,	/* also fast read 0x0B */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "28F001BN/BX-B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F001B,
		.total_size	= 128,
		.page_size	= 128 * 1024, /* 8k + 2x4k + 112k */
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 1},
					{4 * 1024, 2},
					{112 * 1024, 1},
				},
				.block_erase = erase_block_82802ab,
			},
		},
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Intel",
		.name		= "28F001BN/BX-T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F001T,
		.total_size	= 128,
		.page_size	= 128 * 1024, /* 112k + 2x4k + 8k */
		.tested		= TEST_OK_PR,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{112 * 1024, 1},
					{4 * 1024, 2},
					{8 * 1024, 1},
				},
				.block_erase = erase_block_82802ab,
			},
		},
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Intel",
		.name		= "28F002BC/BL/BV/BX-T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F002T,
		.total_size	= 256,
		.page_size	= 256 * 1024,
		.tested		= TEST_OK_PRE,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_ZERO, /* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{128 * 1024, 1},
					{96 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_block_82802ab,
			},
		},
		.write		= write_82802ab,
		.read		= read_memmapped,
	},

	{
		.vendor		= "Intel",
		.name		= "28F008S3/S5/SC",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F004S3,
		.total_size	= 512,
		.page_size	= 256,
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_block_82802ab,
			},
		},
		.unlock		= unlock_28f004s5,
		.write		= write_82802ab,
		.read		= read_memmapped,
	},

	{
		.vendor		= "Intel",
		.name		= "28F004B5/BE/BV/BX-B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F004B,
		.total_size	= 512,
		.page_size	= 128 * 1024, /* maximal block size */
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{96 * 1024, 1},
					{128 * 1024, 3},
				},
				.block_erase = erase_block_82802ab,
			},
		},
		.write		= write_82802ab,
		.read		= read_memmapped,
	},

	{
		.vendor		= "Intel",
		.name		= "28F004B5/BE/BV/BX-T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F004T,
		.total_size	= 512,
		.page_size	= 128 * 1024, /* maximal block size */
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{128 * 1024, 3},
					{96 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_block_82802ab,
			},
		},
		.write		= write_82802ab,
		.read		= read_memmapped,
	},

	{
		.vendor		= "Intel",
		.name		= "28F400BV/BX/CE/CV-B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F400B,
		.total_size	= 512,
		.page_size	= 128 * 1024, /* maximal block size */
		.feature_bits	= FEATURE_ADDR_SHIFTED,
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{96 * 1024, 1},
					{128 * 1024, 3},
				},
				.block_erase = erase_block_82802ab,
			},
		},
		.write		= write_82802ab,
		.read		= read_memmapped,
	},

	{
		.vendor		= "Intel",
		.name		= "28F400BV/BX/CE/CV-T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_28F400T,
		.total_size	= 512,
		.page_size	= 128 * 1024, /* maximal block size */
		.feature_bits	= FEATURE_ADDR_SHIFTED,
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{128 * 1024, 3},
					{96 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_block_82802ab,
			},
		},
		.write		= write_82802ab,
		.read		= read_memmapped,
	},

	{
		.vendor		= "Intel",
		.name		= "82802AB",
		.bustype	= BUS_FWH,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_82802AB,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_IGNORED, /* routine does not use probe_timing (82802ab.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_block_82802ab,
			},
		},
		.unlock		= unlock_regspace2_uniform_64k,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Intel",
		.name		= "82802AC",
		.bustype	= BUS_FWH,
		.manufacture_id	= INTEL_ID,
		.model_id	= INTEL_82802AC,
		.total_size	= 1024,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_IGNORED, /* routine does not use probe_timing (82802ab.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = erase_block_82802ab,
			},
		},
		.unlock		= unlock_regspace2_uniform_64k,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
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
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 127},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
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
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 127},
					{8 * 1024, 8},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
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
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
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
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX23L1654",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX23L1654,
		.total_size	= 2048,
		.page_size	= 256,
		.tested		= {.probe = NT, .read = NT, .erase = NA, .write = NA},
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.write		= NULL, /* MX23L1654 is a mask ROM, so it is read-only */
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX23L3254",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX23L3254,
		.total_size	= 4096,
		.page_size	= 256,
		.tested		= {.probe = OK, .read = OK, .erase = NA, .write = NA},
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.write		= NULL, /* MX23L3254 is a mask ROM, so it is read-only */
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX23L6454",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX23L6454,
		.total_size	= 8192,
		.page_size	= 256,
		.tested		= {.probe = NT, .read = NT, .erase = NA, .write = NA},
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.write		= NULL, /* MX23L6454 is a mask ROM, so it is read-only */
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX23L12854",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX23L12854,
		.total_size	= 16384,
		.page_size	= 256,
		.tested		= {.probe = NT, .read = NT, .erase = NA, .write = NA},
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.write		= NULL, /* MX23L12854 is a mask ROM, so it is read-only */
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L512(E)/MX25V512(C)",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L512,
		.total_size	= 64,
		.page_size	= 256,
		/* MX25L512E supports SFDP */
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported, MX25L512E supports dual I/O */
		.voltage	= {2700, 3600}, /* 2.35-3.6V for MX25V512(C) */
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L1005(C)/MX25L1006E",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L1005,
		.total_size	= 128,
		.page_size	= 256,
		/* MX25L1006E supports SFDP */
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 2} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported, MX25L1006E supports dual I/O */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L2005(C)/MX25L2006E",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L2005,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L4005(A/C)/MX25L4006E",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L4005,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L8005/MX25L8006E/MX25L8008E/MX25V8005",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L8005,
		.total_size	= 1024,
		.page_size	= 256,
		/* MX25L8006E, MX25L8008E support SFDP */
		/* OTP: 64B total; enter 0xB1, exit 0xC1 (MX25L8006E, MX25L8008E only) */
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600}, /* 2.35-3.6V for MX25V8005 */
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L1605",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L1605,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd, /* bit6: error flag */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L1605A/MX25L1606E/MX25L1608E",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L1605,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 64B total; enter 0xB1, exit 0xC1 (MX25L1606E and MX25L1608E only) */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* MX25L1605A bp2 only */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported (MX25L1608E supports dual-I/O read) */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L1605D/MX25L1608D/MX25L1673E",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L1605,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6: Continuously Program (CP) mode, for 73E is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L1635D",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L1635D,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 64B total; enter 0xB1, exit 0xC1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L1635E",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L1635E,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 64B total; enter 0xB1, exit 0xC1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L3205(A)",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L3205,
		.total_size	= 4096,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd, /* bit6: error flag */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L3205D/MX25L3208D",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L3205,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 64B total; enter 0xB1, exit 0xC1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6: continuously program mode */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and dual I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L3206E/MX25L3208E",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L3205,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 64B total; enter 0xB1, exit 0xC1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd,
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and dual I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L3273E",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L3205,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 64B total; enter 0xB1, exit 0xC1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 128} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd,
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and dual I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L3235D",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L3235D,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 256B total; enter 0xB1, exit 0xC1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L6405",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L6405,
		.total_size	= 8192,
		.page_size	= 256,
		/* Has an additional 512B EEPROM sector */
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6: error flag */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L6405D",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L6405,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 64B total; enter 0xB1, exit 0xC1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6: continuously program mode */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O read (0xBB) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L6406E/MX25L6408E",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L6405,
		.total_size	= 8192,
		.page_size	= 256,
		/* MX25L6406E supports SFDP */
		/* OTP: 06E 64B total; enter 0xB1, exit 0xC1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd,
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O read supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L6436E/MX25L6445E/MX25L6465E/MX25L6473E",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L6405,
		.total_size	= 8192,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 512B total; enter 0xB1, exit 0xC1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L12805D",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L12805D,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 64B total; enter 0xB1, exit 0xC1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd,
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L12835F/MX25L12845E/MX25L12865E",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L12805D,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 512B total; enter 0xB1, exit 0xC1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 512} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		/* TODO: security register and SBLK/SBULK; MX25L12835F: configuration register */
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25U1635E",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25U1635E,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 512B total; enter 0xB1, exit 0xC1 */
		/* QPI enable 0x35, disable 0xF5 (0xFF et al. work too) */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PR,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		/* TODO: security register */
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1650, 2000},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25U3235E/F",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25U3235E,
		.total_size	= 4096,
		.page_size	= 256,
		/* F model supports SFDP */
		/* OTP: 512B total; enter 0xB1, exit 0xC1 */
		/* QPI enable 0x35, disable 0xF5 (0xFF et al. work too) */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 128} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		/* TODO: security register */
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1650, 2000},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25U6435E/F",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25U6435E,
		.total_size	= 8192,
		.page_size	= 256,
		/* F model supports SFDP */
		/* OTP: 512B total; enter 0xB1, exit 0xC1 */
		/* QPI enable 0x35, disable 0xF5 (0xFF et al. work too) */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		/* TODO: security register */
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1650, 2000},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25U12835F",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25U12835E,
		.total_size	= 16384,
		.page_size	= 256,
		/* OTP: 512B total; enter 0xB1, exit 0xC1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 512} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		/* TODO: security register */
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1650, 2000},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX25L6495F",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX25L6495F,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 1024B total; enter 0xB1, exit 0xC1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29F001B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29F001B,
		.total_size	= 128,
		.page_size	= 32 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 1},
					{4 * 1024, 2},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29F001T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29F001T,
		.total_size	= 128,
		.page_size	= 32 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 1},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{4 * 1024, 2},
					{8 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29F002(N)B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29F002B,
		.total_size	= 256,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 3},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29F002(N)T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29F002T,
		.total_size	= 256,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
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
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29F022(N)B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29F022B,
		.total_size	= 256,
		.page_size	= 0, /* unused */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 3},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29F022(N)T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29F022T,
		.total_size	= 256,
		.page_size	= 0, /* unused */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
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
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29F040",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29F040,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29GL320EB",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29GL320EB,
		.total_size	= 4096,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 63},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29GL320ET",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29GL320ET,
		.total_size	= 4096,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 63},
					{8 * 1024, 8},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29GL320EH/L",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29GL320EHL,
		.total_size	= 4096,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29GL640EB",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29GL640EB,
		.total_size	= 8192,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 127},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29GL640ET",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29GL640ET,
		.total_size	= 8192,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 127},
					{8 * 1024, 8},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29GL640EH/L",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29GL640EHL,
		.total_size	= 8192,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29GL128F",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29GL128F,
		.total_size	= 16384,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Macronix",
		.name		= "MX29LV040",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= MACRONIX_MX29LV040,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25P05-A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25P05A,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* TODO: check */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	/* The ST M25P05 is a bit of a problem. It has the same ID as the
	 * ST M25P05-A in RES mode, but supports only 128 byte writes instead
	 * of 256 byte writes. We rely heavily on the fact that probe_spi_res1
	 * only is successful if RDID does not work.
	 */
	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25P05",
		.bustype	= BUS_SPI,
		.manufacture_id	= 0, /* Not used. */
		.model_id	= ST_M25P05_RES,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_res1,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* TODO: check */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_1, /* 128 */
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25P10-A",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25P10A,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PRE,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* TODO: check */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	/* The ST M25P10 has the same problem as the M25P05. */
	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25P10",
		.bustype	= BUS_SPI,
		.manufacture_id	= 0, /* Not used. */
		.model_id	= ST_M25P10_RES,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_res1,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* TODO: check */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_1, /* 128 */
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST", /* Numonyx */
		.name		= "M25P20",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25P20,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25P20-old",
		.bustype	= BUS_SPI,
		.manufacture_id	= 0, /* Not used. */
		.model_id	= ST_M25P20_RES,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_res1,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST", /* Numonyx */
		.name		= "M25P40",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25P40,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* TODO: check */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25P40-old",
		.bustype	= BUS_SPI,
		.manufacture_id	= 0, /* Not used. */
		.model_id	= ST_M25P40_RES,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_res1,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* TODO: check */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25P80",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25P80,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* TODO: check */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25P16",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25P16,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* TODO: check */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25P32",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25P32,
		.total_size	= 4096,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* TODO: check */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25P64",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25P64,
		.total_size	= 8192,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* TODO: check */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25P128",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25P128,
		.total_size	= 16384,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* TODO: check */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25PE10",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25PE10,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 2} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25PE20",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25PE20,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25PE40",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25PE40,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25PE80",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25PE80,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25PE16",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25PE16,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25PX80",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25PX80,
		.total_size	= 1024,
		.page_size	= 256,
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { { 4 * 1024, 256 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd, /* bit5: T/B */
		.unlock		= spi_disable_blockprotect_bp2_srwd, /* TODO: per 64kB sector lock registers */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25PX16",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25PX16,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 64B total; read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 512 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd, /* bit5: T/B */
		.unlock		= spi_disable_blockprotect_bp2_srwd, /* TODO: per 64kB sector lock registers */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25PX32",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25PX32,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 64B total; read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PRE,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 1024 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd, /* bit5: T/B */
		.unlock		= spi_disable_blockprotect_bp2_srwd, /* TODO: per 64kB sector lock registers */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M25PX64",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M25PX64,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 64B total; read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { { 4 * 1024, 2048 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd, /* bit5: T/B */
		.unlock		= spi_disable_blockprotect_bp2_srwd, /* TODO: per 64kB sector lock registers */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M45PE10",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M45PE10,
		.total_size	= 128,
		.page_size	= 256,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {256, 512} },
				.block_erase = spi_block_erase_db,
			}, {
				.eraseblocks = { {64 * 1024, 2} },
				.block_erase = spi_block_erase_d8,
			}
		},
		.printlock	= spi_prettyprint_status_register_default_welwip,
		.unlock		= NULL, /* #WP pin write-protects lower 64kB. */
		.write		= spi_chip_write_256, /* Page write (similar to PP but allows 0->1 changes) */
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M45PE20",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M45PE20,
		.total_size	= 256,
		.page_size	= 256,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {256, 1024} },
				.block_erase = spi_block_erase_db,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}
		},
		.printlock	= spi_prettyprint_status_register_default_welwip,
		.unlock		= NULL, /* #WP pin write-protects lower 64kB. */
		.write		= spi_chip_write_256, /* Page write (similar to PP but allows 0->1 changes) */
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M45PE40",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M45PE40,
		.total_size	= 512,
		.page_size	= 256,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {256, 2048} },
				.block_erase = spi_block_erase_db,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}
		},
		.printlock	= spi_prettyprint_status_register_default_welwip,
		.unlock		= NULL, /* #WP pin write-protects lower 64kB. */
		.write		= spi_chip_write_256, /* Page write (similar to PP but allows 0->1 changes) */
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M45PE80",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M45PE80,
		.total_size	= 1024,
		.page_size	= 256,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {256, 4096} },
				.block_erase = spi_block_erase_db,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}
		},
		.printlock	= spi_prettyprint_status_register_default_welwip,
		.unlock		= NULL, /* #WP pin write-protects lower 64kB. */
		.write		= spi_chip_write_256, /* Page write (similar to PP but allows 0->1 changes) */
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "M45PE16",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M45PE16,
		.total_size	= 2048,
		.page_size	= 256,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {256, 8192} },
				.block_erase = spi_block_erase_db,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}
		},
		.printlock	= spi_prettyprint_status_register_default_welwip,
		.unlock		= NULL, /* #WP pin write-protects lower 64kB. */
		.write		= spi_chip_write_256, /* Page write (similar to PP but allows 0->1 changes) */
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "N25Q016",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_N25Q016__1E,
		.total_size	= 2048,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_n25q, /* TODO: config, lock, flag regs */
		.unlock		= spi_disable_blockprotect_n25q, /* TODO: per 64kB sector lock registers */
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "N25Q032..1E",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_N25Q032__1E,
		.total_size	= 4096,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_n25q, /* TODO: config, lock, flag regs */
		.unlock		= spi_disable_blockprotect_n25q, /* TODO: per 64kB sector lock registers */
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "N25Q032..3E",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_N25Q032__3E,
		.total_size	= 4096,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_n25q, /* TODO: config, lock, flag regs */
		.unlock		= spi_disable_blockprotect_n25q, /* TODO: per 64kB sector lock registers */
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "N25Q064..1E", /* ..1E = 1.8V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= ST_N25Q064__1E,
		.total_size	= 8192,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_n25q, /* TODO: config, lock, flag regs */
		.unlock		= spi_disable_blockprotect_n25q, /* TODO: per 64kB sector lock registers */
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "N25Q064..3E", /* ..3E = 3V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id = ST_ID,
		.model_id	= ST_N25Q064__3E,
		.total_size	= 8192,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_n25q, /* TODO: config, lock, flag regs */
		.unlock		= spi_disable_blockprotect_n25q, /* TODO: per 64kB sector lock registers */
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "N25Q128..1E", /* ..1E = 1.8V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id = ST_ID,
		.model_id	= ST_N25Q128__1E,
		.total_size	= 16384,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 4096 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {16384 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_n25q, /* TODO: config, lock, flag regs */
		.unlock		= spi_disable_blockprotect_n25q, /* TODO: per 64kB sector lock registers */
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {1700, 2000},
	},

	{
		.vendor		= "Micron/Numonyx/ST",
		.name		= "N25Q128..3E", /* ..3E = 3V, uniform 64KB/4KB blocks/sectors */
		.bustype	= BUS_SPI,
		.manufacture_id = ST_ID,
		.model_id	= ST_N25Q128__3E,
		.total_size	= 16384,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 64B total; read 0x4B, write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 4096 } },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {16384 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_n25q, /* TODO: config, lock, flag regs */
		.unlock		= spi_disable_blockprotect_n25q, /* TODO: per 64kB sector lock registers */
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "MoselVitelic",
		.name		= "V29C51000B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29C51000B,
		.total_size	= 64,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "MoselVitelic",
		.name		= "V29C51000T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29C51000T,
		.total_size	= 64,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "MoselVitelic",
		.name		= "V29C51400B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29C51400B,
		.total_size	= 512,
		.page_size	= 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024, 512} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "MoselVitelic",
		.name		= "V29C51400T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29C51400T,
		.total_size	= 512,
		.page_size	= 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024, 512} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "MoselVitelic",
		.name		= "V29LC51000",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29LC51000,
		.total_size	= 64,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "MoselVitelic",
		.name		= "V29LC51001",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29LC51001,
		.total_size	= 128,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 256} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "MoselVitelic",
		.name		= "V29LC51002",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= MVC_V29LC51002,
		.total_size	= 256,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 512} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 2} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd,
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O read (0x3B) supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {32 * 1024, 8} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd,
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O read (0x3B) supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd,
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O read (0x3B) supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd,
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O read (0x3B) supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd,
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O read (0x3B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LD256C",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LD256C,
		.total_size	= 32,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 8} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 8} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {32 * 1024, 1} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {32 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {32 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O supported */
		.voltage	= {2700, 3600},
	},
	{
		.vendor		= "PMC",
		.name		= "Pm25LD512(C)",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LD512,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect, /* FIXME: C version supports "Safe Guard" */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O supported */
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LD010(C)",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LD010,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect, /* FIXME: C version supports "Safe Guard" */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O supported */
		.voltage	= {2700, 3600}, /* 2.3-3.6V for Pm25LD010 */
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LD020(C)",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LD020,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect, /* FIXME: C version supports "Safe Guard" */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O supported */
		.voltage	= {2700, 3600}, /* 2.3-3.6V for Pm25LD020 */
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LD040(C)",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LV040,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B), dual I/O supported */
		.voltage	= {2700, 3600}, /* 2.3-3.6V for Pm25LD040 */
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LQ020",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LQ020,
		.total_size	= 256,
		.page_size	= 256,
		/* OTP: 256B total; read 0x4B, write 0xB1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LQ040",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LQ040,
		.total_size	= 512,
		.page_size	= 256,
		/* OTP: 256B total; read 0x4B, write 0xB1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LQ080",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LQ080,
		.total_size	= 1024,
		.page_size	= 256,
		/* OTP: 64B total; read 0x4B, write 0xB1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LQ016",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LQ016,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 256B total; read 0x4B, write 0xB1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2048 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LQ032C",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LQ032C,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 64B total; read 0x4B, write 0xB1 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PR,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4096 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4096 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd, /* bit6 is quad enable */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LV512(A)",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID_NOPREFIX,
		.model_id	= PMC_PM25LV512,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_res2, /* The continuation code is transferred as the 3rd byte m( */
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LV010",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID_NOPREFIX,
		.model_id	= PMC_PM25LV010,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_res2, /* The continuation code is transferred as the 3rd byte m( */
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LV010A",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LV010,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LV020",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LV020,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LV040",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LV040,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LV080B",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LV080B,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm25LV016B",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= PMC_PM25LV016B,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm29F002T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= PMC_ID_NOPREFIX,
		.model_id	= PMC_PM29F002T,
		.total_size	= 256,
		.page_size	= 8 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_FIXME,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{128 * 1024, 1},
					{96 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm29F002B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= PMC_ID_NOPREFIX,
		.model_id	= PMC_PM29F002B,
		.total_size	= 256,
		.page_size	= 8 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_FIXME,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{96 * 1024, 1},
					{128 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm39LV010",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= PMC_ID_NOPREFIX,
		.model_id	= PMC_PM39F010,	/* Pm39LV010 and Pm39F010 have identical IDs but different voltage */
		.total_size	= 128,
		.page_size	= 4096,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 2} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm39LV020",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= PMC_ID_NOPREFIX,
		.model_id	= PMC_PM39LV020,
		.total_size	= 256,
		.page_size	= 4096,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers =
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm39LV040",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= PMC_ID_NOPREFIX,
		.model_id	= PMC_PM39LV040,
		.total_size	= 512,
		.page_size	= 4096,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PR,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers =
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm39LV512",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= PMC_ID_NOPREFIX,
		.model_id	= PMC_PM39LV512,
		.total_size	= 64,
		.page_size	= 4096,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm49FL002",
		.bustype	= BUS_LPC | BUS_FWH, /* A/A Mux */
		.manufacture_id	= PMC_ID_NOPREFIX,
		.model_id	= PMC_PM49FL002,
		.total_size	= 256,
		.page_size	= 16 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PR,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* routine is wrapper to probe_jedec (pm49fl00x.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {16 * 1024, 16} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.unlock		= unlock_regspace2_uniform_32k,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "PMC",
		.name		= "Pm49FL004",
		.bustype	= BUS_LPC | BUS_FWH, /* A/A Mux */
		.manufacture_id	= PMC_ID_NOPREFIX,
		.model_id	= PMC_PM49FL004,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.unlock		= unlock_regspace2_uniform_64k,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Sanyo",
		.name		= "LE25FW106",
		.bustype	= BUS_SPI,
		.manufacture_id	= SANYO_ID,
		.model_id	= SANYO_LE25FW106,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_res2,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {2 * 1024, 64} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp1_srwd, /* FIXME: Add ERSER error flag. */
		.unlock		= spi_disable_blockprotect_bp1_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Sanyo",
		.name		= "LE25FW406A",
		.bustype	= BUS_SPI,
		.manufacture_id	= SANYO_ID,
		.model_id	= SANYO_LE25FW406A,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_res2,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_d7,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
				}
			},
		.printlock	= spi_prettyprint_status_register_plain,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Sanyo",
		.name		= "LE25FU406B",
		.bustype	= BUS_SPI,
		.manufacture_id	= SANYO_ID,
		.model_id	= SANYO_LE25FU406B,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_res2,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_d7,
			},	{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			},	{
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
				}
			},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect, /* #WP pin write-protects SRWP bit. */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2300, 3600},
	},

	{
		.vendor		= "Sanyo",
		.name		= "LE25FW203A",
		.bustype	= BUS_SPI,
		.manufacture_id	= SANYO_ID,
		.model_id	= SANYO_LE25FW203A,
		.total_size	= 256,
		.page_size	= 256,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256, 1024} },
				.block_erase = spi_block_erase_db,
			},	{
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			},	{
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_default_welwip,
		.unlock		= NULL, /* #WP pin write-protects lower 64kB. */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Sanyo",
		.name		= "LE25FW403A",
		.bustype	= BUS_SPI,
		.manufacture_id	= SANYO_ID,
		.model_id	= SANYO_LE25FW403A,
		.total_size	= 512,
		.page_size	= 256,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {256, 2 * 1024} },
				.block_erase = spi_block_erase_db,
			},	{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			},	{
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_default_welwip,
		.unlock		= NULL, /* #WP pin write-protects lower 64kB. */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Sanyo",
		.name		= "LE25FW418A",
		.bustype	= BUS_SPI,
		.manufacture_id	= SANYO_ID,
		.model_id	= SANYO_LE25FW418A,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_res2,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_d7,
			},	{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			},	{
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
				}
			},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect, /* #WP pin write-protects SRWP bit. */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* some quad-read supported ("HD_READ mode") */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Sanyo",
		.name		= "LE25FW806",
		.bustype	= BUS_SPI,
		.manufacture_id	= SANYO_ID,
		.model_id	= SANYO_LE25FW806,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_res2,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			},	{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_d7,
			},	{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			},	{
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect, /* #WP pin write-protects SRWP bit. */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Sanyo",
		.name		= "LE25FW808",
		.bustype	= BUS_SPI,
		.manufacture_id	= SANYO_ID,
		.model_id	= SANYO_LE25FW808,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_res2,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {8 * 1024, 128} },
				.block_erase = spi_block_erase_d7,
			},	{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			},	{
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect, /* #WP pin write-protects SRWP bit. */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* some quad-read supported ("HD_READ mode") */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Sharp",
		.name		= "LH28F008BJT-BTLZ1",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SHARP_ID,
		.model_id	= SHARP_LH28F008BJ__PB,
		.total_size	= 1024,
		.page_size	= 64 * 1024,
		.tested		= TEST_OK_PREW,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 15}
				 },
				.block_erase = erase_block_82802ab,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = erase_sector_49lfxxxc,
			}
		},
		.unlock		= unlock_lh28f008bjt,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Sharp",
		.name		= "LHF00L04",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= SHARP_ID,
		.model_id	= SHARP_LHF00L04,
		.total_size	= 1024,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET | FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 15},
					{8 * 1024, 8}
				 },
				.block_erase = erase_block_82802ab,
			}, {
				.eraseblocks = {
					{1024 * 1024, 1}
				},
				.block_erase = NULL, /* 30 D0, only in A/A mux mode */
			},
		},
		.unlock		= unlock_regspace2_uniform_64k,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.tested		= TEST_OK_PR,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
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
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd,
		.unlock		= spi_disable_blockprotect_bp3_srwd, /* #WP pin write-protects SRWP bit. */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and dual I/O (0x3B) supported */
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
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd,
		.unlock		= spi_disable_blockprotect_bp3_srwd, /* #WP pin write-protects SRWP bit. */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and dual I/O (0x3B) supported */
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
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 2048 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 2048 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd,
		.unlock		= spi_disable_blockprotect_bp3_srwd, /* #WP pin write-protects SRWP bit. */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and dual I/O (0x3B) supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 4096 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 4096 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd, /* TODO: improve */
		.unlock		= spi_disable_blockprotect_bp2_srwd, /* #WP pin write-protects SRWP bit. */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 8192 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 8192 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd, /* TODO: improve */
		.unlock		= spi_disable_blockprotect_bp2_srwd, /* #WP pin write-protects SRWP bit. */
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				/* This chip supports erasing of 32 so-called "parameter sectors" with
				 * opcode 0x20 which may be configured to be on top or bottom of the address
				 * space. Trying to access an address outside these 4kB blocks does have no
				 * effect on the memory contents, e.g.
				.eraseblocks = {
					{4 * 1024, 32},
					{64 * 1024, 254} // inaccessible
				},
				.block_erase = spi_block_erase_20,
			}, { */
				.eraseblocks = { { 64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect_bp2_srwd, /* #WP pin write-protects SRWP bit. */
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {256 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect_bp2_srwd, /* #WP pin write-protects SRWP bit. */
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
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
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp3_srwd,
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {256 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_srwd,
		.unlock		= spi_disable_blockprotect_bp2_srwd,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
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
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				/* This chip supports erasing of the 32 so-called "parameter sectors" with
				 * opcode 0x20. Trying to access an address outside these 4kB blocks does
				 * have no effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{4 * 1024, 32},
					{64 * 1024, 254} // inaccessible
				},
				.block_erase = spi_block_erase_20,
			}, { */
				.eraseblocks = { { 64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_ep_srwd, /* TODO: SR2 and many others */
		.unlock		= spi_disable_blockprotect_bp2_srwd, /* TODO: various other locks */
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {256 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_ep_srwd, /* TODO: SR2 and many others */
		.unlock		= spi_disable_blockprotect_bp2_srwd, /* TODO: various other locks */
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
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
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
			/* FIXME: This chip supports erasing of the 32 so-called "parameter sectors" with
			 * opcode 0x20. Trying to access an address outside these 4kB blocks does have no
			 * effect on the memory contents, but sets a flag in the SR.
				.eraseblocks = {
					{4 * 1024, 32},
					{64 * 1024, 254} // inaccessible
				},
				.block_erase = spi_block_erase_20,
			}, { */
			/* FIXME: Additionally it also supports erase opcode 40h for the respective 2*4 kB pairs
				.eraseblocks = {
					{8 * 1024, 16},
					{64 * 1024, 254} // inaccessible
				},
				.block_erase = spi_block_erase_40,
			}, { */
				.eraseblocks = { { 64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_ep_srwd, /* TODO: Configuration register */
		.unlock		= spi_disable_blockprotect_bp2_srwd,
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Spansion",
		.name		= "S25FL129P......1", /* uniform 256 kB sectors */
		.bustype	= BUS_SPI,
		.manufacture_id	= SPANSION_ID,
		.model_id	= SPANSION_S25FL128,
		.total_size	= 16384,
		.page_size	= 512,
		/* OTP: 506B total, 16B reserved; read 0x4B; write 0x42 */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	= {
			{
				.eraseblocks = { {256 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { { 16384 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_bp2_ep_srwd, /* TODO: Configuration register */
		.unlock		= spi_disable_blockprotect_bp2_srwd,
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25LF020A",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF020_REMS,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EWSR,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rems,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 8} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* FIXME: No BP2 & 3 */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_1, /* AAI supported, but opcode is 0xAF */
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25LF040A",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF040_REMS,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EWSR,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_res2,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* TODO: check */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_1, /* AAI supported, but opcode is 0xAF */
		.read		= spi_chip_read,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25LF080(A)",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF080_REMS,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EITHER,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_res2,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* TODO: check */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_1, /* AAI supported, but opcode is 0xAF */
		.read		= spi_chip_read,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25VF512(A)",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF512_REMS,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EWSR,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rems,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_d8, /* Supported by SST25VF512A only */
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = spi_block_erase_c7, /* Supported by SST25VF512A only */
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* FIXME: No BP2 & 3 */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_1, /* AAI supported, but opcode is 0xAF */
		.read		= spi_chip_read, /* Fast read (0x0B) supported by SST25VF512A only */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25VF010(A)",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF010_REMS,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EWSR,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rems,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_d8, /* Supported by SST25VF010A only */
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7, /* Supported by SST25VF010A only */
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* FIXME: No BP2 & 3 */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_1, /* AAI supported, but opcode is 0xAF */
		.read		= spi_chip_read, /* Fast read (0x0B) supported by SST25VF010A only */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25VF020",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF020_REMS,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EWSR,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rems,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 8} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* FIXME: No BP2 & 3 */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_1, /* AAI supported, but opcode is 0xAF */
		.read		= spi_chip_read, /* only */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25VF020B",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF020B,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EWSR,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 8} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* FIXME: No BP2 & 3 and 2nd SR */
		.unlock		= spi_disable_blockprotect, /* FIXME: 2nd SR */
		.write		= spi_aai_write, /* AAI supported (0xAD) */
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25VF040",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF040_REMS,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EWSR,
		.tested		= TEST_OK_PR,
		.probe		= probe_spi_rems,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* TODO: check */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_1, /* AAI supported, but opcode is 0xAF */
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25VF040B",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF040B,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EWSR,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25vf040b,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_aai_write, /* AAI supported (0xAD) */
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25VF040B.REMS",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF040B_REMS,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EWSR,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rems,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25vf040b,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_aai_write,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25VF080B",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF080B,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EWSR,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* TODO: check */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_aai_write,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25VF016B",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF016B,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EITHER,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25vf016,
		.unlock		= spi_disable_blockprotect,
		.write		= spi_aai_write,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25VF032B",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF032B,
		.total_size	= 4096,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EWSR,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 128} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* TODO: check */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_aai_write,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25VF064C",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25VF064C,
		.total_size	= 8192,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EWSR,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* TODO: check */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST25WF512",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25WF512,
		.total_size	= 64,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EITHER,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 2} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {1024 * 64, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 64, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* FIXME: does not have a BP3 */
		.unlock		= spi_disable_blockprotect_bp2_srwd,
		.write		= spi_aai_write,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "SST",
		.name		= "SST25WF010",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25WF010,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EITHER,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 4} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {1024 * 128, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 128, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* FIXME: does not have a BP3 */
		.unlock		= spi_disable_blockprotect_bp2_srwd,
		.write		= spi_aai_write,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "SST",
		.name		= "SST25WF020",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25WF020,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EITHER,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 8} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 256, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 256, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* FIXME: does not have a BP3 */
		.unlock		= spi_disable_blockprotect_bp2_srwd,
		.write		= spi_aai_write,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "SST",
		.name		= "SST25WF040",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25WF040,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EITHER,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 512, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 512, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* FIXME: does not have a BP3 */
		.unlock		= spi_disable_blockprotect_bp2_srwd,
		.write		= spi_aai_write,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "SST",
		.name		= "SST25WF080",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST25WF080,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_EITHER,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			},
		},
		.printlock	= spi_prettyprint_status_register_sst25, /* *does* have a BP3 but it is useless */
		.unlock		= spi_disable_blockprotect_bp3_srwd,
		.write		= spi_aai_write,
		.read		= spi_chip_read, /* Fast read (0x0B) supported */
		.voltage	= {1650, 1950},
	},

	{
		.vendor		= "SST",
		.name		= "SST28SF040A",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST28SF040,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= 0,
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (sst28sf040.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {128, 4096} },
				.block_erase = erase_sector_28sf040,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_28sf040,
			}
		},
		.unlock		= unprotect_28sf040,
		.write		= write_28sf040,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "SST",
		.name		= "SST29EE010",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST29EE010,
		.total_size	= 128,
		.page_size	= 128,
		.feature_bits	= FEATURE_LONG_RESET,
		.tested		= TEST_OK_PR,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "SST",
		.name		= "SST29LE010",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST29LE010,
		.total_size	= 128,
		.page_size	= 128,
		.feature_bits	= FEATURE_LONG_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST29EE020A",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST29EE020A,
		.total_size	= 256,
		.page_size	= 128,
		.feature_bits	= FEATURE_LONG_RESET,
		.tested		= TEST_OK_PRE,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "SST",
		.name		= "SST29LE020",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST29LE020,
		.total_size	= 256,
		.page_size	= 128,
		.feature_bits	= FEATURE_LONG_RESET,
		.tested		= TEST_OK_PRE,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST39SF512",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST39SF512,
		.total_size	= 64,
		.page_size	= 4096,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 1,			/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "SST",
		.name		= "SST39SF010A",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST39SF010,
		.total_size	= 128,
		.page_size	= 4096,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 1,			/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "SST",
		.name		= "SST39SF020A",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST39SF020,
		.total_size	= 256,
		.page_size	= 4096,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 1,			/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "SST",
		.name		= "SST39SF040",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST39SF040,
		.total_size	= 512,
		.page_size	= 4096,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 1,			/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "SST",
		.name		= "SST39VF512",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST39VF512,
		.total_size	= 64,
		.page_size	= 4096,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 1,			/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 16} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST39VF010",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST39VF010,
		.total_size	= 128,
		.page_size	= 4096,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 1,			/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST39VF020",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST39VF020,
		.total_size	= 256,
		.page_size	= 4096,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 1,			/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST39VF040",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST39VF040,
		.total_size	= 512,
		.page_size	= 4096,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= 1,			/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST39VF080",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST39VF080,
		.total_size	= 1024,
		.page_size	= 4096,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= 1,			/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST49LF002A/B",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST49LF002A,
		.total_size	= 256,
		.page_size	= 16 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 1,		/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {16 * 1024, 16} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = NULL, /* AA 55 80 AA 55 10, only in A/A mux mode */
			}
		},
		.printlock	= printlock_sst_fwhub,
		.unlock		= unlock_sst_fwhub,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST49LF003A/B",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST49LF003A,
		.total_size	= 384,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 1,		/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 96} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 6} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {384 * 1024, 1} },
				.block_erase = NULL, /* AA 55 80 AA 55 10, only in A/A mux mode */
			}
		},
		.printlock	= printlock_sst_fwhub,
		.unlock		= unlock_sst_fwhub,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		/* Contrary to the data sheet, TBL# on the SST49LF004B affects the top 128kB (instead of 64kB)
		 * and is only honored for 64k block erase, but not 4k sector erase.
		 */
		.vendor		= "SST",
		.name		= "SST49LF004A/B",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST49LF004A,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 1,		/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = NULL, /* AA 55 80 AA 55 10, only in A/A mux mode */
			},
		},
		.printlock	= printlock_sst_fwhub,
		.unlock		= unlock_sst_fwhub,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST49LF004C",
		.bustype	= BUS_FWH,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST49LF004C,
		.total_size	= 512,
		.page_size	= 4 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (sst49lfxxxc.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = erase_sector_49lfxxxc,
			}, {
				.eraseblocks = {
					{64 * 1024, 7},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_block_82802ab,
			}
		},
		.printlock	= printlock_regspace2_block_eraser_1,
		.unlock		= unlock_regspace2_block_eraser_1,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST49LF008A",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST49LF008A,
		.total_size	= 1024,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 1,		/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = NULL, /* AA 55 80 AA 55 10, only in A/A mux mode */
			}
		},
		.printlock	= printlock_sst_fwhub,
		.unlock		= unlock_sst_fwhub,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST49LF008C",
		.bustype	= BUS_FWH,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST49LF008C,
		.total_size	= 1024,
		.page_size	= 4 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (sst49lfxxxc.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = erase_sector_49lfxxxc,
			}, {
				.eraseblocks = {
					{64 * 1024, 15},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_block_82802ab,
			}
		},
		.printlock	= printlock_regspace2_block_eraser_1,
		.unlock		= unlock_regspace2_block_eraser_1,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST49LF016C",
		.bustype	= BUS_FWH,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST49LF016C,
		.total_size	= 2048,
		.page_size	= 4 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (sst49lfxxxc.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = erase_sector_49lfxxxc,
			}, {
				.eraseblocks = {
					{64 * 1024, 31},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_block_82802ab,
			}
		},
		.printlock	= printlock_regspace2_block_eraser_1,
		.unlock		= unlock_regspace2_block_eraser_1,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST49LF020",
		.bustype	= BUS_LPC,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST49LF020,
		.total_size	= 256,
		.page_size	= 16 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 1,			/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {16 * 1024, 16} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = NULL,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST49LF020A",
		.bustype	= BUS_LPC,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST49LF020A,
		.total_size	= 256,
		.page_size	= 4 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PRE,
		.probe		= probe_jedec,
		.probe_timing	= 1,			/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {16 * 1024, 16} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = NULL,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST49LF040",
		.bustype	= BUS_LPC,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST49LF040,
		.total_size	= 512,
		.page_size	= 4096,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 1,			/* 150 ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = NULL,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST49LF040B",
		.bustype	= BUS_LPC, /* A/A Mux */
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST49LF040B,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET | FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= probe_jedec,
		.probe_timing	= 1,		/* 150ns */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = NULL,
			}
		},
		.unlock		= unlock_regspace2_uniform_64k,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST49LF080A",
		.bustype	= BUS_LPC, /* A/A Mux */
		.manufacture_id	= SST_ID,
 		.model_id	= SST_SST49LF080A,
		.total_size	= 1024,
		.page_size	= 4096,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_FIXME,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = NULL,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SST",
		.name		= "SST49LF160C",
		.bustype	= BUS_LPC,
		.manufacture_id	= SST_ID,
		.model_id	= SST_SST49LF160C,
		.total_size	= 2048,
		.page_size	= 4 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (sst49lfxxxc.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = erase_sector_49lfxxxc,
			}, {
				.eraseblocks = {
					{64 * 1024, 31},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_block_82802ab,
			}
		},
		.printlock	= printlock_regspace2_block_eraser_1,
		.unlock		= unlock_regspace2_block_eraser_1,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "ST",
		.name		= "M29F002B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29F002B,
		.total_size	= 256,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_AAA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 3},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4750, 5250}, /* 4.75-5.25V for type -X, others 4.5-5.5V */
	},

	{
		.vendor		= "ST",
		.name		= "M29F002T/NT",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29F002T,
		.total_size	= 256,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_AAA | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 3},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4750, 5250}, /* 4.75-5.25V for type -X, others 4.5-5.5V */
	},

	{
		.vendor		= "ST",
		.name		= "M29F040B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29F040B,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO, /* datasheet specifies no timing */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		/* FIXME: this has WORD/BYTE sequences; 2AA for word, 555 for byte */
		.vendor		= "ST",
		.name		= "M29F400BB",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29F400BB,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_SHIFTED | FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= 10, // FIXME: check datasheet. Using the 10 us from probe_m29f400bt
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 7},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},
	{
		/* FIXME: this has WORD/BYTE sequences; 2AA for word, 555 for byte */
		.vendor		= "ST",
		.name		= "M29F400BT",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29F400BT,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_SHIFTED | FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= 10, // FIXME: check datasheet. Using the 10 us from probe_m29f400bt
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 7},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "ST",
		.name		= "M29W010B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29W010B,
		.total_size	= 128,
		.page_size	= 16 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {16 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "ST",
		.name		= "M29W040B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= ST_ID,
		.model_id	= ST_M29W040B,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "ST",
		.name		= "M29W512B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id = ST_ID,
		.model_id	= ST_M29W512B,
		.total_size	= 64,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "ST",
		.name		= "M50FLW040A",
		.bustype	= BUS_FWH | BUS_LPC, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FLW040A,
		.total_size	= 512,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_FIXME,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 16}, /* sector */
					{64 * 1024, 5}, /* block */
					{4 * 1024, 16}, /* sector */
					{4 * 1024, 16}, /* sector */
				},
				.block_erase = erase_sector_stm50,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_block_82802ab,
			}
		},
		.write		= write_82802ab,
		.unlock		= unlock_regspace2_uniform_64k,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FLW040B",
		.bustype	= BUS_FWH | BUS_LPC, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FLW040B,
		.total_size	= 512,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_FIXME,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 16}, /* sector */
					{4 * 1024, 16}, /* sector */
					{64 * 1024, 5}, /* block */
					{4 * 1024, 16}, /* sector */
				},
				.block_erase = erase_sector_stm50,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_block_82802ab,
			}
		},
		.write		= write_82802ab,
		.unlock		= unlock_regspace2_uniform_64k,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FLW080A",
		.bustype	= BUS_FWH | BUS_LPC, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FLW080A,
		.total_size	= 1024,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_FIXME,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 16}, /* sector */
					{64 * 1024, 13}, /* block */
					{4 * 1024, 16}, /* sector */
					{4 * 1024, 16}, /* sector */
				},
				.block_erase = erase_sector_stm50,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = erase_block_82802ab,
			}
		},
		.printlock	= printlock_regspace2_block_eraser_0,
		.unlock		= unlock_regspace2_block_eraser_0,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FLW080B",
		.bustype	= BUS_FWH | BUS_LPC, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FLW080B,
		.total_size	= 1024,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_FIXME,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 16}, /* sector */
					{4 * 1024, 16}, /* sector */
					{64 * 1024, 13}, /* block */
					{4 * 1024, 16}, /* sector */
				},
				.block_erase = erase_sector_stm50,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = erase_block_82802ab,
			}
		},
		.printlock	= printlock_regspace2_block_eraser_0,
		.unlock		= unlock_regspace2_block_eraser_0,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FW002",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FW002,
		.total_size	= 256,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (82802ab.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 3},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_block_82802ab,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = NULL, /* Only in A/A mux mode */
			}
		},
		.printlock	= printlock_regspace2_block_eraser_0,
		.unlock		= unlock_regspace2_block_eraser_0,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FW016",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FW016,
		.total_size	= 2048,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (82802ab.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = erase_block_82802ab,
			}
		},
		.unlock		= unlock_regspace2_uniform_64k,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FW040",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FW040,
		.total_size	= 512,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (82802ab.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_block_82802ab,
			}
		},
		.unlock		= unlock_regspace2_uniform_64k,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50FW080",
		.bustype	= BUS_FWH, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50FW080,
		.total_size	= 1024,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_OK_PR,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (82802ab.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = erase_block_82802ab,
			}
		},
		.unlock		= unlock_regspace2_uniform_64k,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50LPW080",
		.bustype	= BUS_LPC, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50LPW080,
		.total_size	= 1024,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = erase_block_82802ab,
			}
		},
		.unlock		= unlock_regspace2_uniform_64k,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "ST",
		.name		= "M50LPW116",
		.bustype	= BUS_LPC, /* A/A Mux */
		.manufacture_id	= ST_ID,
		.model_id	= ST_M50LPW116,
		.total_size	= 2048,
		.page_size	= 0,
		.feature_bits	= FEATURE_REGISTERMAP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_82802ab,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{4 * 1024, 16},
					{64 * 1024, 30},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_block_82802ab,
			}
		},
		.printlock	= printlock_regspace2_block_eraser_0,
		.unlock		= unlock_regspace2_block_eraser_0,
		.write		= write_82802ab,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program & erase */
	},

	{
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{F,S,V}29C51001B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C51001B,
		.total_size	= 128,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 256} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{F,S,V}29C51001T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C51001T,
		.total_size	= 128,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 256} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{F,S,V}29C51002B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C51002B,
		.total_size	= 256,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 512} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
	},

	{
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{F,S,V}29C51002T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C51002T,
		.total_size	= 256,
		.page_size	= 512,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {512, 512} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
	},

	{
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{F,S,V}29C51004B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C51004B,
		.total_size	= 512,
		.page_size	= 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024, 512} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{F,S,V}29C51004T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C51004T,
		.total_size	= 512,
		.page_size	= 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024, 512} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{S,V}29C31004B",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C31004B,
		.total_size	= 512,
		.page_size	= 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024, 512} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "SyncMOS/MoselVitelic",
		.name		= "{S,V}29C31004T",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= SYNCMOS_MVC_ID,
		.model_id	= SM_MVC_29C31004T,
		.total_size	= 512,
		.page_size	= 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = { {1024, 512} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "TI",
		.name		= "TMS29F002RB",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= TI_OLD_ID,
		.model_id	= TI_TMS29F002RB,
		.total_size	= 256,
		.page_size	= 16384, /* Non-uniform sectors */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{16 * 1024, 1},
					{8 * 1024, 2},
					{32 * 1024, 1},
					{64 * 1024, 3},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "TI",
		.name		= "TMS29F002RT",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= TI_OLD_ID,
		.model_id	= TI_TMS29F002RT,
		.total_size	= 256,
		.page_size	= 16384, /* Non-uniform sectors */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= TIMING_ZERO,	/* Datasheet has no timing info specified */
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 3},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Winbond",
		.name		= "W25Q40.V",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25Q40_V,
		.total_size	= 512,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 756B total; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256, /* Multi I/O supported */
		.read		= spi_chip_read, /* Fast read (0x0B) and multi I/O supported */
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W25Q80.V",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25Q80_V,
		.total_size	= 1024,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W25Q16.V",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25Q16_V,
		.total_size	= 2048,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W25Q32.V",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25Q32_V,
		.total_size	= 4096,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 128} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W25Q64.V",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25Q64_V,
		.total_size	= 8192,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W25Q128.V",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25Q128_V,
		.total_size	= 16384,
		.page_size	= 256,
		/* supports SFDP */
		/* OTP: 1024B total, 256B reserved; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 4096} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 512} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 256} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W25Q20.W",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25Q20_W,
		.total_size	= 256,
		.page_size	= 256,
		/* OTP: 256B total; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 8} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {1700, 1950}, /* Fast read (0x0B) and multi I/O supported */
	},

	{
		.vendor		= "Winbond",
		.name		= "W25Q40.W",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25Q40_W,
		.total_size	= 512,
		.page_size	= 256,
		/* OTP: 256B total; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 16} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {1700, 1950}, /* Fast read (0x0B) and multi I/O supported */
	},

	{
		.vendor		= "Winbond",
		.name		= "W25Q80.W",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25Q80_W,
		.total_size	= 1024,
		.page_size	= 256,
		/* OTP: 256B total; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 32} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {1 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {1700, 1950}, /* Fast read (0x0B) and multi I/O supported */
	},

	{
		.vendor		= "Winbond",
		.name		= "W25Q16.W",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25Q16_W,
		.total_size	= 2048,
		.page_size	= 256,
		/* OTP: 256B total; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_UNTESTED,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {1700, 1950}, /* Fast read (0x0B) and multi I/O supported */
	},

	{
		.vendor		= "Winbond",
		.name		= "W25Q32.W",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25Q32_W,
		.total_size	= 4096,
		.page_size	= 256,
		/* OTP: 256B total; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 128} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {1700, 1950}, /* Fast read (0x0B) and multi I/O supported */
	},

	{
		.vendor		= "Winbond",
		.name		= "W25Q64.W",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25Q64_W,
		.total_size	= 8192,
		.page_size	= 256,
		/* OTP: 256B total; read 0x48; write 0x42, erase 0x44, read ID 0x4B */
		/* QPI enable 0x38, disable 0xFF */
		.feature_bits	= FEATURE_WRSR_WREN | FEATURE_OTP | FEATURE_QPI,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {1700, 1950}, /* Fast read (0x0B) and multi I/O supported */
	},

	{
		.vendor		= "Winbond",
		.name		= "W25X10",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25X10,
		.total_size	= 128,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 2} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W25X20",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25X20,
		.total_size	= 256,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W25X40",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25X40,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W25X80",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25X80,
		.total_size	= 1024,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 256} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W25X16",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25X16,
		.total_size	= 2048,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 512} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 64} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 32} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {2 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W25X32",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25X32,
		.total_size	= 4096,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 1024} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 128} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W25X64",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= WINBOND_NEX_W25X64,
		.total_size	= 8192,
		.page_size	= 256,
		.feature_bits	= FEATURE_WRSR_WREN,
		.tested		= TEST_OK_PROBE,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 2048} },
				.block_erase = spi_block_erase_20,
			}, {
				.eraseblocks = { {32 * 1024, 256} },
				.block_erase = spi_block_erase_52,
			}, {
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = spi_block_erase_d8,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_60,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = spi_block_erase_c7,
			}
		},
		.printlock	= spi_prettyprint_status_register_plain, /* TODO: improve */
		.unlock		= spi_disable_blockprotect,
		.write		= spi_chip_write_256,
		.read		= spi_chip_read,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W29C512A/W29EE512",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W29C512A,
		.total_size	= 64,
		.page_size	= 128,
		.feature_bits	= FEATURE_LONG_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Winbond",
		.name		= "W29C010(M)/W29C011A/W29EE011/W29EE012-old",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W29C010,
		.total_size	= 128,
		.page_size	= 128,
		.feature_bits	= FEATURE_LONG_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_w29ee011,
		.probe_timing	= TIMING_IGNORED, /* routine doesn't use probe_timing (w29ee011.c) */
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec,
		.read		= read_memmapped,
	},

	{/* W29EE011, W29EE012, W29C010M, W29C011A do not support probe_jedec according to the datasheet, but it works for newer(?) steppings. */
		.vendor		= "Winbond",
		.name		= "W29C010(M)/W29C011A/W29EE011/W29EE012",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W29C010,
		.total_size	= 128,
		.page_size	= 128,
		.feature_bits	= FEATURE_LONG_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 10,		/* used datasheet for the W29C011A */
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec,
		.read		= read_memmapped,
	},

	{
		.vendor		= "Winbond",
		.name		= "W29C020(C)/W29C022",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W29C020,
		.total_size	= 256,
		.page_size	= 128,
		.feature_bits	= FEATURE_LONG_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Winbond",
		.name		= "W29C040/P",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W29C040,
		.total_size	= 512,
		.page_size	= 256,
		.feature_bits	= FEATURE_LONG_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Winbond",
		.name		= "W29GL032CB",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID, /* WTF: "Industry Standard compatible Manufacturer ID code of 01h" */
		.model_id	= WINBOND_W29GL032CB,
		.total_size	= 4096,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 63},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W29GL032CT",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID, /* WTF: "Industry Standard compatible Manufacturer ID code of 01h" */
		.model_id	= WINBOND_W29GL032CT,
		.total_size	= 4096,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 63},
					{8 * 1024, 8},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W29GL032CH/L",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID, /* WTF: "Industry Standard compatible Manufacturer ID code of 01h" */
		.model_id	= WINBOND_W29GL032CHL,
		.total_size	= 4096,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 64} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {4 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W29GL064CB",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID, /* WTF: "Industry Standard compatible Manufacturer ID code of 01h" */
		.model_id	= WINBOND_W29GL064CB,
		.total_size	= 8192,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{8 * 1024, 8},
					{64 * 1024, 127},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W29GL064CT",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID, /* WTF: "Industry Standard compatible Manufacturer ID code of 01h" */
		.model_id	= WINBOND_W29GL064CT,
		.total_size	= 8192,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 127},
					{8 * 1024, 8},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W29GL064CH/L",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID, /* WTF: "Industry Standard compatible Manufacturer ID code of 01h" */
		.model_id	= WINBOND_W29GL064CHL,
		.total_size	= 8192,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {8 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W29GL128C",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= AMD_ID, /* WTF: "Industry Standard compatible Manufacturer ID code of 01h" */
		.model_id	= WINBOND_W29GL128CHL,
		.total_size	= 16384,
		.page_size	= 128 * 1024, /* actual page size is 16 */
		.feature_bits	= FEATURE_ADDR_2AA | FEATURE_SHORT_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec_29gl,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {128 * 1024, 128} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {16 * 1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			},
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {2700, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W39F010",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W39F010,
		.total_size	= 128,
		.page_size	= 4 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_w39f010,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Winbond",
		.name		= "W39L010",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W39L010,
		.total_size	= 128,
		.page_size	= 4 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 32} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {128 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_w39l010,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W39L020",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W39L020,
		.total_size	= 256,
		.page_size	= 4 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 64} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 4} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_w39l020,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W39L040",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W39L040,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PR,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_w39l040,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W39V040A",
		.bustype	= BUS_LPC,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W39V040A,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_w39v040a,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W39V040B",
		.bustype	= BUS_LPC,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W39V040B,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_w39v040b,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W39V040C",
		.bustype	= BUS_LPC,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W39V040C,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_w39v040c,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W39V040FA",
		.bustype	= BUS_FWH,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W39V040FA,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PR,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {4 * 1024, 128} },
				.block_erase = erase_block_jedec,
			}, {
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_w39v040fa,
		.unlock		= unlock_regspace2_uniform_64k,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W39V040FB",
		.bustype	= BUS_FWH,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W39V040B,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PR,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_w39v040fb,
		.unlock		= unlock_regspace2_uniform_64k,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program */
	},

	{
		.vendor		= "Winbond",
		.name		= "W39V040FC",
		.bustype	= BUS_FWH,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W39V040C,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_w39v040fc,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program */
	},

	{
		.vendor		= "Winbond",
		.name		= "W39V080A",
		.bustype	= BUS_LPC,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W39V080A,
		.total_size	= 1024,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_w39v080a,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W49F002U/N",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W49F002U,
		.total_size	= 256,
		.page_size	= 128,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{128 * 1024, 1},
					{96 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Winbond",
		.name		= "W49F020",
		.bustype	= BUS_PARALLEL,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W49F020,
		.total_size	= 256,
		.page_size	= 128,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PROBE,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {4500, 5500},
	},

	{
		.vendor		= "Winbond",
		.name		= "W49V002A",
		.bustype	= BUS_LPC,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W49V002A,
		.total_size	= 256,
		.page_size	= 128,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 3},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W49V002FA",
		.bustype	= BUS_FWH,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W49V002FA,
		.total_size	= 256,
		.page_size	= 128,
		.feature_bits	= FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PREW,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = {
					{64 * 1024, 3},
					{32 * 1024, 1},
					{8 * 1024, 2},
					{16 * 1024, 1},
				},
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {256 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600},
	},

	{
		.vendor		= "Winbond",
		.name		= "W39V080FA",
		.bustype	= BUS_FWH,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W39V080FA,
		.total_size	= 1024,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP | FEATURE_EITHER_RESET,
		.tested		= TEST_OK_PR,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 16} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {1024 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_w39v080fa,
		.unlock		= unlock_regspace2_uniform_64k,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program */
	},

	{
		.vendor		= "Winbond",
		.name		= "W39V080FA (dual mode)",
		.bustype	= BUS_FWH,
		.manufacture_id	= WINBOND_ID,
		.model_id	= WINBOND_W39V080FA_DM,
		.total_size	= 512,
		.page_size	= 64 * 1024,
		.feature_bits	= FEATURE_REGISTERMAP | FEATURE_EITHER_RESET,
		.tested		= TEST_UNTESTED,
		.probe		= probe_jedec,
		.probe_timing	= 10,
		.block_erasers	=
		{
			{
				.eraseblocks = { {64 * 1024, 8} },
				.block_erase = erase_sector_jedec,
			}, {
				.eraseblocks = { {512 * 1024, 1} },
				.block_erase = erase_chip_block_jedec,
			}
		},
		.printlock	= printlock_w39v080fa_dual,
		.write		= write_jedec_1,
		.read		= read_memmapped,
		.voltage	= {3000, 3600}, /* Also has 12V fast program */
	},
	
	{
		.vendor		= "Unknown",
		.name		= "SFDP-capable chip",
		.bustype	= BUS_SPI,
		.manufacture_id	= GENERIC_MANUF_ID,
		.model_id	= SFDP_DEVICE_ID,
		/* We present our own "report this" text hence we do not
		 * want the default "This flash part has status UNTESTED..."
		 * text to be printed. */
		.tested		= TEST_OK_PREW,
		.probe		= probe_spi_sfdp,
		.unlock		= spi_disable_blockprotect, /* is this safe? */
		.read		= spi_chip_read,
		/* FIXME: some vendor extensions define this */
		.voltage	= {0},
		 /* Everything below will be set by the probing function. */
		.write		= NULL,
		.total_size	= 0,
		.page_size	= 0,
		.feature_bits	= 0,
		.block_erasers	= {},
	},

	{
		.vendor		= "Programmer",
		.name		= "Opaque flash chip",
		.bustype	= BUS_PROG,
		.manufacture_id	= PROGMANUF_ID,
		.model_id	= PROGDEV_ID,
		.total_size	= 0,
		.page_size	= 256,
		/* probe is assumed to work, rest will be filled in by probe */
		.tested		= TEST_OK_PROBE,
		.probe		= probe_opaque,
		/* eraseblock sizes will be set by the probing function */
		.block_erasers	=
		{
			{
				.block_erase = erase_opaque,
			}
		},
		.write		= write_opaque,
		.read		= read_opaque,
	},

	{
		.vendor		= "AMIC",
		.name		= "unknown AMIC SPI chip",
		.bustype	= BUS_SPI,
		.manufacture_id	= AMIC_ID,
		.model_id	= GENERIC_DEVICE_ID,
		.total_size	= 0,
		.page_size	= 256,
		.tested		= TEST_BAD_PREW,
		.probe		= probe_spi_rdid4,
		.probe_timing	= TIMING_ZERO,
		.write		= NULL,
		.read		= NULL,
	},

	{
		.vendor		= "Atmel",
		.name		= "unknown Atmel SPI chip",
		.bustype	= BUS_SPI,
		.manufacture_id	= ATMEL_ID,
		.model_id	= GENERIC_DEVICE_ID,
		.total_size	= 0,
		.page_size	= 256,
		.tested		= TEST_BAD_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.write		= NULL,
		.read		= NULL,
	},

	{
		.vendor		= "Eon",
		.name		= "unknown Eon SPI chip",
		.bustype	= BUS_SPI,
		.manufacture_id	= EON_ID_NOPREFIX,
		.model_id	= GENERIC_DEVICE_ID,
		.total_size	= 0,
		.page_size	= 256,
		.tested		= TEST_BAD_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.write		= NULL,
		.read		= NULL,
	},

	{
		.vendor		= "Macronix",
		.name		= "unknown Macronix SPI chip",
		.bustype	= BUS_SPI,
		.manufacture_id	= MACRONIX_ID,
		.model_id	= GENERIC_DEVICE_ID,
		.total_size	= 0,
		.page_size	= 256,
		.tested		= TEST_BAD_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.write		= NULL,
		.read		= NULL,
	},

	{
		.vendor		= "PMC",
		.name		= "unknown PMC SPI chip",
		.bustype	= BUS_SPI,
		.manufacture_id	= PMC_ID,
		.model_id	= GENERIC_DEVICE_ID,
		.total_size	= 0,
		.page_size	= 256,
		.tested		= TEST_BAD_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.write		= NULL,
		.read		= NULL,
	},

	{
		.vendor		= "SST",
		.name		= "unknown SST SPI chip",
		.bustype	= BUS_SPI,
		.manufacture_id	= SST_ID,
		.model_id	= GENERIC_DEVICE_ID,
		.total_size	= 0,
		.page_size	= 256,
		.tested		= TEST_BAD_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.write		= NULL,
		.read		= NULL,
	},

	{
		.vendor		= "ST",
		.name		= "unknown ST SPI chip",
		.bustype	= BUS_SPI,
		.manufacture_id	= ST_ID,
		.model_id	= GENERIC_DEVICE_ID,
		.total_size	= 0,
		.page_size	= 256,
		.tested		= TEST_BAD_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.write		= NULL,
		.read		= NULL,
	},

	{
		.vendor		= "Sanyo",
		.name		= "unknown Sanyo SPI chip",
		.bustype	= BUS_SPI,
		.manufacture_id	= SANYO_ID,
		.model_id	= GENERIC_DEVICE_ID,
		.total_size	= 0,
		.page_size	= 256,
		.tested		= TEST_BAD_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.write		= NULL,
		.read		= NULL,
	},

	{
		.vendor		= "Winbond",
		.name		= "unknown Winbond (ex Nexcom) SPI chip",
		.bustype	= BUS_SPI,
		.manufacture_id	= WINBOND_NEX_ID,
		.model_id	= GENERIC_DEVICE_ID,
		.total_size	= 0,
		.page_size	= 256,
		.tested		= TEST_BAD_PREW,
		.probe		= probe_spi_rdid,
		.probe_timing	= TIMING_ZERO,
		.write		= NULL,
		.read		= NULL,
	},

	{
		.vendor		= "Generic",
		.name		= "unknown SPI chip (RDID)",
		.bustype	= BUS_SPI,
		.manufacture_id	= GENERIC_MANUF_ID,
		.model_id	= GENERIC_DEVICE_ID,
		.total_size	= 0,
		.page_size	= 256,
		.tested		= TEST_BAD_PREW,
		.probe		= probe_spi_rdid,
		.write		= NULL,
	},

	{
		.vendor		= "Generic",
		.name		= "unknown SPI chip (REMS)",
		.bustype	= BUS_SPI,
		.manufacture_id	= GENERIC_MANUF_ID,
		.model_id	= GENERIC_DEVICE_ID,
		.total_size	= 0,
		.page_size	= 256,
		.tested		= TEST_BAD_PREW,
		.probe		= probe_spi_rems,
		.write		= NULL,
	},

	{0}
};

const unsigned int flashchips_size = ARRAY_SIZE(flashchips);
