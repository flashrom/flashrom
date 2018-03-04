/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Carl-Daniel Hailfinger
 * Copyright (C) 2014 Justin Chevrier
 * Copyright (C) 2017 Tomislav Gotic
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

	{}
};
struct libusb_context *usb_ctx;
static libusb_device_handle *pickit2_handle;

/* Default USB transaction timeout in ms */
#define DFLT_TIMEOUT			1000U
#define MB1						1048576U
#define KB8						8192U
#define PICKIT2_SCLK_MAX_kHz	1000U
#define PICKIT2_Vdd_MIN_mV		2500
#define PICKIT2_Vdd_MAX_mV		5000
#define PICKIT2_ERR_MASK_LO		0x30
#define PICKIT2_ERR_MASK_HI		0xFC

#define ENDPOINT_OUT			0x01
#define ENDPOINT_IN				0x81
#define USB_PACKET_LENGTH		64U		

#define CMD_FIRMWARE_VERSION	0x76
#define CMD_SET_VDD				0xA0
#define CMD_SET_VPP				0xA1
#define CMD_READ_STATUS			0xA2
#define CMD_READ_VDD_VPP		0xA3
#define CMD_DOWNLOAD_SCRIPT		0xA4
#define CMD_RUN_SCRIPT			0xA5
#define CMD_EXEC_SCRIPT			0xA6
#define CMD_CLR_DOWNLOAD_BUFFER	0xA7
#define CMD_DOWNLOAD_DATA		0xA8
#define CMD_CLR_UPLOAD_BUFFER	0xA9
#define CMD_UPLOAD_DATA			0xAA
#define CMD_CLR_SCRIPT_BUFFER	0xAB
#define CMD_UPLOAD_DATA_NO_LEN	0xAC
#define CMD_END_OF_BUFFER		0xAD

#define SCR_SPI_READ_BUFFER		0xC5
#define SCR_SPI_WRITE_BUFFER	0xC6
#define	SCR_SPI_WRITE_LIT		0xC7
#define SCR_SET_AUX				0xCF
#define SCR_DELAY_LONG			0xE8
#define SCR_LOOP				0xE9
#define SCR_SET_ICSP_CLK_PERIOD	0xEA
#define SCR_SET_PINS			0xF3
#define SCR_BUSY_LED_OFF		0xF4
#define SCR_BUSY_LED_ON			0xF5
#define SCR_MCLR_GND_Q_OFF		0xF6
#define SCR_MCLR_GND_Q_ON		0xF7
#define SCR_VPP_PWM_OFF			0xF8
#define SCR_VPP_PWM_ON			0xF9
#define SCR_VPP_Q_OFF			0xFA
#define SCR_VPP_Q_ON			0xFB
#define SCR_VDD_OFF				0xFE
#define SCR_VDD_ON				0xFF

#define RUN_SCR_SPI_CS_HI		1U
#define RUN_SCR_SPI_WRITE		2U
#define RUN_SCR_SPI_READ		3U
#define RUN_SCR_SPI_STATUS_REG	4U
#define RUN_SCR_LED_ON			5U

/* Might be useful for other USB devices as well. static for now.
 * device parameter allows user to specify one device of multiple installed */
static struct libusb_device_handle *get_device_by_vid_pid_number(uint16_t vid, uint16_t pid, unsigned int num)
{
	struct libusb_device **list;
	ssize_t count = libusb_get_device_list(usb_ctx, &list);
	if (count < 0) {
		msg_perr("Getting the USB device list failed (%s)!\n", libusb_error_name(count));
		return NULL;
	}

	struct libusb_device_handle *handle = NULL;
	ssize_t i = 0;
	for (i = 0; i < count; i++) {
		struct libusb_device *dev = list[i];
		struct libusb_device_descriptor desc;
		int err = libusb_get_device_descriptor(dev, &desc);
		if (err != 0) {
			msg_perr("Reading the USB device descriptor failed (%s)!\n", libusb_error_name(err));
			libusb_free_device_list(list, 1);
			return NULL;
		}
		if ((desc.idVendor == vid) && (desc.idProduct == pid)) {
		  int bus = libusb_get_bus_number(dev);
		  int addr = libusb_get_device_address(dev);
			msg_pinfo("Found USB device %04"PRIx16":%04"PRIx16" at address %d-%d.\n",
				 desc.idVendor, desc.idProduct,
				 bus, addr);
			if (num == 0) {
				err = libusb_open(dev, &handle);
				if (err != 0) {
					msg_perr("Opening the USB device failed (%s)!\n",
						 libusb_error_name(err));
					libusb_free_device_list(list, 1);
					return NULL;
				}
				break;
			}
			num--;
		}
	}
	libusb_free_device_list(list, 1);

