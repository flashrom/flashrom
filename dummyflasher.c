/*
 * This file is part of the flashrom project.
 *
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "flashchips.h"

/* Remove the #define below if you don't want SPI flash chip emulation. */
#define EMULATE_SPI_CHIP 1

#if EMULATE_SPI_CHIP
#define EMULATE_CHIP 1
#include "spi.h"
#endif

#if EMULATE_CHIP
#include <sys/types.h>
#include <sys/stat.h>
#endif

#if EMULATE_CHIP
enum emu_chip {
	EMULATE_NONE,
	EMULATE_ST_M25P10_RES,
	EMULATE_SST_SST25VF040_REMS,
	EMULATE_SST_SST25VF032B,
	EMULATE_MACRONIX_MX25L6436,
	EMULATE_WINBOND_W25Q128FV,
	EMULATE_VARIABLE_SIZE,
};

struct emu_data {
	enum emu_chip emu_chip;
	char *emu_persistent_image;
	unsigned int emu_chip_size;
	int erase_to_zero;
	int emu_modified;	/* is the image modified since reading it? */
	uint8_t emu_status;
	/* If "freq" parameter is passed in from command line, commands will delay
	 * for this period before returning. */
	unsigned long int delay_us;
	unsigned int emu_max_byteprogram_size;
	unsigned int emu_max_aai_size;
	unsigned int emu_jedec_se_size;
	unsigned int emu_jedec_be_52_size;
	unsigned int emu_jedec_be_d8_size;
	unsigned int emu_jedec_ce_60_size;
	unsigned int emu_jedec_ce_c7_size;
	unsigned char spi_blacklist[256];
	unsigned char spi_ignorelist[256];
	unsigned int spi_blacklist_size;
	unsigned int spi_ignorelist_size;

	unsigned int spi_write_256_chunksize;
	uint8_t *flashchip_contents;
};

#if EMULATE_SPI_CHIP
/* A legit complete SFDP table based on the MX25L6436E (rev. 1.8) datasheet. */
static const uint8_t sfdp_table[] = {
	0x53, 0x46, 0x44, 0x50, // @0x00: SFDP signature
	0x00, 0x01, 0x01, 0xFF, // @0x04: revision 1.0, 2 headers
	0x00, 0x00, 0x01, 0x09, // @0x08: JEDEC SFDP header rev. 1.0, 9 DW long
	0x1C, 0x00, 0x00, 0xFF, // @0x0C: PTP0 = 0x1C (instead of 0x30)
	0xC2, 0x00, 0x01, 0x04, // @0x10: Macronix header rev. 1.0, 4 DW long
	0x48, 0x00, 0x00, 0xFF, // @0x14: PTP1 = 0x48 (instead of 0x60)
	0xFF, 0xFF, 0xFF, 0xFF, // @0x18: hole.
	0xE5, 0x20, 0xC9, 0xFF, // @0x1C: SFDP parameter table start
	0xFF, 0xFF, 0xFF, 0x03, // @0x20
	0x00, 0xFF, 0x08, 0x6B, // @0x24
	0x08, 0x3B, 0x00, 0xFF, // @0x28
	0xEE, 0xFF, 0xFF, 0xFF, // @0x2C
	0xFF, 0xFF, 0x00, 0x00, // @0x30
	0xFF, 0xFF, 0x00, 0xFF, // @0x34
	0x0C, 0x20, 0x0F, 0x52, // @0x38
	0x10, 0xD8, 0x00, 0xFF, // @0x3C: SFDP parameter table end
	0xFF, 0xFF, 0xFF, 0xFF, // @0x40: hole.
	0xFF, 0xFF, 0xFF, 0xFF, // @0x44: hole.
	0x00, 0x36, 0x00, 0x27, // @0x48: Macronix parameter table start
	0xF4, 0x4F, 0xFF, 0xFF, // @0x4C
	0xD9, 0xC8, 0xFF, 0xFF, // @0x50
	0xFF, 0xFF, 0xFF, 0xFF, // @0x54: Macronix parameter table end
};

#endif
#endif

void *dummy_map(const char *descr, uintptr_t phys_addr, size_t len)
{
	msg_pspew("%s: Mapping %s, 0x%zx bytes at 0x%0*" PRIxPTR "\n",
		  __func__, descr, len, PRIxPTR_WIDTH, phys_addr);
	return (void *)phys_addr;
}

void dummy_unmap(void *virt_addr, size_t len)
{
	msg_pspew("%s: Unmapping 0x%zx bytes at %p\n", __func__, len, virt_addr);
}

static int dummy_spi_write_256(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	struct emu_data *emu_data = flash->mst->spi.data;
	return spi_write_chunked(flash, buf, start, len,
				 emu_data->spi_write_256_chunksize);
}

static void dummy_chip_writeb(const struct flashctx *flash, uint8_t val, chipaddr addr)
{
	msg_pspew("%s: addr=0x%" PRIxPTR ", val=0x%02x\n", __func__, addr, val);
}

static void dummy_chip_writew(const struct flashctx *flash, uint16_t val, chipaddr addr)
{
	msg_pspew("%s: addr=0x%" PRIxPTR ", val=0x%04x\n", __func__, addr, val);
}

