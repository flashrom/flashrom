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

#define REGISTER_ADDRESS	(0x94 >> 1)
#define PAGE_ADDRESS		(0x9e >> 1)
#define LSPCON_PAGE_SIZE	256
#define MAX_SPI_WAIT_RETRIES	1000

#define CLT2_SPI		0x82
#define SPIEDID_BASE_ADDR2	0x8d
#define ROMADDR_BYTE1		0x8e
#define ROMADDR_BYTE2		0x8f
#define SWSPI_WDATA		0x90
 #define SWSPI_WDATA_CLEAR_STATUS		0x00
 #define SWSPI_WDATA_WRITE_REGISTER		0x01
 #define SWSPI_WDATA_READ_REGISTER		0x05
 #define SWSPI_WDATA_ENABLE_REGISTER		0x06
 #define SWSPI_WDATA_SECTOR_ERASE		0x20
 #define SWSPI_WDATA_PROTECT_BP			0x8c
#define SWSPI_RDATA		0x91
#define SWSPI_LEN		0x92
#define SWSPICTL		0x93
 #define SWSPICTL_ACCESS_TRIGGER			1
 #define SWSPICTL_CLEAR_PTR			(1 << 1)
 #define SWSPICTL_NO_READ			(1 << 2)
 #define SWSPICTL_ENABLE_READBACK		(1 << 3)
 #define SWSPICTL_MOT				(1 << 4)
#define SPISTATUS		0x9e
 #define SPISTATUS_BYTE_PROGRAM_FINISHED		0
 #define SPISTATUS_BYTE_PROGRAM_IN_IF		1
 #define SPISTATUS_BYTE_PROGRAM_SEND_DONE	(1 << 1)
 #define SPISTATUS_SECTOR_ERASE_FINISHED		0
 #define SPISTATUS_SECTOR_ERASE_IN_IF		(1 << 2)
 #define SPISTATUS_SECTOR_ERASE_SEND_DONE	(1 << 3)
 #define SPISTATUS_CHIP_ERASE_FINISHED		0
 #define SPISTATUS_CHIP_ERASE_IN_IF		(1 << 4)
 #define SPISTATUS_CHIP_ERASE_SEND_DONE		(1 << 5)
 #define SPISTATUS_FW_UPDATE_ENABLE		(1 << 6)
#define WRITE_PROTECTION	0xb3
 #define WRITE_PROTECTION_ON			0
 #define WRITE_PROTECTION_OFF			0x10
#define MPU			0xbc
#define PAGE_HW_WRITE		0xda
 #define PAGE_HW_WRITE_DISABLE			0
 #define PAGE_HW_COFIG_REGISTER			0xaa
 #define PAGE_HW_WRITE_ENABLE			0x55

struct lspcon_i2c_spi_data {
	int fd;
};

typedef struct {
	uint8_t command;
	const uint8_t *data;
	uint8_t data_size;
	uint8_t control;
} packet_t;

static int lspcon_i2c_spi_write_data(int fd, uint16_t addr, void *buf, uint16_t len)
{
	i2c_buffer_t data;
	if (i2c_buffer_t_fill(&data, buf, len))
		return SPI_GENERIC_ERROR;

	return i2c_write(fd, addr, &data) == len ? 0 : SPI_GENERIC_ERROR;
}

static int lspcon_i2c_spi_read_data(int fd, uint16_t addr, void *buf, uint16_t len)
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
        const struct lspcon_i2c_spi_data *data =
		(const struct lspcon_i2c_spi_data *)flash->mst->spi.data;

	return data->fd;
}

static int lspcon_i2c_spi_write_register(int fd, uint8_t i2c_register, uint8_t value)
{
	uint8_t command[] = { i2c_register, value };
	return lspcon_i2c_spi_write_data(fd, REGISTER_ADDRESS, command, 2);
}

static int lspcon_i2c_spi_read_register(int fd, uint8_t i2c_register, uint8_t *value)
{
	uint8_t command[] = { i2c_register };
	int ret = lspcon_i2c_spi_write_data(fd, REGISTER_ADDRESS, command, 1);
	ret |= lspcon_i2c_spi_read_data(fd, REGISTER_ADDRESS, value, 1);

	return ret ? SPI_GENERIC_ERROR : 0;
}

static int lspcon_i2c_spi_register_control(int fd, packet_t *packet)
{
	int i;
	int ret = lspcon_i2c_spi_write_register(fd, SWSPI_WDATA, packet->command);
	if (ret)
		return ret;

	/* Higher 4 bits are read size. */
	int write_size = packet->data_size & 0x0f;
	for (i = 0; i < write_size; ++i) {
		ret |= lspcon_i2c_spi_write_register(fd, SWSPI_WDATA, packet->data[i]);
	}

	ret |= lspcon_i2c_spi_write_register(fd, SWSPI_LEN, packet->data_size);
	ret |= lspcon_i2c_spi_write_register(fd, SWSPICTL, packet->control);

	return ret;
}

