/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Paul Fox <pgf@laptop.org>
 * Copyright (C) 2009, 2010 Carl-Daniel Hailfinger
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

#if CONFIG_FT2232_SPI == 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "flash.h"
#include "programmer.h"
#include "spi.h"
#include <ftdi.h>

/* Please keep sorted by vendor ID, then device ID. */

#define FTDI_VID		0x0403
#define FTDI_FT2232H_PID	0x6010
#define FTDI_FT4232H_PID	0x6011
#define TIAO_TUMPA_PID		0x8a98
#define AMONTEC_JTAGKEY_PID	0xCFF8

#define GOEPEL_VID		0x096C
#define GOEPEL_PICOTAP_PID	0x1449

#define FIC_VID			0x1457
#define OPENMOKO_DBGBOARD_PID	0x5118

#define OLIMEX_VID		0x15BA
#define OLIMEX_ARM_OCD_PID	0x0003
#define OLIMEX_ARM_TINY_PID	0x0004
#define OLIMEX_ARM_OCD_H_PID	0x002B
#define OLIMEX_ARM_TINY_H_PID	0x002A

const struct usbdev_status devs_ft2232spi[] = {
	{FTDI_VID, FTDI_FT2232H_PID, OK, "FTDI", "FT2232H"},
	{FTDI_VID, FTDI_FT4232H_PID, OK, "FTDI", "FT4232H"},
	{FTDI_VID, TIAO_TUMPA_PID, OK, "TIAO", "USB Multi-Protocol Adapter"},
	{FTDI_VID, AMONTEC_JTAGKEY_PID, OK, "Amontec", "JTAGkey"},
	{GOEPEL_VID, GOEPEL_PICOTAP_PID, OK, "GOEPEL", "PicoTAP"},
	{FIC_VID, OPENMOKO_DBGBOARD_PID, OK, "FIC",
		"OpenMoko Neo1973 Debug board (V2+)"},
	{OLIMEX_VID, OLIMEX_ARM_OCD_PID, NT, "Olimex", "ARM-USB-OCD"},
	{OLIMEX_VID, OLIMEX_ARM_TINY_PID, OK, "Olimex", "ARM-USB-TINY"},
	{OLIMEX_VID, OLIMEX_ARM_OCD_H_PID, NT, "Olimex", "ARM-USB-OCD-H"},
	{OLIMEX_VID, OLIMEX_ARM_TINY_H_PID, NT, "Olimex", "ARM-USB-TINY-H"},
	{},
};

/*
 * The 'H' chips can run internally at either 12MHz or 60MHz.
 * The non-H chips can only run at 12MHz.
 */
static uint8_t clock_5x = 1;

/*
 * In either case, the divisor is a simple integer clock divider.
 * If clock_5x is set, this divisor divides 30MHz, else it divides 6MHz.
 */
#define DIVIDE_BY 3  /* e.g. '3' will give either 10MHz or 2MHz SPI clock. */

#define BITMODE_BITBANG_NORMAL	1
#define BITMODE_BITBANG_SPI	2

/* Set data bits low-byte command:
 *  value: 0x08  CS=high, DI=low, DO=low, SK=low
 *    dir: 0x0b  CS=output, DI=input, DO=output, SK=output
 *
 * JTAGkey(2) needs to enable its output via Bit4 / GPIOL0
 *  value: 0x18  OE=high, CS=high, DI=low, DO=low, SK=low
 *    dir: 0x1b  OE=output, CS=output, DI=input, DO=output, SK=output
 */
static uint8_t cs_bits = 0x08;
static uint8_t pindir = 0x0b;
static struct ftdi_context ftdic_context;

static const char *get_ft2232_devicename(int ft2232_vid, int ft2232_type)
{
	int i;
	for (i = 0; devs_ft2232spi[i].vendor_name != NULL; i++) {
		if ((devs_ft2232spi[i].device_id == ft2232_type)
			&& (devs_ft2232spi[i].vendor_id == ft2232_vid))
				return devs_ft2232spi[i].device_name;
	}
	return "unknown device";
}

