/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * SPDX-FileCopyrightText: 2009, 2010, 2011, 2012 Carl-Daniel Hailfinger
 * SPDX-FileCopyrightText: 2025 Simon Arlott
 */

/* Website: https://spidriver.com/
 * Firmware: https://github.com/jamesbowman/spidriver
 * Protocol: https://github.com/jamesbowman/spidriver/blob/master/protocol.md
 */

#include <assert.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "flash.h"
#include "programmer.h"
#include "spi.h"
#include "platform/udelay.h"
#include "serial.h"

static int spidriver_serialport_setup(char *dev)
{
	/* 460800bps, 8 databits, no parity, 1 stopbit */
	sp_fd = sp_openserport(dev, 460800);
	if (sp_fd == SER_INV_FD)
		return 1;
	return 0;
}

static int spidriver_sendrecv(unsigned char *buf, unsigned int writecnt,
			      unsigned int readcnt)
{
	unsigned int i;
	int ret = 0;

	msg_pspew("%s: write %i, read %i ", __func__, writecnt, readcnt);
	if (!writecnt && !readcnt) {
		msg_perr("Zero length command!\n");
		return 1;
	}
	if (writecnt)
		msg_pspew("Sending");
	for (i = 0; i < writecnt; i++)
		msg_pspew(" 0x%02x", buf[i]);
	if (writecnt)
		ret = serialport_write(buf, writecnt);
	if (ret)
		return ret;
	if (readcnt)
		ret = serialport_read(buf, readcnt);
	if (ret)
		return ret;
	if (readcnt)
		msg_pspew(", receiving");
	for (i = 0; i < readcnt; i++)
		msg_pspew(" 0x%02x", buf[i]);
	msg_pspew("\n");
	return 0;
}

/* Sending multiple commands too quickly usually fails, so use echo to wait for
 * each command to complete before sending the next one.
 */
static int spidriver_send_command(const struct flashctx *flash, unsigned int writecnt,
	unsigned int readcnt, const unsigned char *writearr, unsigned char *readarr)
{
	int ret = 0;

	{
		unsigned char buf[1 + 2];
		unsigned int i = 0;

		/* Assert CS# */
		buf[i++] = 's';

		/* Echo */
		buf[i++] = 'e';
		buf[i++] = 'S';

		if ((ret = spidriver_sendrecv(buf, i, 1))) {
			msg_perr("Communication error after writing %u reading %u\n", writecnt, readcnt);
			return SPI_GENERIC_ERROR;
		}

		if (buf[0] != 'S') {
			msg_perr("Communication error, unexpected select echo response %u\n", buf[0]);
			return SPI_GENERIC_ERROR;
		}
	}

	while (writecnt > 0) {
		unsigned char buf[1 + 64 + 2];
		unsigned int i = 0;
		unsigned int len = writecnt > 64 ? 64 : writecnt;

		/* Write */
		i = 0;
		buf[i++] = 0xc0 - 1 + len;
		memcpy(&buf[i], writearr, len);
		i += len;
		writearr += len;
		writecnt -= len;

		/* Echo */
		buf[i++] = 'e';
		buf[i++] = 'W';

		if ((ret = spidriver_sendrecv(buf, i, 1))) {
			msg_perr("Communication error writing %u\n", len);
			return SPI_GENERIC_ERROR;
		}

		if (buf[0] != 'W') {
			msg_perr("Communication error, unexpected write echo response %u\n", buf[0]);
			return SPI_GENERIC_ERROR;
		}
	}

	while (readcnt > 0) {
		unsigned char buf[1 + 64];
		unsigned int i = 0;
		unsigned int len = readcnt > 64 ? 64 : readcnt;

		/* Read and write */
		i = 0;
		buf[i++] = 0x80 - 1 + len;
		memset(&buf[i], 0, len);
		i += len;

		if ((ret = spidriver_sendrecv(buf, i, len))) {
			msg_perr("Communication error reading %u\n", len);
			return SPI_GENERIC_ERROR;
		}

		memcpy(readarr, buf, len);
		readarr += len;
		readcnt -= len;
	}

	{
		unsigned char buf[1 + 2];
		unsigned int i = 0;

		/* De-assert CS# */
		buf[i++] = 'u';

		/* Echo */
		buf[i++] = 'e';
		buf[i++] = 'U';

		if ((ret = spidriver_sendrecv(buf, i, 1))) {
			msg_perr("Communication error after writing %u reading %u\n", writecnt, readcnt);
			return SPI_GENERIC_ERROR;
		}

		if (buf[0] != 'U') {
			msg_perr("Communication error, unexpected unselect echo response %u\n", buf[0]);
			return SPI_GENERIC_ERROR;
		}
	}

	return ret;
}

static struct spi_master spi_master_spidriver = {
	.features       = SPI_MASTER_4BA,
	.max_data_read  = MAX_DATA_READ_UNLIMITED,
	.max_data_write = MAX_DATA_WRITE_UNLIMITED,
	.command        = spidriver_send_command,
	.read           = default_spi_read,
	.write_256      = default_spi_write_256,
	.shutdown       = serialport_shutdown,
};

