/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010-2020, Google Inc.
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

#include "flash.h"
#include "hwaccess.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

#define MEC1308_SIO_PORT1	0x2e
#define MEC1308_SIO_PORT2	0x4e
#define MEC1308_SIO_ENTRY_KEY	0x55
#define MEC1308_SIO_EXIT_KEY	0xaa

#define MEC1308_SIOCFG_LDN	0x07	/* LDN Bank Selector */
#define MEC1308_DEVICE_ID_REG	0x20	/* Device ID Register */
#define MEC1308_DEVICE_ID_VAL	0x4d	/* Device ID Value for MEC1308 */
#define MEC1310_DEVICE_ID_VAL	0x04	/* Device ID Value for MEC1310 */
#define MEC1308_DEVICE_REV	0x21	/* Device Revision ID Register */

#define MEC1308_MBX_CMD		0x82	/* mailbox command register offset */
#define MEC1308_MBX_EXT_CMD	0x83	/* mailbox ext. command reg offset */
#define MEC1308_MBX_DATA_START	0x84	/* first mailbox data register offset */
#define MEC1308_MBX_DATA_END	0x91	/* last mailbox data register offset */

static unsigned int mbx_data;	/* Mailbox register interface data address*/

/*
 * These command codes depend on EC firmware. The ones listed below are input
 * using the mailbox interface, though others may be input using the ACPI
 * interface. Some commands also have an output value (ie pass/failure code)
 * which EC writes to the mailbox command register after completion.
 */
#define MEC1308_CMD_SMI_ENABLE		0x84
#define MEC1308_CMD_SMI_DISABLE		0x85
#define MEC1308_CMD_ACPI_ENABLE		0x86
#define MEC1308_CMD_ACPI_DISABLE	0x87

/*
 * Passthru commands are also input using the mailbox interface. Passthru mode
 * enter/start/end commands are special since they require a command word to
 * be written to the data registers. Other passthru commands are performed
 * after passthru mode has been started.
 *
 * Multiple passthru mode commands may be issued before ending passthru mode.
 * You do not need to enter, start, and end passthru mode for each SPI
 * command. However, other mailbox commands might not work when passthru mode
 * is enabled. For example, you may read all SPI chip content while in passthru
 * mode, but you should exit passthru mode before performing other EC commands
 * such as reading fan speed.
 */
#define MEC1308_CMD_PASSTHRU		0x55	/* force EC to process word */
#define MEC1308_CMD_PASSTHRU_SUCCESS	0xaa	/* success code for passthru */
#define MEC1308_CMD_PASSTHRU_FAIL	0xfe	/* failure code for passthru */
#define MEC1308_CMD_PASSTHRU_ENTER	"PathThruMode"	/* not a typo... */
#define MEC1308_CMD_PASSTHRU_START	"Start"
#define MEC1308_CMD_PASSTHRU_EXIT	"End_Mode"
#define MEC1308_CMD_PASSTHRU_CS_EN	0xf0	/* chip-select enable */
#define MEC1308_CMD_PASSTHRU_CS_DIS	0xf1	/* chip-select disable */
#define MEC1308_CMD_PASSTHRU_SEND	0xf2	/* send byte from data0 */
#define MEC1308_CMD_PASSTHRU_READ	0xf3	/* read byte, place in data0 */

typedef struct
{
	unsigned int in_sio_cfgmode;
	unsigned int mbx_idx;	/* Mailbox register interface index address */
	unsigned int mbx_data;	/* Mailbox register interface data address*/
} mec1308_data_t;

static void mec1308_sio_enter(mec1308_data_t *ctx_data, uint16_t port)
{
	if (ctx_data->in_sio_cfgmode)
		return;

	OUTB(MEC1308_SIO_ENTRY_KEY, port);
	ctx_data->in_sio_cfgmode = 1;
}

static void mec1308_sio_exit(mec1308_data_t *ctx_data, uint16_t port)
{
	if (!ctx_data->in_sio_cfgmode)
		return;

	OUTB(MEC1308_SIO_EXIT_KEY, port);
	ctx_data->in_sio_cfgmode = 0;
}

/** probe for super i/o index
 * @port: allocated buffer to store port
 *
 * returns 0 to indicate success, <0 to indicate error
 */
