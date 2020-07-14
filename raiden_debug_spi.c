/*
 * This file is part of the flashrom project.
 *
 * Copyright 2014, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/*
 * This SPI flash programming interface is designed to talk to a Chromium OS
 * device over a Raiden USB connection.  The USB connection is routed to a
 * microcontroller running an image compiled from:
 *
 *     https://chromium.googlesource.com/chromiumos/platform/ec
 *
 * The protocol for the USB-SPI bridge is documented in the following file in
 * that respository:
 *
 *     chip/stm32/usb_spi.c
 *
 * Version 1:
 * SPI transactions of up to 62B in each direction with every command having
 * a response. The initial packet from host contains a 2B header indicating
 * write and read counts with an optional payload length equal to the write
 * count. The device will respond with a message that reports the 2B status
 * code and an optional payload response length equal to read count.
 *
 * Message Format:
 *
 * Command Packet:
 *     +------------------+-----------------+------------------------+
 *     | write count : 1B | read count : 1B | write payload : <= 62B |
 *     +------------------+-----------------+------------------------+
 *
 *     write count:   1 byte, zero based count of bytes to write
 *
 *     read count:    1 byte, zero based count of bytes to read
 *
 *     write payload: Up to 62 bytes of data to write to SPI, the total
 *                    length of all TX packets must match write count.
 *                    Due to data alignment constraints, this must be an
 *                    even number of bytes unless this is the final packet.
 *
 * Response Packet:
 *     +-------------+-----------------------+
 *     | status : 2B | read payload : <= 62B |
 *     +-------------+-----------------------+
 *
 *     status: 2 byte status
 *         0x0000: Success
 *         0x0001: SPI timeout
 *         0x0002: Busy, try again
 *             This can happen if someone else has acquired the shared memory
 *             buffer that the SPI driver uses as /dev/null
 *         0x0003: Write count invalid (V1 > 62B)
 *         0x0004: Read count invalid (V1 > 62B)
 *         0x0005: The SPI bridge is disabled.
 *         0x8000: Unknown error mask
 *             The bottom 15 bits will contain the bottom 15 bits from the EC
 *             error code.
 *
 *     read payload: Up to 62 bytes of data read from SPI, the total
 *                   length of all RX packets must match read count
 *                   unless an error status was returned. Due to data
 *                   alignment constraints, this must be a even number
 *                   of bytes unless this is the final packet.
 *
 * USB Error Codes:
 *
 * send_command return codes have the following format:
 *
 *     0x00000:         Status code success.
 *     0x00001-0x0FFFF: Error code returned by the USB SPI device.
 *     0x10001-0x1FFFF: The host has determined an error has occurred.
 *     0x20001-0x20063  Lower bits store the positive value representation
 *                      of the libusb_error enum. See the libusb documentation:
 *                      http://libusb.sourceforge.net/api-1.0/group__misc.html
 */

#include "programmer.h"
#include "spi.h"
#include "usb_device.h"

#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* FIXME: Add some programmer IDs here */
const struct dev_entry devs_raiden[] = {
	{0},
};

#define GOOGLE_VID                  (0x18D1)
#define GOOGLE_RAIDEN_SPI_SUBCLASS  (0x51)
#define GOOGLE_RAIDEN_SPI_PROTOCOL  (0x01)

enum usb_spi_error {
	USB_SPI_SUCCESS             = 0x0000,
	USB_SPI_TIMEOUT             = 0x0001,
	USB_SPI_BUSY                = 0x0002,
	USB_SPI_WRITE_COUNT_INVALID = 0x0003,
	USB_SPI_READ_COUNT_INVALID  = 0x0004,
	USB_SPI_DISABLED            = 0x0005,
	USB_SPI_UNKNOWN_ERROR       = 0x8000,
};

