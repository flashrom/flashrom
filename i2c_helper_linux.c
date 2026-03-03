/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2020 The Chromium OS Authors
 */

#include "i2c_helper.h"

#include "programmer.h"
#include "platform/i2c.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>


static int get_bus_number(char *bus_str)
{
	char *bus_suffix;
	errno = 0;
	int bus = (int)strtol(bus_str, &bus_suffix, 10);
	if (errno != 0 || bus_str == bus_suffix) {
		msg_perr("%s: Could not convert 'bus'.\n", __func__);
		return -1;
	}

	if (strlen(bus_suffix) > 0) {
		msg_perr("%s: Garbage following 'bus' value.\n", __func__);
		return -1;
	}

	msg_pinfo("Using I2C bus %d.\n", bus);
	return bus;
}

int i2c_open_from_programmer_params(const struct programmer_cfg *cfg, uint16_t addr, int force)
{
	int fd = -1;

	char *bus_str = extract_programmer_param_str(cfg, "bus");
	char *device_path = extract_programmer_param_str(cfg, "devpath");

	if (device_path != NULL && bus_str != NULL) {
		msg_perr("%s: only one of bus and devpath may be specified\n", __func__);
		goto out;
	}
	if (device_path == NULL && bus_str == NULL) {
		msg_perr("%s: one of bus and devpath must be specified\n", __func__);
		goto out;
	}

	if (device_path != NULL)
		fd = i2c_open_path(device_path, addr, force);
	else
		fd = i2c_open(get_bus_number(bus_str), addr, force);

out:
	free(bus_str);
	free(device_path);
	return fd;
}

