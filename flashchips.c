/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2000 Silicon Integrated System Corporation
 * SPDX-FileCopyrightText: 2004 Tyan Corp
 * SPDX-FileCopyrightText: 2005-2008 coresystems GmbH <stepan@openbios.org>
 * SPDX-FileCopyrightText: 2006-2009 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2009 Sean Nelson <audiohacked@gmail.com>
 * SPDX-FileCopyrightText: 2025 Google LLC
 */

#include "flash.h"
#include "flashchips.h"
#include "chipdrivers.h"

/**
 * List of supported flash chips.
 *
 * This file is sorted alphabetically by vendor and name.
 *
 * The usual intention is that that this list is sorted by vendor, then chip
 * family and chip density, which is useful for the output of 'flashrom -L'.
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

/* TODO: Refactor implementation to avoid these .c includes */
#include "flashchips/amd.c"
#include "flashchips/amic.c"
#include "flashchips/atmel.c"
#include "flashchips/boya_bohong.c"
#include "flashchips/bright.c"
#include "flashchips/catalyst.c"
#include "flashchips/ene.c"
#include "flashchips/esi.c"
#include "flashchips/esmt.c"
#include "flashchips/eon.c"
#include "flashchips/fudan.c"
#include "flashchips/fujitsu.c"
#include "flashchips/gigadevice.c"
#include "flashchips/hyundai.c"
#include "flashchips/issi.c"
#include "flashchips/intel.c"
#include "flashchips/macronix.c"
#include "flashchips/micron_numonyx_st.c"
#include "flashchips/micron.c"
#include "flashchips/mosel_vitelic.c"
#include "flashchips/nantronics.c"
#include "flashchips/pmc.c"
#include "flashchips/puya.c"
#include "flashchips/sst.c"
#include "flashchips/st.c"
#include "flashchips/sanyo.c"
#include "flashchips/sharp.c"
#include "flashchips/spansion.c"
#include "flashchips/syncmos_mosel_vitelic.c"
#include "flashchips/ti.c"
#include "flashchips/winbond.c"
#include "flashchips/xmc.c"
#include "flashchips/xtx.c"
#include "flashchips/zetta.c"

	/*
	 * These entries are intentionally placed at the end.
	 */
	{
		.vendor		= "Unknown",
		.name		= "SFDP-capable chip",
		.bustype	= BUS_SPI,
		.manufacture_id	= GENERIC_MANUF_ID,
		.model_id	= SFDP_DEVICE_ID,
		.total_size	= 0, /* set by probing function */
		.page_size	= 0, /* set by probing function */
		.feature_bits	= 0, /* set by probing function */
		/* We present our own "report this" text hence we do not */
		/* want the default "This flash part has status UNTESTED..." */
		/* text to be printed. */
		.tested		= { .probe = OK, .read = OK, .erase = OK, .write = OK, .wp = NA },
		.probe		= PROBE_SPI_SFDP,
		.block_erasers	= {}, /* set by probing function */
		.unlock		= SPI_DISABLE_BLOCKPROTECT, /* is this safe? */
		.write		= 0, /* set by probing function */
		.read		= SPI_CHIP_READ,
		/* FIXME: some vendor extensions define this */
		.voltage	= {0},
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
		.probe		= PROBE_OPAQUE,
		/* eraseblock sizes will be set by the probing function */
		.block_erasers	=
		{
			{
				.block_erase = OPAQUE_ERASE,
			}
		},
		.write		= WRITE_OPAQUE,
		.read		= READ_OPAQUE,
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
		.probe		= PROBE_SPI_RDID4,
		.probe_timing	= TIMING_ZERO,
		.write		= 0,
		.read		= 0,
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
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.write		= 0,
		.read		= 0,
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
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.write		= 0,
		.read		= 0,
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
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.write		= 0,
		.read		= 0,
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
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.write		= 0,
		.read		= 0,
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
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.write		= 0,
		.read		= 0,
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
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.write		= 0,
		.read		= 0,
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
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.write		= 0,
		.read		= 0,
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
		.probe		= PROBE_SPI_RDID,
		.probe_timing	= TIMING_ZERO,
		.write		= 0,
		.read		= 0,
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
		.probe		= PROBE_SPI_RDID,
		.write		= 0,
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
		.probe		= PROBE_SPI_REMS,
		.write		= 0,
	},

	{0}
};

const unsigned int flashchips_size = ARRAY_SIZE(flashchips);
