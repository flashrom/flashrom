/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2014 Boris Baykov
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
 * SPI chip driver functions for 4-bytes addressing
 */

#include <string.h>
#include "flash.h"
#include "chipdrivers.h"
#include "spi.h"
#include "programmer.h"
#include "spi4ba.h"

/* #define MSG_TRACE_4BA_FUNCS 1 */

#ifdef MSG_TRACE_4BA_FUNCS
#define msg_trace(...) print(MSG_DEBUG, __VA_ARGS__)
#else
#define msg_trace(...)
#endif

/* Enter 4-bytes addressing mode (without sending WREN before) */
int spi_enter_4ba_b7(struct flashctx *flash)
{
	int result;
	const unsigned char cmd[JEDEC_ENTER_4_BYTE_ADDR_MODE_OUTSIZE] = { JEDEC_ENTER_4_BYTE_ADDR_MODE };

	msg_trace("-> %s\n", __func__);

	/* Switch to 4-bytes addressing mode */
	result = spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
	if (!result)
		flash->in_4ba_mode = true;

	return result;
}

/* Enter 4-bytes addressing mode with sending WREN before */
int spi_enter_4ba_b7_we(struct flashctx *flash)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_ENTER_4_BYTE_ADDR_MODE_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_ENTER_4_BYTE_ADDR_MODE },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	msg_trace("-> %s\n", __func__);

	/* Switch to 4-bytes addressing mode */
	result = spi_send_multicommand(flash, cmds);
	if (result)
		msg_cerr("%s failed during command execution\n", __func__);
	else
		flash->in_4ba_mode = true;
	return result;
}

/* Exit 4-bytes addressing mode (without sending WREN before) */
int spi_exit_4ba_e9(struct flashctx *flash)
{
	int result;
	const unsigned char cmd[JEDEC_EXIT_4_BYTE_ADDR_MODE_OUTSIZE] = { JEDEC_EXIT_4_BYTE_ADDR_MODE };

	msg_trace("-> %s\n", __func__);

	/* Switch to 3-bytes addressing mode  */
	result = spi_send_command(flash, sizeof(cmd), 0, cmd, NULL);
	if (!result)
		flash->in_4ba_mode = false;

	return result;
}

/* Exit 4-bytes addressing mode with sending WREN before */
int spi_exit_4ba_e9_we(struct flashctx *flash)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_EXIT_4_BYTE_ADDR_MODE_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_EXIT_4_BYTE_ADDR_MODE },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	msg_trace("-> %s\n", __func__);

	/* Switch to 3-bytes addressing mode  */
	result = spi_send_multicommand(flash, cmds);
	if (result)
		msg_cerr("%s failed during command execution\n", __func__);
	else
		flash->in_4ba_mode = false;
	return result;
}

/* Write Extended Address Register value */
int spi_write_extended_address_register(struct flashctx *flash, uint8_t regdata)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_WRITE_EXT_ADDR_REG_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_WRITE_EXT_ADDR_REG,
					regdata
				},
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	msg_trace("-> %s (%02X)\n", __func__, regdata);

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution\n", __func__);
		return result;
	}
	return 0;
}

/* Erase 4 KB of flash with 4-bytes address from ANY mode (3-bytes or 4-bytes)
   JEDEC_SE_4BA (21h) instruction is new for 4-bytes addressing flash chips.
   The presence of this instruction for an exact chip should be checked
   by its datasheet or from SFDP 4-Bytes Address Instruction Table (JESD216B). */
int spi_block_erase_21_4ba_direct(struct flashctx *flash, unsigned int addr,
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
		.writecnt	= JEDEC_SE_4BA_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_SE_4BA,
					(addr >> 24) & 0xff,
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

	msg_trace("-> %s (0x%08X-0x%08X)\n", __func__, addr, addr + blocklen - 1);

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

/* Erase 32 KB of flash with 4-bytes address from ANY mode (3-bytes or 4-bytes)
   JEDEC_BE_5C_4BA (5Ch) instruction is new for 4-bytes addressing flash chips.
   The presence of this instruction for an exact chip should be checked
   by its datasheet or from SFDP 4-Bytes Address Instruction Table (JESD216B). */
int spi_block_erase_5c_4ba_direct(struct flashctx *flash, unsigned int addr,
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
		.writecnt	= JEDEC_BE_5C_4BA_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_BE_5C_4BA,
					(addr >> 24) & 0xff,
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

	msg_trace("-> %s (0x%08X-0x%08X)\n", __func__, addr, addr + blocklen - 1);

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

/* Erase 64 KB of flash with 4-bytes address from ANY mode (3-bytes or 4-bytes)
   JEDEC_BE_DC_4BA (DCh) instruction is new for 4-bytes addressing flash chips.
   The presence of this instruction for an exact chip should be checked
   by its datasheet or from SFDP 4-Bytes Address Instruction Table (JESD216B). */
int spi_block_erase_dc_4ba_direct(struct flashctx *flash, unsigned int addr,
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
		.writecnt	= JEDEC_BE_DC_4BA_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_BE_DC_4BA,
					(addr >> 24) & 0xff,
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

	msg_trace("-> %s (0x%08X-0x%08X)\n", __func__, addr, addr + blocklen - 1);

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
