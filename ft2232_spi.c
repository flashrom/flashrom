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
 */

#include <stdbool.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "flash.h"
#include "programmer.h"
#include "spi.h"
#include <ftdi.h>

/* This is not defined in libftdi.h <0.20 (c7e4c09e68cfa6f5e112334aa1b3bb23401c8dc7 to be exact).
 * Some tests indicate that this is the only change that it is needed to support the FT232H in flashrom. */
#if !defined(HAVE_FT232H)
#define TYPE_232H	6
#endif

/* Please keep sorted by vendor ID, then device ID. */

#define FTDI_VID		0x0403
#define FTDI_FT2232H_PID	0x6010
#define FTDI_FT4232H_PID	0x6011
#define FTDI_FT232H_PID		0x6014
#define FTDI_FT4233H_PID	0x6041
#define TIAO_TUMPA_PID		0x8a98
#define TIAO_TUMPA_LITE_PID	0x8a99
#define KT_LINK_PID		0xbbe2
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

#define GOOGLE_VID		0x18D1
#define GOOGLE_SERVO_PID	0x5001
#define GOOGLE_SERVO_V2_PID0	0x5002
#define GOOGLE_SERVO_V2_PID1	0x5003

static const struct dev_entry devs_ft2232spi[] = {
	{FTDI_VID, FTDI_FT2232H_PID, OK, "FTDI", "FT2232H"},
	{FTDI_VID, FTDI_FT4232H_PID, OK, "FTDI", "FT4232H"},
	{FTDI_VID, FTDI_FT232H_PID, OK, "FTDI", "FT232H"},
	{FTDI_VID, FTDI_FT4233H_PID, OK, "FTDI", "FT4233H"},
	{FTDI_VID, TIAO_TUMPA_PID, OK, "TIAO", "USB Multi-Protocol Adapter"},
	{FTDI_VID, TIAO_TUMPA_LITE_PID, OK, "TIAO", "USB Multi-Protocol Adapter Lite"},
	{FTDI_VID, KT_LINK_PID, OK, "Kristech", "KT-LINK"},
	{FTDI_VID, AMONTEC_JTAGKEY_PID, OK, "Amontec", "JTAGkey"},
	{GOEPEL_VID, GOEPEL_PICOTAP_PID, OK, "GOEPEL", "PicoTAP"},
	{GOOGLE_VID, GOOGLE_SERVO_PID, OK, "Google", "Servo"},
	{GOOGLE_VID, GOOGLE_SERVO_V2_PID0, OK, "Google", "Servo V2 Legacy"},
	{GOOGLE_VID, GOOGLE_SERVO_V2_PID1, OK, "Google", "Servo V2"},
	{FIC_VID, OPENMOKO_DBGBOARD_PID, OK, "FIC", "OpenMoko Neo1973 Debug board (V2+)"},
	{OLIMEX_VID, OLIMEX_ARM_OCD_PID, OK, "Olimex", "ARM-USB-OCD"},
	{OLIMEX_VID, OLIMEX_ARM_TINY_PID, OK, "Olimex", "ARM-USB-TINY"},
	{OLIMEX_VID, OLIMEX_ARM_OCD_H_PID, OK, "Olimex", "ARM-USB-OCD-H"},
	{OLIMEX_VID, OLIMEX_ARM_TINY_H_PID, OK, "Olimex", "ARM-USB-TINY-H"},

	{0},
};

#define FTDI_HW_BUFFER_SIZE 4096 /* in bytes */

#define DEFAULT_DIVISOR 2

#define BITMODE_BITBANG_NORMAL	1
#define BITMODE_BITBANG_SPI	2

/*
 * The variables `cs_bits` and `pindir` store the values for the
 * "set data bits low byte" MPSSE command that sets the initial
 * state and the direction of the I/O pins. `cs_bits` pins default
 * to high and will be toggled during SPI transactions. All other
 * output pins will be kept low all the time. For some programmers,
 * some reserved GPIOL* pins are used as outputs. Free GPIOL* pins
 * are configured as inputs, while it's possible to use them either
 * as generic gpios or as additional CS# signal(s) through the
 * parameter(s) `gpiolX`. On exit, all pins will be reconfigured
 * as inputs.
 *
 * The pin offsets are as follows:
 * TCK/SK is bit 0.
 * TDI/DO is bit 1.
 * TDO/DI is bit 2.
 * TMS/CS is bit 3.
 * GPIOL0 is bit 4.
 * GPIOL1 is bit 5.
 * GPIOL2 is bit 6.
 * GPIOL3 is bit 7.
 *
 * The default values (set below in ft2232_spi_init) are used for
 * most devices:
 *  value: 0x08  CS=high,   DI=low,   DO=low,    SK=low
 *    dir: 0x0b  CS=output, DI=input, DO=output, SK=output
 */
