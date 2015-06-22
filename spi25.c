/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2007, 2008, 2009, 2010 Carl-Daniel Hailfinger
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
 * Contains the common SPI chip driver functions
 */

#include <string.h>
#include "flash.h"
#include "flashchips.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

static int spi_rdid(struct flashctx *flash, unsigned char *readarr, int bytes)
{
	static const unsigned char cmd[JEDEC_RDID_OUTSIZE] = { JEDEC_RDID };
	int ret;
	int i;

	ret = spi_send_command(flash, sizeof(cmd), bytes, cmd, readarr);
	if (ret)
		return ret;
	msg_cspew("RDID returned");
	for (i = 0; i < bytes; i++)
		msg_cspew(" 0x%02x", readarr[i]);
	msg_cspew(". ");
	return 0;
}

static int spi_rems(struct flashctx *flash, unsigned char *readarr)
{
	unsigned char cmd[JEDEC_REMS_OUTSIZE] = { JEDEC_REMS, 0, 0, 0 };
	uint32_t readaddr;
	int ret;

	ret = spi_send_command(flash, sizeof(cmd), JEDEC_REMS_INSIZE, cmd,
			       readarr);
	if (ret == SPI_INVALID_ADDRESS) {
		/* Find the lowest even address allowed for reads. */
		readaddr = (spi_get_valid_read_addr(flash) + 1) & ~1;
		cmd[1] = (readaddr >> 16) & 0xff,
		cmd[2] = (readaddr >> 8) & 0xff,
		cmd[3] = (readaddr >> 0) & 0xff,
		ret = spi_send_command(flash, sizeof(cmd), JEDEC_REMS_INSIZE,
				       cmd, readarr);
	}
	if (ret)
		return ret;
	msg_cspew("REMS returned 0x%02x 0x%02x. ", readarr[0], readarr[1]);
	return 0;
}

static int spi_res(struct flashctx *flash, unsigned char *readarr, int bytes)
{
	unsigned char cmd[JEDEC_RES_OUTSIZE] = { JEDEC_RES, 0, 0, 0 };
	uint32_t readaddr;
	int ret;
	int i;

	ret = spi_send_command(flash, sizeof(cmd), bytes, cmd, readarr);
	if (ret == SPI_INVALID_ADDRESS) {
		/* Find the lowest even address allowed for reads. */
		readaddr = (spi_get_valid_read_addr(flash) + 1) & ~1;
		cmd[1] = (readaddr >> 16) & 0xff,
		cmd[2] = (readaddr >> 8) & 0xff,
		cmd[3] = (readaddr >> 0) & 0xff,
		ret = spi_send_command(flash, sizeof(cmd), bytes, cmd, readarr);
	}
	if (ret)
		return ret;
	msg_cspew("RES returned");
	for (i = 0; i < bytes; i++)
		msg_cspew(" 0x%02x", readarr[i]);
	msg_cspew(". ");
	return 0;
}

int spi_write_enable(struct flashctx *flash)
{
	static const unsigned char cmd[JEDEC_WREN_OUTSIZE] = { JEDEC_WREN };
	int result;

	/* Send WREN (Write Enable) */
	result = spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);

	if (result)
		msg_cerr("%s failed\n", __func__);

	return result;
}

int spi_write_disable(struct flashctx *flash)
{
	static const unsigned char cmd[JEDEC_WRDI_OUTSIZE] = { JEDEC_WRDI };

	/* Send WRDI (Write Disable) */
	return spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
}

