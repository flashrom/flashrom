/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2016 Marc Schink <flashrom-dev@marcschink.de>
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

/*
 * Driver for the J-Link hardware by SEGGER.
 * See https://www.segger.com/ for more info.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <libjaylink/libjaylink.h>

#include "flash.h"
#include "programmer.h"
#include "spi.h"

/*
 * Maximum number of bytes that can be transferred at once via the JTAG
 * interface, see jaylink_jtag_io().
 */
#define JTAG_MAX_TRANSFER_SIZE	(32768 / 8)

/*
 * Default base frequency in Hz. Used when the base frequency can not be
 * retrieved from the device.
 */
#define DEFAULT_FREQ		16000000

/*
 * Default frequency divider. Used when the frequency divider can not be
 * retrieved from the device.
 */
#define DEFAULT_FREQ_DIV	4

/* Minimum target voltage required for operation in mV. */
#define MIN_TARGET_VOLTAGE	1200

enum cs_wiring {
	CS_RESET, /* nCS is wired to nRESET(pin 15) */
	CS_TRST, /* nCS is wired to nTRST(pin 3) */
	CS_TMS, /* nCS is wired to TMS/nCS(pin 7) */
};

struct jlink_spi_data {
	struct jaylink_context *ctx;
	struct jaylink_device_handle *devh;
	enum cs_wiring cs;
	bool enable_target_power;
};

static bool assert_cs(struct jlink_spi_data *jlink_data)
{
	int ret;

	if (jlink_data->cs == CS_RESET) {
		ret = jaylink_clear_reset(jlink_data->devh);

		if (ret != JAYLINK_OK) {
			msg_perr("jaylink_clear_reset() failed: %s.\n", jaylink_strerror(ret));
			return false;
		}
	} else if (jlink_data->cs == CS_TRST) {
		ret = jaylink_jtag_clear_trst(jlink_data->devh);

		if (ret != JAYLINK_OK) {
			msg_perr("jaylink_jtag_clear_trst() failed: %s.\n", jaylink_strerror(ret));
			return false;
		}
	} else {
		ret = jaylink_jtag_clear_tms(jlink_data->devh);

		if (ret != JAYLINK_OK) {
			msg_perr("jaylink_jtag_clear_tms() failed: %s.\n", jaylink_strerror(ret));
			return false;
		}
	}

	return true;
}

static bool deassert_cs(struct jlink_spi_data *jlink_data)
{
	int ret;

	if (jlink_data->cs == CS_RESET) {
		ret = jaylink_set_reset(jlink_data->devh);

		if (ret != JAYLINK_OK) {
			msg_perr("jaylink_set_reset() failed: %s.\n", jaylink_strerror(ret));
			return false;
		}
	} else if (jlink_data->cs == CS_TRST) {
		ret = jaylink_jtag_set_trst(jlink_data->devh);

		if (ret != JAYLINK_OK) {
			msg_perr("jaylink_jtag_set_trst() failed: %s.\n", jaylink_strerror(ret));
			return false;
		}
	} else {
		ret = jaylink_jtag_set_tms(jlink_data->devh);

		if (ret != JAYLINK_OK) {
			msg_perr("jaylink_jtag_set_tms() failed: %s.\n", jaylink_strerror(ret));
			return false;
		}
	}

	return true;
}

static int jlink_spi_send_command(const struct flashctx *flash, unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr)
{
	uint32_t length;
	uint8_t *buffer;
	static const uint8_t zeros[JTAG_MAX_TRANSFER_SIZE] = {0};
	struct jlink_spi_data *jlink_data = flash->mst->spi.data;

	length = writecnt + readcnt;

	if (length > JTAG_MAX_TRANSFER_SIZE)
		return SPI_INVALID_LENGTH;

	buffer = malloc(length);

	if (!buffer) {
		msg_perr("Memory allocation failed.\n");
		return SPI_GENERIC_ERROR;
	}

	/* Reverse all bytes because the device transfers data LSB first. */
	reverse_bytes(buffer, writearr, writecnt);

	memset(buffer + writecnt, 0x00, readcnt);

	if (!assert_cs(jlink_data)) {
		free(buffer);
		return SPI_PROGRAMMER_ERROR;
	}

	/* If CS is wired to TMS, TMS should remain zero. */
	const uint8_t *tms_buffer = jlink_data->cs == CS_TMS ? zeros : buffer;

	int ret;

	ret = jaylink_jtag_io(jlink_data->devh,
				tms_buffer, buffer, buffer, length * 8, JAYLINK_JTAG_VERSION_2);

	if (ret != JAYLINK_OK) {
		msg_perr("jaylink_jtag_io() failed: %s.\n", jaylink_strerror(ret));
		free(buffer);
		return SPI_PROGRAMMER_ERROR;
	}

	if (!deassert_cs(jlink_data)) {
		free(buffer);
		return SPI_PROGRAMMER_ERROR;
	}

	/* Reverse all bytes because the device transfers data LSB first. */
	reverse_bytes(readarr, buffer + writecnt, readcnt);
	free(buffer);

	return 0;
}

