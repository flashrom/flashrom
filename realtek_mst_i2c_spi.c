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


#define MCU_I2C_SLAVE_ADDR	0x94
#define REGISTER_ADDRESS	(0x94 >> 1)
#define RTK_PAGE_SIZE		128
#define MAX_SPI_WAIT_RETRIES	1000

#define MCU_MODE 0x6F
#define 	MCU_ISP_MODE_MASK 0x80
#define 	START_WRITE_XFER 0xA0
#define 	WRITE_XFER_STATUS_MASK  0x20

#define MCU_DATA_PORT 0x70

#define MAP_PAGE_BYTE2 0x64
#define MAP_PAGE_BYTE1 0x65
#define MAP_PAGE_BYTE0 0x66

//opcodes
#define OPCODE_READ  3
#define OPCODE_WRITE 2

#define GPIO_CONFIG_ADDRESS 0x104F
#define GPIO_VALUE_ADDRESS  0xFE3F

struct realtek_mst_i2c_spi_data {
	int fd;
	bool reset;
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

static int realtek_mst_i2c_spi_wait_command_done(int fd, unsigned int offset, int mask,
		int target, int multiplier)
{
	uint8_t val;
	int tried = 0;
	int ret = 0;
	do {
		ret |= realtek_mst_i2c_spi_read_register(fd, offset, &val);
	} while (!ret && ((val & mask) != target) && ++tried < (MAX_SPI_WAIT_RETRIES*multiplier));

	if (tried == MAX_SPI_WAIT_RETRIES) {
		msg_perr("%s: Time out on sending command.\n", __func__);
		return -MAX_SPI_WAIT_RETRIES;
	}

	return (val & mask) != target ? SPI_GENERIC_ERROR : ret;
}

static int realtek_mst_i2c_spi_enter_isp_mode(int fd)
{
	int ret = realtek_mst_i2c_spi_write_register(fd, MCU_MODE, MCU_ISP_MODE_MASK);
	/* wait for ISP mode enter success */
	ret |= realtek_mst_i2c_spi_wait_command_done(fd, MCU_MODE, MCU_ISP_MODE_MASK, MCU_ISP_MODE_MASK, 1);

	if (ret)
		return ret;

	// set internal osc divider register to default to speed up MCU
	// 0x06A0 = 0x74
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF4, 0x9F);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF5, 0x06);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF4, 0xA0);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF5, 0x74);

	return ret;
}

static int realtek_mst_i2c_execute_write(int fd)
{
	int ret = realtek_mst_i2c_spi_write_register(fd, MCU_MODE, START_WRITE_XFER);
	ret |= realtek_mst_i2c_spi_wait_command_done(fd, MCU_MODE, WRITE_XFER_STATUS_MASK, 0, 1);
	return ret;
}

static int realtek_mst_i2c_spi_reset_mpu(int fd)
{
	uint8_t mcu_mode_val;
	int ret = realtek_mst_i2c_spi_read_register(fd, MCU_MODE, &mcu_mode_val);
	if (ret || (mcu_mode_val & MCU_ISP_MODE_MASK) == 0) {
		msg_perr("%s: MST not in ISP mode, cannot perform MCU reset.\n", __func__);
		return SPI_GENERIC_ERROR;
	}

	// 0xFFEE[1] = 1;
	uint8_t val = 0;
	ret |= realtek_mst_i2c_spi_read_register(fd, 0xEE, &val);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xEE, (val & 0xFD) | 0x02);
	return ret;
}

static int realtek_mst_i2c_spi_select_indexed_register(int fd, uint16_t address)
{
	int ret = 0;

	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF4, 0x9F);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF5, address >> 8);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF4, address & 0xFF);

	return ret;
}

static int realtek_mst_i2c_spi_write_indexed_register(int fd, uint16_t address, uint8_t val)
{
	int ret = 0;

	ret |= realtek_mst_i2c_spi_select_indexed_register(fd, address);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0xF5, val);

	return ret;
}