static int probe_spi_rdid_generic(struct flashctx *flash, int bytes)
{
	const struct flashchip *chip = flash->chip;
	unsigned char readarr[4];
	uint32_t id1;
	uint32_t id2;

	if (spi_rdid(flash, readarr, bytes)) {
		return 0;
	}

	if (!oddparity(readarr[0]))
		msg_cdbg("RDID byte 0 parity violation. ");

	/* Check if this is a continuation vendor ID.
	 * FIXME: Handle continuation device IDs.
	 */
	if (readarr[0] == 0x7f) {
		if (!oddparity(readarr[1]))
			msg_cdbg("RDID byte 1 parity violation. ");
		id1 = (readarr[0] << 8) | readarr[1];
		id2 = readarr[2];
		if (bytes > 3) {
			id2 <<= 8;
			id2 |= readarr[3];
		}
	} else {
		id1 = readarr[0];
		id2 = (readarr[1] << 8) | readarr[2];
	}

	msg_cdbg("%s: id1 0x%02x, id2 0x%02x\n", __func__, id1, id2);

	if (id1 == chip->manufacture_id && id2 == chip->model_id)
		return 1;

	/* Test if this is a pure vendor match. */
	if (id1 == chip->manufacture_id && GENERIC_DEVICE_ID == chip->model_id)
		return 1;

	/* Test if there is any vendor ID. */
	if (GENERIC_MANUF_ID == chip->manufacture_id && id1 != 0xff && id1 != 0x00)
		return 1;

	return 0;
}

int probe_spi_rdid(struct flashctx *flash)
{
	return probe_spi_rdid_generic(flash, 3);
}

int probe_spi_rdid4(struct flashctx *flash)
{
	/* Some SPI controllers do not support commands with writecnt=1 and
	 * readcnt=4.
	 */
	switch (flash->mst->spi.type) {
#if CONFIG_INTERNAL == 1
#if defined(__i386__) || defined(__x86_64__)
	case SPI_CONTROLLER_IT87XX:
	case SPI_CONTROLLER_WBSIO:
		msg_cinfo("4 byte RDID not supported on this SPI controller\n");
		return 0;
		break;
#endif
#endif
	default:
		return probe_spi_rdid_generic(flash, 4);
	}

	return 0;
}

int probe_spi_rems(struct flashctx *flash)
{
	const struct flashchip *chip = flash->chip;
	unsigned char readarr[JEDEC_REMS_INSIZE];
	uint32_t id1, id2;

	if (spi_rems(flash, readarr)) {
		return 0;
	}

	id1 = readarr[0];
	id2 = readarr[1];

	msg_cdbg("%s: id1 0x%x, id2 0x%x\n", __func__, id1, id2);

	if (id1 == chip->manufacture_id && id2 == chip->model_id)
		return 1;

	/* Test if this is a pure vendor match. */
	if (id1 == chip->manufacture_id && GENERIC_DEVICE_ID == chip->model_id)
		return 1;

	/* Test if there is any vendor ID. */
	if (GENERIC_MANUF_ID == chip->manufacture_id && id1 != 0xff && id1 != 0x00)
		return 1;

	return 0;
}

int probe_spi_res1(struct flashctx *flash)
{
	static const unsigned char allff[] = {0xff, 0xff, 0xff};
	static const unsigned char all00[] = {0x00, 0x00, 0x00};
	unsigned char readarr[3];
	uint32_t id2;

	/* We only want one-byte RES if RDID and REMS are unusable. */

	/* Check if RDID is usable and does not return 0xff 0xff 0xff or
	 * 0x00 0x00 0x00. In that case, RES is pointless.
	 */
	if (!spi_rdid(flash, readarr, 3) && memcmp(readarr, allff, 3) &&
	    memcmp(readarr, all00, 3)) {
		msg_cdbg("Ignoring RES in favour of RDID.\n");
		return 0;
	}
	/* Check if REMS is usable and does not return 0xff 0xff or
	 * 0x00 0x00. In that case, RES is pointless.
	 */
	if (!spi_rems(flash, readarr) &&
	    memcmp(readarr, allff, JEDEC_REMS_INSIZE) &&
	    memcmp(readarr, all00, JEDEC_REMS_INSIZE)) {
		msg_cdbg("Ignoring RES in favour of REMS.\n");
		return 0;
	}

	if (spi_res(flash, readarr, 1)) {
		return 0;
	}

	id2 = readarr[0];

	msg_cdbg("%s: id 0x%x\n", __func__, id2);

	if (id2 != flash->chip->model_id)
		return 0;

	return 1;
}

