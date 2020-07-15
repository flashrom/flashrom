/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Carl-Daniel Hailfinger
 * Copyright (C) 2014 Justin Chevrier
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

/*
 * Connections are as follows:
 *
 *      +------+-----+----------+
 *      | SPI  | Pin | PICkit2  |
 *      +------+-----+----------+
 *      | /CS  | 1   | VPP/MCLR |
 *      | VCC  | 2   | VDD      |
 *      | GND  | 3   | GND      |
 *      | MISO | 4   | PGD      |
 *      | SCLK | 5   | PDC      |
 *      | MOSI | 6   | AUX      |
 *      +------+-----+----------+
 *
 * Inspiration and some specifics of the interface came via the AVRDude
 * PICkit2 code: https://github.com/steve-m/avrdude/blob/master/pickit2.c
 */

#include "platform.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <libusb.h>

#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

const struct dev_entry devs_pickit2_spi[] = {
	{0x04D8, 0x0033, OK, "Microchip", "PICkit 2"},

	{0}
};

static libusb_device_handle *pickit2_handle;

/* Default USB transaction timeout in ms */
#define DFLT_TIMEOUT            10000

#define CMD_LENGTH              64
#define ENDPOINT_OUT            0x01
#define ENDPOINT_IN             0x81

#define CMD_GET_VERSION         0x76
#define CMD_SET_VDD             0xA0
#define CMD_SET_VPP             0xA1
#define CMD_READ_VDD_VPP        0xA3
#define CMD_EXEC_SCRIPT         0xA6
#define CMD_CLR_DLOAD_BUFF      0xA7
#define CMD_DOWNLOAD_DATA       0xA8
#define CMD_CLR_ULOAD_BUFF      0xA9
#define CMD_UPLOAD_DATA         0xAA
#define CMD_END_OF_BUFFER       0xAD

#define SCR_SPI_READ_BUF        0xC5
#define SCR_SPI_WRITE_BUF       0xC6
#define SCR_SET_AUX             0xCF
#define SCR_LOOP                0xE9
#define SCR_SET_ICSP_CLK_PERIOD 0xEA
#define SCR_SET_PINS            0xF3
#define SCR_BUSY_LED_OFF        0xF4
#define SCR_BUSY_LED_ON         0xF5
#define SCR_MCLR_GND_OFF        0xF6
#define SCR_MCLR_GND_ON         0xF7
#define SCR_VPP_PWM_OFF         0xF8
#define SCR_VPP_PWM_ON          0xF9
#define SCR_VPP_OFF             0xFA
#define SCR_VPP_ON              0xFB
#define SCR_VDD_OFF             0xFE
#define SCR_VDD_ON              0xFF

static int pickit2_get_firmware_version(void)
{
	int ret;
	uint8_t command[CMD_LENGTH] = {CMD_GET_VERSION, CMD_END_OF_BUFFER};
	int transferred;

	ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, command, CMD_LENGTH, &transferred, DFLT_TIMEOUT);

	if (ret != 0) {
		msg_perr("Command Get Firmware Version failed!\n");
		return 1;
	}

	ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_IN, command, CMD_LENGTH, &transferred, DFLT_TIMEOUT);

	if (ret != 0) {
		msg_perr("Command Get Firmware Version failed!\n");
		return 1;
	}

	msg_pdbg("PICkit2 Firmware Version: %d.%d\n", (int)command[0], (int)command[1]);
	return 0;
}

static int pickit2_set_spi_voltage(int millivolt)
{
	double voltage_selector;
	switch (millivolt) {
	case 0:
		/* Admittedly this one is an assumption. */
		voltage_selector = 0;
		break;
	case 1800:
		voltage_selector = 1.8;
		break;
	case 2500:
		voltage_selector = 2.5;
		break;
	case 3500:
		voltage_selector = 3.5;
		break;
	default:
		msg_perr("Unknown voltage %i mV! Aborting.\n", millivolt);
		return 1;
	}
	msg_pdbg("Setting SPI voltage to %u.%03u V\n", millivolt / 1000,
		 millivolt % 1000);

	uint8_t command[CMD_LENGTH] = {
		CMD_SET_VDD,
		voltage_selector * 2048 + 672,
		(voltage_selector * 2048 + 672) / 256,
		voltage_selector * 36,
		CMD_SET_VPP,
		0x40,
		voltage_selector * 18.61,
		voltage_selector * 13,
		CMD_END_OF_BUFFER
	};
	int transferred;
	int ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, command, CMD_LENGTH, &transferred, DFLT_TIMEOUT);

	if (ret != 0) {
		msg_perr("Command Set Voltage failed!\n");
		return 1;
	}

	return 0;
}

struct pickit2_spispeeds {
	const char *const name;
	const int speed;
};

static const struct pickit2_spispeeds spispeeds[] = {
	{ "1M",		0x1 },
	{ "500k",	0x2 },
	{ "333k",	0x3 },
	{ "250k",	0x4 },
	{ NULL,		0x0 },
};

