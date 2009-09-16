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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "flash.h"
#include <string.h>
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
#include <inttypes.h>
#include <termios.h>

#define MSGHEADER "serprog:"

#define S_ACK 0x06
#define S_NAK 0x15
#define S_CMD_NOP		0x00	/* No operation                                 */
#define S_CMD_Q_IFACE		0x01	/* Query interface version                      */
#define S_CMD_Q_CMDMAP		0x02	/* Query supported commands bitmap              */
#define S_CMD_Q_PGMNAME		0x03	/* Query programmer name                        */
#define S_CMD_Q_SERBUF		0x04	/* Query Serial Buffer Size                     */
#define S_CMD_Q_BUSTYPE		0x05	/* Query supported bustypes                     */
#define S_CMD_Q_CHIPSIZE	0x06	/* Query supported chipsize (2^n format)        */
#define S_CMD_Q_OPBUF		0x07	/* Query operation buffer size                  */
#define S_CMD_Q_WRNMAXLEN	0x08	/* Query opbuf-write-N maximum lenght           */
#define S_CMD_R_BYTE		0x09	/* Read a single byte                           */
#define S_CMD_R_NBYTES		0x0A	/* Read n bytes                                 */
#define S_CMD_O_INIT		0x0B	/* Initialize operation buffer                  */
#define S_CMD_O_WRITEB		0x0C	/* Write opbuf: Write byte with address         */
#define S_CMD_O_WRITEN		0x0D	/* Write to opbuf: Write-N                      */
#define S_CMD_O_DELAY		0x0E	/* Write opbuf: udelay                          */
#define S_CMD_O_EXEC		0x0F	/* Execute operation buffer                     */
#define S_CMD_SYNCNOP		0x10	/* Special no-operation that returns NAK+ACK    */
#define S_CMD_Q_RDNMAXLEN	0x11	/* Query read-n maximum length			*/
#define S_CMD_S_BUSTYPE		0x12	/* Set used bustype(s).				*/

static int sp_fd;

static uint16_t sp_device_serbuf_size = 16;
static uint16_t sp_device_opbuf_size = 300;
/* Bitmap of supported commands */
static uint8_t sp_cmdmap[32];

/* sp_prev_was_write used to detect writes with continouous addresses
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

static void __attribute__((noreturn)) sp_die(char *msg)
{
	perror(msg);
	exit(1);
}

static int sp_opensocket(char *ip, unsigned int port)
{
	int flag = 1;
	struct hostent *hostPtr = NULL;
	union { struct sockaddr_in si; struct sockaddr s; } sp = {};
	int sock;
	printf_debug(MSGHEADER "IP %s port %d\n", ip, port);
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
		sp_die("Error: serprog cannot open socket");
	hostPtr = gethostbyname(ip);
	if (NULL == hostPtr) {
		hostPtr = gethostbyaddr(ip, strlen(ip), AF_INET);
		if (NULL == hostPtr)
			sp_die("Error: cannot resolve");
	}
	sp.si.sin_family = AF_INET;
	sp.si.sin_port = htons(port);
	(void)memcpy(&sp.si.sin_addr, hostPtr->h_addr, hostPtr->h_length);
	if (connect(sock, &sp.s, sizeof(sp.si)) < 0) {
		close(sock);
		sp_die("Error: serprog cannot connect");
	}
	/* We are latency limited, and sometimes do write-write-read    *
	 * (write-n) - so enable TCP_NODELAY.				*/
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
	return sock;
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

static int sp_openserport(char *dev, unsigned int baud)
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
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_cflag &= ~CRTSCTS;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | IGNCR | INLCR);
	options.c_oflag &= ~OPOST;
	options.c_cflag |= (CLOCAL | CREAD);
	tcsetattr(fd, TCSANOW, &options);
	return fd;
}

static void sp_flush_incoming(void)
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

static int sp_sync_read_timeout(int loops)
{
	int i;
	unsigned char c;
	for (i = 0; i < loops; i++) {
		ssize_t rv;
		rv = read(sp_fd, &c, 1);
		if (rv == 1)
			return c;
		if ((rv == -1) && (errno != EAGAIN))
			sp_die("read");
		usleep(10 * 1000);	/* 10ms units */
	}
	return -1;
}

/* Synchronize: a bit tricky algorhytm that tries to (and in my tests has *
 * always succeeded in) bring the serial protocol to known waiting-for-   *
 * command state - uses nonblocking read - rest of the driver uses	  *
 * blocking read - TODO: add an alarm() timer for the rest of the app on  *
 * serial operations, though not such a big issue as the first thing to   *
 * do is synchronize (eg. check that device is alive).			  */