static int realtek_mst_i2c_spi_read_indexed_register(int fd, uint16_t address, uint8_t *val)
{
	int ret = 0;

	ret |= realtek_mst_i2c_spi_select_indexed_register(fd, address);
	ret |= realtek_mst_i2c_spi_read_register(fd, 0xF5, val);

	return ret;
}


/* Toggle the GPIO pin 88, reserved for write protection pin of the external flash. */
static int realtek_mst_i2c_spi_toggle_gpio_88_strap(int fd, bool toggle)
{
	int ret = 0;
	uint8_t val = 0;

	/* Read register 0x104F into val. */
	ret |= realtek_mst_i2c_spi_read_indexed_register(fd, GPIO_CONFIG_ADDRESS, &val);
	/* Write 0x104F[3:0] = b0001 to enable the toggle of pin value. */
	ret |= realtek_mst_i2c_spi_write_indexed_register(fd, GPIO_CONFIG_ADDRESS, (val & 0xF0) | 0x01);

	/* Read register 0xFE3F into val. */
	ret |= realtek_mst_i2c_spi_read_indexed_register(fd, GPIO_VALUE_ADDRESS, &val);
	/* Write 0xFE3F[0] = b|toggle| to toggle pin value to low/high. */
	ret |= realtek_mst_i2c_spi_write_indexed_register(fd, GPIO_VALUE_ADDRESS, (val & 0xFE) | toggle);

	return ret;
}

static int realtek_mst_i2c_spi_send_command(const struct flashctx *flash,
				unsigned int writecnt, unsigned int readcnt,
				const unsigned char *writearr,
				unsigned char *readarr)
{
	unsigned i;
	int max_timeout_mul = 1;
	int ret = 0;

	if (writecnt > 4 || readcnt > 3 || writecnt == 0) {
		return SPI_GENERIC_ERROR;
	}

	int fd = get_fd_from_context(flash);
	if (fd < 0)
		return SPI_GENERIC_ERROR;

	/* First byte of writearr should be the spi opcode value, followed by the value to write. */
	writecnt--;

	/**
	 * Before dispatching a SPI opcode the MCU register 0x60 requires
	 * the following configuration byte set:
	 *
	 *  BIT0      - start [0] , end [1].
	 *  BITS[1-4] - counts.
	 *  BITS[5-7] - opcode type.
	 *
	 * | bit7 | bit6 | bit5 |
	 * +------+------+------+
	 * |  0   |  1   |  0   | ~ JEDEC_RDID,REMS,READ
	 * |  0   |  1   |  1   | ~ JEDEC_WRSR
	 * |  1   |  0   |  1   | ~ JEDEC_.. erasures.
	 */
	uint8_t ctrl_reg_val = (writecnt << 3) | (readcnt << 1);
	switch (writearr[0]) {
	/* WREN isn't a supported somehow? ignore it. */
	case JEDEC_WREN: return 0;
	/* WRSR requires BIT6 && BIT5 set. */
	case JEDEC_WRSR:
		ctrl_reg_val |= (1 << 5);
		ctrl_reg_val |= (2 << 5);
		break;
	/* Erasures require BIT7 && BIT5 set. */
	case JEDEC_CE_C7:
		max_timeout_mul *= 20; /* chip erasures take much longer! */
	/* FALLTHRU */
	case JEDEC_CE_60:
	case JEDEC_BE_52:
	case JEDEC_BE_D8:
	case JEDEC_BE_D7:
	case JEDEC_SE:
		ctrl_reg_val |= (1 << 5);
		ctrl_reg_val |= (4 << 5);
		break;
	default:
		/* Otherwise things like RDID,REMS,READ require BIT6 */
		ctrl_reg_val |= (2 << 5);
	}
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x60, ctrl_reg_val);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x61, writearr[0]); /* opcode */

	for (i = 0; i < writecnt; ++i)
		ret |= realtek_mst_i2c_spi_write_register(fd, 0x64 + i, writearr[i + 1]);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x60, ctrl_reg_val | 0x1);
	if (ret)
		return ret;

	ret = realtek_mst_i2c_spi_wait_command_done(fd, 0x60, 0x01, 0, max_timeout_mul);
	if (ret)
		return ret;

	for (i = 0; i < readcnt; ++i)
		ret |= realtek_mst_i2c_spi_read_register(fd, 0x67 + i, &readarr[i]);

	return ret;
}

