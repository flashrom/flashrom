/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Urja Rannikko <urjaman@gmail.com>
 * Copyright (C) 2009,2010 Carl-Daniel Hailfinger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <inttypes.h>
#if IS_WINDOWS
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#endif
#include "flash.h"
#include "programmer.h"
#include "custom_baud.h"

fdtype sp_fd = SER_INV_FD;

/* There is no way defined by POSIX to use arbitrary baud rates. It only defines some macros that can be used to
 * specify respective baud rates and many implementations extend this list with further macros, cf. TERMIOS(3)
 * and http://git.kernel.org/?p=linux/kernel/git/torvalds/linux.git;a=blob;f=include/uapi/asm-generic/termbits.h
 * The code below creates a mapping in sp_baudtable between these macros and the numerical baud rates to deal
 * with numerical user input.
 *
 * On Linux there is a non-standard way to use arbitrary baud rates that we use if there is no
 * matching standard rate, see custom_baud.c
 *
 * On Darwin there is also a non-standard ioctl() to set arbitrary baud rates
 * and any above 230400, see custom_baud_darwin.c and
 * https://opensource.apple.com/source/IOSerialFamily/IOSerialFamily-91/tests/IOSerialTestLib.c.auto.html
 *
 * On Windows there exist similar macros (starting with CBR_ instead of B) but they are only defined for
 * backwards compatibility and the API supports arbitrary baud rates in the same manner as the macros, see
 * http://msdn.microsoft.com/en-us/library/windows/desktop/aa363214(v=vs.85).aspx
 */
#if !IS_WINDOWS
#define BAUDENTRY(baud) { B##baud, baud },

static const struct baudentry sp_baudtable[] = {
	BAUDENTRY(9600) /* unconditional default */
#ifdef B19200
	BAUDENTRY(19200)
#endif
#ifdef B38400
	BAUDENTRY(38400)
#endif
#ifdef B57600
	BAUDENTRY(57600)
#endif
#ifdef B115200
	BAUDENTRY(115200)
#endif
#ifdef B230400
	BAUDENTRY(230400)
#endif
#ifdef B460800
	BAUDENTRY(460800)
#endif
#ifdef B500000
	BAUDENTRY(500000)
#endif
#ifdef B576000
	BAUDENTRY(576000)
#endif
#ifdef B921600
	BAUDENTRY(921600)
#endif
#ifdef B1000000
	BAUDENTRY(1000000)
#endif
#ifdef B1152000
	BAUDENTRY(1152000)
#endif
#ifdef B1500000
	BAUDENTRY(1500000)
#endif
#ifdef B2000000
	BAUDENTRY(2000000)
#endif
#ifdef B2500000
	BAUDENTRY(2500000)
#endif
#ifdef B3000000
	BAUDENTRY(3000000)
#endif
#ifdef B3500000
	BAUDENTRY(3500000)
#endif
#ifdef B4000000
	BAUDENTRY(4000000)
#endif
	{0, 0}			/* Terminator */
};

static const struct baudentry *round_baud(unsigned int baud)
{
	int i;
	/* Round baud rate to next lower entry in sp_baudtable if it exists, else use the lowest entry. */
	for (i = ARRAY_SIZE(sp_baudtable) - 2; i >= 0 ; i--) {
		if (sp_baudtable[i].baud == baud)
			return &sp_baudtable[i];

		if (sp_baudtable[i].baud < baud) {
			msg_pwarn("Warning: given baudrate %d rounded down to %d.\n",
				  baud, sp_baudtable[i].baud);
			return &sp_baudtable[i];
		}
	}
	msg_pinfo("Using slowest possible baudrate: %d.\n", sp_baudtable[0].baud);
	return &sp_baudtable[0];
}
#endif

/* Uses msg_perr to print the last system error.
 * Prints "Error: " followed first by \c msg and then by the description of the last error retrieved via
 * strerror() or FormatMessage() and ending with a linebreak. */
static void msg_perr_strerror(const char *msg)
{
	msg_perr("Error: %s", msg);
#if IS_WINDOWS
	char *lpMsgBuf;
	DWORD nErr = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, nErr,
		      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
	msg_perr(lpMsgBuf);
	/* At least some formatted messages contain a line break at the end. Make sure to always print one */
	if (lpMsgBuf[strlen(lpMsgBuf)-1] != '\n')
		msg_perr("\n");
	LocalFree(lpMsgBuf);
#else
	msg_perr("%s\n", strerror(errno));
#endif
}