	return handle;
}

static int pickit2_get_firmware_version(void)
{
	int ret, transferred = 0;
	uint8_t command[USB_PACKET_LENGTH] = {CMD_FIRMWARE_VERSION, CMD_END_OF_BUFFER};

	ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, command, USB_PACKET_LENGTH, &transferred, DFLT_TIMEOUT);
	if (ret == 0) {
		ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_IN, command, USB_PACKET_LENGTH, &transferred, DFLT_TIMEOUT);
		if (ret == 0) {
			msg_pinfo("PICkit2 Firmware Version: %"PRIu8".%"PRIu8".%"PRIu8"\n", command[0], command[1], command[2]);
			return 0;
		}
	}
	msg_perr("PicKit2 Get Firmware Version Command failed (%s)!\n", libusb_error_name(ret));
	return SPI_PROGRAMMER_ERROR;
}

static int pickit2_get_status()
{
	int ret, transferred = 0;
	uint8_t command[USB_PACKET_LENGTH] = {CMD_READ_STATUS, CMD_END_OF_BUFFER};

	ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, command, USB_PACKET_LENGTH, &transferred, DFLT_TIMEOUT);
	if (ret == 0) {
		ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_IN, command, USB_PACKET_LENGTH, &transferred, DFLT_TIMEOUT);
		if (ret == 0) {
			msg_pinfo("PICkit2 status(0x%"PRIx8" 0x%"PRIx8"):", command[0], command[1]);
			if (command[0] & 0x40)
				msg_pinfo("BTN,");
			if (command[0] & 0x20)
				msg_perr("VPP ERR,");
			if (command[0] & 0x10)
				msg_perr("VDD ERR,");
			if (command[0] & 0x08)
				msg_pinfo("/CS Hi,");
			if (command[0] & 0x04)
				msg_pinfo("/CS Low");
			if (command[0] & 0x02)
				msg_pinfo("VDD OFF,");
			if (command[0] & 0x01)
				msg_pinfo("VDD GND,");
			if (command[1] & 0x80)
				msg_perr("DL FULL ERR,");
			if (command[1] & 0x40)
				msg_perr("SCR ERR,");
			if (command[1] & 0x20)
				msg_perr("SCR EMPTY ERR,");
			if (command[1] & 0x10)
				msg_perr("SCR DL EMPTY ERR,");
			if (command[1] & 0x08)
				msg_perr("SCR UL FULL ERR,");
			if (command[1] & 0x04)
				msg_perr("ICD TOUT ERR");
			if (command[1] & 0x02)
				msg_pinfo("UART On,");
			if (command[1] & 0x01)
				msg_pinfo("RESET");
			
			msg_pinfo("\n");

			return 0;
		}
	}
	msg_perr("PicKit2 Read Status Command failed (%s)!\n", libusb_error_name(ret));
	return SPI_PROGRAMMER_ERROR;
}
static int pickit2_check_errors()
{
	int ret, transferred = 0;
	uint8_t command[USB_PACKET_LENGTH] = {
		CMD_READ_STATUS,
		CMD_RUN_SCRIPT,
		RUN_SCR_LED_ON,
		1,
		CMD_END_OF_BUFFER
	};

	ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, command, USB_PACKET_LENGTH, &transferred, DFLT_TIMEOUT);
	if (ret == 0) {
		ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_IN, command, USB_PACKET_LENGTH, &transferred, DFLT_TIMEOUT);
		if (ret == 0) {
			if ((command[0] & PICKIT2_ERR_MASK_LO) || (command[1] & PICKIT2_ERR_MASK_HI)) {
				msg_perr("PicKit Status Errors: 0x%"PRIx8" 0x%"PRIx8"!\n", (command[0] & PICKIT2_ERR_MASK_LO), (command[1] & PICKIT2_ERR_MASK_HI));
				return SPI_PROGRAMMER_ERROR;
			}
			return 0;
		}
	}
	msg_perr("PicKit2 Read Status Command failed (%s)!\n", libusb_error_name(ret));
	return SPI_PROGRAMMER_ERROR;
}

