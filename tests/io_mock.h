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

/* Define libusb symbols to avoid dependency on libusb.h */
struct libusb_device_handle;
typedef struct libusb_device_handle libusb_device_handle;
struct libusb_context;
typedef struct libusb_context libusb_context;

/* Define struct pci_dev to avoid dependency on pci.h */
struct pci_dev {
	unsigned int device_id;
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
	int (*libusb_control_transfer)(void *state,
					libusb_device_handle *devh,
					uint8_t bmRequestType,
					uint8_t bRequest,
					uint16_t wValue,
					uint16_t wIndex,
					unsigned char *data,
					uint16_t wLength,
					unsigned int timeout);
};

void io_mock_register(const struct io_mock *io);

#endif