static const char *get_ft2232_vendorname(int ft2232_vid, int ft2232_type)
{
	int i;
	for (i = 0; devs_ft2232spi[i].vendor_name != NULL; i++) {
		if ((devs_ft2232spi[i].device_id == ft2232_type)
			&& (devs_ft2232spi[i].vendor_id == ft2232_vid))
				return devs_ft2232spi[i].vendor_name;
	}
	return "unknown vendor";
}

static int send_buf(struct ftdi_context *ftdic, const unsigned char *buf,
		    int size)
{
	int r;
	r = ftdi_write_data(ftdic, (unsigned char *) buf, size);
	if (r < 0) {
		msg_perr("ftdi_write_data: %d, %s\n", r,
				ftdi_get_error_string(ftdic));
		return 1;
	}
	return 0;
}

static int get_buf(struct ftdi_context *ftdic, const unsigned char *buf,
		   int size)
{
	int r;

	while (size > 0) {
		r = ftdi_read_data(ftdic, (unsigned char *) buf, size);
		if (r < 0) {
			msg_perr("ftdi_read_data: %d, %s\n", r,
					ftdi_get_error_string(ftdic));
			return 1;
		}
		buf += r;
		size -= r;
	}
	return 0;
}

static int ft2232_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr);

static const struct spi_programmer spi_programmer_ft2232 = {
	.type		= SPI_CONTROLLER_FT2232,
	.max_data_read	= 64 * 1024,
	.max_data_write	= 256,
	.command	= ft2232_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
};

