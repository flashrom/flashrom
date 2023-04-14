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
 * The protocol for the USB-SPI bridge is implemented in the following files
 * in that repository:
 *
 *     chip/stm32/usb_spi.h
 *     chip/stm32/usb_spi.c
 *
 * bInterfaceProtocol determines which protocol is used by the USB SPI device.
 *
 *
 * USB SPI Version 1:
 *
 *     SPI transactions of up to 62B in each direction with every command having
 *     a response. The initial packet from host contains a 2B header indicating
 *     write and read counts with an optional payload length equal to the write
 *     count. The device will respond with a message that reports the 2B status
 *     code and an optional payload response length equal to read count.
 *
 *
 * Message Packets:
 *
 * Command First Packet (Host to Device):
 *
 *      USB SPI command, containing the number of bytes to write and read
 *      and a payload of bytes to write.
 *
 *     +------------------+-----------------+------------------------+
 *     | write count : 1B | read count : 1B | write payload : <= 62B |
 *     +------------------+-----------------+------------------------+
 *
 *     write count:   1 byte, zero based count of bytes to write
 *
 *     read count:    1 byte, zero based count of bytes to read. Full duplex
 *                    mode is enabled with UINT8_MAX
 *
 *     write payload: Up to 62 bytes of data to write to SPI, the total
 *                    length of all TX packets must match write count.
 *                    Due to data alignment constraints, this must be an
 *                    even number of bytes unless this is the final packet.
 *
 * Response Packet (Device to Host):
 *
 *      USB SPI response, containing the status code and any bytes of the
 *      read payload.
 *
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
 *         0x0003: Write count invalid (over 62 bytes)
 *         0x0004: Read count invalid (over 62 bytes)
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
 *
 * USB SPI Version 2:
 *
 *     USB SPI version 2 adds support for larger SPI transfers and reduces the
 *     number of USB packets transferred. This improves performance when
 *     writing or reading large chunks of memory from a device. A packet ID
 *     field is used to distinguish the different packet types. Additional
 *     packets have been included to query the device for its configuration
 *     allowing the interface to be used on platforms with different SPI
 *     limitations. It includes validation and a packet to recover from the
 *     situations where USB packets are lost.
 *
 *     The USB SPI hosts which support packet version 2 are backwards compatible
 *     and use the bInterfaceProtocol field to identify which type of target
 *     they are connected to.
 *
 *
 * Example: USB SPI request with 128 byte write and 0 byte read.
 *
 *      Packet #1 Host to Device:
 *           packet id   = USB_SPI_PKT_ID_CMD_TRANSFER_START
 *           write count = 128
 *           read count  = 0
 *           payload     = First 58 bytes from the write buffer,
 *                            starting at byte 0 in the buffer
 *           packet size = 64 bytes
 *
 *      Packet #2 Host to Device:
 *           packet id   = USB_SPI_PKT_ID_CMD_TRANSFER_CONTINUE
 *           data index  = 58
 *           payload     = Next 60 bytes from the write buffer,
 *                           starting at byte 58 in the buffer
 *           packet size = 64 bytes
 *
 *      Packet #3 Host to Device:
 *           packet id   = USB_SPI_PKT_ID_CMD_TRANSFER_CONTINUE
 *           data index  = 118
 *           payload     = Next 10 bytes from the write buffer,
 *                           starting at byte 118 in the buffer
 *           packet size = 14 bytes
 *
 *      Packet #4 Device to Host:
 *           packet id   = USB_SPI_PKT_ID_RSP_TRANSFER_START
 *           status code = status code from device
 *           payload     = 0 bytes
 *           packet size = 4 bytes
 *
 * Example: USB SPI request with 2 byte write and 100 byte read.
 *
 *      Packet #1 Host to Device:
 *           packet id   = USB_SPI_PKT_ID_CMD_TRANSFER_START
 *           write count = 2
 *           read count  = 100
 *           payload     = The 2 byte write buffer
 *           packet size = 8 bytes
 *
 *      Packet #2 Device to Host:
 *           packet id   = USB_SPI_PKT_ID_RSP_TRANSFER_START
 *           status code = status code from device
 *           payload     = First 60 bytes from the read buffer,
 *                            starting at byte 0 in the buffer
 *           packet size = 64 bytes
 *
 *      Packet #3 Device to Host:
 *           packet id   = USB_SPI_PKT_ID_RSP_TRANSFER_CONTINUE
 *           data index  = 60
 *           payload     = Next 40 bytes from the read buffer,
 *                           starting at byte 60 in the buffer
 *           packet size = 44 bytes
 *
 *
 * Message Packets:
 *
 * Command Start Packet (Host to Device):
 *
 *      Start of the USB SPI command, contains the number of bytes to write
 *      and read on SPI and up to the first 58 bytes of write payload.
 *      Longer writes will use the continue packets with packet id
 *      USB_SPI_PKT_ID_CMD_TRANSFER_CONTINUE to transmit the remaining data.
 *
 *     +----------------+------------------+-----------------+---------------+
 *     | packet id : 2B | write count : 2B | read count : 2B | w.p. : <= 58B |
 *     +----------------+------------------+-----------------+---------------+
 *
 *     packet id:     2 byte enum defined by packet_id_type
 *                    Valid values packet id = USB_SPI_PKT_ID_CMD_TRANSFER_START
 *
 *     write count:   2 byte, zero based count of bytes to write
 *
 *     read count:    2 byte, zero based count of bytes to read
 *                    UINT16_MAX indicates full duplex mode with a read count
 *                    equal to the write count.
 *
 *     write payload: Up to 58 bytes of data to write to SPI, the total
 *                    length of all TX packets must match write count.
 *                    Due to data alignment constraints, this must be an
 *                    even number of bytes unless this is the final packet.
 *
 *
 * Response Start Packet (Device to Host):
 *
 *      Start of the USB SPI response, contains the status code and up to
 *      the first 60 bytes of read payload. Longer reads will use the
 *      continue packets with packet id USB_SPI_PKT_ID_RSP_TRANSFER_CONTINUE
 *      to transmit the remaining data.
 *
 *     +----------------+------------------+-----------------------+
 *     | packet id : 2B | status code : 2B | read payload : <= 60B |
 *     +----------------+------------------+-----------------------+
 *
 *     packet id:     2 byte enum defined by packet_id_type
 *                    Valid values packet id = USB_SPI_PKT_ID_RSP_TRANSFER_START
 *
 *     status code: 2 byte status code
 *         0x0000: Success
 *         0x0001: SPI timeout
 *         0x0002: Busy, try again
 *             This can happen if someone else has acquired the shared memory
 *             buffer that the SPI driver uses as /dev/null
 *         0x0003: Write count invalid. The byte limit is platform specific
 *             and is set during the configure USB SPI response.
 *         0x0004: Read count invalid. The byte limit is platform specific
 *             and is set during the configure USB SPI response.
 *         0x0005: The SPI bridge is disabled.
 *         0x0006: The RX continue packet's data index is invalid. This
 *             can indicate a USB transfer failure to the device.
 *         0x0007: The RX endpoint has received more data than write count.
 *             This can indicate a USB transfer failure to the device.
 *         0x0008: An unexpected packet arrived that the device could not
 *             process.
 *         0x0009: The device does not support full duplex mode.
 *         0x8000: Unknown error mask
 *             The bottom 15 bits will contain the bottom 15 bits from the EC
 *             error code.
 *
 *     read payload: Up to 60 bytes of data read from SPI, the total
 *                   length of all RX packets must match read count
 *                   unless an error status was returned. Due to data
 *                   alignment constraints, this must be a even number
 *                   of bytes unless this is the final packet.
 *
 *
 * Continue Packet (Bidirectional):
 *
 *      Continuation packet for the writes and read buffers. Both packets
 *      follow the same format, a data index counts the number of bytes
 *      previously transferred in the USB SPI transfer and a payload of bytes.
 *
 *     +----------------+-----------------+-------------------------------+
 *     | packet id : 2B | data index : 2B | write / read payload : <= 60B |
 *     +----------------+-----------------+-------------------------------+
 *
 *     packet id:     2 byte enum defined by packet_id_type
 *                    The packet id has 2 values depending on direction:
 *                    packet id = USB_SPI_PKT_ID_CMD_TRANSFER_CONTINUE
 *                    indicates the packet is being transmitted from the host
 *                    to the device and contains SPI write payload.
 *                    packet id = USB_SPI_PKT_ID_RSP_TRANSFER_CONTINUE
 *                    indicates the packet is being transmitted from the device
 *                    to the host and contains SPI read payload.
 *
 *     data index:    The data index indicates the number of bytes in the
 *                    read or write buffers that have already been transmitted.
 *                    It is used to validate that no packets have been dropped
 *                    and that the prior packets have been correctly decoded.
 *                    This value corresponds to the offset bytes in the buffer
 *                    to start copying the payload into.
 *
 *     read and write payload:
 *                    Contains up to 60 bytes of payload data to transfer to
 *                    the SPI write buffer or from the SPI read buffer.
 *
 *
 * Command Get Configuration Packet (Host to Device):
 *
 *      Query the device to request its USB SPI configuration indicating
 *      the number of bytes it can write and read.
 *
 *     +----------------+
 *     | packet id : 2B |
 *     +----------------+
 *
 *     packet id:     2 byte enum USB_SPI_PKT_ID_CMD_GET_USB_SPI_CONFIG
 *
 * Response Configuration Packet (Device to Host):
 *
 *      Response packet form the device to report the maximum write and
 *      read size supported by the device.
 *
 *     +----------------+----------------+---------------+----------------+
 *     | packet id : 2B | max write : 2B | max read : 2B | feature bitmap |
 *     +----------------+----------------+---------------+----------------+
 *
 *     packet id:         2 byte enum USB_SPI_PKT_ID_RSP_USB_SPI_CONFIG
 *
 *     max write count:   2 byte count of the maximum number of bytes
 *                        the device can write to SPI in one transaction.
 *
 *     max read count:    2 byte count of the maximum number of bytes
 *                        the device can read from SPI in one transaction.
 *
 *     feature bitmap:    Bitmap of supported features.
 *                        BIT(0): Full duplex SPI mode is supported
 *                        BIT(1:15): Reserved for future use
 *
 * Command Restart Response Packet (Host to Device):
 *
 *      Command to restart the response transfer from the device. This enables
 *      the host to recover from a lost packet when reading the response
 *      without restarting the SPI transfer.
 *
 *     +----------------+
 *     | packet id : 2B |
 *     +----------------+
 *
 *     packet id:         2 byte enum USB_SPI_PKT_ID_CMD_RESTART_RESPONSE
 *
 * USB Error Codes:
 *
 * send_command return codes have the following format:
 *
 *     0x00000:         Status code success.
 *     0x00001-0x0FFFF: Error code returned by the USB SPI device.
 *     0x10001-0x1FFFF: Error code returned by the USB SPI host.
 *     0x20001-0x20063  Lower bits store the positive value representation
 *                      of the libusb_error enum. See the libusb documentation:
 *                      http://libusb.sourceforge.net/api-1.0/group__misc.html
 */

