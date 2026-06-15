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
#include "platform/string.h"
#include <stdlib.h>
#include "log.h"


int i2c_write_buffer(int fd, uint16_t addr, void *buf, uint16_t len)
{
	i2c_buffer_t data;
	if (i2c_buffer_t_fill(&data, buf, len))
		return -1;

	return i2c_write(fd, addr, &data) == len ? 0 : -1;
}

int i2c_read_buffer(int fd, uint16_t addr, void *buf, uint16_t len)
{
	i2c_buffer_t data;
	if (i2c_buffer_t_fill(&data, buf, len))
		return -1;

	return i2c_read(fd, addr, &data) == len ? 0 : -1;
}

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

int i2c_require_allow_brick(const struct programmer_cfg *cfg)
{
	char *brick_str = extract_programmer_param_str(cfg, "allow_brick");
	bool allow_brick = false; /* Default behaviour is to bail. */

	if (brick_str) {
		if (!strcmp(brick_str, "yes")) {
			allow_brick = true;
		} else {
			msg_perr("%s: Incorrect param format, allow_brick=yes.\n", __func__);
			free(brick_str);
			return -1;
		}
	}
	free(brick_str);

	/*
	 * TODO: Once board_enable can facilitate safe i2c allow listing
	 * 	 then this can be removed.
	 */
	if (!allow_brick) {
		msg_perr("%s: For i2c drivers you must explicitly 'allow_brick=yes'. ", __func__);
		msg_perr("There is currently no way to determine if the programmer works on a board "
			 "as i2c device address space can be overloaded. Set 'allow_brick=yes' if "
			 "you are sure you know what you are doing.\n");
		return -1;
	}

	return 0;
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

