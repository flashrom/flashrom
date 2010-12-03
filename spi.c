/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2007, 2008, 2009 Carl-Daniel Hailfinger
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

#include <string.h>
#include "flash.h"
#include "flashchips.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

enum spi_controller spi_controller = SPI_CONTROLLER_NONE;

const struct spi_programmer spi_programmer[] = {
	{ /* SPI_CONTROLLER_NONE */
		.command = NULL,
		.multicommand = NULL,
		.read = NULL,
		.write_256 = NULL,
	},

#if CONFIG_INTERNAL == 1
#if defined(__i386__) || defined(__x86_64__)
	{ /* SPI_CONTROLLER_ICH7 */
		.command = ich_spi_send_command,
		.multicommand = ich_spi_send_multicommand,
		.read = ich_spi_read,
		.write_256 = ich_spi_write_256,
	},

	{ /* SPI_CONTROLLER_ICH9 */
		.command = ich_spi_send_command,
		.multicommand = ich_spi_send_multicommand,
		.read = ich_spi_read,
		.write_256 = ich_spi_write_256,
	},

	{ /* SPI_CONTROLLER_IT87XX */
		.command = it8716f_spi_send_command,
		.multicommand = default_spi_send_multicommand,
		.read = it8716f_spi_chip_read,
		.write_256 = it8716f_spi_chip_write_256,
	},

	{ /* SPI_CONTROLLER_SB600 */
		.command = sb600_spi_send_command,
		.multicommand = default_spi_send_multicommand,
		.read = sb600_spi_read,
		.write_256 = sb600_spi_write_256,
	},

	{ /* SPI_CONTROLLER_VIA */
		.command = ich_spi_send_command,
		.multicommand = ich_spi_send_multicommand,
		.read = ich_spi_read,
		.write_256 = ich_spi_write_256,
	},

	{ /* SPI_CONTROLLER_WBSIO */
		.command = wbsio_spi_send_command,
		.multicommand = default_spi_send_multicommand,
		.read = wbsio_spi_read,
		.write_256 = spi_chip_write_1,
	},

	{ /* SPI_CONTROLLER_MCP6X_BITBANG */
		.command = bitbang_spi_send_command,
		.multicommand = default_spi_send_multicommand,
		.read = bitbang_spi_read,
		.write_256 = bitbang_spi_write_256,
	},
#endif
#endif

#if CONFIG_FT2232_SPI == 1
	{ /* SPI_CONTROLLER_FT2232 */
		.command = ft2232_spi_send_command,
		.multicommand = default_spi_send_multicommand,
		.read = ft2232_spi_read,
		.write_256 = ft2232_spi_write_256,
	},
#endif

#if CONFIG_DUMMY == 1
	{ /* SPI_CONTROLLER_DUMMY */
		.command = dummy_spi_send_command,
		.multicommand = default_spi_send_multicommand,
		.read = dummy_spi_read,
		.write_256 = dummy_spi_write_256,
	},
#endif

#if CONFIG_BUSPIRATE_SPI == 1
	{ /* SPI_CONTROLLER_BUSPIRATE */
		.command = buspirate_spi_send_command,
		.multicommand = default_spi_send_multicommand,
		.read = buspirate_spi_read,
		.write_256 = buspirate_spi_write_256,
	},
#endif

#if CONFIG_DEDIPROG == 1
	{ /* SPI_CONTROLLER_DEDIPROG */
		.command = dediprog_spi_send_command,
		.multicommand = default_spi_send_multicommand,
		.read = dediprog_spi_read,
		.write_256 = dediprog_spi_write_256,
	},
#endif

#if CONFIG_RAYER_SPI == 1
	{ /* SPI_CONTROLLER_RAYER */
		.command = bitbang_spi_send_command,
		.multicommand = default_spi_send_multicommand,
		.read = bitbang_spi_read,
		.write_256 = bitbang_spi_write_256,
	},
#endif

#if CONFIG_NICINTEL_SPI == 1
	{ /* SPI_CONTROLLER_NICINTEL */
		.command = bitbang_spi_send_command,
		.multicommand = default_spi_send_multicommand,
		.read = bitbang_spi_read,
		.write_256 = bitbang_spi_write_256,
	},
#endif

#if CONFIG_OGP_SPI == 1
	{ /* SPI_CONTROLLER_OGP */
		.command = bitbang_spi_send_command,
		.multicommand = default_spi_send_multicommand,
		.read = bitbang_spi_read,
		.write_256 = bitbang_spi_write_256,
	},
#endif

	{}, /* This entry corresponds to SPI_CONTROLLER_INVALID. */
};