static void dummy_chip_writel(const struct flashctx *flash, uint32_t val, chipaddr addr)
{
	msg_pspew("%s: addr=0x%" PRIxPTR ", val=0x%08x\n", __func__, addr, val);
}

static void dummy_chip_writen(const struct flashctx *flash, const uint8_t *buf, chipaddr addr, size_t len)
{
	size_t i;
	msg_pspew("%s: addr=0x%" PRIxPTR ", len=0x%zx, writing data (hex):", __func__, addr, len);
	for (i = 0; i < len; i++) {
		if ((i % 16) == 0)
			msg_pspew("\n");
		msg_pspew("%02x ", buf[i]);
	}
}

static uint8_t dummy_chip_readb(const struct flashctx *flash, const chipaddr addr)
{
	msg_pspew("%s:  addr=0x%" PRIxPTR ", returning 0xff\n", __func__, addr);
	return 0xff;
}

static uint16_t dummy_chip_readw(const struct flashctx *flash, const chipaddr addr)
{
	msg_pspew("%s:  addr=0x%" PRIxPTR ", returning 0xffff\n", __func__, addr);
	return 0xffff;
}

static uint32_t dummy_chip_readl(const struct flashctx *flash, const chipaddr addr)
{
	msg_pspew("%s:  addr=0x%" PRIxPTR ", returning 0xffffffff\n", __func__, addr);
	return 0xffffffff;
}

static void dummy_chip_readn(const struct flashctx *flash, uint8_t *buf, const chipaddr addr, size_t len)
{
	msg_pspew("%s:  addr=0x%" PRIxPTR ", len=0x%zx, returning array of 0xff\n", __func__, addr, len);
	memset(buf, 0xff, len);
	return;
}

#if EMULATE_SPI_CHIP
static int emulate_spi_chip_response(unsigned int writecnt,
				     unsigned int readcnt,
				     const unsigned char *writearr,
				     unsigned char *readarr,
				     struct emu_data *data)
{
	unsigned int offs, i, toread;
	static int unsigned aai_offs;
	const unsigned char sst25vf040_rems_response[2] = {0xbf, 0x44};
	const unsigned char sst25vf032b_rems_response[2] = {0xbf, 0x4a};
	const unsigned char mx25l6436_rems_response[2] = {0xc2, 0x16};
	const unsigned char w25q128fv_rems_response[2] = {0xef, 0x17};

	if (writecnt == 0) {
		msg_perr("No command sent to the chip!\n");
		return 1;
	}
	/* spi_blacklist has precedence over spi_ignorelist. */
	for (i = 0; i < data->spi_blacklist_size; i++) {
		if (writearr[0] == data->spi_blacklist[i]) {
			msg_pdbg("Refusing blacklisted SPI command 0x%02x\n",
				 data->spi_blacklist[i]);
			return SPI_INVALID_OPCODE;
		}
	}
	for (i = 0; i < data->spi_ignorelist_size; i++) {
		if (writearr[0] == data->spi_ignorelist[i]) {
			msg_cdbg("Ignoring ignorelisted SPI command 0x%02x\n",
				 data->spi_ignorelist[i]);
			/* Return success because the command does not fail,
			 * it is simply ignored.
			 */
			return 0;
		}
	}

	if (data->emu_max_aai_size && (data->emu_status & SPI_SR_AAI)) {
		if (writearr[0] != JEDEC_AAI_WORD_PROGRAM &&
		    writearr[0] != JEDEC_WRDI &&
		    writearr[0] != JEDEC_RDSR) {
			msg_perr("Forbidden opcode (0x%02x) attempted during "
				 "AAI sequence!\n", writearr[0]);
			return 0;
		}
	}

