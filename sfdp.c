/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2011-2012 Stefan Tauner
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "flash.h"
#include "spi.h"
#include "chipdrivers.h"

static int spi_sfdp_read_sfdp_chunk(struct flashctx *flash, uint32_t address, uint8_t *buf, int len)
{
	int i, ret;
	uint8_t *newbuf;
	const unsigned char cmd[JEDEC_SFDP_OUTSIZE] = {
		JEDEC_SFDP,
		(address >> 16) & 0xff,
		(address >> 8) & 0xff,
		(address >> 0) & 0xff,
		/* FIXME: the following dummy byte explodes on some programmers.
		 * One workaround is to read the dummy byte
		 * instead and discard its value.
		 */
		0
	};
	msg_cspew("%s: addr=0x%"PRIx32", len=%d, data:\n", __func__, address, len);
	newbuf = malloc(len + 1);
	if (!newbuf)
		return SPI_PROGRAMMER_ERROR;
	ret = spi_send_command(flash, sizeof(cmd) - 1, len + 1, cmd, newbuf);
	memmove(buf, newbuf + 1, len);
	free(newbuf);
	if (ret)
		return ret;
	for (i = 0; i < len; i++)
		msg_cspew(" 0x%02x", buf[i]);
	msg_cspew("\n");
	return 0;
}

static int spi_sfdp_read_sfdp(struct flashctx *flash, uint32_t address, uint8_t *buf, int len)
{
	/* FIXME: There are different upper bounds for the number of bytes to
	 * read on the various programmers (even depending on the rest of the
	 * structure of the transaction). 2 is a safe bet. */
	int maxstep = 2;
	int ret = 0;
	while (len > 0) {
		int step = min(len, maxstep);
		ret = spi_sfdp_read_sfdp_chunk(flash, address, buf, step);
		if (ret)
			return ret;
		address += step;
		buf += step;
		len -= step;
	}
	return ret;
}

struct sfdp_tbl_hdr {
	uint8_t id;
	uint8_t v_minor;
	uint8_t v_major;
	uint8_t len;
	uint32_t ptp; /* 24b pointer */
};

static int sfdp_add_uniform_eraser(struct flashchip *chip, uint8_t opcode, uint32_t block_size)
{
	int i;
	uint32_t total_size = chip->total_size * 1024;
	enum block_erase_func erasefn = spi25_get_erasefn_from_opcode(opcode);

	if (erasefn == NO_BLOCK_ERASE_FUNC || total_size == 0 || block_size == 0 ||
	    total_size % block_size != 0) {
		msg_cdbg("%s: invalid input, please report to "
			 "flashrom@flashrom.org\n", __func__);
		return 1;
	}

	for (i = 0; i < NUM_ERASEFUNCTIONS; i++) {
		struct block_eraser *eraser = &chip->block_erasers[i];
		/* Check for duplicates (including (some) non-uniform ones). */
		if (eraser->eraseblocks[0].size == block_size &&
		    eraser->block_erase == erasefn) {
			msg_cdbg2("  Tried to add a duplicate block eraser: "
				  "%"PRId32" x %"PRId32" B with opcode 0x%02x.\n",
				  total_size/block_size, block_size, opcode);
			return 1;
		}
		if (eraser->eraseblocks[0].size != 0 ||
		    eraser->block_erase != NO_BLOCK_ERASE_FUNC) {
			msg_cspew("  Block Eraser %d is already occupied.\n",
				  i);
			continue;
		}

		eraser->block_erase = erasefn;
		eraser->eraseblocks[0].size = block_size;
		eraser->eraseblocks[0].count = total_size/block_size;
		msg_cdbg2("  Block eraser %d: %"PRId32" x %"PRId32" B with opcode "
			  "0x%02x\n", i, total_size/block_size, block_size,
			  opcode);
		return 0;
	}
	msg_cinfo("%s: Not enough space to store another eraser (i=%d)."
		  " Please report this at flashrom@flashrom.org\n",
		  __func__, i);
	return 1;
}