static int pickit2_set_spi_speed(unsigned int spispeed_idx)
{
	msg_pdbg("SPI speed is %sHz\n", spispeeds[spispeed_idx].name);

	uint8_t command[CMD_LENGTH] = {
		CMD_EXEC_SCRIPT,
		2,
		SCR_SET_ICSP_CLK_PERIOD,
		spispeed_idx,
		CMD_END_OF_BUFFER
	};

	int transferred;
	int ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, command, CMD_LENGTH, &transferred, DFLT_TIMEOUT);

	if (ret != 0) {
		msg_perr("Command Set SPI Speed failed!\n");
		return 1;
	}

	return 0;
}

static int pickit2_spi_send_command(struct flashctx *flash, unsigned int writecnt, unsigned int readcnt,
				     const unsigned char *writearr, unsigned char *readarr)
{

	/* Maximum number of bytes per transaction (including command overhead) is 64. Lets play it safe
	 * and always assume the worst case scenario of 20 bytes command overhead.
	 */
	if (writecnt + readcnt + 20 > CMD_LENGTH) {
		msg_perr("\nTotal packetsize (%i) is greater than 64 supported, aborting.\n",
			 writecnt + readcnt + 20);
		return 1;
	}

	uint8_t buf[CMD_LENGTH] = {CMD_DOWNLOAD_DATA, writecnt};
	unsigned int i = 2;
	for (; i < writecnt + 2; i++) {
		buf[i] = writearr[i - 2];
	}

	buf[i++] = CMD_CLR_ULOAD_BUFF;
	buf[i++] = CMD_EXEC_SCRIPT;

	/* Determine script length based on number of bytes to be read or written */
	if (writecnt == 1 && readcnt == 1)
		buf[i++] = 7;
	else if (writecnt == 1 || readcnt == 1)
		buf[i++] = 10;
	else
		buf[i++] = 13;

	/* Assert CS# */
	buf[i++] = SCR_VPP_OFF;
	buf[i++] = SCR_MCLR_GND_ON;

	buf[i++] = SCR_SPI_WRITE_BUF;

	if (writecnt > 1) {
		buf[i++] = SCR_LOOP;
		buf[i++] = 1; /* Loop back one instruction */
		buf[i++] = writecnt - 1; /* Number of times to loop */
	}

	if (readcnt)
		buf[i++] = SCR_SPI_READ_BUF;

	if (readcnt > 1) {
		buf[i++] = SCR_LOOP;
		buf[i++] = 1; /* Loop back one instruction */
		buf[i++] = readcnt - 1; /* Number of times to loop */
	}

	/* De-assert CS# */
	buf[i++] = SCR_MCLR_GND_OFF;
	buf[i++] = SCR_VPP_PWM_ON;
	buf[i++] = SCR_VPP_ON;

	buf[i++] = CMD_UPLOAD_DATA;
	buf[i++] = CMD_END_OF_BUFFER;

	int transferred;
	int ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, buf, CMD_LENGTH, &transferred, DFLT_TIMEOUT);

	if (ret != 0) {
		msg_perr("Send SPI failed!\n");
		return 1;
	}

	if (readcnt) {
		int length = 0;
		ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_IN, buf, CMD_LENGTH, &length, DFLT_TIMEOUT);

		if (length == 0 || ret != 0) {
			msg_perr("Receive SPI failed\n");
			return 1;
		}

		/* First byte indicates number of bytes transferred from upload buffer */
		if (buf[0] != readcnt) {
			msg_perr("Unexpected number of bytes transferred, expected %i, got %i!\n",
				 readcnt, ret);
			return 1;
		}

		/* Actual data starts at byte number two */
		memcpy(readarr, &buf[1], readcnt);
	}

	return 0;
}

/* Copied from dediprog.c */
/* Might be useful for other USB devices as well. static for now. */
static int parse_voltage(char *voltage)
{
	char *tmp = NULL;
	int i;
	int millivolt = 0, fraction = 0;

	if (!voltage || !strlen(voltage)) {
		msg_perr("Empty voltage= specified.\n");
		return -1;
	}
	millivolt = (int)strtol(voltage, &tmp, 0);
	voltage = tmp;
	/* Handle "," and "." as decimal point. Everything after it is assumed
	 * to be in decimal notation.
	 */
	if ((*voltage == '.') || (*voltage == ',')) {
		voltage++;
		for (i = 0; i < 3; i++) {
			fraction *= 10;
			/* Don't advance if the current character is invalid,
			 * but continue multiplying.
			 */
			if ((*voltage < '0') || (*voltage > '9'))
				continue;
			fraction += *voltage - '0';
			voltage++;
		}
		/* Throw away remaining digits. */
		voltage += strspn(voltage, "0123456789");
	}
	/* The remaining string must be empty or "mV" or "V". */
	tolower_string(voltage);

	/* No unit or "V". */
	if ((*voltage == '\0') || !strncmp(voltage, "v", 1)) {
		millivolt *= 1000;
		millivolt += fraction;
	} else if (!strncmp(voltage, "mv", 2) ||
		   !strncmp(voltage, "millivolt", 9)) {
		/* No adjustment. fraction is discarded. */
	} else {
		/* Garbage at the end of the string. */
		msg_perr("Garbage voltage= specified.\n");
		return -1;
	}
	return millivolt;
}

