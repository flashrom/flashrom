/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2020 The Chromium OS Authors
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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "programmer.h"
#include "spi.h"
#include "i2c_helper.h"


#define MCU_I2C_SLAVE_ADDR 0x94
#define REGISTER_ADDRESS	(0x94 >> 1)
#define PAGE_SIZE		256
#define MAX_SPI_WAIT_RETRIES	1000

#define MCU_MODE 0x6F
#define 	ENTER_ISP_MODE 0x80

#define MCU_DATA_PORT 0x70

#define MAP_PAGE_BYTE2 0x64
#define MAP_PAGE_BYTE1 0x65
#define MAP_PAGE_BYTE0 0x66

//opcodes
#define OPCODE_READ  3
#define OPCODE_WRITE 2


struct realtek_mst_i2c_spi_data {
	int fd;
};

static int realtek_mst_i2c_spi_write_data(int fd, uint16_t addr, void *buf, uint16_t len)
{
	i2c_buffer_t data;
	if (i2c_buffer_t_fill(&data, buf, len))
		return SPI_GENERIC_ERROR;

	return i2c_write(fd, addr, &data) == len ? 0 : SPI_GENERIC_ERROR;
}

static int realtek_mst_i2c_spi_read_data(int fd, uint16_t addr, void *buf, uint16_t len)
{
	i2c_buffer_t data;
	if (i2c_buffer_t_fill(&data, buf, len))
		return SPI_GENERIC_ERROR;

	return i2c_read(fd, addr, &data) == len ? 0 : SPI_GENERIC_ERROR;
}

static int get_fd_from_context(const struct flashctx *flash)
{
	if (!flash || !flash->mst || !flash->mst->spi.data) {
		msg_perr("Unable to extract fd from flash context.\n");
		return SPI_GENERIC_ERROR;
	}
        const struct realtek_mst_i2c_spi_data *data =
		(const struct realtek_mst_i2c_spi_data *)flash->mst->spi.data;

	return data->fd;
}

static int realtek_mst_i2c_spi_write_register(int fd, uint8_t reg, uint8_t value)
{
	uint8_t command[] = { reg, value };
	return realtek_mst_i2c_spi_write_data(fd, REGISTER_ADDRESS, command, 2);
}

static int realtek_mst_i2c_spi_read_register(int fd, uint8_t reg, uint8_t *value)
{
	uint8_t command[] = { reg };
	int ret = realtek_mst_i2c_spi_write_data(fd, REGISTER_ADDRESS, command, 1);
	ret |= realtek_mst_i2c_spi_read_data(fd, REGISTER_ADDRESS, value, 1);

	return ret ? SPI_GENERIC_ERROR : 0;
}

static int realtek_mst_i2c_spi_wait_command_done(int fd, unsigned int offset, int mask)
{
	uint8_t val;
	int tried = 0;
	int ret = 0;
	do {
		ret |= realtek_mst_i2c_spi_read_register(fd, offset, &val);
	} while (!ret && (val & mask) && ++tried < MAX_SPI_WAIT_RETRIES);

	if (tried == MAX_SPI_WAIT_RETRIES) {
		msg_perr("%s: Time out on sending command.\n", __func__);
		return -MAX_SPI_WAIT_RETRIES;
	}

	return (val & mask) ? SPI_GENERIC_ERROR : ret;
}

static int realtek_mst_i2c_spi_enter_isp_mode(int fd)
{
	int ret = realtek_mst_i2c_spi_write_register(fd, MCU_MODE, ENTER_ISP_MODE);

	// set internal osc divider register to default to speed up MCU
	// 0x06A0 = 0x74
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF4, 0x9F);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF5, 0x06);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF4, 0xA0);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF5, 0x74);

	return ret;
}

static int realtek_mst_i2c_spi_reset_mpu(int fd)
{
	int ret = 0;
	// 0xFFEE[1] = 1;
	uint8_t val = 0;
	ret |= realtek_mst_i2c_spi_read_register(fd, 0xEE, &val);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xEE, (val & 0xFD) | 0x02);
	return ret;
}

#if 0
static int realtek_mst_i2c_spi_set_defaults(int fd)
{
	// 0xFF1B = 0x02;
	int ret = realtek_mst_i2c_spi_write_register(fd, 0x1B, 0x02);
	ret = realtek_mst_i2c_spi_write_register(fd, 0x1C, 0x30);
	ret = realtek_mst_i2c_spi_write_register(fd, 0x1D, 0x1C);
	ret = realtek_mst_i2c_spi_write_register(fd, 0x1E, 0x02);
	ret = realtek_mst_i2c_spi_write_register(fd, 0x1F, 0x00);

	ret = realtek_mst_i2c_spi_write_register(fd, 0x20, 0x1C);
	ret = realtek_mst_i2c_spi_write_register(fd, 0x2C, 0x02);
	ret = realtek_mst_i2c_spi_write_register(fd, 0x2D, 0x00);
	ret = realtek_mst_i2c_spi_write_register(fd, 0x2E, 0x1C);

	ret = realtek_mst_i2c_spi_write_register(fd, 0x62, 0x06);
	ret = realtek_mst_i2c_spi_write_register(fd, 0x6A, 0x03);
	ret = realtek_mst_i2c_spi_write_register(fd, 0x6B, 0x0B);
	ret = realtek_mst_i2c_spi_write_register(fd, 0x6C, 0x00);

	ret = realtek_mst_i2c_spi_write_register(fd, 0xED, 0x88);
	ret = realtek_mst_i2c_spi_write_register(fd, 0xEE, 0x04);

	return ret;
}
#endif

