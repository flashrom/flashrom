/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2008 Peter Stuge <peter@stuge.se>
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
 */

#include <stdlib.h>

#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "hwaccess_physmap.h"
#include "hwaccess_x86_io.h"
#include "spi.h"

#define WBSIO_PORT1	0x2e
#define WBSIO_PORT2	0x4e

struct wbsio_spi_data {
	uint16_t spibase;
};

static uint16_t wbsio_get_spibase(uint16_t port)
{
	uint8_t id;
	uint16_t flashport = 0;

	w836xx_ext_enter(port);
	id = sio_read(port, 0x20);
	if (id != 0xa0) {
		msg_perr("\nW83627 not found at 0x%x, id=0x%02x want=0xa0.\n", port, id);
		goto done;
	}

	if (0 == (sio_read(port, 0x24) & 2)) {
		msg_perr("\nW83627 found at 0x%x, but SPI pins are not enabled. (CR[0x24] bit 1=0)\n", port);
		goto done;
	}

	sio_write(port, 0x07, 0x06);
	if (0 == (sio_read(port, 0x30) & 1)) {
		msg_perr("\nW83627 found at 0x%x, but SPI is not enabled. (LDN6[0x30] bit 0=0)\n", port);
		goto done;
	}

	flashport = (sio_read(port, 0x62) << 8) | sio_read(port, 0x63);

done:
	w836xx_ext_leave(port);
	return flashport;
}

/* W83627DHG has 11 command modes:
 * 1=1 command only
 * 2=1 command+1 data write
 * 3=1 command+2 data read
 * 4=1 command+3 address
 * 5=1 command+3 address+1 data write
 * 6=1 command+3 address+4 data write
 * 7=1 command+3 address+1 dummy address inserted by wbsio+4 data read
 * 8=1 command+3 address+1 data read
 * 9=1 command+3 address+2 data read
 * a=1 command+3 address+3 data read
 * b=1 command+3 address+4 data read
 *
 * mode[7:4] holds the command mode
 * mode[3:0] holds SPI address bits [19:16]
 *
 * The Winbond SPI master only supports 20 bit addresses on the SPI bus. :\
 * Would one more byte of RAM in the chip (to get all 24 bits) really make
 * such a big difference?
 */
static int wbsio_spi_send_command(const struct flashctx *flash, unsigned int writecnt,
				  unsigned int readcnt,
				  const unsigned char *writearr,
				  unsigned char *readarr)
{
	unsigned int i;
	uint8_t mode = 0;

	msg_pspew("%s:", __func__);

	const struct wbsio_spi_data *data =
		(const struct wbsio_spi_data *)flash->mst->spi.data;

	if (1 == writecnt && 0 == readcnt) {
		mode = 0x10;
	} else if (2 == writecnt && 0 == readcnt) {
		OUTB(writearr[1], data->spibase + 4);
		msg_pspew(" data=0x%02x", writearr[1]);
		mode = 0x20;
	} else if (1 == writecnt && 2 == readcnt) {
		mode = 0x30;
	} else if (4 == writecnt && 0 == readcnt) {
		msg_pspew(" addr=0x%02x", (writearr[1] & 0x0f));
		for (i = 2; i < writecnt; i++) {
			OUTB(writearr[i], data->spibase + i);
			msg_pspew("%02x", writearr[i]);
		}
		mode = 0x40 | (writearr[1] & 0x0f);
	} else if (5 == writecnt && 0 == readcnt) {
		msg_pspew(" addr=0x%02x", (writearr[1] & 0x0f));
		for (i = 2; i < 4; i++) {
			OUTB(writearr[i], data->spibase + i);
			msg_pspew("%02x", writearr[i]);
		}
		OUTB(writearr[i], data->spibase + i);
		msg_pspew(" data=0x%02x", writearr[i]);
		mode = 0x50 | (writearr[1] & 0x0f);
	} else if (8 == writecnt && 0 == readcnt) {
		msg_pspew(" addr=0x%02x", (writearr[1] & 0x0f));
		for (i = 2; i < 4; i++) {
			OUTB(writearr[i], data->spibase + i);
			msg_pspew("%02x", writearr[i]);
		}
		msg_pspew(" data=0x");
		for (; i < writecnt; i++) {
			OUTB(writearr[i], data->spibase + i);
			msg_pspew("%02x", writearr[i]);
		}
		mode = 0x60 | (writearr[1] & 0x0f);
	} else if (5 == writecnt && 4 == readcnt) {
		/* XXX: TODO not supported by flashrom infrastructure!
		 * This mode, 7, discards the fifth byte in writecnt,
		 * but since we can not express that in flashrom, fail
		 * the operation for now.
		 */
		;
	} else if (4 == writecnt && readcnt >= 1 && readcnt <= 4) {
		msg_pspew(" addr=0x%02x", (writearr[1] & 0x0f));
		for (i = 2; i < writecnt; i++) {
			OUTB(writearr[i], data->spibase + i);
			msg_pspew("%02x", writearr[i]);
		}
		mode = ((7 + readcnt) << 4) | (writearr[1] & 0x0f);
	}
	msg_pspew(" cmd=%02x mode=%02x\n", writearr[0], mode);

	if (!mode) {
		msg_perr("%s: unsupported command type wr=%d rd=%d\n",
			__func__, writecnt, readcnt);
		/* Command type refers to the number of bytes read/written. */
		return SPI_INVALID_LENGTH;
	}

	OUTB(writearr[0], data->spibase);
	OUTB(mode, data->spibase + 1);
	default_delay(10);

	if (!readcnt)
		return 0;

	msg_pspew("%s: returning data =", __func__);
	for (i = 0; i < readcnt; i++) {
		readarr[i] = INB(data->spibase + 4 + i);
		msg_pspew(" 0x%02x", readarr[i]);
	}
	msg_pspew("\n");
	return 0;
}

static int wbsio_spi_read(struct flashctx *flash, uint8_t *buf,
			  unsigned int start, unsigned int len)
{
	mmio_readn((void *)(flash->virtual_memory + start), buf, len);
	return 0;
}

static int wbsio_spi_shutdown(void *data)
{
	free(data);
	return 0;
}

static const struct spi_master spi_master_wbsio = {
	.max_data_read	= MAX_DATA_UNSPECIFIED,
	.max_data_write	= MAX_DATA_UNSPECIFIED,
	.command	= wbsio_spi_send_command,
	.map_flash_region	= physmap,
	.unmap_flash_region	= physunmap,
	.read		= wbsio_spi_read,
	.write_256	= spi_chip_write_1,
	.write_aai	= spi_chip_write_1,
	.shutdown	= wbsio_spi_shutdown,
};

int wbsio_check_for_spi(struct board_cfg *cfg)
{
	uint16_t wbsio_spibase = 0;

	if (0 == (wbsio_spibase = wbsio_get_spibase(WBSIO_PORT1)))
		if (0 == (wbsio_spibase = wbsio_get_spibase(WBSIO_PORT2)))
			return 1;

	msg_pspew("\nwbsio_spibase = 0x%x\n", wbsio_spibase);

	msg_pdbg("%s: Winbond saved on 4 register bits so max chip size is "
		 "1024 kB!\n", __func__);
	max_rom_decode.spi = 1024 * 1024;

	struct wbsio_spi_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for extra SPI master data.\n");
		return SPI_GENERIC_ERROR;
	}
	data->spibase = wbsio_spibase;

	return register_spi_master(&spi_master_wbsio, data);
}
