/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2018 Linaro Limited
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
 * Bit bang driver for the 96Boards Developerbox (a.k.a. Synquacer E-series)
 * on-board debug UART.  The Developerbox implements its debug UART using a
 * CP2102N, a USB to UART bridge which also provides four GPIO pins. On
 * Developerbox these can be hooked up to the onboard SPI NOR FLASH and used
 * for emergency de-brick without any additional hardware programmer. Bit
 * banging over USB is extremely slow compared to a proper SPI programmer so
 * this is only practical as a de-brick tool.
 *
 * Schematic is available here:
 * https://www.96boards.org/documentation/enterprise/developerbox/hardware-docs/
 *
 * To prepare a Developerbox for programming via the debug UART, DSW4 must be
 * changed from the default 00000000 to 10001000 (i.e. DSW4-1 and DSW4-5
 * should be turned on).
 */

#include <stdlib.h>
#include <libusb.h>
#include "programmer.h"
#include "spi.h"

/* Bit positions for each pin. */
#define DEVELOPERBOX_SPI_SCK	0
#define DEVELOPERBOX_SPI_CS	1
#define DEVELOPERBOX_SPI_MISO	2
#define DEVELOPERBOX_SPI_MOSI	3

/* Config request types */
#define REQTYPE_HOST_TO_DEVICE  0x40
#define REQTYPE_DEVICE_TO_HOST  0xc0

/* Config request codes */
#define CP210X_VENDOR_SPECIFIC  0xff

/* CP210X_VENDOR_SPECIFIC */
#define CP210X_WRITE_LATCH      0x37e1
#define CP210X_READ_LATCH       0x00c2

static const struct dev_entry devs_developerbox_spi[] = {
	{0x10c4, 0xea60, OK, "Silicon Labs", "CP2102N USB to UART Bridge Controller"},
	{0},
};

struct devbox_spi_data {
	struct libusb_context *usb_ctx;
	libusb_device_handle *cp210x_handle;
};

static int cp210x_gpio_get(void *spi_data)
{
	int res;
	uint8_t gpio;
	struct devbox_spi_data *data = spi_data;

	res = libusb_control_transfer(data->cp210x_handle, REQTYPE_DEVICE_TO_HOST,
			CP210X_VENDOR_SPECIFIC, CP210X_READ_LATCH,
			0, &gpio, 1, 0);
	if (res < 0) {
		msg_perr("Failed to read GPIO pins (%s)\n", libusb_error_name(res));
		return 0;
	}

	return gpio;
}

static void cp210x_gpio_set(uint8_t val, uint8_t mask, void *spi_data)
{
	int res;
	uint16_t gpio;
	struct devbox_spi_data *data = spi_data;

	gpio = ((val & 0xf) << 8) | (mask & 0xf);

	/* Set relay state on the card */
	res = libusb_control_transfer(data->cp210x_handle, REQTYPE_HOST_TO_DEVICE,
			CP210X_VENDOR_SPECIFIC, CP210X_WRITE_LATCH,
			gpio, NULL, 0, 0);
	if (res < 0)
		msg_perr("Failed to read GPIO pins (%s)\n", libusb_error_name(res));
}

static void cp210x_bitbang_set_cs(int val, void *spi_data)
{
	cp210x_gpio_set(val << DEVELOPERBOX_SPI_CS, 1 << DEVELOPERBOX_SPI_CS, spi_data);
}

static void cp210x_bitbang_set_sck(int val, void *spi_data)
{
	cp210x_gpio_set(val << DEVELOPERBOX_SPI_SCK, 1 << DEVELOPERBOX_SPI_SCK, spi_data);
}

static void cp210x_bitbang_set_mosi(int val, void *spi_data)
{
	cp210x_gpio_set(val << DEVELOPERBOX_SPI_MOSI, 1 << DEVELOPERBOX_SPI_MOSI, spi_data);
}

static int cp210x_bitbang_get_miso(void *spi_data)
{
	return !!(cp210x_gpio_get(spi_data) & (1 << DEVELOPERBOX_SPI_MISO));
}

static void cp210x_bitbang_set_sck_set_mosi(int sck, int mosi, void *spi_data)
{
	cp210x_gpio_set(sck << DEVELOPERBOX_SPI_SCK | mosi << DEVELOPERBOX_SPI_MOSI,
			  1 << DEVELOPERBOX_SPI_SCK |    1 << DEVELOPERBOX_SPI_MOSI,
			  spi_data);
}

static const struct bitbang_spi_master bitbang_spi_master_cp210x = {
	.set_cs			= cp210x_bitbang_set_cs,
	.set_sck		= cp210x_bitbang_set_sck,
	.set_mosi		= cp210x_bitbang_set_mosi,
	.get_miso		= cp210x_bitbang_get_miso,
	.set_sck_set_mosi	= cp210x_bitbang_set_sck_set_mosi,
};

static int developerbox_spi_shutdown(void *spi_data)
{
	struct devbox_spi_data *data = spi_data;

	libusb_close(data->cp210x_handle);
	libusb_exit(data->usb_ctx);

	free(data);
	return 0;
}

static int developerbox_spi_init(const struct programmer_cfg *cfg)
{
	struct libusb_context *usb_ctx;
	libusb_device_handle *cp210x_handle;

	if (libusb_init(&usb_ctx)) {
		msg_perr("Could not initialize libusb!\n");
		return 1;
	}

	char *serialno = extract_programmer_param_str(cfg, "serial");
	if (serialno)
		msg_pdbg("Looking for serial number commencing %s\n", serialno);
	cp210x_handle = usb_dev_get_by_vid_pid_serial(usb_ctx,
			devs_developerbox_spi[0].vendor_id, devs_developerbox_spi[0].device_id, serialno);
	free(serialno);
	if (!cp210x_handle) {
		msg_perr("Could not find a Developerbox programmer on USB.\n");
		goto err_exit;
	}

	struct devbox_spi_data *data = calloc(1, sizeof(*data));
	if (!data) {
		msg_perr("Unable to allocate space for SPI master data\n");
		goto err_exit;
	}
	data->usb_ctx = usb_ctx;
	data->cp210x_handle = cp210x_handle;

	if (register_shutdown(developerbox_spi_shutdown, data)) {
		free(data);
		goto err_exit;
	}

	if (register_spi_bitbang_master(&bitbang_spi_master_cp210x, data))
		return 1; /* shutdown function does the cleanup */

	return 0;

err_exit:
	if (cp210x_handle)
		libusb_close(cp210x_handle);
	libusb_exit(usb_ctx);
	return 1;
}

const struct programmer_entry programmer_developerbox = {
	.name			= "developerbox",
	.type			= USB,
	.devs.dev		= devs_developerbox_spi,
	.init			= developerbox_spi_init,
};