int probe_spi_res2(struct flashctx *flash)
{
	unsigned char readarr[2];
	uint32_t id1, id2;

	if (spi_res(flash, readarr, 2)) {
		return 0;
	}

	id1 = readarr[0];
	id2 = readarr[1];

	msg_cdbg("%s: id1 0x%x, id2 0x%x\n", __func__, id1, id2);

	if (id1 != flash->chip->manufacture_id || id2 != flash->chip->model_id)
		return 0;

	return 1;
}

int probe_spi_res3(struct flashctx *flash)
{
	unsigned char readarr[3];
	uint32_t id1, id2;

	if (spi_res(flash, readarr, 3)) {
		return 0;
	}

	id1 = (readarr[0] << 8) | readarr[1];
	id2 = readarr[2];

	msg_cdbg("%s: id1 0x%x, id2 0x%x\n", __func__, id1, id2);

	if (id1 != flash->chip->manufacture_id || id2 != flash->chip->model_id)
		return 0;

	return 1;
}

/* Only used for some Atmel chips. */
int probe_spi_at25f(struct flashctx *flash)
{
	static const unsigned char cmd[AT25F_RDID_OUTSIZE] = { AT25F_RDID };
	unsigned char readarr[AT25F_RDID_INSIZE];
	uint32_t id1;
	uint32_t id2;

	if (spi_send_command(flash, sizeof(cmd), sizeof(readarr), cmd, readarr))
		return 0;

	id1 = readarr[0];
	id2 = readarr[1];

	msg_cdbg("%s: id1 0x%02x, id2 0x%02x\n", __func__, id1, id2);

	if (id1 == flash->chip->manufacture_id && id2 == flash->chip->model_id)
		return 1;

	return 0;
}

int spi_chip_erase_60(struct flashctx *flash)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_CE_60_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_CE_60 },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};
	
	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution\n",
			__func__);
		return result;
	}
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 1-85 s, so wait in 1 s steps.
	 */
	/* FIXME: We assume spi_read_status_register will never fail. */
	while (spi_read_status_register(flash) & SPI_SR_WIP)
		programmer_delay(1000 * 1000);
	/* FIXME: Check the status register for errors. */
	return 0;
}

int spi_chip_erase_62(struct flashctx *flash)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_CE_62_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_CE_62 },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};
	
	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution\n",
			__func__);
		return result;
	}
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 2-5 s, so wait in 100 ms steps.
	 */
	/* FIXME: We assume spi_read_status_register will never fail. */
	while (spi_read_status_register(flash) & SPI_SR_WIP)
		programmer_delay(100 * 1000);
	/* FIXME: Check the status register for errors. */
	return 0;
}

int spi_chip_erase_c7(struct flashctx *flash)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_CE_C7_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_CE_C7 },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution\n", __func__);
		return result;
	}
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 1-85 s, so wait in 1 s steps.
	 */
	/* FIXME: We assume spi_read_status_register will never fail. */
	while (spi_read_status_register(flash) & SPI_SR_WIP)
		programmer_delay(1000 * 1000);
	/* FIXME: Check the status register for errors. */
	return 0;
}

int spi_block_erase_52(struct flashctx *flash, unsigned int addr,
		       unsigned int blocklen)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_BE_52_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_BE_52,
					(addr >> 16) & 0xff,
					(addr >> 8) & 0xff,
					(addr & 0xff)
				},
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n",
			__func__, addr);
		return result;
	}
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 100-4000 ms, so wait in 100 ms steps.
	 */
	while (spi_read_status_register(flash) & SPI_SR_WIP)
		programmer_delay(100 * 1000);
	/* FIXME: Check the status register for errors. */
	return 0;
}

/* Block size is usually
 * 32M (one die) for Micron
 */