static int realtek_mst_i2c_spi_disable_protection(int fd)
{
	int ret = 0;
	uint8_t val = 0;
	// 0xAB[2:0] = b001

	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF4, 0x9F);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF5, 0x10);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF4, 0xAB);

	ret |= realtek_mst_i2c_spi_read_register(fd, 0xF5, &val);

	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF4, 0x9F);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF5, 0x10);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF4, 0xAB);

	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF5, (val & 0xF8) | 0x01);

	/* Set pin value to high, 0xFFD7[0] = 1. */
	ret |= realtek_mst_i2c_spi_read_register(fd, 0xD7, &val);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xD7, (val & 0xFE) | 0x01);

	return ret;
}

static int realtek_mst_i2c_spi_send_command(const struct flashctx *flash,
				unsigned int writecnt, unsigned int readcnt,
				const unsigned char *writearr,
				unsigned char *readarr)
{
	unsigned i;
	int ret = 0;

	if (writecnt > 4 || readcnt > 3 || writecnt == 0) {
		//msg_perr("%s: Invalid read/write count for send command.\n", __func__);
		return SPI_GENERIC_ERROR;
	}
	//printf("%s: writearr[0]=0x%x\n", __func__, writearr[0]);

	int fd = get_fd_from_context(flash);
	if (fd < 0)
		return SPI_GENERIC_ERROR;

	/* First byte of writearr should be the command value, followed by the value to write. */
	// 0xFF60 = cmd;
	writecnt--;
	uint8_t ctrl_reg_val = (writecnt << 3) | (readcnt << 1) | (2 << 5);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x60, ctrl_reg_val);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x61, writearr[0]);

	for (i = 0; i < writecnt; ++i)
		ret |= realtek_mst_i2c_spi_write_register(fd, 0x64 + i, writearr[i + 1]);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x60, ctrl_reg_val | 0x1);
	if (ret)
		return ret;

	ret = realtek_mst_i2c_spi_wait_command_done(fd, 0x60, 0x01);
	if (ret)
		return ret;

	for (i = 0; i < readcnt; ++i)
		ret |= realtek_mst_i2c_spi_read_register(fd, 0x67 + i, &readarr[i]);

	//for (i = 0; i< readcnt; i++)
	//	printf("DEBUG: readarr[%d]=0x%02x\n", i, readarr[i]);

	return ret;
}

static int realtek_mst_i2c_spi_map_page(int fd, uint8_t block_idx, uint8_t page_idx, uint8_t byte_idx)
{
	int ret = 0;
	ret |= realtek_mst_i2c_spi_write_register(fd, MAP_PAGE_BYTE2, block_idx);
	ret |= realtek_mst_i2c_spi_write_register(fd, MAP_PAGE_BYTE1, page_idx);
	ret |= realtek_mst_i2c_spi_write_register(fd, MAP_PAGE_BYTE0, byte_idx);

	return ret ? SPI_GENERIC_ERROR : 0;
}

static int realtek_mst_i2c_spi_read(struct flashctx *flash, uint8_t *buf,
			unsigned int start, unsigned int len)
{
	unsigned i;
	int ret = 0;

	if (start & 0xff)
		return default_spi_read(flash, buf, start, len);

	int fd = get_fd_from_context(flash);
	if (fd < 0)
		return SPI_GENERIC_ERROR;

	start--;
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x60, 0x46); // **
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x61, OPCODE_READ);
	uint8_t block_idx = start >> 16;
	uint8_t page_idx  = start >>  8;
	uint8_t byte_idx  = start;
	ret |= realtek_mst_i2c_spi_map_page(fd, block_idx, page_idx, byte_idx);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x6a, 0x03);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x60, 0x47); // **
	if (ret)
		return ret;

	ret = realtek_mst_i2c_spi_wait_command_done(fd, 0x60, 0x01);
	if (ret)
		return ret;

	/**
	 * The first byte is just a null, probably a status code?
	 * Advance the read by a offset of one byte and continue.
	 */
	uint8_t dummy;
	realtek_mst_i2c_spi_read_register(fd, MCU_DATA_PORT, &dummy);

	for (i = 0; i < len; i += PAGE_SIZE) {
		ret |= realtek_mst_i2c_spi_read_data(fd, REGISTER_ADDRESS,
				buf + i, min(len - i, PAGE_SIZE));
		if (ret)
			return ret;
	}

	return ret;
}