static int pickit2_set_spi_voltage(int millivolt)
{
	double voltage_selector = millivolt / 1000.0;
	if ((millivolt < PICKIT2_Vdd_MIN_mV) || (millivolt > PICKIT2_Vdd_MAX_mV)) {
		msg_perr("Cannot set Vdd to %.3fV! Aborting.\n", voltage_selector);
		return SPI_PROGRAMMER_ERROR;
	}
	msg_pdbg("Setting Vdd voltage to %.3f V\n", voltage_selector);

	uint8_t command[USB_PACKET_LENGTH] = {
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
	int transferred = 0;

	int ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, command, USB_PACKET_LENGTH, &transferred, DFLT_TIMEOUT);

	if (ret != 0) {
		msg_perr("PicKit2 Set SPI Vdd Command failed (%s)!\n", libusb_error_name(ret));
		return SPI_PROGRAMMER_ERROR;
	}

	return 0;
}

static int pickit2_set_spi_speed(unsigned int spispeed)
{
	uint8_t command[USB_PACKET_LENGTH] = {
		CMD_EXEC_SCRIPT,
		2,
		SCR_SET_ICSP_CLK_PERIOD,
		(uint8_t)(PICKIT2_SCLK_MAX_kHz / spispeed),
		CMD_END_OF_BUFFER
	};
	int transferred = 0;
	int ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, command, USB_PACKET_LENGTH, &transferred, DFLT_TIMEOUT);
	
	if (ret != 0) {
		msg_perr("PicKit2 Set SPI Speed Command failed (%s)!\n", libusb_error_name(ret));
		return SPI_PROGRAMMER_ERROR;
	}
	msg_pdbg("SPI CLK frequency %0.3f kHz\n", (1.0f / (int)(PICKIT2_SCLK_MAX_kHz / spispeed)) * 1000.0f);
	
	return 0;
}

