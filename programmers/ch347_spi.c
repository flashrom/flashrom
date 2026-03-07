/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2022 Nicholas Chin <nic.c3.14@gmail.com>
 */

#include "platform/string.h"
#include <stdlib.h>
#include <libusb.h>
#include "platform/endian.h"
#include "programmer.h"
#include "flash.h"
#include "helpers.h"
#include "log.h"
#include "usb_device.h"

#define CH347_CMD_SPI_SET_CFG	0xC0
#define CH347_CMD_SPI_CS_CTRL	0xC1
#define CH347_CMD_SPI_OUT_IN	0xC2
#define CH347_CMD_SPI_IN	0xC3
#define CH347_CMD_SPI_OUT	0xC4
#define CH347_CMD_SPI_GET_CFG	0xCA

#define CH347_CS_ASSERT		0x00
#define CH347_CS_DEASSERT	0x40
#define CH347_CS_CHANGE		0x80
#define CH347_CS_IGNORE		0x00

#define WRITE_EP	0x06
#define READ_EP 	0x86

#define CH347T_IFACE 2
#define CH347F_IFACE 4

/* The USB descriptor says the max transfer size is 512 bytes, but the
 * vendor driver only seems to transfer a maximum of 510 bytes at once,
 * leaving 507 bytes for data as the command + length take up 3 bytes
 */
#define CH347_PACKET_SIZE 510
#define CH347_MAX_DATA_LEN (CH347_PACKET_SIZE - 3)

struct ch347_spi_data {
	struct libusb_device_handle *handle;
	int interface;
};

struct device_speeds {
	const char *name;
	const int divisor;
};

/* TODO: Add support for HID mode */
static const struct dev_entry devs_ch347_spi[] = {
	{0x1A86, 0x55DB, OK, "QinHeng Electronics", "USB To UART+SPI+I2C"},   /* CH347T */
	{0x1A86, 0x55DE, OK, "QinHeng Electronics", "USB To UART+SPI+I2C"},   /* CH347F */
	{0}
};

static int ch347_interface[] = {
	CH347T_IFACE,
	CH347F_IFACE,
};

static const struct device_speeds spispeeds[] = {
	{"60M",     0},
	{"30M",     1},
	{"15M",     2},
	{"7.5M",    3},
	{"3.75M",   4},
	{"1.875M",  5},
	{"937.5K",  6},
	{"468.75K", 7},
	{NULL,      0}
};

static int ch347_spi_shutdown(void *data)
{
	struct ch347_spi_data *ch347_data = data;
	int spi_interface = ch347_data->interface;
	libusb_release_interface(ch347_data->handle, spi_interface);
	libusb_attach_kernel_driver(ch347_data->handle, spi_interface);
	libusb_close(ch347_data->handle);
	libusb_exit(NULL);
	free(data);
	return 0;
}

static int ch347_cs_control(struct ch347_spi_data *ch347_data, uint8_t cs1, uint8_t cs2)
{
	uint8_t cmd[13] = {
		[0] = CH347_CMD_SPI_CS_CTRL,
		/* payload length, uint16 LSB: 10 */
		[1] = 10,
		[3] = cs1,
		[8] = cs2
	};

	int32_t ret = libusb_bulk_transfer(ch347_data->handle, WRITE_EP, cmd, sizeof(cmd), NULL, 1000);
	if (ret < 0) {
		msg_perr("Could not change CS!\n");
		return -1;
	}
	return 0;
}


static int ch347_write(struct ch347_spi_data *ch347_data, unsigned int writecnt, const uint8_t *writearr)
{
	unsigned int data_len;
	int packet_len;
	int transferred;
	int ret;
	uint8_t resp_buf[4] = {0};
	uint8_t buffer[CH347_PACKET_SIZE] = {0};
	unsigned int bytes_written = 0;

	while (bytes_written < writecnt) {
		data_len = min(CH347_MAX_DATA_LEN, writecnt - bytes_written );
		packet_len = data_len + 3;

		buffer[0] = CH347_CMD_SPI_OUT;
		buffer[1] = (data_len) & 0xFF;
		buffer[2] = ((data_len) & 0xFF00) >> 8;
		memcpy(buffer + 3, writearr + bytes_written, data_len);

		ret = libusb_bulk_transfer(ch347_data->handle, WRITE_EP, buffer, packet_len, &transferred, 1000);
		if (ret < 0 || transferred != packet_len) {
			msg_perr("Could not send write command\n");
			return -1;
		}

		ret = libusb_bulk_transfer(ch347_data->handle, READ_EP, resp_buf, sizeof(resp_buf), NULL, 1000);
		if (ret < 0) {
			msg_perr("Could not receive write command response\n");
			return -1;
		}
		bytes_written += data_len;
	}
	return 0;
}