/* Returns 0 upon success, a negative number upon errors. */
int ft2232_spi_init(void)
{
	int f, ret = 0;
	struct ftdi_context *ftdic = &ftdic_context;
	unsigned char buf[512];
	int ft2232_vid = FTDI_VID;
	int ft2232_type = FTDI_FT4232H_PID;
	enum ftdi_interface ft2232_interface = INTERFACE_B;
	char *arg;
	double mpsse_clk;

	arg = extract_programmer_param("type");
	if (arg) {
		if (!strcasecmp(arg, "2232H"))
			ft2232_type = FTDI_FT2232H_PID;
		else if (!strcasecmp(arg, "4232H"))
			ft2232_type = FTDI_FT4232H_PID;
		else if (!strcasecmp(arg, "jtagkey")) {
			ft2232_type = AMONTEC_JTAGKEY_PID;
			ft2232_interface = INTERFACE_A;
			cs_bits = 0x18;
			pindir = 0x1b;
		} else if (!strcasecmp(arg, "picotap")) {
			ft2232_vid = GOEPEL_VID;
			ft2232_type = GOEPEL_PICOTAP_PID;
			ft2232_interface = INTERFACE_A;
		} else if (!strcasecmp(arg, "tumpa")) {
			/* Interface A is SPI1, B is SPI2. */
			ft2232_type = TIAO_TUMPA_PID;
			ft2232_interface = INTERFACE_A;
		} else if (!strcasecmp(arg, "busblaster")) {
			/* In its default configuration it is a jtagkey clone */
			ft2232_type = FTDI_FT2232H_PID;
			ft2232_interface = INTERFACE_A;
			cs_bits = 0x18;
			pindir = 0x1b;
		} else if (!strcasecmp(arg, "openmoko")) {
			ft2232_vid = FIC_VID;
			ft2232_type = OPENMOKO_DBGBOARD_PID;
			ft2232_interface = INTERFACE_A;
		} else if (!strcasecmp(arg, "arm-usb-ocd")) {
			ft2232_vid = OLIMEX_VID;
			ft2232_type = OLIMEX_ARM_OCD_PID;
			ft2232_interface = INTERFACE_A;
		} else if (!strcasecmp(arg, "arm-usb-tiny")) {
			ft2232_vid = OLIMEX_VID;
			ft2232_type = OLIMEX_ARM_TINY_PID;
			ft2232_interface = INTERFACE_A;
		} else if (!strcasecmp(arg, "arm-usb-ocd-h")) {
			ft2232_vid = OLIMEX_VID;
			ft2232_type = OLIMEX_ARM_OCD_H_PID;
			ft2232_interface = INTERFACE_A;
		} else if (!strcasecmp(arg, "arm-usb-tiny-h")) {
			ft2232_vid = OLIMEX_VID;
			ft2232_type = OLIMEX_ARM_TINY_H_PID;
			ft2232_interface = INTERFACE_A;
		} else {
			msg_perr("Error: Invalid device type specified.\n");
			free(arg);
			return -1;
		}
	}
	free(arg);
	arg = extract_programmer_param("port");
	if (arg) {
		switch (toupper((unsigned char)*arg)) {
		case 'A':
			ft2232_interface = INTERFACE_A;
			break;
		case 'B':
			ft2232_interface = INTERFACE_B;
			break;
		default:
			msg_perr("Error: Invalid port/interface specified.\n");
			free(arg);
			return -2;
		}
	}
	free(arg);
	msg_pdbg("Using device type %s %s ",
		 get_ft2232_vendorname(ft2232_vid, ft2232_type),
		 get_ft2232_devicename(ft2232_vid, ft2232_type));
	msg_pdbg("interface %s\n",
		 (ft2232_interface == INTERFACE_A) ? "A" : "B");

	if (ftdi_init(ftdic) < 0) {
		msg_perr("ftdi_init failed\n");
		return -3;
	}

	f = ftdi_usb_open(ftdic, ft2232_vid, ft2232_type);

	if (f < 0 && f != -5) {
		msg_perr("Unable to open FTDI device: %d (%s)\n", f,
				ftdi_get_error_string(ftdic));
		return -4;
	}

	if (ftdic->type != TYPE_2232H && ftdic->type != TYPE_4232H) {
		msg_pdbg("FTDI chip type %d is not high-speed\n",
			ftdic->type);
		clock_5x = 0;
	}

	if (ftdi_set_interface(ftdic, ft2232_interface) < 0) {
		msg_perr("Unable to select interface: %s\n",
				ftdic->error_str);
	}

	if (ftdi_usb_reset(ftdic) < 0) {
		msg_perr("Unable to reset FTDI device\n");
	}

	if (ftdi_set_latency_timer(ftdic, 2) < 0) {
		msg_perr("Unable to set latency timer\n");
	}

	if (ftdi_write_data_set_chunksize(ftdic, 256)) {
		msg_perr("Unable to set chunk size\n");
	}

	if (ftdi_set_bitmode(ftdic, 0x00, BITMODE_BITBANG_SPI) < 0) {
		msg_perr("Unable to set bitmode to SPI\n");
	}

	if (clock_5x) {
		msg_pdbg("Disable divide-by-5 front stage\n");
		buf[0] = 0x8a;		/* Disable divide-by-5. */
		if (send_buf(ftdic, buf, 1)) {
			ret = -5;
			goto ftdi_err;
		}
		mpsse_clk = 60.0;
	} else {
		mpsse_clk = 12.0;
	}

	msg_pdbg("Set clock divisor\n");
	buf[0] = 0x86;		/* command "set divisor" */
	/* valueL/valueH are (desired_divisor - 1) */
	buf[1] = (DIVIDE_BY - 1) & 0xff;
	buf[2] = ((DIVIDE_BY - 1) >> 8) & 0xff;
	if (send_buf(ftdic, buf, 3)) {
		ret = -6;
		goto ftdi_err;
	}

	msg_pdbg("MPSSE clock: %f MHz divisor: %d "
		 "SPI clock: %f MHz\n", mpsse_clk, DIVIDE_BY,
		 (double)(mpsse_clk / (((DIVIDE_BY - 1) + 1) * 2)));

	/* Disconnect TDI/DO to TDO/DI for loopback. */
	msg_pdbg("No loopback of TDI/DO TDO/DI\n");
	buf[0] = 0x85;
	if (send_buf(ftdic, buf, 1)) {
		ret = -7;
		goto ftdi_err;
	}

	msg_pdbg("Set data bits\n");
	buf[0] = SET_BITS_LOW;
	buf[1] = cs_bits;
	buf[2] = pindir;
	if (send_buf(ftdic, buf, 3)) {
		ret = -8;
		goto ftdi_err;
	}

	// msg_pdbg("\nft2232 chosen\n");

	register_spi_programmer(&spi_programmer_ft2232);

	return 0;

ftdi_err:
	if ((f = ftdi_usb_close(ftdic)) < 0) {
		msg_perr("Unable to close FTDI device: %d (%s)\n", f,
		         ftdi_get_error_string(ftdic));
	}

	return ret;
}

