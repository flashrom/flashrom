/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2009, 2011 Urja Rannikko <urjaman@gmail.com>
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

#include "platform.h"

#include <stdio.h>
#if ! IS_WINDOWS /* stuff (presumably) needed for sockets only */
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif
#if IS_WINDOWS
#include <conio.h>
#else
#include <termios.h>
#endif
#include <string.h>
#include <errno.h>
#include "flash.h"
#include "programmer.h"
#include "chipdrivers.h"
#include "serprog.h"

#define MSGHEADER "serprog: "

/*
 * FIXME: This prototype was added to help reduce diffs for the shutdown
 * registration patch, which shifted many lines of code to place
 * serprog_shutdown() before serprog_init(). It should be removed soon.
 */
static int serprog_shutdown(void *data);

static uint16_t sp_device_serbuf_size = 16;
static uint16_t sp_device_opbuf_size = 300;
/* Bitmap of supported commands */
static uint8_t sp_cmdmap[32];

/* sp_prev_was_write used to detect writes with contiguous addresses
	and combine them to write-n's */
static int sp_prev_was_write = 0;
/* sp_write_n_addr used as the starting addr of the currently
	combined write-n operation */
static uint32_t sp_write_n_addr;
/* The maximum length of an write_n operation; 0 = write-n not supported */
static uint32_t sp_max_write_n = 0;
/* The maximum length of a read_n operation; 0 = 2^24 */
static uint32_t sp_max_read_n = 0;

/* A malloc'd buffer for combining the operation's data
	and a counter that tells how much data is there. */
static uint8_t *sp_write_n_buf;
static uint32_t sp_write_n_bytes = 0;

/* sp_streamed_* used for flow control checking */
static int sp_streamed_transmit_ops = 0;
static int sp_streamed_transmit_bytes = 0;

/* sp_opbuf_usage used for counting the amount of
	on-device operation buffer used */
static int sp_opbuf_usage = 0;
/* if true causes sp_docommand to automatically check
	whether the command is supported before doing it */
static int sp_check_avail_automatic = 0;

#if ! IS_WINDOWS
static int sp_opensocket(char *ip, unsigned int port)
{
	int flag = 1;
	struct hostent *hostPtr = NULL;
	union { struct sockaddr_in si; struct sockaddr s; } sp = {};
	int sock;
	msg_pdbg(MSGHEADER "IP %s port %d\n", ip, port);
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		msg_perr("Error: serprog cannot open socket: %s\n", strerror(errno));
		return -1;
	}
	hostPtr = gethostbyname(ip);
	if (NULL == hostPtr) {
		hostPtr = gethostbyaddr(ip, strlen(ip), AF_INET);
		if (NULL == hostPtr) {
			close(sock);
			msg_perr("Error: cannot resolve %s\n", ip);
			return -1;
		}
	}
	sp.si.sin_family = AF_INET;
	sp.si.sin_port = htons(port);
	(void)memcpy(&sp.si.sin_addr, hostPtr->h_addr_list[0], hostPtr->h_length);
	if (connect(sock, &sp.s, sizeof(sp.si)) < 0) {
		close(sock);
		msg_perr("Error: serprog cannot connect: %s\n", strerror(errno));
		return -1;
	}
	/* We are latency limited, and sometimes do write-write-read    *
	 * (write-n) - so enable TCP_NODELAY.				*/
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int))) {
		close(sock);
		msg_perr("Error: serprog cannot set socket options: %s\n", strerror(errno));
		return -1;
	}
	return sock;
}
#endif

/* Synchronize: a bit tricky algorithm that tries to (and in my tests has *
 * always succeeded in) bring the serial protocol to known waiting-for-   *
 * command state - uses nonblocking I/O - rest of the driver uses         *
 * blocking read - TODO: add an alarm() timer for the rest of the app on  *
 * serial operations, though not such a big issue as the first thing to   *
 * do is synchronize (eg. check that device is alive).			  */
