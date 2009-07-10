/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2007, 2008 Carl-Daniel Hailfinger
 * Copyright (C) 2008 Ronald Hoogenboom <ronald@zonnet.nl>
 * Copyright (C) 2008 coresystems GmbH
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
 * Contains the ITE IT87* SPI specific routines
 */

#include <string.h>
#include "flash.h"
#include "spi.h"

#define ITE_SUPERIO_PORT1	0x2e
#define ITE_SUPERIO_PORT2	0x4e

uint16_t it8716f_flashport = 0;
/* use fast 33MHz SPI (<>0) or slow 16MHz (0) */
int fast_spi = 1;

/* Helper functions for most recent ITE IT87xx Super I/O chips */
#define CHIP_ID_BYTE1_REG	0x20
#define CHIP_ID_BYTE2_REG	0x21
void enter_conf_mode_ite(uint16_t port)
{
	OUTB(0x87, port);
	OUTB(0x01, port);
	OUTB(0x55, port);
	if (port == ITE_SUPERIO_PORT1)
		OUTB(0x55, port);
	else
		OUTB(0xaa, port);
}

void exit_conf_mode_ite(uint16_t port)
{
	sio_write(port, 0x02, 0x02);
}

static uint16_t find_ite_spi_flash_port(uint16_t port)
{
	uint8_t tmp = 0;
	uint16_t id, flashport = 0;

	enter_conf_mode_ite(port);

	id = sio_read(port, CHIP_ID_BYTE1_REG) << 8;
	id |= sio_read(port, CHIP_ID_BYTE2_REG);

	/* TODO: Handle more IT87xx if they support flash translation */
	if (0x8716 == id || 0x8718 == id) {
		/* NOLDN, reg 0x24, mask out lowest bit (suspend) */
		tmp = sio_read(port, 0x24) & 0xFE;
		printf("Serial flash segment 0x%08x-0x%08x %sabled\n",
		       0xFFFE0000, 0xFFFFFFFF, (tmp & 1 << 1) ? "en" : "dis");
		printf("Serial flash segment 0x%08x-0x%08x %sabled\n",
		       0x000E0000, 0x000FFFFF, (tmp & 1 << 1) ? "en" : "dis");
		printf("Serial flash segment 0x%08x-0x%08x %sabled\n",
		       0xFFEE0000, 0xFFEFFFFF, (tmp & 1 << 2) ? "en" : "dis");
		printf("Serial flash segment 0x%08x-0x%08x %sabled\n",
		       0xFFF80000, 0xFFFEFFFF, (tmp & 1 << 3) ? "en" : "dis");
		printf("LPC write to serial flash %sabled\n",
		       (tmp & 1 << 4) ? "en" : "dis");
		/* If any serial flash segment is enabled, enable writing. */
		if ((tmp & 0xe) && (!(tmp & 1 << 4))) {
			printf("Enabling LPC write to serial flash\n");
			tmp |= 1 << 4;
			sio_write(port, 0x24, tmp);
		}
		printf("Serial flash pin %i\n", (tmp & 1 << 5) ? 87 : 29);
		/* LDN 0x7, reg 0x64/0x65 */
		sio_write(port, 0x07, 0x7);
		flashport = sio_read(port, 0x64) << 8;
		flashport |= sio_read(port, 0x65);
		printf("Serial flash port 0x%04x\n", flashport);
	}
	exit_conf_mode_ite(port);
	return flashport;
}

int it87spi_common_init(void)
{
	it8716f_flashport = find_ite_spi_flash_port(ITE_SUPERIO_PORT1);

	if (!it8716f_flashport)
		it8716f_flashport = find_ite_spi_flash_port(ITE_SUPERIO_PORT2);

	if (it8716f_flashport)
		spi_controller = SPI_CONTROLLER_IT87XX;

	return (!it8716f_flashport);
}


int it87spi_init(void)
{
	int ret;

	get_io_perms();
	ret = it87spi_common_init();
	if (!ret) {
		buses_supported = CHIP_BUSTYPE_SPI;
	} else {
		buses_supported = CHIP_BUSTYPE_NONE;
	}
	return ret;
}

int it87xx_probe_spi_flash(const char *name)
{
	int ret;

	ret = it87spi_common_init();
	if (!ret)
		buses_supported |= CHIP_BUSTYPE_SPI;
	return ret;
}

/*
 * The IT8716F only supports commands with length 1,2,4,5 bytes including
 * command byte and can not read more than 3 bytes from the device.
 *
 * This function expects writearr[0] to be the first byte sent to the device,
 * whereas the IT8716F splits commands internally into address and non-address
 * commands with the address in inverse wire order. That's why the register
 * ordering in case 4 and 5 may seem strange.
 */
