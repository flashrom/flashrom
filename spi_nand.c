/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2022 Yangfl
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
#include "spi_nand.h"

#define __DEBUG_NAND__
#define __DUMP_RAW_NAND__
/***************************************
 * UTIL FUNCTIONS
 ***************************************/

/* 1 dummy byte between input and output */
#define DUMMY_BYTE 0xFF
#define DUMMY_LEN  1

struct nand_chip_data {
	struct nand_param_page params;
	uint8_t *page_buf;
	unsigned ecc_mode;
	unsigned config; // TODO
	void *ecc;
};

/* For error number */
static int errno;

#ifdef __DEBUG_NAND__

#define HEXDUMP_COLS 16

#include <ctype.h>

static void hexdump(const char *title, const void *mem, unsigned len)
{
  unsigned int i, j;

  if (title) printf("%s\n", title);

  if (!mem || (len == 0)) return;

  for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
  {
    /* print offset */
    if(i % HEXDUMP_COLS == 0)
    {
      printf("0x%06x: ", i);
    }

    /* print hex data */
    if(i < len)
    {
      printf("%02x ", 0xFF & ((const char*)mem)[i]);
    }
    else /* end of block, just aligning for ASCII dump */
    {
      printf("   ");
    }
    
    /* print ASCII dump */
    if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
    {
      for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
      {
        if(j >= len) /* end of block, not really printing */
        {
          putchar(' ');
        }
        else if(isprint(((const char*)mem)[j])) /* printable char */
        {
          putchar(0xFF & ((const char*)mem)[j]);        
        }
        else /* other char */
        {
          putchar('.');
        }
      }
      putchar('\n');
    }
  }
}

#else

#define hexdump(title, buf, len)

#endif

static void spi_nand_release(struct nand_chip_data *data)
{
	if (data) {
		if (data->ecc) {
			spi_nand_ecc_done(data->ecc);
		}
		if (data->page_buf) {
			free(data->page_buf);
		}
		memset(data, 0, sizeof(struct nand_chip_data));
		free(data);
	}
}

static void spi_nand_update_bits(struct flashctx *flash, uint8_t config_b0)
{
	struct nand_chip_data *data = (struct nand_chip_data *)flash->chip->data;
	flash->chip->feature_bits &= ~FEATURE_NAND_HW_ECC;
	if (config_b0 & JEDEC_NAND_FEATURE_B0_ECC_E) flash->chip->feature_bits |= FEATURE_NAND_HW_ECC;
	data->config = config_b0;
}

static void spi_nand_set_row_address(uint8_t cmd_buf[], const unsigned addr)
{
	cmd_buf[1] = (addr >> 16) & 0xff;
	cmd_buf[2] = (addr >>  8) & 0xff;
	cmd_buf[3] = (addr >>  0) & 0xff;
}

static void spi_nand_set_column_address(uint8_t cmd_buf[], const unsigned addr)
{
	cmd_buf[1] = (addr >>  8) & 0xff;
	cmd_buf[2] = (addr >>  0) & 0xff;
}

/***************************************
 * CHIP FEATURE
 ***************************************/

static int spi_nand_read_status_register(struct flashctx *flash, uint8_t reg_addr, uint8_t *value, int count)
{
	/* Feature will be outputted continuously until CS goes high */
	uint8_t cmd[JEDEC_NAND_GET_FEATURE_OUTSIZE] = { JEDEC_NAND_GET_FEATURE, reg_addr };
	uint8_t readarr[128];

	if (count > 100) {
		msg_cerr("%s called with too many 'time'! Please report a bug at "
			 "flashrom@flashrom.org\n", __FUNCTION__);
		errno = SPI_FLASHROM_BUG;
		return -1;
	}

	errno = spi_send_command(flash, sizeof(cmd), count, cmd, readarr);
	if (errno) {
		msg_cerr("%s: failed read Status Register 0x%x!\n", __FUNCTION__, reg_addr);
		return -1;
	}

	*value = readarr[count - 1];

	return 0;
}

static int spi_nand_write_config_register(struct flashctx *flash, uint8_t reg_addr, uint8_t value)
{
	/* Warning: feature will be kept after soft reset! */
	const uint8_t cmd[JEDEC_NAND_SET_FEATURE_OUTSIZE] = { JEDEC_NAND_SET_FEATURE, reg_addr, value };

	errno = spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
	if (errno) {
		msg_cerr("%s failed write Config Register 0x%x\n", __FUNCTION__, reg_addr);
		return -1;
	}
	return 0;
}

