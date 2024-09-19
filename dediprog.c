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
 */

#include <sys/types.h>
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

/* LIBUSB_CALL ensures the right calling conventions on libusb callbacks.
 * However, the macro is not defined everywhere. m(
 */
#ifndef LIBUSB_CALL
#define LIBUSB_CALL
#endif

#define FIRMWARE_VERSION(x,y,z) ((x << 16) | (y << 8) | z)
#define DEFAULT_TIMEOUT 3000
#define DEDIPROG_ASYNC_TRANSFERS 8 /* at most 8 asynchronous transfers */
#define REQTYPE_OTHER_OUT (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_OTHER)	/* 0x43 */
#define REQTYPE_OTHER_IN (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_OTHER)	/* 0xC3 */
#define REQTYPE_EP_OUT (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_ENDPOINT)	/* 0x42 */
#define REQTYPE_EP_IN (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_ENDPOINT)	/* 0xC2 */

enum dediprog_devtype {
	DEV_UNKNOWN		= 0,
	DEV_SF100		= 100,
	DEV_SF200		= 200,
	DEV_SF600		= 600,
};

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
	CMD_SET_VOLTAGE		= 0x0B,	/* Only in firmware older than 6.0.0 */
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
	WRITE_MODE_PAGE_PGM			= 1,
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

enum dediprog_standalone_mode {
	ENTER_STANDALONE_MODE = 0,
	LEAVE_STANDALONE_MODE = 1,
};

/*
 * These are not official designations; they are for use in flashrom only.
 * Order must be preserved so that comparison operators work.
 */
enum protocol {
	PROTOCOL_UNKNOWN,
	PROTOCOL_V1,
	PROTOCOL_V2,
	PROTOCOL_V3,
};

static const struct dev_entry devs_dediprog[] = {
	{0x0483, 0xDADA, OK, "Dediprog", "SF100/SF200/SF600"},

	{0},
};

struct dediprog_data {
	struct libusb_context *usb_ctx;
	libusb_device_handle *handle;
	int in_endpoint;
	int out_endpoint;
	int firmwareversion;
	enum dediprog_devtype devicetype;
};

#if defined(LIBUSB_MAJOR) && defined(LIBUSB_MINOR) && defined(LIBUSB_MICRO) && \
    LIBUSB_MAJOR <= 1 && LIBUSB_MINOR == 0 && LIBUSB_MICRO < 9
/* Quick and dirty replacement for missing libusb_error_name in libusb < 1.0.9 */
const char * LIBUSB_CALL libusb_error_name(int error_code)
{
	if (error_code >= INT16_MIN && error_code <= INT16_MAX) {
		/* 18 chars for text, rest for number (16 b should be enough), sign, nullbyte. */
		static char my_libusb_error[18 + 5 + 2];
		sprintf(my_libusb_error, "libusb error code %i", error_code);
		return my_libusb_error;
	} else {
		return "UNKNOWN";
	}
}
#endif

static enum protocol protocol(const struct dediprog_data *dp_data)
{
	/* Firmware version < 5.0.0 is handled explicitly in some cases. */
	switch (dp_data->devicetype) {
	case DEV_SF100:
	case DEV_SF200:
		if (dp_data->firmwareversion < FIRMWARE_VERSION(5, 5, 0))
			return PROTOCOL_V1;
		else
			return PROTOCOL_V2;
	case DEV_SF600:
		if (dp_data->firmwareversion < FIRMWARE_VERSION(6, 9, 0))
			return PROTOCOL_V1;
		else if (dp_data->firmwareversion <= FIRMWARE_VERSION(7, 2, 21))
			return PROTOCOL_V2;
		else
			return PROTOCOL_V3;
	default:
		return PROTOCOL_UNKNOWN;
	}
}

struct dediprog_transfer_status {
	int error; /* OK if 0, ERROR else */
	unsigned int queued_idx;
	unsigned int finished_idx;
};

static void LIBUSB_CALL dediprog_bulk_read_cb(struct libusb_transfer *const transfer)
{
	struct dediprog_transfer_status *const status = (struct dediprog_transfer_status *)transfer->user_data;
	if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
		status->error = 1;
		msg_perr("SPI bulk read failed!\n");
	}
	++status->finished_idx;
}

static int dediprog_bulk_read_poll(struct libusb_context *usb_ctx,
				   const struct dediprog_transfer_status *const status,
				   const int finish)
{
	if (status->finished_idx >= status->queued_idx)
		return 0;

	do {
		struct timeval timeout = { 10, 0 };
		const int ret = libusb_handle_events_timeout(usb_ctx, &timeout);
		if (ret < 0) {
			msg_perr("Polling read events failed: %i %s!\n", ret, libusb_error_name(ret));
			return 1;
		}
	} while (finish && (status->finished_idx < status->queued_idx));
	return 0;
}

