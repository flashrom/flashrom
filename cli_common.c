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
 */

#include <stdlib.h>
#include <string.h>
#include "flash.h"

void print_chip_support_status(struct flashctx *flash)
{
	if (flash->chip->otp) {
		flash->chip->otp->print_status(flash);
	} else if (flash->chip->feature_bits & FEATURE_OTP) {
		msg_cdbg("This chip may contain one-time programmable memory. flashrom may be able\n"
			 "to read, write, erase and/or lock it, if OTP infrastructure support is added.\n"
			 "You could add support and send the patch to flashrom@flashrom.org\n");
	}

	if ((flash->chip->tested.erase == NA) && (flash->chip->tested.write == NA)) {
		msg_cdbg("This chip's main memory can not be erased/written by design.\n");
	}

	if ((flash->chip->tested.probe == BAD) || (flash->chip->tested.probe == NT) ||
	    (flash->chip->tested.read == BAD)  || (flash->chip->tested.read == NT) ||
	    (flash->chip->tested.erase == BAD) || (flash->chip->tested.erase == NT) ||
	    (flash->chip->tested.write == BAD) || (flash->chip->tested.write == NT)){
		msg_cinfo("===\n");
		if ((flash->chip->tested.probe == BAD) ||
		    (flash->chip->tested.read == BAD) ||
		    (flash->chip->tested.erase == BAD) ||
		    (flash->chip->tested.write == BAD)) {
			msg_cinfo("This flash part has status NOT WORKING for operations:");
			if (flash->chip->tested.probe == BAD)
				msg_cinfo(" PROBE");
			if (flash->chip->tested.read == BAD)
				msg_cinfo(" READ");
			if (flash->chip->tested.erase == BAD)
				msg_cinfo(" ERASE");
			if (flash->chip->tested.write == BAD)
				msg_cinfo(" WRITE");
			msg_cinfo("\n");
		}
		if ((flash->chip->tested.probe == NT) ||
		    (flash->chip->tested.read == NT) ||
		    (flash->chip->tested.erase == NT) ||
		    (flash->chip->tested.write == NT)) {
			msg_cinfo("This flash part has status UNTESTED for operations:");
			if (flash->chip->tested.probe == NT)
				msg_cinfo(" PROBE");
			if (flash->chip->tested.read == NT)
				msg_cinfo(" READ");
			if (flash->chip->tested.erase == NT)
				msg_cinfo(" ERASE");
			if (flash->chip->tested.write == NT)
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