static int pickit2_spi_send_command(struct flashctx __attribute__((unused))*flash, unsigned int writecnt, unsigned int readcnt,
									const unsigned char *writearr, unsigned char *readarr)
{
	int ret, transferred = 0;
	unsigned int i = 0, n, cnt8k = 0, cnt1M = 0;
	uint8_t buf[USB_PACKET_LENGTH] = { 0 };

	/* Write Flash instruction and data to PicKit2's Download buffer (256B).
	* Write Instruction (02b xx xx xx) and Flash data can be larger than download buffer.
	* Instruction and data are sent in 64B USB packets and executed when received. 
	* SCLK must be > 10kHz to avoid download buffer overflow.
	* Each USB interrupt packet is sent in 1ms time slots.
	* At 1MHz SCLK, USB transfer is slower than PicKit2 to chip transfer.
	* Try to minimize number of USB transfers and maximize USB payload. 
	*/
	if (writecnt > 1) {
		do {
			n = (writecnt > 52) ? 52 : writecnt; /* 256 / 5 + 1 */
			buf[i++] = CMD_DOWNLOAD_DATA;
			buf[i++] = n;
			memcpy(buf + i, writearr, n);
			i += n;

			buf[i++] = CMD_RUN_SCRIPT;
			buf[i++] = RUN_SCR_SPI_WRITE;
			buf[i++] = n; 	/* Number of times to run SPI_WRITE script */

			/* 3 = CS# Hi, 4 = READ_DATA */
			if ((writecnt > 52) || (USB_PACKET_LENGTH < i + 3 + (readcnt ? 4 : 0))) {
				if (i < USB_PACKET_LENGTH)
					buf[i] = CMD_END_OF_BUFFER;
				
				/* msg_pdbg2("pickit2_spi_send_command!w%u/%u!%ub\n", n, writecnt, i); */
				ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, buf, USB_PACKET_LENGTH, &transferred, DFLT_TIMEOUT);
				if (ret != 0) {
					msg_perr("PicKit2 Send Write Script failed, sent %d %s!\n", transferred, libusb_error_name(ret));
					return SPI_PROGRAMMER_ERROR;
				}
				i = 0;
			}
			writecnt -= n;
			writearr += n;
		} while (writecnt);
	} 
	else if (writecnt == 1) {
		/* while checking status register, don't change data in the DOWNLOAD buffer*/
		if (writearr[0] == JEDEC_RDSR) {
			buf[i++] = CMD_RUN_SCRIPT;
			buf[i++] = RUN_SCR_SPI_STATUS_REG;
			buf[i++] = 1;
		}
		else {
			buf[i++] = CMD_EXEC_SCRIPT;
			buf[i++] = 4;
			/* CS# Low */
			buf[i++] = SCR_VPP_Q_OFF;
			buf[i++] = SCR_MCLR_GND_Q_ON;
			buf[i++] = SCR_SPI_WRITE_LIT;
			buf[i++] = writearr[0];
		}
	}
	do 
	{
		if (readcnt) {
			n = (readcnt > USB_PACKET_LENGTH) ? USB_PACKET_LENGTH : readcnt; /* max 64B in one packet */
			buf[i++] = CMD_RUN_SCRIPT;
			buf[i++] = RUN_SCR_SPI_READ;
			buf[i++] = n;						/* Number of times to run SPI_READ script / bytes to read */
			buf[i++] = CMD_UPLOAD_DATA_NO_LEN;	/* Fetch data from Upload Buffer */
		}
		/* last USB packet/no packets to read, set CS# Hi */
		if (readcnt <= USB_PACKET_LENGTH) {
			/* CS# High */
			buf[i++] = CMD_RUN_SCRIPT;
			buf[i++] = RUN_SCR_SPI_CS_HI;
			buf[i++] = 1;
		}
		/* mark end of commands */			
		if (i < USB_PACKET_LENGTH)
			buf[i] = CMD_END_OF_BUFFER;

		//msg_pdbg2("pickit2_spi_send_command!w63!%ub\n", i);
		/* send commands to PicKit2 and Flash */
		ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, buf, USB_PACKET_LENGTH, &transferred, DFLT_TIMEOUT);
		if (ret != 0) {
			msg_perr("PicKit2 Send Read Script failed, sent %d %s!\n", transferred, libusb_error_name(ret));
			return SPI_PROGRAMMER_ERROR;
		} 
		i = 0;
		if (readcnt) {
			/* Get data from PicKit2 or from Flash */
			ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_IN, buf, USB_PACKET_LENGTH, &transferred, DFLT_TIMEOUT);
			if (ret != 0) {
				msg_perr("PicKit2 Read Data failed, expected %u, got %d %s!\n", n, transferred, libusb_error_name(ret));
				return SPI_PROGRAMMER_ERROR;
			}
			if (USB_PACKET_LENGTH == transferred) {
				memcpy(readarr, buf, n);
				readcnt -= n;
				readarr += n;
				cnt8k += n;
				cnt1M += n;
			}
			else {
				msg_perr("PicKit2 Data error, expected %u/%u, got %d %s!\n", n, USB_PACKET_LENGTH, transferred, libusb_error_name(ret));
				return SPI_INVALID_LENGTH;
			}
			if (cnt1M / MB1) {
				msg_pdbg2("*\n");
				cnt1M -= MB1;
				cnt8k = cnt1M;
			}
			if (cnt8k / KB8) {
				msg_pdbg2(".");
				cnt8k -= KB8;
			}
		}
	} while (readcnt);
	
	return pickit2_check_errors();
}

/* Copied from dediprog.c */
/* Might be useful for other USB devices as well. static for now. */
static int parse_voltage(char *voltage)
{
	char *tmp = NULL;
	int i;
	int millivolt = 0, fraction = 0;

	if (!(voltage && *voltage)) {
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
	.type			= SPI_CONTROLLER_PICKIT2,
	.max_data_read	= 0xFFFFFF, /* 16MB, read complete chip with one instruction */
	.max_data_write	= 256, 
	.command		= pickit2_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read			= default_spi_read,
	.write_256		= default_spi_write_256,
	.write_aai		= default_spi_write_aai,
};

static int pickit2_shutdown(void __attribute__((unused))*data)
{
	/* Set all pins to float and turn voltages off */
	uint8_t command[USB_PACKET_LENGTH] = {
		CMD_EXEC_SCRIPT,
		8,
		/* CS# float */
		SCR_VPP_Q_OFF,
		SCR_MCLR_GND_Q_OFF,
		SCR_SET_PINS,
		3, 					/* Bit-0=1(PDC In), Bit-1=1(PGD In), Bit-2=0(PDC LL), Bit-3=0(PGD LL) */
		SCR_SET_AUX,
		1, 					/* Bit-0=1(Aux In), Bit-1=0(Aux LL) */
		/*SCR_VPP_PWM_OFF,*/
		SCR_VDD_OFF,		/* Power OFF Vdd */
		SCR_BUSY_LED_OFF,	/* Turn OFF Busy LED */
		CMD_END_OF_BUFFER
	};
	int transferred = 0;
	int ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, command, USB_PACKET_LENGTH, &transferred, DFLT_TIMEOUT);

	if (ret != 0) {
		msg_perr("SPI Shutdown Script failed (%s)!\n", libusb_error_name(ret));
	}

	ret = libusb_release_interface(pickit2_handle, 0);
	if (ret != 0) {
		msg_perr("Could not release USB interface (%s)!\n", libusb_error_name(ret));
		ret = SPI_GENERIC_ERROR;
	}
	libusb_close(pickit2_handle);
	libusb_exit(usb_ctx);
	
	msg_pinfo("Shutdown succeeded\n");

	return ret;
}