static int spi_nand_wait(struct flashctx *flash)
{
	uint8_t feature_c0;

	do {
		errno = spi_nand_read_status_register(flash, JEDEC_NAND_REG_STATUS, &feature_c0, 4);
		if (errno) return -1;
	} while (feature_c0 & JEDEC_NAND_FEATURE_C0_OIP);

	return 0;
}

/******************************
 * Control ECC mode
 * ecc_mode < 0 - hardware ECC
 *         >= 0 - software ECC
 ******************************/

int spi_nand_set_ecc_mode(struct flashctx *flash, int ecc_mode)
{
	struct nand_chip_data *data = (struct nand_chip_data *)flash->chip->data;
	uint8_t config_b0;

	errno = spi_nand_read_status_register(flash, JEDEC_NAND_REG_CONFIG, &config_b0, 1);
	if (errno) return -1;
	msg_cspew("STATUS REG1(B0) ==> %02x\n", config_b0);
	if (data->ecc) {
		spi_nand_ecc_done(data->ecc);
		data->ecc = NULL;
	}
	if (ecc_mode == SPI_NAND_HW_ECC)  {
		config_b0 |= JEDEC_NAND_FEATURE_B0_ECC_E;
	} else {
		config_b0 &= ~JEDEC_NAND_FEATURE_B0_ECC_E;
		// TODO Check other NAND producers
		if (flash->chip->manufacture_id == WINBOND_NEX_ID) {
			// Switch on BUF mode, can't correct data errors without OOB
			config_b0 |= JEDEC_NAND_FEATURE_B0_BUF;
		}
		data->ecc = spi_nand_ecc_init(ecc_mode);
		data->ecc_mode = ecc_mode;
	}
	errno = spi_nand_write_config_register(flash, JEDEC_NAND_REG_CONFIG, config_b0);
	if (errno) return -1;

	msg_cspew("STATUS REG1(B0) <== %02x\n", config_b0);
	spi_nand_update_bits(flash, config_b0);

	return 0;
}

/***************************************
 * READ CYCLE
 ***************************************/

/* Typical NAND read process
 *   1. Send data to cache (Read Page)
 *   2. Wait for page data fill cache (Get Feature)
 *   3. Read from cache ()
 */

static int spi_nand_page_data_read(struct flashctx *flash, unsigned row_addr)
{
	uint8_t cmd[1 + JEDEC_NAND_ROW_ADDR_LEN] = { JEDEC_NAND_READ_PAGE, };
	int ret;

	spi_nand_set_row_address(cmd, row_addr);

	ret = spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
	if (ret) return ret;

	return spi_nand_wait(flash);
}


static int spi_nand_read_page(struct flashctx *flash, unsigned page, uint8_t *dst, unsigned len)
{
	/* Read data */
	uint8_t cmd[1 + JEDEC_NAND_COLUMN_ADDR_LEN + DUMMY_LEN] = { JEDEC_NAND_READ_CACHE, 0, 0, DUMMY_BYTE};
	unsigned read_len;
	int ret;

	spi_nand_set_column_address(cmd, 0);
	ret = spi_nand_page_data_read(flash, page);
	if (ret) return ret;

	read_len = min(len, JEDEC_NAND_PAGE_SIZE);
	/* Send Read fron buffer */
	ret = spi_send_command(flash, sizeof(cmd), read_len, cmd, dst);
	if (ret) return ret;

	return 0;
}


int spi_nand_read(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	struct nand_chip_data *data = (struct nand_chip_data *)flash->chip->data;
	uint8_t *page_buf = data->page_buf;
	unsigned size = data->params.page_size + data->params.spare_size;

	size_t start_address = start;
	size_t end_address = len - start;
	size_t to_read;
	unsigned page_size = flash->chip->page_size;
	int page_address = start_address / page_size;
	bool chip_ecc = flash->chip->feature_bits & FEATURE_NAND_HW_ECC;
	int ret;

	for (; len; len -= to_read, buf += to_read, start += to_read, page_address++) {

		to_read = min(page_size, len);

		if (!chip_ecc) {
			ret = spi_nand_read_page(flash, page_address, page_buf, size);
			if (ret) return ret;
			if (data->ecc) {
				// HW error correction Off. Correct errors
				// spi_nand_ecc_decode(data->ecc, page_buf, data->ecc_mode);
			}
			memcpy(buf, page_buf, to_read);
		} else {
			ret = spi_nand_read_page(flash, page_address, buf, to_read);
			if (ret) return ret;
		}

		update_progress(flash, FLASHROM_PROGRESS_READ, start - start_address + to_read, end_address);
	}
	return 0;
}

