/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Urja Rannikko <urjaman@gmail.com>
 * Copyright (C) 2009 Carl-Daniel Hailfinger
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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <termios.h>
#include "flash.h"

char *serprog_param = NULL;

#define SERPROG_SUPPORT 0
#if SERPROG_SUPPORT == 1
#else
int serprog_init(void)
{
	fprintf(stderr, "Serial programmer support was not compiled in\n");
	exit(1);
}

int serprog_shutdown(void)
{
	fprintf(stderr, "Serial programmer support was not compiled in\n");
	exit(1);
}

void serprog_chip_writeb(uint8_t val, chipaddr addr)
{
	fprintf(stderr, "Serial programmer support was not compiled in\n");
	exit(1);
}

uint8_t serprog_chip_readb(const chipaddr addr)
{
	fprintf(stderr, "Serial programmer support was not compiled in\n");
	exit(1);
}

void serprog_chip_readn(uint8_t *buf, const chipaddr addr, size_t len)
{
	fprintf(stderr, "Serial programmer support was not compiled in\n");
	exit(1);
}

void serprog_delay(int delay)
{
	fprintf(stderr, "Serial programmer support was not compiled in\n");
	exit(1);
}
#endif
