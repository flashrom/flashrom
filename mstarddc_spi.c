/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2014 Alexandre Boeglin <alex@boeglin.org>
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

#if CONFIG_MSTARDDC_SPI == 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include "flash.h"
#include "programmer.h"
#include "spi.h"

static const struct spi_master spi_master_mstarddc;

static int mstarddc_fd;
static int mstarddc_addr;
static int mstarddc_doreset = 1;

// MSTAR DDC Commands
#define MSTARDDC_SPI_WRITE	0x10
#define MSTARDDC_SPI_READ	0x11
#define MSTARDDC_SPI_END	0x12
#define MSTARDDC_SPI_RESET	0x24

/* Returns 0 upon success, a negative number upon errors. */
static int mstarddc_spi_shutdown(void *data)
{
	// Reset, disables ISP mode
	if (mstarddc_doreset == 1) {
		uint8_t cmd = MSTARDDC_SPI_RESET;
		if (write(mstarddc_fd, &cmd, 1) < 0) {
			msg_perr("Error sending reset command: errno %d.\n",
				 errno);
			return -1;
		}
	} else {
		msg_pinfo("Info: Reset command was not sent. "
			  "Either the noreset=1 option was used, "
			  "or an error occured.\n");
	}

	if (close(mstarddc_fd) < 0) {
		msg_perr("Error closing device: errno %d.\n", errno);
		return -1;
	}
	return 0;
}

/* Returns 0 upon success, a negative number upon errors. */
int mstarddc_spi_init(void)
{
	int ret = 0;

	// Get device, address from command-line
	char *i2c_device = extract_programmer_param("dev");
	if (i2c_device != NULL && strlen(i2c_device) > 0) {
		char *i2c_address = strchr(i2c_device, ':');
		if (i2c_address != NULL) {
			*i2c_address = '\0';
			i2c_address++;
		}
		if (i2c_address == NULL || strlen(i2c_address) == 0) {
			msg_perr("Error: no address specified.\n"
				 "Use flashrom -p mstarddc_spi:dev=/dev/device:address.\n");
			ret = -1;
			goto out;
		}
		mstarddc_addr = strtol(i2c_address, NULL, 16); // FIXME: error handling
	} else {
		msg_perr("Error: no device specified.\n"
			 "Use flashrom -p mstarddc_spi:dev=/dev/device:address.\n");
		ret = -1;
		goto out;
	}
	msg_pinfo("Info: Will try to use device %s and address 0x%02x.\n", i2c_device, mstarddc_addr);

	// Get noreset=1 option from command-line
	char *noreset = extract_programmer_param("noreset");
	if (noreset != NULL && noreset[0] == '1')
		mstarddc_doreset = 0;
	free(noreset);
	msg_pinfo("Info: Will %sreset the device at the end.\n", mstarddc_doreset ? "" : "NOT ");
	// Open device
	if ((mstarddc_fd = open(i2c_device, O_RDWR)) < 0) {
		switch (errno) {
		case EACCES:
			msg_perr("Error opening %s: Permission denied.\n"
				 "Please use sudo or run as root.\n",
				 i2c_device);
			break;
		case ENOENT:
			msg_perr("Error opening %s: No such file.\n"
				 "Please check you specified the correct device.\n",
				 i2c_device);
			break;
		default:
			msg_perr("Error opening %s: %s.\n", i2c_device, strerror(errno));
		}
		ret = -1;
		goto out;
	}
	// Set slave address
	if (ioctl(mstarddc_fd, I2C_SLAVE, mstarddc_addr) < 0) {
		msg_perr("Error setting slave address 0x%02x: errno %d.\n",
			 mstarddc_addr, errno);
		ret = -1;
		goto out;
	}
	// Enable ISP mode
	uint8_t cmd[5] = { 'M', 'S', 'T', 'A', 'R' };
	if (write(mstarddc_fd, cmd, 5) < 0) {
		int enable_err = errno;
		uint8_t end_cmd = MSTARDDC_SPI_END;

		// Assume device is already in ISP mode, try to send END command
		if (write(mstarddc_fd, &end_cmd, 1) < 0) {
			msg_perr("Error enabling ISP mode: errno %d & %d.\n"
				 "Please check that device (%s) and address (0x%02x) are correct.\n",
				 enable_err, errno, i2c_device, mstarddc_addr);
			ret = -1;
			goto out;
		}
	}
	// Register shutdown function
	register_shutdown(mstarddc_spi_shutdown, NULL);

	// Register programmer
	register_spi_master(&spi_master_mstarddc);
out:
	free(i2c_device);
	return ret;
}

/* Returns 0 upon success, a negative number upon errors. */
static int mstarddc_spi_send_command(struct flashctx *flash,
				     unsigned int writecnt,
				     unsigned int readcnt,
				     const unsigned char *writearr,
				     unsigned char *readarr)
{
	int ret = 0;
	uint8_t *cmd = malloc((writecnt + 1) * sizeof(uint8_t));
	if (cmd == NULL) {
		msg_perr("Error allocating memory: errno %d.\n", errno);
		ret = -1;
	}

	if (!ret && writecnt) {
		cmd[0] = MSTARDDC_SPI_WRITE;
		memcpy(cmd + 1, writearr, writecnt);
		if (write(mstarddc_fd, cmd, writecnt + 1) < 0) {
			msg_perr("Error sending write command: errno %d.\n",
				 errno);
			ret = -1;
		}
	}

	if (!ret && readcnt) {
		struct i2c_rdwr_ioctl_data i2c_data;
		struct i2c_msg msg[2];

		cmd[0] = MSTARDDC_SPI_READ;
		i2c_data.nmsgs = 2;
		i2c_data.msgs = msg;
		i2c_data.msgs[0].addr = mstarddc_addr;
		i2c_data.msgs[0].len = 1;
		i2c_data.msgs[0].flags = 0;
		i2c_data.msgs[0].buf = cmd;
		i2c_data.msgs[1].addr = mstarddc_addr;
		i2c_data.msgs[1].len = readcnt;
		i2c_data.msgs[1].flags = I2C_M_RD;
		i2c_data.msgs[1].buf = readarr;

		if (ioctl(mstarddc_fd, I2C_RDWR, &i2c_data) < 0) {
			msg_perr("Error sending read command: errno %d.\n",
				 errno);
			ret = -1;
		}
	}

	if (!ret && (writecnt || readcnt)) {
		cmd[0] = MSTARDDC_SPI_END;
		if (write(mstarddc_fd, cmd, 1) < 0) {
			msg_perr("Error sending end command: errno %d.\n",
				 errno);
			ret = -1;
		}
	}

	/* Do not reset if something went wrong, as it might prevent from
	 * retrying flashing. */
	if (ret != 0)
		mstarddc_doreset = 0;

	if (cmd)
		free(cmd);

	return ret;
}

static const struct spi_master spi_master_mstarddc = {
	.type = SPI_CONTROLLER_MSTARDDC,
	.max_data_read = 256,
	.max_data_write = 256,
	.command = mstarddc_spi_send_command,
	.multicommand = default_spi_send_multicommand,
	.read = default_spi_read,
	.write_256 = default_spi_write_256,
	.write_aai = default_spi_write_aai,
};

#endif
