/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Carl-Daniel Hailfinger
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
#include <string.h>
#include <usb.h>
#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

#define FIRMWARE_VERSION(x,y,z) ((x << 16) | (y << 8) | z)
#define DEFAULT_TIMEOUT 3000
static usb_dev_handle *dediprog_handle;
static int dediprog_firmwareversion;
static int dediprog_endpoint;

#if 0
/* Might be useful for other pieces of code as well. */
static void print_hex(void *buf, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		msg_pdbg(" %02x", ((uint8_t *)buf)[i]);
}
#endif

/* Might be useful for other USB devices as well. static for now. */
static struct usb_device *get_device_by_vid_pid(uint16_t vid, uint16_t pid)
{
	struct usb_bus *bus;
	struct usb_device *dev;

	for (bus = usb_get_busses(); bus; bus = bus->next)
		for (dev = bus->devices; dev; dev = dev->next)
			if ((dev->descriptor.idVendor == vid) &&
			    (dev->descriptor.idProduct == pid))
				return dev;

	return NULL;
}

//int usb_control_msg(usb_dev_handle *dev, int requesttype, int request, int value, int index, char *bytes, int size, int timeout);

/* Set/clear LEDs on dediprog */
#define PASS_ON		(0 << 0)
#define PASS_OFF	(1 << 0)
#define BUSY_ON		(0 << 1)
#define BUSY_OFF	(1 << 1)
#define ERROR_ON	(0 << 2)
#define ERROR_OFF	(1 << 2)
static int current_led_status = -1;

static int dediprog_set_leds(int leds)
{
	int ret, target_leds;

	if (leds < 0 || leds > 7)
		leds = 0; // Bogus value, enable all LEDs

	if (leds == current_led_status)
		return 0;

	/* Older Dediprogs with 2.x.x and 3.x.x firmware only had
	 * two LEDs, and they were reversed. So map them around if 
	 * we have an old device. On those devices the LEDs map as
	 * follows:
	 *   bit 2 == 0: green light is on.
	 *   bit 0 == 0: red light is on. 
	 */
	if (dediprog_firmwareversion < FIRMWARE_VERSION(5,0,0)) {
		target_leds = ((leds & ERROR_OFF) >> 2) | 
			((leds & PASS_OFF) << 2);
	} else {
		target_leds = leds;
	}

	ret = usb_control_msg(dediprog_handle, 0x42, 0x07, 0x09, target_leds,
			      NULL, 0x0, DEFAULT_TIMEOUT);
	if (ret != 0x0) {
		msg_perr("Command Set LED 0x%x failed (%s)!\n",
			 leds, usb_strerror());
		return 1;
	}

	current_led_status = leds;

	return 0;
}

static int dediprog_set_spi_voltage(int millivolt)
{
	int ret;
	uint16_t voltage_selector;

	switch (millivolt) {
	case 0:
		/* Admittedly this one is an assumption. */
		voltage_selector = 0x0;
		break;
	case 1800:
		voltage_selector = 0x12;
		break;
	case 2500:
		voltage_selector = 0x11;
		break;
	case 3500:
		voltage_selector = 0x10;
		break;
	default:
		msg_perr("Unknown voltage %i mV! Aborting.\n", millivolt);
		return 1;
	}
	msg_pdbg("Setting SPI voltage to %u.%03u V\n", millivolt / 1000,
		 millivolt % 1000);

	if (voltage_selector == 0) {
		/* Wait some time as the original driver does. */
		programmer_delay(200 * 1000);
	}
	ret = usb_control_msg(dediprog_handle, 0x42, 0x9, voltage_selector,
			      0xff, NULL, 0x0, DEFAULT_TIMEOUT);
	if (ret != 0x0) {
		msg_perr("Command Set SPI Voltage 0x%x failed!\n",
			 voltage_selector);
		return 1;
	}
	if (voltage_selector != 0) {
		/* Wait some time as the original driver does. */
		programmer_delay(200 * 1000);
	}
	return 0;
}