int serialport_config(fdtype fd, int baud)
{
	if (fd == SER_INV_FD) {
		msg_perr("%s: File descriptor is invalid.\n", __func__);
		return 1;
	}

#if IS_WINDOWS
	DCB dcb;
	if (!GetCommState(fd, &dcb)) {
		msg_perr_strerror("Could not fetch original serial port configuration: ");
		return 1;
	}
	if (baud >= 0) {
		dcb.BaudRate = baud;
	}
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	if (!SetCommState(fd, &dcb)) {
		msg_perr_strerror("Could not change serial port configuration: ");
		return 1;
	}
	if (!GetCommState(fd, &dcb)) {
		msg_perr_strerror("Could not fetch new serial port configuration: ");
		return 1;
	}
	msg_pdbg("Baud rate is %ld.\n", dcb.BaudRate);
#else
	int custom_baud = (baud >= 0 && use_custom_baud(baud, sp_baudtable));
	struct termios wanted, observed;
	if (tcgetattr(fd, &observed) != 0) {
		msg_perr_strerror("Could not fetch original serial port configuration: ");
		return 1;
	}
	wanted = observed;
	if (baud >= 0) {
		if (custom_baud) {
			if (set_custom_baudrate(fd, baud, BEFORE_FLAGS, NULL)) {
				msg_perr_strerror("Could not set custom baudrate: ");
				return 1;
			}
			/* We want whatever the termios looks like now, so the rest of the
			   setup doesn't mess up the custom rate. */
			if (tcgetattr(fd, &wanted) != 0) {
				/* This should pretty much never happen (see above), but.. */
				msg_perr_strerror("Could not fetch serial port configuration: ");
				return 1;
			}
		} else {
			const struct baudentry *entry = round_baud(baud);
			if (cfsetispeed(&wanted, entry->flag) != 0 || cfsetospeed(&wanted, entry->flag) != 0) {
				msg_perr_strerror("Could not set serial baud rate: ");
				return 1;
			}
		}
	}
	wanted.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);
	wanted.c_cflag |= (CS8 | CLOCAL | CREAD);
	wanted.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN);
	wanted.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | IGNCR | INLCR);
	wanted.c_oflag &= ~OPOST;
	if (custom_baud && set_custom_baudrate(fd, baud, WITH_FLAGS, &wanted)) {
		msg_perr_strerror("Could not set custom baudrate: ");
		return 1;
	}
	if (tcsetattr(fd, TCSANOW, &wanted) != 0) {
		msg_perr_strerror("Could not change serial port configuration: ");
		return 1;
	}
	if (tcgetattr(fd, &observed) != 0) {
		msg_perr_strerror("Could not fetch new serial port configuration: ");
		return 1;
	}
	if (observed.c_cflag != wanted.c_cflag ||
	    observed.c_lflag != wanted.c_lflag ||
	    observed.c_iflag != wanted.c_iflag ||
	    observed.c_oflag != wanted.c_oflag) {
		msg_pwarn("Some requested serial options did not stick, continuing anyway.\n");
		msg_pdbg("          observed    wanted\n"
			 "c_cflag:  0x%08lX  0x%08lX\n"
			 "c_lflag:  0x%08lX  0x%08lX\n"
			 "c_iflag:  0x%08lX  0x%08lX\n"
			 "c_oflag:  0x%08lX  0x%08lX\n",
			 (long)observed.c_cflag, (long)wanted.c_cflag,
			 (long)observed.c_lflag, (long)wanted.c_lflag,
			 (long)observed.c_iflag, (long)wanted.c_iflag,
			 (long)observed.c_oflag, (long)wanted.c_oflag
			);
	}
	if (custom_baud) {
		if (set_custom_baudrate(fd, baud, AFTER_FLAGS, &wanted)) {
			msg_perr_strerror("Could not set custom baudrate: ");
			return 1;
		}
		msg_pdbg("Using custom baud rate.\n");
	}
	if (cfgetispeed(&observed) != cfgetispeed(&wanted) ||
	    cfgetospeed(&observed) != cfgetospeed(&wanted)) {
		msg_pwarn("Could not set baud rates exactly.\n");
		msg_pdbg("Actual baud flags are: ispeed: 0x%08lX, ospeed: 0x%08lX\n",
			  (long)cfgetispeed(&observed), (long)cfgetospeed(&observed));
	}
	// FIXME: display actual baud rate - at least if none was specified by the user.