int spi_block_erase_c4(struct flashctx *flash, unsigned int addr, unsigned int blocklen)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_BE_C4_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_BE_C4,
					(addr >> 16) & 0xff,
					(addr >> 8) & 0xff,
					(addr & 0xff)
				},
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n", __func__, addr);
		return result;
	}
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 240-480 s, so wait in 500 ms steps.
	 */
	while (spi_read_status_register(flash) & SPI_SR_WIP)
		programmer_delay(500 * 1000 * 1000);
	/* FIXME: Check the status register for errors. */
	return 0;
}

/* Block size is usually
 * 64k for Macronix
 * 32k for SST
 * 4-32k non-uniform for EON
 */
int spi_block_erase_d8(struct flashctx *flash, unsigned int addr,
		       unsigned int blocklen)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_BE_D8_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_BE_D8,
					(addr >> 16) & 0xff,
					(addr >> 8) & 0xff,
					(addr & 0xff)
				},
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n",
			__func__, addr);
		return result;
	}
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 100-4000 ms, so wait in 100 ms steps.
	 */
	while (spi_read_status_register(flash) & SPI_SR_WIP)
		programmer_delay(100 * 1000);
	/* FIXME: Check the status register for errors. */
	return 0;
}

/* Block size is usually
 * 4k for PMC
 */
int spi_block_erase_d7(struct flashctx *flash, unsigned int addr,
		       unsigned int blocklen)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_BE_D7_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_BE_D7,
					(addr >> 16) & 0xff,
					(addr >> 8) & 0xff,
					(addr & 0xff)
				},
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n",
			__func__, addr);
		return result;
	}
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 100-4000 ms, so wait in 100 ms steps.
	 */
	while (spi_read_status_register(flash) & SPI_SR_WIP)
		programmer_delay(100 * 1000);
	/* FIXME: Check the status register for errors. */
	return 0;
}

/* Page erase (usually 256B blocks) */
int spi_block_erase_db(struct flashctx *flash, unsigned int addr, unsigned int blocklen)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_PE_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_PE,
					(addr >> 16) & 0xff,
					(addr >> 8) & 0xff,
					(addr & 0xff)
				},
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	} };

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n", __func__, addr);
		return result;
	}

	/* Wait until the Write-In-Progress bit is cleared.
	 * This takes up to 20 ms usually (on worn out devices up to the 0.5s range), so wait in 1 ms steps. */
	while (spi_read_status_register(flash) & SPI_SR_WIP)
		programmer_delay(1 * 1000);
	/* FIXME: Check the status register for errors. */
	return 0;
}

/* Sector size is usually 4k, though Macronix eliteflash has 64k */
int spi_block_erase_20(struct flashctx *flash, unsigned int addr,
		       unsigned int blocklen)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_SE_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_SE,
					(addr >> 16) & 0xff,
					(addr >> 8) & 0xff,
					(addr & 0xff)
				},
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n",
			__func__, addr);
		return result;
	}
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 15-800 ms, so wait in 10 ms steps.
	 */
	while (spi_read_status_register(flash) & SPI_SR_WIP)
		programmer_delay(10 * 1000);
	/* FIXME: Check the status register for errors. */
	return 0;
}

int spi_block_erase_50(struct flashctx *flash, unsigned int addr, unsigned int blocklen)
{
	int result;
	struct spi_command cmds[] = {
	{
/*		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, { */
		.writecnt	= JEDEC_BE_50_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_BE_50,
					(addr >> 16) & 0xff,
					(addr >> 8) & 0xff,
					(addr & 0xff)
				},
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n", __func__, addr);
		return result;
	}
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 10 ms, so wait in 1 ms steps.
	 */
	while (spi_read_status_register(flash) & SPI_SR_WIP)
		programmer_delay(1 * 1000);
	/* FIXME: Check the status register for errors. */
	return 0;
}

int spi_block_erase_81(struct flashctx *flash, unsigned int addr, unsigned int blocklen)
{
	int result;
	struct spi_command cmds[] = {
	{
/*		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, { */
		.writecnt	= JEDEC_BE_81_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_BE_81,
					(addr >> 16) & 0xff,
					(addr >> 8) & 0xff,
					(addr & 0xff)
				},
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n", __func__, addr);
		return result;
	}
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 8 ms, so wait in 1 ms steps.
	 */
	while (spi_read_status_register(flash) & SPI_SR_WIP)
		programmer_delay(1 * 1000);
	/* FIXME: Check the status register for errors. */
	return 0;
}

