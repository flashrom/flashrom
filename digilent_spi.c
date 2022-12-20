/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2018 Lubomir Rintel <lkundrak@v3.sk>
 *
 * Based on ft2232_spi.c:
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/*
 * The reverse-engineered protocol description was obtained from the
 * iceBurn project <https://github.com/davidcarne/iceBurn> by
 * David Carne <davidcarne@gmail.com>.
 */

#include <stdlib.h>
#include <string.h>
#include <libusb.h>
#include "programmer.h"

/* This is pretty much arbitrarily chosen. After one second without a
 * response we can be pretty sure we're not going to succeed. */
#define USB_TIMEOUT		1000

#define	CMD_WRITE_EP		0x01
#define	CMD_READ_EP		0x82
#define	DATA_WRITE_EP		0x03
#define	DATA_READ_EP		0x84

struct digilent_spi_data {
	struct libusb_device_handle *handle;
	bool reset_board;
};

#define DIGILENT_VID		0x1443
#define DIGILENT_JTAG_PID	0x0007

static const struct dev_entry devs_digilent_spi[] = {
	{ DIGILENT_VID, DIGILENT_JTAG_PID, OK, "Digilent", "Development board JTAG" },
	{ 0 },
};

/* Control endpoint commands. */
enum {
	GET_BOARD_TYPE		= 0xe2,
	GET_BOARD_SERIAL	= 0xe4,
};

/* Command bulk endpoint command groups. */
enum {
	CMD_GPIO		= 0x03,
	CMD_BOARD		= 0x04,
	CMD_SPI			= 0x06,
};

/* GPIO subcommands. */
enum {
	CMD_GPIO_OPEN		= 0x00,
	CMD_GPIO_CLOSE		= 0x01,
	CMD_GPIO_SET_DIR	= 0x04,
	CMD_GPIO_SET_VAL	= 0x06,
};

/* Board subcommands. */
enum {
	CMD_BOARD_OPEN		= 0x00,
	CMD_BOARD_CLOSE		= 0x01,
	CMD_BOARD_SET_REG	= 0x04,
	CMD_BOARD_GET_REG	= 0x05,
	CMD_BOARD_PL_STAT	= 0x85,
};

/* SPI subcommands. */
enum {
	CMD_SPI_OPEN		= 0x00,
	CMD_SPI_CLOSE		= 0x01,
	CMD_SPI_SET_SPEED	= 0x03,
	CMD_SPI_SET_MODE	= 0x05,
	CMD_SPI_SET_CS		= 0x06,
	CMD_SPI_START_IO	= 0x07,
	CMD_SPI_TX_END		= 0x87,
};

static int do_command(uint8_t *req, int req_len, uint8_t *res, int res_len, struct libusb_device_handle *handle)
{
	int tx_len = 0;
	int ret;

	req[0] = req_len - 1;
	ret = libusb_bulk_transfer(handle, CMD_WRITE_EP, req, req_len, &tx_len, USB_TIMEOUT);
	if (ret) {
		msg_perr("Failed to issue a command: '%s'\n", libusb_error_name(ret));
		return -1;
	}

	if (tx_len != req_len) {
		msg_perr("Short write issuing a command\n");
		return -1;
	}

	ret = libusb_bulk_transfer(handle, CMD_READ_EP, res, res_len, &tx_len, USB_TIMEOUT);
	if (ret) {
		msg_perr("Failed to get a response: '%s'\n", libusb_error_name(ret));
		return -1;
	}

	if (tx_len != res_len) {
		msg_perr("Short read getting a response\n");
		return -1;
	}

	if (res[0] != res_len -1) {
		msg_perr("Response indicates incorrect length.\n");
		return -1;
	}

	return 0;
}

static int gpio_open(struct libusb_device_handle *handle)
{
	uint8_t req[] = { 0x00, CMD_GPIO, CMD_GPIO_OPEN, 0x00 };
	uint8_t res[2];

	return do_command(req, sizeof(req), res, sizeof(res), handle);
}

static int gpio_set_dir(uint8_t direction, struct libusb_device_handle *handle)
{
	uint8_t req[] = { 0x00, CMD_GPIO, CMD_GPIO_SET_DIR, 0x00,
			  direction, 0x00, 0x00, 0x00 };
	uint8_t res[6];

	return do_command(req, sizeof(req), res, sizeof(res), handle);
}

static int gpio_set_value(uint8_t value, struct libusb_device_handle *handle)
{
	uint8_t req[] = { 0x00, CMD_GPIO, CMD_GPIO_SET_VAL, 0x00,
			  value, 0x00, 0x00, 0x00 };
	uint8_t res[2];

	return do_command(req, sizeof(req), res, sizeof(res), handle);
}

