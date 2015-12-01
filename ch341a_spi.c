/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2011 asbokid <ballymunboy@gmail.com>
 * Copyright (C) 2014 Pluto Yang <yangyj.ee@gmail.com>
 * Copyright (C) 2015 Stefan Tauner
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <string.h>
#include "flash.h"
#include "programmer.h"
#include <libusb.h>

/* Vendor defines

#define		mCH341_ENDP_INTER_UP	0x81
#define		mCH341_ENDP_INTER_DOWN	0x01

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

#define		mCH341A_BUF_CLEAR		0xB2
#define		mCH341A_I2C_CMD_X		0x54
#define		mCH341A_DELAY_MS		0x5E
#define		mCH341A_GET_VER			0x5F

#define		mCH341_EPP_IO_MAX		( mCH341_PACKET_LENGTH - 1 )
#define		mCH341A_EPP_IO_MAX		0xFF

#define		mCH341A_CMD_IO_ADDR_W	0x00
#define		mCH341A_CMD_IO_ADDR_R	0x80

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
#define	 DEFAULT_TIMEOUT		1000	// 1000ms for USB timeouts
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

#define USB_IN_TRANSFERS 32
/* Need to use many queued transfers for any resemblance of performance (especially on windows)
 * because USB spec says that transfers end on non-full packet and the device sends the 31 reply
 * data bytes to each 32-byte packet with command + 31 bytes of data... */

static struct libusb_device_handle *devHandle = NULL;
static struct libusb_transfer *transfer_out = NULL;
static struct libusb_transfer *transfer_ins[USB_IN_TRANSFERS] = {0};
static unsigned int stored_delay_us = 0;


const struct dev_entry devs_ch341a_spi[] = {
	{0x1A86, 0x5512, OK, "Winchiphead (WCH)", "CH341A"},

	{0},
};

static void print_hex(const void *buf, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		msg_pspew(" %02x", ((uint8_t *)buf)[i]);
		if (i % CH341_PACKET_LENGTH == CH341_PACKET_LENGTH - 1)
			msg_pspew("\n");
	}
}

static void cbGeneric(const char *func, struct libusb_transfer *transfer)
{
	int *transfer_cnt = (int*)transfer->user_data;

	if (transfer->status == LIBUSB_TRANSFER_CANCELLED) {
		/* Silently ACK and exit. */
		*transfer_cnt = -2;
		return;
	}

	if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
		msg_perr("\n%s: error: %s\n", func, libusb_error_name(transfer->status));
		*transfer_cnt = -1;
	} else {
		*transfer_cnt += transfer->actual_length;
	}
}

/* callback for bulk out async transfer */
static void LIBUSB_CALL cbBulkOut(struct libusb_transfer *transfer)
{
	cbGeneric(__func__, transfer);
}

/* callback for bulk in async transfer */
static void LIBUSB_CALL cbBulkIn(struct libusb_transfer *transfer)
{
	cbGeneric(__func__, transfer);
}