int spi_block_erase_60(struct flashctx *flash, unsigned int addr,
		       unsigned int blocklen)
{
	if ((addr != 0) || (blocklen != flash->chip->total_size * 1024)) {
		msg_cerr("%s called with incorrect arguments\n",
			__func__);
		return -1;
	}
	return spi_chip_erase_60(flash);
}

int spi_block_erase_62(struct flashctx *flash, unsigned int addr, unsigned int blocklen)
{
	if ((addr != 0) || (blocklen != flash->chip->total_size * 1024)) {
		msg_cerr("%s called with incorrect arguments\n",
			__func__);
		return -1;
	}
	return spi_chip_erase_62(flash);
}

int spi_block_erase_c7(struct flashctx *flash, unsigned int addr,
		       unsigned int blocklen)
{
	if ((addr != 0) || (blocklen != flash->chip->total_size * 1024)) {
		msg_cerr("%s called with incorrect arguments\n",
			__func__);
		return -1;
	}
	return spi_chip_erase_c7(flash);
}

erasefunc_t *spi_get_erasefn_from_opcode(uint8_t opcode)
{
	switch(opcode){
	case 0xff:
	case 0x00:
		/* Not specified, assuming "not supported". */
		return NULL;
	case 0x20:
		return &spi_block_erase_20;
	case 0x50:
		return &spi_block_erase_50;
	case 0x52:
		return &spi_block_erase_52;
	case 0x60:
		return &spi_block_erase_60;
	case 0x62:
		return &spi_block_erase_62;
	case 0x81:
		return &spi_block_erase_81;
	case 0xc4:
		return &spi_block_erase_c4;
	case 0xc7:
		return &spi_block_erase_c7;
	case 0xd7:
		return &spi_block_erase_d7;
	case 0xd8:
		return &spi_block_erase_d8;
	case 0xdb:
		return &spi_block_erase_db;
	default:
		msg_cinfo("%s: unknown erase opcode (0x%02x). Please report "
			  "this at flashrom@flashrom.org\n", __func__, opcode);
		return NULL;
	}
}

int spi_byte_program(struct flashctx *flash, unsigned int addr,
		     uint8_t databyte)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_BYTE_PROGRAM_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_BYTE_PROGRAM,
					(addr >> 16) & 0xff,
					(addr >> 8) & 0xff,
					(addr & 0xff),
					databyte
				},
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n",
			__func__, addr);
	}
	return result;
}

int spi_nbyte_program(struct flashctx *flash, unsigned int addr, const uint8_t *bytes, unsigned int len)
{
	int result;
	/* FIXME: Switch to malloc based on len unless that kills speed. */
	unsigned char cmd[JEDEC_BYTE_PROGRAM_OUTSIZE - 1 + 256] = {
		JEDEC_BYTE_PROGRAM,
		(addr >> 16) & 0xff,
		(addr >> 8) & 0xff,
		(addr >> 0) & 0xff,
	};
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_BYTE_PROGRAM_OUTSIZE - 1 + len,
		.writearr	= cmd,
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	if (!len) {
		msg_cerr("%s called for zero-length write\n", __func__);
		return 1;
	}
	if (len > 256) {
		msg_cerr("%s called for too long a write\n", __func__);
		return 1;
	}

	memcpy(&cmd[4], bytes, len);

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n",
			__func__, addr);
	}
	return result;
}

int spi_nbyte_read(struct flashctx *flash, unsigned int address, uint8_t *bytes,
		   unsigned int len)
{
	const unsigned char cmd[JEDEC_READ_OUTSIZE] = {
		JEDEC_READ,
		(address >> 16) & 0xff,
		(address >> 8) & 0xff,
		(address >> 0) & 0xff,
	};

	/* Send Read */
	return spi_send_command(flash, sizeof(cmd), len, cmd, bytes);
}

