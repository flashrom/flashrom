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