static int sfdp_fill_flash(struct flashchip *chip, uint8_t *buf, uint16_t len)
{
	uint8_t opcode_4k_erase = 0xFF;
	uint32_t tmp32;
	uint8_t tmp8;
	uint32_t total_size; /* in bytes */
	uint32_t block_size;
	int j;

	msg_cdbg("Parsing JEDEC flash parameter table... ");
	msg_cdbg2("\n");

	/* 1. double word */
	tmp32 =  ((unsigned int)buf[(4 * 0) + 0]);
	tmp32 |= ((unsigned int)buf[(4 * 0) + 1]) << 8;
	tmp32 |= ((unsigned int)buf[(4 * 0) + 2]) << 16;
	tmp32 |= ((unsigned int)buf[(4 * 0) + 3]) << 24;

	tmp8 = (tmp32 >> 17) & 0x3;
	switch (tmp8) {
	case 0x0:
		msg_cdbg2("  3-Byte only addressing.\n");
		break;
	case 0x1:
		msg_cdbg2("  3-Byte (and optionally 4-Byte) addressing.\n");
		break;
	case 0x2:
		msg_cdbg("  4-Byte only addressing (not supported by "
			 "flashrom).\n");
		return 1;
	default:
		msg_cdbg("  Required addressing mode (0x%x) not supported.\n",
			 tmp8);
		return 1;
	}

	msg_cdbg2("  Status register is ");
	if (tmp32 & (1 << 3)) {
		msg_cdbg2("volatile and writes to the status register have to "
			  "be enabled with ");
		if (tmp32 & (1 << 4)) {
			chip->feature_bits = FEATURE_WRSR_WREN;
			msg_cdbg2("WREN (0x06).\n");
		} else {
			chip->feature_bits = FEATURE_WRSR_EWSR;
			msg_cdbg2("EWSR (0x50).\n");
		}
	} else {
		msg_cdbg2("non-volatile and the standard does not allow "
			  "vendors to tell us whether EWSR/WREN is needed for "
			  "status register writes - assuming EWSR.\n");
			chip->feature_bits = FEATURE_WRSR_EWSR;
		}

	msg_cdbg2("  Write chunk size is ");
	if (tmp32 & (1 << 2)) {
		msg_cdbg2("at least 64 B.\n");
		chip->page_size = 64;
		chip->write = SPI_CHIP_WRITE256;
	} else {
		msg_cdbg2("1 B only.\n");
		chip->page_size = 256;
		chip->write = SPI_CHIP_WRITE1;
	}

	if ((tmp32 & 0x3) == 0x1) {
		opcode_4k_erase = (tmp32 >> 8) & 0xFF;
		msg_cspew("  4kB erase opcode is 0x%02x.\n", opcode_4k_erase);
		/* add the eraser later, because we don't know total_size yet */
	} else
		msg_cspew("  4kB erase opcode is not defined.\n");

	/* 2. double word */
	tmp32 =  ((unsigned int)buf[(4 * 1) + 0]);
	tmp32 |= ((unsigned int)buf[(4 * 1) + 1]) << 8;
	tmp32 |= ((unsigned int)buf[(4 * 1) + 2]) << 16;
	tmp32 |= ((unsigned int)buf[(4 * 1) + 3]) << 24;

	if (tmp32 & (1 << 31)) {
		msg_cdbg("Flash chip size >= 4 Gb/512 MB not supported.\n");
		return 1;
	}
	total_size = ((tmp32 & 0x7FFFFFFF) + 1) / 8;
	chip->total_size = total_size / 1024;
	msg_cdbg2("  Flash chip size is %d kB.\n", chip->total_size);
	if (total_size > (1 << 24)) {
		msg_cdbg("Flash chip size is bigger than what 3-Byte addressing "
			 "can access.\n");
		return 1;
	}

	if (opcode_4k_erase != 0xFF)
		sfdp_add_uniform_eraser(chip, opcode_4k_erase, 4 * 1024);

	/* FIXME: double words 3-7 contain unused fast read information */

	if (len == 4 * 4) {
		msg_cdbg("  It seems like this chip supports the preliminary "
			 "Intel version of SFDP, skipping processing of double "
			 "words 3-9.\n");
		goto done;
	}

	/* 8. double word */
	for (j = 0; j < 4; j++) {
		/* 7 double words from the start + 2 bytes for every eraser */
		tmp8 = buf[(4 * 7) + (j * 2)];
		msg_cspew("   Erase Sector Type %d Size: 0x%02x\n", j + 1,
			  tmp8);
		if (tmp8 == 0) {
			msg_cspew("  Erase Sector Type %d is unused.\n", j);
			continue;
		}
		if (tmp8 >= 31) {
			msg_cdbg2("  Block size of erase Sector Type %d (2^%d) "
				 "is too big for flashrom.\n", j, tmp8);
			continue;
		}
		block_size = 1 << (tmp8); /* block_size = 2 ^ field */

		tmp8 = buf[(4 * 7) + (j * 2) + 1];
		msg_cspew("   Erase Sector Type %d Opcode: 0x%02x\n", j + 1,
			  tmp8);
		sfdp_add_uniform_eraser(chip, tmp8, block_size);
	}

done:
	msg_cdbg("done.\n");
	return 0;
}

