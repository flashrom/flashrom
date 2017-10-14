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

/* Program one flash byte from 4-bytes addressing mode */
int spi_byte_program_4ba(struct flashctx *flash, unsigned int addr, uint8_t databyte)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_BYTE_PROGRAM_OUTSIZE + 1,
		.writearr	= (const unsigned char[]){
					JEDEC_BYTE_PROGRAM,
					(addr >> 24) & 0xff,
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

	msg_trace("-> %s (0x%08X)\n", __func__, addr);

	result = spi_send_multicommand(flash, cmds);
	if (result)
		msg_cerr("%s failed during command execution at address 0x%x\n", __func__, addr);
	return result;
}

/* Program flash bytes from 4-bytes addressing mode */
int spi_nbyte_program_4ba(struct flashctx *flash, unsigned int addr, const uint8_t *bytes, unsigned int len)
{
	int result;
	unsigned char cmd[(JEDEC_BYTE_PROGRAM_OUTSIZE + 1) - 1 + 256] = {
		JEDEC_BYTE_PROGRAM,
		(addr >> 24) & 0xff,
		(addr >> 16) & 0xff,
		(addr >> 8) & 0xff,
		(addr >> 0) & 0xff
	};
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= (JEDEC_BYTE_PROGRAM_OUTSIZE + 1) - 1 + len,
		.writearr	= cmd,
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	msg_trace("-> %s (0x%08X-0x%08X)\n", __func__, addr, addr + len - 1);

	if (!len) {
		msg_cerr("%s called for zero-length write\n", __func__);
		return 1;
	}
	if (len > 256) {
		msg_cerr("%s called for too long a write\n", __func__);
		return 1;
	}

	memcpy(&cmd[(JEDEC_BYTE_PROGRAM_OUTSIZE + 1) - 1], bytes, len);

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n",
			__func__, addr);
	}
	return result;
}

/* Read flash bytes from 4-bytes addressing mode */
int spi_nbyte_read_4ba(struct flashctx *flash, unsigned int addr, uint8_t *bytes, unsigned int len)
{
	const unsigned char cmd[JEDEC_READ_OUTSIZE + 1] = {
		JEDEC_READ,
		(addr >> 24) & 0xff,
		(addr >> 16) & 0xff,
		(addr >> 8) & 0xff,
		(addr >> 0) & 0xff
	};

	msg_trace("-> %s (0x%08X-0x%08X)\n", __func__, addr, addr + len - 1);

	/* Send Read */
	return spi_send_command(flash, sizeof(cmd), len, cmd, bytes);
}