#include "programmer.h"
#include "spi.h"
#include "usb_device.h"

#include <libusb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Table is empty as raiden_debug_spi matches against the class and
 * subclass of the connected USB devices, rather than looking for a
 * device with a specific vid:pid.
 */
static const struct dev_entry devs_raiden[] = {
	{0},
};

#define GOOGLE_VID                  (0x18D1)
#define GOOGLE_RAIDEN_SPI_SUBCLASS  (0x51)

enum {
	GOOGLE_RAIDEN_SPI_PROTOCOL_V1 = 0x01,
	GOOGLE_RAIDEN_SPI_PROTOCOL_V2 = 0x02,
};

enum {
	/* The host failed to transfer the data with no libusb error. */
	USB_SPI_HOST_TX_BAD_TRANSFER      = 0x10001,
	/* The number of bytes written did not match expected. */
	USB_SPI_HOST_TX_WRITE_FAILURE     = 0x10002,

	/* We did not receive the expected USB packet. */
	USB_SPI_HOST_RX_UNEXPECTED_PACKET = 0x11001,
	/* We received a continue packet with an invalid data index. */
	USB_SPI_HOST_RX_BAD_DATA_INDEX    = 0x11002,
	/* We received too much data. */
	USB_SPI_HOST_RX_DATA_OVERFLOW     = 0x11003,
	/* The number of bytes read did not match expected. */
	USB_SPI_HOST_RX_READ_FAILURE      = 0x11004,

	/* We were unable to configure the device. */
	USB_SPI_HOST_INIT_FAILURE         = 0x12001,
};