#endif
	return 0;
}

fdtype sp_openserport(char *dev, int baud)
{
	fdtype fd;
#if IS_WINDOWS
	char *dev2 = dev;
	if ((strlen(dev) > 3) &&
	    (tolower((unsigned char)dev[0]) == 'c') &&
	    (tolower((unsigned char)dev[1]) == 'o') &&
	    (tolower((unsigned char)dev[2]) == 'm')) {
		dev2 = malloc(strlen(dev) + 5);
		if (!dev2) {
			msg_perr_strerror("Out of memory: ");
			return SER_INV_FD;
		}
		strcpy(dev2, "\\\\.\\");
		strcpy(dev2 + 4, dev);
	}
	fd = CreateFile(dev2, GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
	if (dev2 != dev)
		free(dev2);
	if (fd == INVALID_HANDLE_VALUE) {
		msg_perr_strerror("Cannot open serial port: ");
		return SER_INV_FD;
	}
	if (serialport_config(fd, baud) != 0) {
		CloseHandle(fd);
		return SER_INV_FD;
	}
	return fd;
#else
	fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY); // Use O_NDELAY to ignore DCD state
	if (fd < 0) {
		msg_perr_strerror("Cannot open serial port: ");
		return SER_INV_FD;
	}

	/* Ensure that we use blocking I/O */
	const int flags = fcntl(fd, F_GETFL);
	if (flags == -1) {
		msg_perr_strerror("Could not get serial port mode: ");
		goto err;
	}
	if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) != 0) {
		msg_perr_strerror("Could not set serial port mode to blocking: ");
		goto err;
	}

	if (serialport_config(fd, baud) != 0) {
		goto err;
	}
	return fd;
err:
	close(fd);
	return SER_INV_FD;
#endif
}

void sp_set_pin(enum SP_PIN pin, int val) {
#if IS_WINDOWS
	DWORD ctl;

	if(pin == PIN_TXD) {
		ctl = val ? SETBREAK: CLRBREAK;
	}
	else if(pin == PIN_DTR) {
		ctl = val ? SETDTR: CLRDTR;
	}
	else {
		ctl = val ? SETRTS: CLRRTS;
	}
	EscapeCommFunction(sp_fd, ctl);
#else
	int ctl, s;

	if(pin == PIN_TXD) {
		ioctl(sp_fd, val ? TIOCSBRK : TIOCCBRK, 0);
	}
	else {
		s = (pin == PIN_DTR) ? TIOCM_DTR : TIOCM_RTS;
		ioctl(sp_fd, TIOCMGET, &ctl);

		if (val) {
			ctl |= s;
		}
		else {
			ctl &= ~s;
		}
		ioctl(sp_fd, TIOCMSET, &ctl);
	}
#endif
}

int sp_get_pin(enum SP_PIN pin) {
	int s;
#if IS_WINDOWS
	DWORD ctl;

	s = (pin == PIN_CTS) ? MS_CTS_ON : MS_DSR_ON;
	GetCommModemStatus(sp_fd, &ctl);
#else
	int ctl;
	s = (pin == PIN_CTS) ? TIOCM_CTS : TIOCM_DSR;
	ioctl(sp_fd, TIOCMGET, &ctl);
#endif

	return ((ctl & s) ? 1 : 0);

}

void sp_flush_incoming(void)
{
#if IS_WINDOWS
	PurgeComm(sp_fd, PURGE_RXCLEAR);
#else
	if (!tcflush(sp_fd, TCIFLUSH))
		return;

	if (errno == ENOTTY) { // TCP socket case: sp_fd is not a terminal descriptor - tcflush is not supported
		unsigned char c;
		int ret;

		do {
			ret = serialport_read_nonblock(&c, 1, 1, NULL);
		} while (ret == 0);

		// positive error code indicates no data available immediately - similar to EAGAIN/EWOULDBLOCK
		//   i.e. all buffered data was read
		// negative error code indicates a permanent error
		if (ret < 0)
			msg_perr("Could not flush serial port incoming buffer: read has failed");
	} else { // any other errno indicates an unrecoverable sp_fd state
		msg_perr_strerror("Could not flush serial port incoming buffer: ");
	}
#endif
}