static void sp_synchronize(void)
{
	int i;
	int flags = fcntl(sp_fd, F_GETFL);
	unsigned char buf[8];
	flags |= O_NONBLOCK;
	fcntl(sp_fd, F_SETFL, flags);
	/* First sends 8 NOPs, then flushes the return data - should cause *
	 * the device serial parser to get to a sane state, unless if it   *
	 * is waiting for a real long write-n.                             */
	memset(buf, S_CMD_NOP, 8);
	if (write(sp_fd, buf, 8) != 8)
		sp_die("flush write");
	/* A second should be enough to get all the answers to the buffer */
	usleep(1000 * 1000);
	sp_flush_incoming();

	/* Then try upto 8 times to send syncnop and get the correct special *
	 * return of NAK+ACK. Timing note: upto 10 characters, 10*50ms =     *
	 * upto 500ms per try, 8*0.5s = 4s; +1s (above) = upto 5s sync       *
	 * attempt, ~1s if immediate success.                                */
	for (i = 0; i < 8; i++) {
		int n;
		unsigned char c = S_CMD_SYNCNOP;
		if (write(sp_fd, &c, 1) != 1)
			sp_die("sync write");
		printf_debug(".");
		fflush(stdout);
		for (n = 0; n < 10; n++) {
			c = sp_sync_read_timeout(5);	/* wait upto 50ms */
			if (c != S_NAK)
				continue;
			c = sp_sync_read_timeout(2);
			if (c != S_ACK)
				continue;
			c = S_CMD_SYNCNOP;
			if (write(sp_fd, &c, 1) != 1)
				sp_die("sync write");
			c = sp_sync_read_timeout(50);
			if (c != S_NAK)
				break;	/* fail */
			c = sp_sync_read_timeout(10);
			if (c != S_ACK)
				break;	/* fail */
			/* Ok, synchronized; back to blocking reads and return. */
			flags &= ~O_NONBLOCK;
			fcntl(sp_fd, F_SETFL, flags);
			printf_debug("\n");
			return;
		}
	}
	fprintf(stderr,
		"Error: cannot synchronize protocol\n"
		"- check communications and reset device?\n");
	exit(1);
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
		printf_debug ("Warning: Automatic command availability check"
				" failed for cmd %d - wont execute cmd\n",cmd);
		return 1;
		}
	return 0;
}

static int sp_docommand(uint8_t command, uint32_t parmlen,
			     uint8_t * params, uint32_t retlen, void *retparms)
{
	unsigned char *sendpacket;
	unsigned char c;
	if (sp_automatic_cmdcheck(command))
		return 1;
	sendpacket = malloc(1 + parmlen);
	if (!sendpacket)
		sp_die("Error: cannot malloc command buffer");
	sendpacket[0] = command;
	memcpy(&(sendpacket[1]), params, parmlen);
	if (write(sp_fd, sendpacket, 1 + parmlen) != (1 + parmlen)) {
		sp_die("Error: cannot write command");
	}
	free(sendpacket);
	if (read(sp_fd, &c, 1) != 1)
		sp_die("Error: cannot read from device");
	if (c == S_NAK) return 1;
	if (c != S_ACK) {
		fprintf(stderr,
			"Error: invalid response 0x%02X from device\n",c);
		exit(1);
	}
	if (retlen) {
		int rd_bytes = 0;
		do {
			int r;
			r = read(sp_fd, retparms + rd_bytes,
				 retlen - rd_bytes);
			if (r <= 0) sp_die
				    ("Error: cannot read return parameters");
			rd_bytes += r;
		} while (rd_bytes != retlen);
	}
	return 0;
}

static void sp_flush_stream(void)
{
	if (sp_streamed_transmit_ops)
		do {
			unsigned char c;
			if (read(sp_fd, &c, 1) != 1) {
				sp_die
				    ("Error: cannot read from device (flushing stream)");
			}
			if (c == S_NAK) {
				fprintf(stderr,
					"Error: NAK to a stream buffer operation\n");
				exit(1);
			}
			if (c != S_ACK) {
				fprintf(stderr,
					"Error: Invalid reply 0x%02X from device\n",
					c);
				exit(1);
			}
		} while (--sp_streamed_transmit_ops);
	sp_streamed_transmit_ops = 0;
	sp_streamed_transmit_bytes = 0;
}