enum usb_spi_error {
	USB_SPI_SUCCESS                 = 0x0000,
	USB_SPI_TIMEOUT                 = 0x0001,
	USB_SPI_BUSY                    = 0x0002,
	USB_SPI_WRITE_COUNT_INVALID     = 0x0003,
	USB_SPI_READ_COUNT_INVALID      = 0x0004,
	USB_SPI_DISABLED                = 0x0005,
	/* The RX continue packet's data index is invalid. */
	USB_SPI_RX_BAD_DATA_INDEX       = 0x0006,
	/* The RX endpoint has received more data than write count. */
	USB_SPI_RX_DATA_OVERFLOW        = 0x0007,
	/* An unexpected packet arrived on the device. */
	USB_SPI_RX_UNEXPECTED_PACKET    = 0x0008,
	/* The device does not support full duplex mode. */
	USB_SPI_UNSUPPORTED_FULL_DUPLEX = 0x0009,
	USB_SPI_UNKNOWN_ERROR           = 0x8000,
};

/* Corresponds with 'enum usb_spi_request' in,
 * platform/cr50/chip/g/usb_spi.h and,
 * platform/ec/chip/stm32/usb_spi.h.
 */
enum raiden_debug_spi_request {
	RAIDEN_DEBUG_SPI_REQ_ENABLE           = 0x0000,
	RAIDEN_DEBUG_SPI_REQ_DISABLE          = 0x0001,
	RAIDEN_DEBUG_SPI_REQ_ENABLE_AP        = 0x0002,
	RAIDEN_DEBUG_SPI_REQ_ENABLE_EC        = 0x0003,
	RAIDEN_DEBUG_SPI_REQ_ENABLE_H1        = 0x0004,
	RAIDEN_DEBUG_SPI_REQ_RESET            = 0x0005,
	RAIDEN_DEBUG_SPI_REQ_BOOT_CFG         = 0x0006,
	RAIDEN_DEBUG_SPI_REQ_SOCKET           = 0x0007,
	RAIDEN_DEBUG_SPI_REQ_SIGNING_START    = 0x0008,
	RAIDEN_DEBUG_SPI_REQ_SIGNING_SIGN     = 0x0009,
	RAIDEN_DEBUG_SPI_REQ_ENABLE_AP_CUSTOM = 0x000a,
};

/*
 * Servo Micro has an error where it is capable of acknowledging USB packets
 * without loading it into the USB endpoint buffers or triggering interrupts.
 * See crbug.com/952494. Retry mechanisms have been implemented to recover
 * from these rare failures allowing the process to continue.
 */
#define WRITE_RETRY_ATTEMPTS      (3)
#define READ_RETRY_ATTEMPTS       (3)
#define GET_CONFIG_RETRY_ATTEMPTS (3)
#define RETRY_INTERVAL_US         (100 * 1000)

/*
 * This timeout is so large because the Raiden SPI timeout is 800ms.
 */
#define TRANSFER_TIMEOUT_MS     (200 + 800)

struct raiden_debug_spi_data {
	struct usb_device *dev;
	uint8_t in_ep;
	uint8_t out_ep;
	uint8_t protocol_version;
	/*
	 * Note: Due to bugs, flashrom does not always treat the max_data_write
	 * and max_data_read counts as the maximum packet size. As a result, we
	 * have to store a local copy of the actual max packet sizes and validate
	 * against it when performing transfers.
	 */
	uint16_t max_spi_write_count;
	uint16_t max_spi_read_count;
	struct spi_master *spi_config;
};
/*
 * USB permits a maximum bulk transfer of 64B.
 */
#define USB_MAX_PACKET_SIZE             (64)
#define PACKET_HEADER_SIZE              (2)

/*
 * All of the USB SPI packets have size equal to the max USB packet size of 64B
 */
#define PAYLOAD_SIZE_V1                 (62)

#define SPI_TRANSFER_V1_MAX             (PAYLOAD_SIZE_V1)

/*
 * Version 1 protocol specific attributes
 */

struct usb_spi_command_v1 {
	uint8_t write_count;
	/* UINT8_MAX indicates full duplex mode on compliant devices. */
	uint8_t read_count;
	uint8_t data[PAYLOAD_SIZE_V1];
} __attribute__((packed));

struct usb_spi_response_v1 {
	uint16_t status_code;
	uint8_t data[PAYLOAD_SIZE_V1];
} __attribute__((packed));

union usb_spi_packet_v1 {
	struct usb_spi_command_v1 command;
	struct usb_spi_response_v1 response;
} __attribute__((packed));

/*
 * Version 2 protocol specific attributes
 */

#define USB_SPI_FULL_DUPLEX_ENABLED_V2      (UINT16_MAX)

#define USB_SPI_PAYLOAD_SIZE_V2_START       (58)

#define USB_SPI_PAYLOAD_SIZE_V2_RESPONSE    (60)

#define USB_SPI_PAYLOAD_SIZE_V2_CONTINUE    (60)

enum packet_id_type {
	/* Request USB SPI configuration data from device. */
	USB_SPI_PKT_ID_CMD_GET_USB_SPI_CONFIG = 0,
	/* USB SPI configuration data from device. */
	USB_SPI_PKT_ID_RSP_USB_SPI_CONFIG     = 1,
	/*
	 * Start a USB SPI transfer specifying number of bytes to write,
	 * read and deliver first packet of data to write.
	 */
	USB_SPI_PKT_ID_CMD_TRANSFER_START     = 2,
	/* Additional packets containing write payload. */
	USB_SPI_PKT_ID_CMD_TRANSFER_CONTINUE  = 3,
	/*
	 * Request the device restart the response enabling us to recover
	 * from packet loss without another SPI transfer.
	 */
	USB_SPI_PKT_ID_CMD_RESTART_RESPONSE   = 4,
	/*
	 * First packet of USB SPI response with the status code
	 * and read payload if it was successful.
	 */
	USB_SPI_PKT_ID_RSP_TRANSFER_START     = 5,
	/* Additional packets containing read payload. */
	USB_SPI_PKT_ID_RSP_TRANSFER_CONTINUE  = 6,
};

enum feature_bitmap {
	/* Indicates the platform supports full duplex mode. */
	USB_SPI_FEATURE_FULL_DUPLEX_SUPPORTED = 0x01
};

struct usb_spi_response_configuration_v2 {
	uint16_t packet_id;
	uint16_t max_write_count;
	uint16_t max_read_count;
	uint16_t feature_bitmap;
} __attribute__((packed));

struct usb_spi_command_v2 {
	uint16_t packet_id;
	uint16_t write_count;
	/* UINT16_MAX Indicates readback all on halfduplex compliant devices. */
	uint16_t read_count;
	uint8_t data[USB_SPI_PAYLOAD_SIZE_V2_START];
} __attribute__((packed));

