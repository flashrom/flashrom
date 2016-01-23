/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2010 Carl-Daniel Hailfinger
 * Copyright (C) 2015 Simon Glass
 * Copyright (C) 2015 Stefan Tauner
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

#include "platform.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#if IS_WINDOWS
#include <lusb0_usb.h>
#else
#include <usb.h>
#endif

#include "flash.h"
#include "chipdrivers.h"
#include "programmer.h"
#include "spi.h"

#define FIRMWARE_VERSION(x,y,z) ((x << 16) | (y << 8) | z)
#define DEFAULT_TIMEOUT 3000
#define REQTYPE_OTHER_OUT (USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_OTHER)	/* 0x43 */
#define REQTYPE_OTHER_IN (USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_OTHER)		/* 0xC3 */
#define REQTYPE_EP_OUT (USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_ENDPOINT)	/* 0x42 */
#define REQTYPE_EP_IN (USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_ENDPOINT)		/* 0xC2 */
static usb_dev_handle *dediprog_handle;
static int dediprog_endpoint;

enum dediprog_leds {
	LED_INVALID		= -1,
	LED_NONE		= 0,
	LED_PASS		= 1 << 0,
	LED_BUSY		= 1 << 1,
	LED_ERROR		= 1 << 2,
	LED_ALL			= 7,
};

/* IO bits for CMD_SET_IO_LED message */
enum dediprog_ios {
	IO1			= 1 << 0,
	IO2			= 1 << 1,
	IO3			= 1 << 2,
	IO4			= 1 << 3,
};

enum dediprog_cmds {
	CMD_TRANSCEIVE		= 0x01,
	CMD_POLL_STATUS_REG	= 0x02,
	CMD_SET_VPP		= 0x03,
	CMD_SET_TARGET		= 0x04,
	CMD_READ_EEPROM		= 0x05,
	CMD_WRITE_EEPROM	= 0x06,
	CMD_SET_IO_LED		= 0x07,
	CMD_READ_PROG_INFO	= 0x08,
	CMD_SET_VCC		= 0x09,
	CMD_SET_STANDALONE	= 0x0A,
	CMD_GET_BUTTON		= 0x11,
	CMD_GET_UID		= 0x12,
	CMD_SET_CS		= 0x14,
	CMD_IO_MODE		= 0x15,
	CMD_FW_UPDATE		= 0x1A,
	CMD_FPGA_UPDATE		= 0x1B,
	CMD_READ_FPGA_VERSION	= 0x1C,
	CMD_SET_HOLD		= 0x1D,
	CMD_READ		= 0x20,
	CMD_WRITE		= 0x30,
	CMD_WRITE_AT45DB	= 0x31,
	CMD_NAND_WRITE		= 0x32,
	CMD_NAND_READ		= 0x33,
	CMD_SET_SPI_CLK		= 0x61,
	CMD_CHECK_SOCKET	= 0x62,
	CMD_DOWNLOAD_PRJ	= 0x63,
	CMD_READ_PRJ_NAME	= 0x64,
	// New protocol/firmware only
	CMD_CHECK_SDCARD	= 0x65,
	CMD_READ_PRJ		= 0x66,
};

enum dediprog_target {
	FLASH_TYPE_APPLICATION_FLASH_1	= 0,
	FLASH_TYPE_FLASH_CARD,
	FLASH_TYPE_APPLICATION_FLASH_2,
	FLASH_TYPE_SOCKET,
};

enum dediprog_readmode {
	READ_MODE_STD			= 1,
	READ_MODE_FAST			= 2,
	READ_MODE_ATMEL45		= 3,
	READ_MODE_4B_ADDR_FAST		= 4,
	READ_MODE_4B_ADDR_FAST_0x0C	= 5, /* New protocol only */
};

