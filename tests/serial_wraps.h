/*
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2026 Antonio Vázquez Blanco <antoniovazquezblanco@gmail.com>
 */

#ifndef SERIAL_WRAPS_H
#define SERIAL_WRAPS_H

#include "platform/serial.h"

serialport_fdtype __wrap_serialport_openserport(char *dev, int baud);
void __wrap_serialport_flush_incoming(serialport_fdtype fd);
int __wrap_serialport_shutdown(serialport_fdtype fd);
int __wrap_serialport_read(serialport_fdtype fd, unsigned char *buf, unsigned int readcnt);
int __wrap_serialport_write(serialport_fdtype fd, const unsigned char *buf, unsigned int writecnt);

#endif /* SERIAL_WRAPS_H */