/***************************************
 * PROGRAM CYCLE
 ***************************************/

static int spi_nand_page_data_program(struct flashctx *flash, unsigned row_addr)
{
	uint8_t cmd[1 + JEDEC_NAND_ROW_ADDR_LEN] = { JEDEC_NAND_PROGRAM_EXECUTE, };
	int ret;

	spi_nand_set_row_address(cmd, row_addr);

	ret = spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
	if (ret) return ret;

	return spi_nand_wait(flash);
}


static int spi_nand_program_page(struct flashctx *flash, unsigned page, uint8_t *src, unsigned len)
{
	/* Read data */
	uint8_t cmd[1 + JEDEC_NAND_COLUMN_ADDR_LEN + JEDEC_NAND_PAGE_SIZE] = { JEDEC_NAND_PROGRAM_LOAD, 0, 0};
	unsigned ofs = 1 + JEDEC_NAND_COLUMN_ADDR_LEN;
	unsigned write_len;
	int ret;

	if (!len) return 0; // Should be at least one byte

	spi_nand_set_column_address(cmd, 0);
	write_len = min(len, JEDEC_NAND_PAGE_SIZE);
	memcpy(&cmd[ofs], src, write_len);
	write_len += ofs;

	ret = spi_send_command(flash, write_len, 0, cmd, NULL);
	if (ret) return ret;

	/* Send Program fron buffer */
	ret = spi_nand_page_data_program(flash, page);
	if (ret) return ret;

	return 0;
}


int spi_nand_write(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	struct nand_chip_data *data = (struct nand_chip_data *)flash->chip->data;
	uint8_t *page_buf = data->page_buf;
	unsigned size = data->params.page_size + data->params.spare_size;

	size_t start_address = start;
	size_t end_address = len - start;
	size_t to_write;
	unsigned page_size = flash->chip->page_size;
	int page_address = start_address / page_size;
	bool chip_ecc = flash->chip->feature_bits & FEATURE_NAND_HW_ECC;
	int ret;

	for (; len; len -= to_write, buf += to_write, start += to_write, page_address++) {

		to_write = min(page_size, len);
		if (!chip_ecc) {
			memset(page_buf, 0xFF, size);
			memcpy(page_buf, buf, to_write);
			if (data->ecc) {
				// HW error correction Off. Add OOB + ECC
				// spi_nand_ecc_encode(data->ecc, page_buf, data->ecc_mode);
			}
			ret = spi_nand_program_page(flash, page_address, page_buf, size);
		} else {
			ret = spi_nand_program_page(flash, page_address, buf, to_write);
		}
		if (ret) return ret;

		update_progress(flash, FLASHROM_PROGRESS_READ, start - start_address + to_write, end_address);
	}
	return 0;
}

/***************************************
 * PROBE
 ***************************************/

static char *strndup_endwith(const char *src, size_t maxlen, int ch)
{
	char *retbuf;
	size_t len;
	
	for (len = 0; len < maxlen; len++)
		if (src[len] == ch) break;

	if ((retbuf = malloc(1 + len)) != NULL) {
		memcpy(retbuf, src, len);
		retbuf[len] = '\0';
	}
	return retbuf;
}

static int spi_nand_get_parameters(struct flashctx *flash, uint8_t m_id, uint16_t model_id, const uint8_t *param_page)
{
	const struct nand_param_page *params = (struct nand_param_page *)param_page;
	struct nand_chip_data *data = 0;

	hexdump("SPI NAND Parameters Page", param_page, sizeof(struct nand_param_page));

	// TODO? Check Page Signature 

	char *vendor = strndup_endwith((const char *)params->manufacturer, sizeof(params->manufacturer), ' ');
	char *name = strndup_endwith((const char *)params->model, sizeof(params->model), ' ');
	if (flash->chip->vendor == NULL || flash->chip->name == NULL) goto memory_error;

	flash->chip->vendor = vendor;
	flash->chip->name = name;
	flash->chip->manufacture_id = m_id;
	flash->chip->model_id = model_id;
	flash->chip->page_size = params->page_size;
	flash->chip->total_size = params->page_size * params->block_pages * params->unit_blocks * params->units / 1024;

	flash->address_high_byte = -1; // Winbond load page 0 on power on

	data = malloc(sizeof(struct nand_chip_data));
	if (!data) goto memory_error;
	memset(data, 0, sizeof(struct nand_chip_data));

	// Save NAND parameters page
	memcpy(&data->params, params, sizeof(struct nand_param_page));

	// Allocate memory for full NAND page
	data->page_buf = (uint8_t *)malloc(params->page_size + params->spare_size);
	if (!data->page_buf) goto memory_error;

	data->ecc_mode = 0;
	data->config = 0;
	flash->chip->data = data;

	return 0;

memory_error:
	msg_gerr("Could not allocate memory");
	spi_nand_release(data);
	if (vendor) free(vendor);
	if (name) free(name);
	return -1;
}