static int realtek_mst_i2c_spi_write_256(struct flashctx *flash, const uint8_t *buf,
				unsigned int start, unsigned int len)
{
	unsigned i;
	int ret = 0;

	if (start & 0xff)
		return default_spi_write_256(flash, buf, start, len);

	int fd = get_fd_from_context(flash);
	if (fd < 0)
		return SPI_GENERIC_ERROR;

	ret = realtek_mst_i2c_spi_disable_protection(fd);
	if (ret)
		return ret;

	start--;
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x60, 0x46); // **
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x61, OPCODE_WRITE);
	uint8_t block_idx = start >> 16;
	uint8_t page_idx  = start >>  8;
	uint8_t byte_idx  = start;
	ret |= realtek_mst_i2c_spi_map_page(fd, block_idx, page_idx, byte_idx);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x6a, 0x03);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x60, 0x47); // **
	if (ret)
		goto fail;

	ret = realtek_mst_i2c_spi_wait_command_done(fd, 0x60, 0x01);
	if (ret)
		goto fail;

	for (i = 0; i < len; i += PAGE_SIZE) {
		ret |= realtek_mst_i2c_spi_write_data(fd, REGISTER_ADDRESS,
				(uint8_t *)buf + i, min(len - i, PAGE_SIZE));
		if (ret)
			break;
	}

fail:
	/* TODO: re-enable the write protection? */

	return ret;
}

static int realtek_mst_i2c_spi_write_aai(struct flashctx *flash, const uint8_t *buf,
				 unsigned int start, unsigned int len)
{
	msg_perr("%s: AAI write function is not supported.\n", __func__);
	return SPI_GENERIC_ERROR;
}

static struct spi_master spi_master_i2c_realtek_mst = {
	.max_data_read = 16,
	.max_data_write = 8,
	.command = realtek_mst_i2c_spi_send_command,
	.multicommand = default_spi_send_multicommand,
	.read = realtek_mst_i2c_spi_read,
	.write_256 = realtek_mst_i2c_spi_write_256,
	.write_aai = realtek_mst_i2c_spi_write_aai,
};

static int realtek_mst_i2c_spi_shutdown(void *data)
{
	int ret = 0;
        struct realtek_mst_i2c_spi_data *realtek_mst_data =
		(struct realtek_mst_i2c_spi_data *)data;
	int fd = realtek_mst_data->fd;
	ret |= realtek_mst_i2c_spi_reset_mpu(fd);
	i2c_close(fd);
	free(data);

	return ret;
}

static int get_params(int *i2c_bus)
{
	char *bus_str = NULL;
        int ret = SPI_GENERIC_ERROR;

	bus_str = extract_programmer_param("bus");
	if (bus_str) {
		char *bus_suffix;
		errno = 0;
		int bus = (int)strtol(bus_str, &bus_suffix, 10);
		if (errno != 0 || bus_str == bus_suffix) {
			msg_perr("%s: Could not convert 'bus'.\n", __func__);
			goto get_params_done;
		}

		if (bus < 0 || bus > 255) {
			msg_perr("%s: Value for 'bus' is out of range(0-255).\n", __func__);
			goto get_params_done;
		}

		if (strlen(bus_suffix) > 0) {
			msg_perr("%s: Garbage following 'bus' value.\n", __func__);
			goto get_params_done;
		}

		msg_pinfo("Using i2c bus %i.\n", bus);
		*i2c_bus = bus;
		ret = 0;
		goto get_params_done;
	} else {
		msg_perr("%s: Bus number not specified.\n", __func__);
	}
get_params_done:
	if (bus_str)
		free(bus_str);

	return ret;
}

int realtek_mst_i2c_spi_init(void)
{
	int ret = 0;
	int i2c_bus = 0;

	if (get_params(&i2c_bus))
		return SPI_GENERIC_ERROR;

	int fd = i2c_open(i2c_bus, REGISTER_ADDRESS, 0);
	if (fd < 0)
		return fd;

	/* Ensure we are in a known state before entering ISP mode */
	ret |= realtek_mst_i2c_spi_reset_mpu(fd);
	if (ret)
		return ret;

	ret |= realtek_mst_i2c_spi_enter_isp_mode(fd);
	if (ret)
		return ret;
// XXX: maybe make into a mode:defaults cli param?
#if 0
	ret |= realtek_mst_i2c_spi_set_defaults(fd);
	if (ret)
		return ret;
#endif

	struct realtek_mst_i2c_spi_data *data = calloc(1, sizeof(struct realtek_mst_i2c_spi_data));
	if (!data) {
		msg_perr("Unable to allocate space for extra SPI master data.\n");
		return SPI_GENERIC_ERROR;
	}

	data->fd = fd;
	ret |= register_shutdown(realtek_mst_i2c_spi_shutdown, data);

	spi_master_i2c_realtek_mst.data = data;
	ret |= register_spi_master(&spi_master_i2c_realtek_mst);

	return ret;
}