static int sp_synchronize(void)
{
	int i;
	unsigned char buf[8];
	/* First sends 8 NOPs, then flushes the return data - should cause *
	 * the device serial parser to get to a sane state, unless if it   *
	 * is waiting for a real long write-n.                             */
	memset(buf, S_CMD_NOP, 8);
	if (serialport_write_nonblock(buf, 8, 1, NULL) != 0) {
		goto err_out;
	}
	/* A second should be enough to get all the answers to the buffer */
	internal_delay(1000 * 1000);
	sp_flush_incoming();

	/* Then try up to 8 times to send syncnop and get the correct special *
	 * return of NAK+ACK. Timing note: up to 10 characters, 10*50ms =     *
	 * up to 500ms per try, 8*0.5s = 4s; +1s (above) = up to 5s sync      *
	 * attempt, ~1s if immediate success.                                 */
	for (i = 0; i < 8; i++) {
		int n;
		unsigned char c = S_CMD_SYNCNOP;
		if (serialport_write_nonblock(&c, 1, 1, NULL) != 0) {
			goto err_out;
		}
		msg_pdbg(".");
		fflush(stdout);
		for (n = 0; n < 10; n++) {
			int ret = serialport_read_nonblock(&c, 1, 50, NULL);
			if (ret < 0)
				goto err_out;
			if (ret > 0 || c != S_NAK)
				continue;
			ret = serialport_read_nonblock(&c, 1, 20, NULL);
			if (ret < 0)
				goto err_out;
			if (ret > 0 || c != S_ACK)
				continue;
			c = S_CMD_SYNCNOP;
			if (serialport_write_nonblock(&c, 1, 1, NULL) != 0) {
				goto err_out;
			}
			ret = serialport_read_nonblock(&c, 1, 500, NULL);
			if (ret < 0)
				goto err_out;
			if (ret > 0 || c != S_NAK)
				break;	/* fail */
			ret = serialport_read_nonblock(&c, 1, 100, NULL);
			if (ret > 0 || ret < 0)
				goto err_out;
			if (c != S_ACK)
				break;	/* fail */
			msg_pdbg("\n");
			return 0;
		}
	}
err_out:
	msg_perr("Error: cannot synchronize protocol - check communications and reset device?\n");
	return 1;
}

static int sp_check_commandavail(uint8_t command)
{
	int byteoffs, bitoffs;
	byteoffs = command / 8;
	bitoffs = command % 8;
	return (sp_cmdmap[byteoffs] & (1 << bitoffs)) ? 1 : 0;
}

static int sp_automatic_cmdcheck(uint8_t cmd)
{
	if ((sp_check_avail_automatic) && (sp_check_commandavail(cmd) == 0)) {
		msg_pdbg("Warning: Automatic command availability check failed "
			 "for cmd 0x%02x - won't execute cmd\n", cmd);
		return 1;
		}
	return 0;
}

static int sp_docommand(uint8_t command, uint32_t parmlen,
			uint8_t *params, uint32_t retlen, void *retparms)
{
	unsigned char c;
	if (sp_automatic_cmdcheck(command))
		return 1;
	if (serialport_write(&command, 1) != 0) {
		msg_perr("Error: cannot write op code: %s\n", strerror(errno));
		return 1;
	}
	if (serialport_write(params, parmlen) != 0) {
		msg_perr("Error: cannot write parameters: %s\n", strerror(errno));
		return 1;
	}
	if (serialport_read(&c, 1) != 0) {
		msg_perr("Error: cannot read from device: %s\n", strerror(errno));
		return 1;
	}
	if (c == S_NAK)
		return 1;
	if (c != S_ACK) {
		msg_perr("Error: invalid response 0x%02X from device\n", c);
		return 1;
	}
	if (retlen) {
		if (serialport_read(retparms, retlen) != 0) {
			msg_perr("Error: cannot read return parameters: %s\n", strerror(errno));
			return 1;
		}
	}
	return 0;
}

static int sp_flush_stream(void)
{
	if (sp_streamed_transmit_ops)
		do {
			unsigned char c;
			if (serialport_read(&c, 1) != 0) {
				msg_perr("Error: cannot read from device (flushing stream)");
				return 1;
			}
			if (c == S_NAK) {
				msg_perr("Error: NAK to a stream buffer operation\n");
				return 1;
			}
			if (c != S_ACK) {
				msg_perr("Error: Invalid reply 0x%02X from device\n", c);
				return 1;
			}
		} while (--sp_streamed_transmit_ops);
	sp_streamed_transmit_ops = 0;
	sp_streamed_transmit_bytes = 0;
	return 0;
}

static int sp_stream_buffer_op(uint8_t cmd, uint32_t parmlen, uint8_t *parms)
{
	uint8_t *sp;
	if (sp_automatic_cmdcheck(cmd))
		return 1;

	sp = malloc(1 + parmlen);
	if (!sp) {
		msg_perr("Error: cannot malloc command buffer\n");
		return 1;
	}
	sp[0] = cmd;
	memcpy(&(sp[1]), parms, parmlen);

	if (sp_streamed_transmit_bytes >= (1 + parmlen + sp_device_serbuf_size)) {
		if (sp_flush_stream() != 0) {
			free(sp);
			return 1;
		}
	}
	if (serialport_write(sp, 1 + parmlen) != 0) {
		msg_perr("Error: cannot write command\n");
		free(sp);
		return 1;
	}
	sp_streamed_transmit_ops += 1;
	sp_streamed_transmit_bytes += 1 + parmlen;

	free(sp);
	return 0;
}

