/*
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2026 Antonio Vázquez Blanco <antoniovazquezblanco@gmail.com>
 */

#include <stdint.h>

#include <include/test.h>
#include "io_mock.h"
#include "serial_wraps.h"

/* Handle of a port opened by the serial_open mock, never passed to the OS. */
#define MOCK_SERIAL_FD ((serialport_fdtype)(intptr_t)MOCK_FD)

serialport_fdtype __wrap_serialport_openserport(char *dev, int baud)
{
	LOG_ME;
	if (get_io() && get_io()->serial_open && !get_io()->serial_open(get_io()->state, dev, baud))
		return MOCK_SERIAL_FD;
	return SERIALPORT_INV_FD;
}

void __wrap_serialport_flush_incoming(serialport_fdtype fd)
{
	LOG_ME;
}

int __wrap_serialport_shutdown(serialport_fdtype fd)
{
	LOG_ME;
	return 0;
}

int __wrap_serialport_read(serialport_fdtype fd, unsigned char *buf, unsigned int readcnt)
{
	LOG_ME;
	if (get_io() && get_io()->serial_read) {
		assert_true(fd == MOCK_SERIAL_FD);
		return get_io()->serial_read(get_io()->state, buf, readcnt);
	}
	return 1;
}

int __wrap_serialport_write(serialport_fdtype fd, const unsigned char *buf, unsigned int writecnt)
{
	LOG_ME;
	if (get_io() && get_io()->serial_write) {
		assert_true(fd == MOCK_SERIAL_FD);
		return get_io()->serial_write(get_io()->state, buf, writecnt);
	}
	return 1;
}
