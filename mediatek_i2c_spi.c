/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2021 The Chromium OS Authors
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

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "flash.h"
#include "i2c_helper.h"
#include "programmer.h"
#include "spi.h"

#define ISP_PORT   (0x92 >> 1)
#define DEBUG_PORT (0xb2 >> 1)

#define MTK_CMD_WRITE 0x10
#define MTK_CMD_READ 0x11
#define MTK_CMD_END 0x12

struct mediatek_data {
	int fd;
	unsigned long funcs;
};

// Returns null pointer on error.
static const struct mediatek_data *get_data_from_context(
	const struct flashctx *flash)
{
	if (!flash || !flash->mst || !flash->mst->spi.data) {
		msg_pdbg("Unable to extract data from flash context\n");
		return NULL;
	}
	return (const struct mediatek_data *)flash->mst->spi.data;
}

// Returns bytes read, negative value on error.
static int read_raw(const struct mediatek_data *port, uint16_t addr,
	uint8_t *buf, size_t len)
{
	int ret;

	if (len < 1 || len > I2C_SMBUS_BLOCK_MAX) {
		msg_pdbg("Invalid length for read command: %zu\n", len);
		return SPI_INVALID_LENGTH;
	}

	ret = ioctl(port->fd, I2C_SLAVE, addr);
	if (ret) {
		msg_pdbg("Failed to set slave address 0x%02x (%d)\n", addr, ret);
		return ret;
	}

	if (port->funcs & I2C_FUNC_I2C) {
		// Use raw I2C read.
		uint8_t command = MTK_CMD_READ;
		if (write(port->fd, &command, 1) != 1) {
			return SPI_GENERIC_ERROR;
		}
		if (read(port->fd, buf, len) != (int)len) {
			return SPI_GENERIC_ERROR;
		}
		return len;
	}

	union i2c_smbus_data data = {
		.block[0] = len,
	};
	struct i2c_smbus_ioctl_data args = {
		.read_write = I2C_SMBUS_READ,
		.command = MTK_CMD_READ,
		.size = I2C_SMBUS_I2C_BLOCK_DATA,
		.data = &data,
	};
	ret = ioctl(port->fd, I2C_SMBUS, &args);
	if (ret) {
		msg_pdbg("Failed to read SMBus I2C block data\n");
		return ret;
	}
	memcpy(buf, args.data->block + 1, len);
	return args.data->block[0];
}

// Returns non-zero value on error.
static int write_command(const struct mediatek_data *port, uint16_t addr,
	uint8_t command, const uint8_t *buf, size_t len)
{
	int ret;

	if (len > I2C_SMBUS_BLOCK_MAX) {
		msg_pdbg("Invalid length for write command: %zu\n", len);
		return SPI_INVALID_LENGTH;
	}

	ret = ioctl(port->fd, I2C_SLAVE, addr);
	if (ret) {
		msg_pdbg("Failed to set slave address: 0x%02x\n", addr);
		return ret;
	}

	if (port->funcs & I2C_FUNC_I2C) {
		// Use raw I2C write.
		uint8_t raw_buffer[I2C_SMBUS_BLOCK_MAX + 1];
		raw_buffer[0] = command;
		if (len > 0) {
			memcpy(raw_buffer + 1, buf, len);
		}
		if (write(port->fd, raw_buffer, len + 1) != (int)(len + 1)) {
			return SPI_GENERIC_ERROR;
		}
		return 0;
	}

	union i2c_smbus_data data = {0};
	struct i2c_smbus_ioctl_data args = {
		.read_write = I2C_SMBUS_WRITE,
		.command = command,
		.data = &data,
	};

	// Special case single byte commands, as empty I2C block data commands
	// failed on this component in practice.
	if (len == 0) {
		args.size = I2C_SMBUS_BYTE;
		ret = ioctl(port->fd, I2C_SMBUS, &args);
		if (ret) {
			msg_pdbg("Failed to write SMBus byte: 0x%02x\n", command);
		}
		return ret;
	}

	args.size = I2C_SMBUS_I2C_BLOCK_DATA;
	data.block[0] = len;
	if (buf && len) {
		memcpy(data.block + 1, buf, len);
	}
	ret = ioctl(port->fd, I2C_SMBUS, &args);
	if (ret) {
		msg_pdbg("Failed to write SMBus I2C block data: 0x%02x\n", command);
	}
	return ret;
}