#if 0
/* After dediprog_set_spi_speed, the original app always calls
 * dediprog_set_spi_voltage(0) and then
 * dediprog_check_devicestring() four times in a row.
 * After that, dediprog_command_a() is called.
 * This looks suspiciously like the microprocessor in the SF100 has to be
 * restarted/reinitialized in case the speed changes.
 */
static int dediprog_set_spi_speed(uint16_t speed)
{
	int ret;
	unsigned int khz;

	/* Case 1 and 2 are in weird order. Probably an organically "grown"
	 * interface.
	 * Base frequency is 24000 kHz, divisors are (in order)
	 * 1, 3, 2, 8, 11, 16, 32, 64.
	 */
	switch (speed) {
	case 0x0:
		khz = 24000;
		break;
	case 0x1:
		khz = 8000;
		break;
	case 0x2:
		khz = 12000;
		break;
	case 0x3:
		khz = 3000;
		break;
	case 0x4:
		khz = 2180;
		break;
	case 0x5:
		khz = 1500;
		break;
	case 0x6:
		khz = 750;
		break;
	case 0x7:
		khz = 375;
		break;
	default:
		msg_perr("Unknown frequency selector 0x%x! Aborting.\n", speed);
		return 1;
	}
	msg_pdbg("Setting SPI speed to %u kHz\n", khz);

	ret = usb_control_msg(dediprog_handle, 0x42, 0x61, speed, 0xff, NULL,
			      0x0, DEFAULT_TIMEOUT);
	if (ret != 0x0) {
		msg_perr("Command Set SPI Speed 0x%x failed!\n", speed);
		return 1;
	}
	return 0;
}
#endif

/* Bulk read interface, will read multiple 512 byte chunks aligned to 512 bytes.
 * @start	start address
 * @len		length
 * @return	0 on success, 1 on failure
 */
static int dediprog_spi_bulk_read(struct flashctx *flash, uint8_t *buf,
				  unsigned int start, unsigned int len)
{
	int ret;
	unsigned int i;
	/* chunksize must be 512, other sizes will NOT work at all. */
	const unsigned int chunksize = 0x200;
	const unsigned int count = len / chunksize;
	const char count_and_chunk[] = {count & 0xff,
					(count >> 8) & 0xff,
					chunksize & 0xff,
					(chunksize >> 8) & 0xff};

	if ((start % chunksize) || (len % chunksize)) {
		msg_perr("%s: Unaligned start=%i, len=%i! Please report a bug "
			 "at flashrom@flashrom.org\n", __func__, start, len);
		return 1;
	}

	/* No idea if the hardware can handle empty reads, so chicken out. */
	if (!len)
		return 0;
	/* Command Read SPI Bulk. No idea which read command is used on the
	 * SPI side.
	 */
	ret = usb_control_msg(dediprog_handle, 0x42, 0x20, start % 0x10000,
			      start / 0x10000, (char *)count_and_chunk,
			      sizeof(count_and_chunk), DEFAULT_TIMEOUT);
	if (ret != sizeof(count_and_chunk)) {
		msg_perr("Command Read SPI Bulk failed, %i %s!\n", ret,
			 usb_strerror());
		return 1;
	}

	for (i = 0; i < count; i++) {
		ret = usb_bulk_read(dediprog_handle, 0x80 | dediprog_endpoint,
				    (char *)buf + i * chunksize, chunksize,
				    DEFAULT_TIMEOUT);
		if (ret != chunksize) {
			msg_perr("SPI bulk read %i failed, expected %i, got %i "
				 "%s!\n", i, chunksize, ret, usb_strerror());
			return 1;
		}
	}

	return 0;
}