static const struct spi_master spi_master_pickit2 = {
	.max_data_read	= 40,
	.max_data_write	= 40,
	.command	= pickit2_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

static int pickit2_shutdown(void *data)
{
	/* Set all pins to float and turn voltages off */
	uint8_t command[CMD_LENGTH] = {
		CMD_EXEC_SCRIPT,
		8,
		SCR_SET_PINS,
		3, /* Bit-0=1(PDC In), Bit-1=1(PGD In), Bit-2=0(PDC LL), Bit-3=0(PGD LL) */
		SCR_SET_AUX,
		1, /* Bit-0=1(Aux In), Bit-1=0(Aux LL) */
		SCR_MCLR_GND_OFF,
		SCR_VPP_OFF,
		SCR_VDD_OFF,
		SCR_BUSY_LED_OFF,
		CMD_END_OF_BUFFER
	};

	int transferred;
	int ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, command, CMD_LENGTH, &transferred, DFLT_TIMEOUT);

	if (ret != 0) {
		msg_perr("Command Shutdown failed!\n");
		ret = 1;
	}
	if (libusb_release_interface(pickit2_handle, 0) != 0) {
		msg_perr("Could not release USB interface!\n");
		ret = 1;
	}
	libusb_close(pickit2_handle);
	libusb_exit(NULL);
	return ret;
}

int pickit2_spi_init(void)
{
	uint8_t buf[CMD_LENGTH] = {
		CMD_EXEC_SCRIPT,
		10,			/* Script length */
		SCR_SET_PINS,
		2, /* Bit-0=0(PDC Out), Bit-1=1(PGD In), Bit-2=0(PDC LL), Bit-3=0(PGD LL) */
		SCR_SET_AUX,
		0, /* Bit-0=0(Aux Out), Bit-1=0(Aux LL) */
		SCR_VDD_ON,
		SCR_MCLR_GND_OFF,	/* Let CS# float */
		SCR_VPP_PWM_ON,
		SCR_VPP_ON,		/* Pull CS# high */
		SCR_BUSY_LED_ON,
		CMD_CLR_DLOAD_BUFF,
		CMD_CLR_ULOAD_BUFF,
		CMD_END_OF_BUFFER
	};


	int spispeed_idx = 0;
	char *spispeed = extract_programmer_param("spispeed");
	if (spispeed != NULL) {
		int i = 0;
		for (; spispeeds[i].name; i++) {
			if (strcasecmp(spispeeds[i].name, spispeed) == 0) {
				spispeed_idx = i;
				break;
			}
		}
		if (spispeeds[i].name == NULL) {
			msg_perr("Error: Invalid 'spispeed' value.\n");
			free(spispeed);
			return 1;
		}
		free(spispeed);
	}

	int millivolt = 3500;
	char *voltage = extract_programmer_param("voltage");
	if (voltage != NULL) {
		millivolt = parse_voltage(voltage);
		free(voltage);
		if (millivolt < 0)
			return 1;
	}

	if (libusb_init(NULL) < 0) {
		msg_perr("Couldn't initialize libusb!\n");
		return -1;
	}

#if LIBUSB_API_VERSION < 0x01000106
	libusb_set_debug(NULL, 3);
#else
	libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
#endif

	const uint16_t vid = devs_pickit2_spi[0].vendor_id;
	const uint16_t pid = devs_pickit2_spi[0].device_id;
	pickit2_handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (pickit2_handle == NULL) {
		msg_perr("Could not open device PICkit2!\n");
		libusb_exit(NULL);
		return 1;
	}

	if (libusb_set_configuration(pickit2_handle, 1) != 0) {
		msg_perr("Could not set USB device configuration.\n");
		libusb_close(pickit2_handle);
		libusb_exit(NULL);
		return 1;
	}
	if (libusb_claim_interface(pickit2_handle, 0) != 0) {
		msg_perr("Could not claim USB device interface\n");
		libusb_close(pickit2_handle);
		libusb_exit(NULL);
		return 1;
	}

	if (register_shutdown(pickit2_shutdown, NULL) != 0) {
		return 1;
	}

	if (pickit2_get_firmware_version()) {
		return 1;
	}

	/* Command Set SPI Speed */
	if (pickit2_set_spi_speed(spispeed_idx)) {
		return 1;
	}

	/* Command Set SPI Voltage */
	msg_pdbg("Setting voltage to %i mV.\n", millivolt);
	if (pickit2_set_spi_voltage(millivolt) != 0) {
		return 1;
	}

	/* Perform basic setup.
	 * Configure pin directions and logic levels, turn Vdd on, turn busy LED on and clear buffers. */
	int transferred;
	if (libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, buf, CMD_LENGTH, &transferred, DFLT_TIMEOUT) != 0) {
		msg_perr("Command Setup failed!\n");
		return 1;
	}

	register_spi_master(&spi_master_pickit2);

	return 0;
}
