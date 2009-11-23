/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Urja Rannikko <urjaman@gmail.com>
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
#include "flash.h"
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <inttypes.h>
#include <termios.h>

int sp_fd;

void __attribute__((noreturn)) sp_die(char *msg)
{
	perror(msg);
	exit(1);
}

struct baudentry {
	int flag;
	unsigned int baud;
};

/* I'd like if the C preprocessor could have directives in macros */
#define BAUDENTRY(baud) { B##baud, baud },
static const struct baudentry sp_baudtable[] = {
	BAUDENTRY(9600)
	BAUDENTRY(19200)
	BAUDENTRY(38400)
	BAUDENTRY(57600)
	BAUDENTRY(115200)
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

int sp_openserport(char *dev, unsigned int baud)
{
	struct termios options;
	int fd, i;
	fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0)
		sp_die("Error: cannot open serial port");
	fcntl(fd, F_SETFL, 0);
	tcgetattr(fd, &options);
	for (i = 0;; i++) {
		if (sp_baudtable[i].baud == 0) {
			close(fd);
			fprintf(stderr,
				"Error: cannot configure for baudrate %d\n",
				baud);
			exit(1);
		}
		if (sp_baudtable[i].baud == baud) {
			cfsetispeed(&options, sp_baudtable[i].flag);
			cfsetospeed(&options, sp_baudtable[i].flag);
			break;
		}
	}
	options.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);
	options.c_cflag |= (CS8 | CLOCAL | CREAD);
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | IGNCR | INLCR);
	options.c_oflag &= ~OPOST;
	tcsetattr(fd, TCSANOW, &options);
	return fd;
}

void sp_flush_incoming(void)
{
	int i;
	for (i=0;i<100;i++) { /* In case the device doesnt do EAGAIN, just read 0 */
		unsigned char flush[16];
		ssize_t rv;
		rv = read(sp_fd, flush, sizeof(flush));
		if ((rv == -1) && (errno == EAGAIN))
			break;
		if (rv == -1)
			sp_die("flush read");
	}
	return;
}
