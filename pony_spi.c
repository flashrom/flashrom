/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2012 Virgil-Adrian Teaca
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

/* Driver for serial programmers compatible with SI-Prog or AJAWe.
 *
 * See http://www.lancos.com/siprogsch.html for SI-Prog schematics and instructions.
 * See http://www.ajawe.pl/ajawe0208.htm for AJAWe serial programmer documentation.
 *
 * Pin layout for SI-Prog-like hardware:
 *
 * MOSI <-------< DTR
 * MISO >-------> CTS
 * SCK  <---+---< RTS
 *          +---> DSR
 * CS#  <-------< TXD
 *
 * and for the AJAWe serial programmer:
 *
 * MOSI <-------< DTR
 * MISO >-------> CTS
 * SCK  <-------< RTS
 * CS#  <-------< TXD
 *
 * DCE  >-------> DSR
 */

#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include "flash.h"
#include "programmer.h"

enum pony_type {
	TYPE_SI_PROG,
	TYPE_SERBANG,
	TYPE_AJAWE
};

/* Pins for master->slave direction */
static int pony_negate_cs = 1;
static int pony_negate_sck = 0;
static int pony_negate_mosi = 0;
/* Pins for slave->master direction */
static int pony_negate_miso = 0;

static void pony_bitbang_set_cs(int val)
{
	if (pony_negate_cs)
		val ^=  1;

	sp_set_pin(PIN_TXD, val);
}

static void pony_bitbang_set_sck(int val)
{
	if (pony_negate_sck)
		val ^=  1;

	sp_set_pin(PIN_RTS, val);
}

static void pony_bitbang_set_mosi(int val)
{
	if (pony_negate_mosi)
		val ^=  1;

	sp_set_pin(PIN_DTR, val);
}

static int pony_bitbang_get_miso(void)
{
	int tmp = sp_get_pin(PIN_CTS);

	if (pony_negate_miso)
		tmp ^= 1;

	return tmp;
}

static const struct bitbang_spi_master bitbang_spi_master_pony = {
	.type = BITBANG_SPI_MASTER_PONY,
	.set_cs = pony_bitbang_set_cs,
	.set_sck = pony_bitbang_set_sck,
	.set_mosi = pony_bitbang_set_mosi,
	.get_miso = pony_bitbang_get_miso,
	.half_period = 0,
};

int pony_spi_init(void)
{
	int i, data_out;
	char *arg = NULL;
	enum pony_type type = TYPE_SI_PROG;
	char *name;
	int have_device = 0;
	int have_prog = 0;

	/* The parameter is in format "dev=/dev/device,type=serbang" */
	arg = extract_programmer_param("dev");
	if (arg && strlen(arg)) {
		sp_fd = sp_openserport(arg, 9600);
		if (sp_fd == SER_INV_FD) {
			free(arg);
			return 1;
		}
		have_device++;
	}
	free(arg);

	if (!have_device) {
		msg_perr("Error: No valid device specified.\n"
			 "Use flashrom -p pony_spi:dev=/dev/device[,type=name]\n");
		return 1;
	}

	arg = extract_programmer_param("type");
	if (arg && !strcasecmp(arg, "serbang")) {
		type = TYPE_SERBANG;
	} else if (arg && !strcasecmp(arg, "si_prog")) {
		type = TYPE_SI_PROG;
	} else if (arg && !strcasecmp( arg, "ajawe")) {
		type = TYPE_AJAWE;
	} else if (arg && !strlen(arg)) {
		msg_perr("Error: Missing argument for programmer type.\n");
		free(arg);
		return 1;
	} else if (arg){
		msg_perr("Error: Invalid programmer type specified.\n");
		free(arg);
		return 1;
	}
	free(arg);

	/*
	 * Configure the serial port pins, depending on the used programmer.
	 */
	switch (type) {
	case TYPE_AJAWE:
		pony_negate_cs = 1;
		pony_negate_sck = 1;
		pony_negate_mosi = 1;
		pony_negate_miso = 1;
		name = "AJAWe";
		break;
	case TYPE_SERBANG:
		pony_negate_cs = 0;
		pony_negate_sck = 0;
		pony_negate_mosi = 0;
		pony_negate_miso = 1;
		name = "serbang";
		break;
	default:
	case TYPE_SI_PROG:
		pony_negate_cs = 1;
		pony_negate_sck = 0;
		pony_negate_mosi = 0;
		pony_negate_miso = 0;
		name = "SI-Prog";
		break;
	}
	msg_pdbg("Using %s programmer pinout.\n", name);

	/*
	 * Detect if there is a compatible hardware programmer connected.
	 */
	pony_bitbang_set_cs(1);
	pony_bitbang_set_sck(1);
	pony_bitbang_set_mosi(1);

	switch (type) {
	case TYPE_AJAWE:
		have_prog = 1;
		break;
	case TYPE_SI_PROG:
	case TYPE_SERBANG:
	default:
		have_prog = 1;
		/* We toggle RTS/SCK a few times and see if DSR changes too. */
		for (i = 1; i <= 10; i++) {
			data_out = i & 1;
			sp_set_pin(PIN_RTS, data_out);
			programmer_delay(1000);

			/* If DSR does not change, we are not connected to what we think */
			if (data_out != sp_get_pin(PIN_DSR)) {
				have_prog = 0;
				break;
			}
		}
		break;
	}

	if (!have_prog) {
		msg_perr("No programmer compatible with %s detected.\n", name);
		return 1;
	}

	if (register_spi_bitbang_master(&bitbang_spi_master_pony)) {
		return 1;
	}
	return 0;
}
