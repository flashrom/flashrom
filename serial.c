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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
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
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#endif
#include "flash.h"
#include "programmer.h"

fdtype sp_fd;

void __attribute__((noreturn)) sp_die(char *msg)
{
	perror(msg);
	exit(1);
}

#ifdef _WIN32
struct baudentry {
	DWORD flag;
	unsigned int baud;
};
#define BAUDENTRY(baud) { CBR_##baud, baud },
#else
struct baudentry {
	int flag;
	unsigned int baud;
};
#define BAUDENTRY(baud) { B##baud, baud },
#endif

/* I'd like if the C preprocessor could have directives in macros.
 * See TERMIOS(3) and http://msdn.microsoft.com/en-us/library/windows/desktop/aa363214(v=vs.85).aspx and
 * http://git.kernel.org/?p=linux/kernel/git/torvalds/linux.git;a=blob;f=include/uapi/asm-generic/termbits.h */
static const struct baudentry sp_baudtable[] = {
	BAUDENTRY(9600) /* unconditional default */
#if defined(B19200) || defined(CBR_19200)
	BAUDENTRY(19200)
#endif
#if defined(B38400) || defined(CBR_38400)
	BAUDENTRY(38400)
#endif
#if defined(B57600) || defined(CBR_57600)
	BAUDENTRY(57600)
#endif
#if defined(B115200) || defined(CBR_115200)
	BAUDENTRY(115200)
#endif
#if defined(B230400) || defined(CBR_230400)
	BAUDENTRY(230400)
#endif
#if defined(B460800) || defined(CBR_460800)
	BAUDENTRY(460800)
#endif
#if defined(B500000) || defined(CBR_500000)
	BAUDENTRY(500000)
#endif
#if defined(B576000) || defined(CBR_576000)
	BAUDENTRY(576000)
#endif
#if defined(B921600) || defined(CBR_921600)
	BAUDENTRY(921600)
#endif
#if defined(B1000000) || defined(CBR_1000000)
	BAUDENTRY(1000000)
#endif
#if defined(B1152000) || defined(CBR_1152000)
	BAUDENTRY(1152000)
#endif
#if defined(B1500000) || defined(CBR_1500000)
	BAUDENTRY(1500000)
#endif
#if defined(B2000000) || defined(CBR_2000000)
	BAUDENTRY(2000000)
#endif
#if defined(B2500000) || defined(CBR_2500000)
	BAUDENTRY(2500000)
#endif
#if defined(B3000000) || defined(CBR_3000000)
	BAUDENTRY(3000000)
#endif
#if defined(B3500000) || defined(CBR_3500000)
	BAUDENTRY(3500000)
#endif
#if defined(B4000000) || defined(CBR_4000000)
	BAUDENTRY(4000000)
#endif
	{0, 0}			/* Terminator */
};

const struct baudentry *round_baud(unsigned int baud)
{
	int i;
	/* Round baud rate to next lower entry in sp_baudtable if it exists, else use the lowest entry. */
	for (i = ARRAY_SIZE(sp_baudtable) - 2; i >= 0 ; i--) {
		if (sp_baudtable[i].baud == baud)
			return &sp_baudtable[i];

		if (sp_baudtable[i].baud < baud) {
			msg_pinfo("Warning: given baudrate %d rounded down to %d.\n",
				  baud, sp_baudtable[i].baud);
			return &sp_baudtable[i];
		}
	}
	return &sp_baudtable[0];
}

/* Uses msg_perr to print the last system error.
 * Prints "Error: " followed first by \c msg and then by the description of the last error retrieved via
 * strerror() or FormatMessage() and ending with a linebreak. */
static void msg_perr_strerror(const char *msg)
{
	msg_perr("Error: %s", msg);
#ifdef _WIN32
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

fdtype sp_openserport(char *dev, unsigned int baud)
{
#ifdef _WIN32
	HANDLE fd;
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
	DCB dcb;
	if (!GetCommState(fd, &dcb)) {
		msg_perr_strerror("Could not fetch serial port configuration: ");
		return SER_INV_FD;
	}
	const struct baudentry *entry = round_baud(baud);
	dcb.BaudRate = entry->baud;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	if (!SetCommState(fd, &dcb)) {
		msg_perr_strerror("Could not change serial port configuration: ");
		return SER_INV_FD;
	}
	if (!GetCommState(fd, &dcb)) {
		msg_perr_strerror("Could not fetch serial port configuration: ");
		return SER_INV_FD;
	}
	msg_pdbg("Baud rate is %ld.\n", dcb.BaudRate);
	return fd;
#else
	struct termios options;
	int fd;
	fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0) {
		msg_perr_strerror("Cannot open serial port: ");
		return SER_INV_FD;
	}
	fcntl(fd, F_SETFL, 0);
	tcgetattr(fd, &options);
	const struct baudentry *entry = round_baud(baud);
	cfsetispeed(&options, entry->flag);
	cfsetospeed(&options, entry->flag);
	msg_pdbg("Setting baud rate to %d.\n", entry->baud);
	options.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);
	options.c_cflag |= (CS8 | CLOCAL | CREAD);
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | IGNCR | INLCR);
	options.c_oflag &= ~OPOST;
	tcsetattr(fd, TCSANOW, &options);
	return fd;
#endif
}

void sp_set_pin(enum SP_PIN pin, int val) {
#ifdef _WIN32
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
#ifdef _WIN32
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
#ifdef _WIN32
	PurgeComm(sp_fd, PURGE_RXCLEAR);
#else
	/* FIXME: error handling */
	tcflush(sp_fd, TCIFLUSH);
#endif
	return;
}

int serialport_shutdown(void *data)
{
#ifdef _WIN32
	CloseHandle(sp_fd);
#else
	close(sp_fd);
#endif
	return 0;
}

int serialport_write(unsigned char *buf, unsigned int writecnt)
{
#ifdef _WIN32
	DWORD tmp = 0;
#else
	ssize_t tmp = 0;
#endif
	unsigned int empty_writes = 250; /* results in a ca. 125ms timeout */

	while (writecnt > 0) {
#ifdef _WIN32
		WriteFile(sp_fd, buf, writecnt, &tmp, NULL);
#else
		tmp = write(sp_fd, buf, writecnt);
#endif
		if (tmp == -1) {
			msg_perr("Serial port write error!\n");
			return 1;
		}
		if (!tmp) {
			msg_pdbg2("Empty write\n");
			empty_writes--;
			programmer_delay(500);
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
#ifdef _WIN32
	DWORD tmp = 0;
#else
	ssize_t tmp = 0;
#endif

	while (readcnt > 0) {
#ifdef _WIN32
		ReadFile(sp_fd, buf, readcnt, &tmp, NULL);
#else
		tmp = read(sp_fd, buf, readcnt);
#endif
		if (tmp == -1) {
			msg_perr("Serial port read error!\n");
			return 1;
		}
		if (!tmp)
			msg_pdbg("Empty read\n");
		readcnt -= tmp;
		buf += tmp;
	}

	return 0;
}
