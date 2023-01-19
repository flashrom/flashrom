/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2018 Miklós Márton martonmiklosqdev@gmail.com
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

/* The ni845x header does need the WIN32 symbol to be defined and meson does not do it.
 * Define it just here, since this driver will only work on 32-bit Windows.
 */
#ifndef WIN32
#define WIN32
#endif

#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ni845x.h>
#include <unistd.h>

#include "flash.h"
#include "programmer.h"
#include "spi.h"

#define NI845x_FIND_DEVICE_NO_DEVICE_FOUND			-301701

enum USB845x_type {
	USB8451 = 0x7166,
	USB8452 = 0x7514,
	Unknown_NI845X_Device
};

enum voltage_coerce_mode {
	USE_LOWER,
	USE_HIGHER
};

struct ni845x_spi_data {
	unsigned char CS_number; // use chip select 0 as default
	enum USB845x_type device_pid;
	uInt32 device_handle;
	NiHandle configuration_handle;
	uint16_t io_voltage_in_mV;
	bool ignore_io_voltage_limits;
};

// USB-8452 supported voltages, keep this array in ascending order!
static const uint8_t usb8452_io_voltages_in_100mV[5] = {
	kNi845x12Volts,
	kNi845x15Volts,
	kNi845x18Volts,
	kNi845x25Volts,
	kNi845x33Volts
};

/* Copied from dediprog.c */
/* Might be useful for other USB devices as well. static for now. */
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
	} else if (!strncmp(voltage, "mv", 2) || !strncmp(voltage, "millivolt", 9)) {
		/* No adjustment. fraction is discarded. */
	} else {
		/* Garbage at the end of the string. */
		msg_perr("Garbage voltage= specified.\n");
		return -1;
	}
	return millivolt;
}

static void ni845x_report_error(const char *const func, const int32 err)
{
	static char buf[1024];

	ni845xStatusToString(err, sizeof(buf), buf);
	msg_perr("%s failed with: %s (%d)\n", func, buf, (int)err);
}

static void ni845x_report_warning(const char *const func, const int32 err)
{
	static char buf[1024];

	ni845xStatusToString(err, sizeof(buf), buf);
	msg_pwarn("%s failed with: %s (%d)\n", func, buf, (int)err);
}

/**
 * @brief ni845x_spi_open_resource
 * @param resource_handle the resource handle returned by the ni845xFindDevice or ni845xFindDeviceNext
 * @param opened_handle the opened handle from the ni845xOpen
 * @return the 0 on successful competition, negative error code on failure positive code on warning
 */
static int32 ni845x_spi_open_resource(char *resource_handle, uInt32 *opened_handle, enum USB845x_type pid)
{
	// NI-845x driver loads the FPGA bitfile at the first time
	// which can take couple seconds
	if (pid == USB8452)
		msg_pwarn("Opening NI-8452, this might take a while for the first time\n");

	int32 tmp = ni845xOpen(resource_handle, opened_handle);

	if (tmp < 0)
		ni845x_report_error("ni845xOpen", tmp);
	else if (tmp > 0)
		ni845x_report_warning("ni845xOpen", tmp);
	return tmp;
}

/**
 * @param serial a null terminated string containing the serial number of the specific device or NULL
 * @return the 0 on successful completion, negative error code on failure
 */
