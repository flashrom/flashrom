/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Paul Fox <pgf@laptop.org>
 * Copyright (C) 2009 Carl-Daniel Hailfinger
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "flash.h"
#include "spi.h"

#if FT2232_SPI_SUPPORT == 1

#include <ftdi.h>

/* the 'H' chips can run internally at either 12Mhz or 60Mhz.
 * the non-H chips can only run at 12Mhz. */
#define CLOCK_5X 1

/* in either case, the divisor is a simple integer clock divider. 
 * if CLOCK_5X is set, this divisor divides 30Mhz, else it
 * divides 6Mhz */
#define DIVIDE_BY 3  // e.g. '3' will give either 10Mhz or 2Mhz spi clock


static struct ftdi_context ftdic_context;

int send_buf(struct ftdi_context *ftdic, const unsigned char *buf, int size)
{
	int r;
	r = ftdi_write_data(ftdic, (unsigned char *) buf, size);
	if (r < 0) {
		fprintf(stderr, "ftdi_write_data: %d, %s\n", r,
				ftdi_get_error_string(ftdic));
		return 1;
	}
	return 0;
}

int get_buf(struct ftdi_context *ftdic, const unsigned char *buf, int size)
{
	int r;
	r = ftdi_read_data(ftdic, (unsigned char *) buf, size);
	if (r < 0) {
		fprintf(stderr, "ftdi_read_data: %d, %s\n", r,
				ftdi_get_error_string(ftdic));
		return 1;
	}
	return 0;
}

int ft2232_spi_init(void)
{
	int f;
	struct ftdi_context *ftdic = &ftdic_context;
	unsigned char buf[512];
	unsigned char port_val = 0;


	if (ftdi_init(ftdic) < 0) {
		fprintf(stderr, "ftdi_init failed\n");
		return EXIT_FAILURE;
	}

	// f = ftdi_usb_open(ftdic, 0x0403, 0x6010); // FT2232
	f = ftdi_usb_open(ftdic, 0x0403, 0x6011); // FT4232

	if (f < 0 && f != -5) {
		fprintf(stderr, "Unable to open ftdi device: %d (%s)\n", f,
				ftdi_get_error_string(ftdic));
		exit(-1);
	}

	if (ftdi_set_interface(ftdic, INTERFACE_B) < 0) {
		fprintf(stderr, "Unable to select FT2232 channel B: %s\n",
				ftdic->error_str);
	}

	if (ftdi_usb_reset(ftdic) < 0) {
		fprintf(stderr, "Unable to reset ftdi device\n");
	}

	if (ftdi_set_latency_timer(ftdic, 2) < 0) {
		fprintf(stderr, "Unable to set latency timer\n");
	}

	if (ftdi_write_data_set_chunksize(ftdic, 512)) {
		fprintf(stderr, "Unable to set chunk size\n");
	}

	if (ftdi_set_bitmode(ftdic, 0x00, 2) < 0) {
		fprintf(stderr, "Unable to set bitmode\n");
	}

#if CLOCK_5X
	printf_debug("Disable divide-by-5 front stage\n");
	buf[0] = 0x8a;		/* disable divide-by-5 */
	if (send_buf(ftdic, buf, 1))
		return -1;
#define MPSSE_CLK 60.0

#else

#define MPSSE_CLK 12.0

#endif
	printf_debug("Set clock divisor\n");
	buf[0] = 0x86;		/* command "set divisor" */
	/* valueL/valueH are (desired_divisor - 1) */
	buf[1] = (DIVIDE_BY-1) & 0xff;
	buf[2] = ((DIVIDE_BY-1) >> 8) & 0xff;
	if (send_buf(ftdic, buf, 3))
		return -1;

	printf("SPI clock is %fMHz\n",
		(double)(MPSSE_CLK / (((DIVIDE_BY-1) + 1) * 2)));

	/* Disconnect TDI/DO to TDO/DI for Loopback */
	printf_debug("No loopback of tdi/do tdo/di\n");
	buf[0] = 0x85;
	if (send_buf(ftdic, buf, 1))
		return -1;

	printf_debug("Set data bits\n");
	/* Set data bits low-byte command:
	 *  value: 0x08  CS=high, DI=low, DO=low, SK=low
	 *    dir: 0x0b  CS=output, DI=input, DO=output, SK=output
	 */
#define CS_BIT 0x08

	buf[0] = SET_BITS_LOW;
	buf[1] = (port_val = CS_BIT);
	buf[2] = 0x0b;
	if (send_buf(ftdic, buf, 3))
		return -1;

	printf_debug("\nft2232 chosen\n");

	buses_supported = CHIP_BUSTYPE_SPI;
	spi_controller = SPI_CONTROLLER_FT2232;

	return 0;
}

