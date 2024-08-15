/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2019 Miklós Márton martonmiklosqdev@gmail.com
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
 */

/*
 * Driver for programming SPI flash chips using the SPI port
 * of the STMicroelectronics's STLINK-V3 programmer/debugger.
 *
 * The implementation is inspired by the ST's STLINK-V3-BRIDGE C++ API:
 * https://www.st.com/en/development-tools/stlink-v3-bridge.html
 */

#include "flash.h"
#include "programmer.h"
#include "spi.h"

#include <libusb.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum fw_version_check_result {
	FW_VERSION_OK,
	FW_VERSION_OLD,
};

enum spi_prescaler {
	SPI_BAUDRATEPRESCALER_2 = 0,
	SPI_BAUDRATEPRESCALER_4 = 1,
	SPI_BAUDRATEPRESCALER_8 = 2,
	SPI_BAUDRATEPRESCALER_16 = 3,
	SPI_BAUDRATEPRESCALER_32 = 4,
	SPI_BAUDRATEPRESCALER_64 = 5,
	SPI_BAUDRATEPRESCALER_128 = 6,
	SPI_BAUDRATEPRESCALER_256 = 7
};

enum spi_dir {
	SPI_DIRECTION_2LINES_FULLDUPLEX = 0,
	SPI_DIRECTION_2LINES_RXONLY = 1,
	SPI_DIRECTION_1LINE_RX = 2,
	SPI_DIRECTION_1LINE_TX = 3
};

enum spi_mode {
	SPI_MODE_SLAVE = 0,
	SPI_MODE_MASTER = 1
};

enum spi_datasize {
	SPI_DATASIZE_16B = 0,
	SPI_DATASIZE_8B = 1
};

enum spi_cpol {
	SPI_CPOL_LOW = 0,
	SPI_CPOL_HIGH = 1
};

enum spi_cpha {
	SPI_CPHA_1EDGE = 0,
	SPI_CPHA_2EDGE = 1
};

enum spi_firstbit {
	SPI_FIRSTBIT_LSB = 0,
	SPI_FIRSTBIT_MSB = 1
};

// ST calls the Chip select (CS) NSS == Negated Slave Select
enum spi_nss {
	SPI_NSS_SOFT = 0,
	SPI_NSS_HARD = 1
};

enum spi_nss_level {
	SPI_NSS_LOW = 0,
	SPI_NSS_HIGH = 1
};

#define ST_GETVERSION_EXT					0xFB

#define STLINK_BRIDGE_COMMAND					0xFC
#define STLINK_BRIDGE_CLOSE					0x01
#define STLINK_BRIDGE_GET_RWCMD_STATUS				0x02
#define STLINK_BRIDGE_GET_CLOCK					0x03
#define STLINK_BRIDGE_INIT_SPI					0x20
#define STLINK_BRIDGE_WRITE_SPI					0x21
#define STLINK_BRIDGE_READ_SPI					0x22
#define STLINK_BRIDGE_CS_SPI					0x23

#define STLINK_BRIDGE_SPI_ERROR					0x02

#define STLINK_SPI_COM						0x02

#define STLINK_EP_OUT						0x06
#define STLINK_EP_IN						0x86

#define FIRST_COMPATIBLE_BRIDGE_FW_VERSION			3

#define USB_TIMEOUT_IN_MS					5000

static const struct dev_entry devs_stlinkv3_spi[] = {
	{0x0483, 0x374E, BAD, "STMicroelectronics", "STLINK-V3E"},
	{0x0483, 0x374F, OK, "STMicroelectronics", "STLINK-V3S"},
	{0x0483, 0x3753, OK, "STMicroelectronics", "STLINK-V3 dual VCP"},
	{0x0483, 0x3754, NT, "STMicroelectronics", "STLINK-V3 no MSD"},
	{0}
};

struct stlinkv3_spi_data {
	struct libusb_context *usb_ctx;
	libusb_device_handle *handle;
};

