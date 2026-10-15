/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2009 Urja Rannikko <urjaman@gmail.com>
 * SPDX-FileCopyrightText: 2009,2010 Carl-Daniel Hailfinger
 */

#include "platform/serial.h"

#include "log.h"
#include "platform/string.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

/* Windows has macros similar to the POSIX baud rate ones (starting with CBR_ instead of B) but they are only
 * defined for backwards compatibility and the API supports arbitrary baud rates in the same manner as the
 * macros, see http://msdn.microsoft.com/en-us/library/windows/desktop/aa363214(v=vs.85).aspx
 */

/* Uses msg_perr to print the last system error.
 * Prints "Error: " followed first by \c msg and then by the description of the last error retrieved via
 * FormatMessage() and ending with a linebreak. */
static void msg_perr_strerror(const char *msg)
{
	msg_perr("Error: %s", msg);
	char *lpMsgBuf;
	DWORD nErr = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, nErr,
		      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
	msg_perr(lpMsgBuf);
	/* At least some formatted messages contain a line break at the end. Make sure to always print one */
	if (lpMsgBuf[strlen(lpMsgBuf)-1] != '\n')
		msg_perr("\n");
	LocalFree(lpMsgBuf);
}

int serialport_config(serialport_fdtype fd, int baud)
{
	if (fd == SERIALPORT_INV_FD) {
		msg_perr("%s: File descriptor is invalid.\n", __func__);
		return 1;
	}

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
	dcb.fBinary = true;
	dcb.fParity = false;
	dcb.fOutxCtsFlow = false;
	dcb.fOutxDsrFlow = false;
	dcb.fDsrSensitivity = false;
	dcb.fOutX = false;
	dcb.fInX = false;
	dcb.fErrorChar = false;
	dcb.fNull = false;
	dcb.fAbortOnError = false;
	if (dcb.fDtrControl == DTR_CONTROL_HANDSHAKE)
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
	if (dcb.fRtsControl == RTS_CONTROL_HANDSHAKE)
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
	if (!SetCommState(fd, &dcb)) {
		msg_perr_strerror("Could not change serial port configuration: ");
		return 1;
	}
	if (!GetCommState(fd, &dcb)) {
		msg_perr_strerror("Could not fetch new serial port configuration: ");
		return 1;
	}
	msg_pdbg("Baud rate is %ld.\n", dcb.BaudRate);
	return 0;
}

serialport_fdtype serialport_openserport(char *dev, int baud)
{
	serialport_fdtype fd;
	char *dev2 = dev;
	if ((strlen(dev) > 3) &&
	    (tolower((unsigned char)dev[0]) == 'c') &&
	    (tolower((unsigned char)dev[1]) == 'o') &&
	    (tolower((unsigned char)dev[2]) == 'm')) {
		dev2 = malloc(strlen(dev) + 5);
		if (!dev2) {
			msg_perr_strerror("Out of memory: ");
			return SERIALPORT_INV_FD;
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
		return SERIALPORT_INV_FD;
	}
	if (serialport_config(fd, baud) != 0) {
		CloseHandle(fd);
		return SERIALPORT_INV_FD;
	}
	return fd;
}

void serialport_set_pin(serialport_fdtype fd, enum SERIALPORT_PIN pin, int val) {
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
	EscapeCommFunction(fd, ctl);
}

int serialport_get_pin(serialport_fdtype fd, enum SERIALPORT_PIN pin) {
	int s;
	DWORD ctl;

	s = (pin == PIN_CTS) ? MS_CTS_ON : MS_DSR_ON;
	GetCommModemStatus(fd, &ctl);

	return ((ctl & s) ? 1 : 0);

}

void serialport_flush_incoming(serialport_fdtype fd)
{
	PurgeComm(fd, PURGE_RXCLEAR);
}

int serialport_shutdown(serialport_fdtype fd)
{
	CloseHandle(fd);
	return 0;
}

int serialport_raw_read(serialport_fdtype fd, unsigned char *buf, unsigned int readcnt, unsigned int *actual)
{
	DWORD rv;
	if (!ReadFile(fd, buf, readcnt, &rv, NULL)) {
		msg_perr_strerror("Serial port read error: ");
		return 1;
	}
	*actual = rv;
	return 0;
}

int serialport_raw_write(serialport_fdtype fd, const unsigned char *buf, unsigned int writecnt, unsigned int *actual)
{
	DWORD rv;
	if (!WriteFile(fd, buf, writecnt, &rv, NULL)) {
		msg_perr_strerror("Serial port write error: ");
		return 1;
	}
	*actual = rv;
	return 0;
}

static COMMTIMEOUTS saved_timeouts;

int serialport_nonblock_enter(serialport_fdtype fd)
{
	COMMTIMEOUTS timeouts = {
		.ReadIntervalTimeout = MAXDWORD,
		.ReadTotalTimeoutMultiplier = 0,
		.ReadTotalTimeoutConstant = 0,
		.WriteTotalTimeoutMultiplier = 0,
		.WriteTotalTimeoutConstant = 0
	};
	if (!GetCommTimeouts(fd, &saved_timeouts)) {
		msg_perr_strerror("Could not get serial port timeout settings: ");
		return 1;
	}
	if (!SetCommTimeouts(fd, &timeouts)) {
		msg_perr_strerror("Could not set serial port timeout settings: ");
		return 1;
	}
	return 0;
}

int serialport_nonblock_leave(serialport_fdtype fd)
{
	if (!SetCommTimeouts(fd, &saved_timeouts)) {
		msg_perr_strerror("Could not restore serial port timeout settings: ");
		return 1;
	}
	return 0;
}