static int mec1308_get_sio_index(mec1308_data_t *ctx_data, uint16_t *port)
{
	uint16_t ports[] = { MEC1308_SIO_PORT1,
	                     MEC1308_SIO_PORT2,
	};
	size_t i;
	static uint16_t port_internal, port_found = 0;

	if (port_found) {
		*port = port_internal;
		return 0;
	}

	if (rget_io_perms())
		return -1;

	for (i = 0; i < ARRAY_SIZE(ports); i++) {
		uint8_t tmp8;

		/*
		 * Only after config mode has been successfully entered will the
		 * index port will read back the last value written to it.
		 * So we will attempt to enter config mode, set the index
		 * register, and see if the index register retains the value.
		 *
		 * Note: It seems to work "best" when using a device ID register
		 * as the index and reading from the data port before reading
		 * the index port.
		 */
		mec1308_sio_enter(ctx_data, ports[i]);
		OUTB(MEC1308_DEVICE_ID_REG, ports[i]);
		tmp8 = INB(ports[i] + 1);
		tmp8 = INB(ports[i]);
		if ((tmp8 != MEC1308_DEVICE_ID_REG)) {
			ctx_data->in_sio_cfgmode = 0;
			continue;
		}

		port_internal = ports[i];
		port_found = 1;
		break;
	}

	if (!port_found) {
		msg_cdbg("\nfailed to obtain super i/o index\n");
		return -1;
	}

	msg_cdbg("\nsuper i/o index = 0x%04x\n", port_internal);
	*port = port_internal;
	return 0;
}

static uint8_t mbx_read(mec1308_data_t *ctx_data, uint8_t idx)
{
	OUTB(idx, ctx_data->mbx_idx);
	return INB(mbx_data);
}

static int mbx_wait(mec1308_data_t *ctx_data)
{
	int i;
	int max_attempts = 10000;
	int rc = 0;

	for (i = 0; mbx_read(ctx_data, MEC1308_MBX_CMD); i++) {
		if (i == max_attempts) {
			rc = 1;
			break;
		}
		/* FIXME: This delay adds determinism to the delay period. It
		   was chosen arbitrarily thru some experiments. */
		programmer_delay(2);
	}

	return rc;
}

static int mbx_write(mec1308_data_t *ctx_data, uint8_t idx, uint8_t data)
{
	int rc = 0;

	if (idx == MEC1308_MBX_CMD && mbx_wait(ctx_data)) {
		msg_perr("%s: command register not clear\n", __func__);
		return 1;
	}

	OUTB(idx, ctx_data->mbx_idx);
	OUTB(data, mbx_data);

	if (idx == MEC1308_MBX_CMD)
		rc = mbx_wait(ctx_data);

	return rc;
}

static void mbx_clear(mec1308_data_t *ctx_data)
{
	int reg;

	for (reg = MEC1308_MBX_DATA_START; reg < MEC1308_MBX_DATA_END; reg++)
		mbx_write(ctx_data, reg, 0x00);
	mbx_write(ctx_data, MEC1308_MBX_CMD, 0x00);
}

static int mec1308_exit_passthru_mode(mec1308_data_t *ctx_data)
{
	uint8_t tmp8;
	size_t i;

	/* exit passthru mode */
	for (i = 0; i < strlen(MEC1308_CMD_PASSTHRU_EXIT); i++) {
		mbx_write(ctx_data, MEC1308_MBX_DATA_START + i,
		MEC1308_CMD_PASSTHRU_EXIT[i]);
	}

	if (mbx_write(ctx_data, MEC1308_MBX_CMD, MEC1308_CMD_PASSTHRU)) {
		msg_pdbg("%s(): exit passthru command timed out\n", __func__);
		return 1;
	}

	tmp8 = mbx_read(ctx_data, MEC1308_MBX_DATA_START);
	msg_pdbg("%s: result: 0x%02x ", __func__, tmp8);
	if (tmp8 == MEC1308_CMD_PASSTHRU_SUCCESS) {
		msg_pdbg("(exited passthru mode)\n");
	} else if (tmp8 == MEC1308_CMD_PASSTHRU_FAIL) {
		msg_pdbg("(failed to exit passthru mode)\n");
	}

	return 0;
}