static int32_t usbTransferRW(const char *func, unsigned int writecnt, unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr)
{
	if (devHandle == NULL)
		return -1;

	int i;
	int32_t ret = 0;
	int transferred_out = 0;
	int transferred_ins[USB_IN_TRANSFERS] = {0};
	transfer_out->buffer = (uint8_t*)writearr;
	transfer_out->length = writecnt;
	transfer_out->user_data = &transferred_out;

	unsigned int read_que_left = readcnt;
	if (readcnt) {
		int er=0;
		for (i=0;i<USB_IN_TRANSFERS;i++) {
			unsigned int read_now = min(CH341_PACKET_LENGTH -1, read_que_left);
			transfer_ins[i]->buffer = readarr;
			transfer_ins[i]->length = read_now;
			transfer_ins[i]->user_data = &transferred_ins[i];
			readarr += read_now;
			read_que_left -= read_now;
			if (read_now) {
				er = libusb_submit_transfer(transfer_ins[i]);
				if (er) {
					transferred_ins[i] = -2;
					read_que_left = 0; /* Submit no more. */
				}
			} else {
				transferred_ins[i] = -2; /* Mark as already "done". */
			}
		}
		if (er) {
			msg_perr("%s: failed to submit IN transfer: %s\n", func, libusb_error_name(er));
			ret = -1;
			transferred_out = -2; /* Need to signal the OUT as done for the cleanup. */
		}
	}
	if ((writecnt)&&(!ret)) {
		int er = libusb_submit_transfer(transfer_out);
		if (er) {
			msg_perr("%s: failed to submit OUT transfer: %s\n", func, libusb_error_name(er));
			transferred_out = -1;
			ret = -1;
		}
	}
	unsigned int ip = 0; /* The In Packet we expect to be ready next. */
	unsigned int in_done = 0;
	if (!ret) do {	/* TODO: This could made to be even more async if the buffers were non-stack and
		 * state was remembered - atleast by one Out+In transfer in-flight while flashrom runs,
		 * but only when the command doesnt require a read. */
		struct timeval tv = { 0, 100 };
		libusb_handle_events_timeout(NULL, &tv);
		int work;
		if (in_done < readcnt) do {
			work = 0;
			if (transferred_ins[ip] < 0) {
				ret = -1;
				break;
			}
			if (transferred_ins[ip] > 0) {
				unsigned int read_que_now = min( CH341_PACKET_LENGTH -1, read_que_left );
				in_done += transferred_ins[ip];
				if (read_que_now) {
					transferred_ins[ip] = 0;
					transfer_ins[ip]->length = read_que_now;
					transfer_ins[ip]->buffer = readarr;
					libusb_submit_transfer(transfer_ins[ip]);
					read_que_left -= read_que_now;
					readarr += read_que_now;
				} else {
					/* Done with this slot. */
					transferred_ins[ip] = -2;
				}
				ip++;
				if (ip >= USB_IN_TRANSFERS) ip = 0;
				if (in_done >= readcnt) break;
				work = 1;
			}
		} while(work);
		if (ret < 0) break;
		if (transferred_out < 0) {
			ret = -1;
			break;
		}
	} while ((transferred_out < writecnt)||(in_done < readcnt));
	if (ret < 0) {
		msg_perr("%s: Failed to %s %d bytes\n",
			func, (transferred_out < 0) ? "write" : "read", transferred_out < 0 ? writecnt : readcnt);
		/* We must cancel any ongoing requests and wait for them to be cancelled. */
		if ((writecnt)&&(transferred_out == 0)) {
			libusb_cancel_transfer(transfer_out);
		}
		if (readcnt) for (i=0;i<USB_IN_TRANSFERS;i++) {
			if (transferred_ins[i] == 0) libusb_cancel_transfer(transfer_ins[i]);
		}
		int waiting;
		do {
			struct timeval tv = { 0, 100 };
			libusb_handle_events_timeout(NULL, &tv);
			waiting = 0;
			if ((writecnt)&&(transferred_out == 0)) waiting++;
			if (readcnt) for (i=0;i<USB_IN_TRANSFERS;i++) {
				if (transferred_ins[i] == 0) waiting++;
			}
		} while(waiting);
		return -1;
	}
	if (transferred_out) {
		msg_pspew("Wrote %d bytes:\n", transferred_out);
		print_hex(writearr, transferred_out);
		msg_pspew("\n\n");
	}
	if (in_done) {
		msg_pspew("Read %d bytes:\n", in_done);
		print_hex(readarr, in_done);
		msg_pspew("\n\n");
	}
	return transferred_out + in_done;
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

	int32_t ret = usbTransferRW(__func__, sizeof(buf), 0, buf, NULL);
	if (ret < 0) {
		msg_perr("Could not set SPI frequency.\n");
	}
	return ret;
}

/* ch341 requres LSB first, swap the bit order before send and after receive */
static uint8_t swapByte(uint8_t x)
{
	x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
	x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
	x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
	return x;
}

