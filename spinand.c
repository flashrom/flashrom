/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2018 Yangfl
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

/*
 * Contains the SPI NAND chip driver functions
 */

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "flash.h"
#include "flashchips.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"
#include "spinand.h"

/***************************************
 * UTIL FUNCTIONS
 ***************************************/

/* For error number */
static int errno;

/* 1 dummy byte between input and output */
#define DUMMY_BYTE 1

static int spi_nand_prepare_row_address(uint8_t cmd_buf[], const unsigned int addr)
{
	cmd_buf[1] = (addr >> 16) & 0xff;
	cmd_buf[2] = (addr >>  8) & 0xff;
	cmd_buf[3] = (addr >>  0) & 0xff;
	return 3;
}

static int spi_nand_prepare_column_address(uint8_t cmd_buf[], const unsigned int addr)
{
	cmd_buf[1] = (addr >>  8) & 0xff;
	cmd_buf[2] = (addr >>  0) & 0xff;
	return 2;
}

/***************************************
 * CHIP FEATURE
 ***************************************/

uint8_t spi_nand_get_feature_multi(struct flashctx *flash, unsigned char addr, int time)
{
	/* Feature will be outputted continuously until CS goes high */
	unsigned char cmd[SPINAND_GET_FEATURE_OUTSIZE] = { SPINAND_GET_FEATURE, addr };
	int ret;
	unsigned char readarr[time];

	if (time > 100) {
		msg_cerr("%s called with too many 'time'! Please report a bug at "
			 "flashrom@flashrom.org\n", __func__);
		errno = SPI_FLASHROM_BUG;
		return 0;
	}

	ret = spi_send_command(flash, sizeof(cmd), sizeof(readarr), cmd, readarr);
	if (ret) {
		msg_cerr("GET FEATURE failed!\n");
		errno = ret;
		return 0;
	}

	msg_cspew("GET FEATURE 0x%01x returned 0x%01x. ", addr, readarr[time - 1]);
	return readarr[time - 1];
}

uint8_t spi_nand_get_feature(struct flashctx *flash, unsigned char addr)
{
	return spi_nand_get_feature_multi(flash, addr, 1);
}

int spi_nand_set_feature(struct flashctx *flash, unsigned char addr, uint8_t feature)
{
	/* Warning: feature will be kept after soft reset! */
	unsigned char cmd[SPINAND_SET_FEATURE_OUTSIZE] = { SPINAND_SET_FEATURE, addr, feature };

	return spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
}

int spi_nand_wait(struct flashctx *flash)
{
	uint8_t feature_c0;

	do {
		feature_c0 = spi_nand_get_feature_multi(flash, 0xc0, 4);
		if (errno)
			return errno;
	} while (feature_c0 & SPINAND_FEATURE_C0_OIP);

	return 0;
}

/***************************************
 * READ CYCLE
 ***************************************/

/* Typical NAND read process
 *   1. Read Page (prepare data to the cache)
 *   2. Get Feature (wait for data cache)
 *   3. Read Cache
 */

static int spi_nand_read_page(struct flashctx *flash, unsigned int row_addr)
{
	uint8_t cmd[1 + SPINAND_ROW_ADDR_LEN] = { SPINAND_READ_PAGE, };

	int addr_len = spi_nand_prepare_row_address(cmd, row_addr);

	return spi_send_command(flash, 1 + addr_len, 0, cmd, NULL);
}

static int spi_nand_wait_for_page(struct flashctx *flash, unsigned int row_addr)
{
	int ret;

	ret = spi_nand_read_page(flash, row_addr);
	if (ret)
		return ret;
	return spi_nand_wait(flash);
}

static int spi_nand_read_cache(struct flashctx *flash, unsigned int column_addr,
                               uint8_t *bytes, unsigned int len)
{
	uint8_t cmd[1 + SPINAND_COLUMN_ADDR_LEN] = { SPINAND_READ_CACHE, };

	int addr_len = spi_nand_prepare_column_address(cmd, column_addr);

	/* Send Read */
	return spi_send_command(flash, 1 + addr_len + DUMMY_BYTE, len, cmd, bytes);
}