static int dediprog_spi_read(struct flashctx *flash, uint8_t *buf,
			     unsigned int start, unsigned int len)
{
	int ret;
	/* chunksize must be 512, other sizes will NOT work at all. */
	const unsigned int chunksize = 0x200;
	unsigned int residue = start % chunksize ? chunksize - start % chunksize : 0;
	unsigned int bulklen;

	dediprog_set_leds(PASS_OFF|BUSY_ON|ERROR_OFF);

	if (residue) {
		msg_pdbg("Slow read for partial block from 0x%x, length 0x%x\n",
			 start, residue);
		ret = spi_read_chunked(flash, buf, start, residue, 16);
		if (ret) {
			dediprog_set_leds(PASS_OFF|BUSY_OFF|ERROR_ON);
			return ret;
		}
	}

	/* Round down. */
	bulklen = (len - residue) / chunksize * chunksize;
	ret = dediprog_spi_bulk_read(flash, buf + residue, start + residue,
				     bulklen);
	if (ret) {
		dediprog_set_leds(PASS_OFF|BUSY_OFF|ERROR_ON);
		return ret;
	}

	len -= residue + bulklen;
	if (len) {
		msg_pdbg("Slow read for partial block from 0x%x, length 0x%x\n",
			 start, len);
		ret = spi_read_chunked(flash, buf + residue + bulklen,
				       start + residue + bulklen, len, 16);
		if (ret) {
			dediprog_set_leds(PASS_OFF|BUSY_OFF|ERROR_ON);
			return ret;
		}
	}

	dediprog_set_leds(PASS_ON|BUSY_OFF|ERROR_OFF);
	return 0;
}

/* Bulk write interface, will write multiple page_size byte chunks aligned to page_size bytes.
 * @start	start address
 * @len		length
 * @return	0 on success, 1 on failure
 */
static int dediprog_spi_bulk_write(struct flashctx *flash, uint8_t *buf,
				   unsigned int start, unsigned int len)
{
	int ret;
	unsigned int i;
	/* USB transfer size must be 512, other sizes will NOT work at all.
	 * chunksize is the real data size per USB bulk transfer. The remaining
	 * space in a USB bulk transfer must be filled with 0xff padding.
	 */
	const unsigned int chunksize = flash->page_size;
	const unsigned int count = len / chunksize;
	const char count_and_chunk[] = {count & 0xff,
					(count >> 8) & 0xff,
					chunksize & 0xff,
					(chunksize >> 8) & 0xff};
	char usbbuf[512];

	if ((start % chunksize) || (len % chunksize)) {
		msg_perr("%s: Unaligned start=%i, len=%i! Please report a bug "
			 "at flashrom@flashrom.org\n", __func__, start, len);
		return 1;
	}

	/* No idea if the hardware can handle empty writes, so chicken out. */
	if (!len)
		return 0;
	/* Command Write SPI Bulk. No idea which write command is used on the
	 * SPI side.
	 */
	ret = usb_control_msg(dediprog_handle, 0x42, 0x30, start % 0x10000,
			      start / 0x10000, (char *)count_and_chunk,
			      sizeof(count_and_chunk), DEFAULT_TIMEOUT);
	if (ret != sizeof(count_and_chunk)) {
		msg_perr("Command Write SPI Bulk failed, %i %s!\n", ret,
			 usb_strerror());
		return 1;
	}

	for (i = 0; i < count; i++) {
		memset(usbbuf, 0xff, sizeof(usbbuf));
		memcpy(usbbuf, buf + i * chunksize, chunksize);
		ret = usb_bulk_write(dediprog_handle, dediprog_endpoint,
				    usbbuf, 512,
				    DEFAULT_TIMEOUT);
		if (ret != 512) {
			msg_perr("SPI bulk write failed, expected %i, got %i "
				 "%s!\n", 512, ret, usb_strerror());
			return 1;
		}
	}

	return 0;
}

