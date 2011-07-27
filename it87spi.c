/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2007, 2008, 2009 Carl-Daniel Hailfinger
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

#if defined(__i386__) || defined(__x86_64__)

#include <string.h>
#include <stdlib.h>
#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

#define ITE_SUPERIO_PORT1	0x2e
#define ITE_SUPERIO_PORT2	0x4e

uint16_t it8716f_flashport = 0;
/* use fast 33MHz SPI (<>0) or slow 16MHz (0) */
static int fast_spi = 1;

/* Helper functions for most recent ITE IT87xx Super I/O chips */
#define CHIP_ID_BYTE1_REG	0x20
#define CHIP_ID_BYTE2_REG	0x21
#define CHIP_VER_REG		0x22
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

uint16_t probe_id_ite(uint16_t port)
{
	uint16_t id;

	enter_conf_mode_ite(port);
	id = sio_read(port, CHIP_ID_BYTE1_REG) << 8;
	id |= sio_read(port, CHIP_ID_BYTE2_REG);
	exit_conf_mode_ite(port);

	return id;
}

void probe_superio_ite(void)
{
	struct superio s = {};
	uint16_t ite_ports[] = {ITE_SUPERIO_PORT1, ITE_SUPERIO_PORT2, 0};
	uint16_t *i = ite_ports;

	s.vendor = SUPERIO_VENDOR_ITE;
	for (; *i; i++) {
		s.port = *i;
		s.model = probe_id_ite(s.port);
		switch (s.model >> 8) {
		case 0x82:
		case 0x86:
		case 0x87:
			/* FIXME: Print revision for all models? */
			msg_pdbg("Found ITE Super I/O, ID 0x%04hx on port "
				 "0x%x\n", s.model, s.port);
			register_superio(s);
			break;
		case 0x85:
			msg_pdbg("Found ITE EC, ID 0x%04hx,"
			         "Rev 0x%02x on port 0x%x.\n",
			         s.model,
			         sio_read(s.port, CHIP_VER_REG),
			         s.port);
			register_superio(s);
			break;
		}
	}

	return;
}

static int it8716f_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			const unsigned char *writearr, unsigned char *readarr);
static int it8716f_spi_chip_read(struct flashchip *flash, uint8_t *buf, int start, int len);
static int it8716f_spi_chip_write_256(struct flashchip *flash, uint8_t *buf, int start, int len);

static const struct spi_programmer spi_programmer_it87xx = {
	.type = SPI_CONTROLLER_IT87XX,
	.max_data_read = MAX_DATA_UNSPECIFIED,
	.max_data_write = MAX_DATA_UNSPECIFIED,
	.command = it8716f_spi_send_command,
	.multicommand = default_spi_send_multicommand,
	.read = it8716f_spi_chip_read,
	.write_256 = it8716f_spi_chip_write_256,
};

static uint16_t it87spi_probe(uint16_t port)
{
	uint8_t tmp = 0;
	char *portpos = NULL;
	uint16_t flashport = 0;

	enter_conf_mode_ite(port);
	/* NOLDN, reg 0x24, mask out lowest bit (suspend) */
	tmp = sio_read(port, 0x24) & 0xFE;
	/* If IT87SPI was not explicitly selected, we want to check
	 * quickly if LPC->SPI translation is active.
	 */
	if ((programmer == PROGRAMMER_INTERNAL) && !(tmp & (0x0E))) {
		msg_pdbg("No IT87* serial flash segment enabled.\n");
		exit_conf_mode_ite(port);
		/* Nothing to do. */
		return 0;
	}
	msg_pdbg("Serial flash segment 0x%08x-0x%08x %sabled\n",
		 0xFFFE0000, 0xFFFFFFFF, (tmp & 1 << 1) ? "en" : "dis");
	msg_pdbg("Serial flash segment 0x%08x-0x%08x %sabled\n",
		 0x000E0000, 0x000FFFFF, (tmp & 1 << 1) ? "en" : "dis");
	msg_pdbg("Serial flash segment 0x%08x-0x%08x %sabled\n",
		 0xFFEE0000, 0xFFEFFFFF, (tmp & 1 << 2) ? "en" : "dis");
	msg_pdbg("Serial flash segment 0x%08x-0x%08x %sabled\n",
		 0xFFF80000, 0xFFFEFFFF, (tmp & 1 << 3) ? "en" : "dis");
	msg_pdbg("LPC write to serial flash %sabled\n",
		 (tmp & 1 << 4) ? "en" : "dis");
	/* The LPC->SPI force write enable below only makes sense for
	 * non-programmer mode.
	 */
	/* If any serial flash segment is enabled, enable writing. */
	if ((tmp & 0xe) && (!(tmp & 1 << 4))) {
		msg_pdbg("Enabling LPC write to serial flash\n");
		tmp |= 1 << 4;
		sio_write(port, 0x24, tmp);
	}
	msg_pdbg("Serial flash pin %i\n", (tmp & 1 << 5) ? 87 : 29);
	/* LDN 0x7, reg 0x64/0x65 */
	sio_write(port, 0x07, 0x7);
	flashport = sio_read(port, 0x64) << 8;
	flashport |= sio_read(port, 0x65);
	msg_pdbg("Serial flash port 0x%04x\n", flashport);
	/* Non-default port requested? */
	portpos = extract_programmer_param("it87spiport");
	if (portpos) {
		char *endptr = NULL;
		unsigned long forced_flashport;
		forced_flashport = strtoul(portpos, &endptr, 0);
		/* Port 0, port >0x1000, unaligned ports and garbage strings
		 * are rejected.
		 */
		if (!forced_flashport || (forced_flashport >= 0x1000) ||
		    (forced_flashport & 0x7) || (*endptr != '\0')) {
			/* Using ports below 0x100 is a really bad idea, and
			 * should only be done if no port between 0x100 and
			 * 0xff8 works due to routing issues.
			 */
			msg_perr("Error: it87spiport specified, but no valid "
				 "port specified.\nPort must be a multiple of "
				 "0x8 and lie between 0x100 and 0xff8.\n");
			free(portpos);
			return 1;
		} else {
			flashport = (uint16_t)forced_flashport;
			msg_pinfo("Forcing serial flash port 0x%04x\n",
				  flashport);
			sio_write(port, 0x64, (flashport >> 8));
			sio_write(port, 0x65, (flashport & 0xff));
		}
	}
	free(portpos);
	exit_conf_mode_ite(port);
	it8716f_flashport = flashport;
	if (buses_supported & BUS_SPI)
		msg_pdbg("Overriding chipset SPI with IT87 SPI.\n");
	/* FIXME: Add the SPI bus or replace the other buses with it? */
	register_spi_programmer(&spi_programmer_it87xx);
	return 0;
}