	switch (writearr[0]) {
	case JEDEC_RES:
		if (writecnt < JEDEC_RES_OUTSIZE)
			break;
		/* offs calculation is only needed for SST chips which treat RES like REMS. */
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		offs += writecnt - JEDEC_REMS_OUTSIZE;
		switch (data->emu_chip) {
		case EMULATE_ST_M25P10_RES:
			if (readcnt > 0)
				memset(readarr, 0x10, readcnt);
			break;
		case EMULATE_SST_SST25VF040_REMS:
			for (i = 0; i < readcnt; i++)
				readarr[i] = sst25vf040_rems_response[(offs + i) % 2];
			break;
		case EMULATE_SST_SST25VF032B:
			for (i = 0; i < readcnt; i++)
				readarr[i] = sst25vf032b_rems_response[(offs + i) % 2];
			break;
		case EMULATE_MACRONIX_MX25L6436:
			if (readcnt > 0)
				memset(readarr, 0x16, readcnt);
			break;
		case EMULATE_WINBOND_W25Q128FV:
			if (readcnt > 0)
				memset(readarr, 0x17, readcnt);
			break;
		default: /* ignore */
			break;
		}
		break;
	case JEDEC_REMS:
		/* REMS response has wraparound and uses an address parameter. */
		if (writecnt < JEDEC_REMS_OUTSIZE)
			break;
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		offs += writecnt - JEDEC_REMS_OUTSIZE;
		switch (data->emu_chip) {
		case EMULATE_SST_SST25VF040_REMS:
			for (i = 0; i < readcnt; i++)
				readarr[i] = sst25vf040_rems_response[(offs + i) % 2];
			break;
		case EMULATE_SST_SST25VF032B:
			for (i = 0; i < readcnt; i++)
				readarr[i] = sst25vf032b_rems_response[(offs + i) % 2];
			break;
		case EMULATE_MACRONIX_MX25L6436:
			for (i = 0; i < readcnt; i++)
				readarr[i] = mx25l6436_rems_response[(offs + i) % 2];
			break;
		case EMULATE_WINBOND_W25Q128FV:
			for (i = 0; i < readcnt; i++)
				readarr[i] = w25q128fv_rems_response[(offs + i) % 2];
			break;
		default: /* ignore */
			break;
		}
		break;
	case JEDEC_RDID:
		switch (data->emu_chip) {
		case EMULATE_SST_SST25VF032B:
			if (readcnt > 0)
				readarr[0] = 0xbf;
			if (readcnt > 1)
				readarr[1] = 0x25;
			if (readcnt > 2)
				readarr[2] = 0x4a;
			break;
		case EMULATE_MACRONIX_MX25L6436:
			if (readcnt > 0)
				readarr[0] = 0xc2;
			if (readcnt > 1)
				readarr[1] = 0x20;
			if (readcnt > 2)
				readarr[2] = 0x17;
			break;
		case EMULATE_WINBOND_W25Q128FV:
			if (readcnt > 0)
				readarr[0] = 0xef;
			if (readcnt > 1)
				readarr[1] = 0x40;
			if (readcnt > 2)
				readarr[2] = 0x18;
			break;
		case EMULATE_VARIABLE_SIZE:
			if (readcnt > 0)
				readarr[0] = (PROGMANUF_ID >> 8) & 0xff;
			if (readcnt > 1)
				readarr[1] = PROGMANUF_ID & 0xff;
			if (readcnt > 2)
				readarr[2] = (PROGDEV_ID >> 8) & 0xff;
			if (readcnt > 3)
				readarr[3] = PROGDEV_ID & 0xff;
			break;
		default: /* ignore */
			break;
		}
		break;
	case JEDEC_RDSR:
		memset(readarr, data->emu_status, readcnt);
		break;
	/* FIXME: this should be chip-specific. */
	case JEDEC_EWSR:
	case JEDEC_WREN:
		data->emu_status |= SPI_SR_WEL;
		break;
	case JEDEC_WRSR:
		if (!(data->emu_status & SPI_SR_WEL)) {
			msg_perr("WRSR attempted, but WEL is 0!\n");
			break;
		}
		/* FIXME: add some reasonable simulation of the busy flag */
		data->emu_status = writearr[1] & ~SPI_SR_WIP;
		msg_pdbg2("WRSR wrote 0x%02x.\n", data->emu_status);
		break;
	case JEDEC_READ:
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		/* Truncate to emu_chip_size. */
		offs %= data->emu_chip_size;
		if (readcnt > 0)
			memcpy(readarr, data->flashchip_contents + offs, readcnt);
		break;
	case JEDEC_READ_4BA:
		offs = writearr[1] << 24 | writearr[2] << 16 | writearr[3] << 8 | writearr[4];
		/* Truncate to emu_chip_size. */
		offs %= data->emu_chip_size;
		if (readcnt > 0)
			memcpy(readarr, data->flashchip_contents + offs, readcnt);
		break;
	case JEDEC_BYTE_PROGRAM:
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		/* Truncate to emu_chip_size. */
		offs %= data->emu_chip_size;
		if (writecnt < 5) {
			msg_perr("BYTE PROGRAM size too short!\n");
			return 1;
		}
		if (writecnt - 4 > data->emu_max_byteprogram_size) {
			msg_perr("Max BYTE PROGRAM size exceeded!\n");
			return 1;
		}
		memcpy(data->flashchip_contents + offs, writearr + 4, writecnt - 4);
		data->emu_modified = 1;
		break;
	case JEDEC_BYTE_PROGRAM_4BA:
		offs = writearr[1] << 24 | writearr[2] << 16 | writearr[3] << 8 | writearr[4];
		/* Truncate to emu_chip_size. */
		offs %= data->emu_chip_size;
		if (writecnt < 6) {
			msg_perr("BYTE PROGRAM size too short!\n");
			return 1;
		}
		if (writecnt - 5 > data->emu_max_byteprogram_size) {
			msg_perr("Max BYTE PROGRAM size exceeded!\n");
			return 1;
		}
		memcpy(data->flashchip_contents + offs, writearr + 5, writecnt - 5);
		data->emu_modified = 1;
		break;
	case JEDEC_AAI_WORD_PROGRAM:
		if (!data->emu_max_aai_size)
			break;
		if (!(data->emu_status & SPI_SR_AAI)) {
			if (writecnt < JEDEC_AAI_WORD_PROGRAM_OUTSIZE) {
				msg_perr("Initial AAI WORD PROGRAM size too "
					 "short!\n");
				return 1;
			}
			if (writecnt > JEDEC_AAI_WORD_PROGRAM_OUTSIZE) {
				msg_perr("Initial AAI WORD PROGRAM size too "
					 "long!\n");
				return 1;
			}
			data->emu_status |= SPI_SR_AAI;
			aai_offs = writearr[1] << 16 | writearr[2] << 8 |
				   writearr[3];
			/* Truncate to emu_chip_size. */
			aai_offs %= data->emu_chip_size;
			memcpy(data->flashchip_contents + aai_offs, writearr + 4, 2);
			aai_offs += 2;
		} else {
			if (writecnt < JEDEC_AAI_WORD_PROGRAM_CONT_OUTSIZE) {
				msg_perr("Continuation AAI WORD PROGRAM size "
					 "too short!\n");
				return 1;
			}
			if (writecnt > JEDEC_AAI_WORD_PROGRAM_CONT_OUTSIZE) {
				msg_perr("Continuation AAI WORD PROGRAM size "
					 "too long!\n");
				return 1;
			}
			memcpy(data->flashchip_contents + aai_offs, writearr + 1, 2);
			aai_offs += 2;
		}
		data->emu_modified = 1;
		break;
	case JEDEC_WRDI:
		if (data->emu_max_aai_size)
			data->emu_status &= ~SPI_SR_AAI;
		break;
	case JEDEC_SE:
		if (!data->emu_jedec_se_size)
			break;
		if (writecnt != JEDEC_SE_OUTSIZE) {
			msg_perr("SECTOR ERASE 0x20 outsize invalid!\n");
			return 1;
		}
		if (readcnt != JEDEC_SE_INSIZE) {
			msg_perr("SECTOR ERASE 0x20 insize invalid!\n");
			return 1;
		}
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		if (offs & (data->emu_jedec_se_size - 1))
			msg_pdbg("Unaligned SECTOR ERASE 0x20: 0x%x\n", offs);
		offs &= ~(data->emu_jedec_se_size - 1);
		memset(data->flashchip_contents + offs, 0xff, data->emu_jedec_se_size);
		data->emu_modified = 1;
		break;
	case JEDEC_BE_52:
		if (!data->emu_jedec_be_52_size)
			break;
		if (writecnt != JEDEC_BE_52_OUTSIZE) {
			msg_perr("BLOCK ERASE 0x52 outsize invalid!\n");
			return 1;
		}
		if (readcnt != JEDEC_BE_52_INSIZE) {
			msg_perr("BLOCK ERASE 0x52 insize invalid!\n");
			return 1;
		}
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		if (offs & (data->emu_jedec_be_52_size - 1))
			msg_pdbg("Unaligned BLOCK ERASE 0x52: 0x%x\n", offs);
		offs &= ~(data->emu_jedec_be_52_size - 1);
		memset(data->flashchip_contents + offs, 0xff, data->emu_jedec_be_52_size);
		data->emu_modified = 1;
		break;
	case JEDEC_BE_D8:
		if (!data->emu_jedec_be_d8_size)
			break;
		if (writecnt != JEDEC_BE_D8_OUTSIZE) {
			msg_perr("BLOCK ERASE 0xd8 outsize invalid!\n");
			return 1;
		}
		if (readcnt != JEDEC_BE_D8_INSIZE) {
			msg_perr("BLOCK ERASE 0xd8 insize invalid!\n");
			return 1;
		}
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];
		if (offs & (data->emu_jedec_be_d8_size - 1))
			msg_pdbg("Unaligned BLOCK ERASE 0xd8: 0x%x\n", offs);
		offs &= ~(data->emu_jedec_be_d8_size - 1);
		memset(data->flashchip_contents + offs, 0xff, data->emu_jedec_be_d8_size);
		data->emu_modified = 1;
		break;
	case JEDEC_CE_60:
		if (!data->emu_jedec_ce_60_size)
			break;
		if (writecnt != JEDEC_CE_60_OUTSIZE) {
			msg_perr("CHIP ERASE 0x60 outsize invalid!\n");
			return 1;
		}
		if (readcnt != JEDEC_CE_60_INSIZE) {
			msg_perr("CHIP ERASE 0x60 insize invalid!\n");
			return 1;
		}
		/* JEDEC_CE_60_OUTSIZE is 1 (no address) -> no offset. */
		/* emu_jedec_ce_60_size is emu_chip_size. */
		memset(data->flashchip_contents, 0xff, data->emu_jedec_ce_60_size);
		data->emu_modified = 1;
		break;
	case JEDEC_CE_C7:
		if (!data->emu_jedec_ce_c7_size)
			break;
		if (writecnt != JEDEC_CE_C7_OUTSIZE) {
			msg_perr("CHIP ERASE 0xc7 outsize invalid!\n");
			return 1;
		}
		if (readcnt != JEDEC_CE_C7_INSIZE) {
			msg_perr("CHIP ERASE 0xc7 insize invalid!\n");
			return 1;
		}
		/* JEDEC_CE_C7_OUTSIZE is 1 (no address) -> no offset. */
		/* emu_jedec_ce_c7_size is emu_chip_size. */
		memset(data->flashchip_contents, 0xff, data->emu_jedec_ce_c7_size);
		data->emu_modified = 1;
		break;
	case JEDEC_SFDP:
		if (data->emu_chip != EMULATE_MACRONIX_MX25L6436)
			break;
		if (writecnt < 4)
			break;
		offs = writearr[1] << 16 | writearr[2] << 8 | writearr[3];