int pickit2_spi_init(void)
{
	int usedevice = 0;

	uint8_t buf[USB_PACKET_LENGTH] = {
		CMD_EXEC_SCRIPT,
		8,					/* Script length */
		SCR_SET_PINS,
		2,					/* Bit-0=0(PDC Out), Bit-1=1(PGD In), Bit-2=0(PDC LL), Bit-3=0(PGD LL) */
		SCR_SET_AUX,
		0,					/* Bit-0=0(Aux Out), Bit-1=0(Aux LL) */
		SCR_VDD_ON,			/* Power ON Vdd */
		SCR_MCLR_GND_Q_OFF,	/* Let CS# float */
		SCR_VPP_Q_ON,		/* Pull CS# high */
		/*SCR_VPP_PWM_ON,*/	/* no need to Power ON Vpp, CS# is internaly connected to Vdd */
		SCR_BUSY_LED_ON,	/* Busy LED ON*/

		CMD_CLR_DOWNLOAD_BUFFER,
		CMD_CLR_UPLOAD_BUFFER,
		CMD_CLR_SCRIPT_BUFFER,

		CMD_DOWNLOAD_SCRIPT,
		RUN_SCR_SPI_CS_HI,
		2,
		SCR_MCLR_GND_Q_OFF,
		SCR_VPP_Q_ON,

		CMD_DOWNLOAD_SCRIPT,
		RUN_SCR_SPI_WRITE,
		3,
		SCR_VPP_Q_OFF,
		SCR_MCLR_GND_Q_ON,
		SCR_SPI_WRITE_BUFFER,

		CMD_DOWNLOAD_SCRIPT,
		RUN_SCR_SPI_READ,
		3,
		SCR_VPP_Q_OFF,
		SCR_MCLR_GND_Q_ON,
		SCR_SPI_READ_BUFFER,

		CMD_DOWNLOAD_SCRIPT,
		RUN_SCR_SPI_STATUS_REG,
		4,
		SCR_VPP_Q_OFF,
		SCR_MCLR_GND_Q_ON,
		SCR_SPI_WRITE_LIT,
		JEDEC_RDSR,

		CMD_DOWNLOAD_SCRIPT,
		RUN_SCR_LED_ON,
		1,
		SCR_BUSY_LED_ON,

		CMD_END_OF_BUFFER
	};

	unsigned int spispeed = PICKIT2_SCLK_MAX_kHz; /* 1 MHz */
	char *spispeed_str = extract_programmer_param("spispeed");
	if (spispeed_str != NULL) {
		char* end_ptr = NULL;
		errno = 0;
		spispeed = strtoul(spispeed_str, &end_ptr, 10);
		if ((errno != 0) || (spispeed_str == end_ptr)) {
			msg_perr("Error: Could not convert 'spispeed'.\n");
			free(spispeed_str);
			return SPI_GENERIC_ERROR;
		}
		if (end_ptr && *end_ptr == 'M')
			spispeed *= 1000;

		if ((spispeed == 0) || (spispeed > PICKIT2_SCLK_MAX_kHz) || (spispeed < 10)) {
			msg_perr("Error: Value for 'spispeed' is out of range.\n");
			free(spispeed_str);
			return SPI_GENERIC_ERROR;
		}
		free(spispeed_str);
	}

	int millivolt = 3000;

	char *voltage = extract_programmer_param("voltage");
	if (voltage != NULL) {
		millivolt = parse_voltage(voltage);
		free(voltage);
		if (millivolt < 0)
			return SPI_GENERIC_ERROR;
	}
	/* from dediproc.c */
	char *device = extract_programmer_param("device");
	if (device) {
		char *dev_suffix = NULL;
		errno = 0;
		usedevice = strtol(device, &dev_suffix, 10);
		if ((errno != 0) || (device == dev_suffix)) {
			msg_perr("Error: Could not convert 'device'.\n");
			free(device);
			return SPI_GENERIC_ERROR;
		}
		if (usedevice < 0) {
			msg_perr("Error: Value for 'device' is out of range.\n");
			free(device);
			return SPI_GENERIC_ERROR;
		}
		if (*dev_suffix) {
			msg_perr("Error: Garbage following 'device' value.\n");
			free(device);
			return SPI_GENERIC_ERROR;
		}
		msg_pinfo("Using device %d.\n", usedevice);
	}
	free(device);

	/* Here comes the USB stuff */
	libusb_init(&usb_ctx);
	if (!usb_ctx) {
		msg_perr("Could not initialize libusb!\n");
		return SPI_PROGRAMMER_ERROR;
	}
#if LIBUSB_API_VERSION >= 0x01000106
	libusb_set_option(usb_ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);
#endif
	const uint16_t vid = devs_pickit2_spi[0].vendor_id;
	const uint16_t pid = devs_pickit2_spi[0].device_id;
	
	pickit2_handle = get_device_by_vid_pid_number(vid, pid, (unsigned int) usedevice);
	if (pickit2_handle == NULL) {
		msg_perr("Could not find a PICkit2 on USB!\n");
		libusb_exit(usb_ctx);
		return SPI_PROGRAMMER_ERROR;
	}
    int config = 0;
    int ret = libusb_get_configuration(pickit2_handle, &config);
    if ((ret == LIBUSB_SUCCESS) && (config != 1)) {
      ret = libusb_set_configuration(pickit2_handle, 1);
	  if (ret != LIBUSB_SUCCESS) {
		msg_perr("Could not set USB device configuration: %d %s\n", ret, libusb_error_name(ret));
		libusb_close(pickit2_handle);
		libusb_exit(usb_ctx);
		return SPI_PROGRAMMER_ERROR;
	  }
    }
    else if (ret != LIBUSB_SUCCESS) {
      msg_perr("Could not get USB device configuration: %d %s\n", ret, libusb_error_name(ret));
      libusb_close(pickit2_handle);
      libusb_exit(usb_ctx);
      return SPI_PROGRAMMER_ERROR;
    }
	ret = libusb_claim_interface(pickit2_handle, 0);
	if (ret != LIBUSB_SUCCESS) {
		msg_perr("Could not claim USB device interface %d: %d %s\n", 0, ret, libusb_error_name(ret));
		libusb_close(pickit2_handle);
		libusb_exit(usb_ctx);
		return SPI_PROGRAMMER_ERROR;
	}
	if (register_shutdown(pickit2_shutdown, NULL)) {
		return SPI_PROGRAMMER_ERROR;
	}

	if (pickit2_get_status())
		return SPI_PROGRAMMER_ERROR;

	if (pickit2_get_firmware_version()) {
		return SPI_PROGRAMMER_ERROR;
	}

	if (pickit2_set_spi_speed(spispeed)) {
		return SPI_PROGRAMMER_ERROR;
	}

	if (pickit2_set_spi_voltage(millivolt) != 0) {
		return SPI_PROGRAMMER_ERROR;
	}

	/* Perform basic setup.
	 * Configure pin directions and logic levels, turn Vdd on, turn busy LED on, clear buffers and download canned scripts. */
	int transferred = 0;
	ret = libusb_interrupt_transfer(pickit2_handle, ENDPOINT_OUT, buf, USB_PACKET_LENGTH, &transferred, DFLT_TIMEOUT);
	if (ret != LIBUSB_SUCCESS) {
		msg_perr("PicKit2 Setup Command failed (%s)!\n", libusb_error_name(ret));
		return SPI_PROGRAMMER_ERROR;
	}

	register_spi_master(&spi_master_pickit2);

	return 0;
}