static int lspcon_i2c_spi_wait_command_done(int fd, unsigned int offset, int mask)
{
	uint8_t val;
	int tried = 0;
	int ret = 0;
	do {
		ret |= lspcon_i2c_spi_read_register(fd, offset, &val);
	} while (!ret && (val & mask) && ++tried < MAX_SPI_WAIT_RETRIES);

	if (tried == MAX_SPI_WAIT_RETRIES) {
		msg_perr("%s: Time out on sending command.\n", __func__);
		return -MAX_SPI_WAIT_RETRIES;
	}

	return (val & mask) ? SPI_GENERIC_ERROR : ret;
}

static int lspcon_i2c_spi_wait_rom_free(int fd)
{
	uint8_t val;
	int tried = 0;
	int ret = 0;
	ret |= lspcon_i2c_spi_wait_command_done(fd, SPISTATUS,
		SPISTATUS_SECTOR_ERASE_IN_IF | SPISTATUS_SECTOR_ERASE_SEND_DONE);
	if (ret)
		return ret;

	do {
		packet_t packet = { SWSPI_WDATA_READ_REGISTER, NULL, 0,  SWSPICTL_ACCESS_TRIGGER };
		ret |= lspcon_i2c_spi_register_control(fd, &packet);
		ret |= lspcon_i2c_spi_wait_command_done(fd, SWSPICTL, SWSPICTL_ACCESS_TRIGGER);
		ret |= lspcon_i2c_spi_read_register(fd, SWSPI_RDATA, &val);
	} while (!ret && (val & SWSPICTL_ACCESS_TRIGGER) && ++tried < MAX_SPI_WAIT_RETRIES);

	if (tried == MAX_SPI_WAIT_RETRIES) {
		msg_perr("%s: Time out on waiting ROM free.\n", __func__);
		return -MAX_SPI_WAIT_RETRIES;
	}

	return (val & SWSPICTL_ACCESS_TRIGGER) ? SPI_GENERIC_ERROR : ret;
}

static int lspcon_i2c_spi_toggle_register_protection(int fd, int toggle)
{
	return lspcon_i2c_spi_write_register(fd, WRITE_PROTECTION,
		toggle ? WRITE_PROTECTION_OFF : WRITE_PROTECTION_ON);
}

static int lspcon_i2c_spi_enable_write_status_register(int fd)
{
	int ret = lspcon_i2c_spi_toggle_register_protection(fd, 1);
	packet_t packet = {
		SWSPI_WDATA_ENABLE_REGISTER, NULL, 0, SWSPICTL_ACCESS_TRIGGER | SWSPICTL_NO_READ };
	ret |= lspcon_i2c_spi_register_control(fd, &packet);
	ret |= lspcon_i2c_spi_toggle_register_protection(fd, 0);

	return ret;
}

static int lspcon_i2c_spi_enable_write_status_register_protection(int fd)
{
	int ret = lspcon_i2c_spi_toggle_register_protection(fd, 1);
	uint8_t data[] = { SWSPI_WDATA_PROTECT_BP };
	packet_t packet = {
		SWSPI_WDATA_WRITE_REGISTER, data, 1, SWSPICTL_ACCESS_TRIGGER | SWSPICTL_NO_READ };
	ret |= lspcon_i2c_spi_register_control(fd, &packet);
	ret |= lspcon_i2c_spi_toggle_register_protection(fd, 0);

	return ret;
}

static int lspcon_i2c_spi_disable_protection(int fd)
{
	int ret = lspcon_i2c_spi_toggle_register_protection(fd, 1);
	uint8_t data[] = { SWSPI_WDATA_CLEAR_STATUS };
	packet_t packet = {
		SWSPI_WDATA_WRITE_REGISTER, data, 1, SWSPICTL_ACCESS_TRIGGER | SWSPICTL_NO_READ };
	ret |= lspcon_i2c_spi_register_control(fd, &packet);
	ret |= lspcon_i2c_spi_toggle_register_protection(fd, 0);

	return ret;
}

static int lspcon_i2c_spi_disable_hw_write(int fd)
{
	return lspcon_i2c_spi_write_register(fd, PAGE_HW_WRITE, PAGE_HW_WRITE_DISABLE);
}

static int lspcon_i2c_spi_enable_write_protection(int fd)
{
	int ret = lspcon_i2c_spi_enable_write_status_register(fd);
	ret |= lspcon_i2c_spi_enable_write_status_register_protection(fd);
	ret |= lspcon_i2c_spi_wait_rom_free(fd);
	ret |= lspcon_i2c_spi_disable_hw_write(fd);

	return ret;
}