		/* SFDP expects one dummy byte after the address. */
		if (writecnt == 4) {
			/* The dummy byte was not written, make sure it is read instead.
			 * Shifting and shortening the read array does achieve this goal.
			 */
			readarr++;
			readcnt--;
		} else {
			/* The response is shifted if more than 5 bytes are written, because SFDP data is
			 * already shifted out by the chip while those superfluous bytes are written. */
			offs += writecnt - 5;
		}

		/* The SFDP spec implies that the start address of an SFDP read may be truncated to fit in the
		 * SFDP table address space, i.e. the start address may be wrapped around at SFDP table size.
		 * This is a reasonable implementation choice in hardware because it saves a few gates. */
		if (offs >= sizeof(sfdp_table)) {
			msg_pdbg("Wrapping the start address around the SFDP table boundary (using 0x%x "
				 "instead of 0x%x).\n", (unsigned int)(offs % sizeof(sfdp_table)), offs);
			offs %= sizeof(sfdp_table);
		}
		toread = min(sizeof(sfdp_table) - offs, readcnt);
		memcpy(readarr, sfdp_table + offs, toread);
		if (toread < readcnt)
			msg_pdbg("Crossing the SFDP table boundary in a single "
				 "continuous chunk produces undefined results "
				 "after that point.\n");
		break;
	default:
		/* No special response. */
		break;
	}
	if (writearr[0] != JEDEC_WREN && writearr[0] != JEDEC_EWSR)
		data->emu_status &= ~SPI_SR_WEL;
	return 0;
}
#endif

