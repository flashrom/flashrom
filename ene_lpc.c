/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2012-2020, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#if defined(__i386__) || defined(__x86_64__)
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "chipdrivers.h"
#include "flash.h"
#include "programmer.h"
#include "hwaccess.h"
#include "spi.h"

/* MCU registers */
#define REG_EC_HWVER    0xff00
#define REG_EC_FWVER    0xff01
#define REG_EC_EDIID    0xff24
#define REG_8051_CTRL   0xff14
#define REG_EC_EXTCMD   0xff10

#define CPU_RESET 1

/* MCU SPI peripheral registers */
#define REG_SPI_DATA    0xfeab
#define REG_SPI_COMMAND 0xfeac
#define REG_SPI_CONFIG  0xfead

#define CFG_CSn_FORCE_LOW            (1 << 4)
#define CFG_COMMAND_WRITE_ENABLE     (1 << 3)
#define CFG_STATUS                   (1 << 1)
#define CFG_ENABLE_BUSY_STATUS_CHECK (1 << 0)

/* Timeout */
#define EC_COMMAND_TIMEOUT   4
#define EC_RESTART_TIMEOUT  10
#define ENE_SPI_DELAY_CYCLE  4
#define EC_PAUSE_TIMEOUT    12
#define EC_RESET_TRIES       3

#define ENE_KB94X_PAUSE_WAKEUP_PORT  0x64

#define MASK_INPUT_BUFFER_FULL  2
#define MASK_OUTPUT_BUFFER_FULL 1

const int port_ene_bank   = 1;
const int port_ene_offset = 2;
const int port_ene_data   = 3;

/* Supported ENE ECs, ENE_LAST should always be LAST member */
enum ene_chip_id {
	ENE_KB932 = 0,
	ENE_KB94X,
	ENE_LAST
};

/* EC state */
enum ene_ec_state {
	EC_STATE_NORMAL,
	EC_STATE_IDLE,
	EC_STATE_RESET,
	EC_STATE_UNKNOWN
};

/* chip-specific parameters */
typedef struct {
	enum ene_chip_id chip_id;
	uint8_t          hwver;
	uint8_t          ediid;
	uint32_t         port_bios;
	uint32_t         port_ec_command;
	uint32_t         port_ec_data;
	uint8_t          ec_reset_cmd;
	uint8_t          ec_reset_data;
	uint8_t          ec_restart_cmd;
	uint8_t          ec_restart_data;
	uint8_t          ec_pause_cmd;
	uint8_t          ec_pause_data;
	uint16_t         ec_status_buf;
	uint8_t          ec_is_stopping;
	uint8_t          ec_is_running;
	uint8_t          ec_is_pausing;
	uint32_t         port_io_base;
} ene_chip_t;

typedef struct
{
	/* pointer to table entry of identified chip */
	ene_chip_t *chip;
	/* current ec state */
	enum ene_ec_state ec_state;
	struct timeval pause_begin;
} ene_lpc_data_t;

/* table of supported chips + parameters */
static ene_chip_t ene_chips[] = {
	{
	  ENE_KB932,       /* chip_id */
	  0xa2, 0x02,      /* hwver + ediid */
	  0x66,            /* port_bios */
	  0x6c, 0x68,      /* port_ec_{command,data} */
	  0x59, 0xf2,      /* ec_reset_{cmd,data} */
	  0x59, 0xf9,      /* ec_restart_{cmd,data} */
	  0x59, 0xf1,      /* ec_pause_{cmd,data} */
	  0xf554,          /* ec_status_buf */
	  0xa5, 0x00,      /* ec_is_{stopping,running} masks */
	  0x33,            /* ec_is_pausing mask */
	  0xfd60           /* port_io_base */
	},
	{
	  ENE_KB94X,       /* chip_id */
	  0xa3, 0x05,      /* hwver + ediid */
	  0x66,            /* port_bios */
	  0x66, 0x68,      /* port_ec_{command,data} */
	  0x7d, 0x10,      /* ec_reset_{cmd,data} */
	  0x7f, 0x10,      /* ec_restart_{cmd,data} */
	  0x7e, 0x10,      /* ec_pause_{cmd,data} */
	  0xf710,          /* ec_status_buf */
	  0x02, 0x00,      /* ec_is_{stopping,running} masks */
	  0x01,            /* ec_is_pausing mask */
	  0x0380           /* port_io_base */
	}
};

