/*
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2022 Google LLC
 */

/*
 * This header provides a temporary solution to unblock build system
 * work. The main goal is to remove unconditional dependency on libusb
 * for unit tests. The dependency is still present, but now it is present
 * only when it is needed and only when the header is present in the env.
 *
 * The contents of the file will be modified in a very near future.
 */

#ifndef _FTDI_UNITTESTS_H_
#define _FTDI_UNITTESTS_H_

#if CONFIG_FT2232_SPI == 1 || CONFIG_USBBLASTER_SPI == 1

#include <ftdi.h>

#else

#define TYPE_4232H 5

struct ftdi_context {
    int type;
};
typedef struct ftdi_context ftdi_context;

struct ftdi_device_list;
typedef struct ftdi_device_list ftdi_device_list;

enum ftdi_interface { UNUSED };

#endif

#endif /* _FTDI_UNITTESTS_H_ */
