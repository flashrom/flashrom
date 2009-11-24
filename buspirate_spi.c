/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009 Carl-Daniel Hailfinger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include "flash.h"
#include "spi.h"

/* Change this to #define if you want to test without a serial implementation */
#undef FAKE_COMMUNICATION

#ifndef FAKE_COMMUNICATION
int buspirate_serialport_setup(char *dev)
{
	/* 115200bps, 8 databits, no parity, 1 stopbit */
	sp_fd = sp_openserport(dev, 115200);
	return 0;
}

int buspirate_serialport_shutdown(void)
{
	close(sp_fd);
	return 0;
}

int serialport_write(unsigned char *buf, unsigned int writecnt)
{
	int tmp = 0;

	while (tmp != writecnt) {
		tmp = write(sp_fd, buf + tmp, writecnt - tmp);
		if (tmp == -1)
			return 1;
		if (!tmp)
			printf_debug("Empty write\n");
	}

	return 0;
}

int serialport_read(unsigned char *buf, unsigned int readcnt)
{
	int tmp = 0;

	while (tmp != readcnt) {
		tmp = read(sp_fd, buf + tmp, readcnt - tmp);
		if (tmp == -1)
			return 1;
		if (!tmp)
			printf_debug("Empty read\n");
	}

	return 0;
}