enum raiden_debug_spi_request {
	RAIDEN_DEBUG_SPI_REQ_ENABLE    = 0x0000,
	RAIDEN_DEBUG_SPI_REQ_DISABLE   = 0x0001,
	RAIDEN_DEBUG_SPI_REQ_ENABLE_AP = 0x0002,
	RAIDEN_DEBUG_SPI_REQ_ENABLE_EC = 0x0003,
};

#define PACKET_HEADER_SIZE      (2)
#define MAX_PACKET_SIZE         (64)
#define PAYLOAD_SIZE            (MAX_PACKET_SIZE - PACKET_HEADER_SIZE)

/*
 * Servo Micro has an error where it is capable of acknowledging USB packets
 * without loading it into the USB endpoint buffers or triggering interrupts.
 * See crbug.com/952494. Retry mechanisms have been implemented to recover
 * from these rare failures allowing the process to continue.
 */
#define WRITE_RETY_ATTEMPTS     (3)
#define READ_RETY_ATTEMPTS      (3)
#define RETY_INTERVAL_US        (100 * 1000)

/*
 * This timeout is so large because the Raiden SPI timeout is 800ms.
 */
#define TRANSFER_TIMEOUT_MS     (200 + 800)

struct raiden_debug_spi_data {
	struct usb_device *dev;
	uint8_t in_ep;
	uint8_t out_ep;
};

typedef struct {
	int8_t write_count;
	/* -1 Indicates readback all on halfduplex compliant devices. */
	int8_t read_count;
	uint8_t data[PAYLOAD_SIZE];
} __attribute__((packed)) usb_spi_command_t;

typedef struct {
	uint16_t status_code;
	uint8_t data[PAYLOAD_SIZE];
} __attribute__((packed)) usb_spi_response_t;

/*
 * This function will return true when an error code can potentially recover
 * if we attempt to write SPI data to the device or read from it. We know
 * that some conditions are not recoverable in the current state so allows us
 * to bypass the retry logic and terminate early.
 */
static bool retry_recovery(int error_code)
{
	if (error_code < 0x10000) {
		/*
		 * Handle error codes returned from the device. USB_SPI_TIMEOUT,
		 * USB_SPI_BUSY, and USB_SPI_WRITE_COUNT_INVALID have been observed
		 * during transfer errors to the device and can be recovered.
		 */
		if (USB_SPI_READ_COUNT_INVALID <= error_code &&
		    error_code <= USB_SPI_DISABLED) {
			return false;
		}
	} else if (usb_device_is_libusb_error(error_code)) {
		/* Handle error codes returned from libusb. */
		if (error_code == LIBUSB_ERROR(LIBUSB_ERROR_NO_DEVICE)) {
			return false;
		}
	}
	return true;
}

static const struct raiden_debug_spi_data *
	get_raiden_data_from_context(const struct flashctx *flash)
{
	return (const struct raiden_debug_spi_data *)flash->mst->spi.data;
}

static int write_command(const struct flashctx *flash,
		unsigned int write_count,
		unsigned int read_count,
		const unsigned char *write_buffer,
		unsigned char *read_buffer)
{

	int transferred;
	int ret;
	usb_spi_command_t command_packet;
	const struct raiden_debug_spi_data * ctx_data = get_raiden_data_from_context(flash);

	if (write_count > PAYLOAD_SIZE) {
		msg_perr("Raiden: Invalid write_count of %d\n", write_count);
		return SPI_INVALID_LENGTH;
	}

	if (read_count > PAYLOAD_SIZE) {
		msg_perr("Raiden: Invalid read_count of %d\n", read_count);
		return SPI_INVALID_LENGTH;
	}

	command_packet.write_count = write_count;
	command_packet.read_count = read_count;

	memcpy(command_packet.data, write_buffer, write_count);

	ret = LIBUSB(libusb_bulk_transfer(ctx_data->dev->handle,
				ctx_data->out_ep,
				(void*)&command_packet,
				write_count + PACKET_HEADER_SIZE,
				&transferred,
				TRANSFER_TIMEOUT_MS));
	if (ret != 0) {
		msg_perr("Raiden: OUT transfer failed\n"
		         "    write_count = %d\n"
		         "    read_count  = %d\n",
		         write_count, read_count);
		return ret;
	}

	if ((unsigned) transferred != write_count + PACKET_HEADER_SIZE) {
		msg_perr("Raiden: Write failure (wrote %d, expected %d)\n",
			 transferred, write_count + PACKET_HEADER_SIZE);
		return 0x10001;
	}

	return 0;
}

