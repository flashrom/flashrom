/*
 * Support for Atmel AT45DB series DataFlash chips.
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2012 Aidan Thornton
 * Copyright (C) 2013 Stefan Tauner
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
#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

/* Status register bits */
#define AT45DB_READY	(1<<7)
#define AT45DB_CMP	(1<<6)
#define AT45DB_PROT	(1<<1)
#define AT45DB_POWEROF2	(1<<0)

/* Opcodes */
#define AT45DB_STATUS 0xD7 /* NB: this is a block erase command on most other chips(!). */
#define AT45DB_DISABLE_PROTECT 0x3D, 0x2A, 0x7F, 0x9A
#define AT45DB_READ_ARRAY 0xE8
#define AT45DB_READ_PROTECT 0x32
#define AT45DB_READ_LOCKDOWN 0x35
#define AT45DB_PAGE_ERASE 0x81
#define AT45DB_BLOCK_ERASE 0x50
#define AT45DB_SECTOR_ERASE 0x7C
#define AT45DB_CHIP_ERASE 0xC7
#define AT45DB_CHIP_ERASE_ADDR 0x94809A /* Magic address. See usage. */
#define AT45DB_BUFFER1_WRITE 0x84
#define AT45DB_BUFFER1_PAGE_PROGRAM 0x88
/* Buffer 2 is unused yet.
#define AT45DB_BUFFER2_WRITE 0x87
#define AT45DB_BUFFER2_PAGE_PROGRAM 0x89
*/

static uint8_t at45db_read_status_register(struct flashctx *flash, uint8_t *status)
{
	static const uint8_t cmd[] = { AT45DB_STATUS };

	int ret = spi_send_command(flash, sizeof(cmd), 1, cmd, status);
	if (ret != 0)
		msg_cerr("Reading the status register failed!\n");
	else
		msg_cspew("Status register: 0x%02x.\n", *status);
	return ret;
}

int spi_disable_blockprotect_at45db(struct flashctx *flash)
{
	static const uint8_t cmd[4] = { AT45DB_DISABLE_PROTECT }; /* NB: 4 bytes magic number */
	int ret = spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
	if (ret != 0) {
		msg_cerr("Sending disable lockdown failed!\n");
		return ret;
	}
	uint8_t status;
	ret = at45db_read_status_register(flash, &status);
	if (ret != 0 || ((status & AT45DB_PROT) != 0)) {
		msg_cerr("Disabling lockdown failed!\n");
		return 1;
	}

	return 0;
}

static unsigned int at45db_get_sector_count(struct flashctx *flash)
{
	unsigned int i, j;
	unsigned int cnt = 0;
	for (i = 0; i < NUM_ERASEFUNCTIONS; i++) {
		const struct block_eraser *const eraser = &flash->chip->block_erasers[i];
		if (eraser->block_erase == SPI_ERASE_AT45DB_SECTOR) {
			for (j = 0; j < NUM_ERASEREGIONS; j++) {
				cnt += eraser->eraseblocks[j].count;
			}
		}
	}
	msg_cspew("%s: number of sectors=%u\n", __func__, cnt);
	return cnt;
}

/* Reads and prettyprints protection/lockdown registers.
 * Some elegance of the printouts had to be cut down a bit to share this code. */
static uint8_t at45db_prettyprint_protection_register(struct flashctx *flash, uint8_t opcode, const char *regname)
{
	const uint8_t cmd[] = { opcode, 0, 0, 0 };
	const size_t sec_count = at45db_get_sector_count(flash);
	if (sec_count < 2)
		return 0;

	/* The first two sectors share the first result byte. */
	uint8_t buf[at45db_get_sector_count(flash) - 1];

	int ret = spi_send_command(flash, sizeof(cmd), sizeof(buf), cmd, buf);
	if (ret != 0) {
		msg_cerr("Reading the %s register failed!\n", regname);
		return ret;
	}

	unsigned int i;
	for (i = 0; i < sizeof(buf); i++) {
		if (buf[i] != 0x00)
			break;
		if (i == sizeof(buf) - 1) {
			msg_cdbg("No Sector is %sed.\n", regname);
			return 0;
		}
	}

	/* TODO: print which addresses are mapped to (un)locked sectors. */
	msg_cdbg("Sector 0a is %s%sed.\n", ((buf[0] & 0xC0) == 0x00) ? "un" : "", regname);
	msg_cdbg("Sector 0b is %s%sed.\n", ((buf[0] & 0x30) == 0x00) ? "un" : "", regname);
	for (i = 1; i < sizeof(buf); i++)
		msg_cdbg("Sector %2u is %s%sed.\n", i, (buf[i] == 0x00) ? "un" : "", regname);

	return 0;
}