static int stlinkv3_command(uint8_t *command, size_t command_length,
		     uint8_t *answer, size_t answer_length, const char *command_name,
		     libusb_device_handle *stlinkv3_handle)
{
	int actual_length = 0;
	int rc = libusb_bulk_transfer(stlinkv3_handle, STLINK_EP_OUT,
				  command, command_length,
				  &actual_length, USB_TIMEOUT_IN_MS);
	if (rc != LIBUSB_TRANSFER_COMPLETED || (size_t)actual_length != command_length) {
		msg_perr("Failed to issue the %s command: '%s'\n",
			 command_name,
			 libusb_error_name(rc));
		return -1;
	}

	rc = libusb_bulk_transfer(stlinkv3_handle, STLINK_EP_IN,
				  answer, answer_length,
				  &actual_length, USB_TIMEOUT_IN_MS);
	if (rc != LIBUSB_TRANSFER_COMPLETED || (size_t)actual_length != answer_length) {
		msg_perr("Failed to get %s answer: '%s'\n",
			 command_name,
			 libusb_error_name(rc));
		return -1;
	}
	return 0;
}

/**
 * @param[out] bridge_input_clk Current input frequency in kHz of the given com.
 */
static int stlinkv3_get_clk(uint32_t *bridge_input_clk, libusb_device_handle *stlinkv3_handle)
{
	uint8_t command[16] = { 0 };
	uint8_t answer[12];

	if (bridge_input_clk == NULL)
		return -1;

	command[0] = STLINK_BRIDGE_COMMAND;
	command[1] = STLINK_BRIDGE_GET_CLOCK;
	command[2] = STLINK_SPI_COM;

	if (stlinkv3_command(command, sizeof(command),
				answer, sizeof(answer),
				"STLINK_BRIDGE_GET_CLOCK",
				stlinkv3_handle) != 0)
		return -1;

	*bridge_input_clk = (uint32_t)answer[4]
			  | (uint32_t)answer[5]<<8
			  | (uint32_t)answer[6]<<16
			  | (uint32_t)answer[7]<<24;
	return 0;

}

static int stlinkv3_spi_calc_prescaler(uint16_t reqested_freq_in_kHz,
				       enum spi_prescaler *prescaler,
				       uint16_t *calculated_freq_in_kHz,
				       libusb_device_handle *stlinkv3_handle)
{
	uint32_t bridge_clk_in_kHz;
	uint32_t calculated_prescaler = 1;
	uint16_t prescaler_value;

	if (stlinkv3_get_clk(&bridge_clk_in_kHz, stlinkv3_handle))
		return -1;

	calculated_prescaler  = bridge_clk_in_kHz/reqested_freq_in_kHz;
	// Apply a smaller frequency if not exact
	if (calculated_prescaler  <= 2) {
		*prescaler = SPI_BAUDRATEPRESCALER_2;
		prescaler_value = 2;
	} else if (calculated_prescaler  <= 4) {
		*prescaler = SPI_BAUDRATEPRESCALER_4;
		prescaler_value = 4;
	} else if (calculated_prescaler  <= 8) {
		*prescaler = SPI_BAUDRATEPRESCALER_8;
		prescaler_value = 8;
	} else if (calculated_prescaler  <= 16) {
		*prescaler = SPI_BAUDRATEPRESCALER_16;
		prescaler_value = 16;
	} else if (calculated_prescaler  <= 32) {
		*prescaler = SPI_BAUDRATEPRESCALER_32;
		prescaler_value = 32;
	} else if (calculated_prescaler  <= 64) {
		*prescaler = SPI_BAUDRATEPRESCALER_64;
		prescaler_value = 64;
	} else if (calculated_prescaler  <= 128) {
		*prescaler = SPI_BAUDRATEPRESCALER_128;
		prescaler_value = 128;
	} else if (calculated_prescaler  <= 256) {
		*prescaler = SPI_BAUDRATEPRESCALER_256;
		prescaler_value = 256;
	} else {
		// smaller frequency not possible
		*prescaler = SPI_BAUDRATEPRESCALER_256;
		prescaler_value = 256;
	}

	*calculated_freq_in_kHz = bridge_clk_in_kHz / prescaler_value;

	return 0;
}

