/*
 * This file is part of the flashrom project.
 *
 * Copyright 2020 Google LLC
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
#include "io_mock.h"
#include "tests.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* redefinitions/wrapping */
#define LOG_ME printf("%s is called\n", __func__)
#define MOCK_HANDLE 2021

static const struct io_mock *current_io = NULL;

void io_mock_register(const struct io_mock *io)
{
	current_io = io;
}

/* Workaround for https://github.com/clibs/cmocka/issues/17 */
char *__wrap_strdup(const char *s)
{
	size_t len = strlen(s) + 1;
	void *new = malloc(len);
	if (new == NULL)
		return NULL;
	return (char *)memcpy(new, s, len);
}

void __wrap_physunmap(void *virt_addr, size_t len)
{
	LOG_ME;
}

void *__wrap_physmap(const char *descr, uintptr_t phys_addr, size_t len)
{
	LOG_ME;
	return NULL;
}

struct pci_dev mock_pci_dev = {
	.device_id = MOCK_HANDLE,
};

struct pci_dev *__wrap_pcidev_init(void *devs, int bar)
{
	LOG_ME;
	return &mock_pci_dev;
}

uintptr_t __wrap_pcidev_readbar(void *dev, int bar)
{
	LOG_ME;
	return MOCK_HANDLE;
}

void __wrap_sio_write(uint16_t port, uint8_t reg, uint8_t data)
{
	LOG_ME;
}

uint8_t __wrap_sio_read(uint16_t port, uint8_t reg)
{
	LOG_ME;
	return (uint8_t)mock();
}

int __wrap_open(const char *pathname, int flags)
{
	LOG_ME;
	if (current_io && current_io->open)
		return current_io->open(current_io->state, pathname, flags);
	return MOCK_HANDLE;
}

int __wrap_open64(const char *pathname, int flags)
{
	LOG_ME;
	if (current_io && current_io->open)
		return current_io->open(current_io->state, pathname, flags);
	return MOCK_HANDLE;
}

int __wrap_ioctl(int fd, unsigned long int request, ...)
{
	LOG_ME;
	if (current_io && current_io->ioctl) {
		va_list args;
		int out;
		va_start(args, request);
		out = current_io->ioctl(current_io->state, fd, request, args);
		va_end(args);
		return out;
	}
	return MOCK_HANDLE;
}

int __wrap_write(int fd, const void *buf, size_t sz)
{
	LOG_ME;
	if (current_io && current_io->write)
		return current_io->write(current_io->state, fd, buf, sz);
	return sz;
}

int __wrap_read(int fd, void *buf, size_t sz)
{
	LOG_ME;
	if (current_io && current_io->read)
		return current_io->read(current_io->state, fd, buf, sz);
	return sz;
}

FILE *__wrap_fopen(const char *pathname, const char *mode)
{
	LOG_ME;
	if (current_io && current_io->fopen)
		return current_io->fopen(current_io->state, pathname, mode);
	return (void *)MOCK_HANDLE;
}

FILE *__wrap_fopen64(const char *pathname, const char *mode)
{
	LOG_ME;
	if (current_io && current_io->fopen)
		return current_io->fopen(current_io->state, pathname, mode);
	return (void *)MOCK_HANDLE;
}

int __wrap_stat(const char *path, void *buf)
{
	LOG_ME;
	return 0;
}

int __wrap_stat64(const char *path, void *buf)
{
	LOG_ME;
	return 0;
}

char *__wrap_fgets(char *buf, int len, FILE *fp)
{
	LOG_ME;
	if (current_io && current_io->fgets)
		return current_io->fgets(current_io->state, buf, len, fp);
	return NULL;
}

size_t __wrap_fread(void *ptr, size_t size, size_t len, FILE *fp)
{
	LOG_ME;
	if (current_io && current_io->fread)
		return current_io->fread(current_io->state, ptr, size, len, fp);
	return 0;
}

int __wrap_setvbuf(FILE *fp, char *buf, int type, size_t size)
{
	LOG_ME;
	return 0;
}

int __wrap_fclose(FILE *fp)
{
	LOG_ME;
	if (current_io && current_io->fclose)
		return current_io->fclose(current_io->state, fp);
	return 0;
}

int __wrap_feof(FILE *fp)
{
	/* LOG_ME; */
	return 0;
}

int __wrap_ferror(FILE *fp)
{
	/* LOG_ME; */
	return 0;
}
void __wrap_clearerr(FILE *fp)
{
	/* LOG_ME; */
	return;
}

int __wrap_rget_io_perms(void)
{
	LOG_ME;
	return 0;
}

void __wrap_test_outb(unsigned char value, unsigned short port)
{
	/* LOG_ME; */
	if (current_io && current_io->outb)
		current_io->outb(current_io->state, value, port);
}

unsigned char __wrap_test_inb(unsigned short port)
{
	/* LOG_ME; */
	if (current_io && current_io->inb)
		return current_io->inb(current_io->state, port);
	return 0;
}

void __wrap_test_outw(unsigned short value, unsigned short port)
{
	/* LOG_ME; */
	if (current_io && current_io->outw)
		current_io->outw(current_io->state, value, port);
}

