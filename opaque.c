/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2011,2013,2014 Carl-Daniel Hailfinger
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

int register_opaque_master(const struct opaque_master *mst, void *data)
{
	struct registered_master rmst = {0};

	if (mst->shutdown) {
		if (register_shutdown(mst->shutdown, data)) {
			mst->shutdown(data); /* cleanup */
			return 1;
		}
	}

	if (!mst->probe || !mst->read || !mst->write || !mst->erase) {
		msg_perr("%s called with incomplete master definition. "
			 "Please report a bug at flashrom@flashrom.org\n",
			 __func__);
		return ERROR_FLASHROM_BUG;
	}
	rmst.buses_supported = BUS_PROG;
	rmst.opaque = *mst;
	if (data)
		rmst.opaque.data = data;
	return register_master(&rmst);
}
