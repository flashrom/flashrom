/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2008 Wang Qingpei <Qingpei.Wang@amd.com>
 * Copyright (C) 2008 Joe Bao <Zheng.Bao@amd.com>
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include <string.h>
#include "flash.h"
#include "spi.h"

/* This struct is unused, but helps visualize the SB600 SPI BAR layout.
 *struct sb600_spi_controller {
 *	unsigned int spi_cntrl0;	/ * 00h * /
 *	unsigned int restrictedcmd1;	/ * 04h * /
 *	unsigned int restrictedcmd2;	/ * 08h * /
 *	unsigned int spi_cntrl1;	/ * 0ch * /
 *	unsigned int spi_cmdvalue0;	/ * 10h * /
 *	unsigned int spi_cmdvalue1;	/ * 14h * /
 *	unsigned int spi_cmdvalue2;	/ * 18h * /
 *	unsigned int spi_fakeid;	/ * 1Ch * /
 *};
 */

uint8_t *sb600_spibar = NULL;

int sb600_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	/* Maximum read length is 8 bytes. */
	return spi_read_chunked(flash, buf, start, len, 8);
}

/* FIXME: SB600 can write 5 bytes per transaction. */
int sb600_spi_write_1(struct flashchip *flash, uint8_t *buf)
{
	int rc = 0, i;
	int total_size = flash->total_size * 1024;
	int result;

	spi_disable_blockprotect();
	/* Erase first */
	printf("Erasing flash before programming... ");
	if (erase_flash(flash)) {
		fprintf(stderr, "ERASE FAILED!\n");
		return -1;
	}
	printf("done.\n");

	printf("Programming flash");
	for (i = 0; i < total_size; i++, buf++) {
		result = spi_byte_program(i, *buf);
		/* wait program complete. */
		if (i % 0x8000 == 0)
			printf(".");
		while (spi_read_status_register() & JEDEC_RDSR_BIT_WIP)
			;
	}
	printf(" done.\n");
	return rc;
}

static void reset_internal_fifo_pointer(void)
{
	mmio_writeb(mmio_readb(sb600_spibar + 2) | 0x10, sb600_spibar + 2);

	while (mmio_readb(sb600_spibar + 0xD) & 0x7)
		printf("reset\n");
}

static void execute_command(void)
{
	mmio_writeb(mmio_readb(sb600_spibar + 2) | 1, sb600_spibar + 2);

	while (mmio_readb(sb600_spibar + 2) & 1)
		;
}

int sb600_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		      const unsigned char *writearr, unsigned char *readarr)
{
	int count;
	/* First byte is cmd which can not being sent through FIFO. */
	unsigned char cmd = *writearr++;
	unsigned int readoffby1;

	writecnt--;

	printf_debug("%s, cmd=%x, writecnt=%x, readcnt=%x\n",
		     __func__, cmd, writecnt, readcnt);

	if (readcnt > 8) {
		printf("%s, SB600 SPI controller can not receive %d bytes, "
		       "it is limited to 8 bytes\n", __func__, readcnt);
		return SPI_INVALID_LENGTH;
	}

	if (writecnt > 8) {
		printf("%s, SB600 SPI controller can not send %d bytes, "
		       "it is limited to 8 bytes\n", __func__, writecnt);
		return SPI_INVALID_LENGTH;
	}

	/* This is a workaround for a bug in SB600 and SB700. If we only send
	 * an opcode and no additional data/address, the SPI controller will
	 * read one byte too few from the chip. Basically, the last byte of
	 * the chip response is discarded and will not end up in the FIFO.
	 * It is unclear if the CS# line is set high too early as well.
	 */
	readoffby1 = (writecnt) ? 0 : 1;
	mmio_writeb((readcnt + readoffby1) << 4 | (writecnt), sb600_spibar + 1);
	mmio_writeb(cmd, sb600_spibar + 0);

	/* Before we use the FIFO, reset it first. */
	reset_internal_fifo_pointer();

	/* Send the write byte to FIFO. */
	for (count = 0; count < writecnt; count++, writearr++) {
		printf_debug(" [%x]", *writearr);
		mmio_writeb(*writearr, sb600_spibar + 0xC);
	}
	printf_debug("\n");

	/*
	 * We should send the data by sequence, which means we need to reset
	 * the FIFO pointer to the first byte we want to send.
	 */
	reset_internal_fifo_pointer();

	execute_command();

	/*
	 * After the command executed, we should find out the index of the
	 * received byte. Here we just reset the FIFO pointer and skip the
	 * writecnt.
	 * It would be possible to increase the FIFO pointer by one instead
	 * of reading and discarding one byte from the FIFO.
	 * The FIFO is implemented on top of an 8 byte ring buffer and the
	 * buffer is never cleared. For every byte that is shifted out after
	 * the opcode, the FIFO already stores the response from the chip.
	 * Usually, the chip will respond with 0x00 or 0xff.
	 */
	reset_internal_fifo_pointer();

	/* Skip the bytes we sent. */
	for (count = 0; count < writecnt; count++) {
		cmd = mmio_readb(sb600_spibar + 0xC);
		printf_debug("[ %2x]", cmd);
	}

	printf_debug("The FIFO pointer after skipping is %d.\n",
		     mmio_readb(sb600_spibar + 0xd) & 0x07);
	for (count = 0; count < readcnt; count++, readarr++) {
		*readarr = mmio_readb(sb600_spibar + 0xC);
		printf_debug("[%02x]", *readarr);
	}
	printf_debug("\n");

	return 0;
}