static int sp_stream_buffer_op(uint8_t cmd, uint32_t parmlen, uint8_t * parms)
{
	uint8_t *sp;
	if (sp_automatic_cmdcheck(cmd))
		return 1;
	sp = malloc(1 + parmlen);
	if (!sp) sp_die("Error: cannot malloc command buffer");
	sp[0] = cmd;
	memcpy(&(sp[1]), parms, parmlen);
	if (sp_streamed_transmit_bytes >= (1 + parmlen + sp_device_serbuf_size))
		sp_flush_stream();
	if (write(sp_fd, sp, 1 + parmlen) != (1 + parmlen))
		sp_die("Error: cannot write command");
	free(sp);
	sp_streamed_transmit_ops += 1;
	sp_streamed_transmit_bytes += 1 + parmlen;
	return 0;
}

int serprog_init(void)
{
	uint16_t iface;
	int len;
	unsigned char pgmname[17];
	unsigned char rbuf[3];
	unsigned char c;
	char *num;
	char *dev;
	printf_debug("%s\n", __func__);
	/* the parameter is either of format "/dev/device:baud" or "ip:port" */
	if ((!programmer_param) || (!strlen(programmer_param))) {
		nodevice:
		fprintf(stderr,
			"Error: No device/host given for the serial programmer driver.\n"
			"Use flashrom -p serprog=/dev/device:baud or flashrom -p serprog=ip:port\n");
		exit(1);
	}
	num = strstr(programmer_param, ":");
	len = num - programmer_param;
	if (!len) goto nodevice;
	if (!num) {
		fprintf(stderr,
			"Error: No port or baudrate specified to serial programmer driver.\n"
			"Use flashrom -p serprog=/dev/device:baud or flashrom -p serprog=ip:port\n");
		exit(1);
	}
	len = num - programmer_param;
	dev = malloc(len + 1);
	if (!dev) sp_die("Error: memory allocation failure");
	memcpy(dev, programmer_param, len);
	dev[len] = 0;
	num = strdup(num + 1);
	if (!num) sp_die("Error: memory allocation failure");
	free(programmer_param);
	programmer_param = NULL;

	if (dev[0] == '/') sp_fd = sp_openserport(dev, atoi(num));
	else sp_fd = sp_opensocket(dev, atoi(num));

	free(dev); dev = NULL;
	free(num); num = NULL;

	printf_debug(MSGHEADER "connected - attempting to synchronize\n");

	sp_check_avail_automatic = 0;

	sp_synchronize();

	printf_debug(MSGHEADER "Synchronized\n");

	if (sp_docommand(S_CMD_Q_IFACE, 0, NULL, 2, &iface)) {
		fprintf(stderr, "Error: NAK to Query Interface version\n");
		exit(1);
	}

	if (iface != 1) {
		fprintf(stderr, "Error: Unknown interface version %d\n", iface);
		exit(1);
	}

	printf_debug(MSGHEADER "Interface version ok.\n");

	if (sp_docommand(S_CMD_Q_CMDMAP, 0, NULL, 32, sp_cmdmap)) {
		fprintf(stderr, "Error: query command map not supported\n");
		exit(1);
	}

	sp_check_avail_automatic = 1;

	/* Check for the minimum operational set of commands */
	if (sp_check_commandavail(S_CMD_R_BYTE) == 0) {
		fprintf(stderr, "Error: Single byte read not supported\n");
		exit(1);
	}
	/* This could be translated to single byte reads (if missing),	*
	 * but now we dont support that.				*/
	if (sp_check_commandavail(S_CMD_R_NBYTES) == 0) {
		fprintf(stderr, "Error: Read n bytes not supported\n");
		exit(1);
	}
	/* In the future one could switch to read-only mode if these	*
	 * are not available.						*/
	if (sp_check_commandavail(S_CMD_O_INIT) == 0) {
		fprintf(stderr,
			"Error: Initialize operation buffer not supported\n");
		exit(1);
	}
	if (sp_check_commandavail(S_CMD_O_WRITEB) == 0) {
		fprintf(stderr,
			"Error: Write to opbuf: write byte not supported\n");
		exit(1);
	}
	if (sp_check_commandavail(S_CMD_O_DELAY) == 0) {
		fprintf(stderr, "Error: Write to opbuf: delay not supported\n");
		exit(1);
	}
	if (sp_check_commandavail(S_CMD_O_EXEC) == 0) {
		fprintf(stderr,
			"Error: Execute operation buffer not supported\n");
		exit(1);
	}

	if (sp_docommand(S_CMD_Q_PGMNAME, 0, NULL, 16, pgmname)) {
		fprintf(stderr, "Warning: NAK to query programmer name\n");
		strcpy((char *)pgmname, "(unknown)");
	}
	pgmname[16] = 0;
	printf(MSGHEADER "Programmer name \"%s\"\n", pgmname);

	if (sp_docommand(S_CMD_Q_SERBUF, 0, NULL, 2, &sp_device_serbuf_size)) {
		fprintf(stderr, "Warning: NAK to query serial buffer size\n");
	}
	printf_debug(MSGHEADER "serial buffer size %d\n",
		     sp_device_serbuf_size);

	if (sp_docommand(S_CMD_Q_OPBUF, 0, NULL, 2, &sp_device_opbuf_size)) {
		fprintf(stderr,
			"Warning: NAK to query operation buffer size\n");
	}
	printf_debug(MSGHEADER "operation buffer size %d\n",
		     sp_device_opbuf_size);

	if (sp_docommand(S_CMD_Q_BUSTYPE, 0, NULL, 1, &c)) {
		fprintf(stderr, "Warning: NAK to query supported buses\n");
		c = CHIP_BUSTYPE_NONSPI;	/* A reasonable default for now. */
	}
	buses_supported = c;

	if (sp_docommand(S_CMD_O_INIT, 0, NULL, 0, NULL)) {
		fprintf(stderr, "Error: NAK to initialize operation buffer\n");
		exit(1);
	}

	if (sp_docommand(S_CMD_Q_WRNMAXLEN, 0, NULL, 3, rbuf)) {
		printf_debug(MSGHEADER "Write-n not supported");
		sp_max_write_n = 0;
	} else {
		sp_max_write_n = ((unsigned int)(rbuf[0]) << 0);
		sp_max_write_n |= ((unsigned int)(rbuf[1]) << 8);
		sp_max_write_n |= ((unsigned int)(rbuf[2]) << 16);
		printf_debug(MSGHEADER "Maximum write-n length %d\n",
			     sp_max_write_n);
		sp_write_n_buf = malloc(sp_max_write_n);
		if (!sp_write_n_buf) {
			fprintf(stderr,
				"Error: cannot allocate memory for Write-n buffer\n");
			exit(1);
		}
		sp_write_n_bytes = 0;
	}
	
	if ((sp_check_commandavail(S_CMD_Q_RDNMAXLEN))
		&&((sp_docommand(S_CMD_Q_RDNMAXLEN,0,NULL, 3, rbuf) == 0))) {
		sp_max_read_n = ((unsigned int)(rbuf[0]) << 0);
		sp_max_read_n |= ((unsigned int)(rbuf[1]) << 8);
		sp_max_read_n |= ((unsigned int)(rbuf[2]) << 16);
		printf_debug(MSGHEADER "Maximum read-n length %d\n",
			sp_max_read_n ? sp_max_read_n : (1<<24));
	} else {
		printf_debug(MSGHEADER "Maximum read-n length not reported\n");
		sp_max_read_n = 0;
	}

	sp_prev_was_write = 0;
	sp_streamed_transmit_ops = 0;
	sp_streamed_transmit_bytes = 0;
	sp_opbuf_usage = 0;
	return 0;
}