static int spi_nand_probe_rdid(struct flashctx *flash, uint8_t *m_id, uint16_t *model_id)
{
	const uint8_t cmd[JEDEC_RDID_OUTSIZE + DUMMY_LEN] = { JEDEC_RDID, DUMMY_BYTE };
	//const uint8_t cmd[JEDEC_RDID_OUTSIZE] = { JEDEC_RDID, };
	uint8_t rdid[JEDEC_RDID_INSIZE];
	int i, ret;

	msg_cdbg("Read RDID ...\n");
	ret = spi_send_command(flash, sizeof(cmd), sizeof(rdid), cmd, rdid);
	msg_cdbg("%s spi_send_command() ret=%d\n", __FUNCTION__, ret);
	if (ret) return ret;

	msg_ginfo("NAND RDID [");
	for (i = 0; i < JEDEC_RDID_INSIZE; i++)
		msg_ginfo(" 0x%02x", rdid[i]);
	msg_ginfo("]\n");

	*m_id = rdid[0];
	*model_id = (rdid[1] << 8) + rdid[2];

	return 0;
}

int probe_spi_nand(struct flashctx *flash)
{
	/* Read the Parameter Page */
	uint8_t param_page[JEDEC_NAND_PARAMETER_PAGE_SIZE];
	uint8_t m_id = 0;
	uint16_t model_id = 0;
	uint8_t config_b0;
	int ret;

	ret = spi_nand_probe_rdid(flash, &m_id, &model_id);
	if (ret) return ret;

	switch (m_id) {
	case KIOXIA_ID:
		msg_ginfo("%s: Kioxia (Toshiba Memory) NAND\n", __FUNCTION__);
		break;
	
	case MACRONIX_ID:
		msg_ginfo("%s: Macronix (MX) NAND\n", __FUNCTION__);
		break;

	case WINBOND_NEX_ID:
		msg_ginfo("%s: Winbond NAND\n", __FUNCTION__);
		break;

	default:
		return 0;
	}
	ret = spi_nand_read_status_register(flash, JEDEC_NAND_REG_CONFIG, &config_b0, 1);
	msg_pdbg("%s spi_nand_read_status_register() ret=%d\n", __FUNCTION__, ret);
	if (ret) return 0;
	msg_pdbg("STATUS REG1(B0) %02x\n", config_b0);
	ret = spi_nand_write_config_register(flash, JEDEC_NAND_REG_CONFIG, config_b0 | JEDEC_NAND_FEATURE_B0_IDR_E);
	if (ret) return 0;

	ret = spi_nand_read_page(flash, 0x01, param_page, sizeof(param_page));
	if (ret) return 0;
	ret = spi_nand_write_config_register(flash, JEDEC_NAND_REG_CONFIG, config_b0);
	if (ret) return 0;
	
	ret = spi_nand_get_parameters(flash, m_id, model_id, param_page);
	if (ret) return 0;

	// flash->chip->feature_bits = 0;

#ifdef __DUMP_RAW_NAND__
	FILE *f = fopen("nand_raw_dump.bin", "wb");
	if (f) {
		struct nand_chip_data *data = (struct nand_chip_data *)flash->chip->data;
		uint8_t *page_buf = data->page_buf;
		unsigned size = data->params.page_size + data->params.spare_size;
		unsigned page_no = data->params.block_pages * data->params.unit_blocks * data->params.units;
		unsigned i, k;

		spi_nand_set_ecc_mode(flash, SPI_NAND_SW_ECC0);
		k = 0;
		for (i = 0; i < page_no; i++) {

			ret = spi_nand_read_page(flash, i, page_buf, size);
			if (ret) {
				msg_perr("Page %x read error\n", i);
				break;
			}
			fwrite(page_buf, 1, size, f);
			if (++k == 64) {
				msg_ginfo("\rREAD pages %.1lf%%(%d)", 100.0*i / page_no, i);
				k = 0;
			}
		}
		fclose(f);
		msg_ginfo("\nNAND dump read %s\n", i == page_no ? "OK" : "FAILED");
	}
#endif
	// Restore default configuration
	spi_nand_write_config_register(flash, JEDEC_NAND_REG_CONFIG, config_b0);
	spi_nand_update_bits(flash, config_b0);

	return 1;
}