static int stlinkv3_check_version(enum fw_version_check_result *result, libusb_device_handle *stlinkv3_handle)
{
	uint8_t answer[12];
	uint8_t command[16] = { 0 };

	command[0] = ST_GETVERSION_EXT;
	command[1] = 0x80;

	if (stlinkv3_command(command, sizeof(command),
				answer, sizeof(answer),
				"ST_GETVERSION_EXT",
				stlinkv3_handle) != 0)
		return -1;

	msg_pinfo("Connected to STLink V3 with bridge FW version: %d\n", answer[4]);
	*result = answer[4] >= FIRST_COMPATIBLE_BRIDGE_FW_VERSION
			? FW_VERSION_OK
			: FW_VERSION_OLD;
	return 0;
}

static int stlinkv3_spi_open(uint16_t reqested_freq_in_kHz, libusb_device_handle *stlinkv3_handle)
{
	uint8_t command[16] = { 0 };
	uint8_t answer[2];
	uint16_t SCK_freq_in_kHz;
	enum spi_prescaler prescaler;
	enum fw_version_check_result fw_check_result;

	if (stlinkv3_check_version(&fw_check_result, stlinkv3_handle)) {
		msg_perr("Failed to query FW version\n");
		return -1;
	}

	if (fw_check_result != FW_VERSION_OK) {
		msg_pinfo("Your STLink V3 has a too old version of the bridge interface\n"
			  "Please update the firmware to version 2.33.25 or newer of the STSW-LINK007\n"
			  "which can be downloaded from here:\n"
			  "https://www.st.com/en/development-tools/stsw-link007.html\n");
		return -1;
	}

	if (stlinkv3_spi_calc_prescaler(reqested_freq_in_kHz,
					&prescaler,
					&SCK_freq_in_kHz,
					stlinkv3_handle)) {
		msg_perr("Failed to calculate SPI clock prescaler\n");
		return -1;
	}
	msg_pinfo("SCK frequency set to %d kHz\n", SCK_freq_in_kHz);

	command[0] = STLINK_BRIDGE_COMMAND;
	command[1] = STLINK_BRIDGE_INIT_SPI;
	command[2] = SPI_DIRECTION_2LINES_FULLDUPLEX;
	command[3] = (SPI_MODE_MASTER
		      | (SPI_CPHA_1EDGE << 1)
		      | (SPI_CPOL_LOW << 2)
		      | (SPI_FIRSTBIT_MSB << 3));
	command[4] = SPI_DATASIZE_8B;
	command[5] = SPI_NSS_SOFT;
	command[6] = (uint8_t)prescaler;

	return stlinkv3_command(command, sizeof(command),
				answer, sizeof(answer),
				"STLINK_BRIDGE_INIT_SPI",
				stlinkv3_handle);
}

static int stlinkv3_get_last_readwrite_status(uint32_t *status, libusb_device_handle *stlinkv3_handle)
{
	uint8_t command[16] = { 0 };
	uint16_t answer[4];

	command[0] = STLINK_BRIDGE_COMMAND;
	command[1] = STLINK_BRIDGE_GET_RWCMD_STATUS;

	if (stlinkv3_command(command, sizeof(command),
			     (uint8_t *)answer, sizeof(answer),
			     "STLINK_BRIDGE_GET_RWCMD_STATUS",
			     stlinkv3_handle) != 0)
		return -1;

	*status = (uint32_t)answer[2] | (uint32_t)answer[3]<<16;
	return 0;
}

static int stlinkv3_spi_set_SPI_NSS(enum spi_nss_level nss_level, libusb_device_handle *stlinkv3_handle)
{
	uint8_t command[16] = { 0 };
	uint8_t answer[2];

	command[0] = STLINK_BRIDGE_COMMAND;
	command[1] = STLINK_BRIDGE_CS_SPI;
	command[2] = (uint8_t) (nss_level);

	if (stlinkv3_command(command, sizeof(command),
				answer, sizeof(answer),
				"STLINK_BRIDGE_CS_SPI",
				stlinkv3_handle) != 0)
		return -1;
	return 0;
}