/* Move an in flashrom buffer existing write-n operation to	*
 * the on-device operation buffer.				*/
static void sp_pass_writen(void)
{
	unsigned char header[7];
	printf_debug(MSGHEADER "Passing write-n bytes=%d addr=0x%x\n",
		     sp_write_n_bytes, sp_write_n_addr);
	if (sp_streamed_transmit_bytes >=
	    (7 + sp_write_n_bytes + sp_device_serbuf_size))
		sp_flush_stream();
	/* In case it's just a single byte send it as a single write. */
	if (sp_write_n_bytes == 1) {
		sp_write_n_bytes = 0;
		header[0] = (sp_write_n_addr >> 0) & 0xFF;
		header[1] = (sp_write_n_addr >> 8) & 0xFF;
		header[2] = (sp_write_n_addr >> 16) & 0xFF;
		header[3] = sp_write_n_buf[0];
		sp_stream_buffer_op(S_CMD_O_WRITEB, 4, header);
		sp_opbuf_usage += 5;
		return;
	}
	header[0] = S_CMD_O_WRITEN;
	header[1] = (sp_write_n_bytes >> 0) & 0xFF;
	header[2] = (sp_write_n_bytes >> 8) & 0xFF;
	header[3] = (sp_write_n_bytes >> 16) & 0xFF;
	header[4] = (sp_write_n_addr >> 0) & 0xFF;
	header[5] = (sp_write_n_addr >> 8) & 0xFF;
	header[6] = (sp_write_n_addr >> 16) & 0xFF;
	if (write(sp_fd, header, 7) != 7)
		sp_die("Error: cannot write write-n command\n");
	if (write(sp_fd, sp_write_n_buf, sp_write_n_bytes) !=
	    sp_write_n_bytes)
		sp_die("Error: cannot write write-n data");
	sp_streamed_transmit_bytes += 7 + sp_write_n_bytes;
	sp_streamed_transmit_ops += 1;
	sp_opbuf_usage += 7 + sp_write_n_bytes;
	sp_write_n_bytes = 0;
	sp_prev_was_write = 0;
}