/*
 * Read a part of the flash chip.
 * FIXME: Use the chunk code from Michael Karcher instead.
 * Each page is read separately in chunks with a maximum size of chunksize.
 */
int spi_read_chunked(struct flashctx *flash, uint8_t *buf, unsigned int start,
		     unsigned int len, unsigned int chunksize)
{
	int rc = 0;
	unsigned int i, j, starthere, lenhere, toread;
	unsigned int page_size = flash->chip->page_size;

	/* Warning: This loop has a very unusual condition and body.
	 * The loop needs to go through each page with at least one affected
	 * byte. The lowest page number is (start / page_size) since that
	 * division rounds down. The highest page number we want is the page
	 * where the last byte of the range lives. That last byte has the
	 * address (start + len - 1), thus the highest page number is
	 * (start + len - 1) / page_size. Since we want to include that last
	 * page as well, the loop condition uses <=.
	 */
	for (i = start / page_size; i <= (start + len - 1) / page_size; i++) {
		/* Byte position of the first byte in the range in this page. */
		/* starthere is an offset to the base address of the chip. */
		starthere = max(start, i * page_size);
		/* Length of bytes in the range in this page. */
		lenhere = min(start + len, (i + 1) * page_size) - starthere;
		for (j = 0; j < lenhere; j += chunksize) {
			toread = min(chunksize, lenhere - j);
			rc = spi_nbyte_read(flash, starthere + j, buf + starthere - start + j, toread);
			if (rc)
				break;
		}
		if (rc)
			break;
	}

	return rc;
}

/*
 * Write a part of the flash chip.
 * FIXME: Use the chunk code from Michael Karcher instead.
 * Each page is written separately in chunks with a maximum size of chunksize.
 */
int spi_write_chunked(struct flashctx *flash, const uint8_t *buf, unsigned int start,
		      unsigned int len, unsigned int chunksize)
{
	int rc = 0;
	unsigned int i, j, starthere, lenhere, towrite;
	/* FIXME: page_size is the wrong variable. We need max_writechunk_size
	 * in struct flashctx to do this properly. All chips using
	 * spi_chip_write_256 have page_size set to max_writechunk_size, so
	 * we're OK for now.
	 */
	unsigned int page_size = flash->chip->page_size;

	/* Warning: This loop has a very unusual condition and body.
	 * The loop needs to go through each page with at least one affected
	 * byte. The lowest page number is (start / page_size) since that
	 * division rounds down. The highest page number we want is the page
	 * where the last byte of the range lives. That last byte has the
	 * address (start + len - 1), thus the highest page number is
	 * (start + len - 1) / page_size. Since we want to include that last
	 * page as well, the loop condition uses <=.
	 */
	for (i = start / page_size; i <= (start + len - 1) / page_size; i++) {
		/* Byte position of the first byte in the range in this page. */
		/* starthere is an offset to the base address of the chip. */
		starthere = max(start, i * page_size);
		/* Length of bytes in the range in this page. */
		lenhere = min(start + len, (i + 1) * page_size) - starthere;
		for (j = 0; j < lenhere; j += chunksize) {
			towrite = min(chunksize, lenhere - j);
			rc = spi_nbyte_program(flash, starthere + j, buf + starthere - start + j, towrite);
			if (rc)
				break;
			while (spi_read_status_register(flash) & SPI_SR_WIP)
				programmer_delay(10);
		}
		if (rc)
			break;
	}

	return rc;
}

/*
 * Program chip using byte programming. (SLOW!)
 * This is for chips which can only handle one byte writes
 * and for chips where memory mapped programming is impossible
 * (e.g. due to size constraints in IT87* for over 512 kB)
 */
/* real chunksize is 1, logical chunksize is 1 */
int spi_chip_write_1(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	unsigned int i;
	int result = 0;

	for (i = start; i < start + len; i++) {
		result = spi_byte_program(flash, i, buf[i - start]);
		if (result)
			return 1;
		while (spi_read_status_register(flash) & SPI_SR_WIP)
			programmer_delay(10);
	}

	return 0;
}