struct ft2232_data {
	uint8_t cs_bits;
	uint8_t aux_bits;
	uint8_t pindir;
	struct ftdi_context ftdic_context;
};

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

static int ft2232_shutdown(void *data)
{
	struct ft2232_data *spi_data = (struct ft2232_data *) data;
	struct ftdi_context *ftdic = &spi_data->ftdic_context;
	unsigned char buf[3];
	int ret = 0;

	msg_pdbg("Releasing I/Os\n");
	buf[0] = SET_BITS_LOW;
	buf[1] = 0; /* Output byte ignored */
	buf[2] = 0; /* Pin direction: all inputs */
	if (send_buf(ftdic, buf, 3)) {
		msg_perr("Unable to set pins back to inputs.\n");
		ret = 1;
	}

	const int close_ret = ftdi_usb_close(ftdic);
	if (close_ret < 0) {
		msg_perr("Unable to close FTDI device: %d (%s)\n", close_ret,
		         ftdi_get_error_string(ftdic));
		ret = 1;
	}

	free(spi_data);
	return ret;
}

static bool ft2232_spi_command_fits(const struct spi_command *cmd, size_t buffer_size)
{
	const size_t cmd_len = 3; /* same length for any ft2232 command */
	return
		/* commands for CS# assertion and de-assertion: */
		cmd_len + cmd_len
		/* commands for either a write, a read or both: */
		+ (cmd->writecnt && cmd->readcnt ? cmd_len + cmd_len : cmd_len)
		/* payload (only writecnt; readcnt concerns another buffer): */
		+ cmd->writecnt
		<= buffer_size;
}

/* Returns 0 upon success, a negative number upon errors. */
static int ft2232_spi_send_multicommand(const struct flashctx *flash, struct spi_command *cmds)
{
	struct ft2232_data *spi_data = flash->mst->spi.data;
	struct ftdi_context *ftdic = &spi_data->ftdic_context;
	static unsigned char buf[FTDI_HW_BUFFER_SIZE];
	size_t i = 0;
	int ret = 0;

	/*
	 * Minimize FTDI-calls by packing as many commands as possible together.
	 */
	for (; cmds->writecnt || cmds->readcnt; cmds++) {

		if (cmds->writecnt > 65536 || cmds->readcnt > 65536)
			return SPI_INVALID_LENGTH;

		if (!ft2232_spi_command_fits(cmds, FTDI_HW_BUFFER_SIZE - i)) {
			msg_perr("Command does not fit\n");
			return SPI_GENERIC_ERROR;
		}

		msg_pspew("Assert CS#\n");
		buf[i++] = SET_BITS_LOW;
		/* assert CS# pins, keep aux_bits, all other output pins stay low */
		buf[i++] = spi_data->aux_bits;
		buf[i++] = spi_data->pindir;

		/* WREN, OP(PROGRAM, ERASE), ADDR, DATA */
		if (cmds->writecnt) {
			buf[i++] = MPSSE_DO_WRITE | MPSSE_WRITE_NEG;
			buf[i++] = (cmds->writecnt - 1) & 0xff;
			buf[i++] = ((cmds->writecnt - 1) >> 8) & 0xff;
			memcpy(buf + i, cmds->writearr, cmds->writecnt);
			i += cmds->writecnt;
		}

		/* An optional read command */
		if (cmds->readcnt) {
			buf[i++] = MPSSE_DO_READ;
			buf[i++] = (cmds->readcnt - 1) & 0xff;
			buf[i++] = ((cmds->readcnt - 1) >> 8) & 0xff;
		}

		/* Add final de-assert CS# */
		msg_pspew("De-assert CS#\n");
		buf[i++] = SET_BITS_LOW;
		buf[i++] = spi_data->cs_bits | spi_data->aux_bits;
		buf[i++] = spi_data->pindir;

		/* continue if there is no read-cmd and further cmds exist */
		if (!cmds->readcnt &&
				((cmds + 1)->writecnt || (cmds + 1)->readcnt) &&
				ft2232_spi_command_fits((cmds + 1), FTDI_HW_BUFFER_SIZE - i)) {
			continue;
		}

		ret = send_buf(ftdic, buf, i);
		i = 0;
		if (ret) {
			msg_perr("send_buf failed: %i\n", ret);
			break;
		}

		if (cmds->readcnt) {
			ret = get_buf(ftdic, cmds->readarr, cmds->readcnt);
			if (ret) {
				msg_perr("get_buf failed: %i\n", ret);
				break;
			}
		}
	}
	return ret ? -1 : 0;
}