static int jlink_spi_shutdown(void *data)
{
	struct jlink_spi_data *jlink_data = data;

	if (jlink_data->enable_target_power) {
		int ret = jaylink_set_target_power(jlink_data->devh, false);

		if (ret != JAYLINK_OK) {
			msg_perr("jaylink_set_target_power() failed: %s.\n",
				jaylink_strerror(ret));
		}
	}

	if (jlink_data->devh)
		jaylink_close(jlink_data->devh);

	jaylink_exit(jlink_data->ctx);
	/* jlink_data->ctx, jlink_data->devh are freed by jaylink_close and jaylink_exit */
	free(jlink_data);
	return 0;
}

static const struct spi_master spi_master_jlink_spi = {
	/* Maximum data read size in one go (excluding opcode+address). */
	.max_data_read	= JTAG_MAX_TRANSFER_SIZE - 5,
	/* Maximum data write size in one go (excluding opcode+address). */
	.max_data_write	= JTAG_MAX_TRANSFER_SIZE - 5,
	.command	= jlink_spi_send_command,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.features	= SPI_MASTER_4BA,
	.shutdown	= jlink_spi_shutdown,
};

static int jlink_spi_init(const struct programmer_cfg *cfg)
{
	char *arg;
	unsigned long speed = 0;
	struct jaylink_context *jaylink_ctx = NULL;
	struct jaylink_device_handle *jaylink_devh = NULL;
	enum cs_wiring cs;
	struct jlink_spi_data *jlink_data = NULL;
	bool enable_target_power;

	arg = extract_programmer_param_str(cfg, "spispeed");

	if (arg) {
		char *endptr;

		errno = 0;
		speed = strtoul(arg, &endptr, 10);

		if (*endptr != '\0' || errno != 0) {
			msg_perr("Invalid SPI speed specified: %s.\n", arg);
			free(arg);
			return 1;
		}

		if (speed < 1) {
			msg_perr("SPI speed must be at least 1 kHz.\n");
			free(arg);
			return 1;
		}
	}

	free(arg);

	int ret;
	bool use_serial_number;
	uint32_t serial_number;

	arg = extract_programmer_param_str(cfg, "serial");

	if (arg) {
		if (!strlen(arg)) {
			msg_perr("Empty serial number specified.\n");
			free(arg);
			return 1;
		}

		ret = jaylink_parse_serial_number(arg, &serial_number);

		if (ret == JAYLINK_ERR) {
			msg_perr("Invalid serial number specified: %s.\n", arg);
			free(arg);
			return 1;
		} if (ret != JAYLINK_OK) {
			msg_perr("jaylink_parse_serial_number() failed: %s.\n", jaylink_strerror(ret));
			free(arg);
			return 1;
		}

		use_serial_number = true;
	} else {
		use_serial_number = false;
	}

	free(arg);

	cs = CS_RESET;
	arg = extract_programmer_param_str(cfg, "cs");

	if (arg) {
		if (!strcasecmp(arg, "reset")) {
			cs = CS_RESET;
		} else if (!strcasecmp(arg, "trst")) {
			cs = CS_TRST;
		} else if (!strcasecmp(arg, "tms")) {
			cs = CS_TMS;
		} else {
			msg_perr("Invalid chip select pin specified: '%s'.\n", arg);
			free(arg);
			return 1;
		}
	}

	free(arg);

	if (cs == CS_RESET)
		msg_pdbg("Using RESET as chip select signal.\n");
	else if (cs == CS_TRST)
		msg_pdbg("Using TRST as chip select signal.\n");
	else
		msg_pdbg("Using TMS/CS as chip select signal.\n");

	enable_target_power = false;
	arg = extract_programmer_param_str(cfg, "power");

	if (arg) {
		if (!strcasecmp(arg, "on")) {
			enable_target_power = true;
		} else {
			msg_perr("Invalid value for 'power' argument: '%s'.\n", arg);
			free(arg);
			return 1;
		}
	}

	free(arg);

	ret = jaylink_init(&jaylink_ctx);

	if (ret != JAYLINK_OK) {
		msg_perr("jaylink_init() failed: %s.\n", jaylink_strerror(ret));
		return 1;
	}

	ret = jaylink_discovery_scan(jaylink_ctx, 0);

	if (ret != JAYLINK_OK) {
		msg_perr("jaylink_discovery_scan() failed: %s.\n", jaylink_strerror(ret));
		goto init_err;
	}

	struct jaylink_device **devs;

	ret = jaylink_get_devices(jaylink_ctx, &devs, NULL);

	if (ret != JAYLINK_OK) {
		msg_perr("jaylink_get_devices() failed: %s.\n", jaylink_strerror(ret));
		goto init_err;
	}

	if (!use_serial_number)
		msg_pdbg("No device selected, using first device.\n");

	size_t i;
	struct jaylink_device *dev;
	bool device_found = false;

	for (i = 0; devs[i]; i++) {
		if (use_serial_number) {
			uint32_t tmp;

			ret = jaylink_device_get_serial_number(devs[i], &tmp);

			if (ret == JAYLINK_ERR_NOT_AVAILABLE) {
				continue;
			} else if (ret != JAYLINK_OK) {
				msg_pwarn("jaylink_device_get_serial_number() failed: %s.\n",
					jaylink_strerror(ret));
				continue;
			}

			if (serial_number != tmp)
				continue;
		}

		ret = jaylink_open(devs[i], &jaylink_devh);

		if (ret == JAYLINK_OK) {
			dev = devs[i];
			device_found = true;
			break;
		}

		jaylink_devh = NULL;
	}

	jaylink_free_devices(devs, true);

	if (!device_found) {
		msg_perr("No J-Link device found.\n");
		goto init_err;
	}

	size_t length;
	char *firmware_version;

	ret = jaylink_get_firmware_version(jaylink_devh, &firmware_version,
		&length);

	if (ret != JAYLINK_OK) {
		msg_perr("jaylink_get_firmware_version() failed: %s.\n", jaylink_strerror(ret));
		goto init_err;
	} else if (length > 0) {
		msg_pdbg("Firmware: %s\n", firmware_version);
		free(firmware_version);
	}

	ret = jaylink_device_get_serial_number(dev, &serial_number);

	if (ret == JAYLINK_OK) {
		msg_pdbg("S/N: %" PRIu32 "\n", serial_number);
	} else if (ret == JAYLINK_ERR_NOT_AVAILABLE) {
		msg_pdbg("S/N: N/A\n");
	} else {
		msg_perr("jaylink_device_get_serial_number() failed: %s.\n", jaylink_strerror(ret));
		goto init_err;
	}

	uint8_t caps[JAYLINK_DEV_EXT_CAPS_SIZE] = { 0 };

	ret = jaylink_get_caps(jaylink_devh, caps);

	if (ret != JAYLINK_OK) {
		msg_perr("jaylink_get_caps() failed: %s.\n", jaylink_strerror(ret));
		goto init_err;
	}

	if (jaylink_has_cap(caps, JAYLINK_DEV_CAP_GET_EXT_CAPS)) {
		ret = jaylink_get_extended_caps(jaylink_devh, caps);

		if (ret != JAYLINK_OK) {
			msg_perr("jaylink_get_extended_caps() failed: %s.\n", jaylink_strerror(ret));
			goto init_err;
		}
	}

	if (enable_target_power) {
		if (!jaylink_has_cap(caps, JAYLINK_DEV_CAP_SET_TARGET_POWER)) {
			msg_perr("Device does not support target power.\n");
			goto init_err;
		}
	}

	uint32_t ifaces;

	ret = jaylink_get_available_interfaces(jaylink_devh, &ifaces);

	if (ret != JAYLINK_OK) {
		msg_perr("jaylink_get_available_interfaces() failed: %s.\n", jaylink_strerror(ret));
		goto init_err;
	}

	if (!(ifaces & (1 << JAYLINK_TIF_JTAG))) {
		msg_perr("Device does not support JTAG interface.\n");
		goto init_err;
	}

	ret = jaylink_select_interface(jaylink_devh, JAYLINK_TIF_JTAG, NULL);

	if (ret != JAYLINK_OK) {
		msg_perr("jaylink_select_interface() failed: %s.\n", jaylink_strerror(ret));
		goto init_err;
	}

	if (enable_target_power) {
		ret = jaylink_set_target_power(jaylink_devh, true);

		if (ret != JAYLINK_OK) {
			msg_perr("jaylink_set_target_power() failed: %s.\n", jaylink_strerror(ret));
			goto init_err;
		}

		/* Wait some time until the target is powered up. */
		internal_sleep(10000);
	}

	struct jaylink_hardware_status hwstat;

	ret = jaylink_get_hardware_status(jaylink_devh, &hwstat);

	if (ret != JAYLINK_OK) {
		msg_perr("jaylink_get_hardware_status() failed: %s.\n", jaylink_strerror(ret));
		goto init_err;
	}

	msg_pdbg("VTarget: %u.%03u V\n", hwstat.target_voltage / 1000,
		hwstat.target_voltage % 1000);

	if (hwstat.target_voltage < MIN_TARGET_VOLTAGE) {
		msg_perr("Target voltage is below %u.%03u V. You need to attach VTref to the I/O voltage of "
			"the chip.\n", MIN_TARGET_VOLTAGE / 1000, MIN_TARGET_VOLTAGE % 1000);
		goto init_err;
	}

	struct jaylink_speed device_speeds;

	device_speeds.freq = DEFAULT_FREQ;
	device_speeds.div = DEFAULT_FREQ_DIV;

	if (jaylink_has_cap(caps, JAYLINK_DEV_CAP_GET_SPEEDS)) {
		ret = jaylink_get_speeds(jaylink_devh, &device_speeds);

		if (ret != JAYLINK_OK) {
			msg_perr("jaylink_get_speeds() failed: %s.\n", jaylink_strerror(ret));
			goto init_err;
		}
	}

	device_speeds.freq /= 1000;

	msg_pdbg("Maximum SPI speed: %" PRIu32 " kHz\n", device_speeds.freq / device_speeds.div);

	if (!speed) {
		speed = device_speeds.freq / device_speeds.div;
		msg_pdbg("SPI speed not specified, using %lu kHz.\n", speed);
	}

	if (speed > (device_speeds.freq / device_speeds.div)) {
		msg_perr("Specified SPI speed of %lu kHz is too high. Maximum is %" PRIu32 " kHz.\n", speed,
			device_speeds.freq / device_speeds.div);
		goto init_err;
	}

	ret = jaylink_set_speed(jaylink_devh, speed);

	if (ret != JAYLINK_OK) {
		msg_perr("jaylink_set_speed() failed: %s.\n", jaylink_strerror(ret));
		goto init_err;
	}

	msg_pdbg("SPI speed: %lu kHz\n", speed);

	jlink_data = calloc(1, sizeof(*jlink_data));
	if (!jlink_data) {
		msg_perr("Unable to allocate space for SPI master data\n");
		goto init_err;
	}

	/* jaylink_ctx, jaylink_devh are allocated by jaylink_init and jaylink_open */
	jlink_data->ctx = jaylink_ctx;
	jlink_data->devh = jaylink_devh;
	jlink_data->cs = cs;
	jlink_data->enable_target_power = enable_target_power;

	/* Ensure that the CS signal is not active initially. */
	if (!deassert_cs(jlink_data))
		goto init_err;

	return register_spi_master(&spi_master_jlink_spi, jlink_data);

init_err:
	if (jaylink_devh)
		jaylink_close(jaylink_devh);

	jaylink_exit(jaylink_ctx);

	/* jaylink_ctx, jaylink_devh are freed by jaylink_close and jaylink_exit */
	if (jlink_data)
		free(jlink_data);

	return 1;
}

const struct programmer_entry programmer_jlink_spi = {
	.name			= "jlink_spi",
	.type			= OTHER,
	.init			= jlink_spi_init,
	.devs.note		= "SEGGER J-Link and compatible devices\n",
};
