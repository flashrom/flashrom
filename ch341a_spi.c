/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2011 asbokid <ballymunboy@gmail.com>
 * Copyright (C) 2014 Pluto Yang <yangyj.ee@gmail.com>
 * Copyright (C) 2015-2016 Stefan Tauner
 * Copyright (C) 2015 Urja Rannikko <urjaman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdlib.h>
#include <string.h>
#include <libusb.h>
#include "flash.h"
#include "programmer.h"

/* LIBUSB_CALL ensures the right calling conventions on libusb callbacks.
 * However, the macro is not defined everywhere. m(
 */
#ifndef LIBUSB_CALL
#define LIBUSB_CALL
#endif

#define	 USB_TIMEOUT		1000	/* 1000 ms is plenty and we have no backup strategy anyway. */
#define	 WRITE_EP		0x02
#define	 READ_EP		0x82

#define	 CH341_PACKET_LENGTH		0x20
#define	 CH341_MAX_PACKETS		256
#define	 CH341_MAX_PACKET_LEN		(CH341_PACKET_LENGTH * CH341_MAX_PACKETS)

#define	 CH341A_CMD_SET_OUTPUT		0xA1
#define	 CH341A_CMD_IO_ADDR		0xA2
#define	 CH341A_CMD_PRINT_OUT		0xA3
#define	 CH341A_CMD_SPI_STREAM		0xA8
#define	 CH341A_CMD_SIO_STREAM		0xA9
#define	 CH341A_CMD_I2C_STREAM		0xAA
#define	 CH341A_CMD_UIO_STREAM		0xAB

#define	 CH341A_CMD_I2C_STM_START	0x74
#define	 CH341A_CMD_I2C_STM_STOP	0x75
#define	 CH341A_CMD_I2C_STM_OUT		0x80
#define	 CH341A_CMD_I2C_STM_IN		0xC0
#define	 CH341A_CMD_I2C_STM_MAX		( min( 0x3F, CH341_PACKET_LENGTH ) )
#define	 CH341A_CMD_I2C_STM_SET		0x60 // bit 2: SPI with two data pairs D5,D4=out, D7,D6=in
#define	 CH341A_CMD_I2C_STM_US		0x40
#define	 CH341A_CMD_I2C_STM_MS		0x50
#define	 CH341A_CMD_I2C_STM_DLY		0x0F
#define	 CH341A_CMD_I2C_STM_END		0x00

#define	 CH341A_CMD_UIO_STM_IN		0x00
#define	 CH341A_CMD_UIO_STM_DIR		0x40
#define	 CH341A_CMD_UIO_STM_OUT		0x80
#define	 CH341A_CMD_UIO_STM_US		0xC0
#define	 CH341A_CMD_UIO_STM_END		0x20

#define	 CH341A_STM_I2C_20K		0x00
#define	 CH341A_STM_I2C_100K		0x01
#define	 CH341A_STM_I2C_400K		0x02
#define	 CH341A_STM_I2C_750K		0x03
#define	 CH341A_STM_SPI_DBL		0x04


/* Number of parallel IN transfers. 32 seems to produce the most stable throughput on Windows. */
#define USB_IN_TRANSFERS 32

struct ch341a_spi_data {
	struct libusb_device_handle *handle;

	/* We need to use many queued IN transfers for any resemblance of performance (especially on Windows)
	 * because USB spec says that transfers end on non-full packets and the device sends the 31 reply
	 * data bytes to each 32-byte packet with command + 31 bytes of data... */
	struct libusb_transfer *transfer_out;
	struct libusb_transfer *transfer_ins[USB_IN_TRANSFERS];

	/* Accumulate delays to be plucked between CS deassertion and CS assertions. */
	unsigned int stored_delay_us;
};

static const struct dev_entry devs_ch341a_spi[] = {
	{0x1A86, 0x5512, OK, "Winchiphead (WCH)", "CH341A"},

	{0},
};

enum trans_state {TRANS_ACTIVE = -2, TRANS_ERR = -1, TRANS_IDLE = 0};

static void print_hex(const void *buf, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++) {
		msg_pspew(" %02x", ((uint8_t *)buf)[i]);
		if (i % CH341_PACKET_LENGTH == CH341_PACKET_LENGTH - 1)
			msg_pspew("\n");
	}
}

