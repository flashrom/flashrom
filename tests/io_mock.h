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

/* Required for `FILE *` */
#include <stdio.h>

/*
 * Explicitly including the header because some tests are using libusb structs
 * in depth, opaque symbols are not sufficient.
 */
#include <libusb.h>

/* Address value needs fit into uint8_t. */
#define USB_DEVICE_ADDRESS 19

/* Define struct pci_dev to avoid dependency on pci.h */
struct pci_dev {
	unsigned int device_id;
};

/* POSIX open() flags, avoiding dependency on fcntl.h */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2

/* Linux I2C interface constants, avoiding linux/i2c-dev.h */
#define I2C_SLAVE 0x0703

/* Always return success for tests. */
#define S_ISREG(x) 0

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

	/* POSIX File I/O */
	int (*open)(void *state, const char *pathname, int flags);
	int (*ioctl)(void *state, int fd, unsigned long request, va_list args);
	int (*read)(void *state, int fd, void *buf, size_t sz);
	int (*write)(void *state, int fd, const void *buf, size_t sz);

	/* Standard I/O */
	FILE* (*fopen)(void *state, const char *pathname, const char *mode);
	char* (*fgets)(void *state, char *buf, int len, FILE *fp);
	size_t (*fread)(void *state, void *buf, size_t size, size_t len, FILE *fp);
	int (*fprintf)(void *state, FILE *fp, const char *fmt, va_list args);
	int (*fclose)(void *state, FILE *fp);
};

void io_mock_register(const struct io_mock *io);

const struct io_mock *get_io(void);

#endif