int serialport_shutdown(void *data)
{
#if IS_WINDOWS
	CloseHandle(sp_fd);
#else
	close(sp_fd);
#endif
	return 0;
}

int serialport_write(const unsigned char *buf, unsigned int writecnt)
{
#if IS_WINDOWS
	DWORD tmp = 0;
#else
	ssize_t tmp = 0;
#endif
	unsigned int empty_writes = 250; /* results in a ca. 125ms timeout */

	while (writecnt > 0) {
#if IS_WINDOWS
		if (!WriteFile(sp_fd, buf, writecnt, &tmp, NULL)) {
			msg_perr("Serial port write error!\n");
			return 1;
		}
#else
		tmp = write(sp_fd, buf, writecnt);
		if (tmp == -1) {
			msg_perr("Serial port write error!\n");
			return 1;
		}
#endif
		if (!tmp) {
			msg_pdbg2("Empty write\n");
			empty_writes--;
			default_delay(500);
			if (empty_writes == 0) {
				msg_perr("Serial port is unresponsive!\n");
				return 1;
			}
		}
		writecnt -= tmp;
		buf += tmp;
	}

	return 0;
}

int serialport_read(unsigned char *buf, unsigned int readcnt)
{
#if IS_WINDOWS
	DWORD tmp = 0;
#else
	ssize_t tmp = 0;
#endif

	while (readcnt > 0) {
#if IS_WINDOWS
		if (!ReadFile(sp_fd, buf, readcnt, &tmp, NULL)) {
			msg_perr("Serial port read error!\n");
			return 1;
		}
#else
		tmp = read(sp_fd, buf, readcnt);
		if (tmp == -1) {
			msg_perr("Serial port read error!\n");
			return 1;
		}
#endif
		if (!tmp)
			msg_pdbg2("Empty read\n");
		readcnt -= tmp;
		buf += tmp;
	}

	return 0;
}

/* Tries up to timeout ms to read readcnt characters and places them into the array starting at c. Returns
 * 0 on success, positive values on temporary errors (e.g. timeouts) and negative ones on permanent errors.
 * If really_read is not NULL, this function sets its contents to the number of bytes read successfully. */
int serialport_read_nonblock(unsigned char *c, unsigned int readcnt, unsigned int timeout, unsigned int *really_read)
{
	int ret = 1;
	/* disable blocked i/o and declare platform-specific variables */
#if IS_WINDOWS
	DWORD rv;
	COMMTIMEOUTS oldTimeout;
	COMMTIMEOUTS newTimeout = {
		.ReadIntervalTimeout = MAXDWORD,
		.ReadTotalTimeoutMultiplier = 0,
		.ReadTotalTimeoutConstant = 0,
		.WriteTotalTimeoutMultiplier = 0,
		.WriteTotalTimeoutConstant = 0
	};
	if(!GetCommTimeouts(sp_fd, &oldTimeout)) {
		msg_perr_strerror("Could not get serial port timeout settings: ");
		return -1;
	}
	if(!SetCommTimeouts(sp_fd, &newTimeout)) {
		msg_perr_strerror("Could not set serial port timeout settings: ");
		return -1;
	}
#else
	ssize_t rv;
	const int flags = fcntl(sp_fd, F_GETFL);
	if (flags == -1) {
		msg_perr_strerror("Could not get serial port mode: ");
		return -1;
	}
	if (fcntl(sp_fd, F_SETFL, flags | O_NONBLOCK) != 0) {
		msg_perr_strerror("Could not set serial port mode to non-blocking: ");
		return -1;
	}
#endif

	unsigned int i;
	unsigned int rd_bytes = 0;
	for (i = 0; i < timeout; i++) {
		msg_pspew("readcnt %u rd_bytes %u\n", readcnt, rd_bytes);
#if IS_WINDOWS
		if (!ReadFile(sp_fd, c + rd_bytes, readcnt - rd_bytes, &rv, NULL)) {
			msg_perr_strerror("Serial port read error: ");
			ret = -1;
			break;
		}
		msg_pspew("read %lu bytes\n", rv);
#else
		rv = read(sp_fd, c + rd_bytes, readcnt - rd_bytes);
		msg_pspew("read %zd bytes\n", rv);
		if ((rv == -1) && (errno != EAGAIN)) {
			msg_perr_strerror("Serial port read error: ");
			ret = -1;
			break;
		}
#endif
		if (rv > 0)
			rd_bytes += rv;
		if (rd_bytes == readcnt) {
			ret = 0;
			break;
		}
		default_delay(1000);	/* 1ms units */
	}
	if (really_read != NULL)
		*really_read = rd_bytes;

	/* restore original blocking behavior */
#if IS_WINDOWS
	if (!SetCommTimeouts(sp_fd, &oldTimeout)) {
		msg_perr_strerror("Could not restore serial port timeout settings: ");
		ret = -1;
	}
#else
	if (fcntl(sp_fd, F_SETFL, flags) != 0) {
		msg_perr_strerror("Could not restore serial port mode to blocking: ");
		ret = -1;
	}
#endif
	return ret;
}