static int dediprog_spi_write_256(struct flashctx *flash, uint8_t *buf,
				  unsigned int start, unsigned int len)
{
	int ret;
	const unsigned int chunksize = flash->page_size;
	unsigned int residue = start % chunksize ? chunksize - start % chunksize : 0;
	unsigned int bulklen;

	dediprog_set_leds(PASS_OFF|BUSY_ON|ERROR_OFF);

	if (residue) {
		msg_pdbg("Slow write for partial block from 0x%x, length 0x%x\n",
			 start, residue);
		/* No idea about the real limit. Maybe 12, maybe more. */
		ret = spi_write_chunked(flash, buf, start, residue, 12);
		if (ret) {
			dediprog_set_leds(PASS_OFF|BUSY_OFF|ERROR_ON);
			return ret;
		}
	}

	/* Round down. */
	bulklen = (len - residue) / chunksize * chunksize;
	ret = dediprog_spi_bulk_write(flash, buf + residue, start + residue,
				     bulklen);
	if (ret) {
		dediprog_set_leds(PASS_OFF|BUSY_OFF|ERROR_ON);
		return ret;
	}

	len -= residue + bulklen;
	if (len) {
		msg_pdbg("Slow write for partial block from 0x%x, length 0x%x\n",
			 start, len);
		ret = spi_write_chunked(flash, buf + residue + bulklen,
				        start + residue + bulklen, len, 12);
		if (ret) {
			dediprog_set_leds(PASS_OFF|BUSY_OFF|ERROR_ON);
			return ret;
		}
	}

	dediprog_set_leds(PASS_ON|BUSY_OFF|ERROR_OFF);
	return 0;
}

static int dediprog_spi_send_command(struct flashctx *flash,
				     unsigned int writecnt,
				     unsigned int readcnt,
				     const unsigned char *writearr,
				     unsigned char *readarr)
{
	int ret;

	msg_pspew("%s, writecnt=%i, readcnt=%i\n", __func__, writecnt, readcnt);
	/* Paranoid, but I don't want to be blamed if anything explodes. */
	if (writecnt > 16) {
		msg_perr("Untested writecnt=%i, aborting.\n", writecnt);
		return 1;
	}
	/* 16 byte reads should work. */
	if (readcnt > 16) {
		msg_perr("Untested readcnt=%i, aborting.\n", readcnt);
		return 1;
	}
	
	ret = usb_control_msg(dediprog_handle, 0x42, 0x1, 0xff,
			      readcnt ? 0x1 : 0x0, (char *)writearr, writecnt,
			      DEFAULT_TIMEOUT);
	if (ret != writecnt) {
		msg_perr("Send SPI failed, expected %i, got %i %s!\n",
			 writecnt, ret, usb_strerror());
		return 1;
	}
	if (!readcnt)
		return 0;
	memset(readarr, 0, readcnt);
	ret = usb_control_msg(dediprog_handle, 0xc2, 0x01, 0xbb8, 0x0000,
			     (char *)readarr, readcnt, DEFAULT_TIMEOUT);
	if (ret != readcnt) {
		msg_perr("Receive SPI failed, expected %i, got %i %s!\n",
			 readcnt, ret, usb_strerror());
		return 1;
	}
	return 0;
}

static int dediprog_check_devicestring(void)
{
	int ret;
	int fw[3];
	char buf[0x11];

	/* Command Prepare Receive Device String. */
	memset(buf, 0, sizeof(buf));
	ret = usb_control_msg(dediprog_handle, 0xc3, 0x7, 0x0, 0xef03, buf,
			      0x1, DEFAULT_TIMEOUT);
	/* The char casting is needed to stop gcc complaining about an always true comparison. */
	if ((ret != 0x1) || (buf[0] != (char)0xff)) {
		msg_perr("Unexpected response to Command Prepare Receive Device"
			 " String!\n");
		return 1;
	}
	/* Command Receive Device String. */
	memset(buf, 0, sizeof(buf));
	ret = usb_control_msg(dediprog_handle, 0xc2, 0x8, 0xff, 0xff, buf,
			      0x10, DEFAULT_TIMEOUT);
	if (ret != 0x10) {
		msg_perr("Incomplete/failed Command Receive Device String!\n");
		return 1;
	}
	buf[0x10] = '\0';
	msg_pdbg("Found a %s\n", buf);
	if (memcmp(buf, "SF100", 0x5)) {
		msg_perr("Device not a SF100!\n");
		return 1;
	}
	if (sscanf(buf, "SF100 V:%d.%d.%d ", &fw[0], &fw[1], &fw[2]) != 3) {
		msg_perr("Unexpected firmware version string!\n");
		return 1;
	}
	/* Only these versions were tested. */
	if (fw[0] < 2 || fw[0] > 5) {
		msg_perr("Unexpected firmware version %d.%d.%d!\n", fw[0],
			 fw[1], fw[2]);
		return 1;
	}
	dediprog_firmwareversion = FIRMWARE_VERSION(fw[0], fw[1], fw[2]);
	return 0;
}