/* The assumed map between UIO command bits, pins on CH341A chip and pins on SPI chip:
 * UIO	CH341A	SPI	CH341A SPI name
 * 0	D0/15	CS/1 	(CS0)
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

static int32_t ch341a_enable_pins(bool enable)
{
	uint8_t buf[] = {
		CH341A_CMD_UIO_STREAM,
		CH341A_CMD_UIO_STM_OUT | 0x37, // CS high (all of them), SCK=0, DOUT*=1
		CH341A_CMD_UIO_STM_DIR | (enable ? 0x3F : 0x00), // Interface output enable / disable
		CH341A_CMD_UIO_STM_END,
	};

	int32_t ret = usbTransferRW(__func__, sizeof(buf), 0, buf, NULL);
	if (ret < 0) {
		msg_perr("Could not %sable output pins.\n", enable ? "en" : "dis");
	}
	return ret;
}

/* De-assert and assert CS in one operation. */
static void ch341pluckCS(uint8_t *ptr)
{
	int i;
	/* This was measured to give 2.25 us deassertion time,
	 * >20x more than needed (100ns) for most SPI chips. */
	int delay_cnt = 2;
	if (stored_delay_us) {
		delay_cnt = (stored_delay_us * 4) / 3;
		stored_delay_us = 0;
	}
	*ptr++ = CH341A_CMD_UIO_STREAM;
	*ptr++ = CH341A_CMD_UIO_STM_OUT | 0x37; /* deasserted */
	for (i=0;i<delay_cnt;i++)
		*ptr++ = CH341A_CMD_UIO_STM_OUT | 0x37; /* "delay" */
	*ptr++ = CH341A_CMD_UIO_STM_OUT | 0x36; /* asserted */
	*ptr++ = CH341A_CMD_UIO_STM_END;
}

void ch341a_delay(unsigned int usecs)
{
	/* Theres space for 28 bytes of 750ns/cycle "delay" in the CS packet, thus max 21us,
	 * but lets not test the boundary, so lets say 20us. */
	if ((usecs + stored_delay_us) > 20) {
		unsigned int inc = 20 - stored_delay_us;
		internal_delay(usecs - inc);
		usecs = inc;
	}
	stored_delay_us += usecs;
}

static int ch341a_spi_spi_send_command(struct flashctx *flash, unsigned int writecnt, unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr)
{
	if (devHandle == NULL)
		return -1;

	/* How many packets ... */
	const size_t packets = ((writecnt+readcnt)+(CH341_PACKET_LENGTH-2)) / (CH341_PACKET_LENGTH-1);

	uint8_t wbuf[packets+1][CH341_PACKET_LENGTH];
	uint8_t rbuf[writecnt + readcnt];
	memset(wbuf[0], 0, CH341_PACKET_LENGTH); // dont want to write stack random to device...

	uint8_t *ptr = wbuf[0];
	/* CS usage is optimized by doing both transitions in one packet.
	 * Final transition to deselected is in the pin disable. */
	ch341pluckCS(ptr);
	unsigned int write_left = writecnt;
	unsigned int read_left = readcnt;
	int p;
	unsigned int i;
	for (p = 0; p < packets; p++) {
		unsigned int write_now = min( CH341_PACKET_LENGTH-1, write_left );
		unsigned int read_now = min ( (CH341_PACKET_LENGTH-1) - write_now, read_left );
		ptr = wbuf[p+1];
		*ptr++ = CH341A_CMD_SPI_STREAM;
		for (i = 0; i < write_now; ++i)
			*ptr++ = swapByte(*writearr++);
		if (read_now) {
			memset(ptr, 0xFF, read_now);
			read_left -= read_now;
		}
		write_left -= write_now;
	}

	int32_t ret = usbTransferRW(__func__, CH341_PACKET_LENGTH + packets + writecnt + readcnt, writecnt + readcnt, wbuf[0], rbuf);
	if (ret < 0)
		return -1;
	for (i = 0; i < readcnt; i++)
		*readarr++ = swapByte(rbuf[writecnt + i]);

	return 0;
}