enum dediprog_writemode {
	WRITE_MODE_PAGE_PGM 			= 1,
	WRITE_MODE_PAGE_WRITE			= 2,
	WRITE_MODE_1B_AAI			= 3,
	WRITE_MODE_2B_AAI			= 4,
	WRITE_MODE_128B_PAGE			= 5,
	WRITE_MODE_PAGE_AT26DF041		= 6,
	WRITE_MODE_SILICON_BLUE_FPGA		= 7,
	WRITE_MODE_64B_PAGE_NUMONYX_PCM		= 8,	/* unit of 512 bytes */
	WRITE_MODE_4B_ADDR_256B_PAGE_PGM	= 9,
	WRITE_MODE_32B_PAGE_PGM_MXIC_512K	= 10,	/* unit of 512 bytes */
	WRITE_MODE_4B_ADDR_256B_PAGE_PGM_0x12	= 11,
	WRITE_MODE_4B_ADDR_256B_PAGE_PGM_FLAGS	= 12,
};


static int dediprog_firmwareversion = FIRMWARE_VERSION(0, 0, 0);

/* Returns true if firmware (and thus hardware) supports the "new" protocol */
static bool is_new_prot(void)
{
	/* if (SF100) */
	return dediprog_firmwareversion >= FIRMWARE_VERSION(5, 5, 0);
	/* else if (SF600)
	return dediprog_firmwareversion >= FIRMWARE_VERSION(6, 9, 0); */
}

static int dediprog_read(enum dediprog_cmds cmd, unsigned int value, unsigned int idx, uint8_t *bytes, size_t size)
{
	return usb_control_msg(dediprog_handle, REQTYPE_EP_IN, cmd, value, idx,
			       (char *)bytes, size, DEFAULT_TIMEOUT);
}

static int dediprog_write(enum dediprog_cmds cmd, unsigned int value, unsigned int idx, const uint8_t *bytes, size_t size)
{
	return usb_control_msg(dediprog_handle, REQTYPE_EP_OUT, cmd, value, idx,
			       (char *)bytes, size, DEFAULT_TIMEOUT);
}

/* Might be useful for other USB devices as well. static for now. */
/* device parameter allows user to specify one device of multiple installed */
static struct usb_device *get_device_by_vid_pid(uint16_t vid, uint16_t pid, unsigned int device)
{
	struct usb_bus *bus;
	struct usb_device *dev;

	for (bus = usb_get_busses(); bus; bus = bus->next)
		for (dev = bus->devices; dev; dev = dev->next)
			if ((dev->descriptor.idVendor == vid) &&
			    (dev->descriptor.idProduct == pid)) {
				if (device == 0)
					return dev;
				device--;
			}

	return NULL;
}

