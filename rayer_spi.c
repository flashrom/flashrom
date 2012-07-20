/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009,2010 Carl-Daniel Hailfinger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
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

/* Driver for the SPIPGM hardware by "RayeR" Martin Rehak.
 * See http://rayer.ic.cz/elektro/spipgm.htm for schematics and instructions.
 */

/* This driver uses non-portable direct I/O port accesses which won't work on
 * any non-x86 platform, and even on x86 there is a high chance there will be
 * collisions with any loaded parallel port drivers.
 * The big advantage of direct port I/O is OS independence and speed because
 * most OS parport drivers will perform many unnecessary accesses although
 * this driver just treats the parallel port as a GPIO set.
 */
#if defined(__i386__) || defined(__x86_64__)

#include <stdlib.h>
#include <string.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"

enum rayer_type {
	TYPE_RAYER,
	TYPE_XILINX_DLC5,
};

/* We have two sets of pins, out and in. The numbers for both sets are
 * independent and are bitshift values, not real pin numbers.
 * Default settings are for the RayeR hardware.
 */
/* Pins for master->slave direction */
static int rayer_cs_bit = 5;
static int rayer_sck_bit = 6;
static int rayer_mosi_bit = 7;
/* Pins for slave->master direction */
static int rayer_miso_bit = 6;

static uint16_t lpt_iobase;

/* Cached value of last byte sent. */
static uint8_t lpt_outbyte;

static void rayer_bitbang_set_cs(int val)
{
	lpt_outbyte &= ~(1 << rayer_cs_bit);
	lpt_outbyte |= (val << rayer_cs_bit);
	OUTB(lpt_outbyte, lpt_iobase);
}

static void rayer_bitbang_set_sck(int val)
{
	lpt_outbyte &= ~(1 << rayer_sck_bit);
	lpt_outbyte |= (val << rayer_sck_bit);
	OUTB(lpt_outbyte, lpt_iobase);
}

static void rayer_bitbang_set_mosi(int val)
{
	lpt_outbyte &= ~(1 << rayer_mosi_bit);
	lpt_outbyte |= (val << rayer_mosi_bit);
	OUTB(lpt_outbyte, lpt_iobase);
}

static int rayer_bitbang_get_miso(void)
{
	uint8_t tmp;

	tmp = INB(lpt_iobase + 1);
	tmp = (tmp >> rayer_miso_bit) & 0x1;
	return tmp;
}

static const struct bitbang_spi_master bitbang_spi_master_rayer = {
	.type = BITBANG_SPI_MASTER_RAYER,
	.set_cs = rayer_bitbang_set_cs,
	.set_sck = rayer_bitbang_set_sck,
	.set_mosi = rayer_bitbang_set_mosi,
	.get_miso = rayer_bitbang_get_miso,
	.half_period = 0,
};

int rayer_spi_init(void)
{
	char *arg = NULL;
	enum rayer_type rayer_type = TYPE_RAYER;

	/* Non-default port requested? */
	arg = extract_programmer_param("iobase");
	if (arg) {
		char *endptr = NULL;
		unsigned long tmp;
		tmp = strtoul(arg, &endptr, 0);
		/* Port 0, port >0x10000, unaligned ports and garbage strings
		 * are rejected.
		 */
		if (!tmp || (tmp >= 0x10000) || (tmp & 0x3) ||
		    (*endptr != '\0')) {
			/* Using ports below 0x100 is a really bad idea, and
			 * should only be done if no port between 0x100 and
			 * 0xfffc works due to routing issues.
			 */
			msg_perr("Error: iobase= specified, but the I/O base "
				 "given was invalid.\nIt must be a multiple of "
				 "0x4 and lie between 0x100 and 0xfffc.\n");
			free(arg);
			return 1;
		} else {
			lpt_iobase = (uint16_t)tmp;
			msg_pinfo("Non-default I/O base requested. This will "
				  "not change the hardware settings.\n");
		}
	} else {
		/* Pick a default value for the I/O base. */
		lpt_iobase = 0x378;
	}
	free(arg);
	
	msg_pdbg("Using address 0x%x as I/O base for parallel port access.\n",
		 lpt_iobase);

	arg = extract_programmer_param("type");
	if (arg) {
		if (!strcasecmp(arg, "rayer")) {
			rayer_type = TYPE_RAYER;
		} else if (!strcasecmp(arg, "xilinx")) {
			rayer_type = TYPE_XILINX_DLC5;
		} else {
			msg_perr("Error: Invalid device type specified.\n");
			free(arg);
			return 1;
		}
	}
	free(arg);
	switch (rayer_type) {
	case TYPE_RAYER:
		msg_pdbg("Using RayeR SPIPGM pinout.\n");
		/* Bits for master->slave direction */
		rayer_cs_bit = 5;
		rayer_sck_bit = 6;
		rayer_mosi_bit = 7;
		/* Bits for slave->master direction */
		rayer_miso_bit = 6;
		break;
	case TYPE_XILINX_DLC5:
		msg_pdbg("Using Xilinx Parallel Cable III (DLC 5) pinout.\n");
		/* Bits for master->slave direction */
		rayer_cs_bit = 2;
		rayer_sck_bit = 1;
		rayer_mosi_bit = 0;
		/* Bits for slave->master direction */
		rayer_miso_bit = 4;
	}

	get_io_perms();

	/* Get the initial value before writing to any line. */
	lpt_outbyte = INB(lpt_iobase);

	if (bitbang_spi_init(&bitbang_spi_master_rayer))
		return 1;

	return 0;
}

#else
#error PCI port I/O access is not supported on this architecture yet.
#endif