static int dediprog_read(libusb_device_handle *dediprog_handle,
			 enum dediprog_cmds cmd, unsigned int value, unsigned int idx,
			 uint8_t *bytes, size_t size)
{
	return libusb_control_transfer(dediprog_handle, REQTYPE_EP_IN, cmd, value, idx,
				      (unsigned char *)bytes, size, DEFAULT_TIMEOUT);
}

static int dediprog_write(libusb_device_handle *dediprog_handle,
			  enum dediprog_cmds cmd, unsigned int value, unsigned int idx,
			  const uint8_t *bytes, size_t size)
{
	return libusb_control_transfer(dediprog_handle, REQTYPE_EP_OUT, cmd, value, idx,
				      (unsigned char *)bytes, size, DEFAULT_TIMEOUT);
}


/* This function sets the GPIOs connected to the LEDs as well as IO1-IO4. */
static int dediprog_set_leds(int leds, const struct dediprog_data *dp_data)
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
	if (protocol(dp_data) >= PROTOCOL_V2) {
		target_leds = (leds ^ 7) << 8;
		ret = dediprog_write(dp_data->handle, CMD_SET_IO_LED, target_leds, 0, NULL, 0);
	} else {
		if (dp_data->firmwareversion < FIRMWARE_VERSION(5, 0, 0)) {
			target_leds = ((leds & LED_ERROR) >> 2) | ((leds & LED_PASS) << 2);
		} else {
			target_leds = leds;
		}
		target_leds ^= 7;

		ret = dediprog_write(dp_data->handle, CMD_SET_IO_LED, 0x9, target_leds, NULL, 0);
	}

	if (ret != 0x0) {
		msg_perr("Command Set LED 0x%x failed (%s)!\n", leds, libusb_error_name(ret));
		return 1;
	}

	return 0;
}