static int spi_open(struct libusb_device_handle *handle)
{
	uint8_t req[] = { 0x00, CMD_SPI, CMD_SPI_OPEN, 0x00 };
	uint8_t res[2];

	return do_command(req, sizeof(req), res, sizeof(res), handle);
}

static int spi_set_speed(uint32_t speed, struct libusb_device_handle *handle)
{
	uint8_t req[] = { 0x00, CMD_SPI, CMD_SPI_SET_SPEED, 0x00,
			  (speed) & 0xff,
			  (speed >> 8) & 0xff,
			  (speed >> 16) & 0xff,
			  (speed >> 24) & 0xff };
	uint8_t res[6];
	uint32_t real_speed;
	int ret;

	ret = do_command(req, sizeof(req), res, sizeof(res), handle);
	if (ret)
		return ret;

	real_speed = (res[5] << 24) | (res[4] << 16) | (res[3] << 8) | res[2];
	if (real_speed != speed)
		msg_pwarn("SPI speed set to %d instead of %d\n", real_speed, speed);

	return 0;
}

static int spi_set_mode(uint8_t mode, struct libusb_device_handle *handle)
{
	uint8_t req[] = { 0x00, CMD_SPI, CMD_SPI_SET_MODE, 0x00, mode };
	uint8_t res[2];

	return do_command(req, sizeof(req), res, sizeof(res), handle);
}

static int spi_set_cs(uint8_t cs, struct libusb_device_handle *handle)
{
	uint8_t req[] = { 0x00, CMD_SPI, CMD_SPI_SET_CS, 0x00, cs };
	uint8_t res[2];

	return do_command(req, sizeof(req), res, sizeof(res), handle);
}

static int spi_start_io(uint8_t read_follows, uint32_t write_len, struct libusb_device_handle *handle)
{
	uint8_t req[] = { 0x00, CMD_SPI, CMD_SPI_START_IO, 0x00,
			  0x00, 0x00, /* meaning unknown */
			  read_follows,
			  (write_len) & 0xff,
			  (write_len >> 8) & 0xff,
			  (write_len >> 16) & 0xff,
			  (write_len >> 24) & 0xff };
	uint8_t res[2];

	return do_command(req, sizeof(req), res, sizeof(res), handle);
}

static int spi_tx_end(uint8_t read_follows, uint32_t tx_len, struct libusb_device_handle *handle)
{
	uint8_t req[] = { 0x00, CMD_SPI, CMD_SPI_TX_END, 0x00 };
	uint8_t res[read_follows ? 10 : 6];
	int ret;
	uint32_t count;

	ret = do_command(req, sizeof(req), res, sizeof(res), handle);
	if (ret != 0)
		return ret;

	if ((res[1] & 0x80) == 0) {
		msg_perr("%s: response missing a write count\n", __func__);
		return -1;
	}

	count = res[2] | (res[3] << 8) | (res[4] << 16) | res[5] << 24;
	if (count != tx_len) {
		msg_perr("%s: wrote only %d bytes instead of %d\n", __func__, count, tx_len);
		return -1;
	}

	if (read_follows) {
		if ((res[1] & 0x40) == 0) {
			msg_perr("%s: response missing a read count\n", __func__);
			return -1;
		}

		count = res[6] | (res[7] << 8) | (res[8] << 16) | res[9] << 24;
		if (count != tx_len) {
			msg_perr("%s: read only %d bytes instead of %d\n", __func__, count, tx_len);
			return -1;
		}
	}

	return 0;
}

static int digilent_spi_send_command(const struct flashctx *flash, unsigned int writecnt, unsigned int readcnt,
				     const unsigned char *writearr, unsigned char *readarr)
{
	int ret;
	int len = writecnt + readcnt;
	int tx_len = 0;
	uint8_t buf[len];
	uint8_t read_follows = readcnt > 0 ? 1 : 0;
	struct digilent_spi_data *digilent_data = flash->mst->spi.data;

	memcpy(buf, writearr, writecnt);
	memset(buf + writecnt, 0xff, readcnt);

	ret = spi_set_cs(0, digilent_data->handle);
	if (ret != 0)
		return ret;

	ret = spi_start_io(read_follows, writecnt, digilent_data->handle);
	if (ret != 0)
		return ret;

	ret = libusb_bulk_transfer(digilent_data->handle, DATA_WRITE_EP, buf, len, &tx_len, USB_TIMEOUT);
	if (ret != 0) {
		msg_perr("%s: failed to write data: '%s'\n", __func__, libusb_error_name(ret));
		return -1;
	}
	if (tx_len != len) {
		msg_perr("%s: short write\n", __func__);
		return -1;
	}

	if (read_follows) {
		ret = libusb_bulk_transfer(digilent_data->handle, DATA_READ_EP, buf, len, &tx_len, USB_TIMEOUT);
		if (ret != 0) {
			msg_perr("%s: failed to read data: '%s'\n", __func__, libusb_error_name(ret));
			return -1;
		}
		if (tx_len != len) {
			msg_perr("%s: short read\n", __func__);
			return -1;
		}
	}

	ret = spi_tx_end(read_follows, len, digilent_data->handle);
	if (ret != 0)
		return ret;

	ret = spi_set_cs(1, digilent_data->handle);
	if (ret != 0)
		return ret;

	memcpy(readarr, &buf[writecnt], readcnt);

	return 0;
}