// Returns non-zero value on error.
static int write_raw(const struct mediatek_data *port, uint16_t addr,
	const uint8_t *buf, size_t len)
{
	if (len < 1) {
		msg_pdbg("Invalid write length: %zu\n", len);
		return SPI_INVALID_LENGTH;
	}
	return write_command(port, addr, buf[0], buf + 1, len - 1);
}

// Read from a GPIO register via debug port. Returns non-zero value on error.
static int mediatek_read_gpio(const struct mediatek_data *port,
	uint16_t gpio_addr, uint8_t *value)
{
	int ret;

	uint8_t data[2];
	data[0] = gpio_addr >> 8;
	data[1] = gpio_addr & 0xff;

	ret = write_command(port, DEBUG_PORT, MTK_CMD_WRITE, data, ARRAY_SIZE(data));
	if (ret) {
		msg_pdbg("Failed to issue read GPIO command at address 0x%04x\n", gpio_addr);
		return ret;
	}

	size_t len = 1;
	ret = read_raw(port, DEBUG_PORT, value, len);
	if (ret < 0) {
		msg_pdbg("Failed to read GPIO register at address 0x%04x\n", gpio_addr);
		return ret;
	}

	if (ret != 1) {
		msg_pdbg("GPIO read returned improper length: %d\n", ret);
		return SPI_INVALID_LENGTH;
	}

	return 0;
}

// Write to a GPIO register via the debug port. Returns non-zero value on error.
static int mediatek_write_gpio(const struct mediatek_data *port,
	uint16_t gpio_addr, uint8_t value)
{
	uint8_t data[3];
	data[0] = gpio_addr >> 8;
	data[1] = gpio_addr & 0xff;
	data[2] = value;

	return write_command(port, DEBUG_PORT, MTK_CMD_WRITE, data, ARRAY_SIZE(data));
}

// Write a single bit value to a GPIO register via the debug port. Returns
// non-zero value on error.
static int mediatek_set_gpio_bit(const struct mediatek_data *port,
	uint16_t gpio_addr, int bit, int value)
{
	int ret;
	uint8_t data;

	ret = mediatek_read_gpio(port, gpio_addr, &data);
	if (ret) {
		msg_pdbg("Failed to read GPIO 0x%04x\n", gpio_addr);
		return ret;
	}

	if (value) {
		data |= (1 << bit);
	} else {
		data &= ~(1 << bit);
	}

	ret = mediatek_write_gpio(port, gpio_addr, data);
	if (ret) {
		msg_pdbg("Failed to set GPIO 0x%04x to 0x%02x\n", gpio_addr, data);
		return ret;
	}
	return ret;
}

// Poke the GPIO line that goes to SPI WP# using the MST9U GPIO register
// addresses. Returns non-zero value on error.
static int mediatek_set_write_protect(const struct mediatek_data *port,
	int protected)
{
	int ret;

	ret = mediatek_set_gpio_bit(port, 0x426, 7, (protected == 0));
	if (ret) {
		msg_perr("Failed to set GPIO out value: %d\n", protected == 0);
		return ret;
	}

	ret = mediatek_set_gpio_bit(port, 0x428, 7, (protected != 0));
	if (ret) {
		msg_perr("Failed to set GPIO enable value: %d\n", protected != 0);
		return ret;
	}

	return 0;
}