static int serprog_spi_send_command(struct flashctx *flash,
				    unsigned int writecnt, unsigned int readcnt,
				    const unsigned char *writearr,
				    unsigned char *readarr);
static int serprog_spi_read(struct flashctx *flash, uint8_t *buf,
			    unsigned int start, unsigned int len);
static struct spi_master spi_master_serprog = {
	.type		= SPI_CONTROLLER_SERPROG,
	.max_data_read	= MAX_DATA_READ_UNLIMITED,
	.max_data_write	= MAX_DATA_WRITE_UNLIMITED,
	.command	= serprog_spi_send_command,
	.multicommand	= default_spi_send_multicommand,
	.read		= serprog_spi_read,
	.write_256	= default_spi_write_256,
	.write_aai	= default_spi_write_aai,
};

static void serprog_chip_writeb(const struct flashctx *flash, uint8_t val,
				chipaddr addr);
static uint8_t serprog_chip_readb(const struct flashctx *flash,
				  const chipaddr addr);
static void serprog_chip_readn(const struct flashctx *flash, uint8_t *buf,
			       const chipaddr addr, size_t len);
static const struct par_master par_master_serprog = {
		.chip_readb		= serprog_chip_readb,
		.chip_readw		= fallback_chip_readw,
		.chip_readl		= fallback_chip_readl,
		.chip_readn		= serprog_chip_readn,
		.chip_writeb		= serprog_chip_writeb,
		.chip_writew		= fallback_chip_writew,
		.chip_writel		= fallback_chip_writel,
		.chip_writen		= fallback_chip_writen,
};

static enum chipbustype serprog_buses_supported = BUS_NONE;