/* Returns 0 upon success, a negative number upon errors. */
static int ft2232_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr)
{
	struct ftdi_context *ftdic = &ftdic_context;
	static unsigned char *buf = NULL;
	/* failed is special. We use bitwise ops, but it is essentially bool. */
	int i = 0, ret = 0, failed = 0;
	int bufsize;
	static int oldbufsize = 0;

	if (writecnt > 65536 || readcnt > 65536)
		return SPI_INVALID_LENGTH;

	/* buf is not used for the response from the chip. */
	bufsize = max(writecnt + 9, 260 + 9);
	/* Never shrink. realloc() calls are expensive. */
	if (bufsize > oldbufsize) {
		buf = realloc(buf, bufsize);
		if (!buf) {
			msg_perr("Out of memory!\n");
			/* TODO: What to do with buf? */
			return SPI_GENERIC_ERROR;
		}
		oldbufsize = bufsize;
	}

	/*
	 * Minimize USB transfers by packing as many commands as possible
	 * together. If we're not expecting to read, we can assert CS#, write,
	 * and deassert CS# all in one shot. If reading, we do three separate
	 * operations.
	 */
	msg_pspew("Assert CS#\n");
	buf[i++] = SET_BITS_LOW;
	buf[i++] = 0 & ~cs_bits; /* assertive */
	buf[i++] = pindir;

	if (writecnt) {
		buf[i++] = 0x11;
		buf[i++] = (writecnt - 1) & 0xff;
		buf[i++] = ((writecnt - 1) >> 8) & 0xff;
		memcpy(buf + i, writearr, writecnt);
		i += writecnt;
	}

	/*
	 * Optionally terminate this batch of commands with a
	 * read command, then do the fetch of the results.
	 */
	if (readcnt) {
		buf[i++] = 0x20;
		buf[i++] = (readcnt - 1) & 0xff;
		buf[i++] = ((readcnt - 1) >> 8) & 0xff;
		ret = send_buf(ftdic, buf, i);
		failed = ret;
		/* We can't abort here, we still have to deassert CS#. */
		if (ret)
			msg_perr("send_buf failed before read: %i\n", ret);
		i = 0;
		if (ret == 0) {
			/*
			 * FIXME: This is unreliable. There's no guarantee that
			 * we read the response directly after sending the read
			 * command. We may be scheduled out etc.
			 */
			ret = get_buf(ftdic, readarr, readcnt);
			failed |= ret;
			/* We can't abort here either. */
			if (ret)
				msg_perr("get_buf failed: %i\n", ret);
		}
	}

	msg_pspew("De-assert CS#\n");
	buf[i++] = SET_BITS_LOW;
	buf[i++] = cs_bits;
	buf[i++] = pindir;
	ret = send_buf(ftdic, buf, i);
	failed |= ret;
	if (ret)
		msg_perr("send_buf failed at end: %i\n", ret);

	return failed ? -1 : 0;
}

void print_supported_usbdevs(const struct usbdev_status *devs)
{
	int i;

	msg_pinfo("USB devices:\n");
	for (i = 0; devs[i].vendor_name != NULL; i++) {
		msg_pinfo("%s %s [%04x:%04x]%s\n", devs[i].vendor_name,
			  devs[i].device_name, devs[i].vendor_id,
			  devs[i].device_id,
			  (devs[i].status == NT) ? " (untested)" : "");
	}
}

#endif