static int spi_nand_inpage_read(struct flashctx *flash, unsigned int row_addr, unsigned int column_addr,
                                uint8_t *bytes, unsigned int len, unsigned int chunksize)
{
	/* Read data within one page */
	int ret;

	if (chunksize == 0)
		chunksize = len;
	ret = spi_nand_wait_for_page(flash, row_addr);
	if (ret)
		return ret;

	while (len) {
		unsigned int data_to_read = min(chunksize, len);
		ret = spi_nand_read_cache(flash, column_addr, bytes, data_to_read);
		if (ret)
			return ret;
		len -= data_to_read;
		column_addr += data_to_read;
		bytes += data_to_read;
		printf("page %d\n",len);
	}

	return 0;
}

static inline unsigned int ilog2(unsigned int x) {
	int res = 0;
	while (x >>= 1)
		++res;
	return res;
}

int spi_nand_read_chunked(struct flashctx *flash, uint8_t *buf, unsigned int start,
                          unsigned int len, unsigned int chunksize)
{
	/* Read data over multiple pages */
	int ret;

	/* Calculate acture column address len */
	unsigned int column_addr_len = ilog2(flash->chip->page_size);
	unsigned int column_addr_mask = ~(-1 << column_addr_len);
	printf("%d %d %d\n",flash->chip->page_size,column_addr_len,column_addr_mask);

	while (len) {
		unsigned int data_to_read = min(chunksize, len);
		ret = spi_nand_inpage_read(flash, start >> column_addr_len, start & column_addr_mask, buf, len, data_to_read);
		if (ret)
			return ret;
		len -= data_to_read;
		start += data_to_read;
		buf += data_to_read;
		printf("chunked %d\n",len);
	}

	return 0;
}

/***************************************
 * PROGRAM CYCLE
 ***************************************/


/***************************************
 * PROBE
 ***************************************/

static char *strncpy_tospace(char *dest, const char *src, unsigned int len)
{
	int i;

	for (i = 0; i < len; i++) {
		dest[i] = src[i];
		if (dest[i] == ' ') {
			dest[i] = '\0';
			break;
		}
	}
	return dest;
}

static void probe_spi_nand_toshiba(struct flashctx *flash, const uint8_t *parameters)
{
	char *vendor = (char *)malloc(12);
	char *name = (char *)malloc(20);

	flash->chip->vendor = strncpy_tospace(vendor, (const char *)(parameters + 32), sizeof(vendor));
	flash->chip->name = strncpy_tospace(name, (const char *)(parameters + 44), sizeof(name));
	flash->chip->page_size = *(uint32_t *)(parameters + 80);
	flash->chip->data_per_spare = flash->chip->page_size / *(uint16_t *)(parameters + 84);
	flash->chip->partial_page_size = *(uint32_t *)(parameters + 86);
	flash->chip->block_size = flash->chip->page_size * *(uint32_t *)(parameters + 92);
	flash->chip->unit_size = flash->chip->block_size * *(uint32_t *)(parameters + 96);
	flash->chip->total_size = flash->chip->unit_size * parameters[100] / 1024;
	flash->chip->total_size = 64;
}

#define HAS_MAGIC(data, magic) (strncmp((const char *)magic, (const char *)data, sizeof(magic)) == 0)

int probe_spi_nand(struct flashctx *flash)
{
	/* Read the Parameter Page */
	uint8_t parameters[SPINAND_MAX_PARAMETER_PAGE_SIZE];
	int i;

	uint8_t feature_b0 = spi_nand_get_feature(flash, 0xB0);
	spi_nand_set_feature(flash, 0xB0, feature_b0 | SPINAND_FEATURE_B0_IDR_E);
	spi_nand_inpage_read(flash, 0x01, 0x00, parameters, SPINAND_MAX_PARAMETER_PAGE_SIZE, 0);
	spi_nand_set_feature(flash, 0xB0, feature_b0);

	msg_cdbg("SPI NAND returned");
	for (i = 0; i < sizeof(parameters); i++)
		msg_cdbg(" %02x", parameters[i]);
	msg_cdbg("\n");

	if (HAS_MAGIC(parameters, spi_nand_magic_toshiba)) {
		msg_cdbg("%s: Found Toshiba\n", __func__);
		probe_spi_nand_toshiba(flash, parameters);
		return 1;
	}

	if (HAS_MAGIC(parameters, spi_nand_magic_micron)) {
		msg_cdbg("%s: Found Micron\n", __func__);
		probe_spi_nand_toshiba(flash, parameters);
		return 1;
	}

	return 0;
}