static int digilent_spi_shutdown(void *data)
{
	struct digilent_spi_data *digilent_data = data;

	if (digilent_data->reset_board)
		gpio_set_dir(0, digilent_data->handle);

	libusb_close(digilent_data->handle);

	free(data);
	return 0;
}

static const struct spi_master spi_master_digilent_spi = {
	.features	= SPI_MASTER_4BA,
	.max_data_read	= 252,
	.max_data_write	= 252,
	.command	= digilent_spi_send_command,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.shutdown	= digilent_spi_shutdown,
};

static bool default_reset(struct libusb_device_handle *handle)
{
	char board[17];

	libusb_control_transfer(handle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
	                        GET_BOARD_TYPE, 0, 0,
	                        (unsigned char *)board, sizeof(board) - 1, USB_TIMEOUT);
	board[sizeof(board) -1] = '\0';

	if (strcmp(board, "iCE40") == 0)
		return true;

	msg_pwarn("%s: unknown board '%s' not attempting a reset. "
	          "Override with '-p digilent_spi=reset=1'.\n", __func__, board);
	return false;
}

struct digilent_spispeeds {
        const char *const name;
        const int speed;
};

static const struct digilent_spispeeds spispeeds[] = {
	{ "4M",		4000000 },
	{ "2M",		2000000 },
	{ "1M",		1000000 },
	{ "500k",	500000 },
	{ "250k",	250000 },
	{ "125k",	125000 },
	{ "62.5k",	62500 },
	{ NULL,		0 },
};

static int digilent_spi_init(const struct programmer_cfg *cfg)
{
	char *param_str;
	uint32_t speed_hz = spispeeds[0].speed;
	int i;
	struct libusb_device_handle *handle = NULL;
	bool reset_board;

	int32_t ret = libusb_init(NULL);
	if (ret < 0) {
		msg_perr("%s: couldn't initialize libusb!\n", __func__);
		return -1;
	}

#if LIBUSB_API_VERSION < 0x01000106
	libusb_set_debug(NULL, 3);
#else
	libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
#endif

	uint16_t vid = devs_digilent_spi[0].vendor_id;
	uint16_t pid = devs_digilent_spi[0].device_id;
	handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (handle == NULL) {
		msg_perr("%s: couldn't open device %04x:%04x.\n", __func__, vid, pid);
		return -1;
	}

	ret = libusb_claim_interface(handle, 0);
	if (ret != 0) {
		msg_perr("%s: failed to claim interface 0: '%s'\n", __func__, libusb_error_name(ret));
		goto close_handle;
	}

	param_str = extract_programmer_param_str(cfg, "spispeed");
	if (param_str) {
		for (i = 0; spispeeds[i].name; ++i) {
			if (!strcasecmp(spispeeds[i].name, param_str)) {
				speed_hz = spispeeds[i].speed;
				break;
			}
		}
		if (!spispeeds[i].name) {
			msg_perr("Error: Invalid spispeed value: '%s'.\n", param_str);
			free(param_str);
			goto close_handle;
		}
		free(param_str);
	}

	param_str = extract_programmer_param_str(cfg, "reset");
	if (param_str && strlen(param_str))
		reset_board = (param_str[0] == '1');
	else
		reset_board = default_reset(handle);
	free(param_str);


	if (reset_board) {
		if (gpio_open(handle) != 0)
			goto close_handle;
		if (gpio_set_dir(1, handle) != 0)
			goto close_handle;
		if (gpio_set_value(0, handle) != 0)
			goto close_handle;
	}

	if (spi_open(handle) != 0)
		goto close_handle;
	if (spi_set_speed(speed_hz, handle) != 0)
		goto close_handle;
	if (spi_set_mode(0x00, handle) != 0)
		goto close_handle;

	struct digilent_spi_data *digilent_data = calloc(1, sizeof(*digilent_data));
	if (!digilent_data) {
		msg_perr("Unable to allocate space for SPI master data\n");
		goto close_handle;
	}
	digilent_data->reset_board = reset_board;
	digilent_data->handle = handle;

	return register_spi_master(&spi_master_digilent_spi, digilent_data);

close_handle:
	libusb_close(handle);
	return -1;
}

const struct programmer_entry programmer_digilent_spi = {
	.name			= "digilent_spi",
	.type			= USB,
	.devs.dev		= devs_digilent_spi,
	.init			= digilent_spi_init,
};
