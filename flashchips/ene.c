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
		.vendor		= "ENE",
		.name		= "KB9012 (EDI)",
		.bustype	= BUS_SPI,
		.total_size	= 128,
		.page_size	= 128,
		.feature_bits	= FEATURE_ERASED_ZERO,
		.tested		= TEST_OK_PREW,
		.spi_cmd_set	= SPI_EDI,
		.probe		= PROBE_EDI_KB9012,
		.probe_timing	= TIMING_ZERO,
		.block_erasers	=
		{
			{
				.eraseblocks = { {128, 1024} },
				.block_erase = EDI_CHIP_BLOCK_ERASE,
			},
		},
		.write		= EDI_CHIP_WRITE,
		.read		= EDI_CHIP_READ,
		.voltage	= {2700, 3600},
		.gran		= WRITE_GRAN_128BYTES,
	},