int default_spi_write_aai(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	uint32_t pos = start;
	int result;
	unsigned char cmd[JEDEC_AAI_WORD_PROGRAM_CONT_OUTSIZE] = {
		JEDEC_AAI_WORD_PROGRAM,
	};
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_AAI_WORD_PROGRAM_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_AAI_WORD_PROGRAM,
					(start >> 16) & 0xff,
					(start >> 8) & 0xff,
					(start & 0xff),
					buf[0],
					buf[1]
				},
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	switch (flash->mst->spi.type) {
#if CONFIG_INTERNAL == 1
#if defined(__i386__) || defined(__x86_64__)
	case SPI_CONTROLLER_IT87XX:
	case SPI_CONTROLLER_WBSIO:
		msg_perr("%s: impossible with this SPI controller,"
				" degrading to byte program\n", __func__);
		return spi_chip_write_1(flash, buf, start, len);
#endif
#endif
	default:
		break;
	}

	/* The even start address and even length requirements can be either
	 * honored outside this function, or we can call spi_byte_program
	 * for the first and/or last byte and use AAI for the rest.
	 * FIXME: Move this to generic code.
	 */
	/* The data sheet requires a start address with the low bit cleared. */
	if (start % 2) {
		msg_cerr("%s: start address not even! Please report a bug at "
			 "flashrom@flashrom.org\n", __func__);
		if (spi_chip_write_1(flash, buf, start, start % 2))
			return SPI_GENERIC_ERROR;
		pos += start % 2;
		cmds[1].writearr = (const unsigned char[]){
					JEDEC_AAI_WORD_PROGRAM,
					(pos >> 16) & 0xff,
					(pos >> 8) & 0xff,
					(pos & 0xff),
					buf[pos - start],
					buf[pos - start + 1]
				};
		/* Do not return an error for now. */
		//return SPI_GENERIC_ERROR;
	}
	/* The data sheet requires total AAI write length to be even. */
	if (len % 2) {
		msg_cerr("%s: total write length not even! Please report a "
			 "bug at flashrom@flashrom.org\n", __func__);
		/* Do not return an error for now. */
		//return SPI_GENERIC_ERROR;
	}


	result = spi_send_multicommand(flash, cmds);
	if (result != 0) {
		msg_cerr("%s failed during start command execution: %d\n", __func__, result);
		goto bailout;
	}
	while (spi_read_status_register(flash) & SPI_SR_WIP)
		programmer_delay(10);

	/* We already wrote 2 bytes in the multicommand step. */
	pos += 2;

	/* Are there at least two more bytes to write? */
	while (pos < start + len - 1) {
		cmd[1] = buf[pos++ - start];
		cmd[2] = buf[pos++ - start];
		result = spi_send_command(flash, JEDEC_AAI_WORD_PROGRAM_CONT_OUTSIZE, 0, cmd, NULL);
		if (result != 0) {
			msg_cerr("%s failed during followup AAI command execution: %d\n", __func__, result);
			goto bailout;
		}
		while (spi_read_status_register(flash) & SPI_SR_WIP)
			programmer_delay(10);
	}

	/* Use WRDI to exit AAI mode. This needs to be done before issuing any other non-AAI command. */
	result = spi_write_disable(flash);
	if (result != 0) {
		msg_cerr("%s failed to disable AAI mode.\n", __func__);
		return SPI_GENERIC_ERROR;
	}

	/* Write remaining byte (if any). */
	if (pos < start + len) {
		if (spi_chip_write_1(flash, buf + pos - start, pos, pos % 2))
			return SPI_GENERIC_ERROR;
		pos += pos % 2;
	}

	return 0;

bailout:
	result = spi_write_disable(flash);
	if (result != 0)
		msg_cerr("%s failed to disable AAI mode.\n", __func__);
	return SPI_GENERIC_ERROR;
}