/* Command A seems to be some sort of device init. It is either followed by
 * dediprog_check_devicestring (often) or Command A (often) or
 * Command F (once).
 */
static int dediprog_command_a(void)
{
	int ret;
	char buf[0x1];

	memset(buf, 0, sizeof(buf));
	ret = usb_control_msg(dediprog_handle, 0xc3, 0xb, 0x0, 0x0, buf,
			      0x1, DEFAULT_TIMEOUT);
	if (ret < 0) {
		msg_perr("Command A failed (%s)!\n", usb_strerror());
		return 1;
	}
	if ((ret != 0x1) || (buf[0] != 0x6f)) {
		msg_perr("Unexpected response to Command A!\n");
		return 1;
	}
	return 0;
}

#if 0
/* Something.
 * Present in eng_detect_blink.log with firmware 3.1.8
 * Always preceded by Command Receive Device String
 */
static int dediprog_command_b(void)
{
	int ret;
	char buf[0x3];

	memset(buf, 0, sizeof(buf));
	ret = usb_control_msg(dediprog_handle, 0xc3, 0x7, 0x0, 0xef00, buf,
			      0x3, DEFAULT_TIMEOUT);
	if (ret < 0) {
		msg_perr("Command B failed (%s)!\n", usb_strerror());
		return 1;
	}
	if ((ret != 0x3) || (buf[0] != 0xff) || (buf[1] != 0xff) ||
	    (buf[2] != 0xff)) {
		msg_perr("Unexpected response to Command B!\n");
		return 1;
	}

	return 0;
}
#endif

/* Command C is only sent after dediprog_check_devicestring, but not after every
 * invocation of dediprog_check_devicestring. It is only sent after the first
 * dediprog_command_a(); dediprog_check_devicestring() sequence in each session.
 * I'm tempted to call this one start_SPI_engine or finish_init.
 */
static int dediprog_command_c(void)
{
	int ret;

	ret = usb_control_msg(dediprog_handle, 0x42, 0x4, 0x0, 0x0, NULL,
			      0x0, DEFAULT_TIMEOUT);
	if (ret != 0x0) {
		msg_perr("Command C failed (%s)!\n", usb_strerror());
		return 1;
	}
	return 0;
}

#if 0
/* Very strange. Seems to be a programmer keepalive or somesuch.
 * Wait unsuccessfully for timeout ms to read one byte.
 * Is usually called after setting voltage to 0.
 * Present in all logs with Firmware 2.1.1 and 3.1.8
 */
static int dediprog_command_f(int timeout)
{
	int ret;
	char buf[0x1];

	memset(buf, 0, sizeof(buf));
	ret = usb_control_msg(dediprog_handle, 0xc2, 0x11, 0xff, 0xff, buf,
			      0x1, timeout);
	/* This check is most probably wrong. Command F always causes a timeout
	 * in the logs, so we should check for timeout instead of checking for
	 * success.
	 */
	if (ret != 0x1) {
		msg_perr("Command F failed (%s)!\n", usb_strerror());
		return 1;
	}
	return 0;
}

/* Start/stop blinking?
 * Present in eng_detect_blink.log with firmware 3.1.8
 * Preceded by Command J
 */
static int dediprog_command_g(void)
{
	int ret;

	ret = usb_control_msg(dediprog_handle, 0x42, 0x07, 0x09, 0x03, NULL, 0x0, DEFAULT_TIMEOUT);
	if (ret != 0x0) {
		msg_perr("Command G failed (%s)!\n", usb_strerror());
		return 1;
	}
	return 0;
}

/* Something.
 * Present in all logs with firmware 5.1.5
 * Always preceded by Command Receive Device String
 * Always followed by Command Set SPI Voltage nonzero
 */
static int dediprog_command_h(void)
{
	int ret;

	ret = usb_control_msg(dediprog_handle, 0x42, 0x07, 0x09, 0x05, NULL, 0x0, DEFAULT_TIMEOUT);
	if (ret != 0x0) {
		msg_perr("Command H failed (%s)!\n", usb_strerror());
		return 1;
	}
	return 0;
}