static int ni845x_spi_open(const char *serial, uInt32 *return_handle, enum USB845x_type *pid)
{
	char resource_name[256];
	NiHandle device_find_handle;
	uInt32 found_devices_count = 0;
	int32 tmp = 0;

	unsigned int vid, dev_pid, usb_bus;
	unsigned long int serial_as_number;
	int ret = -1;

	tmp = ni845xFindDevice(resource_name, &device_find_handle, &found_devices_count);
	if (tmp != 0) {
		// suppress warning if no device found
		if (tmp != NI845x_FIND_DEVICE_NO_DEVICE_FOUND)
			ni845x_report_error("ni845xFindDevice", tmp);
		return -1;
	}

	for (; found_devices_count; --found_devices_count) {
		// Read the serial number and the PID here
		// VISA resource name format example:
		// USB0::0x3923::0x7514::DEADBEEF::RAW
		// where the 0x7514 is the PID
		// and the DEADBEEF is the serial of the device
		if (sscanf(resource_name,
			   "USB%u::0x%04X::0x%04X::%08lX::RAW",
			   &usb_bus, &vid, &dev_pid, &serial_as_number) != 4) {
			// malformed resource string detected
			msg_pwarn("Warning: Unable to parse the %s NI-845x resource string.\n",
				  resource_name);
			msg_pwarn("Please report a bug at flashrom@flashrom.org\n");
			continue;
		}

		*pid = dev_pid;

		if (!serial || strtoul(serial, NULL, 16) == serial_as_number)
			break;

		if (found_devices_count > 1) {
			tmp = ni845xFindDeviceNext(device_find_handle, resource_name);
			if (tmp) {
				ni845x_report_error("ni845xFindDeviceNext", tmp);
				goto _close_ret;
			}
		}
	}

	if (found_devices_count)
		ret = ni845x_spi_open_resource(resource_name, return_handle, *pid);

_close_ret:
	tmp = ni845xCloseFindDeviceHandle(device_find_handle);
	if (tmp) {
		ni845x_report_error("ni845xCloseFindDeviceHandle", tmp);
		return -1;
	}
	return ret;
}

/**
 * @brief usb8452_spi_set_io_voltage sets the IO voltage for the USB-8452 devices
 * @param requested_io_voltage_mV the desired IO voltage in mVolts
 * @param set_io_voltage_mV the IO voltage which was set in mVolts
 * @param coerce_mode if set to USE_LOWER the closest supported IO voltage which is lower or equal to
 * the requested_io_voltage_mV will be selected. Otherwise the next closest supported voltage will be chosen
 * which is higher or equal to the requested_io_voltage_mV.
 * @return 0 on success, negative on error, positive on warning
 */