static void ec_command(const ene_chip_t *chip, uint8_t cmd, uint8_t data)
{
	struct timeval begin, now;

	/* Spin wait for EC input buffer empty */
	gettimeofday(&begin, NULL);
	while (INB(chip->port_ec_command) & MASK_INPUT_BUFFER_FULL) {
		gettimeofday(&now, NULL);
		if (now.tv_sec - begin.tv_sec >= EC_COMMAND_TIMEOUT) {
			msg_pdbg("%s: buf not empty\n", __func__);
			return;
		}
	}

	/* Write command */
	OUTB(cmd, chip->port_ec_command);

	if (chip->chip_id == ENE_KB932) {
		/* Spin wait for EC input buffer empty */
		gettimeofday(&begin, NULL);
		while (INB(chip->port_ec_command) & MASK_INPUT_BUFFER_FULL) {
			gettimeofday(&now, NULL);
			if (now.tv_sec - begin.tv_sec >= EC_COMMAND_TIMEOUT) {
				msg_pdbg("%s: buf not empty\n", __func__);
				return;
			}
		}
		/* Write data */
		OUTB(data, chip->port_ec_data);
	}
}

static uint8_t ene_read(const ene_chip_t *chip, uint16_t addr)
{
	uint8_t  bank;
	uint8_t  offset;
	uint8_t  data;
	uint32_t port_io_base;

	bank   = addr >> 8;
	offset = addr & 0xff;
	port_io_base = chip->port_io_base;

	OUTB(bank,   port_io_base + port_ene_bank);
	OUTB(offset, port_io_base + port_ene_offset);
	data = INB(port_io_base + port_ene_data);

	return data;
}

static void ene_write(const ene_chip_t *chip, uint16_t addr, uint8_t data)
{
	uint8_t  bank;
	uint8_t  offset;
	uint32_t port_io_base;

	bank   = addr >> 8;
	offset = addr & 0xff;
	port_io_base = chip->port_io_base;

	OUTB(bank,   port_io_base + port_ene_bank);
	OUTB(offset, port_io_base + port_ene_offset);

	OUTB(data, port_io_base + port_ene_data);
}

/**
 * wait_cycles, wait for n LPC bus clock cycles
 *
 * @param       n: number of LPC cycles to wait
 * @return      void
 */
static void wait_cycles(const ene_chip_t *chip,int n)
{
	while (n--)
		INB(chip->port_io_base + port_ene_bank);
}

static int is_spicmd_write(uint8_t cmd)
{
	switch (cmd) {
	case JEDEC_WREN:
		/* Chip Write Enable */
	case JEDEC_EWSR:
		/* Write Status Enable */
	case JEDEC_CE_60:
		/* Chip Erase 0x60 */
	case JEDEC_CE_C7:
		/* Chip Erase 0xc7 */
	case JEDEC_BE_52:
		/* Block Erase 0x52 */
	case JEDEC_BE_D8:
		/* Block Erase 0xd8 */
	case JEDEC_BE_D7:
		/* Block Erase 0xd7 */
	case JEDEC_SE:
		/* Sector Erase */
	case JEDEC_BYTE_PROGRAM:
		/* Write memory byte */
	case JEDEC_AAI_WORD_PROGRAM:
		/* Write AAI word */
		return 1;
	}
	return 0;
}

static void ene_spi_start(const ene_chip_t *chip)
{
	int cfg;

	cfg = ene_read(chip, REG_SPI_CONFIG);
	cfg |= CFG_CSn_FORCE_LOW;
	cfg |= CFG_COMMAND_WRITE_ENABLE;
	ene_write(chip, REG_SPI_CONFIG, cfg);

	wait_cycles(chip, ENE_SPI_DELAY_CYCLE);
}

static void ene_spi_end(const ene_chip_t *chip)
{
	int cfg;

	cfg = ene_read(chip, REG_SPI_CONFIG);
	cfg &= ~CFG_CSn_FORCE_LOW;
	cfg |= CFG_COMMAND_WRITE_ENABLE;
	ene_write(chip, REG_SPI_CONFIG, cfg);

	wait_cycles(chip, ENE_SPI_DELAY_CYCLE);
}

static int ene_spi_wait(const ene_chip_t *chip)
{
	struct timeval begin, now;

	gettimeofday(&begin, NULL);
	while (ene_read(chip, REG_SPI_CONFIG) & CFG_STATUS) {
		gettimeofday(&now, NULL);
		if (now.tv_sec - begin.tv_sec >= EC_COMMAND_TIMEOUT) {
			msg_pdbg("%s: spi busy\n", __func__);
			return 1;
		}
	}
	return 0;
}