/* Shutdown for firmware 5.x?
 * Present in all logs with firmware 5.1.5
 * Often preceded by a SPI operation (Command Read SPI Bulk or Receive SPI)
 * Always followed by Command Set SPI Voltage 0x0000
 */
static int dediprog_command_i(void)
{
	int ret;

	ret = usb_control_msg(dediprog_handle, 0x42, 0x07, 0x09, 0x06, NULL, 0x0, DEFAULT_TIMEOUT);
	if (ret != 0x0) {
		msg_perr("Command I failed (%s)!\n", usb_strerror());
		return 1;
	}
	return 0;
}

/* Start/stop blinking?
 * Present in all logs with firmware 5.1.5
 * Always preceded by Command Receive Device String on 5.1.5
 * Always followed by Command Set SPI Voltage nonzero on 5.1.5
 * Present in eng_detect_blink.log with firmware 3.1.8
 * Preceded by Command B in eng_detect_blink.log
 * Followed by Command G in eng_detect_blink.log
 */
static int dediprog_command_j(void)
{
	int ret;

	ret = usb_control_msg(dediprog_handle, 0x42, 0x07, 0x09, 0x07, NULL, 0x0, DEFAULT_TIMEOUT);
	if (ret != 0x0) {
		msg_perr("Command J failed (%s)!\n", usb_strerror());
		return 1;
	}
	return 0;
}
#endif

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
		   !strncmp(voltage, "milliv", 6)) {
		/* No adjustment. fraction is discarded. */
	} else {
		/* Garbage at the end of the string. */
		msg_perr("Garbage voltage= specified.\n");
		return -1;
	}
	return millivolt;
}