static int lspcon_i2c_spi_disable_all_protection(int fd)
{
	int ret = lspcon_i2c_spi_enable_write_status_register(fd);
	ret |= lspcon_i2c_spi_disable_protection(fd);
	ret |= lspcon_i2c_spi_wait_rom_free(fd);

	return ret;
}

static int lspcon_i2c_spi_send_command(const struct flashctx *flash,
				unsigned int writecnt, unsigned int readcnt,
				const unsigned char *writearr,
				unsigned char *readarr)
{
	unsigned int i;
	if (writecnt > 16 || readcnt > 16 || writecnt == 0) {
		msg_perr("%s: Invalid read/write count for send command.\n",
                         __func__);
		return SPI_GENERIC_ERROR;
	}

	int fd = get_fd_from_context(flash);
	if (fd < 0)
		return SPI_GENERIC_ERROR;

	int ret = lspcon_i2c_spi_disable_all_protection(fd);
	ret |= lspcon_i2c_spi_enable_write_status_register(fd);
	ret |= lspcon_i2c_spi_toggle_register_protection(fd, 1);

	/* First byte of writearr should be the command value, followed by the value to write.
	   Read length occupies 4 bit and represents 16 level, thus if read 1 byte,
           read length should be set 0. */
	packet_t packet = {
		writearr[0], &writearr[1], (writecnt - 1) | ((readcnt - 1) << 4),
		SWSPICTL_ACCESS_TRIGGER | (readcnt ? 0 : SWSPICTL_NO_READ),
	};

	ret |= lspcon_i2c_spi_register_control(fd, &packet);
	ret |= lspcon_i2c_spi_wait_command_done(fd, SWSPICTL, SWSPICTL_ACCESS_TRIGGER);
	ret |= lspcon_i2c_spi_toggle_register_protection(fd, 0);
	if (ret)
		return ret;

	for (i = 0; i < readcnt; ++i) {
		ret |= lspcon_i2c_spi_read_register(fd, SWSPI_RDATA, &readarr[i]);
	}

	ret |= lspcon_i2c_spi_wait_rom_free(fd);

	return ret;
}

static int lspcon_i2c_spi_enable_hw_write(int fd)
{
	int ret = 0;
	ret |= lspcon_i2c_spi_write_register(fd, PAGE_HW_WRITE, PAGE_HW_COFIG_REGISTER);
	ret |= lspcon_i2c_spi_write_register(fd, PAGE_HW_WRITE, PAGE_HW_WRITE_ENABLE);
	ret |= lspcon_i2c_spi_write_register(fd, PAGE_HW_WRITE, 0x50);
	ret |= lspcon_i2c_spi_write_register(fd, PAGE_HW_WRITE, 0x41);
	ret |= lspcon_i2c_spi_write_register(fd, PAGE_HW_WRITE, 0x52);
	ret |= lspcon_i2c_spi_write_register(fd, PAGE_HW_WRITE, 0x44);

	return ret;
}

static int lspcon_i2c_clt2_spi_reset(int fd)
{
	int ret = 0;
	ret |= lspcon_i2c_spi_write_register(fd, CLT2_SPI, 0x20);
	struct timespec wait_100ms = { 0, (unsigned)1e8 };
	nanosleep(&wait_100ms, NULL);
	ret |= lspcon_i2c_spi_write_register(fd, CLT2_SPI, 0x00);

	return ret;
}

static int lspcon_i2c_spi_set_mpu_active(int fd, int running)
{
	int ret = 0;
	// Cmd mode
	ret |= lspcon_i2c_spi_write_register(fd, MPU, 0xc0);
	// Stop or release MPU
	ret |= lspcon_i2c_spi_write_register(fd, MPU, running ? 0 : 0x40);

	return ret;
}

static int lspcon_i2c_spi_map_page(int fd, unsigned int offset)
{
	int ret = 0;
	/* Page number byte, need to / LSPCON_PAGE_SIZE. */
	ret |= lspcon_i2c_spi_write_register(fd, ROMADDR_BYTE1, (offset >> 8) & 0xff);
	ret |= lspcon_i2c_spi_write_register(fd, ROMADDR_BYTE2, (offset >> 16));

	return ret ? SPI_GENERIC_ERROR : 0;
}