static int realtek_mst_i2c_spi_map_page(int fd, uint32_t addr)
{
	int ret = 0;

	uint8_t block_idx = (addr >> 16) & 0xff;
	uint8_t page_idx  = (addr >>  8) & 0xff;
	uint8_t byte_idx  =  addr        & 0xff;

	ret |= realtek_mst_i2c_spi_write_register(fd, MAP_PAGE_BYTE2, block_idx);
	ret |= realtek_mst_i2c_spi_write_register(fd, MAP_PAGE_BYTE1, page_idx);
	ret |= realtek_mst_i2c_spi_write_register(fd, MAP_PAGE_BYTE0, byte_idx);

	return ret ? SPI_GENERIC_ERROR : 0;
}

static int realtek_mst_i2c_spi_write_page(int fd, uint8_t reg, const uint8_t *buf, unsigned int len)
{
	/**
	 * Using static buffer with maximum possible size,
	 * extra byte is needed for prefixing the data port register at index 0.
	 */
	uint8_t wbuf[RTK_PAGE_SIZE + 1] = { MCU_DATA_PORT };
	if (len > RTK_PAGE_SIZE)
		return SPI_GENERIC_ERROR;

	memcpy(&wbuf[1], buf, len);

	return realtek_mst_i2c_spi_write_data(fd, REGISTER_ADDRESS, wbuf, len + 1);
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
	ret |= realtek_mst_i2c_spi_map_page(fd, start);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x6a, 0x03);
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x60, 0x47); // **
	if (ret)
		return ret;

	ret = realtek_mst_i2c_spi_wait_command_done(fd, 0x60, 0x01, 0, 1);
	if (ret)
		return ret;

	/**
	 * The first byte is just a null, probably a status code?
	 * Advance the read by a offset of one byte and continue.
	 */
	uint8_t dummy;
	realtek_mst_i2c_spi_read_register(fd, MCU_DATA_PORT, &dummy);

	for (i = 0; i < len; i += RTK_PAGE_SIZE) {
		ret |= realtek_mst_i2c_spi_read_data(fd, REGISTER_ADDRESS,
				buf + i, min(len - i, RTK_PAGE_SIZE));
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

	ret |= realtek_mst_i2c_spi_write_register(fd, 0x6D, 0x02); /* write opcode */
	ret |= realtek_mst_i2c_spi_write_register(fd, 0x71, (RTK_PAGE_SIZE - 1)); /* fit len=256 */

	for (i = 0; i < len; i += RTK_PAGE_SIZE) {
		uint16_t page_len = min(len - i, RTK_PAGE_SIZE);
		if (len - i < RTK_PAGE_SIZE)
			ret |= realtek_mst_i2c_spi_write_register(fd, 0x71, page_len-1);
		ret |= realtek_mst_i2c_spi_map_page(fd, start + i);
		if (ret)
			break;

		/* Wait for empty buffer. */
		ret |= realtek_mst_i2c_spi_wait_command_done(fd, MCU_MODE, 0x10, 0x10, 1);
		if (ret)
			break;

		ret |= realtek_mst_i2c_spi_write_page(fd, MCU_DATA_PORT,
				buf + i, page_len);
		if (ret)
			break;
		ret |= realtek_mst_i2c_execute_write(fd);
		if (ret)
			break;
		update_progress(flash, FLASHROM_PROGRESS_WRITE, i + RTK_PAGE_SIZE, len);
	}

	return ret;
}

static int realtek_mst_i2c_spi_write_aai(struct flashctx *flash, const uint8_t *buf,
				 unsigned int start, unsigned int len)
{
	msg_perr("%s: AAI write function is not supported.\n", __func__);
	return SPI_GENERIC_ERROR;
}

