/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009, 2010, 2011, 2012 Carl-Daniel Hailfinger
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
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "flash.h"
#include "programmer.h"
#include "spi.h"

/* Change this to #define if you want to test without a serial implementation */
#undef FAKE_COMMUNICATION

struct buspirate_spispeeds {
	const char *name;
	const int speed;
};

#ifndef FAKE_COMMUNICATION
static int buspirate_serialport_setup(char *dev)
{
	/* 115200bps, 8 databits, no parity, 1 stopbit */
	sp_fd = sp_openserport(dev, 115200);
 	if (sp_fd == SER_INV_FD)
		return 1;
	return 0;
}
#else
#define buspirate_serialport_setup(...) 0
#define serialport_shutdown(...) 0
#define serialport_write(...) 0
#define serialport_read(...) 0
#define sp_flush_incoming(...) 0
#endif

static unsigned char *bp_commbuf = NULL;
static int bp_commbufsize = 0;

static int buspirate_commbuf_grow(int bufsize)
{
	unsigned char *tmpbuf;

	/* Never shrink. realloc() calls are expensive. */
	if (bufsize <= bp_commbufsize)
		return 0;

	tmpbuf = realloc(bp_commbuf, bufsize);
	if (!tmpbuf) {
		/* Keep the existing buffer because memory is already tight. */
		msg_perr("Out of memory!\n");
		return ERROR_OOM;
	}

	bp_commbuf = tmpbuf;
	bp_commbufsize = bufsize;
	return 0;
}

static int buspirate_sendrecv(unsigned char *buf, unsigned int writecnt,
			      unsigned int readcnt)
{
	int i, ret = 0;

	msg_pspew("%s: write %i, read %i ", __func__, writecnt, readcnt);
	if (!writecnt && !readcnt) {
		msg_perr("Zero length command!\n");
		return 1;
	}
	if (writecnt)
		msg_pspew("Sending");
	for (i = 0; i < writecnt; i++)
		msg_pspew(" 0x%02x", buf[i]);
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
	if (readcnt)
		msg_pspew(", receiving");
	for (i = 0; i < readcnt; i++)
		msg_pspew(" 0x%02x", buf[i]);
	msg_pspew("\n");
	return 0;
}

static int buspirate_wait_for_string(unsigned char *buf, char *key)
{
	unsigned int keylen = strlen(key);
	int ret;

	ret = buspirate_sendrecv(buf, 0, keylen);
	while (!ret) {
		if (!memcmp(buf, key, keylen))
			return 0;
		memmove(buf, buf + 1, keylen - 1);
		ret = buspirate_sendrecv(buf + keylen - 1, 0, 1);
	}
	return ret;
}

static int buspirate_spi_send_command_v1(struct flashctx *flash, unsigned int writecnt, unsigned int readcnt,
					 const unsigned char *writearr, unsigned char *readarr);
static int buspirate_spi_send_command_v2(struct flashctx *flash, unsigned int writecnt, unsigned int readcnt,
					 const unsigned char *writearr, unsigned char *readarr);