// Enter serial debug mode using "Option 1" for MST9U and select I2C channel 0
// to configure write protect GPIOs. Returns non-zero value on error.
static int mediatek_enter_serial_debug_mode(const struct mediatek_data *port)
{
	int ret;

	uint8_t enter_serial_debug[] = { 'S', 'E', 'R', 'D', 'B' };
	ret = write_raw(port, DEBUG_PORT, enter_serial_debug,
		ARRAY_SIZE(enter_serial_debug));
	if (ret) {
		msg_perr("Failed to send enter serial debug mode command\n");
		return ret;
	}

	uint8_t enter_single_step_1[] = { 0xc0, 0xc1, 0x53 };
	ret = write_command(port, DEBUG_PORT, MTK_CMD_WRITE,
		enter_single_step_1, ARRAY_SIZE(enter_single_step_1));
	if (ret) {
		msg_perr("Failed to enter serial single step mode (part 1)\n");
		return ret;
	}

	uint8_t enter_single_step_2[] = { 0x1f, 0xc1, 0x53 };
	ret = write_command(port, DEBUG_PORT, MTK_CMD_WRITE, enter_single_step_2,
		ARRAY_SIZE(enter_single_step_2));
	if (ret) {
		msg_perr("Failed to enter serial single step mode (part 2)\n");
		return ret;
	}

	// Send each I2C channel 0 configuration byte individually.
	uint8_t i2c_channel_0_config[] =
		{ 0x80, 0x82, 0x84, 0x51, 0x7f, 0x37, 0x61 };
	for (size_t i = 0; i < ARRAY_SIZE(i2c_channel_0_config); ++i) {
		ret = write_command(
			port, DEBUG_PORT, i2c_channel_0_config[i], NULL, 0);
		if (ret) {
			msg_perr("Failed to configure i2c channel 0: 0x%02x\n",
				i2c_channel_0_config[i]);
			return ret;
		}
	}

	uint8_t enter_single_step_3[] = { 0x00, 0x00, 0x00 };
	ret = write_command(port, DEBUG_PORT, MTK_CMD_WRITE, enter_single_step_3,
		ARRAY_SIZE(enter_single_step_3));
	if (ret) {
		msg_perr("Failed to enter serial single step mode (part 3)\n");
		return ret;
	}

	ret = write_command(port, DEBUG_PORT, 0x35, NULL, 0);
	if (ret) {
		msg_perr("Failed to send serial debug command (part 1)\n");
		return ret;
	}

	ret = write_command(port, DEBUG_PORT, 0x71, NULL, 0);
	if (ret) {
		msg_perr("Failed to send serial debug command (part 2)\n");
		return ret;
	}

	return 0;
}

// Returns non-zero value on error.
static int mediatek_enter_isp(const struct mediatek_data *port)
{
	int ret;

	ret = mediatek_enter_serial_debug_mode(port);
	if (ret) {
		msg_perr("Failed to enter serial debug mode\n");
		return ret;
	}

	// MediaTek documentation says to do this twice just in case.
	uint8_t enter_isp[] = { 'M', 'S', 'T', 'A', 'R' };
	ret = write_raw(port, ISP_PORT, enter_isp, ARRAY_SIZE(enter_isp));
	if (ret) {
		msg_gwarn("Failed to enter ISP mode, trying again\n");
	}
	ret = write_raw(port, ISP_PORT, enter_isp, ARRAY_SIZE(enter_isp));
	if (ret) {
		msg_gwarn("Might already be in ISP mode, ignoring\n");
	}

	ret = mediatek_set_write_protect(port, 0);
	if (ret) {
		msg_perr("Failed to disable write protection\n");
		return ret;
	}

	return 0;
}

// Returns non-zero value on error.
static int mediatek_exit_isp(const struct mediatek_data *port)
{
	int ret;

	ret = mediatek_set_write_protect(port, 1);
	if (ret) {
		msg_perr("Failed to re-enable write protect\n");
		return ret;
	}

	uint8_t exit_single_step[] = { 0xc0, 0xc1, 0xff };
	ret = write_command(port, DEBUG_PORT, MTK_CMD_WRITE, exit_single_step,
		ARRAY_SIZE(exit_single_step));
	if (ret) {
		msg_perr("Failed to exit single step mode\n");
		return ret;
	}

	ret = write_command(port, DEBUG_PORT, 0x34, NULL, 0);
	if (ret) {
		msg_perr("Failed to exit serial debug mode (1), ignoring\n");
	}

	ret = write_command(port, DEBUG_PORT, 0x45, NULL, 0);
	if (ret) {
		msg_perr("Failed to exit serial debug mode (2), ignoring\n");
	}

	ret = write_command(port, ISP_PORT, 0x24, NULL, 0);
	if (ret) {
		msg_perr("Failed to exit ISP mode command, ignoring\n");
	}

	return 0;
}