static int dediprog_set_spi_voltage(libusb_device_handle *dediprog_handle, int millivolt)
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
		default_delay(200 * 1000);
	}
	ret = dediprog_write(dediprog_handle, CMD_SET_VCC, voltage_selector, 0, NULL, 0);
	if (ret != 0x0) {
		msg_perr("Command Set SPI Voltage 0x%x failed!\n",
			 voltage_selector);
		return 1;
	}
	if (voltage_selector != 0) {
		/* Wait some time as the original driver does. */
		default_delay(200 * 1000);
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

static int dediprog_set_spi_speed(unsigned int spispeed_idx, const struct dediprog_data *dp_data)
{
	if (dp_data->firmwareversion < FIRMWARE_VERSION(5, 0, 0)) {
		msg_pwarn("Skipping to set SPI speed because firmware is too old.\n");
		return 0;
	}

	const struct dediprog_spispeeds *spispeed = &spispeeds[spispeed_idx];
	msg_pdbg("SPI speed is %sHz\n", spispeed->name);

	int ret = dediprog_write(dp_data->handle, CMD_SET_SPI_CLK, spispeed->speed, 0, NULL, 0);
	if (ret != 0x0) {
		msg_perr("Command Set SPI Speed 0x%x failed!\n", spispeed->speed);
		return 1;
	}
	return 0;
}

static int prepare_rw_cmd(
		struct flashctx *const flash, uint8_t *data_packet, unsigned int count,
		uint8_t dedi_spi_cmd, unsigned int *value, unsigned int *idx, unsigned int start, int is_read)
{
	const struct dediprog_data *dp_data = flash->mst->spi.data;

	if (count >= 1 << 16) {
		msg_perr("%s: Unsupported transfer length of %u blocks! "
			 "Please report a bug at flashrom@flashrom.org\n",
			 __func__, count);
		return 1;
	}

	/* First 5 bytes are common in both generations. */
	data_packet[0] = count & 0xff;
	data_packet[1] = (count >> 8) & 0xff;
	data_packet[2] = 0; /* RFU */
	data_packet[3] = dedi_spi_cmd; /* Read/Write Mode (currently READ_MODE_STD, WRITE_MODE_PAGE_PGM or WRITE_MODE_2B_AAI) */
	data_packet[4] = 0; /* "Opcode". Specs imply necessity only for READ_MODE_4B_ADDR_FAST and WRITE_MODE_4B_ADDR_256B_PAGE_PGM */

	if (protocol(dp_data) >= PROTOCOL_V2) {
		if (is_read && flash->chip->feature_bits & FEATURE_4BA_FAST_READ) {
			data_packet[3] = READ_MODE_4B_ADDR_FAST_0x0C;
			data_packet[4] = JEDEC_READ_4BA_FAST;
		} else if (dedi_spi_cmd == WRITE_MODE_PAGE_PGM
			   && (flash->chip->feature_bits & FEATURE_4BA_WRITE)) {
			data_packet[3] = WRITE_MODE_4B_ADDR_256B_PAGE_PGM_0x12;
			data_packet[4] = JEDEC_BYTE_PROGRAM_4BA;
		}

		*value = *idx = 0;
		data_packet[5] = 0; /* RFU */
		data_packet[6] = (start >>  0) & 0xff;
		data_packet[7] = (start >>  8) & 0xff;
		data_packet[8] = (start >> 16) & 0xff;
		data_packet[9] = (start >> 24) & 0xff;
		if (protocol(dp_data) >= PROTOCOL_V3) {
			if (is_read) {
				data_packet[10] = 0x00;	/* address length (3 or 4) */
				data_packet[11] = 0x00;	/* dummy cycle / 2 */
			} else {
				/* 16 LSBs and 16 HSBs of page size */
				/* FIXME: This assumes page size of 256. */
				data_packet[10] = 0x00;
				data_packet[11] = 0x01;
				data_packet[12] = 0x00;
				data_packet[13] = 0x00;
			}
		}
	} else {
		if (flash->chip->feature_bits & FEATURE_4BA_EAR_ANY) {
			if (spi_set_extended_address(flash, start >> 24))
				return 1;
		} else if (start >> 24) {
			msg_cerr("Can't handle 4-byte address with dediprog.\n");
			return 1;
		}
		/*
		 * We don't know how the dediprog firmware handles 4-byte
		 * addresses. So let's not tell it what we are doing and
		 * only send the lower 3 bytes.
		 */
		*value = start & 0xffff;
		*idx = (start >> 16) & 0xff;
	}

	return 0;
}

/* Bulk read interface, will read multiple 512 byte chunks aligned to 512 bytes.
 * @start	start address
 * @len		length
 * @return	0 on success, 1 on failure
 */
static int dediprog_spi_bulk_read(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	int err = 1;
	const struct dediprog_data *dp_data = flash->mst->spi.data;

	/* chunksize must be 512, other sizes will NOT work at all. */
	const unsigned int chunksize = 512;
	const unsigned int count = len / chunksize;

	struct dediprog_transfer_status status = { 0, 0, 0 };
	struct libusb_transfer *transfers[DEDIPROG_ASYNC_TRANSFERS] = { NULL, };
	struct libusb_transfer *transfer;

	if (len == 0)
		return 0;

	if ((start % chunksize) || (len % chunksize)) {
		msg_perr("%s: Unaligned start=%i, len=%i! Please report a bug at flashrom@flashrom.org\n",
			 __func__, start, len);
		return 1;
	}

	int command_packet_size;
	switch (protocol(dp_data)) {
	case PROTOCOL_V1:
		command_packet_size = 5;
		break;
	case PROTOCOL_V2:
		command_packet_size = 10;
		break;
	case PROTOCOL_V3:
		command_packet_size = 12;
		break;
	default:
		return 1;
	}

	uint8_t data_packet[command_packet_size];
	unsigned int value, idx;
	if (prepare_rw_cmd(flash, data_packet, count, READ_MODE_STD, &value, &idx, start, 1))
		return 1;

	int ret = dediprog_write(dp_data->handle, CMD_READ, value, idx, data_packet, sizeof(data_packet));
	if (ret != (int)sizeof(data_packet)) {
		msg_perr("Command Read SPI Bulk failed, %i %s!\n", ret, libusb_error_name(ret));
		return 1;
	}

	/*
	 * Ring buffer of bulk transfers.
	 * Poll until at least one transfer is ready,
	 * schedule next transfers until buffer is full.
	 */

	/* Allocate bulk transfers. */
	unsigned int i;
	for (i = 0; i < MIN(DEDIPROG_ASYNC_TRANSFERS, count); ++i) {
		transfers[i] = libusb_alloc_transfer(0);
		if (!transfers[i]) {
			msg_perr("Allocating libusb transfer %i failed: %s!\n", i, libusb_error_name(ret));
			goto err_free;
		}
	}

	/* Now transfer requested chunks using libusb's asynchronous interface. */
	while (!status.error && (status.queued_idx < count)) {
		while ((status.queued_idx < count) &&
		       (status.queued_idx - status.finished_idx) < DEDIPROG_ASYNC_TRANSFERS)
		{
			transfer = transfers[status.queued_idx % DEDIPROG_ASYNC_TRANSFERS];
			libusb_fill_bulk_transfer(transfer, dp_data->handle, 0x80 | dp_data->in_endpoint,
					(unsigned char *)buf + status.queued_idx * chunksize, chunksize,
					dediprog_bulk_read_cb, &status, DEFAULT_TIMEOUT);
			transfer->flags |= LIBUSB_TRANSFER_SHORT_NOT_OK;
			ret = libusb_submit_transfer(transfer);
			if (ret < 0) {
				msg_perr("Submitting SPI bulk read %i failed: %s!\n",
					 status.queued_idx, libusb_error_name(ret));
				goto err_free;
			}
			++status.queued_idx;
		}
		if (dediprog_bulk_read_poll(dp_data->usb_ctx, &status, 0))
			goto err_free;
	}
	/* Wait for transfers to finish. */
	if (dediprog_bulk_read_poll(dp_data->usb_ctx, &status, 1))
		goto err_free;
	/* Check if everything has been transmitted. */
	if ((status.finished_idx < count) || status.error)
		goto err_free;

	err = 0;

err_free:
	dediprog_bulk_read_poll(dp_data->usb_ctx, &status, 1);
	for (i = 0; i < DEDIPROG_ASYNC_TRANSFERS; ++i)
		if (transfers[i]) libusb_free_transfer(transfers[i]);
	return err;
}

static int dediprog_spi_read(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len)
{
	int ret;
	/* chunksize must be 512, other sizes will NOT work at all. */
	const unsigned int chunksize = 0x200;
	unsigned int residue = start % chunksize ? min(len, chunksize - start % chunksize) : 0;
	unsigned int bulklen;
	const struct dediprog_data *dp_data = flash->mst->spi.data;

	dediprog_set_leds(LED_BUSY, dp_data);

	if (residue) {
		msg_pdbg("Slow read for partial block from 0x%x, length 0x%x\n",
			 start, residue);
		ret = default_spi_read(flash, buf, start, residue);
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
		ret = default_spi_read(flash, buf + residue + bulklen,
				       start + residue + bulklen, len);
		if (ret)
			goto err;
	}

	dediprog_set_leds(LED_PASS, dp_data);
	return 0;
err:
	dediprog_set_leds(LED_ERROR, dp_data);
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
	/* USB transfer size must be 256, other sizes will NOT work at all.
	 * chunksize is the real data size per USB bulk transfer. The remaining
	 * space in a USB bulk transfer must be filled with 0xff padding.
	 */
	const unsigned int count = len / chunksize;
	const struct dediprog_data *dp_data = flash->mst->spi.data;

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

	int command_packet_size;
	switch (protocol(dp_data)) {
	case PROTOCOL_V1:
		command_packet_size = 5;
		break;
	case PROTOCOL_V2:
		command_packet_size = 10;
		break;
	case PROTOCOL_V3:
		command_packet_size = 14;
		break;
	default:
		return 1;
	}

	uint8_t data_packet[command_packet_size];
	unsigned int value, idx;
	if (prepare_rw_cmd(flash, data_packet, count, dedi_spi_cmd, &value, &idx, start, 0))
		return 1;
	int ret = dediprog_write(dp_data->handle, CMD_WRITE, value, idx, data_packet, sizeof(data_packet));
	if (ret != (int)sizeof(data_packet)) {
		msg_perr("Command Write SPI Bulk failed, %s!\n", libusb_error_name(ret));
		return 1;
	}

	unsigned int i;
	for (i = 0; i < count; i++) {
		unsigned char usbbuf[512];
		memcpy(usbbuf, buf + i * chunksize, chunksize);
		memset(usbbuf + chunksize, 0xff, sizeof(usbbuf) - chunksize); // fill up with 0xFF
		int transferred;
		ret = libusb_bulk_transfer(dp_data->handle, dp_data->out_endpoint, usbbuf, 512, &transferred,
					   DEFAULT_TIMEOUT);
		if ((ret < 0) || (transferred != 512)) {
			msg_perr("SPI bulk write failed, expected %i, got %s!\n", 512, libusb_error_name(ret));
			return 1;
		}
		update_progress(flash, FLASHROM_PROGRESS_WRITE, i + 1, count);
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
	const struct dediprog_data *dp_data = flash->mst->spi.data;

	dediprog_set_leds(LED_BUSY, dp_data);

	if (chunksize != 256) {
		msg_pdbg("Page sizes other than 256 bytes are unsupported as "
			 "we don't know how dediprog\nhandles them.\n");
		/* Write everything like it was residue. */
		residue = len;
	}

	if (residue) {
		msg_pdbg("Slow write for partial block from 0x%x, length 0x%x\n",
			 start, residue);
		/* No idea about the real limit. Maybe 16 including command and address, maybe more. */
		ret = spi_write_chunked(flash, buf, start, residue, 11);
		if (ret) {
			dediprog_set_leds(LED_ERROR, dp_data);
			return ret;
		}
	}

	/* Round down. */
	bulklen = (len - residue) / chunksize * chunksize;
	ret = dediprog_spi_bulk_write(flash, buf + residue, chunksize, start + residue, bulklen, dedi_spi_cmd);
	if (ret) {
		dediprog_set_leds(LED_ERROR, dp_data);
		return ret;
	}

	len -= residue + bulklen;
	if (len) {
		msg_pdbg("Slow write for partial block from 0x%x, length 0x%x\n",
			 start, len);
		ret = spi_write_chunked(flash, buf + residue + bulklen,
				        start + residue + bulklen, len, 11);
		if (ret) {
			dediprog_set_leds(LED_ERROR, dp_data);
			return ret;
		}
	}

	dediprog_set_leds(LED_PASS, dp_data);
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

static int dediprog_spi_send_command(const struct flashctx *flash,
				     unsigned int writecnt,
				     unsigned int readcnt,
				     const unsigned char *writearr,
				     unsigned char *readarr)
{
	int ret;
	const struct dediprog_data *dp_data = flash->mst->spi.data;

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
	if (protocol(dp_data) >= PROTOCOL_V2) {
		idx = 0;
		value = readcnt ? 0x1 : 0x0; // Indicate if we require a read
	} else {
		idx = readcnt ? 0x1 : 0x0; // Indicate if we require a read
		value = 0;
	}
	ret = dediprog_write(dp_data->handle, CMD_TRANSCEIVE, value, idx, writearr, writecnt);
	if (ret != (int)writecnt) {
		msg_perr("Send SPI failed, expected %i, got %i %s!\n",
			 writecnt, ret, libusb_error_name(ret));
		return 1;
	}
	if (readcnt == 0) // If we don't require a response, we are done here
		return 0;

	/* The specifications do state the possibility to set a timeout for transceive transactions.
	 * Apparently the "timeout" is a delay, and you can use long delays to accelerate writing - in case you
	 * can predict the time needed by the previous command or so (untested). In any case, using this
	 * "feature" to set sane-looking timouts for the read below will completely trash performance with
	 * SF600 and/or firmwares >= 6.0 while they seem to be benign on SF100 with firmwares <= 5.5.2. *shrug*
	 *
	 * The specification also uses only 0 in its examples, so the lesson to learn here:
	 * "Never trust the description of an interface in the documentation but use the example code and pray."
	const uint8_t read_timeout = 10 + readcnt/512;
	if (protocol() >= PROTOCOL_V2) {
		idx = 0;
		value = min(read_timeout, 0xFF) | (0 << 8) ; // Timeout in lower byte, option in upper byte
	} else {
		idx = (0 & 0xFF);  // Lower byte is option (0x01 = require SR, 0x02 keep CS low)
		value = min(read_timeout, 0xFF); // Possibly two bytes but we play safe here
	}
	ret = dediprog_read(dp_data->dediprog_handle, CMD_TRANSCEIVE, value, idx, readarr, readcnt);
	*/
	ret = dediprog_read(dp_data->handle, CMD_TRANSCEIVE, 0, 0, readarr, readcnt);
	if (ret != (int)readcnt) {
		msg_perr("Receive SPI failed, expected %i, got %i %s!\n", readcnt, ret, libusb_error_name(ret));
		return 1;
	}
	return 0;
}

static int dediprog_check_devicestring(struct dediprog_data *dp_data)
{
	int ret;
	char buf[0x11];

	/* Command Receive Device String. */
	ret = dediprog_read(dp_data->handle, CMD_READ_PROG_INFO, 0, 0, (uint8_t *)buf, 0x10);
	if (ret != 0x10) {
		msg_perr("Incomplete/failed Command Receive Device String!\n");
		return 1;
	}
	buf[0x10] = '\0';
	msg_pdbg("Found a %s\n", buf);
	if (memcmp(buf, "SF100", 0x5) == 0)
		dp_data->devicetype = DEV_SF100;
	else if (memcmp(buf, "SF200", 0x5) == 0)
		dp_data->devicetype = DEV_SF200;
	else if (memcmp(buf, "SF600", 0x5) == 0)
		dp_data->devicetype = DEV_SF600;
	else {
		msg_perr("Device not a SF100, SF200, or SF600!\n");
		return 1;
	}

	int sfnum;
	int fw[3];
	if (sscanf(buf, "SF%d V:%d.%d.%d ", &sfnum, &fw[0], &fw[1], &fw[2]) != 4 ||
	    sfnum != (int)dp_data->devicetype) {
		msg_perr("Unexpected firmware version string '%s'\n", buf);
		return 1;
	}
	/* Only these major versions were tested. */
	if (fw[0] < 2 || fw[0] > 7) {
		msg_perr("Unexpected firmware version %d.%d.%d!\n", fw[0], fw[1], fw[2]);
		return 1;
	}

	dp_data->firmwareversion = FIRMWARE_VERSION(fw[0], fw[1], fw[2]);
	if (protocol(dp_data) == PROTOCOL_UNKNOWN) {
		msg_perr("Internal error: Unable to determine protocol version.\n");
		return 1;
	}

	return 0;
}

/*
 * Read the id from the dediprog. This should return the numeric part of the
 * serial number found on a sticker on the back of the dediprog. Note this
 * number is stored in writable eeprom, so it could get out of sync. Also note,
 * this function only supports SF100 at this time, but SF600 support is not too
 * much different.
 * @return  the id on success, -1 on failure
 */
static int dediprog_read_id(libusb_device_handle *dediprog_handle)
{
	int ret;
	uint8_t buf[3];

	ret = libusb_control_transfer(dediprog_handle, REQTYPE_OTHER_IN,
				      0x7,    /* request */
				      0,      /* value */
				      0xEF00, /* index */
				      buf, sizeof(buf),
				      DEFAULT_TIMEOUT);
	if (ret != sizeof(buf)) {
		msg_perr("Failed to read dediprog id, error %d!\n", ret);
		return -1;
	}

	return buf[0] << 16 | buf[1] << 8 | buf[2];
}

/*
 * This command presumably sets the voltage for the SF100 itself (not the
 * SPI flash). Only use this command with firmware older than V6.0.0. Newer
 * (including all SF600s) do not support it.
 */

/* This command presumably sets the voltage for the SF100 itself (not the SPI flash).
 * Only use dediprog_set_voltage on SF100 programmers with firmware older
 * than V6.0.0. Newer programmers (including all SF600s) do not support it. */
static int dediprog_set_voltage(libusb_device_handle *dediprog_handle)
{
	unsigned char buf[1] = {0};
	int ret = libusb_control_transfer(dediprog_handle, REQTYPE_OTHER_IN, CMD_SET_VOLTAGE, 0x0, 0x0,
			      buf, 0x1, DEFAULT_TIMEOUT);
	if (ret < 0) {
		msg_perr("Command Set Voltage failed (%s)!\n", libusb_error_name(ret));
		return 1;
	}
	if ((ret != 1) || (buf[0] != 0x6f)) {
		msg_perr("Unexpected response to init!\n");
		return 1;
	}

	return 0;
}

static int dediprog_standalone_mode(const struct dediprog_data *dp_data)
{
	int ret;

	if (dp_data->devicetype != DEV_SF600)
		return 0;

	msg_pdbg2("Disabling standalone mode.\n");
	ret = dediprog_write(dp_data->handle, CMD_SET_STANDALONE, LEAVE_STANDALONE_MODE, 0, NULL, 0);
	if (ret) {
		msg_perr("Failed to disable standalone mode: %s\n", libusb_error_name(ret));
		return 1;
	}

	return 0;
}

#if 0
/* Something.
 * Present in eng_detect_blink.log with firmware 3.1.8
 * Always preceded by Command Receive Device String
 */
static int dediprog_command_b(libusb_device_handle *dediprog_handle)
{
	int ret;
	char buf[0x3];

	ret = usb_control_msg(dediprog_handle, REQTYPE_OTHER_IN, 0x7, 0x0, 0xef00,
			      buf, 0x3, DEFAULT_TIMEOUT);
	if (ret < 0) {
		msg_perr("Command B failed (%s)!\n", libusb_error_name(ret));
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

static int set_target_flash(libusb_device_handle *dediprog_handle, enum dediprog_target target)
{
	int ret = dediprog_write(dediprog_handle, CMD_SET_TARGET, target, 0, NULL, 0);
	if (ret != 0) {
		msg_perr("set_target_flash failed (%s)!\n", libusb_error_name(ret));
		return 1;
	}
	return 0;
}

#if 0
/* Returns true if the button is currently pressed. */
static bool dediprog_get_button(libusb_device_handle *dediprog_handle)
{
	char buf[1];
	int ret = usb_control_msg(dediprog_handle, REQTYPE_EP_IN, CMD_GET_BUTTON, 0, 0,
			      buf, 0x1, DEFAULT_TIMEOUT);
	if (ret != 0) {
		msg_perr("Could not get button state (%s)!\n", libusb_error_name(ret));
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

static int dediprog_shutdown(void *data)
{
	int ret = 0;
	struct dediprog_data *dp_data = data;

	/* URB 28. Command Set SPI Voltage to 0. */
	if (dediprog_set_spi_voltage(dp_data->handle, 0x0)) {
		ret = 1;
		goto out;
	}

	if (libusb_release_interface(dp_data->handle, 0)) {
		msg_perr("Could not release USB interface!\n");
		ret = 1;
		goto out;
	}
	libusb_close(dp_data->handle);
	libusb_exit(dp_data->usb_ctx);
out:
	free(data);
	return ret;
}

static struct spi_master spi_master_dediprog = {
	.features	= SPI_MASTER_NO_4BA_MODES,
	.max_data_read	= 16, /* 18 seems to work fine as well, but 19 times out sometimes with FW 5.15. */
	.max_data_write	= 16,
	.command	= dediprog_spi_send_command,
	.read		= dediprog_spi_read,
	.write_256	= dediprog_spi_write_256,
	.write_aai	= dediprog_spi_write_aai,
	.shutdown	= dediprog_shutdown,
};

/*
 * Open a dediprog_handle with the USB device at the given index.
 * @index   index of the USB device
 * @return  0 for success, -1 for error, -2 for busy device
 */
static int dediprog_open(int index, struct dediprog_data *dp_data)
{
	const uint16_t vid = devs_dediprog[0].vendor_id;
	const uint16_t pid = devs_dediprog[0].device_id;
	int ret;

	dp_data->handle = usb_dev_get_by_vid_pid_number(dp_data->usb_ctx, vid, pid, (unsigned int) index);
	if (!dp_data->handle) {
		msg_perr("Could not find a Dediprog programmer on USB.\n");
		libusb_exit(dp_data->usb_ctx);
		return -1;
	}
	ret = libusb_set_configuration(dp_data->handle, 1);
	if (ret != 0) {
		msg_perr("Could not set USB device configuration: %i %s\n",
			 ret, libusb_error_name(ret));
		libusb_close(dp_data->handle);
		return -2;
	}
	ret = libusb_claim_interface(dp_data->handle, 0);
	if (ret < 0) {
		msg_perr("Could not claim USB device interface %i: %i %s\n",
			 0, ret, libusb_error_name(ret));
		libusb_close(dp_data->handle);
		return -2;
	}
	return 0;
}

static int dediprog_init(const struct programmer_cfg *cfg)
{
	char *param_str;
	int spispeed_idx = 1;
	int millivolt = 3500;
	int id = -1; /* -1 defaults to enumeration order */
	int found_id;
	long usedevice = 0;
	long target = FLASH_TYPE_APPLICATION_FLASH_1;
	int i, ret;

	param_str = extract_programmer_param_str(cfg, "spispeed");
	if (param_str) {
		for (i = 0; spispeeds[i].name; ++i) {
			if (!strcasecmp(spispeeds[i].name, param_str)) {
				spispeed_idx = i;
				break;
			}
		}
		if (!spispeeds[i].name) {
			msg_perr("Error: Invalid spispeed value: '%s'.\n", param_str);
			free(param_str);
			return 1;
		}
		free(param_str);
	}

	param_str = extract_programmer_param_str(cfg, "voltage");
	if (param_str) {
		millivolt = parse_voltage(param_str);
		free(param_str);
		if (millivolt < 0)
			return 1;
		msg_pinfo("Setting voltage to %i mV\n", millivolt);
	}

	param_str = extract_programmer_param_str(cfg, "id");
	if (param_str) {
		char prefix0, prefix1;
		if (sscanf(param_str, "%c%c%d", &prefix0, &prefix1, &id) != 3) {
			msg_perr("Error: Could not parse dediprog 'id'.\n");
			msg_perr("Expected a string like SF012345 or DP012345.\n");
			free(param_str);
			return 1;
		}
		if (id < 0 || id >= 0x1000000) {
			msg_perr("Error: id %s is out of range!\n", param_str);
			free(param_str);
			return 1;
		}
		if (!(prefix0 == 'S' && prefix1 == 'F') && !(prefix0 == 'D' && prefix1 == 'P')) {
			msg_perr("Error: %s is an invalid id!\n", param_str);
			free(param_str);
			return 1;
		}
		msg_pinfo("Will search for dediprog id %s.\n", param_str);
	}
	free(param_str);

	param_str = extract_programmer_param_str(cfg, "device");
	if (param_str) {
		char *dev_suffix;
		if (id != -1) {
			msg_perr("Error: Cannot use 'id' and 'device'.\n");
		}
		errno = 0;
		usedevice = strtol(param_str, &dev_suffix, 10);
		if (errno != 0 || param_str == dev_suffix) {
			msg_perr("Error: Could not convert 'device'.\n");
			free(param_str);
			return 1;
		}
		if (usedevice < 0 || usedevice > INT_MAX) {
			msg_perr("Error: Value for 'device' is out of range.\n");
			free(param_str);
			return 1;
		}
		if (strlen(dev_suffix) > 0) {
			msg_perr("Error: Garbage following 'device' value.\n");
			free(param_str);
			return 1;
		}
		msg_pinfo("Using device %li.\n", usedevice);
	}
	free(param_str);

	param_str = extract_programmer_param_str(cfg, "target");
	if (param_str) {
		char *target_suffix;
		errno = 0;
		target = strtol(param_str, &target_suffix, 10);
		if (errno != 0 || param_str == target_suffix) {
			msg_perr("Error: Could not convert 'target'.\n");
			free(param_str);
			return 1;
		}
		if (target < 1 || target > 2) {
			msg_perr("Error: Value for 'target' is out of range.\n");
			free(param_str);
			return 1;
		}
		if (strlen(target_suffix) > 0) {
			msg_perr("Error: Garbage following 'target' value.\n");
			free(param_str);
			return 1;
		}
		switch (target) {
		case 1:
			msg_pinfo("Using target %s.\n", "FLASH_TYPE_APPLICATION_FLASH_1");
			target = FLASH_TYPE_APPLICATION_FLASH_1;
			break;
		case 2:
			msg_pinfo("Using target %s.\n", "FLASH_TYPE_APPLICATION_FLASH_2");
			target = FLASH_TYPE_APPLICATION_FLASH_2;
			break;
		default:
			break;
		}
	}
	free(param_str);

	struct dediprog_data *dp_data = calloc(1, sizeof(*dp_data));
	if (!dp_data) {
		msg_perr("Unable to allocate space for SPI master data\n");
		return 1;
	}
	dp_data->firmwareversion = FIRMWARE_VERSION(0, 0, 0);
	dp_data->devicetype = DEV_UNKNOWN;

	/* Here comes the USB stuff. */
	ret = libusb_init(&dp_data->usb_ctx);
	if (ret) {
		msg_perr("Could not initialize libusb!\n");
		goto init_err_exit;
	}

	if (id != -1) {
		for (i = 0; ; i++) {
			ret = dediprog_open(i, dp_data);
			if (ret == -1) {
				/* no dev */
				goto init_err_exit;
			} else if (ret == -2) {
				/* busy dev */
				continue;
			}

			/* Notice we can only call dediprog_read_id() after
			 * libusb_set_configuration() and
			 * libusb_claim_interface(). When searching by id and
			 * either configuration or claim fails (usually the
			 * device is in use by another instance of flashrom),
			 * the device is skipped and the next device is tried.
			 */
			found_id = dediprog_read_id(dp_data->handle);
			if (found_id < 0) {
				msg_perr("Could not read id.\n");
				libusb_release_interface(dp_data->handle, 0);
				libusb_close(dp_data->handle);
				continue;
			}
			msg_pinfo("Found dediprog id SF%06d.\n", found_id);
			if (found_id != id) {
				libusb_release_interface(dp_data->handle, 0);
				libusb_close(dp_data->handle);
				continue;
			}
			break;
		}
	} else {
		if (dediprog_open(usedevice, dp_data)) {
			goto init_err_exit;
		}
		found_id = dediprog_read_id(dp_data->handle);
	}

	if (found_id >= 0) {
		msg_pinfo("Using dediprog id SF%06d.\n", found_id);
	}

	/* Try reading the devicestring. If that fails and the device is old (FW < 6.0.0, which we can not know)
	 * then we need to try the "set voltage" command and then attempt to read the devicestring again. */
	if (dediprog_check_devicestring(dp_data)) {
		if (dediprog_set_voltage(dp_data->handle))
			goto init_err_cleanup_exit;
		if (dediprog_check_devicestring(dp_data))
			goto init_err_cleanup_exit;
	}

	/* SF100/SF200 uses one in/out endpoint, SF600 uses separate in/out endpoints */
	dp_data->in_endpoint = 2;
	switch (dp_data->devicetype) {
	case DEV_SF100:
	case DEV_SF200:
		dp_data->out_endpoint = 2;
		break;
	default:
		dp_data->out_endpoint = 1;
		break;
	}

	/* Set all possible LEDs as soon as possible to indicate activity.
	 * Because knowing the firmware version is required to set the LEDs correctly we need to this after
	 * dediprog_check_devicestring() has queried the device. */
	dediprog_set_leds(LED_ALL, dp_data);

	/* Select target/socket, frequency and VCC. */
	if (set_target_flash(dp_data->handle, target) ||
	    dediprog_set_spi_speed(spispeed_idx, dp_data) ||
	    dediprog_set_spi_voltage(dp_data->handle, millivolt)) {
		dediprog_set_leds(LED_ERROR, dp_data);
		goto init_err_cleanup_exit;
	}

	if (dediprog_standalone_mode(dp_data))
		goto init_err_cleanup_exit;

	if ((dp_data->devicetype == DEV_SF100) ||
	    (dp_data->devicetype == DEV_SF600 && protocol(dp_data) == PROTOCOL_V3))
		spi_master_dediprog.features &= ~SPI_MASTER_NO_4BA_MODES;

	if (protocol(dp_data) >= PROTOCOL_V2)
		spi_master_dediprog.features |= SPI_MASTER_4BA;

	if (dediprog_set_leds(LED_NONE, dp_data))
		goto init_err_cleanup_exit;

	return register_spi_master(&spi_master_dediprog, dp_data);

init_err_cleanup_exit:
	dediprog_shutdown(dp_data);
	return 1;

init_err_exit:
	free(dp_data);
	return 1;
}

const struct programmer_entry programmer_dediprog = {
	.name			= "dediprog",
	.type			= USB,
	.devs.dev		= devs_dediprog,
	.init			= dediprog_init,
};