static int usb8452_spi_set_io_voltage(uint16_t requested_io_voltage_mV,
				      uint16_t *set_io_voltage_mV,
				      enum voltage_coerce_mode coerce_mode,
				      enum USB845x_type pid,
				      uInt32 device_handle)
{
	unsigned int i = 0;
	uint8_t selected_voltage_100mV = 0;
	uint8_t requested_io_voltage_100mV = 0;

	if (pid == USB8451) {
		*set_io_voltage_mV = 3300;
		msg_pwarn("USB-8451 does not support the changing of the SPI IO voltage\n");
		return 0;
	}

	// limit the IO voltage to 3.3V
	if (requested_io_voltage_mV > 3300) {
		msg_pinfo("USB-8452 maximum IO voltage is 3.3V\n");
		return -1;
	}
	requested_io_voltage_100mV = (requested_io_voltage_mV / 100.0f);

	// usb8452_io_voltages_in_100mV contains the supported voltage levels in increasing order
	for (i = (ARRAY_SIZE(usb8452_io_voltages_in_100mV) - 1); i > 0; --i) {
		if (requested_io_voltage_100mV >= usb8452_io_voltages_in_100mV[i])
			break;
	}

	if (coerce_mode == USE_LOWER) {
		if (requested_io_voltage_100mV < usb8452_io_voltages_in_100mV[0]) {
			msg_perr("Unable to set the USB-8452 IO voltage below %.1fV "
				 "(the minimum supported IO voltage is %.1fV)\n",
				 (float)requested_io_voltage_100mV / 10.0f,
				 (float)usb8452_io_voltages_in_100mV[0] / 10.0f);
			return -1;
		}
		selected_voltage_100mV = usb8452_io_voltages_in_100mV[i];
	} else {
		if (i == ARRAY_SIZE(usb8452_io_voltages_in_100mV) - 1)
			selected_voltage_100mV = usb8452_io_voltages_in_100mV[i];
		else
			selected_voltage_100mV = usb8452_io_voltages_in_100mV[i + 1];
	}

	if (requested_io_voltage_100mV < usb8452_io_voltages_in_100mV[0]) {
		/* unsupported / would have to round up */
		msg_pwarn("The USB-8452 does not support the %.1fV IO voltage\n",
			  requested_io_voltage_mV / 1000.0f);
		selected_voltage_100mV = kNi845x12Volts;
		msg_pwarn("The output voltage is set to 1.2V (this is the lowest voltage)\n");
		msg_pwarn("Supported IO voltages:\n");
		for (i = 0; i < ARRAY_SIZE(usb8452_io_voltages_in_100mV); i++) {
			msg_pwarn("%.1fV", (float)usb8452_io_voltages_in_100mV[i] / 10.0f);
			if (i != ARRAY_SIZE(usb8452_io_voltages_in_100mV) - 1)
				msg_pwarn(", ");
		}
		msg_pwarn("\n");
	} else if (selected_voltage_100mV != requested_io_voltage_100mV) {
		/* we rounded down/up */
		msg_pwarn("USB-8452 IO voltage forced to: %.1f V\n",
			  (float)selected_voltage_100mV / 10.0f);
	} else {
		/* exact match */
		msg_pinfo("USB-8452 IO voltage set to: %.1f V\n",
			  (float)selected_voltage_100mV / 10.0f);
	}

	if (set_io_voltage_mV)
		*set_io_voltage_mV = (selected_voltage_100mV * 100);

	i = ni845xSetIoVoltageLevel(device_handle, selected_voltage_100mV);
	if (i != 0) {
		ni845x_report_error("ni845xSetIoVoltageLevel", i);
		return -1;
	}
	return 0;
}

/**
 * @brief ni845x_spi_set_speed sets the SPI SCK speed
 * @param SCK_freq_in_KHz SCK speed in KHz
 * @return
 */
static int ni845x_spi_set_speed(NiHandle configuration_handle, uint16_t SCK_freq_in_KHz)
{
	int32 i = ni845xSpiConfigurationSetClockRate(configuration_handle, SCK_freq_in_KHz);
	uInt16 clock_freq_read_KHz;

	if (i != 0) {
		ni845x_report_error("ni845xSpiConfigurationSetClockRate", i);
		return -1;
	}

	// read back the clock frequency and notify the user if it is not the same as it was requested
	i = ni845xSpiConfigurationGetClockRate(configuration_handle, &clock_freq_read_KHz);
	if (i != 0) {
		ni845x_report_error("ni845xSpiConfigurationGetClockRate", i);
		return -1;
	}

	if (clock_freq_read_KHz != SCK_freq_in_KHz) {
		msg_pinfo("SPI clock frequency forced to: %d KHz (requested: %d KHz)\n",
			  (int)clock_freq_read_KHz, (int)SCK_freq_in_KHz);
	} else {
		msg_pinfo("SPI clock frequency set to: %d KHz\n", (int)SCK_freq_in_KHz);
	}
	return 0;
}

/**
 * @brief ni845x_spi_print_available_devices prints a list of the available devices
 */