int buspirate_discard_read(void)
{
	int flags;

	printf_debug("%s\n", __func__);
	flags = fcntl(sp_fd, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(sp_fd, F_SETFL, flags);
	sp_flush_incoming();
	flags &= ~O_NONBLOCK;
	fcntl(sp_fd, F_SETFL, flags);

	return 0;
}
#else
#define buspirate_serialport_setup(...) 0
#define buspirate_serialport_shutdown(...) 0
#define serialport_write(...) 0
#define serialport_read(...) 0
#define buspirate_discard_read(...) 0
#endif

int buspirate_sendrecv(unsigned char *buf, unsigned int writecnt, unsigned int readcnt)
{
	int i, ret = 0;

	printf_debug("%s: write %i, read %i\n", __func__, writecnt, readcnt);
	if (!writecnt && !readcnt) {
		fprintf(stderr, "Zero length command!\n");
		return 1;
	}
	printf_debug("Sending");
	for (i = 0; i < writecnt; i++)
		printf_debug(" 0x%02x", buf[i]);
#ifdef FAKE_COMMUNICATION
	/* Placate the caller for now. */
	if (readcnt) {
		buf[0] = 0x01;
		memset(buf + 1, 0xff, readcnt - 1);
	}
	ret = 0;
#else
	if (writecnt)
		ret = serialport_write(buf, writecnt);
	if (ret)
		return ret;
	if (readcnt)
		ret = serialport_read(buf, readcnt);
	if (ret)
		return ret;
#endif
	printf_debug(", receiving");
	for (i = 0; i < readcnt; i++)
		printf_debug(" 0x%02x", buf[i]);
	printf_debug("\n");
	return 0;
}

int buspirate_spi_init(void)
{
	unsigned char buf[512];
	int ret = 0;
	int i;
	char *devpos = NULL;
	char *dev = NULL;
	int devlen;

	if (programmer_param && !strlen(programmer_param)) {
		free(programmer_param);
		programmer_param = NULL;
	}
	if (programmer_param) {
		devpos = strstr(programmer_param, "dev=");
		if (devpos) {
			devpos += 4;
			devlen = strcspn(devpos, ",:");
			if (devlen) {
				dev = malloc(devlen + 1);
				if (!dev) {
					fprintf(stderr, "Out of memory!\n");
					exit(1);
				}
				strncpy(dev, devpos, devlen);
				dev[devlen] = '\0';
			}
		}
		free(programmer_param);
		programmer_param = NULL;
	}
	if (!dev) {
		fprintf(stderr, "No serial device given. Use flashrom -p "
			"buspiratespi:dev=/dev/ttyUSB0\n");
		return 1;
	}

	ret = buspirate_serialport_setup(dev);
	if (ret)
		return ret;

	/* This is the brute force version, but it should work. */
	for (i = 0; i < 19; i++) {
		/* Enter raw bitbang mode */
		buf[0] = 0x00;
		/* Send the command, don't read the response. */
		ret = buspirate_sendrecv(buf, 1, 0);
		if (ret)
			return ret;
		/* Read any response and discard it. */
		ret = buspirate_discard_read();
		if (ret)
			return ret;
	}
	/* Enter raw bitbang mode */
	buf[0] = 0x00;
	ret = buspirate_sendrecv(buf, 1, 5);
	if (ret)
		return ret;
	if (memcmp(buf, "BBIO", 4)) {
		fprintf(stderr, "Entering raw bitbang mode failed!\n");
		return 1;
	}
	printf_debug("Raw bitbang mode version %c\n", buf[4]);
	if (buf[4] != '1') {
		fprintf(stderr, "Can't handle raw bitbang mode version %c!\n",
			buf[4]);
		return 1;
	}
	/* Enter raw SPI mode */
	buf[0] = 0x01;
	ret = buspirate_sendrecv(buf, 1, 4);
	if (memcmp(buf, "SPI", 3)) {
		fprintf(stderr, "Entering raw SPI mode failed!\n");
		return 1;
	}
	printf_debug("Raw SPI mode version %c\n", buf[3]);
	if (buf[3] != '1') {
		fprintf(stderr, "Can't handle raw SPI mode version %c!\n",
			buf[3]);
		return 1;
	}

	/* Initial setup (SPI peripherals config): Enable power, CS high, AUX */
	buf[0] = 0x40 | 0xb;
	ret = buspirate_sendrecv(buf, 1, 1);
	if (ret)
		return 1;
	if (buf[0] != 0x01) {
		fprintf(stderr, "Protocol error while setting power/CS/AUX!\n");
		return 1;
	}

	/* Set speed to 8 MHz */
	buf[0] = 0x60 | 0x7;
	ret = buspirate_sendrecv(buf, 1, 1);
	if (ret)
		return 1;
	if (buf[0] != 0x01) {
		fprintf(stderr, "Protocol error while setting SPI speed!\n");
		return 1;
	}
	
	/* Set SPI config: output type, idle, clock edge, sample */
	buf[0] = 0x80 | 0xa;
	ret = buspirate_sendrecv(buf, 1, 1);
	if (ret)
		return 1;
	if (buf[0] != 0x01) {
		fprintf(stderr, "Protocol error while setting SPI config!\n");
		return 1;
	}

	/* De-assert CS# */
	buf[0] = 0x03;
	ret = buspirate_sendrecv(buf, 1, 1);
	if (ret)
		return 1;
	if (buf[0] != 0x01) {
		fprintf(stderr, "Protocol error while raising CS#!\n");
		return 1;
	}

	buses_supported = CHIP_BUSTYPE_SPI;
	spi_controller = SPI_CONTROLLER_BUSPIRATE;

	return 0;
}

int buspirate_spi_shutdown(void)
{
	unsigned char buf[5];
	int ret = 0;

	/* Exit raw SPI mode (enter raw bitbang mode) */
	buf[0] = 0x00;
	ret = buspirate_sendrecv(buf, 1, 5);
	if (ret)
		return ret;
	if (memcmp(buf, "BBIO", 4)) {
		fprintf(stderr, "Entering raw bitbang mode failed!\n");
		return 1;
	}
	printf_debug("Raw bitbang mode version %c\n", buf[4]);
	if (buf[4] != '1') {
		fprintf(stderr, "Can't handle raw bitbang mode version %c!\n",
			buf[4]);
		return 1;
	}
	/* Reset Bus Pirate (return to user terminal) */
	buf[0] = 0x0f;
	ret = buspirate_sendrecv(buf, 1, 0);
	if (ret)
		return ret;

	/* Shut down serial port communication */
	ret = buspirate_serialport_shutdown();
	if (ret)
		return ret;
	printf_debug("Bus Pirate shutdown completed.\n");

	return 0;
}

int buspirate_spi_send_command(unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr)
{
	static unsigned char *buf = NULL;
	int i = 0, ret = 0;

	if (writecnt > 16 || readcnt > 16 || (readcnt + writecnt) > 16)
		return SPI_INVALID_LENGTH;

	/* +2 is pretty arbitrary. */
	buf = realloc(buf, writecnt + readcnt + 2);
	if (!buf) {
		fprintf(stderr, "Out of memory!\n");
		exit(1); // -1
	}

	/* Assert CS# */
	buf[i++] = 0x02;
	ret = buspirate_sendrecv(buf, 1, 1);
	if (ret)
		return SPI_GENERIC_ERROR;
	if (buf[0] != 0x01) {
		fprintf(stderr, "Protocol error while lowering CS#!\n");
		return SPI_GENERIC_ERROR;
	}

	i = 0;
	buf[i++] = 0x10 | (writecnt + readcnt - 1);
	memcpy(buf + i, writearr, writecnt);
	i += writecnt;
	memset(buf + i, 0, readcnt);
	ret = buspirate_sendrecv(buf, i + readcnt, i + readcnt);
	if (ret)
		return SPI_GENERIC_ERROR;
	if (buf[0] != 0x01) {
		fprintf(stderr, "Protocol error while reading/writing SPI!\n");
		return SPI_GENERIC_ERROR;
	}
	memcpy(readarr, buf + i, readcnt);

	i = 0;
	/* De-assert CS# */
	buf[i++] = 0x03;
	ret = buspirate_sendrecv(buf, 1, 1);
	if (ret)
		return SPI_GENERIC_ERROR;
	if (buf[0] != 0x01) {
		fprintf(stderr, "Protocol error while raising CS#!\n");
		return SPI_GENERIC_ERROR;
	}

	return ret;
}

int buspirate_spi_read(struct flashchip *flash, uint8_t *buf, int start, int len)
{
	/* Maximum read length is 12 bytes, use 8 for now. */
	return spi_read_chunked(flash, buf, start, len, 8);
}

/* We could do 12-byte writes, but for now we use the generic 1-byte code. */