static int read_response(const struct flashctx *flash,
		unsigned int write_count,
		unsigned int read_count,
		const unsigned char *write_buffer,
		unsigned char *read_buffer)
{
	int transferred;
	int ret;
	usb_spi_response_t response_packet;
	const struct raiden_debug_spi_data * ctx_data = get_raiden_data_from_context(flash);

	ret = LIBUSB(libusb_bulk_transfer(ctx_data->dev->handle,
				ctx_data->in_ep,
				(void*)&response_packet,
				read_count + PACKET_HEADER_SIZE,
				&transferred,
				TRANSFER_TIMEOUT_MS));
	if (ret != 0) {
		msg_perr("Raiden: IN transfer failed\n"
		         "    write_count = %d\n"
		         "    read_count  = %d\n",
		         write_count, read_count);
		return ret;
	}

	if ((unsigned) transferred != read_count + PACKET_HEADER_SIZE) {
		msg_perr("Raiden: Read failure (read %d, expected %d)\n",
				transferred, read_count + PACKET_HEADER_SIZE);
		return 0x10002;
	}

	memcpy(read_buffer, response_packet.data, read_count);

	return response_packet.status_code;
}

static int send_command(const struct flashctx *flash,
		unsigned int write_count,
		unsigned int read_count,
		const unsigned char *write_buffer,
		unsigned char *read_buffer)
{
	int status = -1;

	for (int write_attempt = 0; write_attempt < WRITE_RETY_ATTEMPTS;
	         write_attempt++) {

		status = write_command(flash, write_count, read_count,
		                       write_buffer, read_buffer);

		if (status) {
			/* Write operation failed. */
			msg_perr("Raiden: Write command failed\n"
				"Write attempt = %d\n"
				"status = %d\n",
				write_attempt + 1, status);
			if (!retry_recovery(status)) {
				/* Reattempting will not result in a recovery. */
				return status;
			}
			programmer_delay(RETY_INTERVAL_US);
			continue;
		}
		for (int read_attempt = 0; read_attempt < READ_RETY_ATTEMPTS; read_attempt++) {

			status = read_response(flash, write_count, read_count,
					write_buffer, read_buffer);

			if (status != 0) {
				/* Read operation failed. */
				msg_perr("Raiden: Read response failed\n"
					"Write attempt = %d\n"
					"Read attempt = %d\n"
					"status = %d\n",
					write_attempt + 1, read_attempt + 1, status);
				if (!retry_recovery(status)) {
					/* Reattempting will not result in a recovery. */
					return status;
				}
				programmer_delay(RETY_INTERVAL_US);
			} else {
				/* We were successful at performing the SPI transfer. */
				return status;
			}
		}
	}
	return status;
}

/*
 * Unfortunately there doesn't seem to be a way to specify the maximum number
 * of bytes that your SPI device can read/write, these values are the maximum
 * data chunk size that flashrom will package up with an additional five bytes
 * of command for the flash device, resulting in a 62 byte packet, that we then
 * add two bytes to in either direction, making our way up to the 64 byte
 * maximum USB packet size for the device.
 *
 * The largest command that flashrom generates is the byte program command, so
 * we use that command header maximum size here.
 */
#define MAX_DATA_SIZE   (PAYLOAD_SIZE - JEDEC_BYTE_PROGRAM_OUTSIZE)