struct usb_spi_response_v2 {
	uint16_t packet_id;
	uint16_t status_code;
	uint8_t data[USB_SPI_PAYLOAD_SIZE_V2_RESPONSE];
} __attribute__((packed));

struct usb_spi_continue_v2 {
	uint16_t packet_id;
	uint16_t data_index;
	uint8_t data[USB_SPI_PAYLOAD_SIZE_V2_CONTINUE];
} __attribute__((packed));

union usb_spi_packet_v2 {
	uint16_t packet_id;
	struct usb_spi_command_v2 cmd_start;
	struct usb_spi_continue_v2 cmd_continue;
	struct usb_spi_response_configuration_v2 rsp_config;
	struct usb_spi_response_v2 rsp_start;
	struct usb_spi_continue_v2 rsp_continue;
} __attribute__((packed));

struct usb_spi_packet_ctx {
	union {
		uint8_t bytes[USB_MAX_PACKET_SIZE];
		union usb_spi_packet_v1 packet_v1;
		union usb_spi_packet_v2 packet_v2;
	};
	/*
	 * By storing the number of bytes in the header and knowing that the
	 * USB data packets are all 64B long, we are able to use the header
	 * size to store the offset of the buffer and it's size without
	 * duplicating variables that can go out of sync.
	 */
	size_t header_size;
	/* Number of bytes in the packet */
	size_t packet_size;
};

struct usb_spi_transmit_ctx {
	/* Buffer we are reading data from. */
	const uint8_t *buffer;
	/* Number of bytes in the transfer. */
	size_t transmit_size;
	/* Number of bytes transferred. */
	size_t transmit_index;
};

struct usb_spi_receive_ctx {
	/* Buffer we are writing data into. */
	uint8_t *buffer;
	/* Number of bytes in the transfer. */
	size_t receive_size;
	/* Number of bytes transferred. */
	size_t receive_index;
};

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

static struct raiden_debug_spi_data *
	get_raiden_data_from_context(const struct flashctx *flash)
{
	return (struct raiden_debug_spi_data *)flash->mst->spi.data;
}

/*
 * Read data into the receive buffer.
 *
 * @param dst       Destination receive context we are writing data to.
 * @param src       Source packet context we are reading data from.
 *
 * @returns         status code 0 on success.
 *                  USB_SPI_HOST_RX_DATA_OVERFLOW if the source packet is too
 *                  large to fit in read buffer.
 */
static int read_usb_packet(struct usb_spi_receive_ctx *dst,
		const struct usb_spi_packet_ctx *src)
{
	size_t max_read_length = dst->receive_size - dst->receive_index;
	size_t bytes_in_buffer = src->packet_size - src->header_size;
	const uint8_t *packet_buffer = src->bytes + src->header_size;

	if (bytes_in_buffer > max_read_length) {
		/*
		 * An error occurred, we should not receive more data than
		 * the buffer can support.
		 */
		msg_perr("Raiden: Receive packet overflowed\n"
				"    bytes_in_buffer = %zu\n"
				"    max_read_length = %zu\n"
				"    receive_index   = %zu\n"
				"    receive_size    = %zu\n",
				bytes_in_buffer, max_read_length,
				dst->receive_size, dst->receive_index);
		return USB_SPI_HOST_RX_DATA_OVERFLOW;
	}
	memcpy(dst->buffer + dst->receive_index, packet_buffer,
		bytes_in_buffer);

	dst->receive_index += bytes_in_buffer;
	return 0;
}

/*
 * Fill the USB packet with data from the transmit buffer.
 *
 * @param dst       Destination packet context we are writing data to.
 * @param src       Source transmit context we are reading data from.
 */
static void fill_usb_packet(struct usb_spi_packet_ctx *dst,
		struct usb_spi_transmit_ctx *src)
{
	size_t transmit_size = src->transmit_size - src->transmit_index;
	size_t max_buffer_size = USB_MAX_PACKET_SIZE - dst->header_size;
	uint8_t *packet_buffer = dst->bytes + dst->header_size;

	if (transmit_size > max_buffer_size)
		transmit_size = max_buffer_size;

	memcpy(packet_buffer, src->buffer + src->transmit_index, transmit_size);

	dst->packet_size = dst->header_size + transmit_size;
	src->transmit_index += transmit_size;
}

/*
 * Receive the data from the device USB endpoint and store in the packet.
 *
 * @param ctx_data      Raiden SPI config.
 * @param packet        Destination packet used to store the endpoint data.
 *
 * @returns             Returns status code with 0 on success.
 */
static int receive_packet(const struct raiden_debug_spi_data *ctx_data,
		struct usb_spi_packet_ctx *packet)
{
	int received;
	int status = LIBUSB(libusb_bulk_transfer(ctx_data->dev->handle,
				ctx_data->in_ep,
				packet->bytes,
				USB_MAX_PACKET_SIZE,
				&received,
				TRANSFER_TIMEOUT_MS));
	packet->packet_size = received;
	if (status) {
		msg_perr("Raiden: IN transfer failed\n"
			 "    received = %d\n"
			 "    status   = 0x%05x\n",
			 received, status);
	}
	return status;
}

/*
 * Transmit data from the packet to the device's USB endpoint.
 *
 * @param ctx_data      Raiden SPI config.
 * @param packet        Source packet we will write to the endpoint data.
 *
 * @returns             Returns status code with 0 on success.
 */
static int transmit_packet(const struct raiden_debug_spi_data *ctx_data,
		struct usb_spi_packet_ctx *packet)
{
	int transferred;
	int status = LIBUSB(libusb_bulk_transfer(ctx_data->dev->handle,
				ctx_data->out_ep,
				packet->bytes,
				packet->packet_size,
				&transferred,
				TRANSFER_TIMEOUT_MS));
	if (status || (size_t)transferred != packet->packet_size) {
		if (!status) {
			/* No error was reported, but we didn't transmit the data expected. */
			status = USB_SPI_HOST_TX_BAD_TRANSFER;
		}
		msg_perr("Raiden: OUT transfer failed\n"
			 "    transferred = %d\n"
			 "    packet_size = %zu\n"
			 "    status      = 0x%05x\n",
			 transferred, packet->packet_size, status);

	}
	return status;
}

/*
 * Version 1 protocol command to start a USB SPI transfer and write the payload.
 *
 * @param ctx_data      Raiden SPI config.
 * @param write         Write context of data to transmit and write payload.
 * @param read          Read context of data to receive and read buffer.
 *
 * @returns             Returns status code with 0 on success.
 */
