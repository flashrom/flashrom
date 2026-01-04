/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __SERIAL_H__
#define __SERIAL_H__

#if IS_WINDOWS
#include <windows.h>
typedef HANDLE serialport_fdtype;
#define SERIALPORT_INV_FD	INVALID_HANDLE_VALUE
#else
typedef int serialport_fdtype;
#define SERIALPORT_INV_FD	-1
#endif

void serialport_flush_incoming(void);
serialport_fdtype serialport_openserport(char *dev, int baud);
extern serialport_fdtype serialport_fd;
int serialport_config(serialport_fdtype fd, int baud);
int serialport_shutdown(void *data);
int serialport_write(const unsigned char *buf, unsigned int writecnt);
int serialport_write_nonblock(const unsigned char *buf, unsigned int writecnt, unsigned int timeout, unsigned int *really_wrote);
int serialport_read(unsigned char *buf, unsigned int readcnt);
int serialport_read_nonblock(unsigned char *c, unsigned int readcnt, unsigned int timeout, unsigned int *really_read);

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

void serialport_set_pin(enum SERIALPORT_PIN pin, int val);
int serialport_get_pin(enum SERIALPORT_PIN pin);

#endif /* __SERIAL_H__ */