static int dummy_spi_send_command(const struct flashctx *flash, unsigned int writecnt,
				  unsigned int readcnt,
				  const unsigned char *writearr,
				  unsigned char *readarr)
{
	unsigned int i;
	struct emu_data *emu_data = flash->mst->spi.data;
	if (!emu_data) {
		msg_perr("No data in flash context!\n");
		return 1;
	}

	msg_pspew("%s:", __func__);

	msg_pspew(" writing %u bytes:", writecnt);
	for (i = 0; i < writecnt; i++)
		msg_pspew(" 0x%02x", writearr[i]);

	/* Response for unknown commands and missing chip is 0xff. */
	memset(readarr, 0xff, readcnt);
#if EMULATE_SPI_CHIP
	switch (emu_data->emu_chip) {
	case EMULATE_ST_M25P10_RES:
	case EMULATE_SST_SST25VF040_REMS:
	case EMULATE_SST_SST25VF032B:
	case EMULATE_MACRONIX_MX25L6436:
	case EMULATE_WINBOND_W25Q128FV:
	case EMULATE_VARIABLE_SIZE:
		if (emulate_spi_chip_response(writecnt, readcnt, writearr,
					      readarr, emu_data)) {
			msg_pdbg("Invalid command sent to flash chip!\n");
			return 1;
		}
		break;
	default:
		break;
	}
#endif
	msg_pspew(" reading %u bytes:", readcnt);
	for (i = 0; i < readcnt; i++)
		msg_pspew(" 0x%02x", readarr[i]);
	msg_pspew("\n");

	programmer_delay((writecnt + readcnt) * emu_data->delay_us);
	return 0;
}