static void cb_common(const char *func, struct libusb_transfer *transfer)
{
	int *transfer_cnt = (int*)transfer->user_data;

	if (transfer->status == LIBUSB_TRANSFER_CANCELLED) {
		/* Silently ACK and exit. */
		*transfer_cnt = TRANS_IDLE;
		return;
	}

	if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
		msg_perr("\n%s: error: %s\n", func, libusb_error_name(transfer->status));
		*transfer_cnt = TRANS_ERR;
	} else {
		*transfer_cnt = transfer->actual_length;
	}
}

/* callback for bulk out async transfer */
static void LIBUSB_CALL cb_out(struct libusb_transfer *transfer)
{
	cb_common(__func__, transfer);
}

/* callback for bulk in async transfer */
static void LIBUSB_CALL cb_in(struct libusb_transfer *transfer)
{
	cb_common(__func__, transfer);
}

static int32_t usb_transfer(const struct ch341a_spi_data *data, const char *func,
			    unsigned int writecnt, unsigned int readcnt, const uint8_t *writearr, uint8_t *readarr)
{
	int state_out = TRANS_IDLE;
	data->transfer_out->buffer = (uint8_t*)writearr;
	data->transfer_out->length = writecnt;
	data->transfer_out->user_data = &state_out;

	/* Schedule write first */
	if (writecnt > 0) {
		state_out = TRANS_ACTIVE;
		int ret = libusb_submit_transfer(data->transfer_out);
		if (ret) {
			msg_perr("%s: failed to submit OUT transfer: %s\n", func, libusb_error_name(ret));
			state_out = TRANS_ERR;
			goto err;
		}
	}

	/* Handle all asynchronous packets as long as we have stuff to write or read. The write(s) simply need
	 * to complete but we need to scheduling reads as long as we are not done. */
	unsigned int free_idx = 0; /* The IN transfer we expect to be free next. */
	unsigned int in_idx = 0; /* The IN transfer we expect to be completed next. */
	unsigned int in_done = 0;
	unsigned int in_active = 0;
	unsigned int out_done = 0;
	uint8_t *in_buf = readarr;
	int state_in[USB_IN_TRANSFERS] = {0};
	do {
		/* Schedule new reads as long as there are free transfers and unscheduled bytes to read. */
		while ((in_done + in_active) < readcnt && state_in[free_idx] == TRANS_IDLE) {
			unsigned int cur_todo = min(CH341_PACKET_LENGTH - 1, readcnt - in_done - in_active);
			data->transfer_ins[free_idx]->length = cur_todo;
			data->transfer_ins[free_idx]->buffer = in_buf;
			data->transfer_ins[free_idx]->user_data = &state_in[free_idx];
			int ret = libusb_submit_transfer(data->transfer_ins[free_idx]);
			if (ret) {
				state_in[free_idx] = TRANS_ERR;
				msg_perr("%s: failed to submit IN transfer: %s\n",
					 func, libusb_error_name(ret));
				goto err;
			}
			in_buf += cur_todo;
			in_active += cur_todo;
			state_in[free_idx] = TRANS_ACTIVE;
			free_idx = (free_idx + 1) % USB_IN_TRANSFERS; /* Increment (and wrap around). */
		}

		/* Actually get some work done. */
		libusb_handle_events_timeout(NULL, &(struct timeval){1, 0});

		/* Check for the write */
		if (out_done < writecnt) {
			if (state_out == TRANS_ERR) {
				goto err;
			} else if (state_out > 0) {
				out_done += state_out;
				state_out = TRANS_IDLE;
			}
		}
		/* Check for completed transfers. */
		while (state_in[in_idx] != TRANS_IDLE && state_in[in_idx] != TRANS_ACTIVE) {
			if (state_in[in_idx] == TRANS_ERR) {
				goto err;
			}
			/* If a transfer is done, record the number of bytes read and reuse it later. */
			in_done += state_in[in_idx];
			in_active -= state_in[in_idx];
			state_in[in_idx] = TRANS_IDLE;
			in_idx = (in_idx + 1) % USB_IN_TRANSFERS; /* Increment (and wrap around). */
		}
	} while ((out_done < writecnt) || (in_done < readcnt));

	if (out_done > 0) {
		msg_pspew("Wrote %d bytes:\n", out_done);
		print_hex(writearr, out_done);
		msg_pspew("\n\n");
	}
	if (in_done > 0) {
		msg_pspew("Read %d bytes:\n", in_done);
		print_hex(readarr, in_done);
		msg_pspew("\n\n");
	}
	return 0;
err:
	/* Clean up on errors. */
	msg_perr("%s: Failed to %s %d bytes\n", func, (state_out == TRANS_ERR) ? "write" : "read",
		 (state_out == TRANS_ERR) ? writecnt : readcnt);
	/* First, we must cancel any ongoing requests and wait for them to be canceled. */
	if ((writecnt > 0) && (state_out == TRANS_ACTIVE)) {
		if (libusb_cancel_transfer(data->transfer_out) != 0)
			state_out = TRANS_ERR;
	}
	if (readcnt > 0) {
		unsigned int i;
		for (i = 0; i < USB_IN_TRANSFERS; i++) {
			if (state_in[i] == TRANS_ACTIVE)
				if (libusb_cancel_transfer(data->transfer_ins[i]) != 0)
					state_in[i] = TRANS_ERR;
		}
	}

	/* Wait for cancellations to complete. */
	while (1) {
		bool finished = true;
		if ((writecnt > 0) && (state_out == TRANS_ACTIVE))
			finished = false;
		if (readcnt > 0) {
			unsigned int i;
			for (i = 0; i < USB_IN_TRANSFERS; i++) {
				if (state_in[i] == TRANS_ACTIVE)
					finished = false;
			}
		}
		if (finished)
			break;
		libusb_handle_events_timeout(NULL, &(struct timeval){1, 0});
	}
	return -1;
}