static int ene_pause_ec(ene_lpc_data_t *ctx_data)
{
	struct timeval begin, now;
	const ene_chip_t *chip = ctx_data->chip;

	if (!chip->ec_pause_cmd)
		return -1;

	/* EC prepare pause */
	ec_command(chip, chip->ec_pause_cmd, chip->ec_pause_data);

	gettimeofday(&begin, NULL);
	/* Spin wait for EC ready */
	while (ene_read(chip, chip->ec_status_buf) != chip->ec_is_pausing) {
		gettimeofday(&now, NULL);
		if (now.tv_sec - begin.tv_sec >= EC_COMMAND_TIMEOUT) {
			msg_pdbg("%s: unable to pause ec\n", __func__);
			return -1;
		}
	}

	gettimeofday(&ctx_data->pause_begin, NULL);
	ctx_data->ec_state = EC_STATE_IDLE;
	return 0;
}

static int ene_resume_ec(ene_lpc_data_t *ctx_data)
{
	struct timeval begin, now;
	const ene_chip_t *chip = ctx_data->chip;

	if (chip->chip_id == ENE_KB94X)
		OUTB(0xff, ENE_KB94X_PAUSE_WAKEUP_PORT);
	else
		/* Trigger 8051 interrupt to resume */
		ene_write(chip, REG_EC_EXTCMD, 0xff);

	gettimeofday(&begin, NULL);
	while (ene_read(chip, chip->ec_status_buf) != chip->ec_is_running) {
		gettimeofday(&now, NULL);
		if (now.tv_sec - begin.tv_sec >= EC_COMMAND_TIMEOUT) {
			msg_pdbg("%s: unable to resume ec\n", __func__);
			return -1;
		}
	}

	ctx_data->ec_state = EC_STATE_NORMAL;
	return 0;
}

static int ene_pause_timeout_check(ene_lpc_data_t *ctx_data)
{
	struct timeval pause_now;
	gettimeofday(&pause_now, NULL);
	if (pause_now.tv_sec - ctx_data->pause_begin.tv_sec >= EC_PAUSE_TIMEOUT) {
		if (ene_resume_ec(ctx_data) == 0)
			ene_pause_ec(ctx_data);
	}
	return 0;
}

static int ene_reset_ec(ene_lpc_data_t *ctx_data)
{
	uint8_t reg;
	struct timeval begin, now;
	const ene_chip_t *chip = ctx_data->chip;

	gettimeofday(&begin, NULL);

	/* EC prepare reset */
	ec_command(chip, chip->ec_reset_cmd, chip->ec_reset_data);

	/* Spin wait for EC ready */
	while (ene_read(chip, chip->ec_status_buf) != chip->ec_is_stopping) {
		gettimeofday(&now, NULL);
		if (now.tv_sec - begin.tv_sec >= EC_COMMAND_TIMEOUT) {
			msg_pdbg("%s: unable to reset ec\n", __func__);
			return -1;
		}
	}

	/* Wait 1 second */
	sleep(1);

	/* Reset 8051 */
	reg = ene_read(chip, REG_8051_CTRL);
	reg |= CPU_RESET;
	ene_write(chip, REG_8051_CTRL, reg);

	ctx_data->ec_state = EC_STATE_RESET;
	return 0;
}

static int ene_enter_flash_mode(ene_lpc_data_t *ctx_data)
{
	if (ene_pause_ec(ctx_data))
		return ene_reset_ec(ctx_data);
	return 0;
}

static int ene_spi_send_command(const struct flashctx *flash,
				unsigned int writecnt,
				unsigned int readcnt,
				const unsigned char *writearr,
				unsigned char *readarr)
{
	unsigned int i;
	int tries = EC_RESET_TRIES;
	ene_lpc_data_t *ctx_data = (ene_lpc_data_t *)flash->mst->spi.data;
	const ene_chip_t *chip = ctx_data->chip;

	if (ctx_data->ec_state == EC_STATE_IDLE && is_spicmd_write(writearr[0])) {
		do {
			/* Enter reset mode if we need to write/erase */
			if (ene_resume_ec(ctx_data))
				continue;

			if (!ene_reset_ec(ctx_data))
				break;
		} while (--tries > 0);

		if (!tries) {
			msg_perr("%s: EC failed reset, skipping write\n", __func__);
			ctx_data->ec_state = EC_STATE_IDLE;
			return 1;
		}
	} else if (chip->chip_id == ENE_KB94X && ctx_data->ec_state == EC_STATE_IDLE) {
		ene_pause_timeout_check(ctx_data);
	}

	ene_spi_start(chip);

	for (i = 0; i < writecnt; i++) {
		ene_write(chip, REG_SPI_COMMAND, writearr[i]);
		if (ene_spi_wait(chip)) {
			msg_pdbg("%s: write count %d\n", __func__, i);
			return 1;
		}
	}

	for (i = 0; i < readcnt; i++) {
		/* Push data by clock the serial bus */
		ene_write(chip, REG_SPI_COMMAND, 0);
		if (ene_spi_wait(chip)) {
			msg_pdbg("%s: read count %d\n", __func__, i);
			return 1;
		}
		readarr[i] = ene_read(chip, REG_SPI_DATA);
		if (ene_spi_wait(chip)) {
			msg_pdbg("%s: read count %d\n", __func__, i);
			return 1;
		}
	}

	ene_spi_end(chip);
	return 0;
}