/* Erase one sector of flash from 4-bytes addressing mode */
int spi_block_erase_20_4ba(struct flashctx *flash, unsigned int addr, unsigned int blocklen)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_SE_OUTSIZE + 1,
		.writearr	= (const unsigned char[]){
					JEDEC_SE,
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

/* Erase one sector of flash from 4-bytes addressing mode */
int spi_block_erase_52_4ba(struct flashctx *flash, unsigned int addr, unsigned int blocklen)
{
	int result;
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_BE_52_OUTSIZE + 1,
		.writearr	= (const unsigned char[]){
					JEDEC_BE_52,
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

/* Erase one sector of flash from 4-bytes addressing mode */
int spi_block_erase_d8_4ba(struct flashctx *flash, unsigned int addr,
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
		.writecnt	= JEDEC_BE_D8_OUTSIZE + 1,
		.writearr	= (const unsigned char[]){
					JEDEC_BE_D8,
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

/* Assign required value of Extended Address Register. This function
   keeps last value of the register and writes the register if the
   value has to be changed only. */
int set_extended_address_register(struct flashctx *flash, uint8_t data)
{
	static uint8_t ext_addr_reg_state; /* memory for last register state */
	static int ext_addr_reg_state_valid = 0;
	int result;

	if (ext_addr_reg_state_valid == 0 || data != ext_addr_reg_state) {
		result = spi_write_extended_address_register(flash, data);
		if (result) {
			ext_addr_reg_state_valid = 0;
			return result;
		}
		ext_addr_reg_state = data;
		ext_addr_reg_state_valid = 1;
	}
	return 0;
}

/* Program one flash byte using Extended Address Register
   from 3-bytes addressing mode */
int spi_byte_program_4ba_ereg(struct flashctx *flash, unsigned int addr,
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

	msg_trace("-> %s (0x%08X)\n", __func__, addr);

	result = set_extended_address_register(flash, (addr >> 24) & 0xff);
	if (result)
		return result;

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n",
			__func__, addr);
	}
	return result;
}

/* Program flash bytes using Extended Address Register
   from 3-bytes addressing mode */
int spi_nbyte_program_4ba_ereg(struct flashctx *flash, unsigned int addr,
				  const uint8_t *bytes, unsigned int len)
{
	int result;
	unsigned char cmd[JEDEC_BYTE_PROGRAM_OUTSIZE - 1 + 256] = {
		JEDEC_BYTE_PROGRAM,
		(addr >> 16) & 0xff,
		(addr >> 8) & 0xff,
		(addr >> 0) & 0xff
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

	msg_trace("-> %s (0x%08X-0x%08X)\n", __func__, addr, addr + len - 1);

	if (!len) {
		msg_cerr("%s called for zero-length write\n", __func__);
		return 1;
	}
	if (len > 256) {
		msg_cerr("%s called for too long a write\n", __func__);
		return 1;
	}

	memcpy(&cmd[JEDEC_BYTE_PROGRAM_OUTSIZE - 1], bytes, len);

	result = set_extended_address_register(flash, (addr >> 24) & 0xff);
	if (result)
		return result;

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n",
			__func__, addr);
	}
	return result;
}

/* Read flash bytes using Extended Address Register
   from 3-bytes addressing mode */
int spi_nbyte_read_4ba_ereg(struct flashctx *flash, unsigned int addr,
		   uint8_t *bytes, unsigned int len)
{
	int result;
	const unsigned char cmd[JEDEC_READ_OUTSIZE] = {
		JEDEC_READ,
		(addr >> 16) & 0xff,
		(addr >> 8) & 0xff,
		(addr >> 0) & 0xff
	};

	msg_trace("-> %s (0x%08X-0x%08X)\n", __func__, addr, addr + len - 1);

	result = set_extended_address_register(flash, (addr >> 24) & 0xff);
	if (result)
		return result;

	/* Send Read */
	return spi_send_command(flash, sizeof(cmd), len, cmd, bytes);
}

/* Erases 4 KB of flash using Extended Address Register
   from 3-bytes addressing mode */
int spi_block_erase_20_4ba_ereg(struct flashctx *flash, unsigned int addr,
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

	msg_trace("-> %s (0x%08X-0x%08X)\n", __func__, addr, addr + blocklen - 1);

	result = set_extended_address_register(flash, (addr >> 24) & 0xff);
	if (result)
		return result;

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

/* Erases 32 KB of flash using Extended Address Register
   from 3-bytes addressing mode */
int spi_block_erase_52_4ba_ereg(struct flashctx *flash, unsigned int addr,
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

	msg_trace("-> %s (0x%08X-0x%08X)\n", __func__, addr, addr + blocklen - 1);

	result = set_extended_address_register(flash, (addr >> 24) & 0xff);
	if (result)
		return result;

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

/* Erases 64 KB of flash using Extended Address Register
   from 3-bytes addressing mode */
int spi_block_erase_d8_4ba_ereg(struct flashctx *flash, unsigned int addr,
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

	msg_trace("-> %s (0x%08X-0x%08X)\n", __func__, addr, addr + blocklen - 1);

	result = set_extended_address_register(flash, (addr >> 24) & 0xff);
	if (result)
		return result;

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

/* Program one flash byte with 4-bytes address from ANY mode (3-bytes or 4-bytes)
   JEDEC_BYTE_PROGRAM_4BA (12h) instruction is new for 4-bytes addressing flash chips.
   The presence of this instruction for an exact chip should be checked
   by its datasheet or from SFDP 4-Bytes Address Instruction Table (JESD216B). */
int spi_byte_program_4ba_direct(struct flashctx *flash, unsigned int addr,
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
		.writecnt	= JEDEC_BYTE_PROGRAM_4BA_OUTSIZE,
		.writearr	= (const unsigned char[]){
					JEDEC_BYTE_PROGRAM_4BA,
					(addr >> 24) & 0xff,
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

	msg_trace("-> %s (0x%08X)\n", __func__, addr);

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n",
			__func__, addr);
	}
	return result;
}

/* Program flash bytes with 4-bytes address from ANY mode (3-bytes or 4-bytes)
   JEDEC_BYTE_PROGRAM_4BA (12h) instruction is new for 4-bytes addressing flash chips.
   The presence of this instruction for an exact chip should be checked
   by its datasheet or from SFDP 4-Bytes Address Instruction Table (JESD216B). */
int spi_nbyte_program_4ba_direct(struct flashctx *flash, unsigned int addr,
				  const uint8_t *bytes, unsigned int len)
{
	int result;
	unsigned char cmd[JEDEC_BYTE_PROGRAM_4BA_OUTSIZE - 1 + 256] = {
		JEDEC_BYTE_PROGRAM_4BA,
		(addr >> 24) & 0xff,
		(addr >> 16) & 0xff,
		(addr >> 8) & 0xff,
		(addr >> 0) & 0xff
	};
	struct spi_command cmds[] = {
	{
		.writecnt	= JEDEC_WREN_OUTSIZE,
		.writearr	= (const unsigned char[]){ JEDEC_WREN },
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= JEDEC_BYTE_PROGRAM_4BA_OUTSIZE - 1 + len,
		.writearr	= cmd,
		.readcnt	= 0,
		.readarr	= NULL,
	}, {
		.writecnt	= 0,
		.writearr	= NULL,
		.readcnt	= 0,
		.readarr	= NULL,
	}};

	msg_trace("-> %s (0x%08X-0x%08X)\n", __func__, addr, addr + len - 1);

	if (!len) {
		msg_cerr("%s called for zero-length write\n", __func__);
		return 1;
	}
	if (len > 256) {
		msg_cerr("%s called for too long a write\n", __func__);
		return 1;
	}

	memcpy(&cmd[JEDEC_BYTE_PROGRAM_4BA_OUTSIZE - 1], bytes, len);

	result = spi_send_multicommand(flash, cmds);
	if (result) {
		msg_cerr("%s failed during command execution at address 0x%x\n",
			__func__, addr);
	}
	return result;
}

/* Read flash bytes with 4-bytes address from ANY mode (3-bytes or 4-bytes)
   JEDEC_READ_4BA (13h) instruction is new for 4-bytes addressing flash chips.
   The presence of this instruction for an exact chip should be checked
   by its datasheet or from SFDP 4-Bytes Address Instruction Table (JESD216B). */
int spi_nbyte_read_4ba_direct(struct flashctx *flash, unsigned int addr,
		   uint8_t *bytes, unsigned int len)
{
	const unsigned char cmd[JEDEC_READ_4BA_OUTSIZE] = {
		JEDEC_READ_4BA,
		(addr >> 24) & 0xff,
		(addr >> 16) & 0xff,
		(addr >> 8) & 0xff,
		(addr >> 0) & 0xff
	};

	msg_trace("-> %s (0x%08X-0x%08X)\n", __func__, addr, addr + len - 1);

	/* Send Read */
	return spi_send_command(flash, sizeof(cmd), len, cmd, bytes);
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
