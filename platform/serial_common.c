/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2009 Urja Rannikko <urjaman@gmail.com>
 * SPDX-FileCopyrightText: 2009,2010 Carl-Daniel Hailfinger
 */

#include "platform/serial.h"

#include "log.h"
#include "platform/udelay.h"

#include <stddef.h>

int serialport_write(serialport_fdtype fd, const unsigned char *buf, unsigned int writecnt)
{
	unsigned int empty_writes = 250; /* results in a ca. 125ms timeout */

	while (writecnt > 0) {
		unsigned int tmp;

		if (serialport_raw_write(fd, buf, writecnt, &tmp))
			return 1;
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

int serialport_read(serialport_fdtype fd, unsigned char *buf, unsigned int readcnt)
{
	while (readcnt > 0) {
		unsigned int tmp;

		if (serialport_raw_read(fd, buf, readcnt, &tmp))
			return 1;
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
int serialport_read_nonblock(serialport_fdtype fd, unsigned char *c, unsigned int readcnt, unsigned int timeout, unsigned int *really_read)
{
	int ret = 1;

	if (serialport_nonblock_enter(fd))
		return -1;

	unsigned int i;
	unsigned int rd_bytes = 0;
	for (i = 0; i < timeout; i++) {
		unsigned int rv;

		msg_pspew("readcnt %u rd_bytes %u\n", readcnt, rd_bytes);
		if (serialport_raw_read(fd, c + rd_bytes, readcnt - rd_bytes, &rv)) {
			ret = -1;
			break;
		}
		msg_pspew("read %u bytes\n", rv);
		rd_bytes += rv;
		if (rd_bytes == readcnt) {
			ret = 0;
			break;
		}
		default_delay(1000);	/* 1ms units */
	}
	if (really_read != NULL)
		*really_read = rd_bytes;

	if (serialport_nonblock_leave(fd))
		ret = -1;

	return ret;
}

/* Tries up to timeout ms to write writecnt characters from the array starting at buf. Returns
 * 0 on success, positive values on temporary errors (e.g. timeouts) and negative ones on permanent errors.
 * If really_wrote is not NULL, this function sets its contents to the number of bytes written successfully. */
int serialport_write_nonblock(serialport_fdtype fd, const unsigned char *buf, unsigned int writecnt, unsigned int timeout, unsigned int *really_wrote)
{
	int ret = 1;

	if (serialport_nonblock_enter(fd))
		return -1;

	unsigned int i;
	unsigned int wr_bytes = 0;
	for (i = 0; i < timeout; i++) {
		unsigned int rv;

		msg_pspew("writecnt %u wr_bytes %u\n", writecnt, wr_bytes);
		if (serialport_raw_write(fd, buf + wr_bytes, writecnt - wr_bytes, &rv)) {
			ret = -1;
			break;
		}
		msg_pspew("wrote %u bytes\n", rv);
		if (rv) {
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

	if (serialport_nonblock_leave(fd))
		ret = -1;

	return ret;
}
