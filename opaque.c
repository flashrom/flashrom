/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2011,2013,2014 Carl-Daniel Hailfinger
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
 * Contains the opaque master framework.
 * An opaque master is a master which does not provide direct access
 * to the flash chip and which abstracts all flash chip properties into a
 * master specific interface.
 */

#include <stdint.h>
#include "flash.h"
#include "flashchips.h"
#include "chipdrivers.h"
#include "programmer.h"

int probe_opaque(struct flashctx *flash)
{
	return flash->mst->opaque.probe(flash);
}

int read_opaque(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	return flash->mst->opaque.read(flash, buf, start, len);
}

int write_opaque(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	return flash->mst->opaque.write(flash, buf, start, len);
}

int erase_opaque(struct flashctx *flash, unsigned int blockaddr, unsigned int blocklen)
{
	return flash->mst->opaque.erase(flash, blockaddr, blocklen);
}

int register_opaque_master(const struct opaque_master *mst)
{
	struct registered_master rmst;

	if (!mst->probe || !mst->read || !mst->write || !mst->erase) {
		msg_perr("%s called with incomplete master definition. "
			 "Please report a bug at flashrom@flashrom.org\n",
			 __func__);
		return ERROR_FLASHROM_BUG;
	}
	rmst.buses_supported = BUS_PROG;
	rmst.opaque = *mst;
	return register_master(&rmst);
}