int serprog_init(void)
{
	uint16_t iface;
	unsigned char pgmname[17];
	unsigned char rbuf[3];
	unsigned char c;
	char *device;
	char *baudport;
	int have_device = 0;

	/* the parameter is either of format "dev=/dev/device:baud" or "ip=ip:port" */
	device = extract_programmer_param("dev");
	if (device && strlen(device)) {
		baudport = strstr(device, ":");
		if (baudport) {
			/* Split device from baudrate. */
			*baudport = '\0';
			baudport++;
		}
		if (!baudport || !strlen(baudport)) {
			msg_perr("Error: No baudrate specified.\n"
				 "Use flashrom -p serprog:dev=/dev/device:baud\n");
			free(device);
			return 1;
		}
		if (strlen(device)) {
			sp_fd = sp_openserport(device, atoi(baudport));
			if (sp_fd == SER_INV_FD) {
				free(device);
				return 1;
			}
			have_device++;
		}
	}
	if (device && !strlen(device)) {
		msg_perr("Error: No device specified.\n"
			 "Use flashrom -p serprog:dev=/dev/device:baud\n");
		free(device);
		return 1;
	}
	free(device);

#if !IS_WINDOWS
	device = extract_programmer_param("ip");
	if (have_device && device) {
		msg_perr("Error: Both host and device specified.\n"
			 "Please use either dev= or ip= but not both.\n");
		free(device);
		return 1;
	}
	if (device && strlen(device)) {
		baudport = strstr(device, ":");
		if (baudport) {
			/* Split host from port. */
			*baudport = '\0';
			baudport++;
		}
		if (!baudport || !strlen(baudport)) {
			msg_perr("Error: No port specified.\n"
				 "Use flashrom -p serprog:ip=ipaddr:port\n");
			free(device);
			return 1;
		}
		if (strlen(device)) {
			sp_fd = sp_opensocket(device, atoi(baudport));
			if (sp_fd < 0) {
				free(device);
				return 1;
			}
			have_device++;
		}
	}
	if (device && !strlen(device)) {
		msg_perr("Error: No host specified.\n"
			 "Use flashrom -p serprog:ip=ipaddr:port\n");
		free(device);
		return 1;
	}
	free(device);

	if (!have_device) {
		msg_perr("Error: Neither host nor device specified.\n"
			 "Use flashrom -p serprog:dev=/dev/device:baud or "
			 "flashrom -p serprog:ip=ipaddr:port\n");
		return 1;
	}
#endif

	if (register_shutdown(serprog_shutdown, NULL))
		return 1;

	msg_pdbg(MSGHEADER "connected - attempting to synchronize\n");

	sp_check_avail_automatic = 0;

	if (sp_synchronize())
		return 1;

	msg_pdbg(MSGHEADER "Synchronized\n");

	if (sp_docommand(S_CMD_Q_IFACE, 0, NULL, 2, &iface)) {
		msg_perr("Error: NAK to query interface version\n");
		return 1;
	}

	if (iface != 1) {
		msg_perr("Error: Unknown interface version: %d\n", iface);
		return 1;
	}

	msg_pdbg(MSGHEADER "Interface version ok.\n");

	if (sp_docommand(S_CMD_Q_CMDMAP, 0, NULL, 32, sp_cmdmap)) {
		msg_perr("Error: query command map not supported\n");
		return 1;
	}

	sp_check_avail_automatic = 1;

	/* FIXME: This assumes that serprog device bustypes are always
	 * identical with flashrom bustype enums and that they all fit
	 * in a single byte.
	 */
	if (sp_docommand(S_CMD_Q_BUSTYPE, 0, NULL, 1, &c)) {
		msg_pwarn("Warning: NAK to query supported buses\n");
		c = BUS_NONSPI;	/* A reasonable default for now. */
	}
	serprog_buses_supported = c;

	msg_pdbg(MSGHEADER "Bus support: parallel=%s, LPC=%s, FWH=%s, SPI=%s\n",
		 (c & BUS_PARALLEL) ? "on" : "off",
		 (c & BUS_LPC) ? "on" : "off",
		 (c & BUS_FWH) ? "on" : "off",
		 (c & BUS_SPI) ? "on" : "off");
	/* Check for the minimum operational set of commands. */
	if (serprog_buses_supported & BUS_SPI) {
		uint8_t bt = BUS_SPI;
		char *spispeed;
		if (sp_check_commandavail(S_CMD_O_SPIOP) == 0) {
			msg_perr("Error: SPI operation not supported while the "
				 "bustype is SPI\n");
			return 1;
		}
		if (sp_docommand(S_CMD_S_BUSTYPE, 1, &bt, 0, NULL))
			return 1;
		/* Success of any of these commands is optional. We don't need
		   the programmer to tell us its limits, but if it doesn't, we
		   will assume stuff, so it's in the programmers best interest
		   to tell us. */
		if (!sp_docommand(S_CMD_Q_WRNMAXLEN, 0, NULL, 3, rbuf)) {
			uint32_t v;
			v = ((unsigned int)(rbuf[0]) << 0);
			v |= ((unsigned int)(rbuf[1]) << 8);
			v |= ((unsigned int)(rbuf[2]) << 16);
			if (v == 0)
				v = (1 << 24) - 1; /* SPI-op maximum. */
			spi_master_serprog.max_data_write = v;
			msg_pdbg(MSGHEADER "Maximum write-n length is %d\n", v);
		}
		if (!sp_docommand(S_CMD_Q_RDNMAXLEN, 0, NULL, 3, rbuf)) {
			uint32_t v;
			v = ((unsigned int)(rbuf[0]) << 0);
			v |= ((unsigned int)(rbuf[1]) << 8);
			v |= ((unsigned int)(rbuf[2]) << 16);
			if (v == 0)
				v = (1 << 24) - 1; /* SPI-op maximum. */
			spi_master_serprog.max_data_read = v;
			msg_pdbg(MSGHEADER "Maximum read-n length is %d\n", v);
		}
		spispeed = extract_programmer_param("spispeed");
		if (spispeed && strlen(spispeed)) {
			uint32_t f_spi_req, f_spi;
			uint8_t buf[4];
			char *f_spi_suffix;

			errno = 0;
			f_spi_req = strtol(spispeed, &f_spi_suffix, 0);
			if (errno != 0 || spispeed == f_spi_suffix) {
				msg_perr("Error: Could not convert 'spispeed'.\n");
				free(spispeed);
				return 1;
			}
			if (strlen(f_spi_suffix) == 1) {
				if (!strcasecmp(f_spi_suffix, "M"))
					f_spi_req *= 1000000;
				else if (!strcasecmp(f_spi_suffix, "k"))
					f_spi_req *= 1000;
				else {
					msg_perr("Error: Garbage following 'spispeed' value.\n");
					free(spispeed);
					return 1;
				}
			} else if (strlen(f_spi_suffix) > 1) {
				msg_perr("Error: Garbage following 'spispeed' value.\n");
				free(spispeed);
				return 1;
			}

			buf[0] = (f_spi_req >> (0 * 8)) & 0xFF;
			buf[1] = (f_spi_req >> (1 * 8)) & 0xFF;
			buf[2] = (f_spi_req >> (2 * 8)) & 0xFF;
			buf[3] = (f_spi_req >> (3 * 8)) & 0xFF;

			if (sp_check_commandavail(S_CMD_S_SPI_FREQ) == 0)
				msg_pwarn(MSGHEADER "Warning: Setting the SPI clock rate is not supported!\n");
			else if (sp_docommand(S_CMD_S_SPI_FREQ, 4, buf, 4, buf) == 0) {
				f_spi = buf[0];
				f_spi |= buf[1] << (1 * 8);
				f_spi |= buf[2] << (2 * 8);
				f_spi |= buf[3] << (3 * 8);
				msg_pdbg(MSGHEADER "Requested to set SPI clock frequency to %u Hz. "
					 "It was actually set to %u Hz\n", f_spi_req, f_spi);
			} else
				msg_pwarn(MSGHEADER "Setting SPI clock rate to %u Hz failed!\n", f_spi_req);
		}
		free(spispeed);
		bt = serprog_buses_supported;
		if (sp_docommand(S_CMD_S_BUSTYPE, 1, &bt, 0, NULL))
			return 1;
	}

	if (serprog_buses_supported & BUS_NONSPI) {
		if (sp_check_commandavail(S_CMD_O_INIT) == 0) {
			msg_perr("Error: Initialize operation buffer "
				 "not supported\n");
			return 1;
		}

		if (sp_check_commandavail(S_CMD_O_DELAY) == 0) {
			msg_perr("Error: Write to opbuf: "
				 "delay not supported\n");
			return 1;
		}

		/* S_CMD_O_EXEC availability checked later. */

		if (sp_check_commandavail(S_CMD_R_BYTE) == 0) {
			msg_perr("Error: Single byte read not supported\n");
			return 1;
		}
		/* This could be translated to single byte reads (if missing),
		 * but now we don't support that. */
		if (sp_check_commandavail(S_CMD_R_NBYTES) == 0) {
			msg_perr("Error: Read n bytes not supported\n");
			return 1;
		}
		if (sp_check_commandavail(S_CMD_O_WRITEB) == 0) {
			msg_perr("Error: Write to opbuf: "
				 "write byte not supported\n");
			return 1;
		}

		if (sp_docommand(S_CMD_Q_WRNMAXLEN, 0, NULL, 3, rbuf)) {
			msg_pdbg(MSGHEADER "Write-n not supported");
			sp_max_write_n = 0;
		} else {
			sp_max_write_n = ((unsigned int)(rbuf[0]) << 0);
			sp_max_write_n |= ((unsigned int)(rbuf[1]) << 8);
			sp_max_write_n |= ((unsigned int)(rbuf[2]) << 16);
			if (!sp_max_write_n) {
				sp_max_write_n = (1 << 24);
			}
			msg_pdbg(MSGHEADER "Maximum write-n length is %d\n",
				 sp_max_write_n);
			sp_write_n_buf = malloc(sp_max_write_n);
			if (!sp_write_n_buf) {
				msg_perr("Error: cannot allocate memory for "
					 "Write-n buffer\n");
				return 1;
			}
			sp_write_n_bytes = 0;
		}

		if (sp_check_commandavail(S_CMD_Q_RDNMAXLEN) &&
		    (sp_docommand(S_CMD_Q_RDNMAXLEN, 0, NULL, 3, rbuf) == 0)) {
			sp_max_read_n = ((unsigned int)(rbuf[0]) << 0);
			sp_max_read_n |= ((unsigned int)(rbuf[1]) << 8);
			sp_max_read_n |= ((unsigned int)(rbuf[2]) << 16);
			msg_pdbg(MSGHEADER "Maximum read-n length is %d\n",
				 sp_max_read_n ? sp_max_read_n : (1 << 24));
		} else {
			msg_pdbg(MSGHEADER "Maximum read-n length "
				 "not reported\n");
			sp_max_read_n = 0;
		}

	}

	if (sp_docommand(S_CMD_Q_PGMNAME, 0, NULL, 16, pgmname)) {
		msg_pwarn("Warning: NAK to query programmer name\n");
		strcpy((char *)pgmname, "(unknown)");
	}
	pgmname[16] = 0;
	msg_pinfo(MSGHEADER "Programmer name is \"%s\"\n", pgmname);

	if (sp_docommand(S_CMD_Q_SERBUF, 0, NULL, 2, &sp_device_serbuf_size)) {
		msg_pwarn("Warning: NAK to query serial buffer size\n");
	}
	msg_pdbg(MSGHEADER "Serial buffer size is %d\n",
		     sp_device_serbuf_size);

	if (sp_check_commandavail(S_CMD_O_INIT)) {
		/* This would be inconsistent. */
		if (sp_check_commandavail(S_CMD_O_EXEC) == 0) {
			msg_perr("Error: Execute operation buffer not "
				 "supported\n");
			return 1;
		}

		if (sp_docommand(S_CMD_O_INIT, 0, NULL, 0, NULL)) {
			msg_perr("Error: NAK to initialize operation buffer\n");
			return 1;
		}

		if (sp_docommand(S_CMD_Q_OPBUF, 0, NULL, 2,
		    &sp_device_opbuf_size)) {
			msg_pwarn("Warning: NAK to query operation buffer size\n");
		}
		msg_pdbg(MSGHEADER "operation buffer size is %d\n",
			 sp_device_opbuf_size);
  	}

	if (sp_check_commandavail(S_CMD_S_PIN_STATE)) {
		uint8_t en = 1;
		if (sp_docommand(S_CMD_S_PIN_STATE, 1, &en, 0, NULL) != 0) {
			msg_perr("Error: could not enable output buffers\n");
			return 1;
		} else
			msg_pdbg(MSGHEADER "Output drivers enabled\n");
	} else
		msg_pdbg(MSGHEADER "Warning: Programmer does not support toggling its output drivers\n");
	sp_prev_was_write = 0;
	sp_streamed_transmit_ops = 0;
	sp_streamed_transmit_bytes = 0;
	sp_opbuf_usage = 0;
	if (serprog_buses_supported & BUS_SPI)
		register_spi_master(&spi_master_serprog);
	if (serprog_buses_supported & BUS_NONSPI)
		register_par_master(&par_master_serprog, serprog_buses_supported & BUS_NONSPI);
	return 0;
}