static int stlinkv3_spi_transmit(const struct flashctx *flash,
				 unsigned int write_cnt,
				 unsigned int read_cnt,
				 const unsigned char *write_arr,
				 unsigned char *read_arr)
{
	struct stlinkv3_spi_data *stlinkv3_data = flash->mst->spi.data;
	libusb_device_handle *stlinkv3_handle = stlinkv3_data->handle;
	uint8_t command[16] = { 0 };
	int rc = 0;
	int actual_length = 0;
	uint32_t rw_status = 0;
	unsigned int i;

	if (stlinkv3_spi_set_SPI_NSS(SPI_NSS_LOW, stlinkv3_handle)) {
		msg_perr("Failed to set the NSS pin to low\n");
		return -1;
	}

	command[0] = STLINK_BRIDGE_COMMAND;
	command[1] = STLINK_BRIDGE_WRITE_SPI;
	command[2] = (uint8_t)write_cnt;
	command[3] = (uint8_t)(write_cnt >> 8);

	for (i = 0; (i < 8) && (i < write_cnt); i++)
		command[4+i] = write_arr[i];

	rc = libusb_bulk_transfer(stlinkv3_handle, STLINK_EP_OUT,
				  command, sizeof(command),
				  &actual_length, USB_TIMEOUT_IN_MS);
	if (rc != LIBUSB_TRANSFER_COMPLETED || actual_length != sizeof(command)) {
		msg_perr("Failed to issue the STLINK_BRIDGE_WRITE_SPI command: '%s'\n",
			 libusb_error_name(rc));
		goto transmit_err;
	}

	if (write_cnt > 8) {
		rc = libusb_bulk_transfer(stlinkv3_handle,
					  STLINK_EP_OUT,
					  (unsigned char *)&write_arr[8],
					  (unsigned int)(write_cnt - 8),
					  &actual_length,
					  USB_TIMEOUT_IN_MS);
		if (rc != LIBUSB_TRANSFER_COMPLETED || (unsigned int)actual_length != (write_cnt - 8)) {
			msg_perr("Failed to send the  data after the  STLINK_BRIDGE_WRITE_SPI command: '%s'\n",
				 libusb_error_name(rc));
			goto transmit_err;
		}
	}

	if (stlinkv3_get_last_readwrite_status(&rw_status, stlinkv3_handle))
		return -1;

	if (rw_status != 0) {
		msg_perr("SPI read/write failure: %d\n", rw_status);
		goto transmit_err;
	}

	if (read_cnt) {
		command[1] = STLINK_BRIDGE_READ_SPI;
		command[2] = (uint8_t)read_cnt;
		command[3] = (uint8_t)(read_cnt >> 8);

		rc = libusb_bulk_transfer(stlinkv3_handle, STLINK_EP_OUT,
					  command, sizeof(command),
					  &actual_length, USB_TIMEOUT_IN_MS);
		if (rc != LIBUSB_TRANSFER_COMPLETED || (unsigned int)actual_length != sizeof(command)) {
			msg_perr("Failed to issue the STLINK_BRIDGE_READ_SPI command: '%s'\n",
				 libusb_error_name(rc));
			goto transmit_err;
		}

		rc = libusb_bulk_transfer(stlinkv3_handle,
					  STLINK_EP_IN,
					  (unsigned char *)read_arr,
					  (int)read_cnt,
					  &actual_length,
					  USB_TIMEOUT_IN_MS);
		if (rc != LIBUSB_TRANSFER_COMPLETED || (unsigned int)actual_length != read_cnt) {
			msg_perr("Failed to retrieve the STLINK_BRIDGE_READ_SPI answer: '%s'\n",
				 libusb_error_name(rc));
			goto transmit_err;
		}
	}

	if (stlinkv3_get_last_readwrite_status(&rw_status, stlinkv3_handle))
		goto transmit_err;

	if (rw_status != 0) {
		msg_perr("SPI read/write failure: %d\n", rw_status);
		goto transmit_err;
	}

	if (stlinkv3_spi_set_SPI_NSS(SPI_NSS_HIGH, stlinkv3_handle)) {
		msg_perr("Failed to set the NSS pin to high\n");
		return -1;
	}
	return 0;

transmit_err:
	if (stlinkv3_spi_set_SPI_NSS(SPI_NSS_HIGH, stlinkv3_handle))
		msg_perr("Failed to set the NSS pin to high\n");
	return -1;
}