static int write_command_v1(const struct raiden_debug_spi_data *ctx_data,
		struct usb_spi_transmit_ctx *write,
		struct usb_spi_receive_ctx *read)
{
	struct usb_spi_packet_ctx command = {
		.header_size = offsetof(struct usb_spi_command_v1, data),
		.packet_v1.command.write_count = write->transmit_size,
		.packet_v1.command.read_count = read->receive_size
	};

	/* Reset the write context to the start. */
	write->transmit_index = 0;

	fill_usb_packet(&command, write);
	return transmit_packet(ctx_data, &command);
}

/*
 * Version 1 Protocol: Responsible for reading the response of the USB SPI
 * transfer. Status codes from the transfer and any read payload are copied
 * to the read_buffer.
 *
 * @param ctx_data      Raiden SPI config.
 * @param write         Write context of data to transmit and write payload.
 * @param read          Read context of data to receive and read buffer.
 *
 * @returns             Returns status code with 0 on success.
 */
static int read_response_v1(const struct raiden_debug_spi_data *ctx_data,
		struct usb_spi_transmit_ctx *write,
		struct usb_spi_receive_ctx *read)
{
	int status;
	struct usb_spi_packet_ctx response;

	/* Reset the read context to the start. */
	read->receive_index = 0;

	status = receive_packet(ctx_data, &response);
	if (status) {
		/* Return the transfer error since the status_code is unreliable */
		return status;
	}
	if (response.packet_v1.response.status_code) {
		return response.packet_v1.response.status_code;
	}
	response.header_size = offsetof(struct usb_spi_response_v1, data);

	status = read_usb_packet(read, &response);
	return status;
}

/*
 * Version 1 Protocol: Sets up a USB SPI transfer, transmits data to the device,
 * reads the status code and any payload from the device. This will also handle
 * recovery if an error has occurred.
 *
 * @param flash         Flash context storing SPI capabilities and USB device
 *                      information.
 * @param write_count   Number of bytes to write
 * @param read_count    Number of bytes to read
 * @param write_buffer  Address of write buffer
 * @param read_buffer   Address of buffer to store read data
 *
 * @returns             Returns status code with 0 on success.
 */
static int send_command_v1(const struct flashctx *flash,
		unsigned int write_count,
		unsigned int read_count,
		const unsigned char *write_buffer,
		unsigned char *read_buffer)
{
	int status = -1;

	struct usb_spi_transmit_ctx write_ctx = {
		.buffer = write_buffer,
		.transmit_size = write_count
	};
	struct usb_spi_receive_ctx read_ctx = {
		.buffer = read_buffer,
		.receive_size = read_count
	};
	const struct raiden_debug_spi_data *ctx_data = get_raiden_data_from_context(flash);

	if (write_count > ctx_data->max_spi_write_count) {
		msg_perr("Raiden: Invalid write count\n"
			 "    write count = %u\n"
			 "    max write   = %d\n",
			 write_count, ctx_data->max_spi_write_count);
		return SPI_INVALID_LENGTH;
	}

	if (read_count > ctx_data->max_spi_read_count) {
		msg_perr("Raiden: Invalid read count\n"
			 "    read count = %d\n"
			 "    max read   = %d\n",
			 read_count, ctx_data->max_spi_read_count);
		return SPI_INVALID_LENGTH;
	}

	for (unsigned int write_attempt = 0; write_attempt < WRITE_RETRY_ATTEMPTS;
	         write_attempt++) {


		status = write_command_v1(ctx_data, &write_ctx, &read_ctx);

		if (!status &&
			(write_ctx.transmit_index != write_ctx.transmit_size)) {
			/* No errors were reported, but write is incomplete. */
			status = USB_SPI_HOST_TX_WRITE_FAILURE;
		}

		if (status) {
			/* Write operation failed. */
			msg_perr("Raiden: Write command failed\n"
				 "    protocol          = %u\n"
				 "    write count       = %u\n"
				 "    read count        = %u\n"
				 "    transmitted bytes = %zu\n"
				 "    write attempt     = %u\n"
				 "    status            = 0x%05x\n",
				 ctx_data->protocol_version,
				 write_count, read_count, write_ctx.transmit_index,
				 write_attempt + 1, status);
			if (!retry_recovery(status)) {
				/* Reattempting will not result in a recovery. */
				return status;
			}
			default_delay(RETRY_INTERVAL_US);
			continue;
		}

		for (unsigned int read_attempt = 0; read_attempt < READ_RETRY_ATTEMPTS;
				read_attempt++) {

			status = read_response_v1(ctx_data, &write_ctx, &read_ctx);

			if (!status) {
				if (read_ctx.receive_size == read_ctx.receive_index) {
					/* Successful transfer. */
					return status;
				} else {
					/* Report the error from the failed read. */
					status = USB_SPI_HOST_RX_READ_FAILURE;
				}
			}

			/* Read operation failed. */
			msg_perr("Raiden: Read response failed\n"
				 "    protocol       = %u\n"
				 "    write count    = %u\n"
				 "    read count     = %u\n"
				 "    received bytes = %zu\n"
				 "    write attempt  = %u\n"
				 "    read attempt   = %u\n"
				 "    status         = 0x%05x\n",
				 ctx_data->protocol_version,
				 write_count, read_count, read_ctx.receive_index,
				 write_attempt + 1, read_attempt + 1, status);
			if (!retry_recovery(status)) {
				/* Reattempting will not result in a recovery. */
				return status;
			}
			default_delay(RETRY_INTERVAL_US);
		}
	}

	return status;
}

/*
 * Get the USB SPI configuration with the maximum write and read counts, and
 * any enabled features.
 *
 * @param ctx_data      Raiden SPI config.
 *
 * @returns             Returns status code with 0 on success.
 */