static int ch347_read(struct ch347_spi_data *ch347_data, unsigned int readcnt, uint8_t *readarr)
{
	uint8_t *read_ptr = readarr;
	int ret;
	int transferred;
	unsigned int bytes_read = 0;
	uint8_t buffer[CH347_PACKET_SIZE] = {0};
	uint8_t command_buf[7] = {
		[0] = CH347_CMD_SPI_IN,
		[1] = 4,
		[2] = 0,
		[3] = readcnt & 0xFF,
		[4] = (readcnt & 0xFF00) >> 8,
		[5] = (readcnt & 0xFF0000) >> 16,
		[6] = (readcnt & 0xFF000000) >> 24
	};

	ret = libusb_bulk_transfer(ch347_data->handle, WRITE_EP, command_buf, sizeof(command_buf), &transferred, 1000);
	if (ret < 0 || transferred != sizeof(command_buf)) {
		msg_perr("Could not send read command\n");
		return -1;
	}

	while (bytes_read < readcnt) {
		ret = libusb_bulk_transfer(ch347_data->handle, READ_EP, buffer, CH347_PACKET_SIZE, &transferred, 1000);
		if (ret < 0) {
			msg_perr("Could not read data\n");
			return -1;
		}
		if (transferred > CH347_PACKET_SIZE) {
			msg_perr("libusb bug: bytes received overflowed buffer\n");
			return -1;
		}
		/* Response: u8 command, u16 data length, then the data that was read */
		if (transferred < 3) {
			msg_perr("CH347 returned an invalid response to read command\n");
			return -1;
		}
		int ch347_data_length = read_le16(buffer, 1);
		if (transferred - 3 < ch347_data_length) {
			msg_perr("CH347 returned less data than data length header indicates\n");
			return -1;
		}
		bytes_read += ch347_data_length;
		if (bytes_read > readcnt) {
			msg_perr("CH347 returned more bytes than requested\n");
			return -1;
		}
		memcpy(read_ptr, buffer + 3, ch347_data_length);
		read_ptr += ch347_data_length;
	}
	return 0;
}

static int ch347_spi_send_command(const struct flashctx *flash, unsigned int writecnt,
		unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr)
{
	struct ch347_spi_data *ch347_data = flash->mst->spi.data;
	int ret = 0;

	ch347_cs_control(ch347_data, CH347_CS_ASSERT | CH347_CS_CHANGE, CH347_CS_IGNORE);
	if (writecnt) {
		ret = ch347_write(ch347_data, writecnt, writearr);
		if (ret < 0) {
			msg_perr("CH347 write error\n");
			return -1;
		}
	}
	if (readcnt) {
		ret = ch347_read(ch347_data, readcnt, readarr);
		if (ret < 0) {
			msg_perr("CH347 read error\n");
			return -1;
		}
	}
	ch347_cs_control(ch347_data, CH347_CS_DEASSERT | CH347_CS_CHANGE, CH347_CS_IGNORE);

	return 0;
}

/*
 * CH347 SPI config packet (0xC0), 29 bytes: a 3-byte header followed
 * by a 26-byte payload.
 *
 * The payload maps to the vendor StreamHwCfgS struct, which embeds
 * SPI_InitTypeDef, a set of u16 LE fields corresponding to the
 * CH32V SPI Control Register 1 (SPI_CTLR1). All u16 fields are
 * OR'd together by the firmware to produce the final register value.
 *
 * Offset  Size  Field                    Description
 * ------  ----  -----                    -----------
 *  0      u8    Command                  0xC0 (SPI_SET_CFG)
 *  1-2    u16   Payload length           26
 *  3-4    u16   SPI_Direction            Datamode (0=2-line full-duplex)
 *  5-6    u16   SPI_Mode                 Master/Slave (0x0104=master)
 *  7-8    u16   SPI_DataSize             Frame size (0=8-bit)
 *  9-10   u16   SPI_CPOL                 Clock polarity (0=idle low, 0x0002=idle high)
 * 11-12   u16   SPI_CPHA                 Clock phase (0=leading, 0x0001=trailing)
 * 13-14   u16   SPI_NSS                  CS management (0=HW, 0x0200=SW)
 * 15-16   u16   SPI_BaudRatePrescaler    Clock divisor (prescaler*8, 0=60M..7=468.75K)
 * 17-18   u16   SPI_FirstBit             Bit order (0=MSB, 0x0080=LSB)
 * 19-20   u16   SPI_CRCPolynomial        CRC polynomial (default 0x0007)
 * 21-22   u16   SpiWriteReadInterval     R/W interval (us)
 *    23   u8    SpiOutDefaultData        Default MOSI byte during reads
 *    24   u8    OtherCfg                 Bit7: CS1 polarity, Bit6: CS2 polarity
 * 25-28   u8[4] Reserved
 *
 * Research started in: https://github.com/nic3-14159/CH347-Research
 * Reference: WCH vendor driver ch347_lib.h (StreamHwCfgS / SPI_InitTypeDef)
 */
