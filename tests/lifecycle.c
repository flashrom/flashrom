/*
 * This file is part of the flashrom project.
 *
 * Copyright 2021 Google LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <include/test.h>
#include <string.h>
#include <linux/spi/spidev.h>

#include "libflashrom.h"
#include "io_mock.h"
#include "programmer.h"
#include "spi.h"

static void probe_chip(const struct programmer_entry *prog,
			struct flashrom_programmer *flashprog,
			const char *const chip_name)
{
	struct flashrom_flashctx *flashctx;

	printf("Testing flashrom_flash_probe for programmer=%s, chip=%s ... \n", prog->name, chip_name);
	assert_int_equal(0, flashrom_flash_probe(&flashctx, flashprog, chip_name));
	printf("... flashrom_flash_probe for programmer=%s successful\n", prog->name);

	flashrom_flash_release(flashctx); /* cleanup */
}

static void run_lifecycle(void **state, const struct programmer_entry *prog,
				const char *param, const char *const chip_name,
				void (*action)(const struct programmer_entry *prog,
						struct flashrom_programmer *flashprog,
						const char *const chip_name))
{
	(void) state; /* unused */

	struct flashrom_programmer *flashprog;
	char *param_dup = strdup(param);

	printf("Testing flashrom_programmer_init for programmer=%s ...\n", prog->name);
	assert_int_equal(0, flashrom_programmer_init(&flashprog, prog->name, param_dup));
	printf("... flashrom_programmer_init for programmer=%s successful\n", prog->name);

	if (action)
		action(prog, flashprog, chip_name);

	printf("Testing flashrom_programmer_shutdown for programmer=%s ...\n", prog->name);
	assert_int_equal(0, flashrom_programmer_shutdown(flashprog));
	printf("... flashrom_programmer_shutdown for programmer=%s successful\n", prog->name);

	free(param_dup);
}

static void run_basic_lifecycle(void **state, const struct programmer_entry *prog, const char *param)
{
	/* Basic lifecycle only does init and shutdown,
	 * so neither chip name nor action is needed. */
	run_lifecycle(state, prog, param, NULL /* chip_name */, NULL /* action */);
}

static void run_probe_lifecycle(void **state, const struct programmer_entry *prog,
                                 const char *param, const char *const chip_name)
{
	/* Each probe lifecycle should run independently, without cache. */
	clear_spi_id_cache();
	run_lifecycle(state, prog, param, chip_name, &probe_chip);
}

void dummy_basic_lifecycle_test_success(void **state)
{
#if CONFIG_DUMMY == 1
	run_basic_lifecycle(state, &programmer_dummy, "bus=parallel+lpc+fwh+spi");
#else
	skip();
#endif
}

void dummy_probe_lifecycle_test_success(void **state)
{
#if CONFIG_DUMMY == 1
	run_probe_lifecycle(state, &programmer_dummy, "bus=spi,emulate=W25Q128FV", "W25Q128.V");
#else
	skip();
#endif
}

void nicrealtek_basic_lifecycle_test_success(void **state)
{
#if CONFIG_NICREALTEK == 1
	run_basic_lifecycle(state, &programmer_nicrealtek, "");
#else
	skip();
#endif
}

static ssize_t raiden_debug_libusb_get_device_list(void *state, libusb_context *ctx, libusb_device ***list)
{
	*list = calloc(1, sizeof(**list));

	/*
	 * libusb_device is opaque type, it is tossed around between libusb functions but always
	 * stays opaque to the caller.
	 * Given that all libusb functions are mocked in tests, and raiden_debug test is mocking
	 * only one device, we don't need to initialise libusb_device.
	 */
	return 1;
}

static void raiden_debug_libusb_free_device_list(void *state, libusb_device **list, int unref_devices)
{
	free(list);
}

static int raiden_debug_libusb_get_device_descriptor(
		void *state, libusb_device *dev, struct libusb_device_descriptor *desc)
{
	desc->idVendor = 0x18D1; /* GOOGLE_VID */
	desc->idProduct = 0;
	desc->bNumConfigurations = 1;

	return 0;
}

