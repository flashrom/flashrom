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

#include "flash.h"
#include "programmer.h"

/* We have two sets of pins, out and in. The numbers for both sets are
 * independent and are bitshift values, not real pin numbers.
 */
/* Pins for master->slave direction */
#define SPI_CS_PIN 5
#define SPI_SCK_PIN 6
#define SPI_MOSI_PIN 7
/* Pins for slave->master direction */
#define SPI_MISO_PIN 6

static int lpt_iobase;

/* FIXME: All rayer_bitbang_set_* functions could use caching of the value
 * stored at port lpt_iobase to avoid unnecessary INB. In theory, only one
 * INB(lpt_iobase) would be needed on programmer init to get the initial
 * value.
 */

void rayer_bitbang_set_cs(int val)
{
	uint8_t tmp;

	tmp = INB(lpt_iobase);
	tmp &= ~(1 << SPI_CS_PIN);
	tmp |= (val << SPI_CS_PIN);
	OUTB(tmp, lpt_iobase);
}

void rayer_bitbang_set_sck(int val)
{
	uint8_t tmp;

	tmp = INB(lpt_iobase);
	tmp &= ~(1 << SPI_SCK_PIN);
	tmp |= (val << SPI_SCK_PIN);
	OUTB(tmp, lpt_iobase);
}

void rayer_bitbang_set_mosi(int val)
{
	uint8_t tmp;

	tmp = INB(lpt_iobase);
	tmp &= ~(1 << SPI_MOSI_PIN);
	tmp |= (val << SPI_MOSI_PIN);
	OUTB(tmp, lpt_iobase);
}

int rayer_bitbang_get_miso(void)
{
	uint8_t tmp;

	tmp = INB(lpt_iobase + 1);
	tmp = (tmp >> SPI_MISO_PIN) & 0x1;
	return tmp;
}

static const struct bitbang_spi_master bitbang_spi_master_rayer = {
	.type = BITBANG_SPI_MASTER_RAYER,
	.set_cs = rayer_bitbang_set_cs,
	.set_sck = rayer_bitbang_set_sck,
	.set_mosi = rayer_bitbang_set_mosi,
	.get_miso = rayer_bitbang_get_miso,
};

int rayer_spi_init(void)
{
	/* Pick a default value for now. */
	lpt_iobase = 0x378;

	msg_pdbg("Using port 0x%x as I/O base for parallel port access.\n",
		 lpt_iobase);

	get_io_perms();

	/* 1 usec halfperiod delay for now. */
	if (bitbang_spi_init(&bitbang_spi_master_rayer, 1))
		return 1;

	buses_supported = CHIP_BUSTYPE_SPI;
	spi_controller = SPI_CONTROLLER_RAYER;

	return 0;
}

#else
#error PCI port I/O access is not supported on this architecture yet.
#endif