const int spi_programmer_count = ARRAY_SIZE(spi_programmer);

int spi_send_command(unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr)
{
	if (!spi_programmer[spi_controller].command) {
		msg_perr("%s called, but SPI is unsupported on this "
			 "hardware. Please report a bug at "
			 "flashrom@flashrom.org\n", __func__);
		return 1;
	}

	return spi_programmer[spi_controller].command(writecnt, readcnt,
						      writearr, readarr);
}

int spi_send_multicommand(struct spi_command *cmds)
{
	if (!spi_programmer[spi_controller].multicommand) {
		msg_perr("%s called, but SPI is unsupported on this "
			 "hardware. Please report a bug at "
			 "flashrom@flashrom.org\n", __func__);
		return 1;
	}

	return spi_programmer[spi_controller].multicommand(cmds);
}

int default_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			     const unsigned char *writearr, unsigned char *readarr)
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

	return spi_send_multicommand(cmd);
}

int default_spi_send_multicommand(struct spi_command *cmds)
{
	int result = 0;
	for (; (cmds->writecnt || cmds->readcnt) && !result; cmds++) {
		result = spi_send_command(cmds->writecnt, cmds->readcnt,
					  cmds->writearr, cmds->readarr);
	}
	return result;
}

int spi_chip_read(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	int addrbase = 0;
	if (!spi_programmer[spi_controller].read) {
		msg_perr("%s called, but SPI read is unsupported on this "
			 "hardware. Please report a bug at "
			 "flashrom@flashrom.org\n", __func__);
		return 1;
	}

	/* Check if the chip fits between lowest valid and highest possible
	 * address. Highest possible address with the current SPI implementation
	 * means 0xffffff, the highest unsigned 24bit number.
	 */
	addrbase = spi_get_valid_read_addr();
	if (addrbase + flash->total_size * 1024 > (1 << 24)) {
		msg_perr("Flash chip size exceeds the allowed access window. ");
		msg_perr("Read will probably fail.\n");
		/* Try to get the best alignment subject to constraints. */
		addrbase = (1 << 24) - flash->total_size * 1024;
	}
	/* Check if alignment is native (at least the largest power of two which
	 * is a factor of the mapped size of the chip).
	 */
	if (ffs(flash->total_size * 1024) > (ffs(addrbase) ? : 33)) {
		msg_perr("Flash chip is not aligned natively in the allowed "
			 "access window.\n");
		msg_perr("Read will probably return garbage.\n");
	}
	return spi_programmer[spi_controller].read(flash, buf, addrbase + start, len);
}

/*
 * Program chip using page (256 bytes) programming.
 * Some SPI masters can't do this, they use single byte programming instead.
 * The redirect to single byte programming is achieved by setting
 * .write_256 = spi_chip_write_1
 */
/* real chunksize is up to 256, logical chunksize is 256 */
int spi_chip_write_256(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	if (!spi_programmer[spi_controller].write_256) {
		msg_perr("%s called, but SPI page write is unsupported on this "
			 "hardware. Please report a bug at "
			 "flashrom@flashrom.org\n", __func__);
		return 1;
	}

	return spi_programmer[spi_controller].write_256(flash, buf, start, len);
}

/*
 * Get the lowest allowed address for read accesses. This often happens to
 * be the lowest allowed address for all commands which take an address.
 * This is a programmer limitation.
 */
uint32_t spi_get_valid_read_addr(void)
{
	switch (spi_controller) {
#if CONFIG_INTERNAL == 1
#if defined(__i386__) || defined(__x86_64__)
	case SPI_CONTROLLER_ICH7:
		/* Return BBAR for ICH chipsets. */
		return ichspi_bbar;
#endif
#endif
	default:
		return 0;
	}
}