/* This function sets the GPIOs connected to the LEDs as well as IO1-IO4. */
static int dediprog_set_leds(int leds)
{
	if (leds < LED_NONE || leds > LED_ALL)
		leds = LED_ALL;

	/* Older Dediprogs with 2.x.x and 3.x.x firmware only had two LEDs, assigned to different bits. So map
	 * them around if we have an old device. On those devices the LEDs map as follows:
	 *   bit 2 == 0: green light is on.
	 *   bit 0 == 0: red light is on.
	 *
	 * Additionally, the command structure has changed with the "new" protocol.
	 *
	 * FIXME: take IO pins into account
	 */
	int target_leds, ret;
	if (is_new_prot()) {
		target_leds = (leds ^ 7) << 8;
		ret = dediprog_write(CMD_SET_IO_LED, target_leds, 0, NULL, 0);
	} else {
		if (dediprog_firmwareversion < FIRMWARE_VERSION(5, 0, 0)) {
			target_leds = ((leds & LED_ERROR) >> 2) | ((leds & LED_PASS) << 2);
		} else {
			target_leds = leds;
		}
		target_leds ^= 7;

		ret = dediprog_write(CMD_SET_IO_LED, 0x9, target_leds, NULL, 0);
	}

	if (ret != 0x0) {
		msg_perr("Command Set LED 0x%x failed (%s)!\n", leds, usb_strerror());
		return 1;
	}

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
	ret = dediprog_write(CMD_SET_VCC, voltage_selector, 0, NULL, 0);
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

struct dediprog_spispeeds {
	const char *const name;
	const int speed;
};

static const struct dediprog_spispeeds spispeeds[] = {
	{ "24M",	0x0 },
	{ "12M",	0x2 },
	{ "8M",		0x1 },
	{ "3M",		0x3 },
	{ "2.18M",	0x4 },
	{ "1.5M",	0x5 },
	{ "750k",	0x6 },
	{ "375k",	0x7 },
	{ NULL,		0x0 },
};

static int dediprog_set_spi_speed(unsigned int spispeed_idx)
{
	if (dediprog_firmwareversion < FIRMWARE_VERSION(5, 0, 0)) {
		msg_pwarn("Skipping to set SPI speed because firmware is too old.\n");
		return 0;
	}

	const struct dediprog_spispeeds *spispeed = &spispeeds[spispeed_idx];
	msg_pdbg("SPI speed is %sHz\n", spispeed->name);

	int ret = dediprog_write(CMD_SET_SPI_CLK, spispeed->speed, 0, NULL, 0);
	if (ret != 0x0) {
		msg_perr("Command Set SPI Speed 0x%x failed!\n", spispeed->speed);
		return 1;
	}
	return 0;
}

static void fill_rw_cmd_payload(uint8_t *data_packet, unsigned int count, uint8_t dedi_spi_cmd, unsigned int *value, unsigned int *idx, unsigned int start) {
	/* First 5 bytes are common in both generations. */
	data_packet[0] = count & 0xff;
	data_packet[1] = (count >> 8) & 0xff;
	data_packet[2] = 0; /* RFU */
	data_packet[3] = dedi_spi_cmd; /* Read/Write Mode (currently READ_MODE_STD, WRITE_MODE_PAGE_PGM or WRITE_MODE_2B_AAI) */
	data_packet[4] = 0; /* "Opcode". Specs imply necessity only for READ_MODE_4B_ADDR_FAST and WRITE_MODE_4B_ADDR_256B_PAGE_PGM */

	if (is_new_prot()) {
		*value = *idx = 0;
		data_packet[5] = 0; /* RFU */
		data_packet[6] = (start >>  0) & 0xff;
		data_packet[7] = (start >>  8) & 0xff;
		data_packet[8] = (start >> 16) & 0xff;
		data_packet[9] = (start >> 24) & 0xff;
	} else {
		*value = start % 0x10000;
		*idx = start / 0x10000;
	}
}

/* Bulk read interface, will read multiple 512 byte chunks aligned to 512 bytes.
 * @start	start address
 * @len		length
 * @return	0 on success, 1 on failure
 */
static int dediprog_spi_bulk_read(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	/* chunksize must be 512, other sizes will NOT work at all. */
	const unsigned int chunksize = 512;
	const unsigned int count = len / chunksize;

	if ((start % chunksize) || (len % chunksize)) {
		msg_perr("%s: Unaligned start=%i, len=%i! Please report a bug at flashrom@flashrom.org\n",
			 __func__, start, len);
		return 1;
	}

	if (len == 0)
		return 0;

	/* Command packet size of protocols: new 10 B, old 5 B. */
	uint8_t data_packet[is_new_prot() ? 10 : 5];
	unsigned int value, idx;
	fill_rw_cmd_payload(data_packet, count, READ_MODE_STD, &value, &idx, start);

	int ret = dediprog_write(CMD_READ, value, idx, data_packet, sizeof(data_packet));
	if (ret != sizeof(data_packet)) {
		msg_perr("Command Read SPI Bulk failed, %i %s!\n", ret, usb_strerror());
		return 1;
	}

	unsigned int i;
	for (i = 0; i < count; i++) {
		ret = usb_bulk_read(dediprog_handle, 0x80 | dediprog_endpoint,
				    (char *)buf + i * chunksize, chunksize,
				    DEFAULT_TIMEOUT);
		if (ret != chunksize) {
			msg_perr("SPI bulk read %i failed, expected %i, got %i %s!\n",
				 i, chunksize, ret, usb_strerror());
			return 1;
		}
	}

	return 0;
}

static int dediprog_spi_read(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	int ret;
	/* chunksize must be 512, other sizes will NOT work at all. */
	const unsigned int chunksize = 0x200;
	unsigned int residue = start % chunksize ? chunksize - start % chunksize : 0;
	unsigned int bulklen;

	dediprog_set_leds(LED_BUSY);

	if (residue) {
		msg_pdbg("Slow read for partial block from 0x%x, length 0x%x\n",
			 start, residue);
		ret = spi_read_chunked(flash, buf, start, residue, 16);
		if (ret)
			goto err;
	}

	/* Round down. */
	bulklen = (len - residue) / chunksize * chunksize;
	ret = dediprog_spi_bulk_read(flash, buf + residue, start + residue, bulklen);
	if (ret)
		goto err;

	len -= residue + bulklen;
	if (len != 0) {
		msg_pdbg("Slow read for partial block from 0x%x, length 0x%x\n",
			 start, len);
		ret = spi_read_chunked(flash, buf + residue + bulklen,
				       start + residue + bulklen, len, 16);
		if (ret)
			goto err;
	}

	dediprog_set_leds(LED_PASS);
	return 0;
err:
	dediprog_set_leds(LED_ERROR);
	return ret;
}

/* Bulk write interface, will write multiple chunksize byte chunks aligned to chunksize bytes.
 * @chunksize       length of data chunks, only 256 supported by now
 * @start           start address
 * @len             length
 * @dedi_spi_cmd    dediprog specific write command for spi bus
 * @return          0 on success, 1 on failure
 */
static int dediprog_spi_bulk_write(struct flashctx *flash, const uint8_t *buf, unsigned int chunksize,
				   unsigned int start, unsigned int len, uint8_t dedi_spi_cmd)
{
	/* USB transfer size must be 512, other sizes will NOT work at all.
	 * chunksize is the real data size per USB bulk transfer. The remaining
	 * space in a USB bulk transfer must be filled with 0xff padding.
	 */
	const unsigned int count = len / chunksize;

	/*
	 * We should change this check to
	 *   chunksize > 512
	 * once we know how to handle different chunk sizes.
	 */
	if (chunksize != 256) {
		msg_perr("%s: Chunk sizes other than 256 bytes are unsupported, chunksize=%u!\n"
			 "Please report a bug at flashrom@flashrom.org\n", __func__, chunksize);
		return 1;
	}

	if ((start % chunksize) || (len % chunksize)) {
		msg_perr("%s: Unaligned start=%i, len=%i! Please report a bug "
			 "at flashrom@flashrom.org\n", __func__, start, len);
		return 1;
	}

	/* No idea if the hardware can handle empty writes, so chicken out. */
	if (len == 0)
		return 0;

	/* Command packet size of protocols: new 10 B, old 5 B. */
	uint8_t data_packet[is_new_prot() ? 10 : 5];
	unsigned int value, idx;
	fill_rw_cmd_payload(data_packet, count, dedi_spi_cmd, &value, &idx, start);
	int ret = dediprog_write(CMD_WRITE, value, idx, data_packet, sizeof(data_packet));
	if (ret != sizeof(data_packet)) {
		msg_perr("Command Write SPI Bulk failed, %i %s!\n", ret,
			 usb_strerror());
		return 1;
	}

	unsigned int i;
	for (i = 0; i < count; i++) {
		char usbbuf[512];
		memcpy(usbbuf, buf + i * chunksize, chunksize);
		memset(usbbuf + chunksize, 0xff, sizeof(usbbuf) - chunksize); // fill up with 0xFF
		ret = usb_bulk_write(dediprog_handle, dediprog_endpoint, usbbuf, 512, DEFAULT_TIMEOUT);
		if (ret != 512) {
			msg_perr("SPI bulk write failed, expected %i, got %i %s!\n", 512, ret, usb_strerror());
			return 1;
		}
	}

	return 0;
}

static int dediprog_spi_write(struct flashctx *flash, const uint8_t *buf,
			      unsigned int start, unsigned int len, uint8_t dedi_spi_cmd)
{
	int ret;
	const unsigned int chunksize = flash->chip->page_size;
	unsigned int residue = start % chunksize ? chunksize - start % chunksize : 0;
	unsigned int bulklen;

	dediprog_set_leds(LED_BUSY);

	if (chunksize != 256) {
		msg_pdbg("Page sizes other than 256 bytes are unsupported as "
			 "we don't know how dediprog\nhandles them.\n");
		/* Write everything like it was residue. */
		residue = len;
	}

	if (residue) {
		msg_pdbg("Slow write for partial block from 0x%x, length 0x%x\n",
			 start, residue);
		/* No idea about the real limit. Maybe 12, maybe more. */
		ret = spi_write_chunked(flash, buf, start, residue, 12);
		if (ret) {
			dediprog_set_leds(LED_ERROR);
			return ret;
		}
	}

	/* Round down. */
	bulklen = (len - residue) / chunksize * chunksize;
	ret = dediprog_spi_bulk_write(flash, buf + residue, chunksize, start + residue, bulklen, dedi_spi_cmd);
	if (ret) {
		dediprog_set_leds(LED_ERROR);
		return ret;
	}

	len -= residue + bulklen;
	if (len) {
		msg_pdbg("Slow write for partial block from 0x%x, length 0x%x\n",
			 start, len);
		ret = spi_write_chunked(flash, buf + residue + bulklen,
				        start + residue + bulklen, len, 12);
		if (ret) {
			dediprog_set_leds(LED_ERROR);
			return ret;
		}
	}

	dediprog_set_leds(LED_PASS);
	return 0;
}

static int dediprog_spi_write_256(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	return dediprog_spi_write(flash, buf, start, len, WRITE_MODE_PAGE_PGM);
}

static int dediprog_spi_write_aai(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len)
{
	return dediprog_spi_write(flash, buf, start, len, WRITE_MODE_2B_AAI);
}

static int dediprog_spi_send_command(struct flashctx *flash,
				     unsigned int writecnt,
				     unsigned int readcnt,
				     const unsigned char *writearr,
				     unsigned char *readarr)
{
	int ret;

	msg_pspew("%s, writecnt=%i, readcnt=%i\n", __func__, writecnt, readcnt);
	if (writecnt > flash->mst->spi.max_data_write) {
		msg_perr("Invalid writecnt=%i, aborting.\n", writecnt);
		return 1;
	}
	if (readcnt > flash->mst->spi.max_data_read) {
		msg_perr("Invalid readcnt=%i, aborting.\n", readcnt);
		return 1;
	}
	
	unsigned int idx, value;
	/* New protocol has options and timeout combined as value while the old one used the value field for
	 * timeout and the index field for options. */
	if (is_new_prot()) {
		idx = 0;
		value = readcnt ? 0x1 : 0x0; // Indicate if we require a read
	} else {
		idx = readcnt ? 0x1 : 0x0; // Indicate if we require a read
		value = 0;
	}
	ret = dediprog_write(CMD_TRANSCEIVE, value, idx, writearr, writecnt);
	if (ret != writecnt) {
		msg_perr("Send SPI failed, expected %i, got %i %s!\n",
			 writecnt, ret, usb_strerror());
		return 1;
	}
	if (readcnt == 0) // If we don't require a response, we are done here
		return 0;

	const uint8_t read_timeout = 10 + readcnt/512;
	if (is_new_prot()) {
		idx = 0;
		value = min(read_timeout, 0xFF) | (0 << 8) ; // Timeout in lower byte, option in upper byte
	} else {
		idx = (0 & 0xFF);  // Lower byte is option (0x01 = require SR, 0x02 keep CS low)
		value = min(read_timeout, 0xFF); // Possibly two bytes but we play safe here
	}
	ret = dediprog_read(CMD_TRANSCEIVE, value, idx, readarr, readcnt);
	if (ret != readcnt) {
		msg_perr("Receive SPI failed, expected %i, got %i %s!\n", readcnt, ret, usb_strerror());
		return 1;
	}
	return 0;
}

static int dediprog_check_devicestring(void)
{
	int ret;
	int fw[3];
	char buf[0x11];

	/* Command Receive Device String. */
	ret = dediprog_read(CMD_READ_PROG_INFO, 0, 0, (uint8_t *)buf, 0x10);
	if (ret != 0x10) {
		msg_perr("Incomplete/failed Command Receive Device String!\n");
		return 1;
	}
	buf[0x10] = '\0';
	msg_pdbg("Found a %s\n", buf);
	if (memcmp(buf, "SF100", 0x5) != 0) {
		msg_perr("Device not a SF100!\n");
		return 1;
	}
	if (sscanf(buf, "SF100 V:%d.%d.%d ", &fw[0], &fw[1], &fw[2]) != 3) {
		msg_perr("Unexpected firmware version string '%s'\n", buf);
		return 1;
	}
	/* Only these major versions were tested. */
	if (fw[0] < 2 || fw[0] > 6) {
		msg_perr("Unexpected firmware version %d.%d.%d!\n", fw[0], fw[1], fw[2]);
		return 1;
	}
	dediprog_firmwareversion = FIRMWARE_VERSION(fw[0], fw[1], fw[2]);

	return 0;
}

static int dediprog_device_init(void)
{
	int ret;
	char buf[0x1];

	memset(buf, 0, sizeof(buf));
	ret = usb_control_msg(dediprog_handle, REQTYPE_OTHER_IN, 0x0B, 0x0, 0x0,
			      buf, 0x1, DEFAULT_TIMEOUT);
	if (ret < 0) {
		msg_perr("Command A failed (%s)!\n", usb_strerror());
		return 1;
	}
	if ((ret != 0x1) || (buf[0] != 0x6f)) {
		msg_perr("Unexpected response to init!\n");
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

	ret = usb_control_msg(dediprog_handle, REQTYPE_OTHER_IN, 0x7, 0x0, 0xef00,
			      buf, 0x3, DEFAULT_TIMEOUT);
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

static int set_target_flash(enum dediprog_target target)
{
	int ret = usb_control_msg(dediprog_handle, REQTYPE_EP_OUT, CMD_SET_TARGET, target, 0,
			          NULL, 0, DEFAULT_TIMEOUT);
	if (ret != 0) {
		msg_perr("set_target_flash failed (%s)!\n", usb_strerror());
		return 1;
	}
	return 0;
}

#if 0
/* Returns true if the button is currently pressed. */
static bool dediprog_get_button(void)
{
	char buf[1];
	int ret = usb_control_msg(dediprog_handle, REQTYPE_EP_IN, CMD_GET_BUTTON, 0, 0,
			      buf, 0x1, DEFAULT_TIMEOUT);
	if (ret != 0) {
		msg_perr("Could not get button state (%s)!\n", usb_strerror());
		return 1;
	}
	return buf[0] != 1;
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

static const struct spi_master spi_master_dediprog = {
	.type		= SPI_CONTROLLER_DEDIPROG,
	.max_data_read	= 16, /* 18 seems to work fine as well, but 19 times out sometimes with FW 5.15. */
	.max_data_write	= 16,
	.command	= dediprog_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= dediprog_spi_read,
	.write_256	= dediprog_spi_write_256,
	.write_aai	= dediprog_spi_write_aai,
};

static int dediprog_shutdown(void *data)
{
	dediprog_firmwareversion = FIRMWARE_VERSION(0, 0, 0);

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

int dediprog_init(void)
{
	struct usb_device *dev;
	char *voltage, *device, *spispeed, *target_str;
	int spispeed_idx = 1;
	int millivolt = 3500;
	long usedevice = 0;
	long target = 1;
	int i, ret;

	spispeed = extract_programmer_param("spispeed");
	if (spispeed) {
		for (i = 0; spispeeds[i].name; ++i) {
			if (!strcasecmp(spispeeds[i].name, spispeed)) {
				spispeed_idx = i;
				break;
			}
		}
		if (!spispeeds[i].name) {
			msg_perr("Error: Invalid spispeed value: '%s'.\n", spispeed);
			free(spispeed);
			return 1;
		}
		free(spispeed);
	}

	voltage = extract_programmer_param("voltage");
	if (voltage) {
		millivolt = parse_voltage(voltage);
		free(voltage);
		if (millivolt < 0)
			return 1;
		msg_pinfo("Setting voltage to %i mV\n", millivolt);
	}

	device = extract_programmer_param("device");
	if (device) {
		char *dev_suffix;
		errno = 0;
		usedevice = strtol(device, &dev_suffix, 10);
		if (errno != 0 || device == dev_suffix) {
			msg_perr("Error: Could not convert 'device'.\n");
			free(device);
			return 1;
		}
		if (usedevice < 0 || usedevice > UINT_MAX) {
			msg_perr("Error: Value for 'device' is out of range.\n");
			free(device);
			return 1;
		}
		if (strlen(dev_suffix) > 0) {
			msg_perr("Error: Garbage following 'device' value.\n");
			free(device);
			return 1;
		}
		msg_pinfo("Using device %li.\n", usedevice);
	}
	free(device);

	target_str = extract_programmer_param("target");
	if (target_str) {
		char *target_suffix;
		errno = 0;
		target = strtol(target_str, &target_suffix, 10);
		if (errno != 0 || target_str == target_suffix) {
			msg_perr("Error: Could not convert 'target'.\n");
			free(target_str);
			return 1;
		}
		if (target < 1 || target > 2) {
			msg_perr("Error: Value for 'target' is out of range.\n");
			free(target_str);
			return 1;
		}
		if (strlen(target_suffix) > 0) {
			msg_perr("Error: Garbage following 'target' value.\n");
			free(target_str);
			return 1;
		}
		msg_pinfo("Using target %li.\n", target);
	}
	free(target_str);

	/* Here comes the USB stuff. */
	usb_init();
	usb_find_busses();
	usb_find_devices();
	dev = get_device_by_vid_pid(0x0483, 0xdada, (unsigned int) usedevice);
	if (!dev) {
		msg_perr("Could not find a Dediprog SF100 on USB!\n");
		return 1;
	}
	msg_pdbg("Found USB device (%04x:%04x).\n",
		 dev->descriptor.idVendor, dev->descriptor.idProduct);
	dediprog_handle = usb_open(dev);
	if (!dediprog_handle) {
		msg_perr("Could not open USB device: %s\n", usb_strerror());
		return 1;
	}
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

	/* Perform basic setup. */
	if (dediprog_device_init())
		return 1;
	if (dediprog_check_devicestring())
		return 1;

	/* Set all possible LEDs as soon as possible to indicate activity.
	 * Because knowing the firmware version is required to set the LEDs correctly we need to this after
	 * dediprog_check_devicestring() has queried the device and set dediprog_firmwareversion. */
	dediprog_set_leds(LED_ALL);

	/* Select target/socket, frequency and VCC. */
	if (set_target_flash(FLASH_TYPE_APPLICATION_FLASH_1) ||
	    dediprog_set_spi_speed(spispeed_idx) ||
	    dediprog_set_spi_voltage(millivolt)) {
		dediprog_set_leds(LED_ERROR);
		return 1;
	}

	if (register_spi_master(&spi_master_dediprog) || dediprog_set_leds(LED_NONE))
		return 1;

	return 0;
}

