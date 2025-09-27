/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2021 Google LLC
 */

#include "lifecycle.h"

#if CONFIG_DEDIPROG == 1
static int dediprog_libusb_init(void *state, libusb_context **ctx)
{
	*ctx = not_null();
	return 0;
}

static int dediprog_libusb_control_transfer(void *state,
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
	struct io_mock_fallback_open_state dediprog_fallback_open_state = {
		.noc = 0,
		.paths = { NULL },
	};
	const struct io_mock dediprog_io = {
		.libusb_init = dediprog_libusb_init,
		.libusb_control_transfer = dediprog_libusb_control_transfer,
		.fallback_open_state = &dediprog_fallback_open_state,
	};

	run_basic_lifecycle(state, &dediprog_io, &programmer_dediprog, "voltage=3.5V");
}
#else
	SKIP_TEST(dediprog_basic_lifecycle_test_success)
#endif /* CONFIG_DEDIPROG */
