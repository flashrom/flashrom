/*
 * This file is part of the flashrom project.
 *
 * Copyright (c) 2021 Nico Huber <nico.h@gmx.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _IO_MOCK_H_
#define _IO_MOCK_H_

#include <include/test.h>

/* Required for `FILE *` */
#include <stdio.h>

#include <stdint.h>

/* Required for struct timeval and mode_t */
#include <sys/types.h>
#include <sys/time.h>

#include "usb_unittests.h"

/* Address value needs fit into uint8_t. */
#define USB_DEVICE_ADDRESS 19

/* Define struct pci_dev to avoid dependency on pci.h */
struct pci_dev {
	char padding[18];
	unsigned int device_id;
};

/* Linux I2C interface constants, avoiding linux/i2c-dev.h */
#define I2C_SLAVE 0x0703

/* Always return success for tests. */
#define S_ISREG(x) 0

/* Maximum number of open calls to mock. This number is arbitrary. */
#define MAX_MOCK_OPEN 4

struct io_mock_fallback_open_state {
	unsigned int noc;
	const char *paths[MAX_MOCK_OPEN];
	int flags[MAX_MOCK_OPEN];
};

struct io_mock {
	void *state;

	/* Port I/O */
	void (*outb)(void *state, unsigned char value, unsigned short port);
	unsigned char (*inb)(void *state, unsigned short port);

	void (*outw)(void *state, unsigned short value, unsigned short port);
	unsigned short (*inw)(void *state, unsigned short port);

	void (*outl)(void *state, unsigned int value, unsigned short port);
	unsigned int (*inl)(void *state, unsigned short port);

	/* USB I/O */
	int (*libusb_init)(void *state, libusb_context **ctx);
	int (*libusb_control_transfer)(void *state,
					libusb_device_handle *devh,
					uint8_t bmRequestType,
					uint8_t bRequest,
					uint16_t wValue,
					uint16_t wIndex,
					unsigned char *data,
					uint16_t wLength,
					unsigned int timeout);
	ssize_t (*libusb_get_device_list)(void *state, libusb_context *, libusb_device ***list);
	void (*libusb_free_device_list)(void *state, libusb_device **list, int unref_devices);
	int (*libusb_get_device_descriptor)(void *state, libusb_device *, struct libusb_device_descriptor *);
	int (*libusb_get_config_descriptor)(void *state,
						libusb_device *,
						uint8_t config_index,
						struct libusb_config_descriptor **);
	void (*libusb_free_config_descriptor)(void *state, struct libusb_config_descriptor *);
	struct libusb_transfer* (*libusb_alloc_transfer)(void *state, int iso_packets);
	int (*libusb_submit_transfer)(void *state, struct libusb_transfer *transfer);
	void (*libusb_free_transfer)(void *state, struct libusb_transfer *transfer);
	int (*libusb_handle_events_timeout)(void *state, libusb_context *ctx, struct timeval *tv);

	/* POSIX File I/O */
	int (*iom_open)(void *state, const char *pathname, int flags, mode_t mode);
	int (*iom_ioctl)(void *state, int fd, unsigned long request, va_list args);
	int (*iom_read)(void *state, int fd, void *buf, size_t sz);
	int (*iom_write)(void *state, int fd, const void *buf, size_t sz);

	/* Standard I/O */
	FILE* (*iom_fopen)(void *state, const char *pathname, const char *mode);
	char* (*iom_fgets)(void *state, char *buf, int len, FILE *fp);
	size_t (*iom_fread)(void *state, void *buf, size_t size, size_t len, FILE *fp);
	size_t (*iom_fwrite)(void *state, const void *buf, size_t size, size_t len, FILE *fp);
	int (*iom_fprintf)(void *state, FILE *fp, const char *fmt, va_list args);
	int (*iom_fclose)(void *state, FILE *fp);
	FILE *(*iom_fdopen)(void *state, int fd, const char *mode);

	/*
	 * An alternative to custom open mock. A test can either register its
	 * own mock open function or fallback_open_state.
	 */
	struct io_mock_fallback_open_state *fallback_open_state;
};

void io_mock_register(const struct io_mock *io);

const struct io_mock *get_io(void);

#endif