/* Move an in flashrom buffer existing write-n operation to the on-device operation buffer. */
static int sp_pass_writen(void)
{
	unsigned char header[7];
	msg_pspew(MSGHEADER "Passing write-n bytes=%d addr=0x%x\n", sp_write_n_bytes, sp_write_n_addr);
	if (sp_streamed_transmit_bytes >= (7 + sp_write_n_bytes + sp_device_serbuf_size)) {
		if (sp_flush_stream() != 0) {
			return 1;
		}
	}
	/* In case it's just a single byte send it as a single write. */
	if (sp_write_n_bytes == 1) {
		sp_write_n_bytes = 0;
		header[0] = (sp_write_n_addr >> 0) & 0xFF;
		header[1] = (sp_write_n_addr >> 8) & 0xFF;
		header[2] = (sp_write_n_addr >> 16) & 0xFF;
		header[3] = sp_write_n_buf[0];
		if (sp_stream_buffer_op(S_CMD_O_WRITEB, 4, header) != 0)
			return 1;
		sp_opbuf_usage += 5;
		return 0;
	}
	header[0] = S_CMD_O_WRITEN;
	header[1] = (sp_write_n_bytes >> 0) & 0xFF;
	header[2] = (sp_write_n_bytes >> 8) & 0xFF;
	header[3] = (sp_write_n_bytes >> 16) & 0xFF;
	header[4] = (sp_write_n_addr >> 0) & 0xFF;
	header[5] = (sp_write_n_addr >> 8) & 0xFF;
	header[6] = (sp_write_n_addr >> 16) & 0xFF;
	if (serialport_write(header, 7) != 0) {
		msg_perr(MSGHEADER "Error: cannot write write-n command\n");
		return 1;
	}
	if (serialport_write(sp_write_n_buf, sp_write_n_bytes) != 0) {
		msg_perr(MSGHEADER "Error: cannot write write-n data");
		return 1;
	}
	sp_streamed_transmit_bytes += 7 + sp_write_n_bytes;
	sp_streamed_transmit_ops += 1;
	sp_opbuf_usage += 7 + sp_write_n_bytes;
	sp_write_n_bytes = 0;
	sp_prev_was_write = 0;
	return 0;
}

