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

/* Driver for the SI-Prog like hardware by Lancos.com.
 * See http://www.lancos.com/siprogsch.html for schematics and instructions.
 */

#include <stdlib.h>
#include <string.h>

#include "flash.h"
#include "programmer.h"

/* We have:

 MOSI -----> DTR
 MISO -----> CTS
 SCK  --+--> RTS
        +--> DSR
 CS#  -----> TXD

*/

enum pony_type {
	TYPE_SI_PROG,
	TYPE_SERBANG,
};

/* The hardware programmer used. */
static enum pony_type pony_type = TYPE_SI_PROG;

static void pony_bitbang_set_cs(int val)
{
	if (pony_type == TYPE_SI_PROG)
	{
		/* CS# is negated by the SI-Prog programmer. */
		val ^=  1;
	}
	sp_set_pin(PIN_TXD, val);
}

static void pony_bitbang_set_sck(int val)
{
	sp_set_pin(PIN_RTS, val);
}

static void pony_bitbang_set_mosi(int val)
{
	sp_set_pin(PIN_DTR, val);
}

static int pony_bitbang_get_miso(void)
{
	int tmp;

	tmp = sp_get_pin(PIN_CTS);

	if (pony_type == TYPE_SERBANG)
	{
		/* MISO is negated by the SERBANG programmer. */
		tmp ^= 1;
	}
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
	char *arg = NULL;
	int i, data_in, data_out, have_device = 0;
	int have_prog = 1;

	/* The parameter is on format "dev=/dev/device,type=serbang" */
	arg = extract_programmer_param("dev");

	if (arg && strlen(arg)) {
		sp_fd = sp_openserport( arg, 9600 );
		if (sp_fd < 0) {
			free(arg);
			return 1;
		}
		have_device++;
	}
	free(arg);

	if (!have_device) {
		msg_perr("Error: No valid device specified.\n"
			 "Use flashrom -p pony_spi:dev=/dev/device\n");

		return 1;
	}
	/* Register the shutdown callback. */
	if (register_shutdown(serialport_shutdown, NULL)) {
		return 1;
	}
	arg = extract_programmer_param("type");

	if (arg) {
		if (!strcasecmp( arg, "serbang")) {
			pony_type = TYPE_SERBANG;
		} else if (!strcasecmp( arg, "si_prog") ) {
			pony_type = TYPE_SI_PROG;
		} else {
			msg_perr("Error: Invalid programmer type specified.\n");
			free(arg);
			return 1;
		}
	}
	free(arg);

	msg_pdbg("Using the %s programmer.\n", ((pony_type == TYPE_SI_PROG ) ? "SI-Prog" : "SERBANG"));
	/*
	 * Detect if there is a SI-Prog compatible programmer connected.
	 */
	pony_bitbang_set_cs(1);
	pony_bitbang_set_mosi(1);

	/* We toggle SCK while we keep MOSI and CS# on. */
	for (i = 1; i <= 10; ++i) {
		data_out = i & 1;
		sp_set_pin(PIN_RTS, data_out);
		programmer_delay( 1000 );
		data_in = sp_get_pin(PIN_DSR);

		if (data_out != data_in) {
			have_prog = 0;
			break;
		}
	}

	if (!have_prog) {
		msg_perr( "No SI-Prog compatible hardware detected.\n" );
		return 1;
	}

	if (bitbang_spi_init(&bitbang_spi_master_pony)) {
		return 1;
	}
	return 0;
}