static const struct spi_programmer spi_programmer_dediprog = {
	.type		= SPI_CONTROLLER_DEDIPROG,
	.max_data_read	= MAX_DATA_UNSPECIFIED,
	.max_data_write	= MAX_DATA_UNSPECIFIED,
	.command	= dediprog_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= dediprog_spi_read,
	.write_256	= dediprog_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

static int dediprog_shutdown(void *data)
{
	msg_pspew("%s\n", __func__);

#if 0
	/* Shutdown on firmware 5.x */
	if (dediprog_firmwareversion == 5)
		if (dediprog_command_i())
			return 1;
#endif

	/* URB 28. Command Set SPI Voltage to 0. */
	if (dediprog_set_spi_voltage(0x0))
		return 1;

	if (usb_release_interface(dediprog_handle, 0)) {
		msg_perr("Could not release USB interface!\n");
		return 1;
	}
	if (usb_close(dediprog_handle)) {
		msg_perr("Could not close USB device!\n");
		return 1;
	}
	return 0;
}

/* URB numbers refer to the first log ever captured. */
int dediprog_init(void)
{
	struct usb_device *dev;
	char *voltage;
	int millivolt = 3500;
	int ret;

	msg_pspew("%s\n", __func__);

	voltage = extract_programmer_param("voltage");
	if (voltage) {
		millivolt = parse_voltage(voltage);
		free(voltage);
		if (millivolt < 0)
			return 1;
		msg_pinfo("Setting voltage to %i mV\n", millivolt);
	}

	/* Here comes the USB stuff. */
	usb_init();
	usb_find_busses();
	usb_find_devices();
	dev = get_device_by_vid_pid(0x0483, 0xdada);
	if (!dev) {
		msg_perr("Could not find a Dediprog SF100 on USB!\n");
		return 1;
	}
	msg_pdbg("Found USB device (%04x:%04x).\n",
		 dev->descriptor.idVendor, dev->descriptor.idProduct);
	dediprog_handle = usb_open(dev);
	ret = usb_set_configuration(dediprog_handle, 1);
	if (ret < 0) {
		msg_perr("Could not set USB device configuration: %i %s\n",
			 ret, usb_strerror());
		if (usb_close(dediprog_handle))
			msg_perr("Could not close USB device!\n");
		return 1;
	}
	ret = usb_claim_interface(dediprog_handle, 0);
	if (ret < 0) {
		msg_perr("Could not claim USB device interface %i: %i %s\n",
			 0, ret, usb_strerror());
		if (usb_close(dediprog_handle))
			msg_perr("Could not close USB device!\n");
		return 1;
	}
	dediprog_endpoint = 2;
	
	if (register_shutdown(dediprog_shutdown, NULL))
		return 1;

	dediprog_set_leds(PASS_ON|BUSY_ON|ERROR_ON);

	/* URB 6. Command A. */
	if (dediprog_command_a()) {
		dediprog_set_leds(PASS_OFF|BUSY_OFF|ERROR_ON);
		return 1;
	}
	/* URB 7. Command A. */
	if (dediprog_command_a()) {
		dediprog_set_leds(PASS_OFF|BUSY_OFF|ERROR_ON);
		return 1;
	}
	/* URB 8. Command Prepare Receive Device String. */
	/* URB 9. Command Receive Device String. */
	if (dediprog_check_devicestring()) {
		dediprog_set_leds(PASS_OFF|BUSY_OFF|ERROR_ON);
		return 1;
	}
	/* URB 10. Command C. */
	if (dediprog_command_c()) {
		dediprog_set_leds(PASS_OFF|BUSY_OFF|ERROR_ON);
		return 1;
	}
	/* URB 11. Command Set SPI Voltage. */
	if (dediprog_set_spi_voltage(millivolt)) {
		dediprog_set_leds(PASS_OFF|BUSY_OFF|ERROR_ON);
		return 1;
	}

	register_spi_programmer(&spi_programmer_dediprog);

	/* RE leftover, leave in until the driver is complete. */
#if 0
	/* Execute RDID by hand if you want to test it. */
	dediprog_do_stuff();
#endif

	dediprog_set_leds(PASS_OFF|BUSY_OFF|ERROR_OFF);

	return 0;
}

#if 0
/* Leftovers from reverse engineering. Keep for documentation purposes until
 * completely understood.
 */
static int dediprog_do_stuff(void)
{
	char buf[0x4];
	/* SPI command processing starts here. */

	/* URB 12. Command Send SPI. */
	/* URB 13. Command Receive SPI. */
	memset(buf, 0, sizeof(buf));
	/* JEDEC RDID */
	msg_pdbg("Sending RDID\n");
	buf[0] = JEDEC_RDID;
	if (dediprog_spi_send_command(JEDEC_RDID_OUTSIZE, JEDEC_RDID_INSIZE,
				(unsigned char *)buf, (unsigned char *)buf))
		return 1;
	msg_pdbg("Receiving response: ");
	print_hex(buf, JEDEC_RDID_INSIZE);
	/* URB 14-27 are more SPI commands. */
	/* URB 28. Command Set SPI Voltage. */
	if (dediprog_set_spi_voltage(0x0))
		return 1;
	/* URB 29-38. Command F, unsuccessful wait. */
	if (dediprog_command_f(544))
		return 1;
	/* URB 39. Command Set SPI Voltage. */
	if (dediprog_set_spi_voltage(0x10))
		return 1;
	/* URB 40. Command Set SPI Speed. */
	if (dediprog_set_spi_speed(0x2))
		return 1;
	/* URB 41 is just URB 28. */
	/* URB 42,44,46,48,51,53 is just URB 8. */
	/* URB 43,45,47,49,52,54 is just URB 9. */
	/* URB 50 is just URB 6/7. */
	/* URB 55-131 is just URB 29-38. (wait unsuccessfully for 4695 (maybe 4751) ms)*/
	/* URB 132,134 is just URB 6/7. */
	/* URB 133 is just URB 29-38. */
	/* URB 135 is just URB 8. */
	/* URB 136 is just URB 9. */
	/* URB 137 is just URB 11. */

	/* Command Start Bulk Read. Data is u16 blockcount, u16 blocksize. */
	/* Command Start Bulk Write. Data is u16 blockcount, u16 blocksize. */
	/* Bulk transfer sizes for Command Start Bulk Read/Write are always
	 * 512 bytes, rest is filled with 0xff.
	 */

	return 0;
}
#endif	