static int sp_execute_opbuf_noflush(void)
{
	if ((sp_max_write_n) && (sp_write_n_bytes)) {
		if (sp_pass_writen() != 0) {
			msg_perr("Error: could not transfer write buffer\n");
			return 1;
		}
	}
	if (sp_stream_buffer_op(S_CMD_O_EXEC, 0, NULL) != 0) {
		msg_perr("Error: could not execute command buffer\n");
		return 1;
	}
	msg_pspew(MSGHEADER "Executed operation buffer of %d bytes\n", sp_opbuf_usage);
	sp_opbuf_usage = 0;
	sp_prev_was_write = 0;
	return 0;
}

static int sp_execute_opbuf(void)
{
	if (sp_execute_opbuf_noflush() != 0)
		return 1;
	if (sp_flush_stream() != 0)
		return 1;

	return 0;
}

static int serprog_shutdown(void *data)
{
	if ((sp_opbuf_usage) || (sp_max_write_n && sp_write_n_bytes))
	if (sp_execute_opbuf() != 0)
		msg_pwarn("Could not flush command buffer.\n");
	if (sp_check_commandavail(S_CMD_S_PIN_STATE)) {
		uint8_t dis = 0;
		if (sp_docommand(S_CMD_S_PIN_STATE, 1, &dis, 0, NULL) == 0)
			msg_pdbg(MSGHEADER "Output drivers disabled\n");
		else
			msg_pwarn(MSGHEADER "%s: Warning: could not disable output buffers\n", __func__);
	}
	/* FIXME: fix sockets on windows(?), especially closing */
	serialport_shutdown(&sp_fd);
	if (sp_max_write_n)
		free(sp_write_n_buf);
	return 0;
}