static int raiden_debug_libusb_get_config_descriptor(
		void *state, libusb_device *dev, uint8_t config_index, struct libusb_config_descriptor **config)
{
	*config = calloc(1, sizeof(**config));

	struct libusb_endpoint_descriptor *tmp_endpoint = calloc(2, sizeof(*tmp_endpoint));
	struct libusb_interface_descriptor *tmp_interface_desc = calloc(1, sizeof(*tmp_interface_desc));
	struct libusb_interface *tmp_interface = calloc(1, sizeof(*tmp_interface));

	/* in endpoint */
	tmp_endpoint[0].bEndpointAddress = 0x80;
	tmp_endpoint[0].bmAttributes = 0x2;
	/* out endpoint */
	tmp_endpoint[1].bEndpointAddress = 0x0;
	tmp_endpoint[1].bmAttributes = 0x2;

	tmp_interface_desc->bInterfaceClass = 0xff; /* LIBUSB_CLASS_VENDOR_SPEC */
	tmp_interface_desc->bInterfaceSubClass = 0x51; /* GOOGLE_RAIDEN_SPI_SUBCLASS */
	tmp_interface_desc->bInterfaceProtocol = 0x01; /* GOOGLE_RAIDEN_SPI_PROTOCOL_V1 */
	tmp_interface_desc->bNumEndpoints = 2; /* in_endpoint and out_endpoint */
	tmp_interface_desc->endpoint = tmp_endpoint;

	tmp_interface->num_altsetting = 1;
	tmp_interface->altsetting = tmp_interface_desc;

	(*config)->bConfigurationValue = 0;
	(*config)->bNumInterfaces = 1;
	(*config)->interface = tmp_interface;

	return 0;
}

static void raiden_debug_libusb_free_config_descriptor(void *state, struct libusb_config_descriptor *config)
{
	free((void *)config->interface->altsetting->endpoint);
	free((void *)config->interface->altsetting);
	free((void *)config->interface);
	free(config);
}

void raiden_debug_basic_lifecycle_test_success(void **state)
{
#if CONFIG_RAIDEN_DEBUG_SPI == 1
	const struct io_mock raiden_debug_io = {
		.libusb_get_device_list = raiden_debug_libusb_get_device_list,
		.libusb_free_device_list = raiden_debug_libusb_free_device_list,
		.libusb_get_device_descriptor = raiden_debug_libusb_get_device_descriptor,
		.libusb_get_config_descriptor = raiden_debug_libusb_get_config_descriptor,
		.libusb_free_config_descriptor = raiden_debug_libusb_free_config_descriptor,
	};

	/*
	 * 12 is the length of programmer param string for 3-digit address.
	 * Address can be max 3-digit because it needs to fit into uint8_t.
	 */
	char raiden_debug_param[12];
	snprintf(raiden_debug_param, 12, "address=%d", USB_DEVICE_ADDRESS);

	io_mock_register(&raiden_debug_io);

	run_basic_lifecycle(state, &programmer_raiden_debug_spi, raiden_debug_param);

	io_mock_register(NULL);
#else
	skip();
#endif
}

int dediprog_libusb_init(void *state, libusb_context **ctx)
{
	*ctx = not_null();
	return 0;
}

int dediprog_libusb_control_transfer(void *state,
					libusb_device_handle *devh,
					uint8_t bmRequestType,
					uint8_t bRequest,
					uint16_t wValue,
					uint16_t wIndex,
					unsigned char *data,
					uint16_t wLength,
					unsigned int timeout)
{
	if (bRequest == 0x08 /* dediprog_cmds CMD_READ_PROG_INFO */) {
		/* Provide dediprog Device String into data buffer */
		memcpy(data, "SF600 V:7.2.2   ", wLength);
	}
	return wLength;
}

void dediprog_basic_lifecycle_test_success(void **state)
{
#if CONFIG_DEDIPROG == 1
	const struct io_mock dediprog_io = {
		.libusb_init = dediprog_libusb_init,
		.libusb_control_transfer = dediprog_libusb_control_transfer,
	};

	io_mock_register(&dediprog_io);

	run_basic_lifecycle(state, &programmer_dediprog, "voltage=3.5V");

	io_mock_register(NULL);
#else
	skip();
#endif
}

struct linux_mtd_io_state {
	char *fopen_path;
};

FILE *linux_mtd_fopen(void *state, const char *pathname, const char *mode)
{
	struct linux_mtd_io_state *io_state = state;

	io_state->fopen_path = strdup(pathname);

	return not_null();
}

size_t linux_mtd_fread(void *state, void *buf, size_t size, size_t len, FILE *fp)
{
	struct linux_mtd_fread_mock_entry {
		const char *path;
		const char *data;
	};
	const struct linux_mtd_fread_mock_entry fread_mock_map[] = {
		{ "/sys/class/mtd/mtd0//type",            "nor"    },
		{ "/sys/class/mtd/mtd0//name",            "Device" },
		{ "/sys/class/mtd/mtd0//flags",           ""       },
		{ "/sys/class/mtd/mtd0//size",            "1024"   },
		{ "/sys/class/mtd/mtd0//erasesize",       "512"    },
		{ "/sys/class/mtd/mtd0//numeraseregions", "0"      },
	};

	struct linux_mtd_io_state *io_state = state;
	unsigned int i;

	if (!io_state->fopen_path)
		return 0;

	for (i = 0; i < ARRAY_SIZE(fread_mock_map); i++) {
		const struct linux_mtd_fread_mock_entry *entry = &fread_mock_map[i];

		if (!strcmp(io_state->fopen_path, entry->path)) {
			size_t data_len = min(size * len, strlen(entry->data));
			memcpy(buf, entry->data, data_len);
			return data_len;
		}
	}

	return 0;
}