static struct spi_master spi_master_raiden_debug = {
	.features	= SPI_MASTER_4BA,
	.max_data_read	= MAX_DATA_SIZE,
	.max_data_write	= MAX_DATA_SIZE,
	.command	= send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

static int match_endpoint(struct libusb_endpoint_descriptor const *descriptor,
                          enum libusb_endpoint_direction direction)
{
	return (((descriptor->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) ==
		 direction) &&
		((descriptor->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) ==
		 LIBUSB_TRANSFER_TYPE_BULK));
}

static int find_endpoints(struct usb_device *dev, uint8_t *in_ep, uint8_t *out_ep)
{
	int i;
	int in_count  = 0;
	int out_count = 0;

	for (i = 0; i < dev->interface_descriptor->bNumEndpoints; i++) {
		struct libusb_endpoint_descriptor const  *endpoint =
			&dev->interface_descriptor->endpoint[i];

		if (match_endpoint(endpoint, LIBUSB_ENDPOINT_IN)) {
			in_count++;
			*in_ep = endpoint->bEndpointAddress;
		} else if (match_endpoint(endpoint, LIBUSB_ENDPOINT_OUT)) {
			out_count++;
			*out_ep = endpoint->bEndpointAddress;
		}
	}

	if (in_count != 1 || out_count != 1) {
		msg_perr("Raiden: Failed to find one IN and one OUT endpoint\n"
			 "        found %d IN and %d OUT endpoints\n",
			 in_count,
			 out_count);
		return 1;
	}

	msg_pdbg("Raiden: Found IN  endpoint = 0x%02x\n", *in_ep);
	msg_pdbg("Raiden: Found OUT endpoint = 0x%02x\n", *out_ep);

	return 0;
}

static int raiden_debug_spi_shutdown(void * data)
{
	struct raiden_debug_spi_data * ctx_data =
		(struct raiden_debug_spi_data *)data;

	int ret = LIBUSB(libusb_control_transfer(
				ctx_data->dev->handle,
				LIBUSB_ENDPOINT_OUT |
				LIBUSB_REQUEST_TYPE_VENDOR |
				LIBUSB_RECIPIENT_INTERFACE,
				RAIDEN_DEBUG_SPI_REQ_DISABLE,
				0,
				ctx_data->dev->interface_descriptor->bInterfaceNumber,
				NULL,
				0,
				TRANSFER_TIMEOUT_MS));
	if (ret != 0) {
		msg_perr("Raiden: Failed to disable SPI bridge\n");
		return ret;
	}

	usb_device_free(ctx_data->dev);
	libusb_exit(NULL);
	free(ctx_data);

	return 0;
}

static int get_target(void)
{
	int request_enable = RAIDEN_DEBUG_SPI_REQ_ENABLE;

	char *target_str = extract_programmer_param("target");
	if (target_str) {
		if (!strcasecmp(target_str, "ap"))
			request_enable = RAIDEN_DEBUG_SPI_REQ_ENABLE_AP;
		else if (!strcasecmp(target_str, "ec"))
			request_enable = RAIDEN_DEBUG_SPI_REQ_ENABLE_EC;
		else {
			msg_perr("Invalid target: %s\n", target_str);
			request_enable = -1;
		}
	}
	free(target_str);

	return request_enable;
}

static void free_dev_list(struct usb_device **dev_lst)
{
	struct usb_device *dev = *dev_lst;
	/* free devices we don't care about */
	dev = dev->next;
	while (dev)
		dev = usb_device_free(dev);
}

int raiden_debug_spi_init(void)
{
	struct usb_match match;
	char *serial = extract_programmer_param("serial");
	struct usb_device *current;
	struct usb_device *device = NULL;
	int found = 0;
	int ret;

	int request_enable = get_target();
	if (request_enable < 0) {
		free(serial);
		return 1;
	}

	usb_match_init(&match);

	usb_match_value_default(&match.vid,      GOOGLE_VID);
	usb_match_value_default(&match.class,    LIBUSB_CLASS_VENDOR_SPEC);
	usb_match_value_default(&match.subclass, GOOGLE_RAIDEN_SPI_SUBCLASS);
	usb_match_value_default(&match.protocol, GOOGLE_RAIDEN_SPI_PROTOCOL);

	ret = LIBUSB(libusb_init(NULL));
	if (ret != 0) {
		msg_perr("Raiden: libusb_init failed\n");
		free(serial);
		return ret;
	}

	ret = usb_device_find(&match, &current);
	if (ret != 0) {
		msg_perr("Raiden: Failed to find devices\n");
		free(serial);
		return ret;
	}

	uint8_t in_endpoint  = 0;
	uint8_t out_endpoint = 0;
	while (current) {
		device = current;

		if (find_endpoints(device, &in_endpoint, &out_endpoint)) {
			msg_pdbg("Raiden: Failed to find valid endpoints on device");
			usb_device_show(" ", current);
			goto loop_end;
		}

		if (usb_device_claim(device)) {
			msg_pdbg("Raiden: Failed to claim USB device");
			usb_device_show(" ", current);
			goto loop_end;
		}

		if (!serial) {
			found = 1;
			goto loop_end;
		} else {
			unsigned char dev_serial[32];
			struct libusb_device_descriptor descriptor;
			int rc;

			memset(dev_serial, 0, sizeof(dev_serial));

			if (libusb_get_device_descriptor(device->device, &descriptor)) {
				msg_pdbg("USB: Failed to get device descriptor.\n");
				goto loop_end;
			}

			rc = libusb_get_string_descriptor_ascii(device->handle,
					descriptor.iSerialNumber,
					dev_serial,
					sizeof(dev_serial));
			if (rc < 0) {
				LIBUSB(rc);
			} else {
				if (strcmp(serial, (char *)dev_serial)) {
					msg_pdbg("Raiden: Serial number %s did not match device", serial);
					usb_device_show(" ", current);
				} else {
					msg_pinfo("Raiden: Serial number %s matched device", serial);
					usb_device_show(" ", current);
					found = 1;
				}
			}
		}

loop_end:
		if (found)
			break;
		else
			current = usb_device_free(current);
	}

	if (!device || !found) {
		msg_perr("Raiden: No usable device found.\n");
		free(serial);
		return 1;
	}

	free_dev_list(&current);

	ret = LIBUSB(libusb_control_transfer(
				device->handle,
				LIBUSB_ENDPOINT_OUT |
				LIBUSB_REQUEST_TYPE_VENDOR |
				LIBUSB_RECIPIENT_INTERFACE,
				request_enable,
				0,
				device->interface_descriptor->bInterfaceNumber,
				NULL,
				0,
				TRANSFER_TIMEOUT_MS));
	if (ret != 0) {
		msg_perr("Raiden: Failed to enable SPI bridge\n");
		return ret;
	}

	/*
	 * Allow for power to settle on the AP and EC flash devices.
	 * Load switches can have a 1-3 ms turn on time, and SPI flash devices
	 * can require up to 10 ms from power on to the first write.
	 */
	if ((request_enable == RAIDEN_DEBUG_SPI_REQ_ENABLE_AP) ||
		(request_enable == RAIDEN_DEBUG_SPI_REQ_ENABLE_EC))
		usleep(50 * 1000);

	struct raiden_debug_spi_data *data = calloc(1, sizeof(struct raiden_debug_spi_data));
	if (!data) {
		msg_perr("Unable to allocate space for extra SPI master data.\n");
		return SPI_GENERIC_ERROR;
	}

	data->dev = device;
	data->in_ep = in_endpoint;
	data->out_ep = out_endpoint;

	spi_master_raiden_debug.data = data;

	register_spi_master(&spi_master_raiden_debug);
	register_shutdown(raiden_debug_spi_shutdown, data);

	return 0;
}