/*   Set the I2C bus speed (speed(b1b0): 0 = 20kHz; 1 = 100kHz, 2 = 400kHz, 3 = 750kHz).
 *   Set the SPI bus data width (speed(b2): 0 = Single, 1 = Double).  */
static int32_t config_stream(const struct ch341a_spi_data *data, uint32_t speed)
{
	uint8_t buf[] = {
		CH341A_CMD_I2C_STREAM,
		CH341A_CMD_I2C_STM_SET | (speed & 0x7),
		CH341A_CMD_I2C_STM_END
	};

	int32_t ret = usb_transfer(data, __func__, sizeof(buf), 0, buf, NULL);
	if (ret < 0) {
		msg_perr("Could not configure stream interface.\n");
	}
	return ret;
}

/* The assumed map between UIO command bits, pins on CH341A chip and pins on SPI chip:
 * UIO	CH341A	SPI	CH341A SPI name
 * 0	D0/15	CS/1	(CS0)
 * 1	D1/16	unused	(CS1)
 * 2	D2/17	unused	(CS2)
 * 3	D3/18	SCK/6	(DCK)
 * 4	D4/19	unused	(DOUT2)
 * 5	D5/20	SI/5	(DOUT)
 * - The UIO stream commands seem to only have 6 bits of output, and D6/D7 are the SPI inputs,
 *  mapped as follows:
 *	D6/21	unused	(DIN2)
 *	D7/22	SO/2	(DIN)
 */
static int32_t enable_pins(const struct ch341a_spi_data *data, bool enable)
{
	uint8_t buf[] = {
		CH341A_CMD_UIO_STREAM,
		CH341A_CMD_UIO_STM_OUT | 0x37, // CS high (all of them), SCK=0, DOUT*=1
		CH341A_CMD_UIO_STM_DIR | (enable ? 0x3F : 0x00), // Interface output enable / disable
		CH341A_CMD_UIO_STM_END,
	};

	int32_t ret = usb_transfer(data, __func__, sizeof(buf), 0, buf, NULL);
	if (ret < 0) {
		msg_perr("Could not %sable output pins.\n", enable ? "en" : "dis");
	}
	return ret;
}