static void ni845x_spi_print_available_devices(void)
{
	char resource_handle[256], device_type_string[16];
	NiHandle device_find_handle;
	uInt32 found_devices_count = 0;
	int32 tmp = 0;
	unsigned int pid, vid, usb_bus;
	unsigned long int serial_as_number;

	tmp = ni845xFindDevice(resource_handle, &device_find_handle, &found_devices_count);
	if (tmp != 0) {
		// suppress warning if no device found
		if (tmp != NI845x_FIND_DEVICE_NO_DEVICE_FOUND)
			ni845x_report_error("ni845xFindDevice", tmp);
		return;
	}

	if (found_devices_count) {
		msg_pinfo("Available devices:\n");
		do {
			tmp = sscanf(resource_handle, "USB%d::0x%04X::0x%04X::%08lX::RAW",
				     &usb_bus, &vid, &pid, &serial_as_number);
			if (tmp == 4) {
				switch (pid) {
				case USB8451:
					snprintf(device_type_string,
							 ARRAY_SIZE(device_type_string), "USB-8451");
					break;
				case USB8452:
					snprintf(device_type_string,
							 ARRAY_SIZE(device_type_string), "USB-8452");
					break;
				default:
					snprintf(device_type_string,
							 ARRAY_SIZE(device_type_string), "Unknown device");
					break;
				}
				msg_pinfo("- %lX (%s)\n", serial_as_number, device_type_string);

				found_devices_count--;
				if (found_devices_count) {
					tmp = ni845xFindDeviceNext(device_find_handle, resource_handle);
					if (tmp)
						ni845x_report_error("ni845xFindDeviceNext", tmp);
				}
			}
		} while (found_devices_count);
	}

	tmp = ni845xCloseFindDeviceHandle(device_find_handle);
	if (tmp)
		ni845x_report_error("ni845xCloseFindDeviceHandle", tmp);
}

static int ni845x_spi_shutdown(void *data)
{
	struct ni845x_spi_data *ni_data = data;
	int32 ret = 0;

	if (ni_data->configuration_handle != 0) {
		ret = ni845xSpiConfigurationClose(ni_data->configuration_handle);
		if (ret)
			ni845x_report_error("ni845xSpiConfigurationClose", ret);
	}

	if (ni_data->device_handle != 0) {
		ret = ni845xClose(ni_data->device_handle);
		if (ret)
			ni845x_report_error("ni845xClose", ret);
	}

	free(data);
	return ret;
}

static void ni845x_warn_over_max_voltage(const struct flashctx *flash)
{
	const struct ni845x_spi_data *data = flash->mst->spi.data;

	if (data->device_pid == USB8451) {
		msg_pwarn("The %s chip maximum voltage is %.1fV, while the USB-8451 "
			  "IO voltage levels are 3.3V.\n"
			  "Ignoring this because ignore_io_voltage_limits parameter is set.\n",
			 flash->chip->name,
			 flash->chip->voltage.max / 1000.0f);
	} else if (data->device_pid == USB8452) {
		msg_pwarn("The %s chip maximum voltage is %.1fV, while the USB-8452 "
			  "IO voltage is set to %.1fV.\n"
			  "Ignoring this because ignore_io_voltage_limits parameter is set.\n",
			  flash->chip->name,
			  flash->chip->voltage.max / 1000.0f,
			  data->io_voltage_in_mV / 1000.0f);
	}
}