static int stlinkv3_spi_shutdown(void *data)
{
	struct stlinkv3_spi_data *stlinkv3_data = data;
	uint8_t command[16] = { 0 };
	uint8_t answer[2];

	command[0] = STLINK_BRIDGE_COMMAND;
	command[1] = STLINK_BRIDGE_CLOSE;
	command[2] = STLINK_SPI_COM;

	stlinkv3_command(command, sizeof(command),
				answer, sizeof(answer),
				"STLINK_BRIDGE_CLOSE",
				stlinkv3_data->handle);

	libusb_close(stlinkv3_data->handle);
	libusb_exit(stlinkv3_data->usb_ctx);

	free(data);
	return 0;
}

static const struct spi_master spi_programmer_stlinkv3 = {
	.max_data_read	= UINT16_MAX,
	.max_data_write	= UINT16_MAX,
	.command	= stlinkv3_spi_transmit,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.shutdown	= stlinkv3_spi_shutdown,
};

static int stlinkv3_spi_init(const struct programmer_cfg *cfg)
{
	uint16_t sck_freq_kHz = 1000;	// selecting 1 MHz SCK is a good bet
	char *param_str;
	char *endptr = NULL;
	int ret = 1;
	int devIndex = 0;
	struct libusb_context *usb_ctx;
	/* Initialize stlinkv3_handle to NULL for suppressing scan-build false positive core.uninitialized.Branch */
	libusb_device_handle *stlinkv3_handle = NULL;
	struct stlinkv3_spi_data *stlinkv3_data;

	if (libusb_init(&usb_ctx)) {
		msg_perr("Could not initialize libusb!\n");
		return 1;
	}

	param_str = extract_programmer_param_str(cfg, "serial");
	if (param_str)
		msg_pdbg("Opening STLINK-V3 with serial: %s\n", param_str);


	while (devs_stlinkv3_spi[devIndex].vendor_id != 0) {
		stlinkv3_handle = usb_dev_get_by_vid_pid_serial(usb_ctx,
								devs_stlinkv3_spi[devIndex].vendor_id,
								devs_stlinkv3_spi[devIndex].device_id,
								param_str);
		if (stlinkv3_handle) {
			if (devs_stlinkv3_spi[devIndex].status == BAD) {
				msg_perr("The STLINK-V3 Mini/MiniE does not support the bridge interface\n");
				free(param_str);
				goto init_err_exit;
			}
			break;
		}
		devIndex++;
	}

	if (!stlinkv3_handle) {
		if (param_str)
			msg_perr("No STLINK-V3 seems to be connected with serial %s\n", param_str);
		else
			msg_perr("Could not find any connected STLINK-V3\n");
		free(param_str);
		goto init_err_exit;
	}
	free(param_str);

	param_str = extract_programmer_param_str(cfg, "spispeed");
	if (param_str) {
		sck_freq_kHz = strtoul(param_str, &endptr, 0);
		if (*endptr || sck_freq_kHz == 0) {
			msg_perr("The spispeed parameter passed with invalid format: %s\n",
				 param_str);
			msg_perr("Please pass the parameter "
				 "with a simple non-zero number in kHz\n");
			free(param_str);
			ret = -1;
			goto init_err_exit;
		}
		free(param_str);
	}

	if (stlinkv3_spi_open(sck_freq_kHz, stlinkv3_handle))
		goto init_err_exit;

	stlinkv3_data = calloc(1, sizeof(*stlinkv3_data));
	if (!stlinkv3_data) {
		msg_perr("Unable to allocate space for SPI master data\n");
		goto init_err_exit;
	}

	stlinkv3_data->usb_ctx = usb_ctx;
	stlinkv3_data->handle = stlinkv3_handle;

	return register_spi_master(&spi_programmer_stlinkv3, stlinkv3_data);

init_err_exit:
	if (stlinkv3_handle)
		libusb_close(stlinkv3_handle);
	libusb_exit(usb_ctx);
	return ret;
}

const struct programmer_entry programmer_stlinkv3_spi = {
	.name			= "stlinkv3_spi",
	.type			= USB,
	.devs.dev		= devs_stlinkv3_spi,
	.init			= stlinkv3_spi_init,
};