/* bit 7: busy flag
 * bit 6: memory/buffer compare result
 * bit 5-2: density (encoding see below)
 * bit 1: protection enabled (soft or hard)
 * bit 0: "power of 2" page size indicator (e.g. 1 means 256B; 0 means 264B)
 *
 * 5-2 encoding: bit 2 is always 1, bits 3-5 encode the density as "2^(bits - 1)" in Mb e.g.:
 * AT45DB161D  1011  16Mb */
int spi_prettyprint_status_register_at45db(struct flashctx *flash)
{
	uint8_t status;
	if (at45db_read_status_register(flash, &status) != 0) {
		return 1;
	}

	/* AT45DB321C does not support lockdown or a page size of a power of 2... */
	const bool isAT45DB321C = (strcmp(flash->chip->name, "AT45DB321C") == 0);
	msg_cdbg("Chip status register is 0x%02x\n", status);
	msg_cdbg("Chip status register: Bit 7 / Ready is %sset\n", (status & AT45DB_READY) ? "" : "not ");
	msg_cdbg("Chip status register: Bit 6 / Compare match is %sset\n", (status & AT45DB_CMP) ? "" : "not ");
	spi_prettyprint_status_register_bit(status, 5);
	spi_prettyprint_status_register_bit(status, 4);
	spi_prettyprint_status_register_bit(status, 3);
	spi_prettyprint_status_register_bit(status, 2);
	const uint8_t dens = (status >> 3) & 0x7; /* Bit 2 is always 1, we use the other bits only */
	msg_cdbg("Chip status register: Density is %u Mb\n", 1 << (dens - 1));
	msg_cdbg("Chip status register: Bit 1 / Protection is %sset\n", (status & AT45DB_PROT) ? "" : "not ");

	if (isAT45DB321C)
		spi_prettyprint_status_register_bit(status, 0);
	else
		msg_cdbg("Chip status register: Bit 0 / \"Power of 2\" is %sset\n",
			 (status & AT45DB_POWEROF2) ? "" : "not ");

	if (status & AT45DB_PROT)
		at45db_prettyprint_protection_register(flash, AT45DB_READ_PROTECT, "protect");

	if (!isAT45DB321C)
		at45db_prettyprint_protection_register(flash, AT45DB_READ_LOCKDOWN, "lock");

	return 0;
}

/* Probe function for AT45DB* chips that support multiple page sizes. */
int probe_spi_at45db(struct flashctx *flash)
{
	uint8_t status;
	struct flashchip *chip = flash->chip;

	if (!probe_spi_rdid(flash))
		return 0;

	/* Some AT45DB* chips support two different page sizes each (e.g. 264 and 256 B). In order to tell which
	 * page size this chip has we need to read the status register. */
	if (at45db_read_status_register(flash, &status) != 0)
		return 0;

	/* We assume sane power-of-2 page sizes and adjust the chip attributes in case this is not the case. */
	if ((status & AT45DB_POWEROF2) == 0) {
		chip->total_size = (chip->total_size / 32) * 33;
		chip->page_size = (chip->page_size / 32) * 33;

		unsigned int i, j;
		for (i = 0; i < NUM_ERASEFUNCTIONS; i++) {
			struct block_eraser *eraser = &chip->block_erasers[i];
			for (j = 0; j < NUM_ERASEREGIONS; j++) {
				eraser->eraseblocks[j].size = (eraser->eraseblocks[j].size / 32) * 33;
			}
		}
	}

	switch (chip->page_size) {
	case 256: chip->gran = WRITE_GRAN_256BYTES; break;
	case 264: chip->gran = WRITE_GRAN_264BYTES; break;
	case 512: chip->gran = WRITE_GRAN_512BYTES; break;
	case 528: chip->gran = WRITE_GRAN_528BYTES; break;
	case 1024: chip->gran = WRITE_GRAN_1024BYTES; break;
	case 1056: chip->gran = WRITE_GRAN_1056BYTES; break;
	default:
		msg_cerr("%s: unknown page size %d.\n", __func__, chip->page_size);
		return 0;
	}

	msg_cdbg2("%s: total size %i kB, page size %i B\n", __func__, chip->total_size * 1024, chip->page_size);

	return 1;
}

/* In case of non-power-of-two page sizes we need to convert the address flashrom uses to the address the
 * DataFlash chips use. The latter uses a segmented address space where the page address is encoded in the
 * more significant bits and the offset within the page is encoded in the less significant bits. The exact
 * partition depends on the page size.
 */