static int get_spi_config_v2(struct raiden_debug_spi_data *ctx_data)
{
	int status;
	unsigned int config_attempt;
	struct usb_spi_packet_ctx rsp_config;

	struct usb_spi_packet_ctx cmd_get_config = {
		.header_size = PACKET_HEADER_SIZE,
		.packet_size = PACKET_HEADER_SIZE,
		.packet_v2.packet_id = USB_SPI_PKT_ID_CMD_GET_USB_SPI_CONFIG
	};

	for (config_attempt = 0; config_attempt < GET_CONFIG_RETRY_ATTEMPTS; config_attempt++) {

		status = transmit_packet(ctx_data, &cmd_get_config);
		if (status) {
			msg_perr("Raiden: Failed to transmit get config\n"
			         "    config attempt = %d\n"
			         "    status         = 0x%05x\n",
			         config_attempt + 1, status);
			default_delay(RETRY_INTERVAL_US);
			continue;
		}

		status = receive_packet(ctx_data, &rsp_config);
		if (status) {
			msg_perr("Raiden: Failed to receive packet\n"
			         "    config attempt = %d\n"
			         "    status         = 0x%05x\n",
			         config_attempt + 1, status);
			default_delay(RETRY_INTERVAL_US);
			continue;
		}

		/*
		 * Perform validation on the packet received to verify it is a valid
		 * configuration. If it is, we are ready to perform transfers.
		 */
		if ((rsp_config.packet_v2.packet_id ==
				USB_SPI_PKT_ID_RSP_USB_SPI_CONFIG) ||
				(rsp_config.packet_size ==
				sizeof(struct usb_spi_response_configuration_v2))) {

			/* Set the parameters from the configuration. */
			ctx_data->max_spi_write_count =
				rsp_config.packet_v2.rsp_config.max_write_count;
			ctx_data->max_spi_read_count =
				rsp_config.packet_v2.rsp_config.max_read_count;
			return status;
		}

		/*
		 * Check if we received an error from the device. An error will have no
		 * response data, just the packet_id and status_code.
		 */
		const size_t err_packet_size = sizeof(struct usb_spi_response_v2) -
			USB_SPI_PAYLOAD_SIZE_V2_RESPONSE;
		if (rsp_config.packet_size == err_packet_size &&
				rsp_config.packet_v2.rsp_start.status_code !=
				USB_SPI_SUCCESS) {
			status = rsp_config.packet_v2.rsp_start.status_code;
			if (status == USB_SPI_DISABLED) {
				msg_perr("Raiden: Target SPI bridge is disabled (is WP enabled?)\n");
				return status;
			}
		}

		msg_perr("Raiden: Packet is not a valid config\n"
		         "    config attempt = %d\n"
		         "    packet id      = %u\n"
		         "    packet size    = %zu\n",
		         config_attempt + 1,
		         rsp_config.packet_v2.packet_id,
		         rsp_config.packet_size);
		default_delay(RETRY_INTERVAL_US);
	}
	return USB_SPI_HOST_INIT_FAILURE;
}

/*
 * Version 2 protocol restart the SPI response. This allows us to recover from
 * USB packet errors without restarting the SPI transfer.
 *
 * @param ctx_data      Raiden SPI config.
 *
 * @returns             Returns status code with 0 on success.
 */
static int restart_response_v2(const struct raiden_debug_spi_data *ctx_data)
{
	struct usb_spi_packet_ctx restart_response = {
		.header_size = PACKET_HEADER_SIZE,
		.packet_size = PACKET_HEADER_SIZE,
		.packet_v2.packet_id = USB_SPI_PKT_ID_CMD_RESTART_RESPONSE
	};

	return transmit_packet(ctx_data, &restart_response);
}

/*
 * Version 2 Protocol: command to start a USB SPI transfer and write the payload.
 *
 * @param ctx_data      Raiden SPI config.
 * @param write         Write context of data to transmit and write payload.
 * @param read          Read context of data to receive and read buffer.
 *
 * @returns             Returns status code with 0 on success.
 */
static int write_command_v2(const struct raiden_debug_spi_data *ctx_data,
		struct usb_spi_transmit_ctx *write,
		struct usb_spi_receive_ctx *read)
{
	int status;
	struct usb_spi_packet_ctx continue_packet;

	struct usb_spi_packet_ctx start_usb_spi_packet = {
		.header_size = offsetof(struct usb_spi_command_v2, data),
		.packet_v2.cmd_start.packet_id = USB_SPI_PKT_ID_CMD_TRANSFER_START,
		.packet_v2.cmd_start.write_count = write->transmit_size,
		.packet_v2.cmd_start.read_count = read->receive_size
	};

	/* Reset the write context to the start. */
	write->transmit_index = 0;

	fill_usb_packet(&start_usb_spi_packet, write);
	status = transmit_packet(ctx_data, &start_usb_spi_packet);
	if (status) {
		return status;
	}

	while (write->transmit_index < write->transmit_size) {
		/* Transmit any continue packets. */
		continue_packet.header_size = offsetof(struct usb_spi_continue_v2, data);
		continue_packet.packet_v2.cmd_continue.packet_id =
				USB_SPI_PKT_ID_CMD_TRANSFER_CONTINUE;
		continue_packet.packet_v2.cmd_continue.data_index =
				write->transmit_index;

		fill_usb_packet(&continue_packet, write);

		status = transmit_packet(ctx_data, &continue_packet);
		if (status) {
			return status;
		}
	}

	return status;
}

/*
 * Version 2 Protocol: Command to read a USB SPI transfer response and read the payload.
 *
 * @param ctx_data      Raiden SPI config.
 * @param write         Write context of data to transmit and write payload.
 * @param read          Read context of data to receive and read buffer.
 *
 * @returns             Returns status code with 0 on success.
 */
static int read_response_v2(const struct raiden_debug_spi_data *ctx_data,
		struct usb_spi_transmit_ctx *write,
		struct usb_spi_receive_ctx *read)
{
	int status = -1;
	struct usb_spi_packet_ctx response;

	/* Reset the read context to the start. */
	read->receive_index = 0;

	/* Receive the payload to the servo micro. */
	do {
		status = receive_packet(ctx_data, &response);
		if (status) {
			/* Return the transfer error. */
			return status;
		}
		if (response.packet_v2.packet_id == USB_SPI_PKT_ID_RSP_TRANSFER_START) {
			/*
			 * The host should only see this packet if an error occurs
			 * on the device or if it's the first response packet.
			 */
			if (response.packet_v2.rsp_start.status_code) {
				return response.packet_v2.rsp_start.status_code;
			}
			if (read->receive_index) {
				msg_perr("Raiden: Unexpected start packet id = %u\n",
					 response.packet_v2.rsp_start.packet_id);
				return USB_SPI_HOST_RX_UNEXPECTED_PACKET;
			}
			response.header_size = offsetof(struct usb_spi_response_v2, data);
		} else if (response.packet_v2.packet_id ==
				USB_SPI_PKT_ID_RSP_TRANSFER_CONTINUE) {

			/* We validate that no packets were missed. */
			if (read->receive_index !=
					response.packet_v2.rsp_continue.data_index) {
				msg_perr("Raiden: Bad Index = %u Expected = %zu\n",
					 response.packet_v2.rsp_continue.data_index,
					 read->receive_index);
				return USB_SPI_HOST_RX_BAD_DATA_INDEX;
			}
			response.header_size = offsetof(struct usb_spi_continue_v2, data);
		} else {
			msg_perr("Raiden: Unexpected packet id = %u\n",
				 response.packet_v2.packet_id);
			return USB_SPI_HOST_RX_UNEXPECTED_PACKET;
		}
		status = read_usb_packet(read, &response);
		if (status) {
			return status;
		}
	} while (read->receive_index < read->receive_size);