static int sp_check_opbuf_usage(int bytes_to_be_added)
{
	if (sp_device_opbuf_size <= (sp_opbuf_usage + bytes_to_be_added)) {
		/* If this happens in the middle of a page load the page load will probably fail. */
		msg_pwarn(MSGHEADER "Warning: executed operation buffer due to size reasons\n");
		if (sp_execute_opbuf() != 0)
			return 1;
	}
	return 0;
}

static void serprog_chip_writeb(const struct flashctx *flash, uint8_t val,
				chipaddr addr)
{
	msg_pspew("%s\n", __func__);
	if (sp_max_write_n) {
		if ((sp_prev_was_write)
		    && (addr == (sp_write_n_addr + sp_write_n_bytes))) {
			sp_write_n_buf[sp_write_n_bytes++] = val;
		} else {
			if ((sp_prev_was_write) && (sp_write_n_bytes))
				sp_pass_writen();
			sp_prev_was_write = 1;
			sp_write_n_addr = addr;
			sp_write_n_bytes = 1;
			sp_write_n_buf[0] = val;
		}
		sp_check_opbuf_usage(7 + sp_write_n_bytes);
		if (sp_write_n_bytes >= sp_max_write_n)
			sp_pass_writen();
	} else {
		/* We will have to do single writeb ops. */
		unsigned char writeb_parm[4];
		sp_check_opbuf_usage(6);
		writeb_parm[0] = (addr >> 0) & 0xFF;
		writeb_parm[1] = (addr >> 8) & 0xFF;
		writeb_parm[2] = (addr >> 16) & 0xFF;
		writeb_parm[3] = val;
		sp_stream_buffer_op(S_CMD_O_WRITEB, 4, writeb_parm); // FIXME: return error
		sp_opbuf_usage += 5;
	}
}

static uint8_t serprog_chip_readb(const struct flashctx *flash,
				  const chipaddr addr)
{
	unsigned char c;
	unsigned char buf[3];
	/* Will stream the read operation - eg. add it to the stream buffer, *
	 * then flush the buffer, then read the read answer.		     */
	if ((sp_opbuf_usage) || (sp_max_write_n && sp_write_n_bytes))
		sp_execute_opbuf_noflush();
	buf[0] = ((addr >> 0) & 0xFF);
	buf[1] = ((addr >> 8) & 0xFF);
	buf[2] = ((addr >> 16) & 0xFF);
	sp_stream_buffer_op(S_CMD_R_BYTE, 3, buf); // FIXME: return error
	sp_flush_stream(); // FIXME: return error
	if (serialport_read(&c, 1) != 0)
		msg_perr(MSGHEADER "readb byteread");  // FIXME: return error
	msg_pspew("%s addr=0x%" PRIxPTR " returning 0x%02X\n", __func__, addr, c);
	return c;
}

/* Local version that really does the job, doesn't care of max_read_n. */
static int sp_do_read_n(uint8_t * buf, const chipaddr addr, size_t len)
{
	unsigned char sbuf[6];
	msg_pspew("%s: addr=0x%" PRIxPTR " len=%zu\n", __func__, addr, len);
	/* Stream the read-n -- as above. */
	if ((sp_opbuf_usage) || (sp_max_write_n && sp_write_n_bytes))
		sp_execute_opbuf_noflush();
	sbuf[0] = ((addr >> 0) & 0xFF);
	sbuf[1] = ((addr >> 8) & 0xFF);
	sbuf[2] = ((addr >> 16) & 0xFF);
	sbuf[3] = ((len >> 0) & 0xFF);
	sbuf[4] = ((len >> 8) & 0xFF);
	sbuf[5] = ((len >> 16) & 0xFF);
	sp_stream_buffer_op(S_CMD_R_NBYTES, 6, sbuf);
	if (sp_flush_stream() != 0)
		return 1;
	if (serialport_read(buf, len) != 0) {
		msg_perr(MSGHEADER "Error: cannot read read-n data");
		return 1;
	}
	return 0;
}