static unsigned int at45db_convert_addr(unsigned int addr, unsigned int page_size)
{
	unsigned int page_bits = address_to_bits(page_size - 1);
	unsigned int at45db_addr = ((addr / page_size) << page_bits) | (addr % page_size);
	msg_cspew("%s: addr=0x%x, page_size=%u, page_bits=%u -> at45db_addr=0x%x\n",
		  __func__, addr, page_size, page_bits, at45db_addr);
	return at45db_addr;
}

int spi_read_at45db(struct flashctx *flash, uint8_t *buf, unsigned int addr, unsigned int len)
{
	const unsigned int page_size = flash->chip->page_size;
	const unsigned int total_size = flash->chip->total_size * 1024;
	if ((addr + len) > total_size) {
		msg_cerr("%s: tried to read beyond flash boundary: addr=%u, len=%u, size=%u\n",
			 __func__, addr, len, total_size);
		return 1;
	}

	/* We have to split this up into chunks to fit within the programmer's read size limit, but those
	 * chunks can cross page boundaries. */
	const unsigned int max_data_read = flash->mst->spi.max_data_read;
	const unsigned int max_chunk = (max_data_read > 0) ? max_data_read : page_size;
	while (len > 0) {
		unsigned int chunk = min(max_chunk, len);
		int ret = spi_nbyte_read(flash, at45db_convert_addr(addr, page_size), buf, chunk);
		if (ret) {
			msg_cerr("%s: error sending read command!\n", __func__);
			return ret;
		}
		addr += chunk;
		buf += chunk;
		len -= chunk;
	}

	return 0;
}

/* Legacy continuous read, used where spi_read_at45db() is not available.
 * The first 4 (dummy) bytes read need to be discarded. */
int spi_read_at45db_e8(struct flashctx *flash, uint8_t *buf, unsigned int addr, unsigned int len)
{
	const unsigned int page_size = flash->chip->page_size;
	const unsigned int total_size = flash->chip->total_size * 1024;
	if ((addr + len) > total_size) {
		msg_cerr("%s: tried to read beyond flash boundary: addr=%u, len=%u, size=%u\n",
			 __func__, addr, len, total_size);
		return 1;
	}

	/* We have to split this up into chunks to fit within the programmer's read size limit, but those
	 * chunks can cross page boundaries. */
	const unsigned int max_data_read = flash->mst->spi.max_data_read;
	const unsigned int max_chunk = (max_data_read > 0) ? max_data_read : page_size;
	while (len > 0) {
		const unsigned int addr_at45 = at45db_convert_addr(addr, page_size);
		const unsigned char cmd[] = {
			AT45DB_READ_ARRAY,
			(addr_at45 >> 16) & 0xff,
			(addr_at45 >> 8) & 0xff,
			(addr_at45 >> 0) & 0xff
		};
		/* We need to leave place for 4 dummy bytes and handle them explicitly. */
		unsigned int chunk = min(max_chunk, len + 4);
		uint8_t tmp[chunk];
		int ret = spi_send_command(flash, sizeof(cmd), chunk, cmd, tmp);
		if (ret) {
			msg_cerr("%s: error sending read command!\n", __func__);
			return ret;
		}
		/* Copy result without dummy bytes into buf and advance address counter respectively. */
		memcpy(buf, tmp + 4, chunk - 4);
		addr += chunk - 4;
		buf += chunk - 4;
		len -= chunk - 4;
	}
	return 0;
}

/* Returns 0 when ready, 1 on errors and timeouts. */
static int at45db_wait_ready (struct flashctx *flash, unsigned int us, unsigned int retries)
{
	while (true) {
		uint8_t status;
		int ret = at45db_read_status_register(flash, &status);
		if ((status & AT45DB_READY) == AT45DB_READY)
			return 0;
		if (ret != 0 || retries-- == 0)
			return 1;
		programmer_delay(flash, us);
	}
}

static int at45db_erase(struct flashctx *flash, uint8_t opcode, unsigned int at45db_addr, unsigned int stepsize, unsigned int retries)
{
	const uint8_t cmd[] = {
		opcode,
		(at45db_addr >> 16) & 0xff,
		(at45db_addr >> 8) & 0xff,
		(at45db_addr >> 0) & 0xff
	};

	/* Send erase command. */
	int ret = spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
	if (ret != 0) {
		msg_cerr("%s: error sending erase command!\n", __func__);
		return ret;
	}

	/* Wait for completion. */
	ret = at45db_wait_ready(flash, stepsize, retries);
	if (ret != 0)
		msg_cerr("%s: chip did not become ready again after sending the erase command!\n", __func__);

	return ret;
}