	return status;
}

/*
 * Version 2 Protocol: Sets up a USB SPI transfer, transmits data to the device,
 * reads the status code and any payload from the device. This will also handle
 * recovery if an error has occurred.
 *
 * In order to avoid having the v2 protocol held back by requiring
 * backwards compatibility with v1 we are duplicating the send_command
 * function. This will allow the 2 versions to diverge in the future
 * so fixes in one do not need to be compatible with the legacy.
 *
 * @param flash         Flash context storing SPI capabilities and USB device
 *                      information.
 * @param write_count   Number of bytes to write
 * @param read_count    Number of bytes to read
 * @param write_buffer  Address of write buffer
 * @param read_buffer   Address of buffer to store read data
 *
 * @returns             Returns status code with 0 on success.
 */
static int send_command_v2(const struct flashctx *flash,
		unsigned int write_count,
		unsigned int read_count,
		const unsigned char *write_buffer,
		unsigned char *read_buffer)
{
	const struct raiden_debug_spi_data *ctx_data =
		get_raiden_data_from_context(flash);
	int status = -1;
	unsigned int write_attempt;
	unsigned int read_attempt;

	struct usb_spi_transmit_ctx write_ctx = {
		.buffer = write_buffer,
		.transmit_size = write_count
	};
	struct usb_spi_receive_ctx read_ctx = {
		.buffer = read_buffer,
		.receive_size = read_count
	};

	if (write_count > ctx_data->max_spi_write_count) {
		msg_perr("Raiden: Invalid write count\n"
			 "    write count = %u\n"
			 "    max write   = %u\n",
			 write_count, ctx_data->max_spi_write_count);
		return SPI_INVALID_LENGTH;
	}

	if (read_count > ctx_data->max_spi_read_count) {
		msg_perr("Raiden: Invalid read count\n"
			 "    read count = %u\n"
			 "    max read   = %u\n",
			 read_count, ctx_data->max_spi_read_count);
		return SPI_INVALID_LENGTH;
	}

	for (write_attempt = 0; write_attempt < WRITE_RETRY_ATTEMPTS;
			write_attempt++) {

		status = write_command_v2(ctx_data, &write_ctx, &read_ctx);

		if (!status &&
			(write_ctx.transmit_index != write_ctx.transmit_size)) {
			/* No errors were reported, but write is incomplete. */
			status = USB_SPI_HOST_TX_WRITE_FAILURE;
		}

		if (status) {
			/* Write operation failed. */
			msg_perr("Raiden: Write command failed\n"
				 "    protocol          = %u\n"
				 "    write count       = %u\n"
				 "    read count        = %u\n"
				 "    transmitted bytes = %zu\n"
				 "    write attempt     = %u\n"
				 "    status            = 0x%05x\n",
				 ctx_data->protocol_version,
				 write_count, read_count, write_ctx.transmit_index,
				 write_attempt + 1, status);
			if (!retry_recovery(status)) {
				/* Reattempting will not result in a recovery. */
				return status;
			}
			default_delay(RETRY_INTERVAL_US);
			continue;
		}
		for (read_attempt = 0; read_attempt < READ_RETRY_ATTEMPTS;
				read_attempt++) {

			status = read_response_v2(ctx_data, &write_ctx, &read_ctx);

			if (!status) {
				if (read_ctx.receive_size == read_ctx.receive_index) {
					/* Successful transfer. */
					return status;
				} else {
					/* Report the error from the failed read. */
					status = USB_SPI_HOST_RX_READ_FAILURE;
				}
			}

			if (status) {
				/* Read operation failed. */
				msg_perr("Raiden: Read response failed\n"
					 "    protocol       = %u\n"
					 "    write count    = %u\n"
					 "    read count     = %u\n"
					 "    received bytes = %zu\n"
					 "    write attempt  = %u\n"
					 "    read attempt   = %u\n"
					 "    status         = 0x%05x\n",
					 ctx_data->protocol_version,
					 write_count, read_count, read_ctx.receive_index,
					 write_attempt + 1, read_attempt + 1, status);
				if (!retry_recovery(status)) {
					/* Reattempting will not result in a recovery. */
					return status;
				}
				/* Device needs to reset its transmit index. */
				restart_response_v2(ctx_data);
				default_delay(RETRY_INTERVAL_US);
			}
		}
	}
	return status;
}

static int raiden_debug_spi_shutdown(void * data)
{
	struct raiden_debug_spi_data *ctx_data = (struct raiden_debug_spi_data *)data;
	struct spi_master *spi_config = ctx_data->spi_config;

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
		free(ctx_data);
		free(spi_config);
		return ret;
	}

	usb_device_free(ctx_data->dev);
	libusb_exit(NULL);
	free(ctx_data);
	free(spi_config);

	return 0;
}