static int realtek_mst_i2c_spi_shutdown(void *data)
{
	int ret = 0;
	struct realtek_mst_i2c_spi_data *realtek_mst_data =
		(struct realtek_mst_i2c_spi_data *)data;
	int fd = realtek_mst_data->fd;
	ret |= realtek_mst_i2c_spi_toggle_gpio_88_strap(fd, false);
	if (realtek_mst_data->reset) {
		/*
		 * Return value for reset mpu is not checked since
		 * the return value is not guaranteed to be 0 on a
		 * success reset. Currently there is no way to fix
		 * that. For more details see b:147402710.
		 */
		realtek_mst_i2c_spi_reset_mpu(fd);
	}
	i2c_close(fd);
	free(data);

	return ret;
}

static const struct spi_master spi_master_i2c_realtek_mst = {
	.max_data_read	= 16,
	.max_data_write	= 8,
	.command	= realtek_mst_i2c_spi_send_command,
	.read		= realtek_mst_i2c_spi_read,
	.write_256	= realtek_mst_i2c_spi_write_256,
	.write_aai	= realtek_mst_i2c_spi_write_aai,
	.shutdown	= realtek_mst_i2c_spi_shutdown,
};

static int get_params(const struct programmer_cfg *cfg, bool *reset, bool *enter_isp, bool *allow_brick)
{
	char *param_str;
	int ret = 0;

	*allow_brick = false; /* Default behaviour is to bail. */
	param_str = extract_programmer_param_str(cfg, "allow_brick");
	if (param_str) {
		if (!strcmp(param_str, "yes")) {
			*allow_brick = true;
		} else {
			msg_perr("%s: Incorrect param format, allow_brick=yes.\n", __func__);
			ret = SPI_GENERIC_ERROR;
		}
	}
	free(param_str);

	*reset = false; /* Default behaviour is no MCU reset on tear-down. */
	param_str = extract_programmer_param_str(cfg, "reset_mcu");
	if (param_str) {
		if (param_str[0] == '1') {
			*reset = true;
		} else if (param_str[0] == '0') {
			*reset = false;
		} else {
			msg_perr("%s: Incorrect param format, reset_mcu=1 or 0.\n", __func__);
			ret = SPI_GENERIC_ERROR;
		}
	}
	free(param_str);

	*enter_isp = true; /* Default behaviour is enter ISP on setup. */
	param_str = extract_programmer_param_str(cfg, "enter_isp");
	if (param_str) {
		if (param_str[0] == '1') {
			*enter_isp = true;
		} else if (param_str[0] == '0') {
			*enter_isp = false;
		} else {
			msg_perr("%s: Incorrect param format, enter_isp=1 or 0.\n", __func__);
			ret = SPI_GENERIC_ERROR;
		}
	}
	free(param_str);

	return ret;
}

static int realtek_mst_i2c_spi_init(const struct programmer_cfg *cfg)
{
	int ret = 0;
	bool reset, enter_isp, allow_brick;

	if (get_params(cfg, &reset, &enter_isp, &allow_brick))
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

	int fd = i2c_open_from_programmer_params(cfg, REGISTER_ADDRESS, 0);
	if (fd < 0)
		return fd;

	if (enter_isp) {
		ret |= realtek_mst_i2c_spi_enter_isp_mode(fd);
		if (ret)
			return ret;
	}

	ret |= realtek_mst_i2c_spi_toggle_gpio_88_strap(fd, true);
	if (ret) {
		msg_perr("Unable to toggle gpio 88 strap to True.\n");
		return ret;
	}

	struct realtek_mst_i2c_spi_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for extra SPI master data.\n");
		return SPI_GENERIC_ERROR;
	}

	data->fd = fd;
	data->reset = reset;
	return register_spi_master(&spi_master_i2c_realtek_mst, data);
}

const struct programmer_entry programmer_realtek_mst_i2c_spi = {
	.name			= "realtek_mst_i2c_spi",
	.type			= OTHER,
	.devs.note		= "Device files /dev/i2c-*.\n",
	.init			= realtek_mst_i2c_spi_init,
};