static struct spi_master spi_master_buspirate = {
	.type		= SPI_CONTROLLER_BUSPIRATE,
	.max_data_read	= MAX_DATA_UNSPECIFIED,
	.max_data_write	= MAX_DATA_UNSPECIFIED,
	.command	= NULL,
	.multicommand	= default_spi_send_multicommand,
	.read		= default_spi_read,
	.write_256	= default_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

static const struct buspirate_spispeeds spispeeds[] = {
	{"30k",		0x0},
	{"125k",	0x1},
	{"250k",	0x2},
	{"1M",		0x3},
	{"2M",		0x4},
	{"2.6M",	0x5},
	{"4M",		0x6},
	{"8M",		0x7},
	{NULL,		0x0},
};

static int buspirate_spi_shutdown(void *data)
{
	int ret = 0, ret2 = 0;
	/* No need to allocate a buffer here, we know that bp_commbuf is at least DEFAULT_BUFSIZE big. */

	/* Exit raw SPI mode (enter raw bitbang mode) */
	bp_commbuf[0] = 0x00;
	if ((ret = buspirate_sendrecv(bp_commbuf, 1, 0)))
		goto out_shutdown;
	if ((ret = buspirate_wait_for_string(bp_commbuf, "BBIO")))
		goto out_shutdown;
	if ((ret = buspirate_sendrecv(bp_commbuf, 0, 1)))
		goto out_shutdown;
	msg_pdbg("Raw bitbang mode version %c\n", bp_commbuf[0]);
	if (bp_commbuf[0] != '1') {
		msg_perr("Can't handle raw bitbang mode version %c!\n", bp_commbuf[0]);
		ret = 1;
		goto out_shutdown;
	}
	/* Reset Bus Pirate (return to user terminal) */
	bp_commbuf[0] = 0x0f;
	ret = buspirate_sendrecv(bp_commbuf, 1, 0);

out_shutdown:
	/* Shut down serial port communication */
	ret2 = serialport_shutdown(NULL);
	/* Keep the oldest error, it is probably the best indicator. */
	if (ret2 && !ret)
		ret = ret2;
	bp_commbufsize = 0;
	free(bp_commbuf);
	bp_commbuf = NULL;
	if (ret)
		msg_pdbg("Bus Pirate shutdown failed.\n");
	else
		msg_pdbg("Bus Pirate shutdown completed.\n");

	return ret;
}

#define BP_FWVERSION(a,b)	((a) << 8 | (b))

int buspirate_spi_init(void)
{
	char *tmp;
	char *dev;
	int i;
	unsigned int fw_version_major = 0;
	unsigned int fw_version_minor = 0;
	int spispeed = 0x7;
	int ret = 0;
	int pullup = 0;

	dev = extract_programmer_param("dev");
	if (dev && !strlen(dev)) {
		free(dev);
		dev = NULL;
	}
	if (!dev) {
		msg_perr("No serial device given. Use flashrom -p buspirate_spi:dev=/dev/ttyUSB0\n");
		return 1;
	}

	tmp = extract_programmer_param("spispeed");
	if (tmp) {
		for (i = 0; spispeeds[i].name; i++) {
			if (!strncasecmp(spispeeds[i].name, tmp, strlen(spispeeds[i].name))) {
				spispeed = spispeeds[i].speed;
				break;
			}
		}
		if (!spispeeds[i].name)
			msg_perr("Invalid SPI speed, using default.\n");
	}
	free(tmp);

	tmp = extract_programmer_param("pullups");
	if (tmp) {
		if (strcasecmp("on", tmp) == 0)
			pullup = 1;
		else if (strcasecmp("off", tmp) == 0)
			; // ignore
		else
			msg_perr("Invalid pullups state, not using them.\n");
	}
	free(tmp);

	/* Default buffer size is 19: 16 bytes data, 3 bytes control. */
#define DEFAULT_BUFSIZE (16 + 3)
	bp_commbuf = malloc(DEFAULT_BUFSIZE);
	if (!bp_commbuf) {
		bp_commbufsize = 0;
		msg_perr("Out of memory!\n");
		free(dev);
		return ERROR_OOM;
	}
	bp_commbufsize = DEFAULT_BUFSIZE;

	ret = buspirate_serialport_setup(dev);
	free(dev);
	if (ret) {
		bp_commbufsize = 0;
		free(bp_commbuf);
		bp_commbuf = NULL;
		return ret;
	}

	if (register_shutdown(buspirate_spi_shutdown, NULL) != 0) {
		bp_commbufsize = 0;
		free(bp_commbuf);
		bp_commbuf = NULL;
		return 1;
	}

	/* This is the brute force version, but it should work.
	 * It is likely to fail if a previous flashrom run was aborted during a write with the new SPI commands
	 * in firmware v5.5 because that firmware may wait for up to 4096 bytes of input before responding to
	 * 0x00 again. The obvious workaround (sending 4096 bytes of \0) may cause significant startup delays.
	 */
	for (i = 0; i < 20; i++) {
		/* Enter raw bitbang mode */
		bp_commbuf[0] = 0x00;
		/* Send the command, don't read the response. */
		ret = buspirate_sendrecv(bp_commbuf, 1, 0);
		if (ret)
			return ret;
		/* The old way to handle responses from a Bus Pirate already in BBIO mode was to flush any
		 * response which came in over serial. Unfortunately that does not work reliably on Linux
		 * with FTDI USB-serial.
		 */
		//sp_flush_incoming();
		/* The Bus Pirate can't handle UART input buffer overflow in BBIO mode, and sending a sequence
		 * of 0x00 too fast apparently triggers such an UART input buffer overflow.
		 */
		internal_sleep(10000);
	}
	/* We know that 20 commands of \0 should elicit at least one BBIO1 response. */
	if ((ret = buspirate_wait_for_string(bp_commbuf, "BBIO")))
		return ret;

	/* Reset the Bus Pirate. */
	bp_commbuf[0] = 0x0f;
	/* Send the command, don't read the response. */
	if ((ret = buspirate_sendrecv(bp_commbuf, 1, 0)))
		return ret;
	if ((ret = buspirate_wait_for_string(bp_commbuf, "irate ")))
		return ret;
	/* Read the hardware version string. Last byte of the buffer is reserved for \0. */
	for (i = 0; i < DEFAULT_BUFSIZE - 1; i++) {
		if ((ret = buspirate_sendrecv(bp_commbuf + i, 0, 1)))
			return ret;
		if (strchr("\r\n\t ", bp_commbuf[i]))
			break;
	}
	bp_commbuf[i] = '\0';
	msg_pdbg("Detected Bus Pirate hardware %s\n", bp_commbuf);

	if ((ret = buspirate_wait_for_string(bp_commbuf, "irmware ")))
		return ret;
	/* Read the firmware version string. Last byte of the buffer is reserved for \0. */
	for (i = 0; i < DEFAULT_BUFSIZE - 1; i++) {
		if ((ret = buspirate_sendrecv(bp_commbuf + i, 0, 1)))
			return ret;
		if (strchr("\r\n\t ", bp_commbuf[i]))
			break;
	}
	bp_commbuf[i] = '\0';
	msg_pdbg("Detected Bus Pirate firmware ");
	if (bp_commbuf[0] != 'v')
		msg_pdbg("(unknown version number format)");
	else if (!strchr("0123456789", bp_commbuf[1]))
		msg_pdbg("(unknown version number format)");
	else {
		fw_version_major = strtoul((char *)bp_commbuf + 1, &tmp, 10);
		while ((*tmp != '\0') && !strchr("0123456789", *tmp))
			tmp++;
		fw_version_minor = strtoul(tmp, NULL, 10);
		msg_pdbg("%u.%u", fw_version_major, fw_version_minor);
	}
	msg_pdbg2(" (\"%s\")", bp_commbuf);
	msg_pdbg("\n");

	if ((ret = buspirate_wait_for_string(bp_commbuf, "HiZ>")))
		return ret;
	
	/* Tell the user about missing SPI binary mode in firmware 2.3 and older. */
	if (BP_FWVERSION(fw_version_major, fw_version_minor) < BP_FWVERSION(2, 4)) {
		msg_pinfo("Bus Pirate firmware 2.3 and older does not support binary SPI access.\n");
		msg_pinfo("Please upgrade to the latest firmware (at least 2.4).\n");
		return SPI_PROGRAMMER_ERROR;
	}

	/* Use fast SPI mode in firmware 5.5 and newer. */
	if (BP_FWVERSION(fw_version_major, fw_version_minor) >= BP_FWVERSION(5, 5)) {
		msg_pdbg("Using SPI command set v2.\n"); 
		/* Sensible default buffer size. */
		if (buspirate_commbuf_grow(260 + 5))
			return ERROR_OOM;
		spi_master_buspirate.max_data_read = 2048;
		spi_master_buspirate.max_data_write = 256;
		spi_master_buspirate.command = buspirate_spi_send_command_v2;
	} else {
		msg_pinfo("Bus Pirate firmware 5.4 and older does not support fast SPI access.\n");
		msg_pinfo("Reading/writing a flash chip may take hours.\n");
		msg_pinfo("It is recommended to upgrade to firmware 5.5 or newer.\n");
		/* Sensible default buffer size. */
		if (buspirate_commbuf_grow(16 + 3))
			return ERROR_OOM;
		spi_master_buspirate.max_data_read = 12;
		spi_master_buspirate.max_data_write = 12;
		spi_master_buspirate.command = buspirate_spi_send_command_v1;
	}

	/* Workaround for broken speed settings in firmware 6.1 and older. */
	if (BP_FWVERSION(fw_version_major, fw_version_minor) < BP_FWVERSION(6, 2))
		if (spispeed > 0x4) {
			msg_perr("Bus Pirate firmware 6.1 and older does not support SPI speeds above 2 MHz. "
				 "Limiting speed to 2 MHz.\n");
			msg_pinfo("It is recommended to upgrade to firmware 6.2 or newer.\n");
			spispeed = 0x4;
		}
		
	/* This works because speeds numbering starts at 0 and is contiguous. */
	msg_pdbg("SPI speed is %sHz\n", spispeeds[spispeed].name);

	/* Enter raw bitbang mode */
	for (i = 0; i < 20; i++) {
		bp_commbuf[0] = 0x00;
		if ((ret = buspirate_sendrecv(bp_commbuf, 1, 0)))
			return ret;
	}
	if ((ret = buspirate_wait_for_string(bp_commbuf, "BBIO")))
		return ret;
	if ((ret = buspirate_sendrecv(bp_commbuf, 0, 1)))
		return ret;
	msg_pdbg("Raw bitbang mode version %c\n", bp_commbuf[0]);
	if (bp_commbuf[0] != '1') {
		msg_perr("Can't handle raw bitbang mode version %c!\n", bp_commbuf[0]);
		return 1;
	}
	/* Enter raw SPI mode */
	bp_commbuf[0] = 0x01;
	ret = buspirate_sendrecv(bp_commbuf, 1, 0);
	if ((ret = buspirate_wait_for_string(bp_commbuf, "SPI")))
		return ret;
	if ((ret = buspirate_sendrecv(bp_commbuf, 0, 1)))
		return ret;
	msg_pdbg("Raw SPI mode version %c\n", bp_commbuf[0]);
	if (bp_commbuf[0] != '1') {
		msg_perr("Can't handle raw SPI mode version %c!\n", bp_commbuf[0]);
		return 1;
	}

	/* Initial setup (SPI peripherals config): Enable power, CS high, AUX */
	bp_commbuf[0] = 0x40 | 0x0b;
	if (pullup == 1) {
		bp_commbuf[0] |= (1 << 2);
		msg_pdbg("Enabling pull-up resistors.\n");
	}
	ret = buspirate_sendrecv(bp_commbuf, 1, 1);
	if (ret)
		return 1;
	if (bp_commbuf[0] != 0x01) {
		msg_perr("Protocol error while setting power/CS/AUX(/Pull-up resistors)!\n");
		return 1;
	}

	/* Set SPI speed */
	bp_commbuf[0] = 0x60 | spispeed;
	ret = buspirate_sendrecv(bp_commbuf, 1, 1);
	if (ret)
		return 1;
	if (bp_commbuf[0] != 0x01) {
		msg_perr("Protocol error while setting SPI speed!\n");
		return 1;
	}
	
	/* Set SPI config: output type, idle, clock edge, sample */
	bp_commbuf[0] = 0x80 | 0xa;
	ret = buspirate_sendrecv(bp_commbuf, 1, 1);
	if (ret)
		return 1;
	if (bp_commbuf[0] != 0x01) {
		msg_perr("Protocol error while setting SPI config!\n");
		return 1;
	}

	/* De-assert CS# */
	bp_commbuf[0] = 0x03;
	ret = buspirate_sendrecv(bp_commbuf, 1, 1);
	if (ret)
		return 1;
	if (bp_commbuf[0] != 0x01) {
		msg_perr("Protocol error while raising CS#!\n");
		return 1;
	}

	register_spi_master(&spi_master_buspirate);

	return 0;
}

static int buspirate_spi_send_command_v1(struct flashctx *flash, unsigned int writecnt, unsigned int readcnt,
					 const unsigned char *writearr, unsigned char *readarr)
{
	unsigned int i = 0;
	int ret = 0;

	if (writecnt > 16 || readcnt > 16 || (readcnt + writecnt) > 16)
		return SPI_INVALID_LENGTH;

	/* 3 bytes extra for CS#, len, CS#. */
	if (buspirate_commbuf_grow(writecnt + readcnt + 3))
		return ERROR_OOM;

	/* Assert CS# */
	bp_commbuf[i++] = 0x02;

	bp_commbuf[i++] = 0x10 | (writecnt + readcnt - 1);
	memcpy(bp_commbuf + i, writearr, writecnt);
	i += writecnt;
	memset(bp_commbuf + i, 0, readcnt);

	i += readcnt;
	/* De-assert CS# */
	bp_commbuf[i++] = 0x03;

	ret = buspirate_sendrecv(bp_commbuf, i, i);

	if (ret) {
		msg_perr("Bus Pirate communication error!\n");
		return SPI_GENERIC_ERROR;
	}

	if (bp_commbuf[0] != 0x01) {
		msg_perr("Protocol error while lowering CS#!\n");
		return SPI_GENERIC_ERROR;
	}

	if (bp_commbuf[1] != 0x01) {
		msg_perr("Protocol error while reading/writing SPI!\n");
		return SPI_GENERIC_ERROR;
	}

	if (bp_commbuf[i - 1] != 0x01) {
		msg_perr("Protocol error while raising CS#!\n");
		return SPI_GENERIC_ERROR;
	}

	/* Skip CS#, length, writearr. */
	memcpy(readarr, bp_commbuf + 2 + writecnt, readcnt);

	return ret;
}

static int buspirate_spi_send_command_v2(struct flashctx *flash, unsigned int writecnt, unsigned int readcnt,
					 const unsigned char *writearr, unsigned char *readarr)
{
	int i = 0, ret = 0;

	if (writecnt > 4096 || readcnt > 4096 || (readcnt + writecnt) > 4096)
		return SPI_INVALID_LENGTH;

	/* 5 bytes extra for command, writelen, readlen.
	 * 1 byte extra for Ack/Nack.
	 */
	if (buspirate_commbuf_grow(max(writecnt + 5, readcnt + 1)))
		return ERROR_OOM;

	/* Combined SPI write/read. */
	bp_commbuf[i++] = 0x04;
	bp_commbuf[i++] = (writecnt >> 8) & 0xff;
	bp_commbuf[i++] = writecnt & 0xff;
	bp_commbuf[i++] = (readcnt >> 8) & 0xff;
	bp_commbuf[i++] = readcnt & 0xff;
	memcpy(bp_commbuf + i, writearr, writecnt);
	
	ret = buspirate_sendrecv(bp_commbuf, i + writecnt, 1 + readcnt);

	if (ret) {
		msg_perr("Bus Pirate communication error!\n");
		return SPI_GENERIC_ERROR;
	}

	if (bp_commbuf[0] != 0x01) {
		msg_perr("Protocol error while sending SPI write/read!\n");
		return SPI_GENERIC_ERROR;
	}

	/* Skip Ack. */
	memcpy(readarr, bp_commbuf + 1, readcnt);

	return ret;
}
