/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Carl-Daniel Hailfinger
 * Copyright (C) 2008 coresystems GmbH
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
 * Contains the generic SPI framework
 */

#include <strings.h>
#include <string.h>
#include "flash.h"
#include "flashchips.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

int spi_send_command(struct flashctx *flash, unsigned int writecnt,
		     unsigned int readcnt, const unsigned char *writearr,
		     unsigned char *readarr)
{
	return flash->mst->spi.command(flash, writecnt, readcnt, writearr,
				       readarr);
}

int spi_send_multicommand(struct flashctx *flash, struct spi_command *cmds)
{
	return flash->mst->spi.multicommand(flash, cmds);
}

int default_spi_send_command(struct flashctx *flash, unsigned int writecnt,
			     unsigned int readcnt,
			     const unsigned char *writearr,
			     unsigned char *readarr)
{
	struct spi_command cmd[] = {
	{
		.writecnt = writecnt,
		.readcnt = readcnt,
		.writearr = writearr,
		.readarr = readarr,
	}, {
		.writecnt = 0,
		.writearr = NULL,
		.readcnt = 0,
		.readarr = NULL,
	}};

	return spi_send_multicommand(flash, cmd);
}

int default_spi_send_multicommand(struct flashctx *flash,
				  struct spi_command *cmds)
{
	int result = 0;
	for (; (cmds->writecnt || cmds->readcnt) && !result; cmds++) {
		result = spi_send_command(flash, cmds->writecnt, cmds->readcnt,
					  cmds->writearr, cmds->readarr);
	}
	return result;
}

int default_spi_read(struct flashctx *flash, uint8_t *buf, unsigned int start,
		     unsigned int len)
{
	unsigned int max_data = flash->mst->spi.max_data_read;
	if (max_data == MAX_DATA_UNSPECIFIED) {
		msg_perr("%s called, but SPI read chunk size not defined "
			 "on this hardware. Please report a bug at "
			 "flashrom@flashrom.org\n", __func__);
		return 1;
	}
	return spi_read_chunked(flash, buf, start, len, max_data);
}

int default_spi_write_256(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	unsigned int max_data = flash->mst->spi.max_data_write;
	if (max_data == MAX_DATA_UNSPECIFIED) {
		msg_perr("%s called, but SPI write chunk size not defined "
			 "on this hardware. Please report a bug at "
			 "flashrom@flashrom.org\n", __func__);
		return 1;
	}
	return spi_write_chunked(flash, buf, start, len, max_data);
}

int spi_chip_read(struct flashctx *flash, uint8_t *buf, unsigned int start,
		  unsigned int len)
{
	unsigned int addrbase = 0;

	/* Check if the chip fits between lowest valid and highest possible
	 * address. Highest possible address with the current SPI implementation
	 * means 0xffffff, the highest unsigned 24bit number.
	 */
	addrbase = spi_get_valid_read_addr(flash);
	if (addrbase + flash->chip->total_size * 1024 > (1 << 24)) {
		msg_perr("Flash chip size exceeds the allowed access window. ");
		msg_perr("Read will probably fail.\n");
		/* Try to get the best alignment subject to constraints. */
		addrbase = (1 << 24) - flash->chip->total_size * 1024;
	}
	/* Check if alignment is native (at least the largest power of two which
	 * is a factor of the mapped size of the chip).
	 */
	if (ffs(flash->chip->total_size * 1024) > (ffs(addrbase) ? : 33)) {
		msg_perr("Flash chip is not aligned natively in the allowed "
			 "access window.\n");
		msg_perr("Read will probably return garbage.\n");
	}
	return flash->mst->spi.read(flash, buf, addrbase + start, len);
}

/*
 * Program chip using page (256 bytes) programming.
 * Some SPI masters can't do this, they use single byte programming instead.
 * The redirect to single byte programming is achieved by setting
 * .write_256 = spi_chip_write_1
 */
/* real chunksize is up to 256, logical chunksize is 256 */
int spi_chip_write_256(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	return flash->mst->spi.write_256(flash, buf, start, len);
}

/*
 * Get the lowest allowed address for read accesses. This often happens to
 * be the lowest allowed address for all commands which take an address.
 * This is a master limitation.
 */
uint32_t spi_get_valid_read_addr(struct flashctx *flash)
{
	switch (flash->mst->spi.type) {
#if CONFIG_INTERNAL == 1
#if defined(__i386__) || defined(__x86_64__)
	case SPI_CONTROLLER_ICH7:
	case SPI_CONTROLLER_ICH9:
		/* Return BBAR for ICH chipsets. */
		return ichspi_bbar;
#endif
#endif
	default:
		return 0;
	}
}

int spi_aai_write(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	return flash->mst->spi.write_aai(flash, buf, start, len);
}

int register_spi_master(const struct spi_master *mst)
{
	struct registered_master rmst;

	if (!mst->write_aai || !mst->write_256 || !mst->read || !mst->command ||
	    !mst->multicommand ||
	    ((mst->command == default_spi_send_command) &&
	     (mst->multicommand == default_spi_send_multicommand))) {
		msg_perr("%s called with incomplete master definition. "
			 "Please report a bug at flashrom@flashrom.org\n",
			 __func__);
		return ERROR_FLASHROM_BUG;
	}


	rmst.buses_supported = BUS_SPI;
	rmst.spi = *mst;
	return register_master(&rmst);
}