int it8716f_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			const unsigned char *writearr, unsigned char *readarr)
{
	uint8_t busy, writeenc;
	int i;

	do {
		busy = INB(it8716f_flashport) & 0x80;
	} while (busy);
	if (readcnt > 3) {
		printf("%s called with unsupported readcnt %i.\n",
		       __FUNCTION__, readcnt);
		return 1;
	}
	switch (writecnt) {
	case 1:
		OUTB(writearr[0], it8716f_flashport + 1);
		writeenc = 0x0;
		break;
	case 2:
		OUTB(writearr[0], it8716f_flashport + 1);
		OUTB(writearr[1], it8716f_flashport + 7);
		writeenc = 0x1;
		break;
	case 4:
		OUTB(writearr[0], it8716f_flashport + 1);
		OUTB(writearr[1], it8716f_flashport + 4);
		OUTB(writearr[2], it8716f_flashport + 3);
		OUTB(writearr[3], it8716f_flashport + 2);
		writeenc = 0x2;
		break;
	case 5:
		OUTB(writearr[0], it8716f_flashport + 1);
		OUTB(writearr[1], it8716f_flashport + 4);
		OUTB(writearr[2], it8716f_flashport + 3);
		OUTB(writearr[3], it8716f_flashport + 2);
		OUTB(writearr[4], it8716f_flashport + 7);
		writeenc = 0x3;
		break;
	default:
		printf("%s called with unsupported writecnt %i.\n",
		       __FUNCTION__, writecnt);
		return 1;
	}
	/*
	 * Start IO, 33 or 16 MHz, readcnt input bytes, writecnt output bytes.
	 * Note:
	 * We can't use writecnt directly, but have to use a strange encoding.
	 */
	OUTB(((0x4 + (fast_spi ? 1 : 0)) << 4)
		| ((readcnt & 0x3) << 2) | (writeenc), it8716f_flashport);

	if (readcnt > 0) {
		do {
			busy = INB(it8716f_flashport) & 0x80;
		} while (busy);

		for (i = 0; i < readcnt; i++)
			readarr[i] = INB(it8716f_flashport + 5 + i);
	}

	return 0;
}

/* Page size is usually 256 bytes */
static int it8716f_spi_page_program(int block, uint8_t *buf, uint8_t *bios)
{
	int i;
	int result;

	result = spi_write_enable();
	if (result)
		return result;
	OUTB(0x06, it8716f_flashport + 1);
	OUTB(((2 + (fast_spi ? 1 : 0)) << 4), it8716f_flashport);
	for (i = 0; i < 256; i++) {
		bios[256 * block + i] = buf[256 * block + i];
	}
	OUTB(0, it8716f_flashport);
	/* Wait until the Write-In-Progress bit is cleared.
	 * This usually takes 1-10 ms, so wait in 1 ms steps.
	 */
	while (spi_read_status_register() & JEDEC_RDSR_BIT_WIP)
		programmer_delay(1000);
	return 0;
}

/*
 * Program chip using firmware cycle byte programming. (SLOW!)
 * This is for chips which can only handle one byte writes
 * and for chips where memory mapped programming is impossible due to
 * size constraints in IT87* (over 512 kB)
 */
int it8716f_spi_chip_write_1(struct flashchip *flash, uint8_t *buf)
{
	int total_size = 1024 * flash->total_size;
	int i;
	int result;

	fast_spi = 0;

	spi_disable_blockprotect();
	for (i = 0; i < total_size; i++) {
		result = spi_write_enable();
		if (result)
			return result;
		spi_byte_program(i, buf[i]);
		while (spi_read_status_register() & JEDEC_RDSR_BIT_WIP)
			programmer_delay(10);
	}
	/* resume normal ops... */
	OUTB(0x20, it8716f_flashport);

	return 0;
}

/*
 * IT8716F only allows maximum of 512 kb SPI mapped to LPC memory cycles
 * Need to read this big flash using firmware cycles 3 byte at a time.
 */
int it8716f_spi_chip_read(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	int total_size = 1024 * flash->total_size;
	fast_spi = 0;

	if ((programmer == PROGRAMMER_IT87SPI) || (total_size > 512 * 1024)) {
		spi_read_chunked(flash, buf, start, len, 3);
	} else {
		read_memmapped(flash, buf, start, len);
	}

	return 0;
}

int it8716f_spi_chip_write_256(struct flashchip *flash, uint8_t *buf)
{
	int total_size = 1024 * flash->total_size;
	int i;

	/*
	 * IT8716F only allows maximum of 512 kb SPI chip size for memory
	 * mapped access.
	 */
	if ((programmer == PROGRAMMER_IT87SPI) || (total_size > 512 * 1024)) {
		it8716f_spi_chip_write_1(flash, buf);
	} else {
		for (i = 0; i < total_size / 256; i++) {
			it8716f_spi_page_program(i, buf,
				(uint8_t *)flash->virtual_memory);
		}
	}

	return 0;
}