int probe_spi_sfdp(struct flashctx *flash)
{
	int ret = 0;
	uint8_t buf[8];
	uint32_t tmp32;
	uint8_t nph;
	/* need to limit the table loop by comparing i to uint8_t nph hence: */
	uint16_t i;
	struct sfdp_tbl_hdr *hdrs;
	uint8_t *hbuf;
	uint8_t *tbuf;

	if (spi_sfdp_read_sfdp(flash, 0x00, buf, 4)) {
		msg_cdbg("Receiving SFDP signature failed.\n");
		return 0;
	}
	tmp32 = buf[0];
	tmp32 |= ((unsigned int)buf[1]) << 8;
	tmp32 |= ((unsigned int)buf[2]) << 16;
	tmp32 |= ((unsigned int)buf[3]) << 24;

	if (tmp32 != 0x50444653) {
		msg_cdbg2("Signature = 0x%08"PRIx32" (should be 0x50444653)\n", tmp32);
		msg_cdbg("No SFDP signature found.\n");
		return 0;
	}

	if (spi_sfdp_read_sfdp(flash, 0x04, buf, 3)) {
		msg_cdbg("Receiving SFDP revision and number of parameter "
			 "headers (NPH) failed. ");
		return 0;
	}
	msg_cdbg2("SFDP revision = %d.%d\n", buf[1], buf[0]);
	if (buf[1] != 0x01) {
		msg_cdbg("The chip supports an unknown version of SFDP. "
			  "Aborting SFDP probe!\n");
		return 0;
	}
	nph = buf[2];
	msg_cdbg2("SFDP number of parameter headers is %d (NPH = %d).\n",
		  nph + 1, nph);

	/* Fetch all parameter headers, even if we don't use them all (yet). */
	hbuf = malloc((nph + 1) * 8);
	hdrs = malloc((nph + 1) * sizeof(*hdrs));
	if (hbuf == NULL || hdrs == NULL ) {
		msg_gerr("Out of memory!\n");
		goto cleanup_hdrs;
	}
	if (spi_sfdp_read_sfdp(flash, 0x08, hbuf, (nph + 1) * 8)) {
		msg_cdbg("Receiving SFDP parameter table headers failed.\n");
		goto cleanup_hdrs;
	}

	for (i = 0; i <= nph; i++) {
		uint16_t len;
		hdrs[i].id = hbuf[(8 * i) + 0];
		hdrs[i].v_minor = hbuf[(8 * i) + 1];
		hdrs[i].v_major = hbuf[(8 * i) + 2];
		hdrs[i].len = hbuf[(8 * i) + 3];
		hdrs[i].ptp = hbuf[(8 * i) + 4];
		hdrs[i].ptp |= ((unsigned int)hbuf[(8 * i) + 5]) << 8;
		hdrs[i].ptp |= ((unsigned int)hbuf[(8 * i) + 6]) << 16;
		msg_cdbg2("\nSFDP parameter table header %d/%d:\n", i, nph);
		msg_cdbg2("  ID 0x%02x, version %d.%d\n", hdrs[i].id,
			  hdrs[i].v_major, hdrs[i].v_minor);
		len = hdrs[i].len * 4;
		tmp32 = hdrs[i].ptp;
		msg_cdbg2("  Length %d B, Parameter Table Pointer 0x%06"PRIx32"\n",
			  len, tmp32);

		if (tmp32 + len >= (1 << 24)) {
			msg_cdbg("SFDP Parameter Table %d supposedly overflows "
				  "addressable SFDP area. This most\nprobably "
				  "indicates a corrupt SFDP parameter table "
				  "header. Skipping it.\n", i);
			continue;
		}

		tbuf = malloc(len);
		if (tbuf == NULL) {
			msg_gerr("Out of memory!\n");
			goto cleanup_hdrs;
		}
		if (spi_sfdp_read_sfdp(flash, tmp32, tbuf, len)){
			msg_cdbg("Fetching SFDP parameter table %d failed.\n",
				 i);
			free(tbuf);
			continue;
		}
		msg_cspew("  Parameter table contents:\n");
		for (tmp32 = 0; tmp32 < len; tmp32++) {
			if ((tmp32 % 8) == 0) {
				msg_cspew("    0x%04"PRIx32": ", tmp32);
			}
			msg_cspew(" %02x", tbuf[tmp32]);
			if ((tmp32 % 8) == 7) {
				msg_cspew("\n");
				continue;
			}
			if ((tmp32 % 8) == 3) {
				msg_cspew(" ");
				continue;
			}
		}
		msg_cspew("\n");

		if (i == 0) { /* Mandatory JEDEC SFDP parameter table */
			if (hdrs[i].id != 0)
				msg_cdbg("ID of the mandatory JEDEC SFDP "
					 "parameter table is not 0 as demanded "
					 "by JESD216 (warning only).\n");

			if (hdrs[i].v_major != 0x01) {
				msg_cdbg("The chip contains an unknown "
					  "version of the JEDEC flash "
					  "parameters table, skipping it.\n");
			} else if (len != 4 * 4 && len < 9 * 4) {
				msg_cdbg("Length of the mandatory JEDEC SFDP "
					 "parameter table is wrong (%d B), "
					 "skipping it.\n", len);
			} else if (sfdp_fill_flash(flash->chip, tbuf, len) == 0)
				ret = 1;
		}
		free(tbuf);
	}

cleanup_hdrs:
	free(hdrs);
	free(hbuf);
	return ret;
}