static void sp_execute_opbuf_noflush(void)
{
	if ((sp_max_write_n) && (sp_write_n_bytes))
		sp_pass_writen();
	sp_stream_buffer_op(S_CMD_O_EXEC, 0, 0);
	printf_debug(MSGHEADER "Executed operation buffer of %d bytes\n",
		     sp_opbuf_usage);
	sp_opbuf_usage = 0;
	sp_prev_was_write = 0;
	return;
}

static void sp_execute_opbuf(void)
{
	sp_execute_opbuf_noflush();
	sp_flush_stream();
}

int serprog_shutdown(void)
{
	printf_debug("%s\n", __func__);
	if ((sp_opbuf_usage) || (sp_max_write_n && sp_write_n_bytes))
		sp_execute_opbuf();
	close(sp_fd);
	if (sp_max_write_n)
		free(sp_write_n_buf);
	return 0;
}

static void sp_check_opbuf_usage(int bytes_to_be_added)
{
	if (sp_device_opbuf_size <= (sp_opbuf_usage + bytes_to_be_added)) {
		sp_execute_opbuf();
		/* If this happens in the mid of an page load the page load *
		 * will propably fail.					    */
		printf_debug(MSGHEADER
		"Warning: executed operation buffer due to size reasons\n");
	}
}

void serprog_chip_writeb(uint8_t val, chipaddr addr)
{
	printf_debug("%s\n", __func__);
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
		sp_stream_buffer_op(S_CMD_O_WRITEB, 4, writeb_parm);
		sp_opbuf_usage += 5;
	}
}

uint8_t serprog_chip_readb(const chipaddr addr)
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
	sp_stream_buffer_op(S_CMD_R_BYTE, 3, buf);
	sp_flush_stream();
	if (read(sp_fd, &c, 1) != 1)
		sp_die("readb byteread");
	printf_debug("%s addr=0x%lx returning 0x%02X\n", __func__, addr, c);
	return c;
}

/* Local version that really does the job, doesnt care of max_read_n. */
static void sp_do_read_n(uint8_t * buf, const chipaddr addr, size_t len)
{
	int rd_bytes = 0;
	unsigned char sbuf[6];
	printf_debug("%s: addr=0x%lx len=%lu\n", __func__, addr, (unsigned long)len);
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
	sp_flush_stream();
	do {
		int r = read(sp_fd, buf + rd_bytes, len - rd_bytes);
		if (r <= 0)
			sp_die("Error: cannot read read-n data");
		rd_bytes += r;
	} while (rd_bytes != len);
	return;
}

/* The externally called version that makes sure that max_read_n is obeyed. */
void serprog_chip_readn(uint8_t * buf, const chipaddr addr, size_t len)
{
	size_t lenm = len;
	chipaddr addrm = addr;
	while ((sp_max_read_n)&&(lenm > sp_max_read_n)) {
		sp_do_read_n(&(buf[addrm-addr]),addrm,sp_max_read_n);
		addrm += sp_max_read_n;
		lenm -= sp_max_read_n;
	}
	if (lenm) sp_do_read_n(&(buf[addrm-addr]),addrm,lenm);
}

void serprog_delay(int delay)
{
	unsigned char buf[4];
	printf_debug("%s\n", __func__);
	if ((sp_max_write_n) && (sp_write_n_bytes))
		sp_pass_writen();
	sp_check_opbuf_usage(5);
	buf[0] = ((delay >> 0) & 0xFF);
	buf[1] = ((delay >> 8) & 0xFF);
	buf[2] = ((delay >> 16) & 0xFF);
	buf[3] = ((delay >> 24) & 0xFF);
	sp_stream_buffer_op(S_CMD_O_DELAY, 4, buf);
	sp_opbuf_usage += 5;
	sp_prev_was_write = 0;
}