static int enter_passthru_mode(mec1308_data_t *ctx_data)
{
	uint8_t tmp8;
	size_t i;

	/*
	 * Enter passthru mode. If the EC does not successfully enter passthru
	 * mode the first time, we'll clear the mailbox and issue the "exit
	 * passthru mode" command sequence up to 3 times or until it arrives in
	 * a known state.
	 *
	 * Note: This workaround was developed experimentally.
	 */
	for (i = 0; i < 3; i++) {
		size_t j;

		msg_pdbg("%s(): entering passthru mode, attempt %d out of 3\n",
		         __func__, (int)(i + 1));
		for (j = 0; j < strlen(MEC1308_CMD_PASSTHRU_ENTER); j++) {
			mbx_write(ctx_data, MEC1308_MBX_DATA_START + j,
			          MEC1308_CMD_PASSTHRU_ENTER[j]);
		}

		if (mbx_write(ctx_data, MEC1308_MBX_CMD, MEC1308_CMD_PASSTHRU))
			msg_pdbg("%s(): enter passthru command timed out\n",
			         __func__);

		tmp8 = mbx_read(ctx_data, MEC1308_MBX_DATA_START);
		if (tmp8 == MEC1308_CMD_PASSTHRU_SUCCESS)
			break;

		msg_pdbg("%s(): command failed, clearing data registers and "
		         "issuing full exit passthru command...\n", __func__);
		mbx_clear(ctx_data);
		mec1308_exit_passthru_mode(ctx_data);
	}

	if (tmp8 != MEC1308_CMD_PASSTHRU_SUCCESS) {
		msg_perr("%s(): failed to enter passthru mode, result=0x%02x\n",
		         __func__, tmp8);
		return 1;
	}

	msg_pdbg("%s(): enter passthru mode return code: 0x%02x\n",
	         __func__, tmp8);

	/* start passthru mode */
	for (i = 0; i < strlen(MEC1308_CMD_PASSTHRU_START); i++)
		mbx_write(ctx_data, MEC1308_MBX_DATA_START + i,
		          MEC1308_CMD_PASSTHRU_START[i]);
	if (mbx_write(ctx_data, MEC1308_MBX_CMD, MEC1308_CMD_PASSTHRU)) {
		msg_pdbg("%s(): start passthru command timed out\n", __func__);
		return 1;
	}
	tmp8 = mbx_read(ctx_data, MEC1308_MBX_DATA_START);
	if (tmp8 != MEC1308_CMD_PASSTHRU_SUCCESS) {
		msg_perr("%s(): failed to enter passthru mode, result=%02x\n",
		         __func__, tmp8);
		return 1;
	}
	msg_pdbg("%s(): start passthru mode return code: 0x%02x\n",
	         __func__, tmp8);

	return 0;
}

static int mec1308_shutdown(void *data)
{
	mec1308_data_t *ctx_data = (mec1308_data_t *)data;

	/* Exit passthru mode before performing commands which do not affect
	   the SPI ROM */
	mec1308_exit_passthru_mode(ctx_data);

	/* Re-enable SMI and ACPI.
	   FIXME: is there an ordering dependency? */
	if (mbx_write(ctx_data, MEC1308_MBX_CMD, MEC1308_CMD_SMI_ENABLE))
		msg_pdbg("%s: unable to re-enable SMI\n", __func__);
	if (mbx_write(ctx_data, MEC1308_MBX_CMD, MEC1308_CMD_ACPI_ENABLE))
		msg_pdbg("%s: unable to re-enable ACPI\n", __func__);

	free(data);
	return 0;
}

static int mec1308_chip_select(mec1308_data_t *ctx_data)
{
	return mbx_write(ctx_data, MEC1308_MBX_CMD, MEC1308_CMD_PASSTHRU_CS_EN);
}

static int mec1308_chip_deselect(mec1308_data_t *ctx_data)
{
	return mbx_write(ctx_data, MEC1308_MBX_CMD, MEC1308_CMD_PASSTHRU_CS_DIS);
}

/*
 * MEC1308 will not allow direct access to SPI chip from host if EC is
 * connected to LPC bus. This function will forward commands issued thru
 * mailbox interface to the SPI flash chip.
 */
static int mec1308_spi_send_command(const struct flashctx *flash, unsigned int writecnt,
                                    unsigned int readcnt,
                                    const unsigned char *writearr,
                                    unsigned char *readarr)
{
	int rc = 0;
	size_t i;
	mec1308_data_t *ctx_data = (mec1308_data_t *)flash->mst->spi.data;

	if (mec1308_chip_select(ctx_data))
		return 1;

	for (i = 0; i < writecnt; i++) {
		if (mbx_write(ctx_data, MEC1308_MBX_DATA_START, writearr[i]) ||
		    mbx_write(ctx_data, MEC1308_MBX_CMD, MEC1308_CMD_PASSTHRU_SEND)) {
			msg_pdbg("%s: failed to issue send command\n",__func__);
			rc = 1;
			goto mec1308_spi_send_command_exit;
		}
	}

	for (i = 0; i < readcnt; i++) {
		if (mbx_write(ctx_data, MEC1308_MBX_CMD, MEC1308_CMD_PASSTHRU_READ)) {
			msg_pdbg("%s: failed to issue read command\n",__func__);
			rc = 1;
			goto mec1308_spi_send_command_exit;
		}
		readarr[i] = mbx_read(ctx_data, MEC1308_MBX_DATA_START);
	}

mec1308_spi_send_command_exit:
	rc |= mec1308_chip_deselect(ctx_data);
	return rc;
}