static int32_t ch347_spi_config(struct ch347_spi_data *ch347_data, uint8_t divisor)
{
	int32_t ret;
	int transferred = 0;
	uint8_t buff[29] = {
		[0] = CH347_CMD_SPI_SET_CFG,
		[1] = (sizeof(buff) - 3) & 0xFF,
		[2] = ((sizeof(buff) - 3) & 0xFF00) >> 8,
		[5] = 4,
		[6] = 1,
		[14] = 2,
		[15] = (divisor & 0x7) << 3,
		[19] = 7,
	};

	ret = libusb_bulk_transfer(ch347_data->handle, WRITE_EP, buff, sizeof(buff), NULL, 1000);
	if (ret < 0) {
		msg_perr("Could not configure SPI interface\n");
		return ret;
	}

	/* Read the response into a full-size buffer to drain any extra
	 * bytes from firmware variants that may echo back the entire
	 * config. The expected ACK is 4 bytes: cmd, length(2), status.
	 */
	uint8_t rbuf[sizeof(buff)] = {0};
	ret = libusb_bulk_transfer(ch347_data->handle, READ_EP, rbuf, sizeof(rbuf), &transferred, 1000);
	if (ret < 0) {
		msg_perr("Could not receive configure SPI command response\n");
		return ret;
	}

	if (transferred < 4 || rbuf[0] != CH347_CMD_SPI_SET_CFG || rbuf[3] != 0) {
		msg_perr("CH347 SPI config failed (response: %d bytes, cmd=0x%02x, status=0x%02x)\n",
			transferred, rbuf[0], transferred >= 4 ? rbuf[3] : 0xff);
		return -1;
	}

	return 0;
}

static const struct spi_master spi_master_ch347_spi = {
	.features	= SPI_MASTER_4BA,
	.max_data_read	= MAX_DATA_READ_UNLIMITED,
	.max_data_write	= MAX_DATA_WRITE_UNLIMITED,
	.command	= ch347_spi_send_command,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.write_aai	= default_spi_write_aai,
	.shutdown	= ch347_spi_shutdown,
};

/* Largely copied from ch341a_spi.c */
static int ch347_spi_init(const struct programmer_cfg *cfg)
{
	char *arg;
	uint16_t vid = devs_ch347_spi[0].vendor_id;
	uint16_t pid = 0;
	int index = 0;
	int speed_index;
	struct ch347_spi_data *ch347_data = calloc(1, sizeof(*ch347_data));
	if (!ch347_data) {
		msg_perr("Could not allocate space for SPI data\n");
		return 1;
	}

	int32_t ret = libusb_init(NULL);
	if (ret < 0) {
		msg_perr("Could not initialize libusb!\n");
		free(ch347_data);
		return 1;
	}
	/* Enable information, warning, and error messages (only). */
#if LIBUSB_API_VERSION < 0x01000106
	libusb_set_debug(NULL, 3);
#else
	libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
#endif
	while (devs_ch347_spi[index].vendor_id != 0) {
		vid = devs_ch347_spi[index].vendor_id;
		pid = devs_ch347_spi[index].device_id;
		ch347_data->handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
		if (ch347_data->handle) {
			ch347_data->interface = ch347_interface[index];
			break;
		}
		index++;
	}
	if (!ch347_data->handle) {
		msg_perr("Couldn't find CH347.\n");
		free(ch347_data);
		return 1;
	}

	if (usb_dev_claim_and_describe(ch347_data->handle, ch347_data->interface) != 0)
		goto error_exit;

	/* set CH347 clock division */
	speed_index = 2; /* default: 15MHz */
	arg = extract_programmer_param_str(cfg, "spispeed");
	if (arg) {
		for (speed_index = 0; spispeeds[speed_index].name; speed_index++) {
			if (!strncasecmp(spispeeds[speed_index].name, arg, strlen(spispeeds[speed_index].name))) {
				break;
			}
		}
		if (!spispeeds[speed_index].name) {
			msg_pwarn("Unknown spispeed value '%s', using default 15MHz.\n", arg);
			speed_index = 2;
		}
	}
	free(arg);
	if (ch347_spi_config(ch347_data, spispeeds[speed_index].divisor) < 0) {
		goto error_exit;
	} else {
		msg_pinfo("CH347 SPI clock set to %sHz.\n", spispeeds[speed_index].name);
	}

	return register_spi_master(&spi_master_ch347_spi, ch347_data);

error_exit:
	ch347_spi_shutdown(ch347_data);
	return 1;
}

const struct programmer_entry programmer_ch347_spi = {
	.name		= "ch347_spi",
	.type		= USB,
	.devs.dev	= devs_ch347_spi,
	.init		= ch347_spi_init,
};