static int lspcon_i2c_spi_read(struct flashctx *flash, uint8_t *buf,
			unsigned int start, unsigned int len)
{
	unsigned int i;
	int ret = 0;
	if (start & 0xff)
		return default_spi_read(flash, buf, start, len);

	int fd = get_fd_from_context(flash);
	if (fd < 0)
		return SPI_GENERIC_ERROR;

	for (i = 0; i < len; i += LSPCON_PAGE_SIZE) {
		ret |= lspcon_i2c_spi_map_page(fd, start + i);
		ret |= lspcon_i2c_spi_read_data(fd, PAGE_ADDRESS, buf + i, min(len - i, LSPCON_PAGE_SIZE));
		update_progress(flash, FLASHROM_PROGRESS_READ, i + LSPCON_PAGE_SIZE, len);
	}

	return ret;
}

static int lspcon_i2c_spi_write_page(int fd, const uint8_t *buf, unsigned int len)
{
	/**
         * Using static buffer with maximum possible size,
         * extra byte is needed for prefixing zero at index 0.
         */
	uint8_t write_buffer[LSPCON_PAGE_SIZE + 1] = { 0 };
	if (len > LSPCON_PAGE_SIZE)
		return SPI_GENERIC_ERROR;

	/* First byte represents the writing offset and should always be zero. */
	memcpy(&write_buffer[1], buf, len);

	return lspcon_i2c_spi_write_data(fd, PAGE_ADDRESS, write_buffer, len + 1);
}

static int lspcon_i2c_spi_write_256(struct flashctx *flash, const uint8_t *buf,
				unsigned int start, unsigned int len)
{
	int ret = 0;
	if (start & 0xff)
		return default_spi_write_256(flash, buf, start, len);

	int fd = get_fd_from_context(flash);
	if (fd < 0)
		return SPI_GENERIC_ERROR;

	ret |= lspcon_i2c_spi_disable_all_protection(fd);
	/* Enable hardware write and reset clt2SPI interface. */
	ret |= lspcon_i2c_spi_enable_hw_write(fd);
	ret |= lspcon_i2c_clt2_spi_reset(fd);

	for (unsigned int i = 0; i < len; i += LSPCON_PAGE_SIZE) {
		ret |= lspcon_i2c_spi_map_page(fd, start + i);
		ret |= lspcon_i2c_spi_write_page(fd, buf + i, min(len - i, LSPCON_PAGE_SIZE));
		update_progress(flash, FLASHROM_PROGRESS_WRITE, i + LSPCON_PAGE_SIZE, len);
	}

	ret |= lspcon_i2c_spi_enable_write_protection(fd);
	ret |= lspcon_i2c_spi_disable_hw_write(fd);

	return ret;
}

static int lspcon_i2c_spi_write_aai(struct flashctx *flash, const uint8_t *buf,
				 unsigned int start, unsigned int len)
{
	msg_perr("%s: AAI write function is not supported.\n",
                 __func__);
	return SPI_GENERIC_ERROR;
}

static int lspcon_i2c_spi_shutdown(void *data)
{
	int ret = 0;
        struct lspcon_i2c_spi_data *lspcon_data =
		(struct lspcon_i2c_spi_data *)data;
	int fd = lspcon_data->fd;

	ret |= lspcon_i2c_spi_enable_write_protection(fd);
	ret |= lspcon_i2c_spi_toggle_register_protection(fd, 0);
	ret |= lspcon_i2c_spi_set_mpu_active(fd, 1);
	i2c_close(fd);
	free(data);

	return ret;
}

static const struct spi_master spi_master_i2c_lspcon = {
	.max_data_read	= 16,
	.max_data_write	= 12,
	.command	= lspcon_i2c_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= lspcon_i2c_spi_read,
	.write_256	= lspcon_i2c_spi_write_256,
	.write_aai	= lspcon_i2c_spi_write_aai,
	.shutdown	= lspcon_i2c_spi_shutdown,
	.probe_opcode	= default_spi_probe_opcode,
};

static int lspcon_i2c_spi_init(void)
{
	int fd = i2c_open_from_programmer_params(REGISTER_ADDRESS, 0);
	if (fd < 0)
		return fd;

	int ret = lspcon_i2c_spi_set_mpu_active(fd, 0);
	if (ret) {
		msg_perr("%s: call to set_mpu_active failed.\n", __func__);
		i2c_close(fd);
		return ret;
	}

	struct lspcon_i2c_spi_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for extra SPI master data.\n");
		i2c_close(fd);
		return SPI_GENERIC_ERROR;
	}

	data->fd = fd;

	return register_spi_master(&spi_master_i2c_lspcon, data);
}

const struct programmer_entry programmer_lspcon_i2c_spi = {
	.name			= "lspcon_i2c_spi",
	.type			= OTHER,
	.devs.note		= "Device files /dev/i2c-*.\n",
	.init			= lspcon_i2c_spi_init,
	.map_flash_region	= fallback_map,
	.unmap_flash_region	= fallback_unmap,
	.delay			= internal_delay,
};
