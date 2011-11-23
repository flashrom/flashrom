/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2011 Carl-Daniel Hailfinger
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

/*
 * Contains the opaque programmer framework.
 * An opaque programmer is a programmer which does not provide direct access
 * to the flash chip and which abstracts all flash chip properties into a
 * programmer specific interface.
 */

#include <stdint.h>
#include "flash.h"
#include "flashchips.h"
#include "chipdrivers.h"
#include "programmer.h"

const struct opaque_programmer opaque_programmer_none = {
	.max_data_read = MAX_DATA_UNSPECIFIED,
	.max_data_write = MAX_DATA_UNSPECIFIED,
	.probe = NULL,
	.read = NULL,
	.write = NULL,
	.erase = NULL,
};

const struct opaque_programmer *opaque_programmer = &opaque_programmer_none;

int probe_opaque(struct flashchip *flash)
{
	if (!opaque_programmer->probe) {
		msg_perr("%s called before register_opaque_programmer. "
			 "Please report a bug at flashrom@flashrom.org\n",
			 __func__);
		return 0;
	}

	return opaque_programmer->probe(flash);
}

int read_opaque(struct flashchip *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	if (!opaque_programmer->read) {
		msg_perr("%s called before register_opaque_programmer. "
			 "Please report a bug at flashrom@flashrom.org\n",
			 __func__);
		return 1;
	}
	return opaque_programmer->read(flash, buf, start, len);
}

int write_opaque(struct flashchip *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	if (!opaque_programmer->write) {
		msg_perr("%s called before register_opaque_programmer. "
			 "Please report a bug at flashrom@flashrom.org\n",
			 __func__);
		return 1;
	}
	return opaque_programmer->write(flash, buf, start, len);
}

int erase_opaque(struct flashchip *flash, unsigned int blockaddr, unsigned int blocklen)
{
	if (!opaque_programmer->erase) {
		msg_perr("%s called before register_opaque_programmer. "
			 "Please report a bug at flashrom@flashrom.org\n",
			 __func__);
		return 1;
	}
	return opaque_programmer->erase(flash, blockaddr, blocklen);
}

void register_opaque_programmer(const struct opaque_programmer *pgm)
{
	if (!pgm->probe || !pgm->read || !pgm->write || !pgm->erase) {
		msg_perr("%s called with one of probe/read/write/erase being "
			 "NULL. Please report a bug at flashrom@flashrom.org\n",
			 __func__);
		return;
	}
	opaque_programmer = pgm;
	buses_supported |= BUS_PROG;
}