static int ni845x_spi_io_voltage_check(const struct flashctx *flash)
{
	struct ni845x_spi_data *data = flash->mst->spi.data;
	static bool first_transmit = true;

	if (first_transmit && flash->chip) {
		first_transmit = false;
		if (data->io_voltage_in_mV > flash->chip->voltage.max) {
			if (data->ignore_io_voltage_limits) {
				ni845x_warn_over_max_voltage(flash);
				return 0;
			}

			if (data->device_pid == USB8451) {
				msg_perr("The %s chip maximum voltage is %.1fV, while the USB-8451 "
					 "IO voltage levels are 3.3V.\nAborting operations\n",
					 flash->chip->name,
					 flash->chip->voltage.max / 1000.0f);
				return -1;
			} else if (data->device_pid == USB8452) {
				msg_perr("Lowering IO voltage because the %s chip maximum voltage is %.1fV, "
					 "(%.1fV was set)\n",
					 flash->chip->name,
					 flash->chip->voltage.max / 1000.0f,
					 data->io_voltage_in_mV / 1000.0f);
				if (usb8452_spi_set_io_voltage(flash->chip->voltage.max,
							       &data->io_voltage_in_mV,
							       USE_LOWER,
							       data->device_pid,
							       data->device_handle)) {
					msg_perr("Unable to lower the IO voltage below "
						 "the chip's maximum voltage\n");
					return -1;
				}
			}
		} else if (data->io_voltage_in_mV < flash->chip->voltage.min) {
			if (data->device_pid == USB8451) {
				msg_pwarn("Flash operations might be unreliable, because the %s chip's "
					  "minimum voltage is %.1fV, while the USB-8451's "
					  "IO voltage levels are 3.3V.\n",
					  flash->chip->name,
					  flash->chip->voltage.min / 1000.0f);
				return data->ignore_io_voltage_limits ? 0 : -1;
			} else if (data->device_pid == USB8452) {
				msg_pwarn("Raising the IO voltage because the %s chip's "
					  "minimum voltage is %.1fV, "
					  "(%.1fV was set)\n",
					  flash->chip->name,
					  flash->chip->voltage.min / 1000.0f,
					  data->io_voltage_in_mV / 1000.0f);
				if (usb8452_spi_set_io_voltage(flash->chip->voltage.min,
							       &data->io_voltage_in_mV,
							       USE_HIGHER,
							       data->device_pid,
							       data->device_handle)) {
					msg_pwarn("Unable to raise the IO voltage above the chip's "
						  "minimum voltage\n"
						  "Flash operations might be unreliable.\n");
					return data->ignore_io_voltage_limits ? 0 : -1;
				}
			}
		}
	}
	return 0;
}

static int ni845x_spi_transmit(const struct flashctx *flash,
			       unsigned int write_cnt,
			       unsigned int read_cnt,
			       const unsigned char *write_arr,
			       unsigned char *read_arr)
{
	const struct ni845x_spi_data *data = flash->mst->spi.data;
	uInt32 read_size = 0;
	uInt8 *transfer_buffer = NULL;
	int32 ret = 0;

	if (ni845x_spi_io_voltage_check(flash))
		return -1;

	transfer_buffer = calloc(write_cnt + read_cnt, sizeof(uInt8));
	if (transfer_buffer == NULL) {
		msg_gerr("Memory allocation failed!\n");
		return -1;
	}

	memcpy(transfer_buffer, write_arr, write_cnt);

	ret = ni845xSpiWriteRead(data->device_handle,
				 data->configuration_handle,
				 (write_cnt + read_cnt), transfer_buffer, &read_size, transfer_buffer);
	if (ret < 0) {
		// Negative specifies an error, meaning the function did not perform the expected behavior.
		ni845x_report_error("ni845xSpiWriteRead", ret);
		free(transfer_buffer);
		return -1;
	} else if (ret > 0) {
		// Positive specifies a warning, meaning the function performed as expected,
		// but a condition arose that might require attention.
		ni845x_report_warning("ni845xSpiWriteRead", ret);
	}

	if (read_cnt != 0 && read_arr != NULL) {
		if ((read_cnt + write_cnt) != read_size) {
			msg_perr("%s: expected and returned read count mismatch: %u expected, %ld received\n",
					 __func__, read_cnt, read_size);
			free(transfer_buffer);
			return -1;
		}
		memcpy(read_arr, &transfer_buffer[write_cnt], read_cnt);
	}
	free(transfer_buffer);
	return 0;
}

static const struct spi_master spi_programmer_ni845x = {
	.max_data_read	= MAX_DATA_READ_UNLIMITED,
	.max_data_write	= MAX_DATA_WRITE_UNLIMITED,
	.command	= ni845x_spi_transmit,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.shutdown	= ni845x_spi_shutdown,
};

