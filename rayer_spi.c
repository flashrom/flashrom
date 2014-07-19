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

/* Driver for various LPT adapters.
 *
 * This driver uses non-portable direct I/O port accesses which won't work on
 * any non-x86 platform, and even on x86 there is a high chance there will be
 * collisions with any loaded parallel port drivers.
 * The big advantage of direct port I/O is OS independence and speed because
 * most OS parport drivers will perform many unnecessary accesses although
 * this driver just treats the parallel port as a GPIO set.
 */
#if defined(__i386__) || defined(__x86_64__)

#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"

/* We have two sets of pins, out and in. The numbers for both sets are
 * independent and are bitshift values, not real pin numbers.
 * Default settings are for the RayeR hardware.
 */

struct rayer_programmer {
	const char *type;
	const enum test_state status;
	const char *description;
	const void *dev_data;
};

struct rayer_pinout {
	uint8_t cs_bit;
	uint8_t sck_bit;
	uint8_t mosi_bit;
	uint8_t miso_bit;
	void (*preinit)(const void *);
	int (*shutdown)(void *);
};

static const struct rayer_pinout rayer_spipgm = {
	.cs_bit = 5,
	.sck_bit = 6,
	.mosi_bit = 7,
	.miso_bit = 6,
};

static void dlc5_preinit(const void *);
static int dlc5_shutdown(void *);

static const struct rayer_pinout xilinx_dlc5 = {
	.cs_bit = 2,
	.sck_bit = 1,
	.mosi_bit = 0,
	.miso_bit = 4,
	.preinit =  dlc5_preinit,
	.shutdown = dlc5_shutdown,
};

static void byteblaster_preinit(const void *);
static int byteblaster_shutdown(void *);

static const struct rayer_pinout altera_byteblastermv = {
	.cs_bit = 1,
	.sck_bit = 0,
	.mosi_bit = 6,
	.miso_bit = 7,
	.preinit =  byteblaster_preinit,
	.shutdown = byteblaster_shutdown,
};

static void stk200_preinit(const void *);
static int stk200_shutdown(void *);

static const struct rayer_pinout atmel_stk200 = {
	.cs_bit = 7,
	.sck_bit = 4,
	.mosi_bit = 5,
	.miso_bit = 6,
	.preinit =  stk200_preinit,
	.shutdown = stk200_shutdown,
};

static const struct rayer_pinout wiggler_lpt = {
	.cs_bit = 1,
	.sck_bit = 2,
	.mosi_bit = 3,
	.miso_bit = 7,
};

static const struct rayer_programmer rayer_spi_types[] = {
	{"rayer",		NT,	"RayeR SPIPGM",					&rayer_spipgm},
	{"xilinx",		NT,	"Xilinx Parallel Cable III (DLC 5)",		&xilinx_dlc5},
	{"byteblastermv",	OK,	"Altera ByteBlasterMV",				&altera_byteblastermv},
	{"stk200",		NT,	"Atmel STK200/300 adapter",			&atmel_stk200},
	{"wiggler",		OK,	"Wiggler LPT",					&wiggler_lpt},
	{0},
};

static const struct rayer_pinout *pinout = NULL;

static uint16_t lpt_iobase;

/* Cached value of last byte sent. */
static uint8_t lpt_outbyte;

static void rayer_bitbang_set_cs(int val)
{
	lpt_outbyte &= ~(1 << pinout->cs_bit);
	lpt_outbyte |= (val << pinout->cs_bit);
	OUTB(lpt_outbyte, lpt_iobase);
}

static void rayer_bitbang_set_sck(int val)
{
	lpt_outbyte &= ~(1 << pinout->sck_bit);
	lpt_outbyte |= (val << pinout->sck_bit);
	OUTB(lpt_outbyte, lpt_iobase);
}

static void rayer_bitbang_set_mosi(int val)
{
	lpt_outbyte &= ~(1 << pinout->mosi_bit);
	lpt_outbyte |= (val << pinout->mosi_bit);
	OUTB(lpt_outbyte, lpt_iobase);
}

static int rayer_bitbang_get_miso(void)
{
	uint8_t tmp;

	tmp = INB(lpt_iobase + 1) ^ 0x80; // bit.7 inverted
	tmp = (tmp >> pinout->miso_bit) & 0x1;
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
	const struct rayer_programmer *prog = rayer_spi_types;
	char *arg = NULL;

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
		for (; prog->type != NULL; prog++) {
			if (strcasecmp(arg, prog->type) == 0) {
				break;
			}
		}
		if (prog->type == NULL) {
			msg_perr("Error: Invalid device type specified.\n");
			free(arg);
			return 1;
		}
		free(arg);
	}
	msg_pinfo("Using %s pinout.\n", prog->description);
	pinout = (struct rayer_pinout *)prog->dev_data;

	if (rget_io_perms())
		return 1;

	/* Get the initial value before writing to any line. */
	lpt_outbyte = INB(lpt_iobase);

	if (pinout->shutdown)
		register_shutdown(pinout->shutdown, (void*)pinout);
	if (pinout->preinit)
		pinout->preinit(pinout);

	if (register_spi_bitbang_master(&bitbang_spi_master_rayer))
		return 1;

	return 0;
}

static void byteblaster_preinit(const void *data){
	msg_pdbg("byteblaster_preinit\n");
	/* Assert #EN signal. */
	OUTB(2, lpt_iobase + 2 );
}

static int byteblaster_shutdown(void *data){
	msg_pdbg("byteblaster_shutdown\n");
	/* De-Assert #EN signal. */
	OUTB(0, lpt_iobase + 2 );
	return 0;
}

static void stk200_preinit(const void *data) {
	msg_pdbg("stk200_init\n");
	/* Assert #EN signals, set LED signal. */
	lpt_outbyte = (1 << 6) ;
	OUTB(lpt_outbyte, lpt_iobase);
}

static int stk200_shutdown(void *data) {
	msg_pdbg("stk200_shutdown\n");
	/* Assert #EN signals, clear LED signal. */
	lpt_outbyte = (1 << 2) | (1 << 3);
	OUTB(lpt_outbyte, lpt_iobase);
	return 0;
}

static void dlc5_preinit(const void *data) {
	msg_pdbg("dlc5_preinit\n");
	/* Assert pin 6 to receive MISO. */
	lpt_outbyte |= (1<<4);
	OUTB(lpt_outbyte, lpt_iobase);
}

static int dlc5_shutdown(void *data) {
	msg_pdbg("dlc5_shutdown\n");
	/* De-assert pin 6 to force MISO low. */
	lpt_outbyte &= ~(1<<4);
	OUTB(lpt_outbyte, lpt_iobase);
	return 0;
}

#else
#error PCI port I/O access is not supported on this architecture yet.
#endif