int linux_mtd_fclose(void *state, FILE *fp)
{
	struct linux_mtd_io_state *io_state = state;

	free(io_state->fopen_path);

	return 0;
}

void linux_mtd_probe_lifecycle_test_success(void **state)
{
#if CONFIG_LINUX_MTD == 1
	struct linux_mtd_io_state linux_mtd_io_state = { NULL };
	const struct io_mock linux_mtd_io = {
		.state	= &linux_mtd_io_state,
		.fopen	= linux_mtd_fopen,
		.fread	= linux_mtd_fread,
		.fclose = linux_mtd_fclose,
	};

	io_mock_register(&linux_mtd_io);

	run_probe_lifecycle(state, &programmer_linux_mtd, "", "Opaque flash chip");

	io_mock_register(NULL);
#else
	skip();
#endif
}

static int linux_spi_ioctl(void *state, int fd, unsigned long request, va_list args) {

	if (request == SPI_IOC_MESSAGE(2)) { /* ioctl code for read request */
		struct spi_ioc_transfer* msg = va_arg(args, struct spi_ioc_transfer*);

		/* First message has write array and write count */
		unsigned int writecnt = msg[0].len;
		unsigned char *writearr = (unsigned char *)msg[0].tx_buf;
		/* Second message has read array and read count */
		unsigned int readcnt = msg[1].len;

		/* Detect probing */
		if (writecnt == 1 && writearr[0] == JEDEC_RDID && readcnt == 3) {
			/* We need to populate read array. */
			unsigned char *readarr = (unsigned char *)msg[1].rx_buf;
			readarr[0] = 0xEF; /* WINBOND_NEX_ID */
			readarr[1] = 0x40; /* WINBOND_NEX_W25Q128_V left byte */
			readarr[2] = 0x18; /* WINBOND_NEX_W25Q128_V right byte */
		}
	}

	return 0;
}

char *linux_spi_fgets(void *state, char *buf, int len, FILE *fp)
{
	/* Emulate reading max buffer size from sysfs. */
	const char *max_buf_size = "1048576";

	return memcpy(buf, max_buf_size, min(len, strlen(max_buf_size) + 1));
}

void linux_spi_probe_lifecycle_test_success(void **state)
{
	/*
	 * Current implementation tests a particular path of the init procedure.
	 * Specifically, it is reading the buffer size from sysfs.
	 */
#if CONFIG_LINUX_SPI == 1
	const struct io_mock linux_spi_io = {
		.fgets	= linux_spi_fgets,
		.ioctl	= linux_spi_ioctl,
	};

	io_mock_register(&linux_spi_io);

	run_probe_lifecycle(state, &programmer_linux_spi, "dev=/dev/null", "W25Q128.V");

	io_mock_register(NULL);
#else
	skip();
#endif
}

#define REALTEK_MST_MOCK_FD 0x10ec

static int realtek_mst_open(void *state, const char *pathname, int flags)
{
	assert_string_equal(pathname, "/dev/i2c-254");
	assert_int_equal(flags & O_RDWR, O_RDWR);
	return REALTEK_MST_MOCK_FD;
}

static int realtek_mst_ioctl(void *state, int fd, unsigned long request, va_list args)
{
	assert_int_equal(fd, REALTEK_MST_MOCK_FD);
	assert_int_equal(request, I2C_SLAVE);
	/* Only access to I2C address 0x4a is expected */
	unsigned long addr = va_arg(args, unsigned long);
	assert_int_equal(addr, 0x4a);

	return 0;
}

static int realtek_mst_read(void *state, int fd, void *buf, size_t sz)
{
	assert_int_equal(fd, REALTEK_MST_MOCK_FD);
	assert_int_equal(sz, 1);
	return sz;
}

static int realtek_mst_write(void *state, int fd, const void *buf, size_t sz)
{
	assert_int_equal(fd, REALTEK_MST_MOCK_FD);
	const LargestIntegralType accepted_sizes[] = {1, 2};
	assert_in_set(sz, accepted_sizes, ARRAY_SIZE(accepted_sizes));
	return sz;
}

void realtek_mst_basic_lifecycle_test_success(void **state)
{
#if CONFIG_REALTEK_MST_I2C_SPI == 1
	const struct io_mock realtek_mst_io = {
		.open = realtek_mst_open,
		.ioctl = realtek_mst_ioctl,
		.read = realtek_mst_read,
		.write = realtek_mst_write,
	};
	io_mock_register(&realtek_mst_io);

	run_basic_lifecycle(state, &programmer_realtek_mst_i2c_spi, "bus=254,enter-isp=0");

	io_mock_register(NULL);
#else
	skip();
#endif /* CONFIG_REALTEK_I2C_SPI */
}