/* De-assert and assert CS in one operation. */
static void pluck_cs(uint8_t *ptr, unsigned int *stored_delay_us)
{
	/* This was measured to give a minimum deassertion time of 2.25 us,
	 * >20x more than needed for most SPI chips (100ns). */
	int delay_cnt = 2;
	if (*stored_delay_us) {
		delay_cnt = (*stored_delay_us * 4) / 3;
		*stored_delay_us = 0;
	}
	*ptr++ = CH341A_CMD_UIO_STREAM;
	*ptr++ = CH341A_CMD_UIO_STM_OUT | 0x37; /* deasserted */
	int i;
	for (i = 0; i < delay_cnt; i++)
		*ptr++ = CH341A_CMD_UIO_STM_OUT | 0x37; /* "delay" */
	*ptr++ = CH341A_CMD_UIO_STM_OUT | 0x36; /* asserted */
	*ptr++ = CH341A_CMD_UIO_STM_END;
}

static void ch341a_spi_delay(const struct flashctx *flash, unsigned int usecs)
{
	struct ch341a_spi_data *data = flash->mst->spi.data;

	/* There is space for 28 bytes instructions of 750 ns each in the CS packet (32 - 4 for the actual CS
	 * instructions), thus max 21 us, but we avoid getting too near to this boundary and use
	 * default_delay() for durations over 20 us. */
	if ((usecs + data->stored_delay_us) > 20) {
		unsigned int inc = 20 - data->stored_delay_us;
		default_delay(usecs - inc);
		usecs = inc;
	}
	data->stored_delay_us += usecs;
}

static int ch341a_spi_spi_send_command(const struct flashctx *flash, unsigned int writecnt, unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr)
{
	struct ch341a_spi_data *data = flash->mst->spi.data;

	/* How many packets ... */
	const size_t packets = (writecnt + readcnt + CH341_PACKET_LENGTH - 2) / (CH341_PACKET_LENGTH - 1);

	/* We pluck CS/timeout handling into the first packet thus we need to allocate one extra package. */
	uint8_t wbuf[packets+1][CH341_PACKET_LENGTH];
	uint8_t rbuf[writecnt + readcnt];
	/* Initialize the write buffer to zero to prevent writing random stack contents to device. */
	memset(wbuf[0], 0, CH341_PACKET_LENGTH);

	uint8_t *ptr = wbuf[0];
	/* CS usage is optimized by doing both transitions in one packet.
	 * Final transition to deselected state is in the pin disable. */
	pluck_cs(ptr, &data->stored_delay_us);
	unsigned int write_left = writecnt;
	unsigned int read_left = readcnt;
	unsigned int p;
	for (p = 0; p < packets; p++) {
		unsigned int write_now = min(CH341_PACKET_LENGTH - 1, write_left);
		unsigned int read_now = min ((CH341_PACKET_LENGTH - 1) - write_now, read_left);
		ptr = wbuf[p+1];
		*ptr++ = CH341A_CMD_SPI_STREAM;
		unsigned int i;
		for (i = 0; i < write_now; ++i)
			*ptr++ = reverse_byte(*writearr++);
		if (read_now) {
			memset(ptr, 0xFF, read_now);
			read_left -= read_now;
		}
		write_left -= write_now;
	}

	int32_t ret = usb_transfer(data, __func__, CH341_PACKET_LENGTH + packets + writecnt + readcnt,
				    writecnt + readcnt, wbuf[0], rbuf);
	if (ret < 0)
		return -1;

	unsigned int i;
	for (i = 0; i < readcnt; i++) {
		*readarr++ = reverse_byte(rbuf[writecnt + i]);
	}

	return 0;
}

static int ch341a_spi_shutdown(void *data)
{
	struct ch341a_spi_data *ch341a_data = data;

	enable_pins(ch341a_data, false);
	libusb_free_transfer(ch341a_data->transfer_out);
	int i;
	for (i = 0; i < USB_IN_TRANSFERS; i++)
		libusb_free_transfer(ch341a_data->transfer_ins[i]);
	libusb_release_interface(ch341a_data->handle, 0);
	libusb_attach_kernel_driver(ch341a_data->handle, 0);
	libusb_close(ch341a_data->handle);
	libusb_exit(NULL);

	free(data);
	return 0;
}

static const struct spi_master spi_master_ch341a_spi = {
	.features	= SPI_MASTER_4BA,
	/* flashrom's current maximum is 256 B. CH341A was tested on Linux and Windows to accept at least
	 * 128 kB. Basically there should be no hard limit because transfers are broken up into USB packets
	 * sent to the device and most of their payload streamed via SPI. */
	.max_data_read	= 4 * 1024,
	.max_data_write	= 4 * 1024,
	.command	= ch341a_spi_spi_send_command,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.shutdown	= ch341a_spi_shutdown,
	.delay		= ch341a_spi_delay,
};

