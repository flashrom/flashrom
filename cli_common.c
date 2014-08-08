/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2009 Carl-Daniel Hailfinger
 * Copyright (C) 2011-2014 Stefan Tauner
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

#include <stdlib.h>
#include <string.h>
#include "flash.h"

/*
 * Return a string corresponding to the bustype parameter.
 * Memory is obtained with malloc() and must be freed with free() by the caller.
 */
char *flashbuses_to_text(enum chipbustype bustype)
{
	char *ret = calloc(1, 1);
	/*
	 * FIXME: Once all chipsets and flash chips have been updated, NONSPI
	 * will cease to exist and should be eliminated here as well.
	 */
	if (bustype == BUS_NONSPI) {
		ret = strcat_realloc(ret, "Non-SPI, ");
	} else {
		if (bustype & BUS_PARALLEL)
			ret = strcat_realloc(ret, "Parallel, ");
		if (bustype & BUS_LPC)
			ret = strcat_realloc(ret, "LPC, ");
		if (bustype & BUS_FWH)
			ret = strcat_realloc(ret, "FWH, ");
		if (bustype & BUS_SPI)
			ret = strcat_realloc(ret, "SPI, ");
		if (bustype & BUS_PROG)
			ret = strcat_realloc(ret, "Programmer-specific, ");
		if (bustype == BUS_NONE)
			ret = strcat_realloc(ret, "None, ");
	}
	/* Kill last comma. */
	ret[strlen(ret) - 2] = '\0';
	ret = realloc(ret, strlen(ret) + 1);
	return ret;
}


void print_chip_support_status(const struct flashchip *chip)
{
	if (chip->feature_bits & FEATURE_OTP) {
		msg_cdbg("This chip may contain one-time programmable memory. flashrom cannot read\n"
			 "and may never be able to write it, hence it may not be able to completely\n"
			 "clone the contents of this chip (see man page for details).\n");
	}

	if ((chip->tested.erase == NA) && (chip->tested.write == NA)) {
		msg_cdbg("This chip's main memory can not be erased/written by design.\n");
	}

	if ((chip->tested.probe == BAD) || (chip->tested.probe == NT) ||
	    (chip->tested.read == BAD)  || (chip->tested.read == NT) ||
	    (chip->tested.erase == BAD) || (chip->tested.erase == NT) ||
	    (chip->tested.write == BAD) || (chip->tested.write == NT)){
		msg_cinfo("===\n");
		if ((chip->tested.probe == BAD) ||
		    (chip->tested.read == BAD) ||
		    (chip->tested.erase == BAD) ||
		    (chip->tested.write == BAD)) {
			msg_cinfo("This flash part has status NOT WORKING for operations:");
			if (chip->tested.probe == BAD)
				msg_cinfo(" PROBE");
			if (chip->tested.read == BAD)
				msg_cinfo(" READ");
			if (chip->tested.erase == BAD)
				msg_cinfo(" ERASE");
			if (chip->tested.write == BAD)
				msg_cinfo(" WRITE");
			msg_cinfo("\n");
		}
		if ((chip->tested.probe == NT) ||
		    (chip->tested.read == NT) ||
		    (chip->tested.erase == NT) ||
		    (chip->tested.write == NT)) {
			msg_cinfo("This flash part has status UNTESTED for operations:");
			if (chip->tested.probe == NT)
				msg_cinfo(" PROBE");
			if (chip->tested.read == NT)
				msg_cinfo(" READ");
			if (chip->tested.erase == NT)
				msg_cinfo(" ERASE");
			if (chip->tested.write == NT)
				msg_cinfo(" WRITE");
			msg_cinfo("\n");
		}
		msg_cinfo("The test status of this chip may have been updated in the latest development\n"
			  "version of flashrom. If you are running the latest development version,\n"
			  "please email a report to flashrom@flashrom.org if any of the above operations\n"
			  "work correctly for you with this flash chip. Please include the flashrom log\n"
			  "file for all operations you tested (see the man page for details), and mention\n"
			  "which mainboard or programmer you tested in the subject line.\n"
			  "Thanks for your help!\n");
	}
}

