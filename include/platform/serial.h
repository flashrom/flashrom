/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __PLATFORM_SERIAL_H__
#define __PLATFORM_SERIAL_H__

#if IS_WINDOWS
#include <windows.h>
typedef HANDLE serialport_fdtype;
#define SERIALPORT_INV_FD	INVALID_HANDLE_VALUE
#else
typedef int serialport_fdtype;
#define SERIALPORT_INV_FD	-1
#endif

void serialport_flush_incoming(serialport_fdtype fd);
serialport_fdtype serialport_openserport(char *dev, int baud);
int serialport_config(serialport_fdtype fd, int baud);
int serialport_shutdown(serialport_fdtype fd);
int serialport_write(serialport_fdtype fd, const unsigned char *buf, unsigned int writecnt);
int serialport_write_nonblock(serialport_fdtype fd, const unsigned char *buf, unsigned int writecnt, unsigned int timeout, unsigned int *really_wrote);
int serialport_read(serialport_fdtype fd, unsigned char *buf, unsigned int readcnt);
int serialport_read_nonblock(serialport_fdtype fd, unsigned char *c, unsigned int readcnt, unsigned int timeout, unsigned int *really_read);

/* Low level I/O. The functions below are provided by the platform specific
 * implementations and are meant to be used by the common serial code only. */

/* Reads up to readcnt bytes from the serial port fd into buf. The amount of bytes
 * that could actually be read is stored in actual, which may be zero if the
 * port had nothing to offer. Returns 0 on success and 1 on error. */
int serialport_raw_read(serialport_fdtype fd, unsigned char *buf, unsigned int readcnt, unsigned int *actual);

/* Writes up to writecnt bytes from buf to the serial port fd. The amount of bytes
 * that could actually be written is stored in actual, which may be zero if the
 * port was not ready to accept any. Returns 0 on success and 1 on error. */
int serialport_raw_write(serialport_fdtype fd, const unsigned char *buf, unsigned int writecnt, unsigned int *actual);

/* Makes the serial port fd return from reads and writes immediately instead of
 * waiting for the requested amount of bytes. The previous behavior is saved and
 * can be restored with serialport_nonblock_leave().
 * Returns 0 on success and 1 on error. */
int serialport_nonblock_enter(serialport_fdtype fd);

/* Restores the behavior saved by serialport_nonblock_enter().
 * Returns 0 on success and 1 on error. */
int serialport_nonblock_leave(serialport_fdtype fd);

/* Serial port/pin mapping:

  1	CD	<-
  2	RXD	<-
  3	TXD	->
  4	DTR	->
  5	GND     --
  6	DSR	<-
  7	RTS	->
  8	CTS	<-
  9	RI	<-
*/
enum SERIALPORT_PIN {
	PIN_CD = 1,
	PIN_RXD,
	PIN_TXD,
	PIN_DTR,
	PIN_GND,
	PIN_DSR,
	PIN_RTS,
	PIN_CTS,
	PIN_RI,
};

void serialport_set_pin(serialport_fdtype fd, enum SERIALPORT_PIN pin, int val);
int serialport_get_pin(serialport_fdtype fd, enum SERIALPORT_PIN pin);

#endif /* __PLATFORM_SERIAL_H__ */