int spi_erase_at45db_page(struct flashctx *flash, unsigned int addr, unsigned int blocklen)
{
	const unsigned int page_size = flash->chip->page_size;
	const unsigned int total_size = flash->chip->total_size * 1024;

	if ((addr % page_size) != 0 || (blocklen % page_size) != 0) {
		msg_cerr("%s: cannot erase partial pages: addr=%u, blocklen=%u\n", __func__, addr, blocklen);
		return 1;
	}

	if ((addr + blocklen) > total_size) {
		msg_cerr("%s: tried to erase a block beyond flash boundary: addr=%u, blocklen=%u, size=%u\n",
			 __func__, addr, blocklen, total_size);
		return 1;
	}

	/* Needs typically about 35 ms for completion, so let's wait 100 ms in 500 us steps. */
	return at45db_erase(flash, AT45DB_PAGE_ERASE, at45db_convert_addr(addr, page_size), 500, 200);
}

int spi_erase_at45db_block(struct flashctx *flash, unsigned int addr, unsigned int blocklen)
{
	const unsigned int page_size = flash->chip->page_size;
	const unsigned int total_size = flash->chip->total_size * 1024;

	if ((addr % page_size) != 0 || (blocklen % page_size) != 0) { // FIXME: should check blocks not pages
		msg_cerr("%s: cannot erase partial pages: addr=%u, blocklen=%u\n", __func__, addr, blocklen);
		return 1;
	}

	if ((addr + blocklen) > total_size) {
		msg_cerr("%s: tried to erase a block beyond flash boundary: addr=%u, blocklen=%u, size=%u\n",
			 __func__, addr, blocklen, total_size);
		return 1;
	}

	/* Needs typically between 20 and 100 ms for completion, so let's wait 300 ms in 1 ms steps. */
	return at45db_erase(flash, AT45DB_BLOCK_ERASE, at45db_convert_addr(addr, page_size), 1000, 300);
}

int spi_erase_at45db_sector(struct flashctx *flash, unsigned int addr, unsigned int blocklen)
{
	const unsigned int page_size = flash->chip->page_size;
	const unsigned int total_size = flash->chip->total_size * 1024;

	if ((addr % page_size) != 0 || (blocklen % page_size) != 0) { // FIXME: should check sectors not pages
		msg_cerr("%s: cannot erase partial pages: addr=%u, blocklen=%u\n", __func__, addr, blocklen);
		return 1;
	}

	if ((addr + blocklen) > total_size) {
		msg_cerr("%s: tried to erase a sector beyond flash boundary: addr=%u, blocklen=%u, size=%u\n",
			 __func__, addr, blocklen, total_size);
		return 1;
	}

	/* Needs typically about 5 s for completion, so let's wait 20 seconds in 200 ms steps. */
	return at45db_erase(flash, AT45DB_SECTOR_ERASE, at45db_convert_addr(addr, page_size), 200000, 100);
}

int spi_erase_at45db_chip(struct flashctx *flash, unsigned int addr, unsigned int blocklen)
{
	const unsigned int total_size = flash->chip->total_size * 1024;

	if ((addr + blocklen) > total_size) {
		msg_cerr("%s: tried to erase beyond flash boundary: addr=%u, blocklen=%u, size=%u\n",
			 __func__, addr, blocklen, total_size);
		return 1;
	}

	/* Needs typically from about 5 to over 60 s for completion, so let's wait 100 s in 500 ms steps.
	 * NB: the address is not a real address but a magic number. This hack allows to share code. */
	return at45db_erase(flash, AT45DB_CHIP_ERASE, AT45DB_CHIP_ERASE_ADDR, 500000, 200);
}

/* This one is really special and works only for AT45CS1282. It uses two different opcodes depending on the
 * address and has an asymmetric layout. */