/* The externally called version that makes sure that max_read_n is obeyed. */
static void serprog_chip_readn(const struct flashctx *flash, uint8_t * buf,
			       const chipaddr addr, size_t len)
{
	size_t lenm = len;
	chipaddr addrm = addr;
	while ((sp_max_read_n != 0) && (lenm > sp_max_read_n)) {
		sp_do_read_n(&(buf[addrm-addr]), addrm, sp_max_read_n); // FIXME: return error
		addrm += sp_max_read_n;
		lenm -= sp_max_read_n;
	}
	if (lenm)
		sp_do_read_n(&(buf[addrm-addr]), addrm, lenm); // FIXME: return error
}

void serprog_delay(unsigned int usecs)
{
	unsigned char buf[4];
	msg_pspew("%s usecs=%d\n", __func__, usecs);
	if (!sp_check_commandavail(S_CMD_O_DELAY)) {
		msg_pdbg2("serprog_delay used, but programmer doesn't support delays natively - emulating\n");
		internal_delay(usecs);
		return;
	}
	if ((sp_max_write_n) && (sp_write_n_bytes))
		sp_pass_writen();
	sp_check_opbuf_usage(5);
	buf[0] = ((usecs >> 0) & 0xFF);
	buf[1] = ((usecs >> 8) & 0xFF);
	buf[2] = ((usecs >> 16) & 0xFF);
	buf[3] = ((usecs >> 24) & 0xFF);
	sp_stream_buffer_op(S_CMD_O_DELAY, 4, buf);
	sp_opbuf_usage += 5;
	sp_prev_was_write = 0;
}

static int serprog_spi_send_command(struct flashctx *flash,
				    unsigned int writecnt, unsigned int readcnt,
				    const unsigned char *writearr,
				    unsigned char *readarr)
{
	unsigned char *parmbuf;
	int ret;
	msg_pspew("%s, writecnt=%i, readcnt=%i\n", __func__, writecnt, readcnt);
	if ((sp_opbuf_usage) || (sp_max_write_n && sp_write_n_bytes)) {
		if (sp_execute_opbuf() != 0) {
			msg_perr("Error: could not execute command buffer before sending SPI commands.\n");
			return 1;
		}
	}

	parmbuf = malloc(writecnt + 6);
	if (!parmbuf) {
		msg_perr("Error: could not allocate SPI send param buffer.\n");
		return 1;
	}
	parmbuf[0] = (writecnt >> 0) & 0xFF;
	parmbuf[1] = (writecnt >> 8) & 0xFF;
	parmbuf[2] = (writecnt >> 16) & 0xFF;
	parmbuf[3] = (readcnt >> 0) & 0xFF;
	parmbuf[4] = (readcnt >> 8) & 0xFF;
	parmbuf[5] = (readcnt >> 16) & 0xFF;
	memcpy(parmbuf + 6, writearr, writecnt);
	ret = sp_docommand(S_CMD_O_SPIOP, writecnt + 6, parmbuf, readcnt,
			   readarr);
	free(parmbuf);
	return ret;
}

/* FIXME: This function is optimized so that it does not split each transaction
 * into chip page_size long blocks unnecessarily like spi_read_chunked. This has
 * the advantage that it is much faster for most chips, but breaks those with
 * non-continuous reads. When spi_read_chunked is fixed this method can be removed. */
static int serprog_spi_read(struct flashctx *flash, uint8_t *buf,
			    unsigned int start, unsigned int len)
{
	unsigned int i, cur_len;
	const unsigned int max_read = spi_master_serprog.max_data_read;
	for (i = 0; i < len; i += cur_len) {
		int ret;
		cur_len = min(max_read, (len - i));
		ret = spi_nbyte_read(flash, start + i, buf + i, cur_len);
		if (ret)
			return ret;
	}
	return 0;
}

void *serprog_map(const char *descr, uintptr_t phys_addr, size_t len)
{
	/* Serprog transmits 24 bits only and assumes the underlying implementation handles any remaining bits
	 * correctly (usually setting them to one either in software (for FWH/LPC) or relying on the fact that
	 * the hardware observes a subset of the address bits only). Combined with the standard mapping of
	 * flashrom this creates a 16 MB-wide window just below the 4 GB boundary where serprog can operate (as
	 * needed for non-SPI chips). Below we make sure that the requested range is within this window. */
	if ((phys_addr & 0xFF000000) == 0xFF000000) {
		return (void*)phys_addr;
	} else {
		msg_pwarn(MSGHEADER "requested mapping %s is incompatible: 0x%zx bytes at 0x%0*" PRIxPTR ".\n",
			  descr, len, PRIxPTR_WIDTH, phys_addr);
		return NULL;
	}
}