static const struct spi_master spi_master_ft2232 = {
	.features	= SPI_MASTER_4BA,
	.max_data_read	= 64 * 1024,
	.max_data_write	= 256,
	.multicommand	= ft2232_spi_send_multicommand,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.shutdown	= ft2232_shutdown,
};

/* Returns 0 upon success, a negative number upon errors. */
static int ft2232_spi_init(const struct programmer_cfg *cfg)
{
	int ret = 0;
	unsigned char buf[512];
	int ft2232_vid = FTDI_VID;
	int ft2232_type = FTDI_FT4232H_PID;
	int channel_count = 4; /* Stores the number of channels of the device. */
	enum ftdi_interface ft2232_interface = INTERFACE_A;
	/*
	 * The 'H' chips can run with an internal clock of either 12 MHz or 60 MHz,
	 * but the non-H chips can only run at 12 MHz. We disable the divide-by-5
	 * prescaler on 'H' chips so they run at 60MHz.
	 */
	bool clock_5x = true;
	/* In addition to the prescaler mentioned above there is also another
	 * configurable one on all versions of the chips. Its divisor div can be
	 * set by a 16 bit value x according to the following formula:
	 * div = (1 + x) * 2 <-> x = div / 2 - 1
	 * Hence the expressible divisors are all even numbers between 2 and
	 * 2^17 (=131072) resulting in SCK frequencies of 6 MHz down to about
	 * 92 Hz for 12 MHz inputs and 30 MHz down to about 458 Hz for 60 MHz
	 * inputs.
	 */
	uint32_t divisor = DEFAULT_DIVISOR;
	int f;
	char *arg, *arg2;
	double mpsse_clk;

	uint8_t cs_bits = 0x08;
	uint8_t aux_bits = 0x00;
	uint8_t pindir = 0x0b;
	uint8_t aux_bits_high = 0x00;
	uint8_t pindir_high = 0x00;
	struct ftdi_context ftdic;
	struct ft2232_data *spi_data;

	arg = extract_programmer_param_str(cfg, "type");
	if (arg) {
		if (!strcasecmp(arg, "2232H")) {
			ft2232_type = FTDI_FT2232H_PID;
			channel_count = 2;
		} else if (!strcasecmp(arg, "4232H")) {
			ft2232_type = FTDI_FT4232H_PID;
			channel_count = 4;
		} else if (!strcasecmp(arg, "232H")) {
			ft2232_type = FTDI_FT232H_PID;
			channel_count = 1;
		} else if (!strcasecmp(arg, "4233H")) {
			ft2232_type = FTDI_FT4233H_PID;
			channel_count = 4;
		} else if (!strcasecmp(arg, "jtagkey")) {
			ft2232_type = AMONTEC_JTAGKEY_PID;
			channel_count = 2;
			/* JTAGkey(2) needs to enable its output via Bit4 / GPIOL0
			*  value: 0x18  OE=high, CS=high, DI=low, DO=low, SK=low
			*    dir: 0x1b  OE=output, CS=output, DI=input, DO=output, SK=output */
			cs_bits = 0x18;
			pindir = 0x1b;
		} else if (!strcasecmp(arg, "picotap")) {
			ft2232_vid = GOEPEL_VID;
			ft2232_type = GOEPEL_PICOTAP_PID;
			channel_count = 2;
		} else if (!strcasecmp(arg, "tumpa")) {
			/* Interface A is SPI1, B is SPI2. */
			ft2232_type = TIAO_TUMPA_PID;
			channel_count = 2;
		} else if (!strcasecmp(arg, "tumpalite")) {
			/* Only one channel is used on lite edition */
			ft2232_type = TIAO_TUMPA_LITE_PID;
			channel_count = 1;
		} else if (!strcasecmp(arg, "busblaster")) {
			/* In its default configuration it is a jtagkey clone */
			ft2232_type = FTDI_FT2232H_PID;
			channel_count = 2;
			cs_bits = 0x18;
			pindir = 0x1b;
		} else if (!strcasecmp(arg, "openmoko")) {
			ft2232_vid = FIC_VID;
			ft2232_type = OPENMOKO_DBGBOARD_PID;
			channel_count = 2;
		} else if (!strcasecmp(arg, "arm-usb-ocd")) {
			ft2232_vid = OLIMEX_VID;
			ft2232_type = OLIMEX_ARM_OCD_PID;
			channel_count = 2;
			/* arm-usb-ocd(-h) has an output buffer that needs to be enabled by pulling ADBUS4 low.
			*  value: 0x08  #OE=low, CS=high, DI=low, DO=low, SK=low
			*    dir: 0x1b  #OE=output, CS=output, DI=input, DO=output, SK=output */
			cs_bits = 0x08;
			pindir = 0x1b;
		} else if (!strcasecmp(arg, "arm-usb-tiny")) {
			ft2232_vid = OLIMEX_VID;
			ft2232_type = OLIMEX_ARM_TINY_PID;
			channel_count = 2;
		} else if (!strcasecmp(arg, "arm-usb-ocd-h")) {
			ft2232_vid = OLIMEX_VID;
			ft2232_type = OLIMEX_ARM_OCD_H_PID;
			channel_count = 2;
			/* See arm-usb-ocd */
			cs_bits = 0x08;
			pindir = 0x1b;
		} else if (!strcasecmp(arg, "arm-usb-tiny-h")) {
			ft2232_vid = OLIMEX_VID;
			ft2232_type = OLIMEX_ARM_TINY_H_PID;
			channel_count = 2;
		} else if (!strcasecmp(arg, "google-servo")) {
			ft2232_vid = GOOGLE_VID;
			ft2232_type = GOOGLE_SERVO_PID;
		} else if (!strcasecmp(arg, "google-servo-v2")) {
			ft2232_vid = GOOGLE_VID;
			ft2232_type = GOOGLE_SERVO_V2_PID1;
			/* Default divisor is too fast, and chip ID fails */
			divisor = 6;
		} else if (!strcasecmp(arg, "google-servo-v2-legacy")) {
			ft2232_vid = GOOGLE_VID;
			ft2232_type = GOOGLE_SERVO_V2_PID0;
		} else if (!strcasecmp(arg, "flyswatter")) {
			ft2232_type = FTDI_FT2232H_PID;
			channel_count = 2;
			/* Flyswatter and Flyswatter-2 require GPIO bits 0x80
			 * and 0x40 to be driven low to enable output buffers */
			pindir = 0xcb;
		} else if (!strcasecmp(arg, "kt-link")) {
			ft2232_type = KT_LINK_PID;
			/* port B is used as uart */
			channel_count = 1;
			/* Set GPIOL1 output high - route TMS and TDO through multiplexers */
			aux_bits = 0x20;
			pindir = 0x2b;
			/* Set GPIOH4 output low - enable TMS output buffer */
			/* Set GPIOH5 output low - enable TDI output buffer */
			/* Set GPIOH6 output low - enable TCK output buffer */
			pindir_high = 0x70;
		} else {
			msg_perr("Error: Invalid device type specified.\n");
			free(arg);
			return -1;
		}
	}
	free(arg);

	/* Remember reserved pins before pindir gets modified. */
	const uint8_t rsv_bits = pindir & 0xf0;

	arg = extract_programmer_param_str(cfg, "port");
	if (arg) {
		switch (toupper((unsigned char)*arg)) {
		case 'A':
			ft2232_interface = INTERFACE_A;
			break;
		case 'B':
			ft2232_interface = INTERFACE_B;
			if (channel_count < 2)
				channel_count = -1;
			break;
		case 'C':
			ft2232_interface = INTERFACE_C;
			if (channel_count < 3)
				channel_count = -1;
			break;
		case 'D':
			ft2232_interface = INTERFACE_D;
			if (channel_count < 4)
				channel_count = -1;
			break;
		default:
			channel_count = -1;
			break;
		}
		if (channel_count < 0 || strlen(arg) != 1) {
			msg_perr("Error: Invalid channel/port/interface specified: \"%s\".\n", arg);
			free(arg);
			return -2;
		}
	}
	free(arg);

	arg = extract_programmer_param_str(cfg, "divisor");
	if (arg && strlen(arg)) {
		unsigned int temp = 0;
		char *endptr;
		temp = strtoul(arg, &endptr, 10);
		if (*endptr || temp < 2 || temp > 131072 || temp & 0x1) {
			msg_perr("Error: Invalid SPI frequency divisor specified: \"%s\".\n"
				 "Valid are even values between 2 and 131072.\n", arg);
			free(arg);
			return -2;
		}
		divisor = (uint32_t)temp;
	}
	free(arg);

	bool csgpiol_set = false;
	arg = extract_programmer_param_str(cfg, "csgpiol");
	if (arg) {
		csgpiol_set = true;
		msg_pwarn("Deprecation warning: `csgpiol` is deprecated and will be removed "
			 "in the future.\nUse `gpiolX=C` instead.\n");

		char *endptr;
		unsigned int temp = strtoul(arg, &endptr, 10);
		if (*endptr || endptr == arg || temp > 3) {
			msg_perr("Error: Invalid GPIOL specified: \"%s\".\n"
				 "Valid values are between 0 and 3.\n", arg);
			free(arg);
			return -2;
		}

		unsigned int pin = temp + 4;
		if (rsv_bits & 1 << pin) {
			msg_perr("Error: Invalid GPIOL specified: \"%s\".\n"
				 "The pin is reserved on this programmer.\n",
				 arg);
			free(arg);
			return -2;
		}

		cs_bits |= 1 << pin;
		pindir  |= 1 << pin;
	}
	free(arg);

	/* gpiolX */
	for (int pin = 0; pin < 4; pin++) {
		char gpiol_param[7];
		snprintf(gpiol_param, sizeof(gpiol_param), "gpiol%d", pin);
		arg = extract_programmer_param_str(cfg, gpiol_param);

		if (!arg)
			continue;

		if (csgpiol_set) {
			msg_perr("Error: `csgpiol` and `gpiolX` are mutually exclusive.\n"
				 "Since `csgpiol` is deprecated and will be removed in the "
				 "future, use of `gpiolX=C` is recommended.\n");
			free(arg);
			return -2;
		}

		uint8_t bit = 1 << (pin + 4);
		if (rsv_bits & bit) {
			msg_perr("Error: Invalid GPIOL specified: \"gpiol%d=%s\".\n"
				 "Pin GPIOL%i is reserved on this programmer.\n",
				 pin, arg, pin);
			free(arg);
			return -2;
		}

		if (strlen(arg) != 1)
			goto format_error;

		switch (toupper(arg[0])) {
			case 'H':
				aux_bits |= bit;
				pindir   |= bit;
				break;
			case 'L':
				pindir   |= bit;
				break;
			case 'C':
				cs_bits  |= bit;
				pindir   |= bit;
				break;
			default:
				goto format_error;
		}

		free(arg);
		continue;

format_error:
		msg_perr("Error: Invalid GPIOL specified: \"gpiol%d=%s\".\n"
			 "Valid values are H, L and C.\n"
			 "    H - Set GPIOL output high\n"
			 "    L - Set GPIOL output low\n"
			 "    C - Use GPIOL as additional CS# output\n",
			 pin, arg);

		free(arg);
		return -2;
	}

	msg_pdbg("Using device type %s %s ",
		 get_ft2232_vendorname(ft2232_vid, ft2232_type),
		 get_ft2232_devicename(ft2232_vid, ft2232_type));
	msg_pdbg("channel %s.\n",
		 (ft2232_interface == INTERFACE_A) ? "A" :
		 (ft2232_interface == INTERFACE_B) ? "B" :
		 (ft2232_interface == INTERFACE_C) ? "C" : "D");

	if (ftdi_init(&ftdic) < 0) {
		msg_perr("ftdi_init failed.\n");
		return -3;
	}

	if (ftdi_set_interface(&ftdic, ft2232_interface) < 0) {
		msg_perr("Unable to select channel (%s).\n", ftdi_get_error_string(&ftdic));
	}

	arg = extract_programmer_param_str(cfg, "serial");
	arg2 = extract_programmer_param_str(cfg, "description");

	f = ftdi_usb_open_desc(&ftdic, ft2232_vid, ft2232_type, arg2, arg);

	free(arg);
	free(arg2);

	if (f < 0 && f != -5) {
		msg_perr("Unable to open FTDI device: %d (%s)\n", f,
				ftdi_get_error_string(&ftdic));
		return -4;
	}

	if (ftdic.type != TYPE_2232H && ftdic.type != TYPE_4232H && ftdic.type != TYPE_232H) {
		msg_pdbg("FTDI chip type %d is not high-speed.\n", ftdic.type);
		clock_5x = false;
	}

	if (ftdi_usb_reset(&ftdic) < 0) {
		msg_perr("Unable to reset FTDI device (%s).\n", ftdi_get_error_string(&ftdic));
	}

	if (ftdi_set_latency_timer(&ftdic, 2) < 0) {
		msg_perr("Unable to set latency timer (%s).\n", ftdi_get_error_string(&ftdic));
	}

	if (ftdi_set_bitmode(&ftdic, 0x00, BITMODE_BITBANG_SPI) < 0) {
		msg_perr("Unable to set bitmode to SPI (%s).\n", ftdi_get_error_string(&ftdic));
	}

	if (clock_5x) {
		msg_pdbg("Disable divide-by-5 front stage\n");
		buf[0] = DIS_DIV_5;
		if (send_buf(&ftdic, buf, 1)) {
			ret = -5;
			goto ftdi_err;
		}
		mpsse_clk = 60.0;
	} else {
		mpsse_clk = 12.0;
	}

	msg_pdbg("Set clock divisor\n");
	buf[0] = TCK_DIVISOR;
	buf[1] = (divisor / 2 - 1) & 0xff;
	buf[2] = ((divisor / 2 - 1) >> 8) & 0xff;
	if (send_buf(&ftdic, buf, 3)) {
		ret = -6;
		goto ftdi_err;
	}

	msg_pdbg("MPSSE clock: %f MHz, divisor: %u, SPI clock: %f MHz\n",
		 mpsse_clk, divisor, (double)(mpsse_clk / divisor));

	/* Disconnect TDI/DO to TDO/DI for loopback. */
	msg_pdbg("No loopback of TDI/DO TDO/DI\n");
	buf[0] = LOOPBACK_END;
	if (send_buf(&ftdic, buf, 1)) {
		ret = -7;
		goto ftdi_err;
	}

	msg_pdbg("Set data bits\n");
	buf[0] = SET_BITS_LOW;
	buf[1] = cs_bits | aux_bits;
	buf[2] = pindir;
	if (send_buf(&ftdic, buf, 3)) {
		ret = -8;
		goto ftdi_err;
	}

	if (pindir_high) {
		msg_pdbg("Set data bits HighByte\n");
		buf[0] = SET_BITS_HIGH;
		buf[1] = aux_bits_high;
		buf[2] = pindir_high;
		if (send_buf(&ftdic, buf, 3)) {
			ret = -8;
			goto ftdi_err;
		}
	}

	spi_data = calloc(1, sizeof(*spi_data));
	if (!spi_data) {
		msg_perr("Unable to allocate space for SPI master data\n");
		return SPI_GENERIC_ERROR;
	}
	spi_data->cs_bits = cs_bits;
	spi_data->aux_bits = aux_bits;
	spi_data->pindir = pindir;
	spi_data->ftdic_context = ftdic;

	return register_spi_master(&spi_master_ft2232, spi_data);

ftdi_err:
	if ((f = ftdi_usb_close(&ftdic)) < 0) {
		msg_perr("Unable to close FTDI device: %d (%s)\n", f,
		         ftdi_get_error_string(&ftdic));
	}
	return ret;
}

const struct programmer_entry programmer_ft2232_spi = {
	.name			= "ft2232_spi",
	.type			= USB,
	.devs.dev		= devs_ft2232spi,
	.init			= ft2232_spi_init,
};