int ft2232_spi_command(unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr)
{
	struct ftdi_context *ftdic = &ftdic_context;
	static unsigned char *buf = NULL;
	unsigned char port_val = 0;
	int i, ret = 0;

	buf = realloc(buf, writecnt + readcnt + 100);
	if (!buf) {
		fprintf(stderr, "Out of memory!\n");
		exit(1);
	}

	i = 0;

	/* minimize USB transfers by packing as many commands
	 * as possible together.  if we're not expecting to
	 * read, we can assert CS, write, and deassert CS all
	 * in one shot.  if reading, we do three separate
	 * operations. */
	printf_debug("Assert CS#\n");
	buf[i++] = SET_BITS_LOW;
	buf[i++] = (port_val &= ~CS_BIT);
	buf[i++] = 0x0b;

	if (writecnt) {
		buf[i++] = 0x11;
		buf[i++] = (writecnt - 1) & 0xff;
		buf[i++] = ((writecnt - 1) >> 8) & 0xff;
		memcpy(buf+i, writearr, writecnt);
		i += writecnt;
	}

	/* optionally terminate this batch of commands with a
	 * read command, then do the fetch of the results.
	 */
	if (readcnt) {
		buf[i++] = 0x20;
		buf[i++] = (readcnt - 1) & 0xff;
		buf[i++] = ((readcnt - 1) >> 8) & 0xff;
		ret = send_buf(ftdic, buf, i);
		i = 0;
		if (ret) goto deassert_cs;

		/* FIXME: This is unreliable. There's no guarantee that we read
		 * the response directly after sending the read command.
		 * We may be scheduled out etc.
		 */
		ret = get_buf(ftdic, readarr, readcnt);

	}

    deassert_cs:
	printf_debug("De-assert CS#\n");
	buf[i++] = SET_BITS_LOW;
	buf[i++] = (port_val |= CS_BIT);
	buf[i++] = 0x0b;
	if (send_buf(ftdic, buf, i))
		return -1;

	return ret;
}

int ft2232_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	/* Maximum read length is 64k bytes. */
	return spi_read_chunked(flash, buf, start, len, 64 * 1024);
}

int ft2232_spi_write_256(struct flashchip *flash, uint8_t *buf)
{
	int total_size = 1024 * flash->total_size;
	int i;

	printf_debug("total_size is %d\n", total_size);
	for (i = 0; i < total_size; i += 256) {
		int l, r;
		if (i + 256 <= total_size)
			l = 256;
		else
			l = total_size - i;

		spi_write_enable();
		if ((r = spi_nbyte_program(i, &buf[i], l))) {
			fprintf(stderr, "%s: write fail %d\n", __FUNCTION__, r);
			// spi_write_disable();  chip does this for us
			return 1;
		}
		
		while (spi_read_status_register() & JEDEC_RDSR_BIT_WIP)
			/* loop */;
	}
	// spi_write_disable();  chip does this for us

	return 0;
}

#else
int ft2232_spi_init(void)
{
	fprintf(stderr, "FT2232 SPI support was not compiled in\n");
	exit(1);
}

int ft2232_spi_command(unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr)
{
	fprintf(stderr, "FT2232 SPI support was not compiled in\n");
	exit(1);
}

int ft2232_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	fprintf(stderr, "FT2232 SPI support was not compiled in\n");
	exit(1);
}

int ft2232_spi_write_256(struct flashchip *flash, uint8_t *buf)
{
	fprintf(stderr, "FT2232 SPI support was not compiled in\n");
	exit(1);
}
#endif
