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

/* Vendor defines
#define		IOCTL_CH341_COMMAND		( FILE_DEVICE_UNKNOWN << 16 | FILE_ANY_ACCESS << 14 | 0x0f34 << 2 | METHOD_BUFFERED )
#define		mWIN32_COMMAND_HEAD		mOFFSET( mWIN32_COMMAND, mBuffer )
#define		mCH341_MAX_NUMBER		16
#define		mMAX_BUFFER_LENGTH		0x1000
#define		mMAX_COMMAND_LENGTH		( mWIN32_COMMAND_HEAD + mMAX_BUFFER_LENGTH )
#define		mDEFAULT_BUFFER_LEN		0x0400
#define		mDEFAULT_COMMAND_LEN	( mWIN32_COMMAND_HEAD + mDEFAULT_BUFFER_LEN )

#define		mCH341_ENDP_INTER_UP	0x81
#define		mCH341_ENDP_INTER_DOWN	0x01
#define		mCH341_ENDP_DATA_UP		0x82
#define		mCH341_ENDP_DATA_DOWN	0x02

#define		mPipeDeviceCtrl			0x00000004
#define		mPipeInterUp			0x00000005
#define		mPipeDataUp				0x00000006
#define		mPipeDataDown			0x00000007

#define		mFuncNoOperation		0x00000000
#define		mFuncGetVersion			0x00000001
#define		mFuncGetConfig			0x00000002
#define		mFuncSetTimeout			0x00000009
#define		mFuncSetExclusive		0x0000000b
#define		mFuncResetDevice		0x0000000c
#define		mFuncResetPipe			0x0000000d
#define		mFuncAbortPipe			0x0000000e

#define		mFuncSetParaMode		0x0000000f
#define		mFuncReadData0			0x00000010
#define		mFuncReadData1			0x00000011
#define		mFuncWriteData0			0x00000012
#define		mFuncWriteData1			0x00000013
#define		mFuncWriteRead			0x00000014
#define		mFuncBufferMode			0x00000020
#define		mFuncBufferModeDn		0x00000021

#define		mUSB_CLR_FEATURE		0x01
#define		mUSB_SET_FEATURE		0x03
#define		mUSB_GET_STATUS			0x00
#define		mUSB_SET_ADDRESS		0x05
#define		mUSB_GET_DESCR			0x06
#define		mUSB_SET_DESCR			0x07
#define		mUSB_GET_CONFIG			0x08
#define		mUSB_SET_CONFIG			0x09
#define		mUSB_GET_INTERF			0x0a
#define		mUSB_SET_INTERF			0x0b
#define		mUSB_SYNC_FRAME			0x0c

#define		mCH341_VENDOR_READ		0xC0
#define		mCH341_VENDOR_WRITE		0x40

#define		mCH341_PARA_INIT		0xB1
#define		mCH341_I2C_STATUS		0x52
#define		mCH341_I2C_COMMAND		0x53

#define		mCH341_PARA_CMD_R0		0xAC
#define		mCH341_PARA_CMD_R1		0xAD
#define		mCH341_PARA_CMD_W0		0xA6
#define		mCH341_PARA_CMD_W1		0xA7
#define		mCH341_PARA_CMD_STS		0xA0

#define		mCH341A_CMD_SET_OUTPUT	0xA1
#define		mCH341A_CMD_IO_ADDR		0xA2
#define		mCH341A_CMD_PRINT_OUT	0xA3
#define		mCH341A_CMD_SPI_STREAM	0xA8
#define		mCH341A_CMD_SIO_STREAM	0xA9
#define		mCH341A_CMD_I2C_STREAM	0xAA
#define		mCH341A_CMD_UIO_STREAM	0xAB

#define		mCH341A_BUF_CLEAR		0xB2
#define		mCH341A_I2C_CMD_X		0x54
#define		mCH341A_DELAY_MS		0x5E
#define		mCH341A_GET_VER			0x5F

#define		mCH341_EPP_IO_MAX		( mCH341_PACKET_LENGTH - 1 )
#define		mCH341A_EPP_IO_MAX		0xFF

#define		mCH341A_CMD_IO_ADDR_W	0x00
#define		mCH341A_CMD_IO_ADDR_R	0x80

#define		mCH341A_CMD_I2C_STM_STA	0x74
#define		mCH341A_CMD_I2C_STM_STO	0x75
#define		mCH341A_CMD_I2C_STM_OUT	0x80
#define		mCH341A_CMD_I2C_STM_IN	0xC0
#define		mCH341A_CMD_I2C_STM_MAX	( min( 0x3F, mCH341_PACKET_LENGTH ) )
#define		mCH341A_CMD_I2C_STM_SET	0x60
#define		mCH341A_CMD_I2C_STM_US	0x40
#define		mCH341A_CMD_I2C_STM_MS	0x50
#define		mCH341A_CMD_I2C_STM_DLY	0x0F
#define		mCH341A_CMD_I2C_STM_END	0x00

#define		mCH341A_CMD_UIO_STM_IN	0x00
#define		mCH341A_CMD_UIO_STM_DIR	0x40
#define		mCH341A_CMD_UIO_STM_OUT	0x80
#define		mCH341A_CMD_UIO_STM_US	0xC0
#define		mCH341A_CMD_UIO_STM_END	0x20

#define		mCH341_PARA_MODE_EPP	0x00
#define		mCH341_PARA_MODE_EPP17	0x00
#define		mCH341_PARA_MODE_EPP19	0x01
#define		mCH341_PARA_MODE_MEM	0x02

#define		mStateBitERR			0x00000100
#define		mStateBitPEMP			0x00000200
#define		mStateBitINT			0x00000400
#define		mStateBitSLCT			0x00000800
#define		mStateBitWAIT			0x00002000
#define		mStateBitDATAS			0x00004000
#define		mStateBitADDRS			0x00008000
#define		mStateBitRESET			0x00010000
#define		mStateBitWRITE			0x00020000
#define		mStateBitSDA			0x00800000

*/
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