static const struct spi_master spi_master_ch341a_spi = {
	.type		= SPI_CONTROLLER_CH341A_SPI,
	/* TODO: Flashrom current max is 256. Device was tested on linux to accept atleast 16k  */
	.max_data_read	= 1024, /* Maximum data read size in one go (excluding opcode+address). */
	.max_data_write	= 1024, /* Maximum data write size in one go (excluding opcode+address). */
	.command	= ch341a_spi_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

static int ch341a_spi_shutdown(void *data)
{
	int i;
	if (devHandle == NULL)
		return -1;
	ch341a_enable_pins(false); // disable output pins
	libusb_free_transfer(transfer_out);
	for (i=0;i<USB_IN_TRANSFERS;i++)
		libusb_free_transfer(transfer_ins[i]);
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
	int i;
	int vid = devs_ch341a_spi[0].vendor_id;
	int pid = devs_ch341a_spi[0].device_id;

	if (devHandle != NULL) {
		msg_perr("Call ch341_spi_shutdown before re-configure\n");
		return -1;
	}
	ret = libusb_init(NULL);
	if (ret < 0) {
		msg_perr("Couldnt initialise libusb\n");
		return -1;
	}

	libusb_set_debug(NULL, 3);

	if (!(devHandle = libusb_open_device_with_vid_pid(NULL, vid, pid))) {
		msg_perr("Couldn't open device [%04x:%04x].\n", vid, pid);
		return -1;
	}

	if (!(dev = libusb_get_device(devHandle))) {
		msg_perr("Couldn't get bus number and address.\n");
		goto close_handle;
	}

#if !IS_WINDOWS
	ret = libusb_kernel_driver_active(devHandle, 0);
	if (ret < 0) {
		if (ret != LIBUSB_ERROR_NOT_SUPPORTED) {
			msg_perr("Failed to detach kernel driver: '%s'\n", libusb_error_name(ret));
			goto close_handle;
		}
	} else if (ret == 1) {
		ret = libusb_detach_kernel_driver(devHandle, 0);
		if (ret == LIBUSB_ERROR_NOT_SUPPORTED) {
			msg_pwarn("Could not detach kernel driver. Further accesses will probably fail.\n");
		} else if (ret != 0) {
			msg_perr("Failed to detach kernel driver: '%s'\n", libusb_error_name(ret));
			goto close_handle;
		}
	}
#endif

	ret = libusb_claim_interface(devHandle, 0);
	if (ret != 0) {
		msg_perr("Failed to claim interface 0: '%s'\n", libusb_error_name(ret));
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
	transfer_out = libusb_alloc_transfer(0);
	if (!transfer_out) {
		msg_perr("Failed to alloc libusb OUT transfer\n");
		goto release_interface;
	}
	for (i=0;i<USB_IN_TRANSFERS;i++) {
		transfer_ins[i] = libusb_alloc_transfer(0);
		if (!transfer_ins[i]) {
			int n;
			msg_perr("Failed to alloc libusb IN transfer %d\n", i);
			for  (n=0;n<i;n++)
				libusb_free_transfer(transfer_ins[n]);
			goto dealloc_out;
		}
	}
	/* We use these helpers but dont fill the actual buffer yet. */
	libusb_fill_bulk_transfer(transfer_out, devHandle, BULK_WRITE_ENDPOINT, 0, 0, cbBulkOut, 0, DEFAULT_TIMEOUT);
	for (i=0;i<USB_IN_TRANSFERS;i++)
		libusb_fill_bulk_transfer(transfer_ins[i], devHandle, BULK_READ_ENDPOINT, 0, 0, cbBulkIn, 0, DEFAULT_TIMEOUT);

	ret = ch341SetStream(CH341A_STM_I2C_100K) | ch341a_enable_pins(true);
	if (ret < 0)
		goto dealloc_ins;

	register_shutdown(ch341a_spi_shutdown, NULL);
	register_spi_master(&spi_master_ch341a_spi);

	return 0;

dealloc_ins:
	for (i=0;i<USB_IN_TRANSFERS;i++)
		libusb_free_transfer(transfer_ins[i]);
dealloc_out:
	libusb_free_transfer(transfer_out);
release_interface:
	libusb_release_interface(devHandle, 0);
close_handle:
	libusb_close(devHandle);
	devHandle = NULL;
	return -1;
}