/* Tries up to timeout ms to write writecnt characters from the array starting at buf. Returns
 * 0 on success, positive values on temporary errors (e.g. timeouts) and negative ones on permanent errors.
 * If really_wrote is not NULL, this function sets its contents to the number of bytes written successfully. */
int serialport_write_nonblock(const unsigned char *buf, unsigned int writecnt, unsigned int timeout, unsigned int *really_wrote)
{
	int ret = 1;
	/* disable blocked i/o and declare platform-specific variables */
#if IS_WINDOWS
	DWORD rv;
	COMMTIMEOUTS oldTimeout;
	COMMTIMEOUTS newTimeout = {
		.ReadIntervalTimeout = MAXDWORD,
		.ReadTotalTimeoutMultiplier = 0,
		.ReadTotalTimeoutConstant = 0,
		.WriteTotalTimeoutMultiplier = 0,
		.WriteTotalTimeoutConstant = 0
	};
	if(!GetCommTimeouts(sp_fd, &oldTimeout)) {
		msg_perr_strerror("Could not get serial port timeout settings: ");
		return -1;
	}
	if(!SetCommTimeouts(sp_fd, &newTimeout)) {
		msg_perr_strerror("Could not set serial port timeout settings: ");
		return -1;
	}
#else
	ssize_t rv;
	const int flags = fcntl(sp_fd, F_GETFL);
	if (flags == -1) {
		msg_perr_strerror("Could not get serial port mode: ");
		return -1;
	}
	if (fcntl(sp_fd, F_SETFL, flags | O_NONBLOCK) != 0) {
		msg_perr_strerror("Could not set serial port mode to non-blocking: ");
		return -1;
	}
#endif

	unsigned int i;
	unsigned int wr_bytes = 0;
	for (i = 0; i < timeout; i++) {
		msg_pspew("writecnt %u wr_bytes %u\n", writecnt, wr_bytes);
#if IS_WINDOWS
		if (!WriteFile(sp_fd, buf + wr_bytes, writecnt - wr_bytes, &rv, NULL)) {
			msg_perr_strerror("Serial port write error: ");
			ret = -1;
			break;
		}
		msg_pspew("wrote %lu bytes\n", rv);
#else
		rv = write(sp_fd, buf + wr_bytes, writecnt - wr_bytes);
		msg_pspew("wrote %zd bytes\n", rv);
		if ((rv == -1) && (errno != EAGAIN)) {
			msg_perr_strerror("Serial port write error: ");
			ret = -1;
			break;
		}
#endif
		if (rv > 0) {
			wr_bytes += rv;
			if (wr_bytes == writecnt) {
				msg_pspew("write successful\n");
				ret = 0;
				break;
			}
		}
		default_delay(1000);	/* 1ms units */
	}
	if (really_wrote != NULL)
		*really_wrote = wr_bytes;

	/* restore original blocking behavior */
#if IS_WINDOWS
	if (!SetCommTimeouts(sp_fd, &oldTimeout)) {
		msg_perr_strerror("Could not restore serial port timeout settings: ");
		return -1;
	}
#else
	if (fcntl(sp_fd, F_SETFL, flags) != 0) {
		msg_perr_strerror("Could not restore serial port blocking behavior: ");
		return -1;
	}
#endif
	return ret;
}