static struct spi_master spi_master_mec1308 = {
	.max_data_read = 256,	/* FIXME: should be MAX_DATA_READ_UNLIMITED? */
	.max_data_write = 256,	/* FIXME: should be MAX_DATA_WRITE_UNLIMITED? */
	.command = mec1308_spi_send_command,
	.multicommand = default_spi_send_multicommand,
	.read = default_spi_read,
	.write_256 = default_spi_write_256,
};

static int check_params(void)
{
	int ret = 0;
	char *const p = extract_programmer_param("type");
	if (p && strcmp(p, "ec")) {
		msg_pdbg("mec1308 only supports \"ec\" type devices\n");
		ret = 1;
	}

	free(p);
	return ret;
}

int mec1308_init(void)
{
	uint16_t sio_port;
	uint8_t device_id;
	uint8_t tmp8;
	mec1308_data_t *ctx_data = NULL;

	msg_pdbg("%s(): entered\n", __func__);

	ctx_data = calloc(1, sizeof(mec1308_data_t));
	if (!ctx_data) {
		msg_perr("Unable to allocate space for extra context data.\n");
		return 1;
	}

	if (check_params())
		goto init_err_exit;

	if (mec1308_get_sio_index(ctx_data, &sio_port) < 0) {
		msg_pdbg("MEC1308 not found (probe failed).\n");
		goto init_err_exit;
	}
	device_id = sio_read(sio_port, MEC1308_DEVICE_ID_REG);
	switch(device_id) {
	case MEC1308_DEVICE_ID_VAL:
		msg_pdbg("Found EC: MEC1308 (ID:0x%02x,Rev:0x%02x) on "
		         "sio_port:0x%x.\n", device_id,
			 sio_read(sio_port, MEC1308_DEVICE_REV), sio_port);
		break;
	case MEC1310_DEVICE_ID_VAL:
		msg_pdbg("Found EC: MEC1310 (ID:0x%02x,Rev:0x%02x) on "
		         "sio_port:0x%x.\n", device_id,
			 sio_read(sio_port, MEC1308_DEVICE_REV), sio_port);
		break;
	default:
		msg_pdbg("MEC1308 not found\n");
		goto init_err_exit;
	}

	/*
	 * setup mailbox interface at LDN 9
	 */
	sio_write(sio_port, MEC1308_SIOCFG_LDN, 0x09);
	tmp8 = sio_read(sio_port, 0x30);
	tmp8 |= 1;
	sio_write(sio_port, 0x30, tmp8);	/* activate logical device */

	ctx_data->mbx_idx = (unsigned int)sio_read(sio_port, 0x60) << 8 |
	                                  sio_read(sio_port, 0x61);
	mbx_data = ctx_data->mbx_idx + 1;
	msg_pdbg("%s: mbx_idx: 0x%04x, mbx_data: 0x%04x\n",
	         __func__, ctx_data->mbx_idx, mbx_data);

	/* Exit Super I/O config mode */
	mec1308_sio_exit(ctx_data, sio_port);

	/* Now that we can read the mailbox, we will wait for any remaining
	 * command to finish.*/
	if (mbx_wait(ctx_data) != 0) {
		msg_perr("%s: mailbox is not available\n", __func__);
		goto init_err_exit;
	}

	/* Further setup -- disable SMI and ACPI.
	   FIXME: is there an ordering dependency? */
	if (mbx_write(ctx_data, MEC1308_MBX_CMD, MEC1308_CMD_ACPI_DISABLE)) {
		msg_pdbg("%s: unable to disable ACPI\n", __func__);
		goto init_err_exit;
	}

	if (mbx_write(ctx_data, MEC1308_MBX_CMD, MEC1308_CMD_SMI_DISABLE)) {
		msg_pdbg("%s: unable to disable SMI\n", __func__);
		goto init_err_exit;
	}

	/*
	 * Enter SPI Pass-Thru Mode after commands which do not require access
	 * to SPI ROM are complete. We'll start by doing the exit_passthru_mode
	 * sequence, which is benign if the EC is already in passthru mode.
	 */
	mec1308_exit_passthru_mode(ctx_data);

	if (enter_passthru_mode(ctx_data))
		goto init_err_cleanup_exit;

	internal_buses_supported |= BUS_LPC;	/* for LPC <--> SPI bridging */
	spi_master_mec1308.data = ctx_data;

	if (register_shutdown(mec1308_shutdown, ctx_data))
		goto init_err_cleanup_exit;
	register_spi_master(&spi_master_mec1308, NULL);
	msg_pdbg("%s(): successfully initialized mec1308\n", __func__);

	return 0;

init_err_cleanup_exit:
	mec1308_shutdown(ctx_data);
	return 1;

init_err_exit:
	free(ctx_data);
	return 1;
}
#endif