int init_superio_ite(void)
{
	int i;
	int ret = 0;

	for (i = 0; i < superio_count; i++) {
		if (superios[i].vendor != SUPERIO_VENDOR_ITE)
			continue;

		switch (superios[i].model) {
		case 0x8500:
		case 0x8502:
		case 0x8510:
		case 0x8511:
		case 0x8512:
			/* FIXME: This should be enabled, but we need a check
			 * for laptop whitelisting due to the amount of things
			 * which can go wrong if the EC firmware does not
			 * implement the interface we want.
			 */
			//it85xx_spi_init(superios[i]);
			break;
		case 0x8705:
			ret |= it8705f_write_enable(superios[i].port);
			break;
		case 0x8716:
		case 0x8718:
		case 0x8720:
			ret |= it87spi_probe(superios[i].port);
			break;
		default:
			msg_pdbg("Super I/O ID 0x%04hx is not on the list of "
				 "flash capable controllers.\n",
				 superios[i].model);
		}
	}
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
static int it8716f_spi_send_command(unsigned int writecnt, unsigned int readcnt,
			const unsigned char *writearr, unsigned char *readarr)
{
	uint8_t busy, writeenc;
	int i;

	do {
		busy = INB(it8716f_flashport) & 0x80;
	} while (busy);
	if (readcnt > 3) {
		msg_pinfo("%s called with unsupported readcnt %i.\n",
		       __func__, readcnt);
		return SPI_INVALID_LENGTH;
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
		msg_pinfo("%s called with unsupported writecnt %i.\n",
		       __func__, writecnt);
		return SPI_INVALID_LENGTH;
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
static int it8716f_spi_page_program(struct flashchip *flash, uint8_t *buf, int start)
{
	int i;
	int result;
	chipaddr bios = flash->virtual_memory;

	result = spi_write_enable();
	if (result)
		return result;
	/* FIXME: The command below seems to be redundant or wrong. */
	OUTB(0x06, it8716f_flashport + 1);
	OUTB(((2 + (fast_spi ? 1 : 0)) << 4), it8716f_flashport);
	for (i = 0; i < flash->page_size; i++) {
		chip_writeb(buf[i], bios + start + i);
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
 * IT8716F only allows maximum of 512 kb SPI mapped to LPC memory cycles
 * Need to read this big flash using firmware cycles 3 byte at a time.
 */
static int it8716f_spi_chip_read(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	fast_spi = 0;

	/* FIXME: Check if someone explicitly requested to use IT87 SPI although
	 * the mainboard does not use IT87 SPI translation. This should be done
	 * via a programmer parameter for the internal programmer.
	 */
	if ((flash->total_size * 1024 > 512 * 1024)) {
		spi_read_chunked(flash, buf, start, len, 3);
	} else {
		read_memmapped(flash, buf, start, len);
	}

	return 0;
}

static int it8716f_spi_chip_write_256(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	/*
	 * IT8716F only allows maximum of 512 kb SPI chip size for memory
	 * mapped access. It also can't write more than 1+3+256 bytes at once,
	 * so page_size > 256 bytes needs a fallback.
	 * FIXME: Split too big page writes into chunks IT87* can handle instead
	 * of degrading to single-byte program.
	 * FIXME: Check if someone explicitly requested to use IT87 SPI although
	 * the mainboard does not use IT87 SPI translation. This should be done
	 * via a programmer parameter for the internal programmer.
	 */
	if ((flash->total_size * 1024 > 512 * 1024) ||
	    (flash->page_size > 256)) {
		spi_chip_write_1(flash, buf, start, len);
	} else {
		int lenhere;

		if (start % flash->page_size) {
			/* start to the end of the page or to start + len,
			 * whichever is smaller.
			 */
			lenhere = min(len, flash->page_size - start % flash->page_size);
			spi_chip_write_1(flash, buf, start, lenhere);
			start += lenhere;
			len -= lenhere;
			buf += lenhere;
		}

		while (len >= flash->page_size) {
			it8716f_spi_page_program(flash, buf, start);
			start += flash->page_size;
			len -= flash->page_size;
			buf += flash->page_size;
		}
		if (len)
			spi_chip_write_1(flash, buf, start, len);
	}

	return 0;
}

#endif