static const struct spi_master spi_master_dummyflasher = {
	.features	= SPI_MASTER_4BA,
	.max_data_read	= MAX_DATA_READ_UNLIMITED,
	.max_data_write	= MAX_DATA_UNSPECIFIED,
	.command	= dummy_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= default_spi_read,
	.write_256	= dummy_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

static const struct par_master par_master_dummyflasher = {
		.chip_readb		= dummy_chip_readb,
		.chip_readw		= dummy_chip_readw,
		.chip_readl		= dummy_chip_readl,
		.chip_readn		= dummy_chip_readn,
		.chip_writeb		= dummy_chip_writeb,
		.chip_writew		= dummy_chip_writew,
		.chip_writel		= dummy_chip_writel,
		.chip_writen		= dummy_chip_writen,
};

static int dummy_shutdown(void *data)
{
	msg_pspew("%s\n", __func__);
#if EMULATE_CHIP
	struct emu_data *emu_data = (struct emu_data *)data;
	if (emu_data->emu_chip != EMULATE_NONE) {
		if (emu_data->emu_persistent_image && emu_data->emu_modified) {
			msg_pdbg("Writing %s\n", emu_data->emu_persistent_image);
			write_buf_to_file(emu_data->flashchip_contents,
					  emu_data->emu_chip_size,
					  emu_data->emu_persistent_image);
		}
		free(emu_data->emu_persistent_image);
		free(emu_data->flashchip_contents);
	}
#endif
	free(data);
	return 0;
}

int dummy_init(void)
{
	char *bustext = NULL;
	char *tmp = NULL;
	unsigned int i;
#if EMULATE_SPI_CHIP
	char *status = NULL;
	int size = -1;  /* size for VARIABLE_SIZE chip device */
#endif
#if EMULATE_CHIP
	struct stat image_stat;
#endif
	char *endptr;

	struct emu_data *data = calloc(1, sizeof(struct emu_data));
	if (!data) {
		msg_perr("Out of memory!\n");
		return 1;
	}
	data->emu_chip = EMULATE_NONE;
	data->delay_us = 0;
	data->spi_write_256_chunksize = 256;

	msg_pspew("%s\n", __func__);

	bustext = extract_programmer_param("bus");
	msg_pdbg("Requested buses are: %s\n", bustext ? bustext : "default");
	if (!bustext)
		bustext = strdup("parallel+lpc+fwh+spi");
	/* Convert the parameters to lowercase. */
	tolower_string(bustext);

	enum chipbustype dummy_buses_supported = BUS_NONE;
	if (strstr(bustext, "parallel")) {
		dummy_buses_supported |= BUS_PARALLEL;
		msg_pdbg("Enabling support for %s flash.\n", "parallel");
	}
	if (strstr(bustext, "lpc")) {
		dummy_buses_supported |= BUS_LPC;
		msg_pdbg("Enabling support for %s flash.\n", "LPC");
	}
	if (strstr(bustext, "fwh")) {
		dummy_buses_supported |= BUS_FWH;
		msg_pdbg("Enabling support for %s flash.\n", "FWH");
	}
	if (strstr(bustext, "spi")) {
		dummy_buses_supported |= BUS_SPI;
		msg_pdbg("Enabling support for %s flash.\n", "SPI");
	}
	if (dummy_buses_supported == BUS_NONE)
		msg_pdbg("Support for all flash bus types disabled.\n");
	free(bustext);

	tmp = extract_programmer_param("spi_write_256_chunksize");
	if (tmp) {
		data->spi_write_256_chunksize = strtoul(tmp, &endptr, 0);
		if (*endptr != '\0' || data->spi_write_256_chunksize < 1) {
			msg_perr("invalid spi_write_256_chunksize\n");
			free(tmp);
			return 1;
		}
	}
	free(tmp);

	tmp = extract_programmer_param("spi_blacklist");
	if (tmp) {
		i = strlen(tmp);
		if (!strncmp(tmp, "0x", 2)) {
			i -= 2;
			memmove(tmp, tmp + 2, i + 1);
		}
		if ((i > 512) || (i % 2)) {
			msg_perr("Invalid SPI command blacklist length\n");
			free(tmp);
			return 1;
		}
		data->spi_blacklist_size = i / 2;
		for (i = 0; i < data->spi_blacklist_size * 2; i++) {
			if (!isxdigit((unsigned char)tmp[i])) {
				msg_perr("Invalid char \"%c\" in SPI command "
					 "blacklist\n", tmp[i]);
				free(tmp);
				return 1;
			}
		}
		for (i = 0; i < data->spi_blacklist_size; i++) {
			unsigned int tmp2;
			/* SCNx8 is apparently not supported by MSVC (and thus
			 * MinGW), so work around it with an extra variable
			 */
			sscanf(tmp + i * 2, "%2x", &tmp2);
			data->spi_blacklist[i] = (uint8_t)tmp2;
		}
		msg_pdbg("SPI blacklist is ");
		for (i = 0; i < data->spi_blacklist_size; i++)
			msg_pdbg("%02x ", data->spi_blacklist[i]);
		msg_pdbg(", size %u\n", data->spi_blacklist_size);
	}
	free(tmp);

	tmp = extract_programmer_param("spi_ignorelist");
	if (tmp) {
		i = strlen(tmp);
		if (!strncmp(tmp, "0x", 2)) {
			i -= 2;
			memmove(tmp, tmp + 2, i + 1);
		}
		if ((i > 512) || (i % 2)) {
			msg_perr("Invalid SPI command ignorelist length\n");
			free(tmp);
			return 1;
		}
		data->spi_ignorelist_size = i / 2;
		for (i = 0; i < data->spi_ignorelist_size * 2; i++) {
			if (!isxdigit((unsigned char)tmp[i])) {
				msg_perr("Invalid char \"%c\" in SPI command "
					 "ignorelist\n", tmp[i]);
				free(tmp);
				return 1;
			}
		}
		for (i = 0; i < data->spi_ignorelist_size; i++) {
			unsigned int tmp2;
			/* SCNx8 is apparently not supported by MSVC (and thus
			 * MinGW), so work around it with an extra variable
			 */
			sscanf(tmp + i * 2, "%2x", &tmp2);
			data->spi_ignorelist[i] = (uint8_t)tmp2;
		}
		msg_pdbg("SPI ignorelist is ");
		for (i = 0; i < data->spi_ignorelist_size; i++)
			msg_pdbg("%02x ", data->spi_ignorelist[i]);
		msg_pdbg(", size %u\n", data->spi_ignorelist_size);
	}
	free(tmp);

	/* frequency to emulate in Hz (default), KHz, or MHz */
	tmp = extract_programmer_param("freq");
	if (tmp) {
		unsigned long int freq;
		char *units = tmp;
		char *end = tmp + strlen(tmp);

		errno = 0;
		freq = strtoul(tmp, &units, 0);
		if (errno) {
			msg_perr("Invalid frequency \"%s\", %s\n",
					tmp, strerror(errno));
			free(tmp);
			return 1;
		}

		if ((units > tmp) && (units < end)) {
			int units_valid = 0;

			if (units < end - 3) {
				;
			} else if (units == end - 2) {
				if (!strcasecmp(units, "hz"))
					units_valid = 1;
			} else if (units == end - 3) {
				if (!strcasecmp(units, "khz")) {
					freq *= 1000;
					units_valid = 1;
				} else if (!strcasecmp(units, "mhz")) {
					freq *= 1000000;
					units_valid = 1;
				}
			}

			if (!units_valid) {
				msg_perr("Invalid units: %s\n", units);
				free(tmp);
				return 1;
			}
		}

		/* Assume we only work with bytes and transfer at 1 bit/Hz */
		data->delay_us = (1000000 * 8) / freq;
	}
	free(tmp);

#if EMULATE_CHIP
#if EMULATE_SPI_CHIP
	tmp = extract_programmer_param("size");
	if (tmp) {
		size = strtol(tmp, NULL, 10);
		if (size <= 0 || (size % 1024 != 0)) {
			msg_perr("%s: Chip size is not a multiple of 1024: %s\n",
					 __func__, tmp);
			free(tmp);
			return 1;
		}
		free(tmp);
	}
#endif

	tmp = extract_programmer_param("emulate");
	if (!tmp) {
		msg_pdbg("Not emulating any flash chip.\n");
		/* Nothing else to do. */
		goto dummy_init_out;
	}
#if EMULATE_SPI_CHIP
	if (!strcmp(tmp, "M25P10.RES")) {
		data->emu_chip = EMULATE_ST_M25P10_RES;
		data->emu_chip_size = 128 * 1024;
		data->emu_max_byteprogram_size = 128;
		data->emu_max_aai_size = 0;
		data->emu_jedec_se_size = 0;
		data->emu_jedec_be_52_size = 0;
		data->emu_jedec_be_d8_size = 32 * 1024;
		data->emu_jedec_ce_60_size = 0;
		data->emu_jedec_ce_c7_size = data->emu_chip_size;
		msg_pdbg("Emulating ST M25P10.RES SPI flash chip (RES, page "
			 "write)\n");
	}
	if (!strcmp(tmp, "SST25VF040.REMS")) {
		data->emu_chip = EMULATE_SST_SST25VF040_REMS;
		data->emu_chip_size = 512 * 1024;
		data->emu_max_byteprogram_size = 1;
		data->emu_max_aai_size = 0;
		data->emu_jedec_se_size = 4 * 1024;
		data->emu_jedec_be_52_size = 32 * 1024;
		data->emu_jedec_be_d8_size = 0;
		data->emu_jedec_ce_60_size = data->emu_chip_size;
		data->emu_jedec_ce_c7_size = 0;
		msg_pdbg("Emulating SST SST25VF040.REMS SPI flash chip (REMS, "
			 "byte write)\n");
	}
	if (!strcmp(tmp, "SST25VF032B")) {
		data->emu_chip = EMULATE_SST_SST25VF032B;
		data->emu_chip_size = 4 * 1024 * 1024;
		data->emu_max_byteprogram_size = 1;
		data->emu_max_aai_size = 2;
		data->emu_jedec_se_size = 4 * 1024;
		data->emu_jedec_be_52_size = 32 * 1024;
		data->emu_jedec_be_d8_size = 64 * 1024;
		data->emu_jedec_ce_60_size = data->emu_chip_size;
		data->emu_jedec_ce_c7_size = data->emu_chip_size;
		msg_pdbg("Emulating SST SST25VF032B SPI flash chip (RDID, AAI "
			 "write)\n");
	}
	if (!strcmp(tmp, "MX25L6436")) {
		data->emu_chip = EMULATE_MACRONIX_MX25L6436;
		data->emu_chip_size = 8 * 1024 * 1024;
		data->emu_max_byteprogram_size = 256;
		data->emu_max_aai_size = 0;
		data->emu_jedec_se_size = 4 * 1024;
		data->emu_jedec_be_52_size = 32 * 1024;
		data->emu_jedec_be_d8_size = 64 * 1024;
		data->emu_jedec_ce_60_size = data->emu_chip_size;
		data->emu_jedec_ce_c7_size = data->emu_chip_size;
		msg_pdbg("Emulating Macronix MX25L6436 SPI flash chip (RDID, "
			 "SFDP)\n");
	}
	if (!strcmp(tmp, "W25Q128FV")) {
		data->emu_chip = EMULATE_WINBOND_W25Q128FV;
		data->emu_chip_size = 16 * 1024 * 1024;
		data->emu_max_byteprogram_size = 256;
		data->emu_max_aai_size = 0;
		data->emu_jedec_se_size = 4 * 1024;
		data->emu_jedec_be_52_size = 32 * 1024;
		data->emu_jedec_be_d8_size = 64 * 1024;
		data->emu_jedec_ce_60_size = data->emu_chip_size;
		data->emu_jedec_ce_c7_size = data->emu_chip_size;
		msg_pdbg("Emulating Winbond W25Q128FV SPI flash chip (RDID)\n");
	}

	/* The name of variable-size virtual chip. A 4 MiB flash example:
	 *   flashrom -p dummy:emulate=VARIABLE_SIZE,size=4194304
	 */
	if (!strcmp(tmp, "VARIABLE_SIZE")) {
		if (size == -1) {
			msg_perr("%s: the size parameter is not given.\n", __func__);
			free(tmp);
			return 1;
		}
		data->emu_chip = EMULATE_VARIABLE_SIZE;
		data->emu_chip_size = size;
		data->emu_max_byteprogram_size = 256;
		data->emu_max_aai_size = 0;
		data->emu_jedec_se_size = 4 * 1024;
		data->emu_jedec_be_52_size = 32 * 1024;
		data->emu_jedec_be_d8_size = 64 * 1024;
		data->emu_jedec_ce_60_size = data->emu_chip_size;
		data->emu_jedec_ce_c7_size = data->emu_chip_size;
		msg_pdbg("Emulating generic SPI flash chip (size=%d bytes)\n",
		         data->emu_chip_size);
	}
#endif
	if (data->emu_chip == EMULATE_NONE) {
		msg_perr("Invalid chip specified for emulation: %s\n", tmp);
		free(tmp);
		return 1;
	}
	free(tmp);

	/* Should emulated flash erase to zero (yes/no)? */
	tmp = extract_programmer_param("erase_to_zero");
	if (tmp) {
		if (!strcmp(tmp, "yes")) {
			msg_pdbg("Emulated chip will erase to 0x00\n");
			data->erase_to_zero = 1;
		} else if (!strcmp(tmp, "no")) {
			msg_pdbg("Emulated chip will erase to 0xff\n");
		} else {
			msg_perr("erase_to_zero can be \"yes\" or \"no\"\n");
			free(tmp);
			return 1;
		}
	}
	free(tmp);

#ifdef EMULATE_SPI_CHIP
	status = extract_programmer_param("spi_status");
	if (status) {
		errno = 0;
		data->emu_status = strtoul(status, &endptr, 0);
		if (errno != 0 || status == endptr) {
			free(status);
			msg_perr("Error: initial status register specified, "
				 "but the value could not be converted.\n");
			return 1;
		}
		free(status);
		msg_pdbg("Initial status register is set to 0x%02x.\n",
			 data->emu_status);
	}
#endif

	data->flashchip_contents = malloc(data->emu_chip_size);
	if (!data->flashchip_contents) {
		msg_perr("Out of memory!\n");
		return 1;
	}

	msg_pdbg("Filling fake flash chip with 0x%02x, size %i\n",
			data->erase_to_zero ? 0x00 : 0xff, data->emu_chip_size);
	memset(data->flashchip_contents, data->erase_to_zero ? 0x00 : 0xff, data->emu_chip_size);

	/* Will be freed by shutdown function if necessary. */
	data->emu_persistent_image = extract_programmer_param("image");
	if (!data->emu_persistent_image) {
		/* Nothing else to do. */
		goto dummy_init_out;
	}
	/* We will silently (in default verbosity) ignore the file if it does not exist (yet) or the size does
	 * not match the emulated chip. */
	if (!stat(data->emu_persistent_image, &image_stat)) {
		msg_pdbg("Found persistent image %s, %jd B ",
			 data->emu_persistent_image, (intmax_t)image_stat.st_size);
		if ((uintmax_t)image_stat.st_size == data->emu_chip_size) {
			msg_pdbg("matches.\n");
			msg_pdbg("Reading %s\n", data->emu_persistent_image);
			if (read_buf_from_file(data->flashchip_contents, data->emu_chip_size,
					   data->emu_persistent_image)) {
				msg_perr("Unable to read %s\n", data->emu_persistent_image);
				free(data->emu_persistent_image);
				free(data->flashchip_contents);
				return 1;
			}
		} else {
			msg_pdbg("doesn't match.\n");
		}
	}
#endif

dummy_init_out:
	if (register_shutdown(dummy_shutdown, data)) {
		free(data->emu_persistent_image);
		free(data->flashchip_contents);
		free(data);
		return 1;
	}
	if (dummy_buses_supported & BUS_NONSPI)
		register_par_master(&par_master_dummyflasher,
				    dummy_buses_supported & BUS_NONSPI,
				    data);
	if (dummy_buses_supported & BUS_SPI)
		register_spi_master(&spi_master_dummyflasher, data);

	return 0;
}

#if EMULATE_CHIP && EMULATE_SPI_CHIP
int probe_variable_size(struct flashctx *flash)
{
	unsigned int i;
	const struct emu_data *emu_data = flash->mst->spi.data;

	/* Skip the probing if we don't emulate this chip. */
	if (!emu_data || emu_data->emu_chip != EMULATE_VARIABLE_SIZE)
		return 0;

	/*
	 * This will break if one day flashctx becomes read-only.
	 * Once that happens, we need to have special hacks in functions:
	 *
	 *     erase_and_write_flash() in flashrom.c
	 *     read_flash_to_file()
	 *     handle_romentries()
	 *     ...
	 *
	 * Search "total_size * 1024" in code.
	 */
	flash->chip->total_size = emu_data->emu_chip_size / 1024;
	msg_cdbg("%s: set flash->total_size to %dK bytes.\n", __func__,
	         flash->chip->total_size);

	if (emu_data->erase_to_zero)
		flash->chip->feature_bits |= FEATURE_ERASED_ZERO;

	/* Update the first count of each of the block_erasers. */
	for (i = 0; i < NUM_ERASEFUNCTIONS; i++) {
		struct block_eraser *eraser = &flash->chip->block_erasers[i];
		if (!eraser->block_erase)
			break;

		eraser->eraseblocks[0].count = 1;
		eraser->eraseblocks[0].size = emu_data->emu_chip_size;
		msg_cdbg("%s: eraser.size=%d, .count=%d\n",
		         __func__, eraser->eraseblocks[0].size,
		         eraser->eraseblocks[0].count);
	}

	return 1;
}
#endif