static int mediatek_send_command(const struct flashctx *flash,
	unsigned int writecnt, unsigned int readcnt,
	const uint8_t *writearr, uint8_t *readarr)
{
	const struct mediatek_data *port = get_data_from_context(flash);
	if (!port) {
		msg_perr("Failed to extract chip data for ISP command\n");
		return SPI_GENERIC_ERROR;
	}

	int ret;

	if (writecnt) {
		ret = write_command(port, ISP_PORT, MTK_CMD_WRITE,
			writearr, writecnt);
		if (ret) {
			msg_perr("Failed to issue ISP write command\n");
			return ret;
		}
	}

	if (readcnt) {
		size_t len = readcnt;
		ret = read_raw(port, ISP_PORT, readarr, len);
		if (ret < 0) {
			msg_perr("Failed to read ISP command result\n");
			return ret;
		}

		if (ret != (int)readcnt) {
			msg_perr("Read length mismatched: expected %u got %d\n", readcnt, ret);
			return SPI_INVALID_LENGTH;
		}
	}

	// End the current command.
	ret = write_command(port, ISP_PORT, MTK_CMD_END, NULL, 0);
	if (ret) {
		msg_perr("Failed to end ISP command\n");
		return ret;
	}

	return 0;
}

static int mediatek_shutdown(void *data)
{
	int ret = 0;
	struct mediatek_data *port = (struct mediatek_data *)data;

	ret |= mediatek_exit_isp(port);
	i2c_close(port->fd);
	free(data);

	return ret;
}

static const struct spi_master spi_master_i2c_mediatek = {
	.max_data_read	= I2C_SMBUS_BLOCK_MAX,
	// Leave room for 1-byte command and up to a 4-byte address.
	.max_data_write	= I2C_SMBUS_BLOCK_MAX - 5,
	.command	= mediatek_send_command,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.shutdown	= mediatek_shutdown,
};

static int get_params(const struct programmer_cfg *cfg, bool *allow_brick)
{
	char *brick_str = NULL;
	int ret = 0;

	*allow_brick = false; /* Default behaviour is to bail. */
	brick_str = extract_programmer_param_str(cfg, "allow_brick");
	if (brick_str) {
		if (!strcmp(brick_str, "yes")) {
			*allow_brick = true;
		} else {
			msg_perr("%s: Incorrect param format, allow_brick=yes.\n", __func__);
			ret = SPI_GENERIC_ERROR;
		}
	}
	free(brick_str);

	return ret;
}

static int mediatek_init(const struct programmer_cfg *cfg)
{
	int ret;
	bool allow_brick;

	if (get_params(cfg, &allow_brick))
		return SPI_GENERIC_ERROR;

	/*
	 * TODO: Once board_enable can facilitate safe i2c allow listing
	 * 	 then this can be removed.
	 */
	if (!allow_brick) {
		msg_perr("%s: For i2c drivers you must explicitly 'allow_brick=yes'. ", __func__);
		msg_perr("There is currently no way to determine if the programmer works on a board "
			 "as i2c device address space can be overloaded. Set 'allow_brick=yes' if "
			 "you are sure you know what you are doing.\n");
		return SPI_GENERIC_ERROR;
	}

	int fd = i2c_open_from_programmer_params(cfg, ISP_PORT, 0);
	if (fd < 0) {
		msg_perr("Failed to open i2c\n");
		return fd;
	}

	struct mediatek_data *port = calloc(1, sizeof(*port));
	if (!port) {
		msg_perr("Unable to allocate space for SPI master data\n");
		i2c_close(fd);
		return SPI_GENERIC_ERROR;
	}
	port->fd = fd;

	ret = ioctl(fd, I2C_FUNCS, &(port->funcs));
	if (ret) {
		msg_perr("Failed to fetch I2C bus functionality\n");
		free(port);
		i2c_close(fd);
		return ret;
	}

	ret = mediatek_enter_isp(port);
	if (ret) {
		msg_perr("Failed to enter ISP mode\n");
		free(port);
		i2c_close(fd);
		return ret;
	}

	return register_spi_master(&spi_master_i2c_mediatek, port);
}

const struct programmer_entry programmer_mediatek_i2c_spi = {
	.name			= "mediatek_i2c_spi",
	.type			= OTHER,
	.devs.note		= "Device files /dev/i2c-*\n",
	.init			= mediatek_init,
};