static int ni845x_spi_init(const struct programmer_cfg *cfg)
{
	char *speed_str = NULL;
	char *CS_str = NULL;
	char *voltage = NULL;
	char *endptr = NULL;
	int requested_io_voltage_mV = 1200;     // default the IO voltage to 1.2V
	int spi_speed_KHz = 1000;		// selecting 1 MHz SCK is a good bet
	char *serial_number = NULL;		// by default open the first connected device
	char *ignore_io_voltage_limits_str = NULL;
	bool ignore_io_voltage_limits;
	unsigned char CS_number = 0;
	enum USB845x_type device_pid = Unknown_NI845X_Device;
	uInt32 device_handle;
	int32 tmp = 0;

	// read the cs parameter (which Chip select should we use)
	CS_str = extract_programmer_param_str(cfg, "cs");
	if (CS_str) {
		CS_number = strtoul(CS_str, NULL, 10);
		free(CS_str);
		if (CS_number > 7) {
			msg_perr("Only CS 0-7 supported\n");
			return 1;
		}
	}

	voltage = extract_programmer_param_str(cfg, "voltage");
	if (voltage != NULL) {
		requested_io_voltage_mV = parse_voltage(voltage);
		free(voltage);
		if (requested_io_voltage_mV < 0)
			return 1;
	}

	serial_number = extract_programmer_param_str(cfg, "serial");

	speed_str = extract_programmer_param_str(cfg, "spispeed");
	if (speed_str) {
		spi_speed_KHz = strtoul(speed_str, &endptr, 0);
		if (*endptr) {
			msg_perr("The spispeed parameter passed with invalid format: %s\n",
				 speed_str);
			msg_perr("Please pass the parameter with a simple number in kHz\n");
			return 1;
		}
		free(speed_str);
	}

	ignore_io_voltage_limits = false;
	ignore_io_voltage_limits_str = extract_programmer_param_str(cfg, "ignore_io_voltage_limits");
	if (ignore_io_voltage_limits_str
		&& strcmp(ignore_io_voltage_limits_str, "yes") == 0) {
		ignore_io_voltage_limits = true;
	}

	if (ni845x_spi_open(serial_number, &device_handle, &device_pid)) {
		if (serial_number) {
			msg_pinfo("Could not find any connected NI USB-8451/8452 with serialnumber: %s!\n",
				  serial_number);
			ni845x_spi_print_available_devices();
			msg_pinfo("Check the S/N field on the bottom of the device,\n"
				  "or use 'lsusb -v -d 3923:7166 | grep Serial' for USB-8451\n"
				  "or 'lsusb -v -d 3923:7514 | grep Serial' for USB-8452\n");
			free(serial_number);
		} else {
			msg_pinfo("Could not find any connected NI USB-845x device!\n");
		}
		return 1;
	}
	free(serial_number);

	struct ni845x_spi_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for SPI master data\n");
		return 1;
	}
	data->CS_number = CS_number;
	data->device_pid = device_pid;
	data->device_handle = device_handle;
	data->ignore_io_voltage_limits = ignore_io_voltage_limits;

	// open the SPI config handle
	tmp = ni845xSpiConfigurationOpen(&data->configuration_handle);
	if (tmp != 0) {
		ni845x_report_error("ni845xSpiConfigurationOpen", tmp);
		goto err;
	}

	if (usb8452_spi_set_io_voltage(requested_io_voltage_mV, &data->io_voltage_in_mV,
				       USE_LOWER, data->device_pid, data->device_handle) < 0) {
		// no alert here usb8452_spi_set_io_voltage already printed that
		goto err;
	}

	if (ni845x_spi_set_speed(data->configuration_handle, spi_speed_KHz)) {
		msg_perr("Unable to set SPI speed\n");
		goto err;
	}

	return register_spi_master(&spi_programmer_ni845x, data);

err:
	ni845x_spi_shutdown(data);
	return 1;
}

const struct programmer_entry programmer_ni845x_spi = {
	.name			= "ni845x_spi",
	.type			= OTHER, // choose other because NI-845x uses own USB implementation
	.devs.note		= "National Instruments USB-845x\n",
	.init			= ni845x_spi_init,
};