static void print_hex(void *buf, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		msg_pspew(" %02x", ((uint8_t *)buf)[i]);
		if (i % CH341_PACKET_LENGTH == CH341_PACKET_LENGTH - 1)
			msg_pspew("\n");
	}
}

/* Helper function for libusb_bulk_transfer, display error message with the caller name */
static int32_t usbTransfer(const char *func, uint8_t type, uint8_t *buf, int len)
{
	if (devHandle == NULL)
		return -1;

	int transferred;
	int32_t ret = libusb_bulk_transfer(devHandle, type, buf, len, &transferred, DEFAULT_TIMEOUT);
	if (ret < 0) {
		fprintf(stderr, "%s: Failed to %s %d bytes '%s'\n",
			func, (type == BULK_WRITE_ENDPOINT) ? "write" : "read", len, libusb_error_name(ret));
		return -1;
	}
	msg_pspew("%s %d bytes:\n", (type == BULK_WRITE_ENDPOINT) ? "Wrote" : "Read", len);
	print_hex(buf, len);
	msg_pspew("\n\n");
	return transferred;
}

static int32_t ch341a_enable_pins(bool enable)
{
	uint8_t buf[] = {
		CH341A_CMD_UIO_STREAM,
		CH341A_CMD_UIO_STM_OUT | 0x37, // CS high,
		CH341A_CMD_UIO_STM_DIR | (enable ? 0x3F : 0x3E), // CS output enable
		CH341A_CMD_UIO_STM_END,
	};

	int32_t ret = usbTransfer(__func__, BULK_WRITE_ENDPOINT, buf, sizeof(buf));
	if (ret < 0) {
		msg_perr("Could not %sable output pins.\n", enable ? "en" : "dis");
	}
	return ret;
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
static void ch341SpiCs(uint8_t **ptr, bool selected)
{
	*(*ptr)++ = CH341A_CMD_UIO_STREAM;
	*(*ptr)++ = CH341A_CMD_UIO_STM_OUT | (selected ? 0x36 : 0x37);
	*(*ptr)++ = CH341A_CMD_UIO_STM_END;
}

static int ch341a_spi_spi_send_command(struct flashctx *flash, unsigned int writecnt, unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr)
{
	if (devHandle == NULL)
		return -1;

	/* The transaction size is determined by the data sent out comprising
	 *  - the CS packet
	 *  - SPI stream opcode
	 *  - raw SPI output data
	 *  - raw SPI input data
	 */
	/* How many  packets ... */
	const size_t packets = ((writecnt+readcnt)+(CH341_PACKET_LENGTH-2)) / (CH341_PACKET_LENGTH-1);

	uint8_t buf[CH341_PACKET_LENGTH*2];
	memset(buf, 0, CH341_PACKET_LENGTH); // to silence valgrind, and really, we dont want to write stack random to device

	uint8_t *ptr = buf;
	ch341SpiCs(&ptr, true);
	unsigned int write_left = writecnt;
	unsigned int read_left = readcnt;
	int32_t ret;
	for (int p = 0; p < packets; p++) {
		unsigned int write_now = min( CH341_PACKET_LENGTH-1, write_left );
		unsigned int read_now = min ( (CH341_PACKET_LENGTH-1) - write_now, read_left );
		ptr = buf + CH341_PACKET_LENGTH;
		*ptr++ = CH341A_CMD_SPI_STREAM;
		for (unsigned int i = 0; i < write_now; ++i)
			*ptr++ = swapByte(*writearr++);
		write_left -= write_now;
		ret = usbTransfer(__func__, BULK_WRITE_ENDPOINT, p ? buf + CH341_PACKET_LENGTH : buf, (!p ? CH341_PACKET_LENGTH : 0 ) + 1 + write_now + read_now);
		if (ret < 0)
			return -1;
		ret = usbTransfer(__func__, BULK_READ_ENDPOINT, buf, write_now + read_now);
		if (ret < 0)
			return -1;
		if (read_now) {
			for (unsigned int i = 0; i < read_now; i++)
				*readarr++ = swapByte(buf[write_now + i]);
			read_left -= read_now;
		}
	}
	ptr = buf;
	ch341SpiCs(&ptr, false);
	ret = usbTransfer(__func__, BULK_WRITE_ENDPOINT, buf, 3);
	if (ret < 0)
		return -1;
	return 0;
}

	//int (*read)(struct flashctx *flash, uint8_t *buf, unsigned int start, unsigned int len);
	//int (*write_256)(struct flashctx *flash, const uint8_t *buf, unsigned int start, unsigned int len);

/* read the content of SPI device to buf, make sure the buf is big enough before call  */
//int32_t ch341SpiRead(uint8_t *buf, uint32_t add, uint32_t len)
//{
    //uint8_t out[CH341_MAX_PACKET_LEN];
    //uint8_t in[CH341_PACKET_LENGTH];
//
    //if (devHandle == NULL) return -1;
    ///* what subtracted is: 1. first cs package, 2. leading command for every other packages,
     //* 3. second package contains read flash command and 3 bytes address */
    //const uint32_t max_payload = CH341_MAX_PACKET_LEN - CH341_PACKET_LENGTH - CH341_MAX_PACKETS + 1 - 4;
    //uint32_t tmp, pkg_len, pkg_count;
    //struct libusb_transfer *xferBulkIn, *xferBulkOut;
    //uint32_t idx = 0;
    //uint32_t ret;
    //int32_t old_counter;
    //struct timeval tv = {0, 100};
//
    //memset(out, 0xff, CH341_MAX_PACKET_LEN);
    //for (int i = 1; i < CH341_MAX_PACKETS; ++i) // fill CH341A_CMD_SPI_STREAM for every packet
        //out[i * CH341_PACKET_LENGTH] = CH341A_CMD_SPI_STREAM;
    //memset(in, 0x00, CH341_PACKET_LENGTH);
    //xferBulkIn  = libusb_alloc_transfer(0);
    //xferBulkOut = libusb_alloc_transfer(0);
//
    //while (len > 0) {
        //ch341SpiCs(out, true);
        //idx = CH341_PACKET_LENGTH + 1;
        //out[idx++] = 0xC0; // byte swapped command for Flash Read
        //tmp = add;
        //for (int i = 0; i < 3; ++i) { // starting address of next read
            //out[idx++] = swapByte((tmp >> 16) & 0xFF);
            //tmp <<= 8;
        //}
        //if (len > max_payload) {
            //pkg_len = CH341_MAX_PACKET_LEN;
            //pkg_count = CH341_MAX_PACKETS - 1;
            //len -= max_payload;
            //add += max_payload;
        //} else {
            //pkg_count = (len + 4) / (CH341_PACKET_LENGTH - 1);
            //if ((len + 4) % (CH341_PACKET_LENGTH - 1)) pkg_count ++;
            //pkg_len = (pkg_count) * CH341_PACKET_LENGTH + ((len + 4) % (CH341_PACKET_LENGTH - 1)) + 1;
            //len = 0;
        //}
        //bulkin_count = 0;
        //libusb_fill_bulk_transfer(xferBulkIn, devHandle, BULK_READ_ENDPOINT, in,
                //CH341_PACKET_LENGTH, cbBulkIn, buf, DEFAULT_TIMEOUT);
        //buf += max_payload; // advance user's pointer
        //libusb_submit_transfer(xferBulkIn);
        //libusb_fill_bulk_transfer(xferBulkOut, devHandle, BULK_WRITE_ENDPOINT, out,
                //pkg_len, cbBulkOut, NULL, DEFAULT_TIMEOUT);
        //libusb_submit_transfer(xferBulkOut);
        //old_counter = bulkin_count;
        //while (bulkin_count < pkg_count) {
            //libusb_handle_events_timeout(NULL, &tv);
            //if (bulkin_count == -1) { // encountered error
                //len = 0;
                //ret = -1;
                //break;
            //}
            //if (old_counter != bulkin_count) { // new package came
                //if (bulkin_count != pkg_count)
                    //libusb_submit_transfer(xferBulkIn);  // resubmit bulk in request
                //old_counter = bulkin_count;
            //}
        //}
        //ch341SpiCs(out, false);
        //ret = usbTransfer(__func__, BULK_WRITE_ENDPOINT, out, 3);
        //if (ret < 0) break;
        //if (force_stop == 1) { // user hit ctrl+C
            //force_stop = 0;
            //if (len > 0)
                //fprintf(stderr, "User hit Ctrl+C, reading unfinished.\n");
            //break;
        //}
    //}
    //libusb_free_transfer(xferBulkIn);
    //libusb_free_transfer(xferBulkOut);
    //return ret;
//}

static const struct spi_master spi_master_ch341a_spi = {
	.type		= SPI_CONTROLLER_CH341A_SPI,
	.max_data_read	= 20, /* Maximum data read size in one go (excluding opcode+address). */
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
	ch341a_enable_pins(false); // disable output pins
	libusb_release_interface(devHandle, 0);
	libusb_close(devHandle);
	libusb_exit(NULL);
	devHandle = NULL;
	return 0;
}

int ch341a_spi_init(void)
{
	struct libusb_device *dev;
	int32_t ret;
	int vid = devs_ch341a_spi[0].vendor_id;
	int pid = devs_ch341a_spi[0].device_id;

	if (devHandle != NULL) {
		fprintf(stderr, "Call ch341_spi_shutdown before re-configure\n");
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

	ret = libusb_kernel_driver_active(devHandle, 0);
	if (ret < 0) {
			fprintf(stderr, "Failed to detach kernel driver: '%s'\n", libusb_error_name(ret));
			goto close_handle;
	} else if (ret == 1) {
		ret = libusb_detach_kernel_driver(devHandle, 0);
		if (ret == LIBUSB_ERROR_NOT_SUPPORTED) {
			msg_pwarn("Could not detach kernel driver. Further accesses will probably fail.\n");
		} else if (ret != 0) {
			fprintf(stderr, "Failed to detach kernel driver: '%s'\n", libusb_error_name(ret));
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

	msg_pdbg("Device revision is %d.%01d.%01d\n",
		(desc.bcdDevice >> 8) & 0x00FF,
		(desc.bcdDevice >> 4) & 0x000F,
		(desc.bcdDevice >> 0) & 0x000F);
	ret = ch341SetStream(CH341A_STM_I2C_100K) | ch341a_enable_pins(true);
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
