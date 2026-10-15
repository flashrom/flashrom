/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2009 Urja Rannikko <urjaman@gmail.com>
 * SPDX-FileCopyrightText: 2009,2010 Carl-Daniel Hailfinger
 */

#include "platform/serial.h"

#include "flash.h"
#include "log.h"
#include "platform/custom_baud.h"
#include "platform/string.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

/* There is no way defined by POSIX to use arbitrary baud rates. It only defines some macros that can be used to
 * specify respective baud rates and many implementations extend this list with further macros, cf. TERMIOS(3)
 * and http://git.kernel.org/?p=linux/kernel/git/torvalds/linux.git;a=blob;f=include/uapi/asm-generic/termbits.h
 * The code below creates a mapping in serialport_baudtable between these macros and the numerical baud rates to deal
 * with numerical user input.
 *
 * On Linux there is a non-standard way to use arbitrary baud rates that we use if there is no
 * matching standard rate, see platform/custom_baud.c
 *
 * On Darwin there is also a non-standard ioctl() to set arbitrary baud rates
 * and any above 230400, see platform/custom_baud_darwin.c and
 * https://opensource.apple.com/source/IOSerialFamily/IOSerialFamily-91/tests/IOSerialTestLib.c.auto.html
 */
#define BAUDENTRY(baud) { B##baud, baud },

static const struct baudentry serialport_baudtable[] = {
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
	/* Round baud rate to next lower entry in serialport_baudtable if it exists, else use the lowest entry. */
	for (i = ARRAY_SIZE(serialport_baudtable) - 2; i >= 0 ; i--) {
		if (serialport_baudtable[i].baud == baud)
			return &serialport_baudtable[i];

		if (serialport_baudtable[i].baud < baud) {
			msg_pwarn("Warning: given baudrate %d rounded down to %d.\n",
				  baud, serialport_baudtable[i].baud);
			return &serialport_baudtable[i];
		}
	}
	msg_pinfo("Using slowest possible baudrate: %d.\n", serialport_baudtable[0].baud);
	return &serialport_baudtable[0];
}

/* Uses msg_perr to print the last system error.
 * Prints "Error: " followed first by \c msg and then by the description of the last error retrieved via
 * strerror() and ending with a linebreak. */
static void msg_perr_strerror(const char *msg)
{
	msg_perr("Error: %s", msg);
	msg_perr("%s\n", strerror(errno));
}

int serialport_config(serialport_fdtype fd, int baud)
{
	if (fd == SERIALPORT_INV_FD) {
		msg_perr("%s: File descriptor is invalid.\n", __func__);
		return 1;
	}

	int custom_baud = (baud >= 0 && use_custom_baud(baud, serialport_baudtable));
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
	return 0;
}

serialport_fdtype serialport_openserport(char *dev, int baud)
{
	serialport_fdtype fd;
	fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY); // Use O_NDELAY to ignore DCD state
	if (fd < 0) {
		msg_perr_strerror("Cannot open serial port: ");
		return SERIALPORT_INV_FD;
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
	return SERIALPORT_INV_FD;
}

void serialport_set_pin(serialport_fdtype fd, enum SERIALPORT_PIN pin, int val) {
	int ctl, s;

	if(pin == PIN_TXD) {
		ioctl(fd, val ? TIOCSBRK : TIOCCBRK, 0);
	}
	else {
		s = (pin == PIN_DTR) ? TIOCM_DTR : TIOCM_RTS;
		ioctl(fd, TIOCMGET, &ctl);

		if (val) {
			ctl |= s;
		}
		else {
			ctl &= ~s;
		}
		ioctl(fd, TIOCMSET, &ctl);
	}
}

int serialport_get_pin(serialport_fdtype fd, enum SERIALPORT_PIN pin) {
	int s;
	int ctl;
	s = (pin == PIN_CTS) ? TIOCM_CTS : TIOCM_DSR;
	ioctl(fd, TIOCMGET, &ctl);

	return ((ctl & s) ? 1 : 0);

}

void serialport_flush_incoming(serialport_fdtype fd)
{
	if (!tcflush(fd, TCIFLUSH))
		return;

	if (errno == ENOTTY) { // TCP socket case: fd is not a terminal descriptor - tcflush is not supported
		unsigned char c;
		int ret;

		do {
			ret = serialport_read_nonblock(fd, &c, 1, 1, NULL);
		} while (ret == 0);

		// positive error code indicates no data available immediately - similar to EAGAIN/EWOULDBLOCK
		//   i.e. all buffered data was read
		// negative error code indicates a permanent error
		if (ret < 0)
			msg_perr("Could not flush serial port incoming buffer: read has failed");
	} else { // any other errno indicates an unrecoverable fd state
		msg_perr_strerror("Could not flush serial port incoming buffer: ");
	}
}

int serialport_shutdown(serialport_fdtype fd)
{
	close(fd);
	return 0;
}

int serialport_raw_read(serialport_fdtype fd, unsigned char *buf, unsigned int readcnt, unsigned int *actual)
{
	ssize_t rv = read(fd, buf, readcnt);
	if (rv < 0) {
		/* Having nothing to read is not an error while non-blocking. */
		if (errno == EAGAIN) {
			*actual = 0;
			return 0;
		}
		msg_perr_strerror("Serial port read error: ");
		return 1;
	}
	*actual = rv;
	return 0;
}

int serialport_raw_write(serialport_fdtype fd, const unsigned char *buf, unsigned int writecnt, unsigned int *actual)
{
	ssize_t rv = write(fd, buf, writecnt);
	if (rv < 0) {
		/* Not being able to write right away is not an error while non-blocking. */
		if (errno == EAGAIN) {
			*actual = 0;
			return 0;
		}
		msg_perr_strerror("Serial port write error: ");
		return 1;
	}
	*actual = rv;
	return 0;
}

static int saved_flags;

int serialport_nonblock_enter(serialport_fdtype fd)
{
	saved_flags = fcntl(fd, F_GETFL);
	if (saved_flags == -1) {
		msg_perr_strerror("Could not get serial port mode: ");
		return 1;
	}
	if (fcntl(fd, F_SETFL, saved_flags | O_NONBLOCK) != 0) {
		msg_perr_strerror("Could not set serial port mode to non-blocking: ");
		return 1;
	}
	return 0;
}

int serialport_nonblock_leave(serialport_fdtype fd)
{
	if (fcntl(fd, F_SETFL, saved_flags) != 0) {
		msg_perr_strerror("Could not restore serial port mode to blocking: ");
		return 1;
	}
	return 0;
}