static int ch341a_spi_init(const struct programmer_cfg *cfg)
{
	int32_t ret = libusb_init(NULL);
	if (ret < 0) {
		msg_perr("Couldn't initialize libusb!\n");
		return -1;
	}

	/* Enable information, warning, and error messages (only). */
#if LIBUSB_API_VERSION < 0x01000106
	libusb_set_debug(NULL, 3);
#else
	libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
#endif

	struct ch341a_spi_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Out of memory!\n");
		return 1;
	}

	uint16_t vid = devs_ch341a_spi[0].vendor_id;
	uint16_t pid = devs_ch341a_spi[0].device_id;
	data->handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (data->handle == NULL) {
		msg_perr("Couldn't open device %04x:%04x.\n", vid, pid);
		goto free_data;
	}

	ret = libusb_detach_kernel_driver(data->handle, 0);
	if (ret != 0 && ret != LIBUSB_ERROR_NOT_FOUND)
		msg_pwarn("Cannot detach the existing USB driver. Claiming the interface may fail. %s\n",
			libusb_error_name(ret));

	ret = libusb_claim_interface(data->handle, 0);
	if (ret != 0) {
		msg_perr("Failed to claim interface 0: '%s'\n", libusb_error_name(ret));
		goto close_handle;
	}

	struct libusb_device *dev;
	if (!(dev = libusb_get_device(data->handle))) {
		msg_perr("Failed to get device from device handle.\n");
		goto close_handle;
	}

	struct libusb_device_descriptor desc;
	ret = libusb_get_device_descriptor(dev, &desc);
	if (ret < 0) {
		msg_perr("Failed to get device descriptor: '%s'\n", libusb_error_name(ret));
		goto release_interface;
	}

	msg_pdbg("Device revision is %d.%01d.%01d\n",
		(desc.bcdDevice >> 8) & 0x00FF,
		(desc.bcdDevice >> 4) & 0x000F,
		(desc.bcdDevice >> 0) & 0x000F);

	/* Allocate and pre-fill transfer structures. */
	data->transfer_out = libusb_alloc_transfer(0);
	if (!data->transfer_out) {
		msg_perr("Failed to alloc libusb OUT transfer\n");
		goto release_interface;
	}
	int i;
	for (i = 0; i < USB_IN_TRANSFERS; i++) {
		data->transfer_ins[i] = libusb_alloc_transfer(0);
		if (data->transfer_ins[i] == NULL) {
			msg_perr("Failed to alloc libusb IN transfer %d\n", i);
			goto dealloc_transfers;
		}
	}
	/* We use these helpers but dont fill the actual buffer yet. */
	libusb_fill_bulk_transfer(data->transfer_out, data->handle, WRITE_EP, NULL, 0, cb_out, NULL, USB_TIMEOUT);
	for (i = 0; i < USB_IN_TRANSFERS; i++)
		libusb_fill_bulk_transfer(data->transfer_ins[i], data->handle, READ_EP, NULL, 0, cb_in, NULL, USB_TIMEOUT);

	if ((config_stream(data, CH341A_STM_I2C_100K) < 0) || (enable_pins(data, true) < 0))
		goto dealloc_transfers;

	return register_spi_master(&spi_master_ch341a_spi, data);

dealloc_transfers:
	for (i = 0; i < USB_IN_TRANSFERS; i++) {
		if (data->transfer_ins[i] == NULL)
			break;
		libusb_free_transfer(data->transfer_ins[i]);
	}
	libusb_free_transfer(data->transfer_out);
release_interface:
	libusb_release_interface(data->handle, 0);
close_handle:
	libusb_attach_kernel_driver(data->handle, 0);
	libusb_close(data->handle);
free_data:
	free(data);
	return -1;
}

const struct programmer_entry programmer_ch341a_spi = {
	.name			= "ch341a_spi",
	.type			= USB,
	.devs.dev		= devs_ch341a_spi,
	.init			= ch341a_spi_init,
};