int spi_erase_at45cs_sector(struct flashctx *flash, unsigned int addr, unsigned int blocklen)
{
	const unsigned int page_size = flash->chip->page_size;
	const unsigned int total_size = flash->chip->total_size * 1024;
	const struct block_eraser be = flash->chip->block_erasers[0];
	const unsigned int sec_0a_top = be.eraseblocks[0].size;
	const unsigned int sec_0b_top = be.eraseblocks[0].size + be.eraseblocks[1].size;

	if ((addr + blocklen) > total_size) {
		msg_cerr("%s: tried to erase a sector beyond flash boundary: addr=%u, blocklen=%u, size=%u\n",
			 __func__, addr, blocklen, total_size);
		return 1;
	}

	bool partial_range = false;
	uint8_t opcode = 0x7C; /* Used for all but sector 0a. */
	if (addr < sec_0a_top) {
		opcode = 0x50;
		/* One single sector of 8 pages at address 0. */
		if (addr != 0 || blocklen != (8 * page_size))
			partial_range = true;
	} else if (addr < sec_0b_top) {
		/* One single sector of 248 pages adjacent to the first. */
		if (addr != sec_0a_top || blocklen != (248 * page_size))
			partial_range = true;
	} else {
		/* The rest is filled by 63 aligned sectors of 256 pages. */
		if ((addr % (256 * page_size)) != 0 || (blocklen % (256 * page_size)) != 0)
			partial_range = true;
	}
	if (partial_range) {
		msg_cerr("%s: cannot erase partial sectors: addr=%u, blocklen=%u\n", __func__, addr, blocklen);
		return 1;
	}

	/* Needs up to 4 s for completion, so let's wait 20 seconds in 200 ms steps. */
	return at45db_erase(flash, opcode, at45db_convert_addr(addr, page_size), 200000, 100);
}

static int at45db_fill_buffer1(struct flashctx *flash, const uint8_t *bytes, unsigned int off, unsigned int len)
{
	const unsigned int page_size = flash->chip->page_size;
	if ((off + len) > page_size) {
		msg_cerr("Tried to write %u bytes at offset %u into a buffer of only %u B.\n",
			 len, off, page_size);
		return 1;
	}

	/* Create a suitable buffer to store opcode, address and data chunks for buffer1. */
	const unsigned int max_data_write = flash->mst->spi.max_data_write;
	const unsigned int max_chunk = max_data_write > 4 && max_data_write - 4 <= page_size ?
				       max_data_write - 4 : page_size;
	uint8_t buf[4 + max_chunk];

	buf[0] = AT45DB_BUFFER1_WRITE;
	while (off < page_size) {
		unsigned int cur_chunk = min(max_chunk, page_size - off);
		buf[1] = (off >> 16) & 0xff;
		buf[2] = (off >> 8) & 0xff;
		buf[3] = (off >> 0) & 0xff;
		memcpy(&buf[4], bytes + off, cur_chunk);
		int ret = spi_send_command(flash, 4 + cur_chunk, 0, buf, NULL);
		if (ret != 0) {
			msg_cerr("%s: error sending buffer write!\n", __func__);
			return ret;
		}
		off += cur_chunk;
	}
	return 0;
}

static int at45db_commit_buffer1(struct flashctx *flash, unsigned int at45db_addr)
{
	const uint8_t cmd[] = {
		AT45DB_BUFFER1_PAGE_PROGRAM,
		(at45db_addr >> 16) & 0xff,
		(at45db_addr >> 8) & 0xff,
		(at45db_addr >> 0) & 0xff
	};

	/* Send buffer to device. */
	int ret = spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
	if (ret != 0) {
		msg_cerr("%s: error sending buffer to main memory command!\n", __func__);
		return ret;
	}

	/* Wait for completion (typically a few ms). */
	ret = at45db_wait_ready(flash, 250, 200); // 50 ms
	if (ret != 0) {
		msg_cerr("%s: chip did not become ready again!\n", __func__);
		return ret;
	}

	return 0;
}

static int at45db_program_page(struct flashctx *flash, const uint8_t *buf, unsigned int at45db_addr)
{
	int ret = at45db_fill_buffer1(flash, buf, 0, flash->chip->page_size);
	if (ret != 0) {
		msg_cerr("%s: filling the buffer failed!\n", __func__);
		return ret;
	}

	ret = at45db_commit_buffer1(flash, at45db_addr);
	if (ret != 0) {
		msg_cerr("%s: committing page failed!\n", __func__);
		return ret;
	}

	return 0;
}

int spi_write_at45db(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	const unsigned int page_size = flash->chip->page_size;
	const unsigned int total_size = flash->chip->total_size;

	if ((start % page_size) != 0 || (len % page_size) != 0) {
		msg_cerr("%s: cannot write partial pages: start=%u, len=%u\n", __func__, start, len);
		return 1;
	}

	if ((start + len) > (total_size * 1024)) {
		msg_cerr("%s: tried to write beyond flash boundary: start=%u, len=%u, size=%u\n",
			 __func__, start, len, total_size);
		return 1;
	}

	unsigned int i;
	for (i = 0; i < len; i += page_size) {
		if (at45db_program_page(flash, buf + i, at45db_convert_addr(start + i, page_size)) != 0) {
			msg_cerr("Writing page %u failed!\n", i);
			return 1;
		}
		update_progress(flash, FLASHROM_PROGRESS_WRITE, i + page_size, len);
	}
	return 0;
}
