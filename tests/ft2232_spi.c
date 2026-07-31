/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2026 Kjell Peterson <kjell.peterson@shotover.com>
 */

#include "lifecycle.h"

#if CONFIG_FT2232_SPI == 1

struct ft2232_spi_io_state {
	int bus;
	uint8_t ports[7];
	int ports_len;
	struct ftdi_device_list *devlist;
	int devlist_len;
};

static int ft2232_ftdi_usb_find_all(void *state, struct ftdi_context *ftdi, struct ftdi_device_list **devlist, int vendor, int product)
{
	struct ft2232_spi_io_state *io_state = state;
	*devlist = io_state->devlist;
	return io_state->devlist_len;
}

static int ft2232_libusb_get_bus_number(void *state, libusb_device *dev)
{
	struct ft2232_spi_io_state *io_state = state;
	return io_state->bus;
}

static int ft2232_libusb_get_port_numbers(void *state, libusb_device *dev, uint8_t *port_numbers, int port_numbers_len)
{
	struct ft2232_spi_io_state *io_state = state;

	memcpy(port_numbers, io_state->ports, io_state->ports_len);
	return io_state->ports_len;
}

void ft2232_spi_basic_lifecycle_test_success(void **state)
{
	run_basic_lifecycle(state, NULL, &programmer_ft2232_spi, "");
}

void ft2232_spi_lifecycle_test_usbpath_success(void **state)
{
	struct ftdi_device_list devlist = {
		.next=NULL,
		.dev=NULL // dev is never dereferenced, only passed to mocked libusb functions
	};

	struct ft2232_spi_io_state io_state = {
		.bus = 1,
		.ports = {1, 2, 3, 4},
		.ports_len = 4,
		.devlist = &devlist,
		.devlist_len = 1
	};

	*state = &io_state;

	const struct io_mock ft2232_io = {
		.state = &io_state,
		.libusb_get_bus_number = ft2232_libusb_get_bus_number,
		.libusb_get_port_numbers = ft2232_libusb_get_port_numbers,
		.ftdi_usb_find_all = ft2232_ftdi_usb_find_all,
	};

	run_basic_lifecycle(state, &ft2232_io, &programmer_ft2232_spi, "usbpath=1-1.2.3.4");
}

void ft2232_spi_init_fails_usbpath_no_match(void **state)
{
	struct ftdi_device_list devlist = {
		.next=NULL,
		.dev=NULL // dev is never dereferenced, only passed to mocked libusb functions
	};

	struct ft2232_spi_io_state io_state = {
		.bus = 1,
		.ports = {1, 2, 3, 4},
		.ports_len = 4,
		.devlist = &devlist,
		.devlist_len = 1
	};

	*state = &io_state;

	const struct io_mock ft2232_io = {
		.state = &io_state,
		.libusb_get_bus_number = ft2232_libusb_get_bus_number,
		.libusb_get_port_numbers = ft2232_libusb_get_port_numbers,
		.ftdi_usb_find_all = ft2232_ftdi_usb_find_all,
	};

	run_init_error_path(state, &ft2232_io, &programmer_ft2232_spi, "usbpath=2-1.2.3.4", -4);
	run_init_error_path(state, &ft2232_io, &programmer_ft2232_spi, "usbpath=1-2.2.3.4", -4);
}

void ft2232_spi_init_fails_invalid_usbpath(void **state) {
	run_init_error_path(state, NULL, &programmer_ft2232_spi, "usbpath=", -2);
	run_init_error_path(state, NULL, &programmer_ft2232_spi, "usbpath=1", -2);
	run_init_error_path(state, NULL, &programmer_ft2232_spi, "usbpath=-1-1", -2);
	run_init_error_path(state, NULL, &programmer_ft2232_spi, "usbpath=1.1", -2);
	run_init_error_path(state, NULL, &programmer_ft2232_spi, "usbpath=1-+1", -2);
	run_init_error_path(state, NULL, &programmer_ft2232_spi, "usbpath=1.1.2.3.4.", -2);
	run_init_error_path(state, NULL, &programmer_ft2232_spi, "usbpath=1-1.2.3.4.5.6.7", -2);
}

#else

SKIP_TEST(ft2232_spi_basic_lifecycle_test_success)
SKIP_TEST(ft2232_spi_lifecycle_test_usbpath_success)
SKIP_TEST(ft2232_spi_init_fails_usbpath_no_match)
SKIP_TEST(ft2232_spi_init_fails_invalid_usbpath)

#endif /* CONFIG_FT2232_SPI */