static const struct spi_master spi_master_raiden_debug = {
	.features	= SPI_MASTER_4BA,
	.max_data_read	= 0,
	.max_data_write	= 0,
	.command	= NULL,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.shutdown	= raiden_debug_spi_shutdown,
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

/*
 * Configure the USB SPI master based on the device we are connected to.
 * It will use the device's bInterfaceProtocol to identify which protocol
 * is being used by the device USB SPI interface and if needed query the
 * device for its capabilities.
 *
 * @param ctx_data Raiden SPI data, data contains pointer to config which will be modified.
 *
 * @returns	   Returns status code with 0 on success.
 */
static int configure_protocol(struct raiden_debug_spi_data *ctx_data)
{
	int status = 0;
	struct spi_master *spi_config = ctx_data->spi_config;

	ctx_data->protocol_version =
		ctx_data->dev->interface_descriptor->bInterfaceProtocol;

	switch (ctx_data->protocol_version) {
	case GOOGLE_RAIDEN_SPI_PROTOCOL_V1:
		/*
		 * Protocol V1 is supported by adjusting the max data
		 * read and write sizes which results in no continue packets.
		 */
		spi_config->command = send_command_v1;
		ctx_data->max_spi_write_count = SPI_TRANSFER_V1_MAX;
		ctx_data->max_spi_read_count = SPI_TRANSFER_V1_MAX;
		break;
	case GOOGLE_RAIDEN_SPI_PROTOCOL_V2:
		/*
		 * Protocol V2 requires the host to query the device for
		 * its maximum read and write sizes
		 */
		spi_config->command = send_command_v2;
		status = get_spi_config_v2(ctx_data);
		if (status) {
			return status;
		}
		break;
	default:
		msg_pdbg("Raiden: Unknown USB SPI protocol version = %u\n",
				ctx_data->protocol_version);
		return USB_SPI_HOST_INIT_FAILURE;
	}

	/*
	 * Unfortunately there doesn't seem to be a way to specify the maximum number
	 * of bytes that your SPI device can read/write, these values are the maximum
	 * data chunk size that flashrom will package up with an additional five bytes
	 * of command for the flash device.
	 *
	 * The largest command that flashrom generates is the byte program command, so
	 * we use that command header maximum size here. If we didn't include the
	 * offset, flashrom may request a SPI transfer that is too large for the SPI
	 * device to support.
	 */
	spi_config->max_data_write = ctx_data->max_spi_write_count -
		JEDEC_BYTE_PROGRAM_OUTSIZE;
	spi_config->max_data_read = ctx_data->max_spi_read_count -
		JEDEC_BYTE_PROGRAM_OUTSIZE;

	return 0;
}

static int get_ap_request_type(const struct programmer_cfg *cfg)
{
	int ap_request = RAIDEN_DEBUG_SPI_REQ_ENABLE_AP;
	char *custom_rst_str = extract_programmer_param_str(cfg, "custom_rst");
	if (custom_rst_str) {
		if (!strcasecmp(custom_rst_str, "true")) {
			ap_request = RAIDEN_DEBUG_SPI_REQ_ENABLE_AP_CUSTOM;
		} else if (!strcasecmp(custom_rst_str, "false")) {
			ap_request = RAIDEN_DEBUG_SPI_REQ_ENABLE_AP;
		} else {
			msg_perr("Invalid custom rst param: %s\n",
			         custom_rst_str);
			ap_request = -1;
		}
	}
	free(custom_rst_str);
	return ap_request;
}

static int decode_programmer_param(const struct programmer_cfg *cfg, uint8_t *request,
                                   uint16_t *request_parameter)
{
	/**
	 * REQ_ENABLE doesn't specify a target bus, and will be rejected
	 * by adapters that support more than one target.
	 */
	uint8_t request_enable = RAIDEN_DEBUG_SPI_REQ_ENABLE;
        uint16_t parameter = 0;
        int ret = 0;

	char *target_str = extract_programmer_param_str(cfg, "target");
        printf("FISK: %s\n", target_str);

	if (target_str) {
		char *endptr;
		int index = strtol(target_str, &endptr, 0);
		if (*target_str && !*endptr && index >= 0 && index < 256) {
			request_enable = RAIDEN_DEBUG_SPI_REQ_ENABLE;
                        parameter = index;
		} else if (!strcasecmp(target_str, "ap"))
			request_enable = get_ap_request_type(cfg);
		else if (!strcasecmp(target_str, "ec"))
			request_enable = RAIDEN_DEBUG_SPI_REQ_ENABLE_EC;
		else {
			msg_perr("Invalid target: %s\n", target_str);
			ret = 1;
		}
	}
	free(target_str);
	if (ret == 0) {
		msg_pinfo("Raiden target: %d,%d\n", request_enable, parameter);

                *request = request_enable;
                *request_parameter = parameter;
        }
        return ret;
}

static void free_dev_list(struct usb_device **dev_lst)
{
	struct usb_device *dev = *dev_lst;
	/* free devices we don't care about */
	dev = dev->next;
	while (dev)
		dev = usb_device_free(dev);
}

static int raiden_debug_spi_init(const struct programmer_cfg *cfg)
{
	struct usb_match match;
	char *serial = extract_programmer_param_str(cfg, "serial");
	struct usb_device *current;
	struct usb_device *device = NULL;
	bool found = false;
	int ret;

	uint8_t request_enable;
        uint16_t request_parameter;
        ret = decode_programmer_param(cfg, &request_enable, &request_parameter);
	if (ret != 0) {
		free(serial);
		return ret;
	}

	usb_match_init(cfg, &match);

	usb_match_value_default(&match.vid,      GOOGLE_VID);
	usb_match_value_default(&match.class,    LIBUSB_CLASS_VENDOR_SPEC);
	usb_match_value_default(&match.subclass, GOOGLE_RAIDEN_SPI_SUBCLASS);

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
			found = true;
			goto loop_end;
		} else {
			unsigned char dev_serial[32] = { 0 };
			struct libusb_device_descriptor descriptor;
			int rc;

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
					found = true;
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
				request_parameter,
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

	struct spi_master *spi_config = calloc(1, sizeof(*spi_config));
	if (!spi_config) {
		msg_perr("Unable to allocate space for SPI master.\n");
		return SPI_GENERIC_ERROR;
	}
	struct raiden_debug_spi_data *data = calloc(1, sizeof(*data));
	if (!data) {
		free(spi_config);
		msg_perr("Unable to allocate space for extra SPI master data.\n");
		return SPI_GENERIC_ERROR;
	}

	*spi_config = spi_master_raiden_debug;

	data->dev = device;
	data->in_ep = in_endpoint;
	data->out_ep = out_endpoint;
	data->spi_config = spi_config;

	/*
	 * The SPI master needs to be configured based on the device connected.
	 * Using the device protocol interrogation, we will set the limits on
	 * the write and read sizes and switch command functions.
	 */
	ret = configure_protocol(data);
	if (ret) {
		msg_perr("Raiden: Error configuring protocol\n"
			 "    protocol       = %u\n"
			 "    status         = 0x%05x\n",
			 data->dev->interface_descriptor->bInterfaceProtocol, ret);
		free(data);
		free(spi_config);
		return SPI_GENERIC_ERROR;
	}

	return register_spi_master(spi_config, data);
}

const struct programmer_entry programmer_raiden_debug_spi = {
	.name			= "raiden_debug_spi",
	.type			= USB,
	.devs.dev		= devs_raiden,
	.init			= raiden_debug_spi_init,
};