unsigned short __wrap_test_inw(unsigned short port)
{
	/* LOG_ME; */
	if (current_io && current_io->inw)
		return current_io->inw(current_io->state, port);
	return 0;
}

void __wrap_test_outl(unsigned int value, unsigned short port)
{
	/* LOG_ME; */
	if (current_io && current_io->outl)
		current_io->outl(current_io->state, value, port);
}

unsigned int __wrap_test_inl(unsigned short port)
{
	/* LOG_ME; */
	if (current_io && current_io->inl)
		return current_io->inl(current_io->state, port);
	return 0;
}

void *__wrap_usb_dev_get_by_vid_pid_number(
		libusb_context *usb_ctx, uint16_t vid, uint16_t pid, unsigned int num)
{
	LOG_ME;
	return (void *)MOCK_HANDLE;
}

int __wrap_libusb_set_configuration(libusb_device_handle *devh, int config)
{
	LOG_ME;
	return 0;
}

int __wrap_libusb_claim_interface(libusb_device_handle *devh, int interface_number)
{
	LOG_ME;
	return 0;
}

int __wrap_libusb_control_transfer(libusb_device_handle *devh, uint8_t bmRequestType,
		uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data,
		uint16_t wLength, unsigned int timeout)
{
	LOG_ME;
	if (current_io && current_io->libusb_control_transfer)
		return current_io->libusb_control_transfer(current_io->state,
				devh, bmRequestType, bRequest, wValue, wIndex, data, wLength, timeout);
	return 0;
}

int __wrap_libusb_release_interface(libusb_device_handle *devh, int interface_number)
{
	LOG_ME;
	return 0;
}

void __wrap_libusb_close(libusb_device_handle *devh)
{
	LOG_ME;
}

void __wrap_libusb_exit(libusb_context *ctx)
{
	LOG_ME;
}

int main(void)
{
	int ret = 0;

	cmocka_set_message_output(CM_OUTPUT_STDOUT);

	const struct CMUnitTest helpers_tests[] = {
		cmocka_unit_test(address_to_bits_test_success),
		cmocka_unit_test(bitcount_test_success),
		cmocka_unit_test(minmax_test_success),
		cmocka_unit_test(strcat_realloc_test_success),
		cmocka_unit_test(tolower_string_test_success),
		cmocka_unit_test(reverse_byte_test_success),
		cmocka_unit_test(reverse_bytes_test_success),
	};
	ret |= cmocka_run_group_tests_name("helpers.c tests", helpers_tests, NULL, NULL);

	const struct CMUnitTest flashrom_tests[] = {
		cmocka_unit_test(flashbuses_to_text_test_success),
	};
	ret |= cmocka_run_group_tests_name("flashrom.c tests", flashrom_tests, NULL, NULL);

	const struct CMUnitTest spi25_tests[] = {
		cmocka_unit_test(spi_write_enable_test_success),
		cmocka_unit_test(spi_write_disable_test_success),
		cmocka_unit_test(probe_spi_rdid_test_success),
		cmocka_unit_test(probe_spi_rdid4_test_success),
		cmocka_unit_test(probe_spi_rems_test_success),
		cmocka_unit_test(probe_spi_res1_test_success),
		cmocka_unit_test(probe_spi_res2_test_success),
		cmocka_unit_test(probe_spi_res3_test_success),
		cmocka_unit_test(probe_spi_at25f_test_success),
		cmocka_unit_test(probe_spi_st95_test_success), /* spi95.c */
	};
	ret |= cmocka_run_group_tests_name("spi25.c tests", spi25_tests, NULL, NULL);

	const struct CMUnitTest init_shutdown_tests[] = {
		cmocka_unit_test(dummy_init_and_shutdown_test_success),
		cmocka_unit_test(mec1308_init_and_shutdown_test_success),
		cmocka_unit_test(nicrealtek_init_and_shutdown_test_success),
		cmocka_unit_test(dediprog_init_and_shutdown_test_success),
		cmocka_unit_test(ene_lpc_init_and_shutdown_test_success),
		cmocka_unit_test(linux_mtd_init_and_shutdown_test_success),
		cmocka_unit_test(linux_spi_init_and_shutdown_test_success),
		cmocka_unit_test(realtek_mst_init_and_shutdown_test_success),
	};
	ret |= cmocka_run_group_tests_name("init_shutdown.c tests", init_shutdown_tests, NULL, NULL);

	const struct CMUnitTest layout_tests[] = {
		cmocka_unit_test(included_regions_dont_overlap_test_success),
		cmocka_unit_test(included_regions_overlap_test_success),
		cmocka_unit_test(region_not_included_overlap_test_success),
		cmocka_unit_test(layout_pass_sanity_checks_test_success),
		cmocka_unit_test(layout_region_invalid_address_test_success),
		cmocka_unit_test(layout_region_invalid_range_test_success),
	};
	ret |= cmocka_run_group_tests_name("layout.c tests", layout_tests, NULL, NULL);

	const struct CMUnitTest chip_tests[] = {
		cmocka_unit_test(erase_chip_test_success),
		cmocka_unit_test(erase_chip_with_dummyflasher_test_success),
	};
	ret |= cmocka_run_group_tests_name("chip.c tests", chip_tests, NULL, NULL);

	return ret;
}