static int ene_leave_flash_mode(void *data)
{
	ene_lpc_data_t *ctx_data = (ene_lpc_data_t *)data;
	const ene_chip_t *chip = ctx_data->chip;
	int rv = 0;
	uint8_t reg;
	struct timeval begin, now;

	if (ctx_data->ec_state == EC_STATE_RESET) {
		reg = ene_read(chip, REG_8051_CTRL);
		reg &= ~CPU_RESET;
		ene_write(chip, REG_8051_CTRL, reg);

		gettimeofday(&begin, NULL);
		/* EC restart */
		while (ene_read(chip, chip->ec_status_buf) != chip->ec_is_running) {
			gettimeofday(&now, NULL);
			if (now.tv_sec - begin.tv_sec >= EC_RESTART_TIMEOUT) {
				msg_pdbg("%s: ec restart busy\n", __func__);
				rv = 1;
				goto exit;
			}
		}
		msg_pdbg("%s: send ec restart\n", __func__);
		ec_command(chip, chip->ec_restart_cmd, chip->ec_restart_data);

		ctx_data->ec_state = EC_STATE_NORMAL;
		rv = 0;
		goto exit;
	}

	rv = ene_resume_ec(ctx_data);

exit:
	/*
	 * Trigger ec interrupt after pause/reset by sending 0x80
	 * to bios command port.
	 */
	OUTB(0x80, chip->port_bios);
	free(data);
	return rv;
}

static const struct spi_master spi_master_ene = {
	.max_data_read = 256,
	.max_data_write = 256,
	.command = ene_spi_send_command,
	.multicommand = default_spi_send_multicommand,
	.read = default_spi_read,
	.write_256 = default_spi_write_256,
	.write_aai = default_spi_write_aai,
	.shutdown = ene_leave_flash_mode,
};

static int check_params(void)
{
	int ret = 0;
	char *const p = extract_programmer_param("type");
	if (p && strcmp(p, "ec")) {
		msg_pdbg("ene_lpc only supports \"ec\" type devices\n");
		ret = 1;
	}

	free(p);
	return ret;
}

static int ene_lpc_init()
{
	uint8_t hwver, ediid, i;
	ene_lpc_data_t *ctx_data = NULL;

	msg_pdbg("%s\n", __func__);

	ctx_data = calloc(1, sizeof(ene_lpc_data_t));
	if (!ctx_data) {
		msg_perr("Unable to allocate space for extra context data.\n");
		return 1;
	}
	ctx_data->ec_state = EC_STATE_NORMAL;

	if (check_params())
		goto init_err_exit;

	for (i = 0; i < ENE_LAST; ++i) {
		ctx_data->chip = &ene_chips[i];

		hwver = ene_read(ctx_data->chip, REG_EC_HWVER);
		ediid = ene_read(ctx_data->chip, REG_EC_EDIID);

		if(hwver == ene_chips[i].hwver &&
		   ediid == ene_chips[i].ediid) {
			break;
		}
	}

	if (i == ENE_LAST) {
		msg_pdbg("ENE EC not found (probe failed)\n");
		goto init_err_exit;
	}

	/* TODO: probe the EC stop protocol
	 *
	 * Compal - ec_command(0x41, 0xa1) returns 43 4f 4d 50 41 4c 9c
	 */

	ene_enter_flash_mode(ctx_data);

	internal_buses_supported |= BUS_LPC;

	return register_spi_master(&spi_master_ene, ctx_data);

init_err_exit:
	free(ctx_data);
	return 1;
}

const struct programmer_entry programmer_ene_lpc = {
	.name			= "ene_lpc",
	.type			= OTHER,
	.devs.note		= "ENE LPC interface keyboard controller\n",
	.init			= ene_lpc_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};

#endif /* __i386__ || __x86_64__ */
