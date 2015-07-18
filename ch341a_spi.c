/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2011 asbokid <ballymunboy@gmail.com> 
 * Copyright (C) 2014 Pluto Yang <yangyj.ee@gmail.com>
 * Copyright (C) 2015 Stefan Tauner
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <string.h>
#include "flash.h"
#include "programmer.h"
#include "libusb-1.0/libusb.h"

#define	 DEFAULT_TIMEOUT		300	// 300ms for USB timeouts
#define	 BULK_WRITE_ENDPOINT		0x02
#define	 BULK_READ_ENDPOINT		0x82

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
#define	 CH341A_CMD_I2C_STM_SET		0x60 // bit 7 apparently SPI bit order, bit 2 spi single vs spi double
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

struct libusb_device_handle *devHandle = NULL;

const struct dev_entry devs_ch341a_spi[] = {
	{0x1A86, 0x5512, OK, "Winchiphead (WCH)", "CH341A"},

	{0},
};

/* Helper function for libusb_bulk_transfer, display error message with the caller name */
static int32_t usbTransfer(const char *func, uint8_t type, uint8_t *buf, int len)
{
	int32_t ret;
	int transfered;
	if (devHandle == NULL)
	return -1;
	ret = libusb_bulk_transfer(devHandle, type, buf, len, &transfered, DEFAULT_TIMEOUT);
	if (ret < 0) {
		fprintf(stderr, "%s: Failed to %s %d bytes '%s'\n", func,
				(type == BULK_WRITE_ENDPOINT) ? "write" : "read", len, strerror(-ret));
		return -1;
	}
	return transfered;
}


/*   set the i2c bus speed (speed(b1b0): 0 = 20kHz; 1 = 100kHz, 2 = 400kHz, 3 = 750kHz)
 *   set the spi bus data width(speed(b2): 0 = Single, 1 = Double)  */
static int32_t ch341SetStream(uint32_t speed)
{
	if (devHandle == NULL)
		return -1;

	uint8_t buf[] = {
		CH341A_CMD_I2C_STREAM,
		CH341A_CMD_I2C_STM_SET | (speed & 0x7),
		CH341A_CMD_I2C_STM_END
	};

	int32_t ret = usbTransfer(__func__, BULK_WRITE_ENDPOINT, buf, sizeof(buf));
	if (ret < 0) {
		msg_perr("Could not set SPI frequency.\n");
	}
	return ret;
}

/* ch341 requres LSB first, swap the bit order before send and after receive */
static uint8_t swapByte(uint8_t c)
{
	uint8_t result=0;
	for (unsigned int i = 0; i < 8; ++i) {
		result = result << 1;
		result |= (c & 1);
		c = c >> 1;
	}
	return result;
}

/* assert or deassert the chip-select pin of the spi device */
static void ch341SpiCs(uint8_t *ptr, bool selected)
{
	*ptr++ = CH341A_CMD_UIO_STREAM;
	*ptr++ = CH341A_CMD_UIO_STM_OUT | (selected ? 0x36 : 0x37);
		*ptr++ = CH341A_CMD_UIO_STM_DIR | 0x3F; // pin direction
	*ptr++ = CH341A_CMD_UIO_STM_END;
}

static int ch341a_spi_spi_send_command(struct flashctx *flash, unsigned int writecnt, unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr)
{
	uint8_t buf[CH341_MAX_PACKET_LEN];
	int32_t ret;

	if (devHandle == NULL)
		return -1;

	if (writecnt + readcnt > CH341_MAX_PACKET_LEN - CH341_PACKET_LENGTH)
		return -1;

	ch341SpiCs(buf, true);
	uint8_t *ptr = buf + CH341_PACKET_LENGTH; // don't care what's after CH341A_CMD_UIO_STM_END
	*ptr++ = CH341A_CMD_SPI_STREAM;
	for (int i = 0; i < writecnt; ++i)
		*ptr++ = swapByte(*writearr++);
	ret = usbTransfer(__func__, BULK_WRITE_ENDPOINT, buf, writecnt + readcnt + CH341_PACKET_LENGTH + 1);
	if (ret < 0)
		return -1;
	ret = usbTransfer(__func__, BULK_READ_ENDPOINT, buf, writecnt + readcnt);
	if (ret < 0)
		return -1;
	ptr = readarr;
	for (int i = 0; i < ret; i++) { // swap the buffer
		ptr[i] = swapByte(buf[writecnt + i]);
	}
	ch341SpiCs(buf, false);
	ret = usbTransfer(__func__, BULK_WRITE_ENDPOINT, buf, 3);
	if (ret < 0)
		return -1;
	return 0;
}

static const struct spi_master spi_master_ch341a_spi = {
	.type		= SPI_CONTROLLER_CH341A_SPI,
	.max_data_read	= 64 * 1024, /* Maximum data read size in one go (excluding opcode+address). */
	.max_data_write	= 256, /* Maximum data write size in one go (excluding opcode+address). */
	.command	= ch341a_spi_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

static int ch341a_spi_shutdown(void *data)
{
	if (devHandle == NULL)
		return -1;
	libusb_release_interface(devHandle, 0);
	libusb_close(devHandle);
	libusb_exit(NULL);
	devHandle = NULL;
	//sigaction(SIGINT, &saold, NULL);
	return 0;
}

int ch341a_spi_init(void)
{
	struct libusb_device *dev;
	int32_t ret;
	int vid = devs_ch341a_spi[0].vendor_id;
	int pid = devs_ch341a_spi[0].device_id;

	if (devHandle != NULL) {
		fprintf(stderr, "Call ch341Release before re-configure\n");
		return -1;
	}
	ret = libusb_init(NULL);
	if (ret < 0) {
		fprintf(stderr, "Couldnt initialise libusb\n");
		return -1;
	}

	libusb_set_debug(NULL, 3);
	
	if (!(devHandle = libusb_open_device_with_vid_pid(NULL, vid, pid))) {
		fprintf(stderr, "Couldn't open device [%04x:%04x].\n", vid, pid);
		return -1;
	}
 
	if (!(dev = libusb_get_device(devHandle))) {
		fprintf(stderr, "Couldn't get bus number and address.\n");
		goto close_handle;
	}

	if (libusb_kernel_driver_active(devHandle, 0)) {
		ret = libusb_detach_kernel_driver(devHandle, 0);
		if (ret) {
			fprintf(stderr, "Failed to detach kernel driver: '%s'\n", strerror(-ret));
			goto close_handle;
		}
	}
	
	ret = libusb_claim_interface(devHandle, 0);
	if (ret != 0) {
		fprintf(stderr, "Failed to claim interface 0: '%s'\n", libusb_error_name(ret));
		goto close_handle;
	}

	struct libusb_device_descriptor desc;
	ret = libusb_get_device_descriptor(dev, &desc);
	if (ret < 0) {
		fprintf(stderr, "Failed to get device descriptor: '%s'\n", libusb_error_name(ret));
		goto release_interface;
	}
	
	ret = ch341SetStream(CH341A_STM_I2C_100K);
	msg_pdbg("Device revision is %d.%01d.%01d\n",
		(desc.bcdDevice >> 8) & 0x00FF,
		(desc.bcdDevice >> 4) & 0x000F,
		(desc.bcdDevice >> 0) & 0x000F);
	if (ret < 0)
		goto release_interface;

	register_shutdown(ch341a_spi_shutdown, NULL);
	register_spi_master(&spi_master_ch341a_spi);

	return 0;

release_interface:
	libusb_release_interface(devHandle, 0);
close_handle:
	libusb_close(devHandle);
	devHandle = NULL;
	return -1;
}