static int spidriver_spi_init(const struct programmer_cfg *cfg)
{
	char *tmp;
	char *dev;
	unsigned long fw_version = 0;
	int ret = 0;
	long mode = 0;
	bool a = true;
	bool b = true;
	size_t i;

	dev = extract_programmer_param_str(cfg, "dev");
	if (dev && !strlen(dev)) {
		free(dev);
		dev = NULL;
	}
	if (!dev) {
		msg_perr("No serial device given. Use flashrom -p spidriver:dev=/dev/ttyUSB0\n");
		return 1;
	}

	tmp = extract_programmer_param_str(cfg, "mode");
	if (tmp) {
		mode = strtol(tmp, NULL, 10);
		if (mode < 0 || mode > 3) {
			msg_perr("Error: Invalid SPI mode %ld\nValid values are 0, 1, 2 or 3\n", mode);
			return 1;
		}
	}
	free(tmp);

	tmp = extract_programmer_param_str(cfg, "a");
	if (tmp) {
		if (strcasecmp("high", tmp) == 0) {
			; /* Default */
		} else if (strcasecmp("low", tmp) == 0) {
			a = false;
		} else {
			msg_perr("Error: Invalid A state %s\nValid values are \"high\" or \"low\"\n", tmp);
			return 1;
		}
	}
	free(tmp);

	tmp = extract_programmer_param_str(cfg, "b");
	if (tmp) {
		if (strcasecmp("high", tmp) == 0) {
			; /* Default */
		} else if (strcasecmp("low", tmp) == 0) {
			b = false;
		} else {
			msg_perr("Error: Invalid B state %s\nValid values are \"high\" or \"low\"\n", tmp);
			return 1;
		}
	}
	free(tmp);

	ret = spidriver_serialport_setup(dev);
	free(dev);
	if (ret)
		return ret;

	/* Largest message is: 1 byte command (tx), 80 byte response plus 1 for
	 * string null termination (rx).
	 */
	unsigned char buf[80 + 1];

	/* Flush any in-progress transfer with 64 zero bytes. */
	i = 64;
	memset(buf, 0, i);
	if ((ret = spidriver_sendrecv(buf, i, 0)))
		goto init_err_cleanup_exit;

	default_delay(1400); /* Enough time to receive 64 bytes at 460800bps */
	sp_flush_incoming();

	memset(buf, 0, 81);
	i = 0;
	buf[i++] = '?';
	if ((ret = spidriver_sendrecv(buf, i, 80)))
		goto init_err_cleanup_exit;

	/* [spidriver2 AAAAAAAA 000000002 5.190 000 21.9 1 1 1 ffff 0                     ] */
	/*  <version>  <serial> <uptime>  ^^^^^ ^^^ ^^^^ ^ ^ ^ ^^^^ ^                       */
	/*                      (seconds) |     |   |    | | | |    |                       */
	/*                                |     |   |    | | | |    ` SPI mode (0-3)        */
	/*                                |     |   |    | | | ` CCITT CRC                  */
	/*                                |     |   |    | | ` Chip select                  */
	/*                                |     |   |    | ` "B" signal                     */
	/*                                |     |   |    ` "A" signal                       */
	/*                                |     |   ` Temperature                           */
	/*                                |     ` Current                                   */
	/*                                ` Voltage                                         */
	if (buf[0] != '[' || buf[79] != ']' || !strcmp((char*)&buf[1], "spidriver")) {
		msg_perr("Invalid status response: %s\n", buf);
		ret = 1;
		goto init_err_cleanup_exit;
	}

	msg_pdbg("Status: %s\n", buf);
	msg_pdbg("Detected SPIDriver hardware ");

	if (!strchr("0123456789", buf[10])) {
		msg_pdbg("(unknown version number format)");
	} else {
		fw_version = strtoul((char*)&buf[10], &tmp, 10);
		msg_pdbg("v%lu", fw_version);
	}
	msg_pdbg("\n");

	/* De-assert CS#, configure A and B signals */
	i = 0;
	buf[i++] = 'u';
	buf[i++] = 'a';
	buf[i++] = a ? 1 : 0;
	buf[i++] = 'b';
	buf[i++] = b ? 1 : 0;
	msg_pdbg("Raising CS#\n");
	msg_pdbg("Driving A %s\n", a ? "high" : "low");
	msg_pdbg("Driving B %s\n", b ? "high" : "low");
	if ((ret = spidriver_sendrecv(buf, i, 0)))
		goto init_err_cleanup_exit;

	if (fw_version >= 2) {
		/* Set SPI mode */
		i = 0;
		buf[i++] = 'm';
		buf[i++] = mode & 0xFF;
		if ((ret = spidriver_sendrecv(buf, i, 0)))
			goto init_err_cleanup_exit;
	} else if (mode != 0) {
		msg_perr("Error: SPI mode %ld not supported by version %lu hardware\n", mode, fw_version);
		ret = 1;
		goto init_err_cleanup_exit;
	}

	return register_spi_master(&spi_master_spidriver, NULL);

init_err_cleanup_exit:
	serialport_shutdown(NULL);
	return ret;
}

const struct programmer_entry programmer_spidriver = {
	.name      = "spidriver",
	.type      = OTHER,
	.devs.note = "SPIDriver\n",
	.init      = spidriver_spi_init,
};
